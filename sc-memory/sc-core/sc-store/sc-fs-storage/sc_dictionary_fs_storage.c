/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifdef SC_DICTIONARY_FS_STORAGE

#  include <memory.h>
#  include <glib.h>
#  include <glib/gstdio.h>

#  include "sc_dictionary_fs_storage.h"
#  include "sc_file_system.h"

sc_char * file_path = 0;
sc_char strings_path[MAX_PATH_LENGTH];
sc_char links_path[MAX_PATH_LENGTH];

sc_dictionary * links_dictionary;
sc_dictionary * strings_dictionary;

inline sc_uint32 sc_addr_to_hash(sc_addr addr)
{
  return SC_ADDR_LOCAL_TO_INT(addr);
}

sc_char * itora(sc_uint32 num)
{
  sc_char * result = g_new0(char, 10);
  sc_char * index = result;

  while (num > 0)
  {
    *index++ = (sc_char)((num % 10) | '0');
    num /= 10;
  }

  return result;
}

inline sc_char * sc_addr_to_str(sc_addr addr)
{
  sc_uint32 hash = sc_addr_to_hash(addr);

  // !reverse translation for more effectively work
  return itora(hash);
}

// ----------------------------------------------

sc_bool sc_dictionary_fs_storage_initialize(const sc_char * path)
{
  file_path = strdup(path);

  g_message("Initialize sc-dictionary from path: %s", path);
  sc_bool result = sc_dictionary_initialize(&links_dictionary);
  if (result == SC_FALSE)
    return SC_FALSE;

  result = sc_dictionary_initialize(&strings_dictionary);
  if (result == SC_FALSE)
    return SC_FALSE;

  g_message("Initialize sc-storage from path: %s", path);
  g_snprintf(strings_path, MAX_PATH_LENGTH, "%s/strings.scdb", path);
  g_snprintf(links_path, MAX_PATH_LENGTH, "%s/links.scdb", path);

  return SC_TRUE;
}

sc_bool sc_dictionary_fs_storage_shutdown()
{
  g_message("Shutdown sc-dictionary");
  if (sc_dictionary_destroy(links_dictionary) == SC_FALSE)
    g_critical("Can't shutdown sc-dictionary");
  links_dictionary = null_ptr;

  if (sc_dictionary_destroy(strings_dictionary) == SC_FALSE)
    g_critical("Can't shutdown sc-dictionary");
  strings_dictionary = null_ptr;

  return SC_FALSE;
}

sc_bool sc_hashes_compare(void * hash, void * other)
{
  return *(sc_addr_hash *)hash == *(sc_addr_hash *)other;
}

void _sc_dictionary_fs_storage_append_sc_string_sc_link_reference(
    sc_addr addr,
    sc_dictionary_node * node,
    sc_char * sc_string,
    sc_uint32 size)
{
  sc_char * hash_str = sc_addr_to_str(addr);
  sc_addr_hash hash_str_len = strlen(hash_str);

  sc_dictionary_node * link_hash_node = sc_dictionary_append_to_node(strings_dictionary->root, hash_str, hash_str_len);

  if (link_hash_node->data_list == null_ptr)
    sc_list_init(&link_hash_node->data_list);

  if (link_hash_node->data_list->size != 0)
    g_free(sc_list_pop_back(link_hash_node->data_list));

  sc_link_content * content = g_new0(sc_link_content, 1);
  content->sc_string = g_new0(sc_char, size + 1);
  content->string_size = size;
  memcpy(content->sc_string, sc_string, size);
  content->node = &(*node);

  sc_list_push_back(link_hash_node->data_list, content);
}

