/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_fs_memory.h"

#include "sc_file_system.h"
#include "sc_fs_memory_builder.h"
#include "sc_dictionary_fs_memory_private.h"

sc_fs_memory_manager * manager;

sc_bool sc_fs_memory_initialize(const sc_char * path, sc_uint32 const max_searchable_string_size, sc_bool clear)
{
  manager = sc_fs_memory_build();

  if (manager->initialize(&manager->fs_memory, path, max_searchable_string_size) != SC_FS_MEMORY_OK)
    return SC_FALSE;

  // clear repository if it needs
  if (clear == SC_TRUE)
  {
    sc_fs_memory_info("Clear sc-fs-memory");
    if (manager->clear(manager->fs_memory) != SC_FS_MEMORY_OK)
      sc_fs_memory_info("Can't clear sc-fs-memory");
  }

  return SC_TRUE;
}

sc_bool sc_fs_memory_shutdown()
{
  if (manager->shutdown(manager->fs_memory) != SC_FS_MEMORY_OK)
    return SC_FALSE;

  sc_mem_free(manager);

  return SC_TRUE;
}

sc_bool sc_fs_memory_link_string(sc_addr_hash const link_hash, sc_char const * string, sc_uint32 const string_size)
{
  return manager->link_string(manager->fs_memory, link_hash, string, string_size) == SC_FS_MEMORY_OK;
}

sc_bool sc_fs_memory_get_string_by_link_hash(sc_addr_hash const link_hash, sc_char ** string, sc_uint32 * string_size)
{
  return manager->get_string_by_link_hash(manager->fs_memory, link_hash, string, (sc_uint64 *)string_size) ==
         SC_FS_MEMORY_OK;
}

sc_bool sc_fs_memory_get_link_hashes_by_string(sc_char const * string, sc_uint32 const string_size, sc_list ** links)
{
  return manager->get_link_hashes_by_string(manager->fs_memory, string, string_size, links) == SC_FS_MEMORY_OK;
}

sc_bool sc_fs_memory_get_link_hashes_by_substring(
    sc_char const * substring,
    sc_uint32 const substring_size,
    sc_uint32 const max_length_to_search_as_prefix,
    sc_list ** link_hashes)
{
  return manager->get_link_hashes_by_substring(
             manager->fs_memory, substring, substring_size, max_length_to_search_as_prefix, link_hashes) ==
         SC_FS_MEMORY_OK;
}

sc_bool sc_fs_memory_get_strings_by_substring(
    const sc_char * substring,
    const sc_uint32 substring_size,
    sc_uint32 const max_length_to_search_as_prefix,
    sc_list ** strings)
{
  return manager->get_strings_by_substring(
             manager->fs_memory, substring, substring_size, max_length_to_search_as_prefix, strings) == SC_FS_MEMORY_OK;
}

sc_bool sc_fs_memory_unlink_string(sc_addr_hash link_hash)
{
  return manager->unlink_string(manager->fs_memory, link_hash) == SC_FS_MEMORY_OK;
}

sc_bool sc_fs_memory_load()
{
  return manager->load(manager->fs_memory) == SC_FS_MEMORY_OK;
}

sc_bool sc_fs_memory_save()
{
  return manager->save(manager->fs_memory) == SC_FS_MEMORY_OK;
}
