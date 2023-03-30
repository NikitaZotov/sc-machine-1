/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_iterator.h"

#include "../sc_memory.h"

#include "sc-base/sc_allocator.h"
#include "sc-container/sc-iterator/sc_container_iterator.h"
#include "sc-fs-memory/sc_io.h"

sc_iterator3 * sc_iterator3_f_a_a_new_ext(sc_storage const * storage, sc_addr el, sc_type arc_type, sc_type end_type)
{
  sc_iterator_param p1, p2, p3;

  p1.is_type = SC_FALSE;
  p1.addr = el;

  p2.is_type = SC_TRUE;
  p2.type = arc_type;

  p3.is_type = SC_TRUE;
  p3.type = end_type;

  return sc_iterator3_new_ext(storage, sc_iterator3_f_a_a, p1, p2, p3);
}

sc_iterator3 * sc_iterator3_f_a_a_new(sc_memory_context const * context, sc_addr el, sc_type arc_type, sc_type end_type)
{
  return sc_iterator3_f_a_a_new_ext(sc_memory_context_get_storage(context), el, arc_type, end_type);
}

sc_iterator3 * sc_iterator3_a_a_f_new_ext(sc_storage const * storage, sc_type beg_type, sc_type arc_type, sc_addr el)
{
  sc_iterator_param p1, p2, p3;

  p1.is_type = SC_TRUE;
  p1.type = beg_type;

  p2.is_type = SC_TRUE;
  p2.type = arc_type;

  p3.is_type = SC_FALSE;
  p3.addr = el;

  return sc_iterator3_new_ext(storage, sc_iterator3_a_a_f, p1, p2, p3);
}

sc_iterator3 * sc_iterator3_a_a_f_new(sc_memory_context const * context, sc_type beg_type, sc_type arc_type, sc_addr el)
{
  return sc_iterator3_a_a_f_new_ext(sc_memory_context_get_storage(context), beg_type, arc_type, el);
}

sc_iterator3 * sc_iterator3_f_a_f_new_ext(sc_storage const * storage, sc_addr el_beg, sc_type arc_type, sc_addr el_end)
{
  sc_iterator_param p1, p2, p3;

  p1.is_type = SC_FALSE;
  p1.addr = el_beg;

  p2.is_type = SC_TRUE;
  p2.type = arc_type;

  p3.is_type = SC_FALSE;
  p3.addr = el_end;

  return sc_iterator3_new_ext(storage, sc_iterator3_f_a_f, p1, p2, p3);
}

sc_iterator3 * sc_iterator3_f_a_f_new(sc_memory_context const * context, sc_addr el_beg, sc_type arc_type, sc_addr el_end)
{
  return sc_iterator3_f_a_f_new_ext(sc_memory_context_get_storage(context), el_beg, arc_type, el_end);
}

sc_iterator3 * sc_iterator3_a_f_a_new_ext(sc_storage const * storage, sc_type beg_type, sc_addr arc_addr, sc_type end_type)
{
  sc_iterator_param p1, p2, p3;

  p1.is_type = SC_TRUE;
  p1.type = beg_type;

  p2.is_type = SC_FALSE;
  p2.addr = arc_addr;

  p3.is_type = SC_TRUE;
  p3.type = end_type;

  return sc_iterator3_new_ext(storage, sc_iterator3_a_f_a, p1, p2, p3);
}

sc_iterator3 * sc_iterator3_a_f_a_new(sc_memory_context const * context, sc_type beg_type, sc_addr arc_addr, sc_type end_type)
{
  return sc_iterator3_a_f_a_new_ext(sc_memory_context_get_storage(context), beg_type, arc_addr, end_type);
}

sc_iterator3 * sc_iterator3_f_f_a_new_ext(sc_storage const * storage, sc_addr beg_addr, sc_addr edge_addr, sc_type end_type)
{
  sc_iterator_param p1, p2, p3;

  p1.is_type = SC_FALSE;
  p1.addr = beg_addr;

  p2.is_type = SC_FALSE;
  p2.addr = edge_addr;

  p3.is_type = SC_TRUE;
  p3.type = end_type;

  return sc_iterator3_new_ext(storage, sc_iterator3_f_f_a, p1, p2, p3);
}

sc_iterator3 * sc_iterator3_f_f_a_new(sc_memory_context const * context, sc_addr beg_addr, sc_addr arc_addr, sc_type end_type)
{
  return sc_iterator3_f_f_a_new_ext(sc_memory_context_get_storage(context), beg_addr, arc_addr, end_type);
}

