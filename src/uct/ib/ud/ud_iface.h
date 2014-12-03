/**
* Copyright (C) Mellanox Technologies Ltd. 2001-2014.  ALL RIGHTS RESERVED.
*
* $COPYRIGHT$
* $HEADER$
*/


#ifndef UCT_UD_IFACE_H
#define UCT_UD_IFACE_H

#include <uct/ib/base/ib_iface.h>
#include <ucs/datastruct/sglib_wrapper.h>

typedef struct uct_ud_iface {
    uct_ib_iface_t           super;

    //uct_rc_ep_t              *eps[UCT_RC_QP_HASH_SIZE];

} uct_ud_iface_t;

typedef struct uct_ud_iface_config {
    uct_ib_iface_config_t    super;
} uct_ud_iface_config_t;


extern ucs_config_field_t uct_ud_iface_config_table[];

void uct_ud_iface_query(uct_ud_iface_t *iface, uct_iface_attr_t *iface_attr);

ucs_status_t uct_ud_iface_get_address(uct_iface_h tl_iface, uct_iface_addr_t *iface_addr);

#if 0
uct_ud_ep_t *uct_ud_iface_lookup_ep(uct_ud_iface_t *iface, unsigned qp_num);

void uct_ud_iface_add_ep(uct_ud_iface_t *iface, uct_rc_ep_t *ep);
void uct_ud_iface_remove_ep(uct_ud_iface_t *iface, uct_rc_ep_t *ep);
#endif

ucs_status_t uct_ud_iface_flush(uct_iface_h tl_iface, uct_req_h *req_p,
        uct_completion_cb_t cb);


#endif