sc_dictionary_node * _sc_dictionary_fs_storage_append_sc_link_unique(sc_char * sc_string, sc_uint32 size, sc_addr addr)
{
  sc_addr_hash other = sc_addr_to_hash(addr);

  sc_link_content * old_content = sc_dictionary_fs_storage_get_link_content(addr);
  if (old_content != null_ptr && old_content->sc_string != null_ptr && old_content->node != null_ptr)
  {
    if (strcmp(old_content->sc_string, sc_string) == 0)
      return old_content->node;

    sc_list * data_list = old_content->node->data_list;
    sc_list_remove_if(data_list, &other, sc_hashes_compare);
  }

  sc_addr_hash * hash = g_new0(sc_addr_hash, 1);
  *hash = other;
  return sc_dictionary_append(links_dictionary, sc_string, size, hash);
}

sc_dictionary_node * sc_dictionary_fs_storage_append_sc_link(sc_addr addr, sc_char * sc_string, sc_uint32 size)
{
  sc_dictionary_node * node = _sc_dictionary_fs_storage_append_sc_link_unique(sc_string, size, addr);
  _sc_dictionary_fs_storage_append_sc_string_sc_link_reference(addr, node, sc_string, size);

  return node;
}

sc_addr * sc_list_to_addr_array(sc_list * list)
{
  sc_addr * addrs = g_new0(sc_addr, list->size);
  sc_addr ** temp = g_new0(sc_addr *, 1);
  *temp = addrs;

  sc_iterator * it = sc_list_iterator(list);
  while (sc_iterator_next(it))
  {
    sc_addr_hash hash = *(sc_addr_hash *)sc_iterator_get(it);
    sc_addr addr;
    addr.offset = SC_ADDR_LOCAL_OFFSET_FROM_INT(hash);
    addr.seg = SC_ADDR_LOCAL_SEG_FROM_INT(hash);

    **temp = addr;
    ++*temp;
  }
  g_free(temp);

  return addrs;
}

sc_addr_hash * sc_list_to_hashes_array(sc_list * list)
{
  sc_addr_hash * hashes = g_new0(sc_addr_hash, list->size);
  sc_addr_hash ** temp = g_new0(sc_addr_hash *, 1);
  *temp = hashes;

  sc_iterator * it = sc_list_iterator(list);
  while (sc_iterator_next(it))
  {
    **temp = *(sc_addr_hash *)sc_iterator_get(it);
    ++*temp;
  }

  return hashes;
}

sc_addr sc_dictionary_fs_storage_get_sc_link(const sc_char * sc_string)
{
  sc_addr empty;
  SC_ADDR_MAKE_EMPTY(empty)

  sc_addr_hash * addr_hash = sc_dictionary_get_data_from_node(links_dictionary->root, sc_string);
  if (addr_hash != null_ptr)
  {
    sc_addr addr;
    addr.seg = SC_ADDR_LOCAL_SEG_FROM_INT(*addr_hash);
    addr.offset = SC_ADDR_LOCAL_OFFSET_FROM_INT(*addr_hash);
    return addr;
  }

  return empty;
}

sc_bool sc_dictionary_fs_storage_get_sc_links(const sc_char * sc_string, sc_addr ** links, sc_uint32 * size)
{
  sc_list * list = sc_dictionary_get_datas_from_node(links_dictionary->root, sc_string);

  if (list == null_ptr)
  {
    *links = null_ptr;
    *size = 0;
    return SC_FALSE;
  }

  *links = sc_list_to_addr_array(list);
  *size = list->size;
  *links = memcpy(*links, *links, *size);

  return SC_TRUE;
}

sc_char * sc_dictionary_fs_storage_get_sc_string(sc_addr addr)
{
  if (SC_ADDR_IS_EMPTY(addr))
    return null_ptr;

  sc_link_content * content = sc_dictionary_get_data_from_node(strings_dictionary->root, sc_addr_to_str(addr));

  if (content == null_ptr || content->sc_string == null_ptr)
    return null_ptr;

  sc_uint32 len = content->string_size;
  sc_char * copy = g_new0(sc_char, len + 1);
  return memcpy(copy, content->sc_string, len);
}

sc_dictionary_node * sc_dictionary_fs_storage_get_node(sc_addr addr)
{
  if (SC_ADDR_IS_EMPTY(addr))
    return null_ptr;

  sc_link_content * content = sc_dictionary_get_data_from_node(strings_dictionary->root, sc_addr_to_str(addr));

  if (content == null_ptr || content->node == null_ptr)
    return null_ptr;

  return content->node;
}

