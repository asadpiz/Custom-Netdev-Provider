/*
 * Copyright (c) 2013 xFlow Research.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 *
 * AUTHORS : Junaid Zulfiqar and Syed Asadullah Hussain
 *
 *
*/

#include "netdev-intel.h"

/*
 * casts netdev_dev to netdev_dev_intel
*/
struct netdev_dev_intel*
netdev_dev_intel_cast(const struct netdev_dev* netdev_dev) {
        const struct netdev_class* netdev_class = netdev_dev_get_class(netdev_dev);

    if (is_netdev_intel_class(netdev_class)) {
        return CONTAINER_OF(netdev_dev, struct netdev_dev_intel, netdev_dev);
    }

    return NULL;
}

/*
 * casts netdev to netdev_intel
*/
struct netdev_intel*
netdev_intel_cast(const struct netdev* netdev) {

        struct netdev_dev* netdev_dev = netdev_get_dev(netdev);
        const struct netdev_class* netdev_class = netdev_dev_get_class(netdev_dev);
   

    if (is_netdev_intel_class(netdev_class)) {
        return CONTAINER_OF(netdev, struct netdev_intel, netdev);
    }

    return NULL;
}


/*
 * Initializes netdev Intel system and Initializes the Seacliff Switch
*/
static int
netdev_intel_init() {
    if (!intinit) {
        intinit = 1;
        fm_uint32 compile_flags, apply_flags, scenario, port_value, port_set;
        fm_status       err;
        fm_switchInfo   swInfo;
        fm_int          cpi, rule, acl;
        fm_int          port;
        fm_aclCondition cond;
        fm_aclValue     value;
        fm_aclActionExt    action;
	fm_aclParamExt	param;
        struct in_addr src, dst, smask;
        char            status[100];

        if (!is_queue_init) {
            sq_init(soft_q);
            is_queue_init = 1;
        }
        fmOSInitialize();
        fmCreateSemaphore("seq", FM_SEM_BINARY, &seqSem, 0);
        fmCreateSemaphore("queue", FM_SEM_BINARY, &qSem, 0);
        fmSignalSemaphore(&qSem);
        if ((err = fmInitialize(eventHandler)) != FM_OK) {
            cleanup("fmInitialize", err);
        }

        fmWaitSemaphore(&seqSem, &wait);

        if ((err = fmSetSwitchState(sw, TRUE)) != FM_OK) {
            cleanup("fmSetSwitchState", err);
        }

        fmCreateVlan(sw, 1);
        fmGetSwitchInfo(sw, &swInfo);
#if DEBUG >= 1
        printf("Total Number of Ports = %d\n", swInfo.numCardPorts);
#endif     
        port_value = FM_PORT_PARSER_STOP_AFTER_L4;

        for (cpi = 1 ; cpi < swInfo.numCardPorts ; cpi++) {
            if ((err = fmMapCardinalPort(sw, cpi, &port, NULL)) != FM_OK) {
                cleanup("fmMapCardinalPort", err);
            }

            if ((err = fmSetPortState(sw, port, FM_PORT_STATE_UP, 0)) != FM_OK) {
                cleanup("fmSetPortState", err);
            }

            if ((err = fmAddVlanPort(sw, 1, port, FALSE)) != FM_OK) {
                cleanup("fmAddVlanPort", err);
            }

            if ((err = fmSetVlanPortState(sw,
                                          1,
                                          port,
                                          FM_STP_STATE_FORWARDING)) != FM_OK) {
                cleanup("fmSetVlanPortState", err);
            }

	    if ( (err = fmSetPortAttribute(sw, port, FM_PORT_PARSER, &port_value) ) != FM_OK )
            {
    	        cleanup("fmSetPortAttribute", err);
            }
		

        }

        acl = 0;
        scenario = FM_ACL_SCENARIO_ANY_FRAME_TYPE | FM_ACL_SCENARIO_ANY_ROUTING_TYPE;

        if ((err = fmCreateACLExt(sw, acl, scenario, 0)) != FM_OK) {
            cleanup("fmCreateACL", err);
        }

        rule = 6144;
        cond = FM_ACL_MATCH_SRC_IP | FM_ACL_MATCH_INGRESS_PORT_SET;
        inet_aton("0.0.0.0", &src);
        inet_aton("0.0.0.0", &smask);
        value.srcIp.addr[0] = src.s_addr;
        value.srcIp.isIPv6 = 0;
        value.srcIpMask.addr[0] = 0;
        value.srcIpMask.isIPv6 = 0;
        action = FM_ACL_ACTIONEXT_TRAP | FM_ACL_ACTIONEXT_COUNT;

        //************************PORT SET*******************************//
        if ((err = fmCreateACLPortSet(sw, &port_set)) != FM_OK)
             cleanup("fmCreateACLPortSet",err);

        for (cpi = 1 ; cpi < swInfo.numCardPorts ; cpi++) {
            port = cpi;

            //if ((err = fmAddACLPort(sw, acl, port, FM_ACL_TYPE_INGRESS)) != FM_OK) {
            //    cleanup("fmAddACLPort", err);
            //}
            
            if( (err = fmAddACLPortSetPort(sw, port_set, port)) != FM_OK)
                cleanup("fmAddACLPortSetPort",err);

        }

        value.portSet = port_set;
  
        if ((err = fmAddACLRuleExt(sw, acl, rule, cond, &value, \
                                action, &param)) != FM_OK) {
            cleanup("fmAddACLRuleExt", err);
        }
        

        compile_flags = FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE;

        if ((err = fmCompileACLExt(sw, status, 100, 0, NULL)) != FM_OK) {
            cleanup("fmCompileACL", err);
        }

        apply_flags = FM_ACL_APPLY_FLAG_NON_DISRUPTIVE;

        if ((err = fmApplyACL(sw, 0)) != FM_OK) {
            cleanup("fmApplyACL", err);
        }
      
        fm_int group, mirrorPort;
        fm_mirrorType mirrorType = FM_MIRROR_TYPE_INGRESS;
        fm_int attr = FM_MIRROR_ACL;
        fm_int mirror_value = FM_ENABLED;
        group = 0;
        mirrorPort = MIRROR_PORT;
        if ( (err = fmCreateMirror(acl,group, mirrorPort, mirrorType) ) != FM_OK)
        {
              cleanup("fmCreateMirror", err);
        }
        if ( (err = fmSetMirrorAttribute(acl, group, attr, &mirror_value) ) != FM_OK) {
              cleanup("fmSetMirrorAttribute", err);
        }
        for (cpi = 1 ; cpi < swInfo.numCardPorts ; cpi++) {
            port = cpi;        
            if ( (err = fmAddMirrorPortExt(acl, group, port,mirrorType) ) != FM_OK) {
                cleanup("fmAddMirrorPort", err);
            }
        }
        printf("All ports added to Mirror Group by default\n");
        
    }
#if DEBUG >= 1
    printf("\nInitialized and ACL created\n\n");
#endif
    return 0;
}


