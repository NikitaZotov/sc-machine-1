/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_storage.h"

#include <math.h>

#include "sc_stream_memory.h"

#include "../sc_memory_private.h"
#include "sc-fs-memory/sc_fs_memory.h"

#include "sc-fs-memory/sc_file_system.h"
#include "sc-fs-memory/sc_io.h"

#include "sc-fs-memory/sc_dictionary_fs_memory_private.h"
#include "sc-base/sc_allocator.h"
#include "sc-base/sc_message.h"
#include "sc-container/sc-string/sc_string.h"
#include "sc-container/sc-pair/sc_pair.h"

sc_result sc_storage_initialize(
    sc_storage ** storage,
    sc_char const * path,
    sc_uint64 max_searchable_string_size,
    sc_bool clear)
{
  sc_bool result = sc_fs_memory_initialize(path, max_searchable_string_size, clear);
  if (result == SC_FALSE)
    goto error;

  if (sc_fs_memory_load() == SC_FALSE)
    goto error;

  sc_memory_info("Initialize");
  if (path == null_ptr)
  {
    sc_memory_info("Path is empty");
    goto error;
  }

  if (sc_fs_is_directory(path) == SC_FALSE)
  {
    if (sc_fs_create_directory(path) == SC_FALSE)
    {
      sc_memory_error("Path `%s` is not correct", path);
      goto error;
    }
  }

  *storage = sc_mem_new(sc_storage, 1);
  {
    sc_str_cpy((*storage)->path, path, sc_str_len(path));
    (*storage)->max_segments = 512;
    (*storage)->max_slots_in_segment = 65536;
    (*storage)->max_connectors_in_slot = 256;

    {
      static sc_char const * elements_types = "elements_types" SC_FS_EXT;
      sc_fs_initialize_file_path(path, elements_types, &(*storage)->elements_types_path);
      if (sc_fs_is_file(path) == SC_FALSE)
        sc_fs_create_file((*storage)->elements_types_path);

      (*storage)->elements_types_channel = sc_io_new_append_channel((*storage)->elements_types_path, null_ptr);
      sc_io_channel_set_encoding((*storage)->elements_types_channel, null_ptr, null_ptr);
      (*storage)->last_addr_hash = 1;

      static sc_char const * connectors_elements = "connectors_elements" SC_FS_EXT;
      sc_fs_initialize_file_path(path, connectors_elements, &(*storage)->connectors_elements_path);
      if (sc_fs_is_file(path) == SC_FALSE)
        sc_fs_create_file((*storage)->connectors_elements_path);

      (*storage)->connectors_elements_channel = sc_io_new_append_channel((*storage)->connectors_elements_path, null_ptr);
      sc_io_channel_set_encoding((*storage)->connectors_elements_channel, null_ptr, null_ptr);
      (*storage)->last_connector_elements_offset = 1;
    }

    (*storage)->input_connectors_segments = sc_mem_new(sc_list ***, (*storage)->max_segments);
    static sc_char const * input_connectors = "input_connectors" SC_FS_EXT;
    sc_fs_initialize_file_path(path, input_connectors, &(*storage)->input_connectors_path);
    if (sc_fs_is_file(path) == SC_FALSE)
      sc_fs_create_file((*storage)->input_connectors_path);

    (*storage)->input_connectors_channel = sc_io_new_append_channel((*storage)->input_connectors_path, null_ptr);
    sc_io_channel_set_encoding((*storage)->input_connectors_channel, null_ptr, null_ptr);
    (*storage)->last_input_connectors_offset = 1;

    (*storage)->output_connectors_segments = sc_mem_new(sc_list ***, (*storage)->max_segments);
    static sc_char const * output_connectors = "output_connectors" SC_FS_EXT;
    sc_fs_initialize_file_path(path, output_connectors, &(*storage)->output_connectors_path);
    if (sc_fs_is_file(path) == SC_FALSE)
      sc_fs_create_file((*storage)->output_connectors_path);

    (*storage)->output_connectors_channel = sc_io_new_append_channel((*storage)->output_connectors_path, null_ptr);
    sc_io_channel_set_encoding((*storage)->output_connectors_channel, null_ptr, null_ptr);
    (*storage)->last_output_connectors_offset = 1;
  }
  sc_memory_info("Configuration:");
  sc_message("\tRepo path: %s", path);

  sc_memory_info("Successfully initialized");

  return SC_RESULT_OK;

error:
{
  if (storage != null_ptr)
    *storage = null_ptr;
  sc_memory_info("Initialized with errors");
  return SC_RESULT_ERROR_IO;
}
}

