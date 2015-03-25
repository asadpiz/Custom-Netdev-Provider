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


#include <stdio.h>
#include <config.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <linux/gen_stats.h>
#include <linux/if_ether.h>
#include <linux/if_tun.h>
#include <linux/types.h>
#include <linux/mii.h>
#include <linux/sockios.h>
#include <linux/version.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <poll.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_packet.h>
#include <net/route.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>

#include "netdev.h"
#include "dpif-linux.h"
#include "dynamic-string.h"
#include "netdev-vport.h"
#include "timer.h"
#include "unaligned.h"
#include "netdev-provider.h"
#include "coverage.h"
#include "fatal-signal.h"
#include "hash.h"
#include "list.h"
#include "ofpbuf.h"
#include "openflow/openflow.h"
#include "packets.h"
#include "poll-loop.h"
#include "shash.h"
#include "smap.h"
#include "sset.h"
#include "svec.h"
#include "byte-order.h"
#include "daemon.h"
#include "dirs.h"
#include "dpif.h"
#include "hmap.h"
#include "socket-util.h"
#include "netdev-linux.h"
#include "simple-queue.h"
#include "fm_sdk.h"
#include "sys/time.h"

#ifndef NETDEV_INTEL_H
#define NETDEV_INTEL_H 1
struct timeval tvalBefore, tvalAfter;
int timerr=0;

/*
 *  Intel device specific netdev management structures and variables
 */

#define FM_MAIN_SWITCH 0
#define ETH_ADDR_LEN    6
#define MAX_INTEL_PORTS 48
#define MIRROR_PORT 3
/**************************************************
* Global Variables
**************************************************/

fm_int sw = 0;
fm_aclCounters counter;
fm_eventPktRecv* pktEvent = 0;
fm_buffer* recvPkt = 0;
fm_timestamp wait = { 5, 0 };
int intinit = 0;
int is_queue_init = 0;  /* Flag to save the state of queues */
int read_index[4] = {0,0,0,0},write_index[4]={0,0,0,0};
static int fd_g = 0;
static int c=0;
/**************************************************
* Semaphore to be used to communicate between event
* handler and main thread
**************************************************/

fm_semaphore seqSem,qSem;

/*
 * netdev Intel struct.
*/
struct netdev_intel {
    struct netdev netdev;
    int fd;
};

/*
 * netdev_dev Intel struct with specific netdev attributes.
*/
struct netdev_dev_intel {
    struct netdev_dev netdev_dev;       /* Common netdev attributes. */
    uint8_t etheraddr[ETH_ADDR_LEN];    /* Ether address of device */
    unsigned int change_seq;
    struct netdev_stats stats;
    fm_int portnum;                     /* Port number of device */
};



/**************************************************
* Application's event handler
**************************************************/

void
eventHandler(fm_int event, fm_int sw, void* ptr) {
   // gettimeofday (&tvalBefore, NULL);
    fm_eventPort* portEvent = (fm_eventPort*) ptr;
    FM_NOT_USED(sw);
    struct timespec t2;
    float push_time = 0.0;
    switch (event) {
    case FM_EVENT_SWITCH_INSERTED:
        printf("Switch #%d inserted!\n", sw);

        if (sw == FM_MAIN_SWITCH) {
            fmSignalSemaphore(&seqSem);
        }

        break;

    case FM_EVENT_PORT:
        printf("port event: port %d is %s\n",
               portEvent->port,
               (portEvent->linkStatus ? "up" : "down"));

        break;

    case FM_EVENT_PKT_RECV:
printf ("I'M IN PACKET RECEIVE OF EVENT HANDLER\n");
        c++;
        pktEvent = (fm_eventPktRecv*) ptr;
        recvPkt = (fm_buffer*)pktEvent->pkt;
#if DEBUG >= 2
	printf("Packet Received of length = %d\n",recvPkt->len);
#endif
printf("Packet Received of length = %d\n",recvPkt->len);
if (recvPkt->len > 800)
{
gettimeofday (&tvalBefore, NULL);
timerr=1;
}
        if (!sq_push(pktEvent->srcPort, &soft_q, (void*)recvPkt->data,
                     (size_t)recvPkt->len)) {
            printf("XFLOW_DEBUG: ERROR: error while pushing the data \n");
            printf("in software queues. \n");
        }
        fd_g++;
#if DEBUG >= 2
        printf("Queue size after pushing is %d \n",soft_q[pktEvent->srcPort].n_pkts);
#endif
        write_index[pktEvent->srcPort-1]++;
        if(recvPkt)
            fmFreeBufferChain(sw, recvPkt);
        break;
    }
}

/*****************************************************************************
* Cleanup: Reports an error and exits
*****************************************************************************/
void
cleanup(const char* src, int err) {
    printf("ERROR: %s: %s\n", src, fmErrorMsg(err));
    exit(1);
}


/*
 * Initializes netdev Intel system and Initializes the Seacliff Switch
*/
static int netdev_intel_init();

/* Attempts to open a network device.  On success, sets 'netdevp'
 * to the new network device. */
static int netdev_intel_open(struct netdev_dev* netdev_dev_,
                             struct netdev** netdevp);

/* Closes 'netdev'. */
static void netdev_intel_close(struct netdev* netdev_);

/* Attempts to create a network device named 'name' in 'netdev_class'.  On
* success sets 'netdev_devp' to the newly created device. */
static int netdev_intel_create(const struct netdev_class* class,
                               const char* name, struct netdev_dev** netdev_devp);

