/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef _sc_fs_memory_private_h_
#define _sc_fs_memory_private_h_

#include "sc_fs_memory.h"

struct _sc_fs_memory_manager
{
  sc_fs_memory * fs_memory;  // file system memory instance
  sc_char const * path;      // repo path
  sc_char * segments_path;   // file path to sc-memory segments

  sc_version version;
  sc_fs_memory_header header;

  sc_fs_memory_status (*initialize)(sc_fs_memory ** memory, sc_memory_params const * params);
  sc_fs_memory_status (*shutdown)(sc_fs_memory * memory);
  sc_fs_memory_status (*load)(sc_fs_memory * memory);
  sc_fs_memory_status (*save)(sc_fs_memory const * memory);
  sc_fs_memory_status (*link_string)(
      sc_fs_memory * memory,
      sc_addr_hash const link_hash,
      sc_char const * string,
      sc_uint64 const string_size,
      sc_bool is_searchable_string);
  sc_fs_memory_status (*get_string_by_link_hash)(
      sc_fs_memory * memory,
      sc_addr_hash const link_hash,
      sc_char ** string,
      sc_uint64 * string_size);
  sc_fs_memory_status (*get_link_hashes_by_string)(
      sc_fs_memory * memory,
      sc_char const * string,
      sc_uint64 const string_size,
      void * data,
      void (*callback)(void * data, sc_addr const link_addr));
  sc_fs_memory_status (*get_link_hashes_by_substring)(
      sc_fs_memory * memory,
      sc_char const * substring,
      sc_uint64 const substring_size,
      sc_uint32 const max_length_to_search_as_prefix,
      void * data,
      void (*callback)(void * data, sc_addr const link_addr));
  sc_fs_memory_status (*get_strings_by_substring)(
      sc_fs_memory * memory,
      sc_char const * substring,
      sc_uint64 const substring_size,
      sc_uint32 const max_length_to_search_as_prefix,
      void * data,
      void (*callback)(void * data, sc_addr const link_addr, sc_char const * link_content));
  sc_fs_memory_status (*unlink_string)(sc_fs_memory * memory, sc_addr_hash const link_hash);
};

#endif