sc_result sc_storage_shutdown(sc_storage * storage, sc_bool save_state)
{
  if (storage == null_ptr)
  {
    sc_memory_info("Storage is empty to shutdown");
    return SC_RESULT_NO;
  }

  sc_memory_info("Shutdown");
  {
    sc_mem_free(storage->path);

    {
      sc_mem_free(storage->elements_types_path);
      sc_io_channel_shutdown(storage->elements_types_channel, SC_TRUE, null_ptr);

      sc_mem_free(storage->connectors_elements_path);
      sc_io_channel_shutdown(storage->connectors_elements_channel, SC_TRUE, null_ptr);
    }
    sc_mem_free(storage->input_connectors_path);
    sc_io_channel_shutdown(storage->input_connectors_channel, SC_TRUE, null_ptr);

    sc_mem_free(storage->output_connectors_path);
    sc_io_channel_shutdown(storage->output_connectors_channel, SC_TRUE, null_ptr);
  }
  sc_mem_free(storage);
  sc_memory_info("Successfully shutdown");

  if (save_state == SC_TRUE)
  {
    if (sc_fs_memory_save() == SC_FALSE)
      return SC_RESULT_READ_ERROR;
  }

  if (sc_fs_memory_shutdown() == SC_FALSE)
    return SC_RESULT_NO;

  return SC_RESULT_OK;
}

sc_bool _sc_storage_write_element_new(sc_storage * storage, sc_type type, sc_uint64 connector_elements_offset)
{
  sc_io_channel_seek(storage->elements_types_channel, storage->last_addr_hash, SC_FS_IO_SEEK_SET, null_ptr);

  sc_uint64 written_bytes = 0;
  if (sc_io_channel_write_chars(
          storage->elements_types_channel, (sc_char *)&storage->last_addr_hash, sizeof(sc_addr_hash), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_addr_hash) != written_bytes)
  {
    sc_memory_error("Error while attribute `last_addr_hash` writing");
    return SC_FALSE;
  }
  storage->last_addr_hash += written_bytes;

  if (sc_io_channel_write_chars(storage->elements_types_channel, (sc_char *)&type, sizeof(sc_type), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_type) != written_bytes)
  {
    sc_memory_error("Error while attribute `type` writing");
    return SC_FALSE;
  }
  storage->last_addr_hash += written_bytes;

  if (sc_io_channel_write_chars(
          storage->elements_types_channel, (sc_char *)&connector_elements_offset, sizeof(sc_uint64), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_uint64) != written_bytes)
  {
    sc_memory_error("Error while attribute `connector_elements_offset` writing");
    return SC_FALSE;
  }
  storage->last_addr_hash += written_bytes;

  if (sc_io_channel_write_chars(
          storage->elements_types_channel, (sc_char *)&storage->last_input_connectors_offset, sizeof(sc_uint64), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_uint64) != written_bytes)
  {
    sc_memory_error("Error while attribute `last_input_connectors_offset` writing");
    return SC_FALSE;
  }
  storage->last_addr_hash += written_bytes;

  if (sc_io_channel_write_chars(
          storage->elements_types_channel, (sc_char *)&storage->last_output_connectors_offset, sizeof(sc_uint64), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_uint64) != written_bytes)
  {
    sc_memory_error("Error while attribute `last_output_connectors_offset` writing");
    return SC_FALSE;
  }
  storage->last_addr_hash += written_bytes;

  return SC_TRUE;
}