/* Attempts to open a network device.  On success, sets 'netdevp'
 * to the new network device. */
static int
netdev_intel_open(struct netdev_dev* netdev_dev_, struct netdev** netdevp) {
    struct netdev_intel* netdev;
    netdev = xzalloc(sizeof * netdev);
    netdev->fd = -1;
    netdev_init(&netdev->netdev, netdev_dev_);
    *netdevp = &netdev->netdev;
    return 0;
}

/* Closes and destroys 'netdev'. */
static void
netdev_intel_close(struct netdev* netdev_) {
    struct netdev_intel* netdev = netdev_intel_cast(netdev_);
    free(netdev);
}


/* Attempts to create a network device named 'name' in 'netdev_class'.  On
* success sets 'netdev_devp' to the newly created device. */
static int
netdev_intel_create(const struct netdev_class* class, const char* name,
                    struct netdev_dev** netdev_devp) {
    struct netdev_dev_intel* netdev_dev;
    netdev_dev = xzalloc(sizeof * netdev_dev);
    netdev_dev->change_seq = 1;
    fm_int port,port1;
    port  = (fm_int) atoi(&name[4]);
    if (name[5] != '\0') {
        port1 = (fm_int) atoi(&name[5]);
        port = (port * 10) + port1;
    }

    netdev_dev->portnum = port;
    netdev_dev_init(&netdev_dev->netdev_dev, name, class);
    *netdev_devp = &netdev_dev->netdev_dev;
    return 0;
}

