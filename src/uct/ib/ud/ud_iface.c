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
    UCS_CLASS_CALL_SUPER_INIT(ops, context, dev_name);
    return UCS_OK;
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


