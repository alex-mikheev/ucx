/**
* Copyright (C) Mellanox Technologies Ltd. 2001-2014.  ALL RIGHTS RESERVED.
*
* $COPYRIGHT$
* $HEADER$
*/

#include "ud_iface.h"

#include <ucs/debug/memtrack.h>
#include <ucs/debug/log.h>
#include <ucs/type/class.h>


static UCS_CLASS_INIT_FUNC(uct_ud_iface_t, uct_iface_ops_t *ops,
        uct_context_h context, const char *dev_name)
{
    /* TODO: exp attrs autoconf */
    struct ibv_exp_qp_init_attr qp_init_attr;
    //struct ibv_qp_attr qp_attr;
    uct_ib_device_t *dev;

    ucs_trace_func("%s: iface=%p ops=%p context=%p", dev_name, self, ops, context);
    ucs_warn("%s: iface=%p ops=%p context=%p", dev_name, self, ops, context);
    UCS_CLASS_CALL_SUPER_INIT(ops, context, dev_name);

    dev = uct_ib_iface_device(&self->super);

    /* Create QP */
    qp_init_attr.qp_context          = NULL;
    qp_init_attr.send_cq             = self->super.send_cq;
    qp_init_attr.recv_cq             = self->super.recv_cq;
    qp_init_attr.srq                 = NULL; /* TODO */
    qp_init_attr.qp_type             = IBV_QPT_UD;
    qp_init_attr.sq_sig_all          = 0;
    
    /* TODO: cap setting */
    qp_init_attr.cap.max_send_wr     = UCT_UD_TX_QP_LEN;
    qp_init_attr.cap.max_recv_wr     = 1024;
    qp_init_attr.cap.max_send_sge    = 2;
    qp_init_attr.cap.max_recv_sge    = 1;
    qp_init_attr.cap.max_inline_data = 0;

#if HAVE_VERBS_EXP_H
    /* TODO: inline rcv */
    self->qp = ibv_exp_create_qp(dev->ibv_context, &qp_init_attr);
#else
    self->qp = ibv_exp_create_qp(dev->pd, &qp_init_attr);
#endif
    if (self->qp == NULL) {
        ucs_error("Failed to create qp: %m [inline: %u rsge: %u ssge: %u rwr: %u swr: %u]",
                qp_init_attr.cap.max_inline_data, qp_init_attr.cap.max_recv_sge,
                qp_init_attr.cap.max_send_sge, qp_init_attr.cap.max_recv_wr,
                qp_init_attr.cap.max_send_wr);
        goto err;
    }
    ucs_warn("UD QP created");

    return UCS_OK;
err:
    return UCS_ERR_INVALID_PARAM;
}

static UCS_CLASS_CLEANUP_FUNC(uct_ud_iface_t)
{
}

UCS_CLASS_DEFINE(uct_ud_iface_t, uct_ib_iface_t);

ucs_config_field_t uct_ud_iface_config_table[] = {
    {"", "", NULL,
        ucs_offsetof(uct_ud_iface_config_t, super), UCS_CONFIG_TYPE_TABLE(uct_ib_iface_config_table)},

    {NULL}
};