/*
 * Destroys 'netdev_dev'.
 *
 * Netdev devices maintain a reference count that is incremented on
 * netdev_open() and decremented on netdev_close().  If 'netdev_dev'
 * has a non-zero reference count, then this function will not be
 * called.
*/
void
netdev_intel_destroy(struct netdev_dev* netdev_dev_) {
    struct netdev_dev_intel* netdev_dev = netdev_dev_intel_cast(netdev_dev_);
    /*
     * free the memory holded by netdev.
    */
    free(netdev_dev);
}


/* Stores the features supported by 'netdev' into each of '*current',
 * '*advertised', '*supported', and '*peer'.  Each value is a bitmap of
 * NETDEV_F_* bits.
 *
 * This function may be set to null if it would always return EOPNOTSUPP.
 * XXX
*/
int
netdev_intel_get_features(struct netdev* netdev, uint32_t* current,
                          uint32_t* advertised, uint32_t* supported,
                          uint32_t* peer) {
    return EOPNOTSUPP;
}


/* Attempts to delete the queue numbered 'queue_id' from 'netdev'.
 *
 * XXX Since Software Queues are Maintained */
int
netdev_intel_delete_queue(struct netdev* netdev, unsigned int queue_id) {
    return 0;
}


/*
 *Enumerates the names of all network devices of this class.
 *
 * The caller has already initialized 'sset' and might already have
 * added some names to it.  This function should not disturb any existing
 * names in 'sset'.
 *
 * If this netdev class does not support enumeration, this may be a null
 * pointer.
*/
int
netdev_intel_enumerate(struct sset* sset) {
    int i;

    for (i = 0; i < MAX_INTEL_PORTS; i++) {
        sset_add(sset, xasprintf("p%u", i));
    }

    return 0;
}


/* Returns the ifindex of 'netdev', if successful, as a positive number.
 * On failure, returns a negative errno value.
 *
 * The desired semantics of the ifindex value are a combination of those
 * specified by POSIX for if_nametoindex() and by SNMP for ifIndex.  An
 * ifindex value should be unique within a host and remain stable at least
 * until reboot.  SNMP says an ifindex "ranges between 1 and the value of
 * ifNumber" but many systems do not follow this rule anyhow.
 *
 * This function may be set to null if it would always return -EOPNOTSUPP.

 * XXX
 */
int
netdev_intel_get_ifindex(const struct netdev* netdev_) {
    return 1;
}


/* Adds to 'types' all of the forms of QoS supported by 'netdev', or leaves
 * it empty if 'netdev' does not support QoS.  Any names added to 'types'
 * should be documented as valid for the "type" column in the "QoS" table
 * in vswitchd/vswitch.xml (which is built as ovs-vswitchd.conf.db(8)).
 *
 * Every network device must support disabling QoS with a type of "", but
 * this function must not add "" to 'types'.
 *
 * The caller is responsible for initializing 'types' (e.g. with
 * sset_init()) before calling this function.  The caller retains ownership
 * of 'types'.
 *
 * May be NULL if 'netdev' does not support QoS at all. */
int
netdev_intel_get_qos_types(const struct netdev* netdev OVS_UNUSED,
                           struct sset* types) {
    sset_add(types, "intel-default");
    return 0;
}



/* Attempts to set up 'netdev' for receiving packets with ->recv().
 * Returns 0 if successful, otherwise a positive errno value.  Return
 * EOPNOTSUPP to indicate that the network device does not implement packet
 * reception through this interface.  This function may be set to null if
 * it would always return EOPNOTSUPP anyhow.  (This will prevent the
 * network device from being usefully used by the netdev-based "userspace
 * datapath".)
 * XXX */
