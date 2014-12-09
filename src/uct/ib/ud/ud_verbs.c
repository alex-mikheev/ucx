/**
* Copyright (C) Mellanox Technologies Ltd. 2001-2014.  ALL RIGHTS RESERVED.
*
* $COPYRIGHT$
* $HEADER$
*/

#include <uct/api/uct.h>
#include <uct/ib/base/ib_context.h>
#include <uct/ib/base/ib_iface.h>
#include <uct/tl/context.h>
#include <ucs/debug/log.h>
#include <ucs/debug/memtrack.h>
#include <ucs/type/class.h>
#include <string.h>
#include <arpa/inet.h> /* For htonl */

#include "ud_iface.h"
#include "ud_ep.h"

typedef struct {
    uct_ud_ep_t          super;
} uct_ud_verbs_ep_t;

typedef struct {
    uct_ud_iface_t     super;
} uct_ud_verbs_iface_t;



static ucs_status_t uct_ud_verbs_query_resources(uct_context_h context,
        uct_resource_desc_t **resources_p,
        unsigned *num_resources_p)
{
    ucs_trace_func("");
    /* TODO take transport overhead into account */
    return uct_ib_query_resources(context, 0, resources_p, num_resources_p);
}

static UCS_CLASS_INIT_FUNC(uct_ud_verbs_ep_t, uct_iface_h tl_iface)
{
    ucs_trace_func("");
    UCS_CLASS_CALL_SUPER_INIT(tl_iface);
    return UCS_OK;
}

static UCS_CLASS_CLEANUP_FUNC(uct_ud_verbs_ep_t)
{
    ucs_trace_func("");
}

UCS_CLASS_DEFINE(uct_ud_verbs_ep_t, uct_ud_ep_t);
static UCS_CLASS_DEFINE_NEW_FUNC(uct_ud_verbs_ep_t, uct_ep_t, uct_iface_h);
static UCS_CLASS_DEFINE_DELETE_FUNC(uct_ud_verbs_ep_t, uct_ep_t);



static ucs_status_t uct_ud_verbs_ep_put_short(uct_ep_h tl_ep, void *buffer,
                                              unsigned length,
                                              uint64_t remote_addr,
                                              uct_rkey_t rkey)
{
//    ucs_trace_data("buf=%p len=%d rva=0x%llx", buffer, length, (unsigned long long)remote_addr);
    return UCS_OK;
}


static void uct_ud_verbs_iface_progress(void *arg)
{
}

static ucs_status_t uct_ud_verbs_iface_query(uct_iface_h tl_iface, uct_iface_attr_t *iface_attr)
{
    uct_ud_iface_t *iface = ucs_derived_of(tl_iface, uct_ud_iface_t);

    ucs_trace_func("");
    uct_ud_iface_query(iface, iface_attr);

    return UCS_OK;
}

static void UCS_CLASS_DELETE_FUNC_NAME(uct_ud_verbs_iface_t)(uct_iface_t*);

uct_iface_ops_t uct_ud_verbs_iface_ops = {
    .iface_close         = UCS_CLASS_DELETE_FUNC_NAME(uct_ud_verbs_iface_t),
    .iface_get_address   = uct_ud_iface_get_address,
    .iface_flush         = uct_ud_iface_flush,
    .ep_get_address      = uct_ud_ep_get_address,
    .ep_connect_to_iface = NULL,
    .ep_connect_to_ep    = uct_ud_ep_connect_to_ep, 
    .iface_query         = uct_ud_verbs_iface_query,
    .ep_put_short        = uct_ud_verbs_ep_put_short,
    .ep_create           = UCS_CLASS_NEW_FUNC_NAME(uct_ud_verbs_ep_t),
    .ep_destroy          = UCS_CLASS_DELETE_FUNC_NAME(uct_ud_verbs_ep_t),
};


static UCS_CLASS_INIT_FUNC(uct_ud_verbs_iface_t, uct_context_h context,
                                   const char *dev_name, uct_iface_config_t *tl_config)
{
    ucs_trace_func("");
    UCS_CLASS_CALL_SUPER_INIT(&uct_ud_verbs_iface_ops, context, dev_name);

    ucs_notifier_chain_add(&context->progress_chain, uct_ud_verbs_iface_progress,
            self);
    return UCS_OK;
}


static UCS_CLASS_CLEANUP_FUNC(uct_ud_verbs_iface_t)
{
    uct_context_h context = self->super.super.super.pd->context;
    ucs_trace_func("");
    ucs_notifier_chain_remove(&context->progress_chain, uct_ud_verbs_iface_progress, self);
}

UCS_CLASS_DEFINE(uct_ud_verbs_iface_t, uct_ud_iface_t);
static UCS_CLASS_DEFINE_NEW_FUNC(uct_ud_verbs_iface_t, uct_iface_t, uct_context_h,
        const char*, uct_iface_config_t*);
static UCS_CLASS_DEFINE_DELETE_FUNC(uct_ud_verbs_iface_t, uct_iface_t);

static uct_tl_ops_t uct_ud_verbs_tl_ops = {
    .query_resources     = uct_ud_verbs_query_resources,
    .iface_open          = UCS_CLASS_NEW_FUNC_NAME(uct_ud_verbs_iface_t),
    .rkey_unpack         = uct_ib_rkey_unpack,
};

static void uct_ud_verbs_register(uct_context_t *context)
{
    uct_register_tl(context, "ud_verbs", uct_ud_iface_config_table,
            sizeof(uct_ud_iface_config_t), &uct_ud_verbs_tl_ops);
}

UCS_COMPONENT_DEFINE(uct_context_t, ud_verbs, uct_ud_verbs_register, ucs_empty_function, 0)