sc_link_content * sc_dictionary_fs_storage_get_link_content(sc_addr addr)
{
  if (SC_ADDR_IS_EMPTY(addr))
    return null_ptr;

  sc_link_content * content = sc_dictionary_get_data_from_node(strings_dictionary->root, sc_addr_to_str(addr));

  if (content == null_ptr || content->node == null_ptr || content->sc_string == null_ptr)
    return null_ptr;

  return content;
}

void sc_dictionary_fs_storage_get_sc_string_ext(sc_addr addr, sc_char ** sc_string, sc_uint32 * size)
{
  if (SC_ADDR_IS_EMPTY(addr))
  {
    *sc_string = null_ptr;
    *size = 0;
    return;
  }

  sc_link_content * content = sc_dictionary_get_data_from_node(strings_dictionary->root, sc_addr_to_str(addr));
  if (content == null_ptr)
  {
    *sc_string = null_ptr;
    *size = 0;
    return;
  }

  sc_uint32 len = content->string_size;
  *sc_string = g_new0(sc_char, len + 1);
  memcpy(*sc_string, content->sc_string, len);
  *size = len;
}

void sc_fs_storage_write_nodes(
    void (*callable)(sc_dictionary_node *, void **),
    GIOChannel * strings_dest,
    GIOChannel * links_dest)
{
  GIOChannel ** dest = g_new0(GIOChannel *, 2);
  dest[0] = strings_dest;
  dest[1] = links_dest;

  sc_dictionary_visit_down_nodes(strings_dictionary, callable, (void **)dest);

  g_free(dest);
}

void sc_fs_storage_write_node(sc_dictionary_node * node, void ** dest)
{
  sc_link_content * content = (sc_link_content *)node->data_list->begin->data;

  if (!sc_dc_node_access_lvl_check_read(content->node))
    return;

  sc_addr_hash * hashes = sc_list_to_hashes_array(content->node->data_list);
  sc_uint8 hashes_size = content->node->data_list->size;

  sc_dc_node_access_lvl_make_no_read(content->node);

  GIOChannel * strings_channel = dest[0];
  g_assert(strings_channel != null_ptr);
  GIOChannel * links_channel = dest[1];
  g_assert(links_channel != null_ptr);

  gsize bytes;
  if (g_io_channel_write_chars(strings_channel, (sc_char *)&hashes_size, sizeof(hashes_size), &bytes, null_ptr) !=
      G_IO_STATUS_NORMAL)
    g_error("Can't write string hashes size %d into %s", hashes_size, strings_path);

  if (g_io_channel_write_chars(
          strings_channel, (sc_char *)hashes, sizeof(sc_addr_hash) * hashes_size, &bytes, null_ptr) !=
      G_IO_STATUS_NORMAL)
    g_error("Can't write string hashes %llu into %s", *hashes, strings_path);

  if (g_io_channel_write_chars(
          strings_channel, (sc_char *)&content->string_size, sizeof(content->string_size), &bytes, null_ptr) !=
      G_IO_STATUS_NORMAL)
    g_error("Can't write sc-string size %d into %s", hashes_size, strings_path);

  if (g_io_channel_write_chars(strings_channel, content->sc_string, content->string_size, &bytes, null_ptr) !=
      G_IO_STATUS_NORMAL)
    g_error("Can't write sc-string %s into %s", content->sc_string, strings_path);

  if (g_io_channel_write_chars(
          links_channel, (sc_char *)hashes, sizeof(sc_addr_hash) * hashes_size, &bytes, null_ptr) != G_IO_STATUS_NORMAL)
    g_error("Can't write string hashes %llu into %s", *hashes, links_path);
}