sc_iterator3 * sc_iterator3_a_f_f_new_ext(sc_storage const * storage, sc_type beg_type, sc_addr edge_addr, sc_addr end_addr)
{
  sc_iterator_param p1, p2, p3;

  p1.is_type = SC_TRUE;
  p1.type = beg_type;

  p2.is_type = SC_FALSE;
  p2.addr = edge_addr;

  p3.is_type = SC_FALSE;
  p3.addr = end_addr;

  return sc_iterator3_new_ext(storage, sc_iterator3_a_f_f, p1, p2, p3);
}

sc_iterator3 * sc_iterator3_a_f_f_new(sc_memory_context const * context, sc_type beg_type, sc_addr arc_addr, sc_addr end_addr)
{
  return sc_iterator3_a_f_f_new_ext(sc_memory_context_get_storage(context), beg_type, arc_addr, end_addr);
}

sc_iterator3 * sc_iterator3_f_f_f_new_ext(sc_storage const * storage, sc_addr beg_addr, sc_addr edge_addr, sc_addr end_addr)
{
  sc_iterator_param p1, p2, p3;

  p1.is_type = SC_FALSE;
  p1.addr = beg_addr;

  p2.is_type = SC_FALSE;
  p2.addr = edge_addr;

  p3.is_type = SC_FALSE;
  p3.addr = end_addr;

  return sc_iterator3_new_ext(storage, sc_iterator3_f_f_f, p1, p2, p3);
}

sc_iterator3 * sc_iterator3_f_f_f_new(sc_memory_context const * context, sc_addr beg_addr, sc_addr arc_addr, sc_addr end_addr)
{
  return sc_iterator3_f_f_f_new_ext(sc_memory_context_get_storage(context), beg_addr, arc_addr, end_addr);
}

sc_iterator3 * sc_iterator3_new_ext(
    sc_storage const * storage,
    sc_iterator3_type type,
    sc_iterator_param p1,
    sc_iterator_param p2,
    sc_iterator_param p3)
{
  // check types
  if (type >= sc_iterator3_count)
    return null_ptr;

  // check params with template
  switch (type)
  {
  case sc_iterator3_f_a_a:
    if (p1.is_type || !p2.is_type || !p3.is_type)
    {
      return null_ptr;
    }
    break;

  case sc_iterator3_a_a_f:
    if (!p1.is_type || !p2.is_type || p3.is_type)
    {
      return null_ptr;
    }
    break;

  case sc_iterator3_f_a_f:
    if (p1.is_type || !p2.is_type || p3.is_type)
    {
      return null_ptr;
    }
    break;

  case sc_iterator3_a_f_a:
    if (!p1.is_type || p2.is_type || !p3.is_type)
    {
      return null_ptr;
    }
    break;

  case sc_iterator3_f_f_a:
    if (p1.is_type || p2.is_type || !p3.is_type)
    {
      return null_ptr;
    }
    break;

  case sc_iterator3_a_f_f:
    if (!p1.is_type || p2.is_type || p3.is_type)
    {
      return null_ptr;
    }
    break;

  case sc_iterator3_f_f_f:
    if (p1.is_type || p2.is_type || p3.is_type)
    {
      return null_ptr;
    }
    break;

  default:
    break;
  }

  sc_iterator3 * it = sc_mem_new(sc_iterator3, 1);

  it->params[0] = p1;
  it->params[1] = p2;
  it->params[2] = p3;

  it->type = type;
  it->storage = storage;
  it->connectors_slots_iterator = null_ptr;
  it->finished = SC_FALSE;

  return it;
}

void sc_iterator3_free(sc_iterator3 * it)
{
  if (it == null_ptr)
    return;

  sc_mem_free(it);
}

