/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_memory_context_manager.h"

#include "sc-store/sc-base/sc_allocator.h"

#include "sc_memory.h"
#include "sc_memory_private.h"
#include "sc-store/sc_event/sc_event_private.h"
#include "sc_helper.h"
#include "sc-store/sc_iterator3.h"
#include "sc-store/sc_iterator5.h"

struct _sc_memory_context_manager
{
  sc_hash_table * context_hash_table;
  sc_uint32 last_context_id;
  sc_uint32 context_count;
  sc_monitor context_monitor;
  sc_addr system_addr;
  sc_hash_table * actor_class_hash_table;
  sc_hash_table * actor_instance_hash_table;
  sc_monitor actor_monitor;
  sc_hash_table * action_class_hash_table;
  sc_access_levels last_action_bit;
  sc_monitor action_monitor;
};

struct _sc_event_emit_params
{
  sc_addr el;
  sc_access_levels el_access;
  sc_event_type type;
  sc_addr edge;
  sc_addr other_el;
};

struct _sc_memory_context
{
  sc_uint32 id;
  sc_access_levels access_levels;
  sc_uint8 flags;
  sc_hash_table_list * pend_events;
  sc_addr actor_addr;
  sc_monitor monitor;
};

sc_addr action_actor_addr = SC_ADDR_EMPTY;
sc_addr nrel_accessible_action_in_sc_memory_addr = SC_ADDR_EMPTY;
sc_addr action_in_sc_memory_class_addr = SC_ADDR_EMPTY;

sc_event * action_actor_class_add_event = null_ptr;
sc_event * action_actor_class_remove_event = null_ptr;
sc_event * action_actor_class_access_add_event = null_ptr;
sc_event * action_actor_class_access_remove_event = null_ptr;
sc_event * action_class_add_event = null_ptr;
sc_event * action_class_remove_event = null_ptr;

#define SC_CONTEXT_FLAG_PENDING_EVENTS 0x1

sc_result _sc_memory_context_manager_add_action_actor(
    sc_event const * event,
    sc_addr arc_addr,
    sc_addr actor_class_addr)
{
  (void)arc_addr;
  sc_memory_context_manager * manager = event->data;

  sc_iterator3 * actor_superclass_it =
      sc_iterator3_a_a_f_new(s_memory_default_ctx, sc_type_node_const, sc_type_arc_pos_const_perm, actor_class_addr);
  while (sc_iterator3_next(actor_superclass_it))
  {
    sc_addr const actor_superclass_addr = sc_iterator3_value(actor_superclass_it, 0);
    sc_iterator5 * actor_superclass_action_it = sc_iterator5_f_a_a_a_f_new(
        s_memory_default_ctx,
        actor_superclass_addr,
        sc_type_arc_common_const,
        sc_type_node_const,
        sc_type_arc_pos_const_perm,
        nrel_accessible_action_in_sc_memory_addr);
    while (sc_iterator5_next(actor_superclass_action_it))
    {
      sc_addr const action_class_addr = sc_iterator5_value(actor_superclass_action_it, 2);
      sc_monitor_acquire_read(&manager->action_monitor);
      sc_pointer key = TABLE_KEY(action_class_addr);
      sc_access_levels const action_bit = (sc_uint64)sc_hash_table_get(manager->action_class_hash_table, key);
      sc_monitor_release_read(&manager->action_monitor);

      sc_monitor_acquire_write(&manager->actor_monitor);
      key = TABLE_KEY(actor_class_addr);
      sc_access_levels action_rights = (sc_uint64)sc_hash_table_get(manager->actor_class_hash_table, key);
      action_rights |= action_bit;
      sc_hash_table_insert(manager->actor_class_hash_table, key, (sc_pointer)(sc_uint64)action_rights);
      sc_monitor_release_write(&manager->actor_monitor);
    }
    sc_iterator5_free(actor_superclass_action_it);
  }
  sc_iterator3_free(actor_superclass_it);

  sc_iterator3 * actor_instance_it =
      sc_iterator3_f_a_a_new(s_memory_default_ctx, actor_class_addr, sc_type_arc_pos_const_perm, sc_type_node_const);
  while (sc_iterator3_next(actor_instance_it))
  {
    sc_addr const action_instance_addr = sc_iterator3_value(actor_instance_it, 2);
    sc_monitor_acquire_write(&manager->actor_monitor);
    sc_pointer key = TABLE_KEY(action_instance_addr);
    sc_hash_table_insert(manager->actor_instance_hash_table, key, TABLE_KEY(actor_class_addr));
    sc_monitor_release_write(&manager->actor_monitor);
  }
  sc_iterator3_free(actor_instance_it);

  return SC_RESULT_OK;
}

