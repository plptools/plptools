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

#include <cstring>
#include <iostream>

#include <plpintl.h>
#include <pthread.h>

#include "ncp_log.h"

using namespace std;

static void *linkThread(void *arg) {
    NCPSession *session = (NCPSession *)arg;
    while (!session->isCancelled()) {
        // psion
        session->iow.watch(1, 0, session->cancellationPipe[0]);
        if (session->ncp->hasFailed()) {
            if (session->autoexit) {
                session->cancel();
                break;
            }
            session->iow.watch(5, 0, session->cancellationPipe[0]);
            if (session->isCancelled()) {
                break;
            }
            if (session->nverbose & NCP_SESSION_LOG)
                lout << "ncp: restarting\n";
            session->ncp->reset();
        }
    }
    return NULL;
}

void *pollSocketConnections(void *arg) {
    NCPSession *session = (NCPSession *)arg;
    while (!session->isCancelled()) {
        session->iow.watch(0, 10000, session->cancellationPipe[0]);
        for (int i = 0; i < session->numScp; i++) {
            session->scp[i]->socketPoll();
            if (session->scp[i]->terminate()) {
                // Requested channel termination
                delete session->scp[i];
                session->numScp--;
                for (int j = i; j < session->numScp; j++)
                    session->scp[j] = session->scp[j + 1];
                i--;
            }
        }
    }
    return NULL;
}

void checkForNewSocketConnection(NCPSession *session) {
    string peer;
    if (session->acceptIOW.watch(5, 0, session->cancellationPipe[0]) <= 0) {
        return;
    }
    if (session->isCancelled()) {
        return;
    }
    ppsocket *next = session->skt.accept(&peer, &session->iow);
    if (next != NULL) {
        next->setWatch(&session->iow);
        // New connect
        if (session->nverbose & NCP_SESSION_LOG)
            lout << "New socket connection from " << peer << endl;
        if ((session->numScp >= session->ncp->maxLinks()) || (!session->ncp->gotLinkChannel())) {
            bufferStore a;

            // Give the client time to send its version request.
            next->dataToGet(1, 0);
            next->getBufferStore(a, false);

            a.init();
            a.addStringT("No Psion Connected\n");
            next->sendBufferStore(a);
            delete next;
            if (session->nverbose & NCP_SESSION_LOG)
                lout << "rejected" << endl;
        } else
            session->scp[session->numScp++] = new socketChan(next, session->ncp);
    }
}

void *runNCPSession(void *arg) {
    NCPSession *session = (NCPSession *)arg;

    session->skt.setWatch(&session->acceptIOW);
    if (!session->skt.listen(session->host.c_str(), session->sockNum)) {
        lerr << "listen on " << session->host << ":" << session->sockNum << ": " << strerror(errno) << endl;
        return nullptr;
    }
    linf
        << _("Listening at ") << session->host << ":" << session->sockNum
        << _(" using device ") << session->serialDevice << endl;

    session->ncp = new NCP(session->serialDevice.c_str(),
                           session->baudRate,
                           session->nverbose,
                           session->cancellationPipe[0]);
    pthread_t thr_a, thr_b;
    if (pthread_create(&thr_a, NULL, linkThread, session) != 0) {
        lerr << "Could not create Link thread" << endl;
        exit(-1);
    }
    if (pthread_create(&thr_b, NULL, pollSocketConnections, session) != 0) {
        lerr << "Could not create Socket thread" << endl;
        exit(-1);
    }
    while (!session->isCancelled())
        checkForNewSocketConnection(session);
    linf << _("terminating") << endl;
    void *ret;
    pthread_join(thr_a, &ret);
    linf << _("joined Link thread") << endl;
    pthread_join(thr_b, &ret);
    linf << _("joined Socket thread") << endl;
    delete session->ncp;
    linf << _("shut down NCP") << endl;
    session->skt.closeSocket();
    linf << _("socket closed") << endl;

    return nullptr;
}

NCPSession::~NCPSession() {
    close(cancellationPipe[0]);
    close(cancellationPipe[1]);
    cancellationPipe[0] = -1;
    cancellationPipe[1] = -1;
}

int NCPSession::start() {
    int result = pipe(cancellationPipe);
    if (result != 0) {
        return result;
    }
    return pthread_create(&threadId_, NULL, runNCPSession, this);
}

void NCPSession::cancel() {
    char b = 0;
    write(cancellationPipe[1], &b, 1);
}

bool NCPSession::isCancelled() {
    linf << _("check cancelled");
    if (cancellationPipe[0] == -1) {
        return false;
    }
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(cancellationPipe[0], &fds);
    struct timeval t = {0, 0};
    return select(cancellationPipe[0] + 1, &fds, NULL, NULL, &t) > 0;
}

void NCPSession::wait() {
    pthread_join(threadId_, 0);
}
