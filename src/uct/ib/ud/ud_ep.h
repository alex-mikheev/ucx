/**
* Copyright (C) Mellanox Technologies Ltd. 2001-2014.  ALL RIGHTS RESERVED.
*
* $COPYRIGHT$
* $HEADER$
*/

#ifndef UCT_UD_EP_H
#define UCT_UD_EP_H

#include "ud_def.h"

#include <uct/api/uct.h>


typedef struct uct_ud_ep_addr {
    uct_ep_addr_t     super;
    uint32_t          ep_id;
} uct_ud_ep_addr_t;


typedef struct uct_ud_ep {
    uct_ep_t          super;
    uint32_t ep_id;
} uct_ud_ep_t;


ucs_status_t uct_ud_ep_get_address(uct_ep_h tl_ep, uct_ep_addr_t *ep_addr);

ucs_status_t uct_ud_ep_connect_to_ep(uct_ep_h tl_ep, uct_iface_addr_t *tl_iface_addr,
        uct_ep_addr_t *tl_ep_addr);

#endif 