sc_result _sc_memory_context_manager_remove_action_actor(
    sc_event const * event,
    sc_addr arc_addr,
    sc_addr actor_class_addr)
{
  (void)arc_addr;
  sc_memory_context_manager * manager = event->data;

  sc_monitor_acquire_write(&manager->actor_monitor);
  sc_hash_table_remove(manager->actor_class_hash_table, TABLE_KEY(actor_class_addr));
  sc_monitor_release_write(&manager->actor_monitor);

  return SC_RESULT_OK;
}

void _sc_memory_context_manager_init_actors_table(sc_memory_context_manager * manager)
{
  (void)manager;

  sc_iterator3 * actor_it =
      sc_iterator3_f_a_a_new(s_memory_default_ctx, action_actor_addr, sc_type_arc_pos_const_perm, sc_type_node_const);
  while (sc_iterator3_next(actor_it))
  {
    sc_addr const actor_class_edge_addr = sc_iterator3_value(actor_it, 1);
    sc_addr const actor_class_addr = sc_iterator3_value(actor_it, 2);

    _sc_memory_context_manager_add_action_actor(action_actor_class_add_event, actor_class_edge_addr, actor_class_addr);
  }
  sc_iterator3_free(actor_it);
}

sc_result _sc_memory_context_manager_add_action_actor_access(
    sc_event const * event,
    sc_addr arc_addr,
    sc_addr actor_class_access_edge_addr)
{
  (void)arc_addr;
  sc_memory_context_manager * manager = event->data;

  sc_addr actor_class_addr;
  sc_addr action_class_addr;
  sc_memory_get_arc_info(s_memory_default_ctx, actor_class_access_edge_addr, &actor_class_addr, &action_class_addr);

  sc_monitor_acquire_read(&manager->action_monitor);
  sc_pointer key = TABLE_KEY(action_class_addr);
  sc_access_levels const action_bit = (sc_uint64)sc_hash_table_get(manager->action_class_hash_table, key);
  sc_monitor_release_read(&manager->action_monitor);

  sc_monitor_acquire_write(&manager->actor_monitor);
  key = TABLE_KEY(actor_class_addr);
  sc_access_levels action_rights = (sc_uint64)sc_hash_table_get(manager->actor_class_hash_table, key);
  action_rights |= action_bit;
  sc_hash_table_insert(manager->actor_class_hash_table, key, (sc_pointer)(sc_uint64)action_rights);
  sc_monitor_release_write(&manager->actor_monitor);

  return SC_RESULT_OK;
}

sc_result _sc_memory_context_manager_remove_action_actor_access(
    sc_event const * event,
    sc_addr arc_addr,
    sc_addr actor_class_access_edge_addr)
{
  (void)arc_addr;
  sc_memory_context_manager * manager = event->data;

  sc_addr actor_class_addr;
  sc_addr action_class_addr;
  sc_memory_get_arc_info(s_memory_default_ctx, actor_class_access_edge_addr, &actor_class_addr, &action_class_addr);

  sc_monitor_acquire_read(&manager->action_monitor);
  sc_pointer key = TABLE_KEY(action_class_addr);
  sc_access_levels const action_bit = (sc_uint64)sc_hash_table_get(manager->action_class_hash_table, key);
  sc_monitor_release_read(&manager->action_monitor);

  sc_monitor_acquire_write(&manager->actor_monitor);
  key = TABLE_KEY(actor_class_addr);
  sc_access_levels action_rights = (sc_uint64)sc_hash_table_get(manager->actor_class_hash_table, key);
  action_rights &= ~action_bit;
  sc_hash_table_insert(manager->actor_class_hash_table, key, (sc_pointer)(sc_uint64)action_rights);
  sc_monitor_release_write(&manager->actor_monitor);

  return SC_RESULT_OK;
}