sc_addr sc_storage_node_new_ext(sc_storage * storage, sc_type type)
{
  sc_addr_hash const addr_hash = storage->last_addr_hash;
  sc_addr new_addr;
  SC_ADDR_LOCAL_FROM_INT(addr_hash, new_addr);

  if (_sc_storage_write_element_new(storage, type, 0) == SC_FALSE)
    return SC_ADDR_EMPTY;

  return new_addr;
}

sc_addr sc_storage_node_new(sc_storage * storage, sc_type type)
{
  return sc_storage_node_new_ext(storage, sc_type_node | type);
}

sc_addr sc_storage_link_new(sc_storage * storage, sc_type type)
{
  return sc_storage_node_new_ext(storage, sc_type_link | type);
}

sc_bool sc_storage_is_element(sc_storage * storage, sc_addr addr)
{
  if (storage->elements_types_channel == null_ptr)
  {
    sc_memory_error("Path `%s` doesn't exist", storage->elements_types_path);
    return SC_FALSE;
  }

  sc_addr_hash addr_offset;
  sc_io_channel_seek(storage->elements_types_channel, SC_ADDR_LOCAL_TO_INT(addr), SC_FS_IO_SEEK_SET, null_ptr);
  {
    sc_uint64 read_bytes;
    if (sc_io_channel_read_chars(storage->elements_types_channel, (sc_char *)&addr_offset, sizeof(sc_addr_hash), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_addr_hash) != read_bytes)
    {
      return SC_FALSE;
    }
  }

  return addr_offset == SC_ADDR_LOCAL_TO_INT(addr);
}

sc_bool _sc_storage_write_connector_elements(sc_storage * storage, sc_addr beg, sc_addr end)
{
  sc_io_channel_seek(storage->connectors_elements_channel, storage->last_connector_elements_offset, SC_FS_IO_SEEK_SET, null_ptr);

  sc_uint64 written_bytes = 0;
  sc_addr_hash const begin_addr_hash = SC_ADDR_LOCAL_TO_INT(beg);
  if (sc_io_channel_write_chars(storage->connectors_elements_channel, (sc_char *)&begin_addr_hash, sizeof(sc_addr_hash), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_addr_hash) != written_bytes)
  {
    sc_memory_error("Error while attribute `beg.offset` writing");
    return SC_FALSE;
  }
  storage->last_connector_elements_offset += written_bytes;

  sc_addr_hash const end_addr_hash = SC_ADDR_LOCAL_TO_INT(end);
  if (sc_io_channel_write_chars(storage->connectors_elements_channel, (sc_char *)&end_addr_hash, sizeof(sc_addr_hash), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_addr_hash) != written_bytes)
  {
    sc_memory_error("Error while attribute `end.offset` writing");
    return SC_FALSE;
  }
  storage->last_connector_elements_offset += written_bytes;

  return SC_TRUE;
}

sc_uint16 _sc_storage_define_connector_syntactic_type(sc_type connector_type)
{
  sc_uint16 connector_syntactic_type;
  if ((connector_type & sc_type_arc_access) == sc_type_arc_access)
    connector_syntactic_type = 1;
  else if ((connector_type & sc_type_arc_common) == sc_type_arc_common)
    connector_syntactic_type = 2;
  else if ((connector_type & sc_type_edge_common) == sc_type_edge_common)
    connector_syntactic_type = 3;
  else
    connector_syntactic_type = 0;

  return connector_syntactic_type;
}

sc_uint16 _sc_storage_define_access_connector_type(sc_type connector_type)
{
  sc_uint16 access_connector_type;
  if ((connector_type & sc_type_arc_pos) == sc_type_arc_pos)
    access_connector_type = 4;
  else if ((connector_type & sc_type_arc_fuz) == sc_type_arc_fuz)
    access_connector_type = 6;
  else if ((connector_type & sc_type_arc_neg) == sc_type_arc_neg)
    access_connector_type = 8;
  else
    access_connector_type = 0;

  return access_connector_type;
}

