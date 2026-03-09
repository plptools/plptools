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
#include "ncp_session.h"

#include <cassert>
#include <cstring>
#include <iostream>

#include <plpintl.h>
#include <pthread.h>

#include "ncp_log.h"

using namespace std;

/**
* @todo There's something really nuanced going on with the use of the @ref NCPSession::socketChannelWatch_ here.
* Specifically, while it might look like it's just being used as a timeout (which _might_ be the intent), new TCP
* sockets are added to it on a successful accept, meaning that this will wake up frequently whenever there's activity on
* the socket. This will cause frequent @ref NCP::hasFailed checks, and very timely resets (@ref NCP::reset) whenever a
* client is connected. It's possible this was introduced as a work-around to connectivity issues. It is also worth
* noting that @ref IOWatch is _not_ thread-safe, so using it in @ref link_thread, and adding to it in
* @ref ncp_session_main_thread (as we are doing) is definitely a bad thing.
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
*
* @todo This thread mutates both @ref NCPSession::socketChannelWatch_ and @ref NCPSession::socketChannels_, neither of
* which are thread-safe and are both accessed and mutated by @ref ncp_session_main_thread.
*/
void *socket_connection_polling_thread(void *arg) {
    NCPSession *session = (NCPSession *)arg;
    while (!session->isCancelled()) {
        session->socketChannelWatch_.watch(0, 10000);
        for (int i = 0; i < session->socketChannelCount_; i++) {
            session->socketChannels_[i]->socketPoll();
            if (session->socketChannels_[i]->terminate()) {
                // Requested channel termination
                delete session->socketChannels_[i];
                session->socketChannelCount_--;
                for (int j = i; j < session->socketChannelCount_; j++)
                    session->socketChannels_[j] = session->socketChannels_[j + 1];
                i--;
            }
        }
    }
    return NULL;
}

void check_for_new_socket_connection(NCPSession *session) {
    string peer;

    // This watch returns false in the case of a time out or cancellation due to a signal, and true in the case of
    // cancellation or one of the file descriptors becoming readable. In the cause a timeout or interrupt, we return
    // flow of control to our calling code.
    if (!session->connectionListenerWatch_.watch(5, 0) || session->isCancelled()) {
        return;
    }
    TCPSocket *next = session->skt_.accept(&peer, &session->socketChannelWatch_);
    if (next != NULL) {
        next->setWatch(&session->socketChannelWatch_);
        // New connect
        if (session->nverbose_ & NCP_SESSION_LOG)
            lout << "New socket connection from " << peer << endl;
        if ((session->socketChannelCount_ >= session->ncp_->maxLinks()) || (!session->ncp_->gotLinkChannel())) {
            bufferStore a;

            // Give the client time to send its version request.
            next->dataToGet(1, 0);
            next->getBufferStore(a, false);

            a.init();
            a.addStringT("No Psion Connected\n");
            next->sendBufferStore(a);
            delete next;
            if (session->nverbose_ & NCP_SESSION_LOG)
                lout << "rejected" << endl;
        } else
            session->socketChannels_[session->socketChannelCount_++] = new SocketChannel(next, session->ncp_);
    }
}

void *ncp_session_main_thread(void *arg) {
    NCPSession *session = (NCPSession *)arg;

    session->skt_.setWatch(&session->connectionListenerWatch_);
    if (!session->skt_.listen(session->host_.c_str(), session->portNumber_)) {
        lerr << "listen on " << session->host_ << ":" << session->portNumber_ << ": " << strerror(errno) << endl;
        return nullptr;
    }
    linf
        << _("Listening at ") << session->host_ << ":" << session->portNumber_
        << _(" using device ") << session->serialDevice_ << endl;

    session->ncp_ = new NCP(session->serialDevice_.c_str(),
                           session->baudRate_,
                           session->nverbose_,
                           session->cancellationPipe_[0]);
    pthread_t thr_a, thr_b;
    if (pthread_create(&thr_a, NULL, link_thread, session) != 0) {
        lerr << "Could not create Link thread" << endl;
        exit(-1);
    }
    if (pthread_create(&thr_b, NULL, socket_connection_polling_thread, session) != 0) {
        lerr << "Could not create Socket thread" << endl;
        exit(-1);
    }
    while (!session->isCancelled())
        check_for_new_socket_connection(session);
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
    connectionListenerWatch_.addIO(cancellationPipe_[0]);
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