/* Destroys 'netdev_dev'.
 *
 * Netdev devices maintain a reference count that is incremented on
 * netdev_open() and decremented on netdev_close().  If 'netdev_dev'
 * has a non-zero reference count, then this function will not be
 * called. */
void netdev_intel_destroy(struct netdev_dev* netdev_dev_);


/* Sets 'netdev''s Ethernet address to 'mac' */
static int netdev_intel_set_etheraddr(struct netdev* netdev,
                                      const uint8_t mac[ETH_ADDR_LEN]);

/* Copies 'netdev''s MAC address to 'mac' which is passed as param. */
static int netdev_intel_get_etheraddr(const struct netdev* netdev_,
                                      uint8_t mac[ETH_ADDR_LEN]);

/* Returns a sequence number which indicates changes in one of 'netdev''s
 * properties.  The returned sequence number must be nonzero so that
 * callers have a value which they may use as a reset when tracking
 * 'netdev'.
 *
 * Minimally, the returned sequence number is required to change whenever
 * 'netdev''s flags, features, ethernet address, or carrier changes.  The
 * returned sequence number is allowed to change even when 'netdev' doesn't
 * change, although implementations should try to avoid this. */
static unsigned int netdev_intel_change_seq(const struct netdev* netdev);


/* Retrieves the current set of flags on 'netdev' into '*old_flags'.
 * Then, turns off the flags that are set to 1 in 'off' and turns on the
 * flags that are set to 1 in 'on'.  (No bit will be set to 1 in both 'off'
 * and 'on'; that is, off & on == 0.)
 *
 * This function may be invoked from a signal handler.  Therefore, it
 * should not do anything that is not signal-safe (such as logging).
 * XXX
 */
int netdev_intel_update_flags(struct netdev* netdev, enum netdev_flags off,
                              enum netdev_flags on, enum netdev_flags* old_flagsp);

/* Retrieves current device stats for 'netdev' into 'stats'.
 *
 * A network device that supports some statistics but not others, it should
 * set the values of the unsupported statistics to all-1-bits
 * (UINT64_MAX). */
static int netdev_intel_get_stats(const struct netdev* netdev,
                                  struct netdev_stats* stats);


/*
 * Returns true if the class is an implementation of netdev_intel
 * by comparing the function pointers
*/
static bool is_netdev_intel_class(const struct netdev_class* netdev_class);


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
int netdev_intel_send(struct netdev* netdev_, void* data, size_t size);


/* Attempts to set up 'netdev' for receiving packets with ->recv().
 * Returns 0 if successful, otherwise a positive errno value.  Return
 * EOPNOTSUPP to indicate that the network device does not implement packet
 * reception through this interface.  This function may be set to null if
 * it would always return EOPNOTSUPP anyhow.  (This will prevent the
 * network device from being usefully used by the netdev-based "userspace
 * datapath".)
 * XXX */
static int netdev_intel_listen(struct netdev* netdev_);

/* Attempts to receive a packet from 'netdev' into the 'size' bytes in
 * 'buffer'.  If successful, returns the number of bytes in the received
 * packet, otherwise a negative errno value.  Returns -EAGAIN immediately
 * if no packet is ready to be received.
 *
 * Returns -EMSGSIZE, and discards the packet, if the received packet is
 * longer than 'size' bytes */
int netdev_intel_recv(struct netdev* netdev_, void* data, size_t size);

/* Stores the features supported by 'netdev' into each of '*current',
 * '*advertised', '*supported', and '*peer'.  Each value is a bitmap of
 * NETDEV_F_* bits.
 *
 * This function may be set to null if it would always return EOPNOTSUPP.
 * XXX
 */
int netdev_intel_get_features(struct netdev* netdev, uint32_t* current,
                              uint32_t* advertised, uint32_t* supported, uint32_t* peer);

/* Attempts to delete the queue numbered 'queue_id' from 'netdev'.
 *
 * XXX Since Software Queues are Maintained */
int netdev_intel_delete_queue(struct netdev* netdev, unsigned int queue_id);

/* Iterates over all of 'netdev''s queues, calling 'cb' with the queue's
 * ID, its statistics, and the 'aux' specified by the caller.  The order of
 * iteration is unspecified, but (when successful) each queue must be
 * visited exactly once.
 *
 * 'cb' will not modify or free the statistics passed in.
 * XXX
 */
int netdev_intel_dump_queue_stats(const struct netdev* netdev_,
                                  netdev_dump_queue_stats_cb* cb, void* aux);

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
int netdev_intel_dump_queues(const struct netdev* netdev,
                             netdev_dump_queues_cb* cb, void* aux);

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
int netdev_intel_enumerate(struct sset* sset);

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
int netdev_intel_get_ifindex(const struct netdev* netdev_);

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
int netdev_intel_get_qos_types(const struct netdev* netdev OVS_UNUSED,
                               struct sset* types);


/* Arranges for poll_block() to wake up if the "run" member function needs
 * to be called.  Implementations are additionally required to wake
 * whenever something changes in any of its netdevs which would cause their
 * ->change_seq() function to change its result.  May be null if nothing is
 * needed here.
 * XXX */
static void netdev_intel_wait();

/* Performs periodic work needed by netdevs of this class.  May be null if
 * no periodic work is necessary.
 * XXX */
static void netdev_intel_run();

//static void netdev_intel_send_wait(struct netdev *netdev_);
static void netdev_intel_recv_wait(struct netdev *netdev_);

#endif