sc_uint16 _sc_storage_define_const_element_type(sc_type element_type)
{
  sc_uint16 const_element_type;
  if ((element_type & sc_type_const) == sc_type_const)
    const_element_type = 10;
  else if ((element_type & sc_type_var) == sc_type_var)
    const_element_type = 14;
  else
    const_element_type = 0;

  return const_element_type;
}

sc_uint16 _sc_storage_define_perm_element_type(sc_type element_type)
{
  sc_uint16 perm_element_type;
  if ((element_type & sc_type_arc_perm) == sc_type_arc_perm)
    perm_element_type = 18;
  else if ((element_type & sc_type_arc_temp) == sc_type_arc_temp)
    perm_element_type = 22;
  else
    perm_element_type = 0;

  return perm_element_type;
}

sc_uint16 sc_storage_define_connector_type_code(sc_type connector_type)
{
  return _sc_storage_define_connector_syntactic_type(connector_type) +
         _sc_storage_define_access_connector_type(connector_type) +
         _sc_storage_define_const_element_type(connector_type) +
         _sc_storage_define_perm_element_type(connector_type);
}

sc_list * _sc_storage_get_all_connector_subtypes(sc_type connector_type)
{
  sc_list * types;
  sc_list_init(&types);
  sc_uint16 const syntactic_connector_type = _sc_storage_define_connector_syntactic_type(connector_type);
  if (syntactic_connector_type != 0)
    sc_list_push_back(types, (void *)(sc_uint64)syntactic_connector_type);

  sc_uint16 const access_connector_type = _sc_storage_define_access_connector_type(connector_type);
  if (access_connector_type != 0)
    sc_list_push_back(types, (void *)(sc_uint64)access_connector_type);

  sc_uint16 const const_element_type = _sc_storage_define_const_element_type(connector_type);
  if (const_element_type != 0)
    sc_list_push_back(types, (void *)(sc_uint64)const_element_type);

  sc_uint16 const perm_element_type = _sc_storage_define_perm_element_type(connector_type);
  if (perm_element_type != 0)
    sc_list_push_back(types, (void *)(sc_uint64)perm_element_type);

  return types;
}

void _sc_storage_update_all_typed_connectors(
    sc_list ** typed_connectors, sc_list * connector_subtypes, sc_pair * element_connectors_slot_info)
{
  sc_uint8 card_size = (sc_uint8)pow(2, connector_subtypes->size);
  for (sc_uint8 i = 0; i < card_size; ++i)
  {
    sc_uint8 num = i;
    sc_uint16 subtype = 0;

    sc_iterator * it = sc_list_iterator(connector_subtypes);
    for (sc_uint8 j = connector_subtypes->size - 1; j >= 0 && sc_iterator_next(it); --j) {
      if (num % 2)
        subtype += (sc_uint16)(sc_uint64)sc_iterator_get(it);

      num /= 2;
    }
    sc_iterator_destroy(it);

    sc_list * element_connectors_slots = typed_connectors[subtype];
    if (element_connectors_slots == null_ptr)
    {
      sc_list_init(&element_connectors_slots);
      typed_connectors[subtype] = element_connectors_slots;
    }

    sc_list_push_back(element_connectors_slots, element_connectors_slot_info);
  }
}

sc_list ** sc_storage_resolve_element_typed_connectors(
    sc_storage * storage,
    sc_list **** connectors_segments,
    sc_addr connector_element_addr)
{
  sc_addr_hash const connector_element_addr_hash = SC_ADDR_LOCAL_TO_INT(connector_element_addr);
  sc_list *** segment = connectors_segments[connector_element_addr_hash / storage->max_slots_in_segment];
  if (segment == null_ptr)
  {
    printf("max_slots_in_segment %llu", storage->last_input_connectors_offset);
    segment = sc_mem_new(sc_list **, storage->max_slots_in_segment);
    connectors_segments[connector_element_addr_hash / storage->max_slots_in_segment] = segment;
  }

  sc_list ** typed_connectors = segment[connector_element_addr_hash / SC_ELEMENT_SIZE];
  if (typed_connectors == null_ptr)
  {
    typed_connectors = sc_mem_new(sc_list *, MAX_SC_CONNECTOR_TYPE_CODE);
    segment[connector_element_addr_hash / SC_ELEMENT_SIZE] = typed_connectors;
  }

  return typed_connectors;
}

