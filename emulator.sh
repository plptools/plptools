#!/bin/bash

mame \
    -window \
    -nomaximize \
    -skip_gameinfo \
    -rompath /Users/jbmorley/Projects/Psion-ROM/build \
    psion3a2 \
    -sibo serial \
    -sibo:serial:rs232 null_modem \
    -bitbanger socket.127.0.0.1:1234 \
    -prescale 2 \
    -resolution 960x320 \
    -bios 340f \
    -autoboot_delay 20 -autoboot_script boot.lua