static int
netdev_intel_listen(struct netdev* netdev_) {
    return 0;
}

/*
 * Returns true if the class is an implementation of netdev_intel
 * by comparing the function pointers
*/
static bool
is_netdev_intel_class(const struct netdev_class* netdev_class) {
    return netdev_class->init == netdev_intel_init;
}


/* Sets 'netdev''s Ethernet address to 'mac' */
static int
netdev_intel_set_etheraddr(struct netdev* netdev_,
                           const uint8_t mac[ETH_ADDR_LEN]) {
    struct netdev_dev_intel* netdev_dev = netdev_dev_intel_cast(
            netdev_get_dev(netdev_));
    memcpy(netdev_dev->etheraddr, mac, ETH_ADDR_LEN);
    return 0;
}

/* Copies 'netdev''s MAC address to 'mac' which is passed as param. */
static int
netdev_intel_get_etheraddr(const struct netdev* netdev_,
                           uint8_t mac[ETH_ADDR_LEN]) {
    struct netdev_dev_intel* netdev_dev =
        netdev_dev_intel_cast(netdev_get_dev(netdev_));
    memcpy(mac, netdev_dev->etheraddr, ETH_ADDR_LEN);
    return 0;
}


/* Retrieves current device stats for 'netdev' into 'stats'.
 *
 * A network device that supports some statistics but not others, it should
 * set the values of the unsupported statistics to all-1-bits
 * (UINT64_MAX). */
static int
netdev_intel_get_stats(const struct netdev* netdev_, struct netdev_stats* stats) {
    struct netdev_dev_intel* netdev_dev =
        netdev_dev_intel_cast(netdev_get_dev(netdev_));
    memcpy(stats, &netdev_dev->stats, sizeof * stats);
    return 0;
}


/* Attempts to receive a packet from 'netdev' into the 'size' bytes in
 * 'buffer'.  If successful, returns the number of bytes in the received
 * packet, otherwise a negative errno value.  Returns -EAGAIN immediately
 * if no packet is ready to be received.
 *
 * Returns -EMSGSIZE, and discards the packet, if the received packet is
 * longer than 'size' bytes */
int
netdev_intel_recv(struct netdev* netdev_, void* data, size_t size) 
{
    //printf("intel recv\n");
    fmGetACLCountExt(sw, 0, 6144, &counter);
    //printf("Trap count = %d\n", (int)counter.cntPkts);
    float pop_time = 0.0;
    struct timespec t2;
    struct netdev_dev_intel* netdev_dev = netdev_dev_intel_cast(netdev_get_dev(netdev_));
    //fm_bool state;
    //fmGetSwitchState(0,&state);
    //printf("state = %d\n",state);
    if (netdev_dev == NULL) 
    {
        return;
    }
    ssize_t retval = 0;
    if (soft_q[netdev_dev->portnum].n_pkts > 0)
    {
        retval = sq_pop(netdev_dev->portnum, &soft_q, data, size);
#if DEBUG >= 2        
        printf("Queue size after pop is %d \n",soft_q[netdev_dev->portnum].n_pkts);
#endif
        if (retval > size) {
            return retval = -EMSGSIZE;   
        }
        if(retval > 0) {
            //read_index[netdev_dev->portnum - 1]++;
            fd_g = 1;
            return retval;
        }
    } 
    return -EAGAIN;
}

static void
netdev_intel_recv_wait(struct netdev *netdev_)
{
     //printf("in recv_wait\n");
     //struct netdev_intel *netdev = netdev_intel_cast(netdev_);
     if(fd_g > 0) {
        //netdev->fd = fd_g; 
        //printf("polling\n");
        poll_immediate_wake();;
     }
}

/*
static void
netdev_intel_send_wait(struct netdev *netdev_)
{
     poll_fd_wait(0, POLLOUT);
}
*/
/* Sends the 'size'-byte packet in 'buffer' on 'netdev'.  Returns 0 if
 * successful, otherwise a positive errno value.  Returns EAGAIN without
 * blocking if the packet cannot be queued immediately.  Returns EMSGSIZE
 * if a partial packet was transmitted or if the packet is too big or too
 * small to transmit on the device.
 *
 * The caller retains ownership of 'buffer' in all cases.
 *
 * The network device is expected to maintain a packet transmission queue,
 * so that the caller does not ordinarily have to do additional queuing of
 * packets.
 *
 * May return EOPNOTSUPP if a network device does not implement packet
 * transmission through this interface. */
