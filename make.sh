#!/bin/sh

set -x
export PATH=$PATH:/dev/shm/usr/bin:/dev/shm/usr/sbin
XF_SPATH1=/home/farrukh/fulcrum/include
XF_SPATH2=/home/farrukh/fulcrum/include/alos/linux
XF_SPATH3=/home/farrukh/fulcrum/include/std/intel/
XF_SPATH4=/home/farrukh/fulcrum/include/platforms/seacliff/
XF_SPATH5=/home/farrukh/fulcrum/include/alos/
export C_INCLUDE_PATH=$XF_SPATH1:$XF_SPATH2:$XF_SPATH3:$XF_SPATH4:$XF_SPATH5
./boot.sh
./configure --prefix=/dev/shm/usr --localstatedir=/dev/shm/usr/var --silent
make -s
make install -s
mkdir -p /dev/shm/usr/etc/openvswitch