sc_bool _sc_iterator3_f_a_a_next(sc_iterator3 * it)
{
  it->results[0] = it->params[0].addr;

  if (it->connectors_slots_iterator == null_ptr)
  {
    sc_dictionary * typed_connectors_dictionary =
        sc_dictionary_get_by_key_uint64(it->storage->output_connectors_dictionary, SC_ADDR_LOCAL_TO_INT(it->params[0].addr));
    if (typed_connectors_dictionary == null_ptr)
      goto finish;

    sc_list * element_connectors_slots = sc_dictionary_get_by_key_uint64(typed_connectors_dictionary, it->params[1].type);
    it->connectors_slots_iterator = sc_list_iterator(element_connectors_slots);
    it->connectors_channel = sc_io_new_read_channel(it->storage->output_connectors_path, null_ptr);
    sc_io_channel_set_encoding(it->connectors_channel, null_ptr, null_ptr);

    goto next_slot;
  }

current_slot:
  while (it->current_connector_position_in_slot != (sc_uint64)it->current_connectors_slot->second)
  {
    sc_uint64 connector_addr_hash;
    sc_uint64 written_bytes = 0;
    if (sc_io_channel_read_chars(
            it->connectors_channel, (sc_char *)&connector_addr_hash, sizeof(sc_addr_hash), &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_addr_hash) != written_bytes)
      goto finish;

    SC_ADDR_LOCAL_FROM_INT(connector_addr_hash, it->results[1]);
    sc_storage_get_arc_end(it->storage, it->results[1], &it->results[2]);

    ++it->current_connector_position_in_slot;

    sc_type type;
    sc_storage_get_element_type(it->storage, it->results[2], &type);
    if ((it->params[2].type & type) == it->params[2].type)
      return SC_TRUE;
  }

next_slot:
  if (sc_iterator_next(it->connectors_slots_iterator))
  {
    it->current_connector_position_in_slot = 0;
    it->current_connectors_slot = sc_iterator_get(it->connectors_slots_iterator);
    sc_io_channel_seek(it->connectors_channel, (sc_uint64)it->current_connectors_slot->first, SC_FS_IO_SEEK_SET, null_ptr);
    goto current_slot;
  }

finish:
  sc_io_channel_shutdown(it->connectors_channel, SC_TRUE, null_ptr);
  sc_iterator_destroy(it->connectors_slots_iterator);
  it->finished = SC_TRUE;
  return SC_FALSE;
}

sc_bool _sc_iterator3_f_a_f_next(sc_iterator3 * it)
{
  it->results[0] = it->params[0].addr;
  it->results[2] = it->params[2].addr;

  if (it->connectors_slots_iterator == null_ptr)
  {
    sc_dictionary * typed_connectors_dictionary =
        sc_dictionary_get_by_key_uint64(it->storage->input_connectors_dictionary, SC_ADDR_LOCAL_TO_INT(it->params[2].addr));
    if (typed_connectors_dictionary == null_ptr)
      goto finish;

    sc_list * element_connectors_slots = sc_dictionary_get_by_key_uint64(typed_connectors_dictionary, it->params[1].type);
    it->connectors_slots_iterator = sc_list_iterator(element_connectors_slots);
    it->connectors_channel = sc_io_new_read_channel(it->storage->input_connectors_path, null_ptr);
    sc_io_channel_set_encoding(it->connectors_channel, null_ptr, null_ptr);

    goto next_slot;
  }

current_slot:
  while (it->current_connector_position_in_slot != (sc_uint64)it->current_connectors_slot->second)
  {
    sc_uint64 connector_addr_hash;
    sc_uint64 written_bytes = 0;
    if (sc_io_channel_read_chars(
            it->connectors_channel, (sc_char *)&connector_addr_hash, sizeof(sc_addr_hash), &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_addr_hash) != written_bytes)
      goto finish;

    SC_ADDR_LOCAL_FROM_INT(connector_addr_hash, it->results[1]);
    sc_storage_get_arc_begin(it->storage, it->results[1], &it->results[0]);

    if (SC_ADDR_IS_EQUAL(it->params[0].addr, it->results[0]))
      return SC_TRUE;

    ++it->current_connector_position_in_slot;
  }

next_slot:
  if (sc_iterator_next(it->connectors_slots_iterator))
  {
    it->current_connector_position_in_slot = 0;
    it->current_connectors_slot = sc_iterator_get(it->connectors_slots_iterator);
    sc_io_channel_seek(it->connectors_channel, (sc_uint64)it->current_connectors_slot->first, SC_FS_IO_SEEK_SET, null_ptr);
    goto current_slot;
  }

finish:
  if (it->connectors_channel)
  {
    sc_io_channel_shutdown(it->connectors_channel, SC_TRUE, null_ptr);
  }
  sc_iterator_destroy(it->connectors_slots_iterator);
  it->finished = SC_TRUE;
  return SC_FALSE;
}