sc_pair * _sc_storage_resolve_element_connectors_slot_info(
    sc_storage * storage,
    sc_list **** element_connectors_segments,
    sc_uint64 * last_element_connectors_offset,
    sc_type connector_type,
    sc_list * connector_subtypes,
    sc_addr element_addr)
{
  sc_list ** typed_connectors =
      sc_storage_resolve_element_typed_connectors(storage, element_connectors_segments, element_addr);

  sc_pair * element_connectors_slot_info;
  sc_list * element_connectors_slots = typed_connectors[sc_storage_define_connector_type_code(connector_type)];
  if (element_connectors_slots == null_ptr)
  {
    element_connectors_slot_info = sc_make_pair((void *)*last_element_connectors_offset, 0);
    *last_element_connectors_offset += storage->max_connectors_in_slot * sizeof(sc_uint64);

    _sc_storage_update_all_typed_connectors(typed_connectors, connector_subtypes, element_connectors_slot_info);
  }
  else
  {
    element_connectors_slot_info = sc_list_back(element_connectors_slots)->data;
    if ((sc_uint64)element_connectors_slot_info->second == storage->max_connectors_in_slot)
    {
      element_connectors_slot_info = sc_make_pair((void *)*last_element_connectors_offset, 0);
      *last_element_connectors_offset += storage->max_connectors_in_slot * sizeof(sc_uint64);

      _sc_storage_update_all_typed_connectors(typed_connectors, connector_subtypes, element_connectors_slot_info);
    }
  }

  return element_connectors_slot_info;
}

sc_bool _sc_storage_write_element_connector_in_slot(
    sc_io_channel * channel,
    sc_uint64 element_connectors_offset,
    sc_addr connector_addr)
{
  sc_io_channel_seek(channel, element_connectors_offset, SC_FS_IO_SEEK_SET, null_ptr);

  sc_uint64 written_bytes = 0;
  sc_addr_hash const addr_hash = SC_ADDR_LOCAL_TO_INT(connector_addr);
  if (sc_io_channel_write_chars(
          channel, (sc_char *)&addr_hash, sizeof(sc_addr_hash), &written_bytes, null_ptr) !=
          SC_FS_IO_STATUS_NORMAL ||
      sizeof(sc_addr_hash) != written_bytes)
  {
    sc_memory_error("Error while attribute `beg.offset` writing");
    return SC_FALSE;
  }

  return SC_TRUE;
}

sc_bool _sc_storage_push_element_connector_in_slot(
    sc_storage * storage,
    sc_io_channel * channel,
    sc_list **** connectors_segments,
    sc_uint64 * last_element_connectors_offset,
    sc_addr connector_addr,
    sc_type connector_type,
    sc_list * connector_subtypes,
    sc_addr element_addr)
{
  sc_pair * element_connectors_slot_info = _sc_storage_resolve_element_connectors_slot_info(
      storage, connectors_segments, last_element_connectors_offset, connector_type, connector_subtypes, element_addr);

  sc_uint64 const element_connectors_offset = (sc_uint64)element_connectors_slot_info->first +
                                              (sc_uint64)element_connectors_slot_info->second * sizeof(sc_uint64);
  element_connectors_slot_info->second = (void *)((sc_uint64)element_connectors_slot_info->second + 1);
  return _sc_storage_write_element_connector_in_slot(
      channel, element_connectors_offset, connector_addr);
}

