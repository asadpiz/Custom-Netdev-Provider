/*
 * Copyright (c) 2011 Nicira, Inc.
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
 */

#ifndef NETDEV_LINUX_H
#define NETDEV_LINUX_H 1

#include <stdint.h>
#include <stdbool.h>
#include "fm_sdk.h"
#include "simple-queue.h"

/* These functions are Linux specific, so they should be used directly only by
 * Linux-specific code. */

struct netdev;
struct netdev_stats;
struct rtnl_link_stats;

int netdev_linux_ethtool_set_flag(struct netdev *netdev, uint32_t flag,
                                  const char *flag_name, bool enable);
int netdev_linux_get_af_inet_sock(void);

int xf_switch_init(); 
#endif /* netdev-linux.h */