sc_result _sc_memory_context_manager_add_action(sc_event const * event, sc_addr arc_addr, sc_addr action_class_addr)
{
  (void)arc_addr;
  sc_memory_context_manager * manager = event->data;

  sc_monitor_acquire_write(&manager->action_monitor);
  sc_pointer key = TABLE_KEY(action_class_addr);
  if (sc_hash_table_get(manager->action_class_hash_table, key) == null_ptr)
  {
    sc_hash_table_insert(manager->action_class_hash_table, key, (sc_pointer)(sc_uint64)manager->last_action_bit);
    manager->last_action_bit = manager->last_action_bit << 1;
  }
  sc_monitor_release_write(&manager->action_monitor);

  return SC_RESULT_OK;
}

sc_result _sc_memory_context_manager_remove_action(sc_event const * event, sc_addr arc_addr, sc_addr action_class_addr)
{
  (void)arc_addr;
  sc_memory_context_manager * manager = event->data;

  sc_monitor_acquire_write(&manager->action_monitor);
  sc_hash_table_remove(manager->action_class_hash_table, TABLE_KEY(action_class_addr));
  sc_monitor_release_write(&manager->action_monitor);

  return SC_RESULT_OK;
}

void _sc_memory_context_manager_init_actions_table(sc_memory_context_manager * manager)
{
  (void)manager;

  sc_iterator3 * action_it = sc_iterator3_f_a_a_new(
      s_memory_default_ctx, action_in_sc_memory_class_addr, sc_type_arc_pos_const_perm, sc_type_node_const);
  while (sc_iterator3_next(action_it))
  {
    sc_addr const action_class_edge_addr = sc_iterator3_value(action_it, 1);
    sc_addr const action_class_addr = sc_iterator3_value(action_it, 2);

    _sc_memory_context_manager_add_action(action_class_add_event, action_class_edge_addr, action_class_addr);
  }
  sc_iterator3_free(action_it);
}

void _sc_memory_context_manager_register_events(sc_memory_context_manager * manager)
{
  sc_helper_resolve_system_identifier(s_memory_default_ctx, "action_actor", &action_actor_addr);
  sc_helper_resolve_system_identifier(
      s_memory_default_ctx, "nrel_accessible_action_in_sc_memory", &nrel_accessible_action_in_sc_memory_addr);
  sc_helper_resolve_system_identifier(s_memory_default_ctx, "action_in_sc_memory", &action_in_sc_memory_class_addr);

  action_actor_class_add_event = sc_event_new_ex(
      s_memory_default_ctx,
      action_actor_addr,
      SC_EVENT_ADD_OUTPUT_ARC,
      manager,
      _sc_memory_context_manager_add_action_actor,
      null_ptr);
  action_actor_class_remove_event = sc_event_new_ex(
      s_memory_default_ctx,
      action_actor_addr,
      SC_EVENT_REMOVE_OUTPUT_ARC,
      manager,
      _sc_memory_context_manager_remove_action_actor,
      null_ptr);

  action_actor_class_access_add_event = sc_event_new_ex(
      s_memory_default_ctx,
      nrel_accessible_action_in_sc_memory_addr,
      SC_EVENT_ADD_OUTPUT_ARC,
      manager,
      _sc_memory_context_manager_add_action_actor_access,
      null_ptr);
  action_actor_class_access_remove_event = sc_event_new_ex(
      s_memory_default_ctx,
      nrel_accessible_action_in_sc_memory_addr,
      SC_EVENT_REMOVE_OUTPUT_ARC,
      manager,
      _sc_memory_context_manager_remove_action_actor_access,
      null_ptr);

  action_class_add_event = sc_event_new_ex(
      s_memory_default_ctx,
      action_in_sc_memory_class_addr,
      SC_EVENT_ADD_OUTPUT_ARC,
      manager,
      _sc_memory_context_manager_add_action,
      null_ptr);
  action_class_remove_event = sc_event_new_ex(
      s_memory_default_ctx,
      action_in_sc_memory_class_addr,
      SC_EVENT_REMOVE_OUTPUT_ARC,
      manager,
      _sc_memory_context_manager_remove_action,
      null_ptr);
}