sc_addr sc_storage_connector_new(sc_storage * storage, sc_type type, sc_addr beg, sc_addr end)
{
  //printf("last_addr_hash %llu", storage->last_addr_hash);
  //printf("last_input_connectors_offset %llu", storage->last_input_connectors_offset);

  if (sc_storage_is_element(storage, beg) == SC_FALSE)
  {
    sc_memory_error("Begin sc-address `{0, %llu}` is not valid", beg.offset);
    return SC_ADDR_EMPTY;
  }

  if (sc_storage_is_element(storage, end) == SC_FALSE)
  {
    sc_memory_error("End sc-address `{0, %llu}` is not valid", end.offset);
    return SC_ADDR_EMPTY;
  }

  sc_uint64 const addr_hash = storage->last_addr_hash;
  sc_addr new_addr;
  SC_ADDR_LOCAL_FROM_INT(addr_hash, new_addr);

  if (_sc_storage_write_element_new(storage, type, storage->last_connector_elements_offset) == SC_FALSE)
    return SC_ADDR_EMPTY;

  if (_sc_storage_write_connector_elements(storage, beg, end) == SC_FALSE)
    return SC_ADDR_EMPTY;

  sc_list * subtypes = _sc_storage_get_all_connector_subtypes(type);
  if (_sc_storage_push_element_connector_in_slot(
          storage,
          storage->input_connectors_channel,
          storage->input_connectors_segments,
          &storage->last_input_connectors_offset,
          new_addr,
          type,
          subtypes,
          end) == SC_FALSE)
    return SC_ADDR_EMPTY;

  if (_sc_storage_push_element_connector_in_slot(
          storage,
          storage->output_connectors_channel,
          storage->output_connectors_segments,
          &storage->last_output_connectors_offset,
          new_addr,
          type,
          subtypes,
          beg) == SC_FALSE)
    return SC_ADDR_EMPTY;

  sc_list_destroy(subtypes);

  return new_addr;
}

sc_result sc_storage_element_free(sc_storage * storage, sc_addr addr)
{
  if (sc_storage_is_element(storage, addr) == SC_FALSE)
  {
    sc_memory_error("Sc-address `{0, %llu}` is not valid to remove its sc-element", SC_ADDR_LOCAL_TO_INT(addr));
    return SC_RESULT_ERROR_IS_NOT_ELEMENT;
  }

  if (storage->elements_types_channel == null_ptr)
  {
    sc_memory_error("Path `%s` doesn't exist", storage->elements_types_path);
    return SC_RESULT_ERROR_IO;
  }

  sc_io_channel_seek(storage->elements_types_channel, SC_ADDR_LOCAL_TO_INT(addr), SC_FS_IO_SEEK_SET, null_ptr);
  {
    sc_uint64 read_bytes;
    sc_addr_hash hash = 0;
    if (sc_io_channel_write_chars(storage->elements_types_channel, (sc_char *)&hash, sizeof(sc_addr_hash), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_addr_hash) != read_bytes)
    {
      return SC_RESULT_READ_ERROR;
    }
  }

  return SC_RESULT_OK;
}

sc_result sc_storage_get_element_type(sc_storage * storage, sc_addr addr, sc_type * result)
{
  if (sc_storage_is_element(storage, addr) == SC_FALSE)
  {
    sc_memory_error("Sc-address `{0, %llu}` is not valid to get its sc-element type", SC_ADDR_LOCAL_TO_INT(addr));
    return SC_RESULT_ERROR_IS_NOT_ELEMENT;
  }

  if (storage->elements_types_channel == null_ptr)
  {
    sc_memory_error("Path `%s` doesn't exist", storage->elements_types_path);
    return SC_RESULT_ERROR_IO;
  }

  sc_io_channel_seek(storage->elements_types_channel, SC_ADDR_LOCAL_TO_INT(addr) + sizeof(sc_uint64), SC_FS_IO_SEEK_SET, null_ptr);
  {
    sc_uint64 read_bytes;
    if (sc_io_channel_read_chars(storage->elements_types_channel, (sc_char *)result, sizeof(sc_type), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_type) != read_bytes)
    {
      *result = 0;
      return SC_RESULT_READ_ERROR;
    }
  }

  return SC_RESULT_OK;
}