int
netdev_intel_send(struct netdev* netdev_, void* data, size_t size) {
    struct netdev_dev_intel* netdev_dev =
        netdev_dev_intel_cast(netdev_get_dev(netdev_));

    if (netdev_dev == NULL) {
        return;
    }

    if(size == 0 )
        return -EAGAIN;

    char* name = (char*)malloc(sizeof(char) * 10);
    snprintf(name, sizeof(netdev_->netdev_dev->name), "%s", netdev_->netdev_dev->name);
    printf("netdev name for send = %s\n",name);
    if (strcmp(name, "br0")) {
        fm_int attr = 0;
        fm_int enabled = FM_ENABLED;
        fm_int mode, state, info;
        fm_status status;
        fm_switchInfo swInfo;
        fm_int port, port1;
        fm_timestamp wait = { 3, 0 };
        fm_buffer* buf;
        int* portList = (int *)malloc(sizeof(int));

        buf = fmAllocateBuffer(sw);
        if(!buf) 
        {
            printf("Fail to allocate buffer\n");
            return 1;   
        }
        memcpy(buf->data, data, size);
        buf->len = size;

        if ((status = fmMapCardinalPort(sw, netdev_dev->portnum, &port, NULL)) != FM_OK) 
        {
            printf("Error: fmMapCardinalPort %s\n", fmErrorMsg(status));
            return -EAGAIN;
        }

        portList[0] = port;
        if ((status = fmGetPortState(sw, port, &mode, &state, &info)) == FM_OK)  
        {
            if (state == FM_PORT_STATE_UP) 
            {
                status = fmSendPacketDirected(sw, portList, 1, buf);
printf ("value of timer is %d\n",timerr);
if (timerr == 1)
{	        
gettimeofday (&tvalAfter, NULL);
         /********************* Printing Time taken in microseconds *************/
printf("Time in microseconds: %lld microseconds\n",((tvalAfter.tv_sec - tvalBefore.tv_sec)*1000000L+tvalAfter.tv_usec) - tvalBefore.tv_usec);
tvalBefore.tv_sec = 0;
tvalBefore.tv_usec = 0;

tvalAfter.tv_sec = 0;
tvalAfter.tv_usec = 0;


timerr = 0;

}
                if (status != FM_OK) {
                    printf("ERROR: fmSend %s\n Packet not send\n",
                           fmErrorMsg(status));
                }
            }     
        }
        
        free(portList);
        free(name);

        //if(buf)
        //    fmFreeBuffer(sw,buf);
        return (int)status;
    }
    return 0;
}

/* Performs periodic work needed by netdevs of this class.  May be null if
 * no periodic work is necessary.
 * XXX*/
static void
netdev_intel_run(void) {
    return; 
    //printf("IN INTEL-RUN\n");
}

/* Arranges for poll_block() to wake up if the "run" member function needs
 * to be called.  Implementations are additionally required to wake
 * whenever something changes in any of its netdevs which would cause their
 * ->change_seq() function to change its result.  May be null if nothing is
 * needed here.
 * XXX */
static void
netdev_intel_wait(void) {
    return;
    //printf("IN INTEL-WAIT\n");
}


/* Iterates over all of 'netdev''s queues, calling 'cb' with the queue's
 * ID, its configuration, and the 'aux' specified by the caller.  The order
 * of iteration is unspecified, but (when successful) each queue is visited
 * exactly once.
 *
 * 'cb' will not modify or free the 'details' argument passed in.  It may
 * delete or modify the queue passed in as its 'queue_id' argument.  It may
 * modify but will not delete any other queue within 'netdev'.  If 'cb'
 * adds new queues, then ->dump_queues is allowed to visit some queues
 * twice or not at all.
 * XXX
 */
