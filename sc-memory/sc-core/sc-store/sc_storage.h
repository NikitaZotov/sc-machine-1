/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef _sc_storage_h_
#define _sc_storage_h_

#include "sc_types.h"
#include "sc_defines.h"
#include "sc_stream.h"
#include "sc-container/sc-list/sc_list.h"
#include "sc-container/sc-dictionary/sc_dictionary.h"

#define SC_ADDR_EMPTY \
  (sc_addr) \
  { \
    0, 0 \
  }
#define INVALID_OFFSET 0

#define SC_ELEMENT_SIZE 34
#define MAX_SC_CONNECTOR_TYPE_CODE 45

typedef struct _sc_storage
{
  sc_char * path;  // path to all dictionary files

  sc_char * elements_types_path;  // path to elements types
  void * elements_types_channel;
  sc_uint64 last_addr_hash;     // last element addr hash

  sc_char * connectors_elements_path;  // path to connectors elements
  void * connectors_elements_channel;
  sc_uint64 last_connector_elements_offset;

  sc_char * input_connectors_path;
  sc_list **** input_connectors_segments;
  void * input_connectors_channel;
  sc_uint64 last_input_connectors_offset;

  sc_char * output_connectors_path;
  sc_list **** output_connectors_segments;
  void * output_connectors_channel;
  sc_uint64 last_output_connectors_offset;

  sc_uint32 max_segments;
  sc_uint32 max_slots_in_segment;
  sc_uint32 max_connectors_in_slot;
} sc_storage;

//! Initialize sc storage in specified path
sc_result sc_storage_initialize(
    sc_storage ** storage,
    sc_char const * path,
    sc_uint64 max_searchable_string_size,
    sc_bool clear);

//! Shutdown sc storage
sc_result sc_storage_shutdown(sc_storage * storage, sc_bool save_state);

/*! Create new sc-node
 * @param type Type of new sc-node
 * @return Return sc-addr of created sc-node or empty sc-addr if sc-node wasn't created
 */
sc_addr sc_storage_node_new(sc_storage * storage, sc_type type);

/*! Create new sc-link
 * @param type Type of new sc-link
 * @return Return sc-addr of created sc-link or empty sc-addr if sc-link wasn't created
 */
sc_addr sc_storage_link_new(sc_storage * storage, sc_type type);

/*! Check if sc-element with specified sc-addr exist
 * @param addr sc-addr of element
 * @return Returns SC_TRUE, if sc-element with \p addr exist; otherwise return false.
 * If element deleted, then return SC_FALSE.
 */
sc_bool sc_storage_is_element(sc_storage * storage, sc_addr addr);

/*! Create new sc-connector.
 * @param type Type of new sc-connector
 * @param beg sc-addr of begin sc-element
 * @param end sc-addr of end sc-element
 *
 * @return Return sc-addr of created sc-arc or empty sc-addr if sc-arc wasn't created
 */
sc_addr sc_storage_connector_new(sc_storage * storage, sc_type type, sc_addr beg, sc_addr end);

sc_uint16 sc_storage_define_connector_type_code(sc_type connector_type);

sc_list ** sc_storage_resolve_element_typed_connectors(
    sc_storage * storage,
    sc_list **** connectors_segments,
    sc_addr connector_element_addr);

/*! Remove sc-element from storage
 * @param addr sc-addr of element to erase
 * @return If input params are correct and element erased, then return SC_OK;
 * otherwise return SC_ERROR
 */
sc_result sc_storage_element_free(sc_storage * storage, sc_addr addr);

/*! Get type of sc-element with specified sc-addr
 * @param addr sc-addr of element to get type
 * @param result Pointer to result container
 * @return If input params are correct and type resolved, then return SC_OK;
 * otherwise return SC_ERROR
 */
sc_result sc_storage_get_element_type(sc_storage * storage, sc_addr addr, sc_type * result);

/*! Change element subtype
 * @param addr sc-addr of element to set new subtype
 * @param type New type of sc-element
 * @return If type changed, then returns SC_RESULT_OK; otherwise returns SC_RESULT_ERROR
 */
sc_result sc_storage_change_element_subtype(sc_storage * storage, sc_addr addr, sc_type type);

/*! Returns sc-addr of begin element of specified arc
 * @param addr sc-addr of arc to get begin element
 * @param result Pointer to result container
 * @return If input params are correct and begin element resolved, then return SC_OK.
 * If element with specified addr isn't an arc, then return SC_INVALID_TYPE
 */