sc_result sc_storage_change_element_subtype(sc_storage * storage, sc_addr addr, sc_type type)
{
  sc_type element_type;
  sc_result const status = sc_storage_get_element_type(storage, addr, &element_type);
  if (status != SC_RESULT_OK)
    return status;

  element_type |= type;

  if (storage->elements_types_channel == null_ptr)
  {
    sc_memory_error("Path `%s` doesn't exist", storage->elements_types_path);
    return SC_RESULT_ERROR_IO;
  }

  sc_io_channel_seek(storage->elements_types_channel, SC_ADDR_LOCAL_TO_INT(addr) + sizeof(sc_uint64), SC_FS_IO_SEEK_SET, null_ptr);
  {
    sc_uint64 read_bytes;
    if (sc_io_channel_write_chars(storage->elements_types_channel, (sc_char *)&element_type, sizeof(sc_type), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_type) != read_bytes)
    {
      return SC_RESULT_READ_ERROR;
    }
  }

  return SC_RESULT_OK;
}

sc_uint64 sc_storage_get_connector_elements_offset(sc_storage * storage, sc_addr addr)
{
  if (storage->elements_types_channel == null_ptr)
  {
    sc_memory_error("Path `%s` doesn't exist", storage->elements_types_path);
    return INVALID_OFFSET;
  }

  sc_uint64 connector_elements_offset;
  sc_io_channel_seek(storage->elements_types_channel, SC_ADDR_LOCAL_TO_INT(addr) + sizeof(sc_uint64) + sizeof(sc_type), SC_FS_IO_SEEK_SET, null_ptr);
  {
    sc_uint64 read_bytes;
    if (sc_io_channel_read_chars(
            storage->elements_types_channel, (sc_char *)&connector_elements_offset, sizeof(sc_uint64), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_uint64) != read_bytes)
    {
      return INVALID_OFFSET;
    }
  }

  return connector_elements_offset;
}

sc_result sc_storage_get_arc_info(
    sc_storage * storage,
    sc_addr addr,
    sc_addr * result_begin_addr,
    sc_addr * result_end_addr)
{
  *result_begin_addr = SC_ADDR_EMPTY;
  *result_end_addr = SC_ADDR_EMPTY;

  if (sc_storage_is_element(storage, addr) == SC_FALSE)
  {
    sc_memory_error("Sc-address `{0, %llu}` is not valid to get its sc-connector begin sc-element", SC_ADDR_LOCAL_TO_INT(addr));
    return SC_RESULT_ERROR_IS_NOT_ELEMENT;
  }

  sc_uint64 const connector_elements_offset = sc_storage_get_connector_elements_offset(storage, addr);
  if (connector_elements_offset == 0)
    return SC_RESULT_READ_ERROR;

  if (storage->connectors_elements_channel == null_ptr)
  {
    sc_memory_error("Path `%s` doesn't exist", storage->connectors_elements_path);
    return SC_RESULT_ERROR_IO;
  }

  sc_addr_hash beg_addr_hash;
  sc_addr_hash end_addr_hash;
  sc_io_channel_seek(storage->connectors_elements_channel, connector_elements_offset, SC_FS_IO_SEEK_SET, null_ptr);
  {
    sc_uint64 read_bytes;
    if (sc_io_channel_read_chars(storage->connectors_elements_channel, (sc_char *)&beg_addr_hash, sizeof(sc_addr_hash), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_addr_hash) != read_bytes)
    {
      return SC_RESULT_READ_ERROR;
    }

    if (sc_io_channel_read_chars(storage->connectors_elements_channel, (sc_char *)&end_addr_hash, sizeof(sc_addr_hash), &read_bytes, null_ptr) !=
            SC_FS_IO_STATUS_NORMAL ||
        sizeof(sc_addr_hash) != read_bytes)
    {
      return SC_RESULT_READ_ERROR;
    }
  }

  SC_ADDR_LOCAL_FROM_INT(beg_addr_hash, (*result_begin_addr));
  SC_ADDR_LOCAL_FROM_INT(end_addr_hash, (*result_end_addr));

  return SC_RESULT_OK;
}

sc_result sc_storage_get_arc_begin(sc_storage * storage, sc_addr addr, sc_addr * result)
{
  sc_addr end_addr;
  return sc_storage_get_arc_info(storage, addr, result, &end_addr);
}

