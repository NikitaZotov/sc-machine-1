/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_memory.h"
#include "sc_memory_private.h"
#include "sc_memory_params.h"

#include "sc_memory_ext.h"
#include "sc_helper_private.h"

#include "sc-store/sc_storage.h"
#include "sc-store/sc_types.h"

#include "sc-store/sc_event/sc_event_private.h"

#include "sc-store/sc-base/sc_allocator.h"
#include "sc-store/sc-base/sc_message.h"
#include "sc-store/sc-base/sc_monitor.h"
#include "sc-store/sc-container/sc-hash-table/sc_hash_table.h"

#include "sc_helper.h"

struct _sc_memory
{
  sc_hash_table * context_hash_table;
  sc_uint32 last_context_id;
  sc_uint32 context_count;
  sc_monitor context_monitor;
};

sc_memory * memory = null_ptr;
sc_memory_context * s_memory_default_ctx = null_ptr;

sc_memory_context * sc_memory_initialize(sc_memory_params const * params)
{
  sc_memory_info("Initialize");

  sc_char * string = sc_version_string_new(&params->version);
  sc_memory_info("Version: %s", string);
  sc_version_string_free(string);

  sc_memory_info("Logger:");
  sc_message("\tLog type: %s", params->log_type);
  sc_message("\tLog file: %s", params->log_file);
  sc_message("\tLog level: %s", params->log_level);

  if (sc_storage_initialize(params) == SC_FALSE)
  {
    sc_memory_error("Error while initialize sc-storage");
    goto error;
  }

  sc_storage_start_new_process();

  memory = sc_mem_new(sc_memory, 1);
  memory->context_hash_table = sc_hash_table_init(g_direct_hash, g_direct_equal, null_ptr, null_ptr);
  memory->last_context_id = 0;
  memory->context_count = 0;
  sc_monitor_init(&memory->context_monitor);
  s_memory_default_ctx = sc_memory_context_new(sc_access_lvl_make(SC_ACCESS_LVL_MAX_VALUE, SC_ACCESS_LVL_MAX_VALUE));

  sc_memory_context * helper_ctx =
      sc_memory_context_new(sc_access_lvl_make(SC_ACCESS_LVL_MIN_VALUE, SC_ACCESS_LVL_MAX_VALUE));
  if (sc_helper_init(helper_ctx) != SC_RESULT_OK)
  {
    sc_memory_context_free(helper_ctx);
    sc_memory_error("Error while initialize sc-helper");
    goto error;
  }
  sc_memory_context_free(helper_ctx);

  sc_memory_info("Build configuration:");
  sc_message("\tResult structure upload: %s", params->init_memory_generated_upload ? "On" : "Off");
  sc_message("\tInit memory generated structure: %s", params->init_memory_generated_structure);
  sc_message("\tExtensions path: %s", params->ext_path);

  sc_addr init_memory_generated_structure = SC_ADDR_EMPTY;
  if (params->init_memory_generated_upload)
    sc_helper_resolve_system_identifier(
        s_memory_default_ctx, params->init_memory_generated_structure, &init_memory_generated_structure);

  if (sc_memory_init_ext(params->ext_path, params->enabled_exts, init_memory_generated_structure) != SC_RESULT_OK)
  {
    sc_memory_error("Error while initialize extensions");
    goto error;
  }

  sc_storage_end_new_process();

  sc_memory_info("Successfully initialized");
  return s_memory_default_ctx;

error:
{
  sc_storage_end_new_process();

  sc_memory_context_free(s_memory_default_ctx);
  s_memory_default_ctx = null_ptr;
  sc_memory_info("Initialized with errors");
  return null_ptr;
}
}

sc_result sc_memory_init_ext(
    sc_char const * ext_path,
    sc_char const ** enabled_list,
    sc_addr const init_memory_generated_structure)
{
  sc_memory_info("Initialize extensions");

  sc_result const ext_res = sc_ext_initialize(ext_path, enabled_list, init_memory_generated_structure);

  switch (ext_res)
  {
  case SC_RESULT_OK:
    sc_memory_info("Extensions initialized");
    break;

  case SC_RESULT_ERROR_INVALID_PARAMS:
    sc_memory_warning("Extensions directory `%s` doesn't exist", ext_path);
    break;

  default:
    sc_memory_warning("Unknown error while extensions initializing");
    break;
  }

  return ext_res;
}