sc_bool sc_dictionary_fs_storage_write_strings()
{
  sc_char * strings_filename = null_ptr;
  GIOChannel * output_strings = sc_fs_open_tmp_file(file_path, &strings_filename, "strings");
  g_io_channel_set_encoding(output_strings, null_ptr, null_ptr);

  sc_char * links_filename = null_ptr;
  GIOChannel * output_links = sc_fs_open_tmp_file(file_path, &links_filename, "links");
  g_io_channel_set_encoding(output_links, null_ptr, null_ptr);

  sc_fs_storage_write_nodes(sc_fs_storage_write_node, output_strings, output_links);

  if (g_file_test(strings_filename, G_FILE_TEST_IS_REGULAR))
  {
    g_io_channel_shutdown(output_strings, SC_TRUE, null_ptr);
    output_strings = null_ptr;

    if (g_rename(strings_filename, strings_path) != 0)
      g_error("Can't rename %s -> %s", strings_filename, strings_path);
  }

  if (g_file_test(links_filename, G_FILE_TEST_IS_REGULAR))
  {
    g_io_channel_shutdown(output_links, SC_TRUE, null_ptr);
    output_links = null_ptr;

    if (g_rename(links_filename, links_path) != 0)
      g_error("Can't rename %s -> %s", links_filename, links_path);
  }

  if (strings_filename != null_ptr)
    g_free(strings_filename);
  if (output_strings != null_ptr)
    g_io_channel_shutdown(output_strings, SC_TRUE, null_ptr);

  if (links_filename != null_ptr)
    g_free(links_filename);
  if (output_links != null_ptr)
    g_io_channel_shutdown(output_links, SC_TRUE, null_ptr);

  return SC_TRUE;
}

sc_bool sc_dictionary_fs_storage_read_strings()
{
  if (g_file_test(strings_path, G_FILE_TEST_IS_REGULAR) == SC_FALSE)
  {
    g_message("There are no strings in %s", strings_path);
    return SC_FALSE;
  }

  GIOChannel * in_file = g_io_channel_new_file(strings_path, "r", null_ptr);
  if (in_file == null_ptr)
  {
    g_critical("Can't open strings from: %s", strings_path);
    return SC_FALSE;
  }

  if (g_io_channel_set_encoding(in_file, null_ptr, null_ptr) != G_IO_STATUS_NORMAL)
  {
    g_critical("Can't setup encoding: %s", strings_path);
    return SC_FALSE;
  }

  gsize bytes_num = 0;
  while (SC_TRUE)
  {
    sc_uint8 hashes_size = 0;
    if (g_io_channel_read_chars(in_file, (sc_char *)&hashes_size, sizeof(hashes_size), &bytes_num, null_ptr) !=
        G_IO_STATUS_NORMAL)
      break;

    sc_addr_hash * hashes = g_new0(sc_addr_hash, hashes_size);
    if (g_io_channel_read_chars(in_file, (sc_char *)hashes, sizeof(sc_addr_hash) * hashes_size, &bytes_num, null_ptr) !=
        G_IO_STATUS_NORMAL)
      break;

    sc_uint32 string_size = 0;
    if (g_io_channel_read_chars(in_file, (sc_char *)&string_size, sizeof(string_size), &bytes_num, null_ptr) !=
        G_IO_STATUS_NORMAL)
      break;

    sc_char * string = g_new0(sc_char, string_size);
    if (g_io_channel_read_chars(in_file, (sc_char *)string, string_size, &bytes_num, null_ptr) != G_IO_STATUS_NORMAL)
      break;

    sc_uint8 i;
    for (i = 0; i < hashes_size; ++i)
    {
      if (hashes[i] == 0)
        continue;

      sc_addr addr;
      addr.seg = SC_ADDR_LOCAL_SEG_FROM_INT(hashes[i]);
      addr.offset = SC_ADDR_LOCAL_OFFSET_FROM_INT(hashes[i]);

      sc_dictionary_fs_storage_append_sc_link(addr, string, string_size);
    }
  }

  g_io_channel_shutdown(in_file, SC_FALSE, null_ptr);

  g_message("Sc-dictionary loaded");

  return SC_TRUE;
}

#endif
