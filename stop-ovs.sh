set -x
rm -rf /dev/shm/usr/etc/openvswitch/conf.db
pkill -9 ovs-vswitchd
pkill -9 ovsdb-server