int
netdev_intel_dump_queues(const struct netdev* netdev, netdev_dump_queues_cb* cb,
                         void* aux) {
    return 0;
}


/* Iterates over all of 'netdev''s queues, calling 'cb' with the queue's
 * ID, its statistics, and the 'aux' specified by the caller.  The order of
 * iteration is unspecified, but (when successful) each queue must be
 * visited exactly once.
 *
 * 'cb' will not modify or free the statistics passed in.
 * XXX
 */
int
netdev_intel_dump_queue_stats(const struct netdev* netdev_,
                              netdev_dump_queue_stats_cb* cb, void* aux) {
    return 0;
}


/* Retrieves the current set of flags on 'netdev' into '*old_flags'.
 * Then, turns off the flags that are set to 1 in 'off' and turns on the
 * flags that are set to 1 in 'on'.  (No bit will be set to 1 in both 'off'
 * and 'on'; that is, off & on == 0.)
 *
 * This function may be invoked from a signal handler.  Therefore, it
 * should not do anything that is not signal-safe (such as logging).
 * XXX
 */
int
netdev_intel_update_flags(struct netdev* netdev, enum netdev_flags off,
                          enum netdev_flags on, enum netdev_flags* old_flagsp) {
    return 0;
}

/* Returns a sequence number which indicates changes in one of 'netdev''s
 * properties.  The returned sequence number must be nonzero so that
 * callers have a value which they may use as a reset when tracking
 * 'netdev'.
 *
 * Minimally, the returned sequence number is required to change whenever
 * 'netdev''s flags, features, ethernet address, or carrier changes.  The
 * returned sequence number is allowed to change even when 'netdev' doesn't
 * change, although implementations should try to avoid this. */
static unsigned int
netdev_intel_change_seq(const struct netdev* netdev) {
    return netdev_dev_intel_cast(netdev_get_dev(netdev))->change_seq;
}


const struct netdev_class netdev_intel_class = {
    "intel",                    /* type */
    netdev_intel_init,          /* init */
    NULL,                       /* run */
    NULL,                       /* wait */

    netdev_intel_create,        /* create */
    netdev_intel_destroy,       /* destroy */
    NULL,                       /* get_config */
    NULL,                       /* set_config */
    NULL,                       /* get_tunnel_config */

    netdev_intel_open,          /* open */
    netdev_intel_close,         /* close */

    netdev_intel_listen,        /*listen*/
    netdev_intel_recv,          /* receive */
    netdev_intel_recv_wait,     /* receive_wait */
    NULL,                       /* drain */

    netdev_intel_send,           /* send */
    NULL,                        /* send_wait */

    netdev_intel_set_etheraddr,  /* set_ethertaddr */
    netdev_intel_get_etheraddr,  /* get_ethertaddr */
    NULL,                        /* get_mtu */
    NULL,                        /* set_mtu */
    NULL,                        /* get_ifindex */
    NULL,                        /* get_carrier */
    NULL,                        /* get_carrier_resets */
    NULL,                        /* get_miimon */
    netdev_intel_get_stats,      /* get_stats */
    NULL,                        /* set_stats */

    NULL,                       /* get_features */
    NULL,                       /* set_advertisements */

    NULL,                       /* set_policing */
    NULL,                       /* get_qos_types */
    NULL,                       /* get_qos_capabilities */
    NULL,                       /* get_qos */
    NULL,                       /* set_qos */
    NULL,                       /* get_queue */
    NULL,                       /* set_queue */
    NULL,                       /* delete_queue */
    NULL,                       /* get_queue_stats */
    NULL,                       /* dump_queues */
    NULL,                       /* dump_queue_stats */

    NULL,                       /* get_in4 */
    NULL,                       /* set_in4 */
    NULL,                       /* get_in6 */
    NULL,                       /* add_router */
    NULL,                       /* get_next_hop */
    NULL,                       /* get_status */
    NULL,                       /* arp_lookup */

    netdev_intel_update_flags,  /* update flags */

    netdev_intel_change_seq,    /* change sequence */
};