sc_bool _sc_iterator3_a_a_f_next(sc_iterator3 * it)
{
  it->results[2] = it->params[2].addr;

  if (it->connectors_slots_iterator == null_ptr)
  {
    sc_dictionary * typed_connectors_dictionary =
        sc_dictionary_get_by_key_uint64(it->storage->input_connectors_dictionary, SC_ADDR_LOCAL_TO_INT(it->params[2].addr));
    if (typed_connectors_dictionary == null_ptr)
      goto finish;

    sc_list * element_connectors_slots = sc_dictionary_get_by_key_uint64(typed_connectors_dictionary, it->params[1].type);
    it->connectors_slots_iterator = sc_list_iterator(element_connectors_slots);
    it->connectors_channel = sc_io_new_read_channel(it->storage->input_connectors_path, null_ptr);
    sc_io_channel_set_encoding(it->connectors_channel, null_ptr, null_ptr);

    goto next_slot;
  }

current_slot:
  while (it->current_connector_position_in_slot != (sc_uint64)it->current_connectors_slot->second)
  {
    sc_uint64 connector_addr_hash;
    sc_uint64 written_bytes = 0;
    if (sc_io_channel_read_chars(
            it->connectors_channel, (sc_char *)&connector_addr_hash, sizeof(sc_addr_hash), &written_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_addr_hash) != written_bytes)
      goto finish;

    SC_ADDR_LOCAL_FROM_INT(connector_addr_hash, it->results[1]);
    sc_storage_get_arc_begin(it->storage, it->results[1], &it->results[0]);

    sc_type type;
    sc_storage_get_element_type(it->storage, it->results[0], &type);
    if ((it->params[0].type & type) == it->params[0].type)
      return SC_TRUE;

    ++it->current_connector_position_in_slot;
  }

next_slot:
  if (sc_iterator_next(it->connectors_slots_iterator))
  {
    it->current_connector_position_in_slot = 0;
    it->current_connectors_slot = sc_iterator_get(it->connectors_slots_iterator);
    sc_io_channel_seek(it->connectors_channel, (sc_uint64)it->current_connectors_slot->first, SC_FS_IO_SEEK_SET, null_ptr);
    goto current_slot;
  }

finish:
  sc_io_channel_shutdown(it->connectors_channel, SC_TRUE, null_ptr);
  sc_iterator_destroy(it->connectors_slots_iterator);
  it->finished = SC_TRUE;
  return SC_FALSE;
}

sc_bool _sc_iterator3_a_f_a_next(sc_iterator3 * it)
{
  it->results[1] = it->params[1].addr;
  it->finished = SC_TRUE;
  return sc_storage_get_arc_info(it->storage, it->results[1], &it->results[0], &it->results[2]);
}

sc_bool _sc_iterator3_f_f_a_next(sc_iterator3 * it)
{
  sc_bool const result = _sc_iterator3_a_f_a_next(it);
  it->finished = SC_TRUE;
  return result && SC_ADDR_IS_EQUAL(it->results[0], it->params[0].addr);
}

sc_bool _sc_iterator3_a_f_f_next(sc_iterator3 * it)
{
  sc_bool const result = _sc_iterator3_a_f_a_next(it);
  it->finished = SC_TRUE;
  return result && SC_ADDR_IS_EQUAL(it->results[2], it->params[2].addr);
}

sc_bool _sc_iterator3_f_f_f_next(sc_iterator3 * it)
{
  return _sc_iterator3_f_f_a_next(it) && SC_ADDR_IS_EQUAL(it->results[2], it->params[2].addr);;
}

sc_bool sc_iterator3_next(sc_iterator3 * it)
{
  if ((it == null_ptr) || (it->finished == SC_TRUE))
    return SC_FALSE;

  switch (it->type)
  {
  case sc_iterator3_f_a_a:
    return _sc_iterator3_f_a_a_next(it);

  case sc_iterator3_f_a_f:
    return _sc_iterator3_f_a_f_next(it);

  case sc_iterator3_a_a_f:
    return _sc_iterator3_a_a_f_next(it);

  case sc_iterator3_a_f_a:
    return _sc_iterator3_a_f_a_next(it);

  case sc_iterator3_f_f_a:
    return _sc_iterator3_f_f_a_next(it);

  case sc_iterator3_a_f_f:
    return _sc_iterator3_a_f_f_next(it);

  case sc_iterator3_f_f_f:
    return _sc_iterator3_f_f_f_next(it);

  default:
    break;
  }

  return SC_FALSE;
}

sc_addr sc_iterator3_value(sc_iterator3 * it, sc_uint vid)
{
  if (vid < 3)
  {
    sc_addr addr = it->results[vid];
    if (SC_ADDR_IS_EMPTY(addr) && vid == 0)
      sc_storage_get_arc_begin(it->storage, it->results[1], &addr);
    else if (SC_ADDR_IS_EMPTY(addr) && vid == 2)
      sc_storage_get_arc_end(it->storage, it->results[1], &addr);

    return addr;
  }

  sc_addr addr;
  SC_ADDR_MAKE_EMPTY(addr);
  return addr;
}
