/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_memory_context_manager.h"

#include "sc-store/sc-base/sc_allocator.h"

#include "sc_memory.h"
#include "sc_memory_private.h"
#include "sc-store/sc-event/sc_event_private.h"

struct _sc_memory_context_manager
{
  sc_hash_table * context_hash_table;
  sc_uint32 context_count;
  sc_monitor context_monitor;
};

struct _sc_event_emit_params
{
  sc_addr subscription_addr;
  sc_access_levels subscription_addr_access;
  sc_event_type type;
  sc_addr edge_addr;
  sc_addr other_addr;
};

struct _sc_memory_context
{
  sc_addr user_addr;
  sc_access_levels access_levels;
  sc_uint8 flags;
  sc_hash_table_list * pend_events;
  sc_monitor monitor;
};

#define SC_CONTEXT_FLAG_PENDING_EVENTS 0x1

void _sc_memory_context_manager_initialize(sc_memory_context_manager ** manager, sc_addr my_self_addr)
{
  sc_memory_info("Initialize context manager");

  *manager = sc_mem_new(sc_memory_context_manager, 1);
  (*manager)->context_hash_table = sc_hash_table_init(g_direct_hash, g_direct_equal, null_ptr, null_ptr);
  (*manager)->context_count = 0;
  sc_monitor_init(&(*manager)->context_monitor);

  s_memory_default_ctx = sc_memory_context_new(my_self_addr);
}

void _sc_memory_context_manager_shutdown(sc_memory_context_manager * manager)
{
  sc_memory_info("Shutdown context manager");

  sc_memory_context_free(s_memory_default_ctx);
  s_memory_default_ctx = null_ptr;

  if (manager->context_hash_table == null_ptr)
    return;

  sc_monitor_acquire_read(&manager->context_monitor);
  sc_uint32 context_count = sc_hash_table_size(manager->context_hash_table);
  sc_monitor_release_read(&manager->context_monitor);
  if (context_count > 0)
    sc_memory_warning("There are %d contexts, wasn't destroyed before sc-memory shutdown", context_count);

  sc_monitor_acquire_write(&manager->context_monitor);
  sc_hash_table_destroy(manager->context_hash_table);
  manager->context_hash_table = null_ptr;
  sc_monitor_release_write(&manager->context_monitor);

  sc_monitor_destroy(&manager->context_monitor);

  sc_mem_free(manager);
}

sc_memory_context * _sc_memory_context_new_impl(sc_memory_context_manager * manager, sc_addr user_addr)
{
  if (manager == null_ptr)
    return null_ptr;

  sc_memory_context * ctx = sc_mem_new(sc_memory_context, 1);

  sc_monitor_acquire_write(&manager->context_monitor);

  if (manager->context_hash_table == null_ptr)
    goto error;

  sc_monitor_init(&ctx->monitor);
  ctx->user_addr = user_addr;
  ctx->access_levels = 0;

  ++manager->context_count;
  goto result;

error:
  sc_mem_free(ctx);
  ctx = null_ptr;

result:
  sc_monitor_release_write(&manager->context_monitor);

  return ctx;
}

void _sc_memory_context_free_impl(sc_memory_context_manager * manager, sc_memory_context * ctx)
{
  if (ctx == null_ptr)
    return;

  sc_monitor_acquire_write(&manager->context_monitor);

  if (manager->context_hash_table == null_ptr)
    goto error;

  sc_monitor_destroy(&ctx->monitor);
  sc_hash_table_remove(manager->context_hash_table, GINT_TO_POINTER(SC_ADDR_LOCAL_TO_INT(ctx->user_addr)));
  --manager->context_count;

  sc_mem_free(ctx);
error:
  sc_monitor_release_write(&manager->context_monitor);
}

sc_bool _sc_memory_context_is_pending(sc_memory_context const * ctx)
{
  sc_monitor_acquire_write((sc_monitor *)&ctx->monitor);
  sc_bool result = ((sc_memory_context *)ctx)->flags & SC_CONTEXT_FLAG_PENDING_EVENTS;
  sc_monitor_release_write((sc_monitor *)&ctx->monitor);
  return result;
}

void _sc_memory_context_pend_event(
    sc_memory_context const * ctx,
    sc_event_type type,
    sc_addr element,
    sc_addr edge,
    sc_addr other_element)
{
  sc_event_emit_params * params = sc_mem_new(sc_event_emit_params, 1);
  params->type = type;
  params->subscription_addr = element;
  params->subscription_addr_access = sc_access_lvl_make_max;
  params->edge_addr = edge;
  params->other_addr = other_element;

  sc_monitor_acquire_write((sc_monitor *)&ctx->monitor);
  ((sc_memory_context *)ctx)->pend_events = g_slist_append(ctx->pend_events, params);
  sc_monitor_release_write((sc_monitor *)&ctx->monitor);
}

void _sc_memory_context_emit_events(sc_memory_context const * ctx)
{
  GSList * item = null_ptr;
  sc_event_emit_params * evt_params = null_ptr;

  while (ctx->pend_events)
  {
    item = ctx->pend_events;
    evt_params = (sc_event_emit_params *)item->data;

    sc_event_emit_impl(
        ctx,
        evt_params->subscription_addr,
        evt_params->subscription_addr_access,
        evt_params->type,
        evt_params->edge_addr,
        evt_params->other_addr);

    sc_mem_free(evt_params);

    ((sc_memory_context *)ctx)->pend_events = g_slist_delete_link(ctx->pend_events, ctx->pend_events);
  }
}

void _sc_memory_context_pending_begin(sc_memory_context * ctx)
{
  sc_monitor_acquire_write(&ctx->monitor);
  ctx->flags |= SC_CONTEXT_FLAG_PENDING_EVENTS;
  sc_monitor_release_write(&ctx->monitor);
}

void _sc_memory_context_pending_end(sc_memory_context * ctx)
{
  sc_monitor_acquire_write(&ctx->monitor);
  ctx->flags = ctx->flags & (~SC_CONTEXT_FLAG_PENDING_EVENTS);
  _sc_memory_context_emit_events(ctx);
  sc_monitor_release_write(&ctx->monitor);
}
