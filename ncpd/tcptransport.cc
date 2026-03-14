/*
 * This file is part of plptools.
 *
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

#include "tcptransport.h"

#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

TCPTransport::TCPTransport(int port)
: port_(port),
  listening_fd_(-1),
  fd_(-1) {
    startListening();  // TODO: Do this on the first run of open.
}

TCPTransport::~TCPTransport() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    if (listening_fd_ >= 0) {
        ::close(listening_fd_);
        listening_fd_ = -1;
    }
}

// TODO: Is this hack really needed?
void TCPTransport::drainRx()
{
    if (fd_ < 0) {
        return;
    }

    char buf[1024];
    while (true) {
        ssize_t n = recv(fd_, buf, sizeof(buf), MSG_DONTWAIT);
        if (n > 0) continue;
        if (n == 0) {
            // peer closed
            ::close(fd_);
            fd_ = -1;
            break;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
        // real error
        ::close(fd_);
        fd_ = -1;
        break;
    }
}

int TCPTransport::startListening() {

    // Create listening socket
    printf("Listening on %d...\n", port_);
    listening_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_fd_ < 0) {
        return -1;
    }

    // Allow reuse of address
    int opt = 1;
    if (setsockopt(listening_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        ::close(listening_fd_);
        listening_fd_ = -1;
        return -1;
    }

    // Bind to port
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listening_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ::close(listening_fd_);
        listening_fd_ = -1;
        return -1;
    }

    printf("Listening on %d...\n", port_);

    // Listen with backlog of 1
    if (listen(listening_fd_, 1) < 0) {
        ::close(listening_fd_);
        listening_fd_ = -1;
        return -1;
    }

}

// TODO: Rename to `accept`.
int TCPTransport::open() {

    // If we're already connected (which we should be), then we drain the read buffer, and hand it back
    // to our code. I'm not 100% sure this is necessary.
    if (fd_ >= 0) {
        drainRx();
        return fd_;
    }

    // Accept one connection (blocks until client connects)
    fd_ = accept(listening_fd_, nullptr, nullptr);
    if (fd_ < 0) {
        ::close(listening_fd_);
        listening_fd_ = -1;
        return -1;
    }

    drainRx();

    return fd_;
}

void TCPTransport::close() {
    // This is a no-op in our current architecture as connections are driven by TCP connection failures, and
    // not by the internal state machine that's driving the serial connection.
    // We should, instead, close the socket at the point that we notice the connection has died in
    // `refreshLink` and handle that state internally such that a subsequent call to `open` will go back into
    // the listening state.
}

bool TCPTransport::isConnected() const {
    return fd_ >= 0;
}

LinkStatus TCPTransport::refreshLink() {

    if (fd_ < 0) {
        return LinkStatus::FATAL;
    }

    return LinkStatus::OK;

    // TODO: Perform subsequent checks and use this to re-enter the listening state.
    // char c;
    // ssize_t n = recv(fd_, &c, 1, MSG_PEEK | MSG_DONTWAIT);
    // if (n == 0)
    //     return LinkStatus::FAILED;
    // if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    //     return LinkStatus::FAILED;
    // return LinkStatus::OK;
}

int TCPTransport::getFd() const {
    return fd_;
}
