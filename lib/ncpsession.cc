/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */
#include "config.h"
#include "ncpsession.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>

#include <plpintl.h>
#include <pthread.h>

#include "ncp_log.h"
#include "socketchannel.h"

using namespace std;

/**
* @todo There's something really nuanced going on with the use of the @ref NCPSession::socketChannelWatch_ here.
* Specifically, while it might look like it's just being used as a timeout (which _might_ be the intent), new TCP
* sockets are added to it on a successful accept, meaning that this will wake up frequently whenever there's activity on
* the socket. This will cause frequent @ref NCP::hasFailed checks, and very timely resets (@ref NCP::reset) whenever a
* client is connected. It's possible this was introduced as a work-around to connectivity issues. It is also worth
* noting that @ref IOWatch is _not_ thread-safe, so using it in @ref link_thread, and adding to it in
* @ref ncp_session_main_thread (as we are doing) is definitely a bad thing.
*
* The @ref NCP::reset call here is currently required (even though it feels like it shouldn't be) as it's responsible
* for preparing the stack after a successful connection has ended (@ref DataLink currently has responsibility for
* performing internal resets when auto-detecting baud rate).
*/
void *link_thread(void *arg) {
    NCPSession *session = (NCPSession *)arg;
    while (!session->isCancelled()) {
        // psion
        session->socketChannelWatch_.watch(1, 0);
        if (session->ncp_->hasFailed()) {
            if (session->autoexit_) {
                session->cancel();
                break;
            }
            session->socketChannelWatch_.watch(5, 0);
            if (session->isCancelled()) {
                break;
            }
            if (session->nverbose_ & NCP_SESSION_LOG)
                lout << "ncp: restarting\n";
            session->ncp_->reset();
        }
    }
    return NULL;
}

/**
* Responsible for driving the @ref SocketChannel instances (incoming TCP connections) by means of
* @ref SocketChannel::socketPoll. This isn't likely to scale particularly well as it polls _all_ connected sockets
* whenever a single one wakes up, but it seems to work (as we never have that many connected clients).
*/
void *socket_connection_polling_thread(void *arg) {
    NCPSession *session = (NCPSession *)arg;
    while (true) {

        // Wait for events on our sockets, or cancellation.
        session->socketChannelWatch_.watch(0, 10000);
        if (session->isCancelled()) {
            break;
        }

        // Poll all the socket channels to drive things forwards.
        // We take a copy of the socket channels array while holding a lock as this can be mutated on other threads. We
        // know that it's safe to operate on a copy and that nothing will get cleaned up under our feet as we're also
        // responsible for clean up (see later).
        {
            std::vector<SocketChannel *> socketChannels;
            {
                std::lock_guard<std::mutex> lock(session->socketChannelLock_);
                socketChannels = session->socketChannels_;
            }
            for (auto &socketChannel : socketChannels) {
                socketChannel->socketPoll();
            }
        }

        // Clean up the terminated sockets while holding a lock.
        {
            std::lock_guard<std::mutex> lock(session->socketChannelLock_);
            session->socketChannels_.erase(std::remove_if(
                session->socketChannels_.begin(),
                session->socketChannels_.end(),
                [](SocketChannel *socketChannel) {
                    if (!socketChannel->shouldTerminate()) {
                        return false;
                    }
                    delete socketChannel;
                    return true;
                }),
                session->socketChannels_.end());
        }

    }
    return nullptr;
}

void check_for_new_socket_connection(NCPSession *session) {

    // Accept the incoming socket.
    string peer;
    TCPSocket *next = session->skt_.accept(&peer, session->cancellationPipe_[0]);
    if (!next) {
        // NULL here can indicate an error, or cancellation, so we return control to our calling code to allow it to
        // decide what to do.
        return;
    }
    if (session->nverbose_ & NCP_SESSION_LOG) {
        lout << "New socket connection from " << peer << endl;
    }

    // Check to see if we can service the incoming socket.
    // We don't perform socket rejection while holding the lock since it can take some time.
    bool didAddSocket = false;
    {
        std::lock_guard<std::mutex> lock(session->socketChannelLock_);
        if ((session->socketChannels_.size() < session->ncp_->maxLinks()) && (session->ncp_->gotLinkChannel())) {
            session->socketChannels_.push_back(new SocketChannel(next, session->ncp_));
            didAddSocket = true;
        }
    }

    // If we accepted the socket, start watching it and return.
    if (didAddSocket) {
        next->setWatch(&session->socketChannelWatch_);
        return;
    }

    // If we weren't able to accept the socket, then we need to clean it up.

    BufferStore a;

    // Give the client time to send its version request.
    next->dataToGet(1, 0);
    next->getBufferStore(a, false);

    a.init();
    a.addStringT("No Psion Connected\n");
    next->sendBufferStore(a);
    delete next;
    if (session->nverbose_ & NCP_SESSION_LOG) {
        lout << "rejected" << endl;
    }
}

void *ncp_session_main_thread(void *arg) {
    NCPSession *session = (NCPSession *)arg;

    if (!session->skt_.listen(session->host_.c_str(), session->portNumber_)) {
        lerr << "listen on " << session->host_ << ":" << session->portNumber_ << ": " << strerror(errno) << endl;
        return nullptr;
    }
    linf
        << _("Listening at ") << session->host_ << ":" << session->portNumber_
        << _(" using device ") << session->serialDevice_ << endl;

    session->ncp_ = new NCP(session->serialDevice_.c_str(),
                           session->baudRate_,
                           session->noDSRCheck_,
                           session->nverbose_,
                           session->cancellationPipe_[0],
                           session->statusCallback_,
                           session->callbackContext_);
    pthread_t thr_a, thr_b;
    if (pthread_create(&thr_a, NULL, link_thread, session) != 0) {
        lerr << "Could not create Link thread" << endl;
        exit(-1);
    }
    if (pthread_create(&thr_b, NULL, socket_connection_polling_thread, session) != 0) {
        lerr << "Could not create Socket thread" << endl;
        exit(-1);
    }
    while (!session->isCancelled()) {
        check_for_new_socket_connection(session);
    }
    if (session->statusCallback_) {
        session->statusCallback_(session->callbackContext_, false, 0);
    }
    linf << _("terminating") << endl;
    void *ret;
    pthread_join(thr_a, &ret);
    linf << _("joined Link thread") << endl;
    pthread_join(thr_b, &ret);
    linf << _("joined Socket thread") << endl;
    delete session->ncp_;
    linf << _("shut down NCP") << endl;
    session->skt_.closeSocket();
    linf << _("socket closed") << endl;

    return nullptr;
}

NCPSession::~NCPSession() {
    close(cancellationPipe_[0]);
    close(cancellationPipe_[1]);
    cancellationPipe_[0] = -1;
    cancellationPipe_[1] = -1;
}

int NCPSession::start() {
    assert(sessionMainThreadId_ == 0);
    int result = pipe(cancellationPipe_);
    if (result != 0) {
        return result;
    }
    socketChannelWatch_.addIO(cancellationPipe_[0]);
    return pthread_create(&sessionMainThreadId_, NULL, ncp_session_main_thread, this);
}

void NCPSession::cancel() {
    char b = 0;
    write(cancellationPipe_[1], &b, 1);
}

bool NCPSession::isCancelled() {
    if (cancellationPipe_[0] == -1) {
        return false;
    }
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(cancellationPipe_[0], &fds);
    struct timeval t = {0, 0};
    return select(cancellationPipe_[0] + 1, &fds, NULL, NULL, &t) > 0;
}

void NCPSession::wait() {
    pthread_join(sessionMainThreadId_, 0);
}