sc_result sc_storage_get_arc_begin(sc_storage * storage, sc_addr addr, sc_addr * result);

/*! Returns sc-addr of end element of specified arc
 * @param addr sc-addr of arc to get end element
 * @param result Pointer to result container
 * @return If input params are correct and end element resolved, then return SC_OK.
 * If element with specified addr isn't an arc, then return SC_INVALID_TYPE
 */
sc_result sc_storage_get_arc_end(sc_storage * storage, sc_addr addr, sc_addr * result);

/*! Like a sc_storage_get_arc_begin and sc_storage_get_arc_end call
 * @see sc_storage_get_arc_begin, @see sc_storage_get_arc_end
 */
sc_result sc_storage_get_arc_info(
    sc_storage * storage,
    sc_addr addr,
    sc_addr * result_begin_addr,
    sc_addr * result_end_addr);

/*! Setup content data for specified sc-link
 * @param addr sc-addr of sc-link to setup content
 * @param stream Pointer to stream
 * @return If content of specified link changed without any errors, then return SC_OK; otherwise
 * returns on of error codes:
 * <ul>
 * <li>SC_INVALID_TYPE - element with \p addr isn't a sc-link</li>
 * <li>SC_ERROR_INVALID_PARAMS - element with specified \p addr doesn't exist
 * <li>SC_ERROR - unknown error</li>
 * </ul>
 */
sc_result sc_storage_set_link_content(sc_storage * storage, sc_addr addr, const sc_stream * stream);

/*! Returns content data from specified sc-link
 * @param addr sc-addr of sc-link to get content data
 * @param stream Pointer to returned data stream
 * @return If content of specified link returned without any errors, then return SC_OK; otherwise
 * returns on of error codes:
 * <ul>
 * <li>SC_INVALID_TYPE - element with \p addr isn't a sc-link</li>
 * <li>SC_ERROR_INVALID_PARAMS - element with specified \p addr doesn't exist
 * <li>SC_ERROR - unknown error</li>
 * </ul>
 */
sc_result sc_storage_get_link_content(sc_storage * storage, sc_addr addr, sc_stream ** stream);

/*! Search sc-link addrs by specified data
 * @param stream Pointer to stream that contains data for search
 * @param result_addrs Pointer to result container
 * @return If sc-links with specified content found, then sc-addrs of found link
 * writes into \p result array and function returns SC_OK; otherwise \p function returns SC_OK.
 * In any case \p result_count contains number of found sc-addrs.
 * @attention \p result array need to be free after usage
 */
sc_result sc_storage_find_links_with_content_string(
    sc_storage * storage,
    sc_stream const * stream,
    sc_list ** result_addrs);

/*! Search sc-link addrs by specified data substring
 * @param stream Pointer to stream that contains data for search
 * @param result_hashes Pointer to result container of sc-links with specified data started with substring
 * @param result_count Container for results count
 * @param max_length_to_search_as_prefix Search by prefix as substring length <= max_length_to_search_as_prefix
 * @return If sc-links with specified substring found, then sc-addrs of found link
 * writes into \p result array and function returns SC_RESULT_OK; otherwise function returns SC_RESULT_OK.
 * In any case \p result_count contains number of found sc-addrs.
 * @attention \p result array need to be free after usage
 */
sc_result sc_storage_find_links_by_content_substring(
    sc_storage * storage,
    sc_stream const * stream,
    sc_list ** result_hashes,
    sc_uint32 max_length_to_search_as_prefix);

/*! Search sc-strings by specified substring
 * @param stream Pointer to stream that contains data for search
 * @param result_strings Pointer to result container of sc-strings with substring
 * @param max_length_to_search_as_prefix Search by prefix as substring length <= max_length_to_search_as_prefix
 * @return If sc-strings with specified substring found, then they
 * writes into \p result array and function returns SC_RESULT_OK; otherwise function returns SC_RESULT_OK.
 * In any case \p result_count contains number of found sc-strings.
 * @attention \p result array need to be free after usage
 */
sc_result sc_storage_find_links_contents_by_content_substring(
    sc_storage * storage,
    sc_stream const * stream,
    sc_list ** result_strings,
    sc_uint32 max_length_to_search_as_prefix);

sc_result sc_storage_save(sc_storage * storage, sc_memory_context const * ctx);

#endif