void sc_memory_shutdown(sc_bool save_state)
{
  sc_memory_info("Shutdown");

  sc_memory_shutdown_ext();
  sc_helper_shutdown();

  sc_storage_shutdown(save_state);

  sc_memory_context_free(s_memory_default_ctx);
  s_memory_default_ctx = null_ptr;

  sc_memory_info("Clear contexts");
  if (memory->context_hash_table != null_ptr)
  {
    sc_monitor_acquire_read(&memory->context_monitor);
    sc_uint32 context_count = sc_hash_table_size(memory->context_hash_table);
    sc_monitor_release_read(&memory->context_monitor);
    if (context_count > 0)
      sc_memory_warning("There are %d contexts, wasn't destroyed before sc-memory shutdown", context_count);

    sc_monitor_acquire_write(&memory->context_monitor);
    sc_hash_table_destroy(memory->context_hash_table);
    memory->context_hash_table = null_ptr;
    sc_monitor_release_write(&memory->context_monitor);
  }

  sc_monitor_destroy(&memory->context_monitor);
  sc_mem_free(memory);
  memory = null_ptr;

  sc_memory_info("Shutdown");
}

void sc_memory_shutdown_ext()
{
  sc_ext_shutdown();
}

sc_memory_context * sc_memory_context_new(sc_uint8 levels)
{
  return sc_memory_context_new_impl(levels);
}

sc_memory_context * sc_memory_context_new_impl(sc_uint8 levels)
{
  if (memory == null_ptr)
    return null_ptr;

  sc_memory_context * ctx = sc_mem_new(sc_memory_context, 1);
  sc_uint32 index;

  ctx->access_levels = levels;

  // setup concurrency id
  sc_monitor_acquire_write(&memory->context_monitor);
  if (memory->context_count >= G_MAXUINT32)
    goto error;

  if (memory->context_hash_table == null_ptr)
    goto error;

  index = (memory->last_context_id + 1) % G_MAXUINT32;
  while (index == 0 ||
         (index != memory->last_context_id && sc_hash_table_get(memory->context_hash_table, GINT_TO_POINTER(index))))
    index = (index + 1) % G_MAXUINT32;

  if (index != memory->last_context_id)
  {
    ctx->id = index;
    sc_monitor_init(&ctx->monitor);

    memory->last_context_id = index;
    sc_hash_table_insert(memory->context_hash_table, GINT_TO_POINTER(ctx->id), (sc_pointer)ctx);
  }
  else
    goto error;

  ++memory->context_count;
  goto result;

error:
{
  sc_mem_free(ctx);
  ctx = null_ptr;
}

result:
  sc_monitor_release_write(&memory->context_monitor);

  return ctx;
}

void sc_memory_context_free(sc_memory_context * ctx)
{
  sc_memory_context_free_impl(ctx);
}

void sc_memory_context_free_impl(sc_memory_context * ctx)
{
  if (ctx == null_ptr)
    return;

  sc_monitor_acquire_write(&memory->context_monitor);

  if (memory->context_hash_table == null_ptr)
    goto error;

  sc_memory_context * context = sc_hash_table_get(memory->context_hash_table, GINT_TO_POINTER(ctx->id));
  if (context == null_ptr)
    goto error;

  sc_monitor_destroy(&ctx->monitor);
  sc_hash_table_remove(memory->context_hash_table, GINT_TO_POINTER(ctx->id));
  --memory->context_count;

error:
  sc_monitor_release_write(&memory->context_monitor);

  sc_mem_free(ctx);
}

void sc_memory_context_pending_begin(sc_memory_context * ctx)
{
  sc_monitor_acquire_write(&ctx->monitor);
  ctx->flags |= SC_CONTEXT_FLAG_PENDING_EVENTS;
  sc_monitor_release_write(&ctx->monitor);
}

void sc_memory_context_pending_end(sc_memory_context * ctx)
{
  sc_monitor_acquire_write(&ctx->monitor);
  ctx->flags = ctx->flags & (~SC_CONTEXT_FLAG_PENDING_EVENTS);
  sc_memory_context_emit_events(ctx);
  sc_monitor_release_write(&ctx->monitor);
}

void sc_memory_context_pend_event(sc_memory_context const * ctx, sc_event_emit_params * params)
{
  sc_monitor_acquire_write((sc_monitor *)&ctx->monitor);
  ((sc_memory_context *)ctx)->pend_events = g_slist_append(ctx->pend_events, params);
  sc_monitor_release_write((sc_monitor *)&ctx->monitor);
}