void _sc_memory_context_manager_unregister_events()
{
  sc_event_destroy(action_actor_class_add_event);
  sc_event_destroy(action_actor_class_remove_event);

  sc_event_destroy(action_actor_class_access_add_event);
  sc_event_destroy(action_actor_class_access_remove_event);

  sc_event_destroy(action_class_add_event);
  sc_event_destroy(action_class_remove_event);
}

void _sc_memory_context_manager_initialize(sc_memory_context_manager ** manager)
{
  sc_memory_info("Initialize context manager");

  *manager = sc_mem_new(sc_memory_context_manager, 1);
  (*manager)->context_hash_table = sc_hash_table_init(g_direct_hash, g_direct_equal, null_ptr, null_ptr);
  (*manager)->last_context_id = 0;
  (*manager)->context_count = 0;
  sc_monitor_init(&(*manager)->context_monitor);

  s_memory_default_ctx = sc_memory_context_new(sc_access_lvl_make_max);

  (*manager)->actor_class_hash_table = sc_hash_table_init(g_direct_hash, g_direct_equal, null_ptr, null_ptr);
  (*manager)->actor_instance_hash_table = sc_hash_table_init(g_direct_hash, g_direct_equal, null_ptr, null_ptr);
  sc_monitor_init(&(*manager)->actor_monitor);

  (*manager)->action_class_hash_table = sc_hash_table_init(g_direct_hash, g_direct_equal, null_ptr, null_ptr);
  (*manager)->last_action_bit = 1;
  sc_monitor_init(&(*manager)->action_monitor);
}

void _sc_memory_context_manager_load_actors_and_actions(sc_memory_context_manager * manager)
{
  sc_helper_resolve_system_identifier(s_memory_default_ctx, "my_self", &manager->system_addr);
  s_memory_default_ctx->actor_addr = manager->system_addr;

  _sc_memory_context_manager_register_events(manager);
  _sc_memory_context_manager_init_actors_table(manager);
  _sc_memory_context_manager_init_actions_table(manager);
}

void _sc_memory_context_manager_shutdown(sc_memory_context_manager * manager)
{
  sc_memory_info("Shutdown context manager");

  _sc_memory_context_manager_unregister_events();

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

  sc_monitor_acquire_write(&manager->actor_monitor);
  sc_hash_table_destroy(manager->actor_class_hash_table);
  manager->actor_class_hash_table = null_ptr;
  sc_hash_table_destroy(manager->actor_instance_hash_table);
  manager->actor_instance_hash_table = null_ptr;
  sc_monitor_release_write(&manager->actor_monitor);

  sc_monitor_destroy(&manager->actor_monitor);

  sc_monitor_acquire_write(&manager->action_monitor);
  sc_hash_table_destroy(manager->action_class_hash_table);
  manager->action_class_hash_table = null_ptr;
  sc_monitor_release_write(&manager->action_monitor);

  sc_monitor_destroy(&manager->action_monitor);

  sc_mem_free(manager);
}

