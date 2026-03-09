/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
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
#pragma once

#include "config.h"
#include "channel.h"

class TCPSocket;

class SocketChannel : public channel {
public:
  SocketChannel(TCPSocket* socket, NCP* ncp);
  virtual ~SocketChannel();

  void ncpDataCallback(bufferStore& a);
  const char* getNcpRegisterName();
  void ncpConnectAck();
  void ncpRegisterAck();
  void ncpDoRegisterAck(int) {}
  void ncpConnectTerminate();
  void ncpConnectNak();

  bool isConnected() const;
  void socketPoll();
private:
  enum protocolVersionType { PV_SERIES_5 = 6, PV_SERIES_3 = 3 };
  bool ncpCommand(bufferStore &a);

  TCPSocket* socket_;
  char* registerName_;
  bool isConnected_;
  int connectTry_;
  int connectTryTimestamp_;
};