void sc_memory_context_emit_events(sc_memory_context const * ctx)
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

sc_bool sc_memory_is_initialized()
{
  return sc_storage_is_initialized();
}

sc_bool sc_memory_is_element(sc_memory_context const * ctx, sc_addr addr)
{
  return sc_storage_is_element(ctx, addr);
}

sc_uint32 sc_memory_get_element_output_arcs_count(sc_memory_context const * ctx, sc_addr addr)
{
  return sc_storage_get_element_output_arcs_count(ctx, addr);
}

sc_uint32 sc_memory_get_element_input_arcs_count(sc_memory_context const * ctx, sc_addr addr)
{
  return sc_storage_get_element_input_arcs_count(ctx, addr);
}

sc_result sc_memory_element_free(sc_memory_context * ctx, sc_addr addr)
{
  return sc_storage_element_free(ctx, addr);
}

sc_addr sc_memory_node_new(const sc_memory_context * ctx, sc_type type)
{
  return sc_storage_node_new(ctx, type);
}

sc_addr sc_memory_link_new(sc_memory_context const * ctx)
{
  return sc_memory_link_new2(ctx, sc_type_const);
}

sc_addr sc_memory_link_new2(sc_memory_context const * ctx, sc_type type)
{
  return sc_storage_link_new(ctx, type);
}

sc_addr sc_memory_arc_new(sc_memory_context * ctx, sc_type type, sc_addr beg, sc_addr end)
{
  return sc_storage_arc_new(ctx, type, beg, end);
}

sc_result sc_memory_get_element_type(sc_memory_context const * ctx, sc_addr addr, sc_type * result)
{
  return sc_storage_get_element_type(ctx, addr, result);
}

sc_result sc_memory_change_element_subtype(sc_memory_context const * ctx, sc_addr addr, sc_type type)
{
  return sc_storage_change_element_subtype(ctx, addr, type);
}

sc_result sc_memory_get_arc_begin(sc_memory_context const * ctx, sc_addr addr, sc_addr * result)
{
  return sc_storage_get_arc_begin(ctx, addr, result);
}

sc_result sc_memory_get_arc_end(sc_memory_context const * ctx, sc_addr addr, sc_addr * result)
{
  return sc_storage_get_arc_end(ctx, addr, result);
}

sc_result sc_memory_get_arc_info(
    sc_memory_context const * ctx,
    sc_addr addr,
    sc_addr * result_start_addr,
    sc_addr * result_end_addr)
{
  return sc_storage_get_arc_info(ctx, addr, result_start_addr, result_end_addr);
}

sc_result sc_memory_set_link_content(sc_memory_context const * ctx, sc_addr addr, const sc_stream * stream)
{
  return sc_memory_set_link_content_ext(ctx, addr, stream, SC_TRUE);
}

sc_result sc_memory_set_link_content_ext(
    sc_memory_context const * ctx,
    sc_addr addr,
    const sc_stream * stream,
    sc_bool is_searchable_string)
{
  return sc_storage_set_link_content_ext(ctx, addr, stream, is_searchable_string);
}

sc_result sc_memory_get_link_content(sc_memory_context const * ctx, sc_addr addr, sc_stream ** stream)
{
  return sc_storage_get_link_content(ctx, addr, stream);
}

sc_result sc_memory_find_links_with_content_string(
    sc_memory_context const * ctx,
    sc_stream const * stream,
    sc_list ** result)
{
  return sc_storage_find_links_with_content_string(ctx, stream, result);
}

sc_result sc_memory_find_links_by_content_substring(
    sc_memory_context const * ctx,
    sc_stream const * stream,
    sc_list ** result,
    sc_uint32 max_length_to_search_as_prefix)
{
  return sc_storage_find_links_by_content_substring(ctx, stream, result, max_length_to_search_as_prefix);
}

sc_result sc_memory_find_links_contents_by_content_substring(
    sc_memory_context const * ctx,
    sc_stream const * stream,
    sc_list ** result,
    sc_uint32 max_length_to_search_as_prefix)
{
  return sc_storage_find_links_contents_by_content_substring(ctx, stream, result, max_length_to_search_as_prefix);
}

sc_result sc_memory_stat(sc_memory_context const * ctx, sc_stat * stat)
{
  return sc_storage_get_elements_stat(stat);
}

sc_result sc_memory_save(sc_memory_context const * ctx)
{
  return sc_storage_save(ctx);
}