sc_memory_context * _sc_memory_context_new(sc_memory_context_manager * manager)
{
  sc_memory_context * ctx = sc_mem_new(sc_memory_context, 1);
  sc_uint32 index;

  // setup concurrency id
  sc_monitor_acquire_write(&manager->context_monitor);
  if (manager->context_count >= G_MAXUINT32)
    goto error;

  if (manager->context_hash_table == null_ptr)
    goto error;

  index = (manager->last_context_id + 1) % G_MAXUINT32;
  while (index == 0 ||
         (index != manager->last_context_id && sc_hash_table_get(manager->context_hash_table, GINT_TO_POINTER(index))))
    index = (index + 1) % G_MAXUINT32;

  if (index != manager->last_context_id)
  {
    ctx->id = index;
    sc_monitor_init(&ctx->monitor);

    manager->last_context_id = index;
    sc_hash_table_insert(manager->context_hash_table, GINT_TO_POINTER(ctx->id), (sc_pointer)ctx);
  }
  else
    goto error;

  ++manager->context_count;
  goto result;

error:
{
  sc_mem_free(ctx);
  ctx = null_ptr;
}

result:
  sc_monitor_release_write(&manager->context_monitor);

  return ctx;
}

sc_memory_context * _sc_memory_context_new_impl(
    sc_memory_context_manager * manager,
    sc_access_levels levels,
    sc_addr actor_addr)
{
  if (manager == null_ptr)
    return null_ptr;

  sc_memory_context * ctx = _sc_memory_context_new(manager);
  if (ctx == null_ptr)
    return null_ptr;

  ctx->access_levels = levels;
  ctx->actor_addr = actor_addr;

  return ctx;
}

void _sc_memory_context_free_impl(sc_memory_context_manager * manager, sc_memory_context * ctx)
{
  if (ctx == null_ptr)
    return;

  sc_monitor_acquire_write(&manager->context_monitor);

  if (manager->context_hash_table == null_ptr)
    goto error;

  sc_memory_context * context = sc_hash_table_get(manager->context_hash_table, GINT_TO_POINTER(ctx->id));
  if (context == null_ptr)
    goto error;

  sc_monitor_destroy(&ctx->monitor);
  sc_hash_table_remove(manager->context_hash_table, GINT_TO_POINTER(ctx->id));
  --manager->context_count;

error:
  sc_monitor_release_write(&manager->context_monitor);

  sc_mem_free(ctx);
}

sc_bool _sc_memory_context_check_rights(
    sc_memory_context_manager * manager,
    sc_memory_context const * ctx,
    sc_addr action_addr)
{
  sc_bool result = SC_TRUE;
  sc_monitor_acquire_read((sc_monitor *)&ctx->monitor);

  if (ctx->access_levels == sc_access_lvl_make_max)
    goto result;

  sc_pointer actor_key = TABLE_KEY(ctx->actor_addr);
  sc_monitor_acquire_read(&manager->actor_monitor);
  sc_addr_hash actor_class_addr_hash = (sc_addr_hash)sc_hash_table_get(manager->actor_instance_hash_table, actor_key);
  sc_access_levels actor_rights =
      (sc_uint64)sc_hash_table_get(manager->actor_class_hash_table, (sc_pointer)actor_class_addr_hash);
  sc_monitor_release_read(&manager->actor_monitor);

  sc_pointer action_key = TABLE_KEY(action_addr);
  sc_monitor_acquire_read(&manager->action_monitor);
  sc_access_levels action_rights = (sc_uint64)sc_hash_table_get(manager->action_class_hash_table, action_key);
  sc_monitor_release_read(&manager->action_monitor);

  result = ((actor_rights & action_rights) == action_rights);

result:
  sc_monitor_release_read((sc_monitor *)&ctx->monitor);
  return result;
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
  params->el = element;
  params->el_access = sc_access_lvl_make_max;
  params->edge = edge;
  params->other_el = other_element;

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
        ctx, evt_params->el, evt_params->el_access, evt_params->type, evt_params->edge, evt_params->other_el);

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