sc_result sc_storage_get_arc_end(sc_storage * storage, sc_addr addr, sc_addr * result)
{
  sc_addr beg_addr;
  return sc_storage_get_arc_info(storage, addr, &beg_addr, result);
}

sc_result sc_storage_set_link_content(sc_storage * storage, sc_addr addr, const sc_stream * stream)
{
  sc_char * string = null_ptr;
  sc_uint32 string_size = 0;
  if (sc_stream_get_data(stream, &string, &string_size) == SC_FALSE)
    goto error;
  else
  {
    if (string == null_ptr)
      sc_string_empty(string);

    sc_fs_memory_link_string(SC_ADDR_LOCAL_TO_INT(addr), string, string_size);
  }
  sc_mem_free(string);
  return SC_RESULT_OK;

error:
  return SC_RESULT_READ_ERROR;
}

sc_result sc_storage_get_link_content(sc_storage * storage, sc_addr addr, sc_stream ** stream)
{
  sc_uint32 sc_string_size = 0;
  sc_char * sc_string = null_ptr;
  sc_fs_memory_get_string_by_link_hash(SC_ADDR_LOCAL_TO_INT(addr), &sc_string, &sc_string_size);

  if (sc_string == null_ptr)
  {
    sc_string_empty(sc_string);
    *stream = sc_stream_memory_new(sc_string, sc_string_size, SC_STREAM_FLAG_READ, SC_TRUE);
    return SC_RESULT_ERROR_NOT_FOUND;
  }

  *stream = sc_stream_memory_new(sc_string, sc_string_size, SC_STREAM_FLAG_READ, SC_TRUE);
  return SC_RESULT_OK;
}

sc_result sc_storage_find_links_with_content_string(
    sc_storage * storage,
    const sc_stream * stream,
    sc_list ** result_hashes)
{
  *result_hashes = null_ptr;

  sc_char * string = null_ptr;
  sc_uint32 string_size = 0;
  if (sc_stream_get_data(stream, &string, &string_size) == SC_FALSE)
    return SC_RESULT_READ_ERROR;

  if (string == null_ptr)
    sc_string_empty(string);

  sc_bool const result = sc_fs_memory_get_link_hashes_by_string(string, string_size, result_hashes);
  sc_mem_free(string);

  if (result == SC_FALSE || result_hashes == null_ptr || *result_hashes == 0)
    return SC_RESULT_READ_ERROR;

  return SC_RESULT_OK;
}

sc_result sc_storage_find_links_by_content_substring(
    sc_storage * storage,
    const sc_stream * stream,
    sc_list ** result_hashes,
    sc_uint32 max_length_to_search_as_prefix)
{
  *result_hashes = null_ptr;

  sc_char * string = null_ptr;
  sc_uint32 string_size = 0;
  if (sc_stream_get_data(stream, &string, &string_size) == SC_FALSE)
    return SC_RESULT_READ_ERROR;

  if (string == null_ptr)
    sc_string_empty(string);

  sc_bool const result =
      sc_fs_memory_get_link_hashes_by_substring(string, string_size, max_length_to_search_as_prefix, result_hashes);
  sc_mem_free(string);

  if (result == SC_FALSE)
    return SC_RESULT_READ_ERROR;

  return SC_RESULT_OK;
}

sc_result sc_storage_find_links_contents_by_content_substring(
    sc_storage * storage,
    sc_stream const * stream,
    sc_list ** result_strings,
    sc_uint32 max_length_to_search_as_prefix)
{
  *result_strings = null_ptr;

  sc_char * string = null_ptr;
  sc_uint32 string_size = 0;
  if (sc_stream_get_data(stream, &string, &string_size) == SC_FALSE)
    return SC_RESULT_READ_ERROR;

  if (string == null_ptr)
    sc_string_empty(string);

  sc_bool const result =
      sc_fs_memory_get_strings_by_substring(string, string_size, max_length_to_search_as_prefix, result_strings);
  sc_mem_free(string);
  if (result == SC_FALSE)
    return SC_RESULT_READ_ERROR;

  return result;
}
