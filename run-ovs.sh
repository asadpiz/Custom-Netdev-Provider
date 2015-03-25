set -x
XF_SPATH1=/home/farrukh/fulcrum/include
XF_SPATH2=/home/farrukh/fulcrum/include/alos/linux
XF_SPATH3=/home/farrukh/fulcrum/include/std/intel/
XF_SPATH4=/home/farrukh/fulcrum/include/platforms/seacliff/
XF_SPATH5=/home/farrukh/fulcrum/include/alos/
export C_INCLUDE_PATH=$XF_SPATH1:$XF_SPATH2:$XF_SPATH3:$XF_SPATH4:$XF_SPATH5
#./boot.sh
#./configure --prefix=/dev/shm/usr --localstatedir=/dev/shm/usr/var
#make
#make install
#mkdir -p /dev/shm/usr/etc/openvswitch
ovsdb-tool create /dev/shm/usr/etc/openvswitch/conf.db vswitchd/vswitch.ovsschema

ovsdb-server /dev/shm/usr/etc/openvswitch/conf.db --remote=punix:/dev/shm/usr/var/run/openvswitch/db.sock \
                    --remote=db:Open_vSwitch,manager_options \
                   --pidfile --detach
ovs-vsctl --no-wait init
sleep 2
ovs-vswitchd unix:/dev/shm/usr/var/run/openvswitch/db.sock --pidfile


