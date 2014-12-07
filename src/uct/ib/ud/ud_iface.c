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
    struct ibv_qp_attr qp_attr;
    uct_ib_device_t *dev;
    int ret;

    ucs_trace_func("%s: iface=%p ops=%p context=%p", dev_name, self, ops, context);
    UCS_CLASS_CALL_SUPER_INIT(ops, context, dev_name);

    dev = uct_ib_iface_device(&self->super);

    /* Create QP */
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
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
    qp_init_attr.cap.max_inline_data = 0; /* TODO */

#if HAVE_VERBS_EXP_H
    qp_init_attr.pd                  = dev->pd;
    qp_init_attr.comp_mask           = IBV_QP_INIT_ATTR_PD;
    /* TODO: inline rcv */
#if 0
    if (mxm_ud_ep_opts(ep)->ud.ib.rx.max_inline > 0) {
        qp_init_attr.comp_mask      |= IBV_EXP_QP_INIT_ATTR_INL_RECV;
        qp_init_attr.max_inl_recv    = mxm_ud_ep_opts(ep)->ud.ib.rx.max_inline;
    }
#endif
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

    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    /* Modify QP to INIT state */
    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.pkey_index = 0;
    qp_attr.port_num = self->super.port_num;
    qp_attr.qkey = UCT_UD_QKEY;
    ret = ibv_modify_qp(self->qp, &qp_attr,
            IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_QKEY);
    if (ret) {
        ucs_error("Failed to modify UD QP to INIT: %m");
        goto err_destroy_qp;
    }

    /* Modify to RTR */
    qp_attr.qp_state = IBV_QPS_RTR;
    ret = ibv_modify_qp(self->qp, &qp_attr, IBV_QP_STATE);
    if (ret) {
        ucs_error("Failed to modify UD QP to RTR: %m");
        goto err_destroy_qp;
    }

    /* Modify to RTS */
    qp_attr.qp_state = IBV_QPS_RTS;
    qp_attr.sq_psn = 0;
    ret = ibv_modify_qp(self->qp, &qp_attr, IBV_QP_STATE | IBV_QP_SQ_PSN);
    if (ret) {
        ucs_error("Failed to modify UD QP to RTS: %m");
        goto err_destroy_qp;
    }

    ucs_debug("%s iface=%p: created qp 0x%x max_send_wr %u max_recv_wr %u max_inline %u",
            dev_name, self, self->qp->qp_num,
            qp_init_attr.cap.max_send_wr,
            qp_init_attr.cap.max_recv_wr,
            qp_init_attr.cap.max_inline_data);

    return UCS_OK;
err_destroy_qp:
    ibv_destroy_qp(self->qp);
err:
    return UCS_ERR_INVALID_PARAM;
}

static UCS_CLASS_CLEANUP_FUNC(uct_ud_iface_t)
{
    ucs_trace_func("");
}

UCS_CLASS_DEFINE(uct_ud_iface_t, uct_ib_iface_t);

ucs_config_field_t uct_ud_iface_config_table[] = {
    {"", "", NULL,
        ucs_offsetof(uct_ud_iface_config_t, super), UCS_CONFIG_TYPE_TABLE(uct_ib_iface_config_table)},

    {NULL}
};


