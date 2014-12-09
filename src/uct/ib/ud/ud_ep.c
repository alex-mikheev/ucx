/**
 * Copyright (C) Mellanox Technologies Ltd. 2001-2014.  ALL RIGHTS RESERVED.
 *
 * $COPYRIGHT$
 * $HEADER$
 */

#include "ud_ep.h"
#include "ud_iface.h"

#include <uct/ib/base/ib_verbs.h>
#include <ucs/debug/memtrack.h>
#include <ucs/debug/log.h>
#include <ucs/type/class.h>

static UCS_CLASS_INIT_FUNC(uct_ud_ep_t, uct_iface_t *tl_iface)
{
    //uct_ud_iface_t *iface = ucs_derived_of(tl_iface, uct_ud_iface_t);
    //uct_ib_device_t *dev = uct_ib_iface_device(&iface->super);
    //ucs_status_t status;
    ucs_trace_func("");

    UCS_CLASS_CALL_SUPER_INIT(tl_iface);

    return UCS_OK;
}

static UCS_CLASS_CLEANUP_FUNC(uct_ud_ep_t)
{
    ucs_trace_func("");
}

UCS_CLASS_DEFINE(uct_ud_ep_t, uct_ep_t);


ucs_status_t uct_ud_ep_get_address(uct_ep_h tl_ep, uct_ep_addr_t *ep_addr)
{
    uct_ud_ep_t *ep = ucs_derived_of(tl_ep, uct_ud_ep_t);

    ((uct_ud_ep_addr_t*)ep_addr)->ep_id = ep->ep_id;
    ucs_debug("ep_addr=%d", ep->ep_id);
    return UCS_OK;
}

ucs_status_t uct_ud_ep_connect_to_ep(uct_ep_h tl_ep, uct_iface_addr_t *tl_iface_addr,
                uct_ep_addr_t *tl_ep_addr)
{
    ucs_trace_func("");
    return UCS_OK;
}
