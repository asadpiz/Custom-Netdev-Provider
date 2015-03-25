set -x
make clean -s
rm -rf /dev/shm/usr
pkill -9 ovs-vswitchd
pkill -9 ovsdb-server

