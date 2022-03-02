/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef _sc_set_iterator3_h_
#define _sc_set_iterator3_h_

#include "sc_defines.h"
#include "sc_types.h"
#include "sc_element.h"
#include "sc_iterator_types.h"


/*! Iterator parameter
 */
struct _sc_set_iterator_param
{
  sc_addrs addrs;
  sc_types types;
};

/*! Structure to store iterator information
 */
struct _sc_set_iterator3
{
  sc_iterator3_type type;           // iterator type (search template)
  sc_set_iterator_param params[3];  // parameters array
  sc_addr results[3];              // results array (same size as params)
  const sc_memory_context *ctx;     // pointer to used memory context
  sc_bool finished;
  sc_uint8 current_constr;
};

/*! Create iterator to find output arcs for specified element
 * @param el sc-addr of element to iterate output arcs
 * @param arc_type Type of output arc to iterate (0 - all types)
 * @param end_type Type of end element for output arcs, to iterate
 * @return If iterator created, then return pointer to it; otherwise return null
 */
_SC_EXTERN sc_set_iterator3 * sc_set_iterator3_f_a_a_new(const sc_memory_context * ctx, sc_addrs sources, sc_types arc_types, sc_types end_types);

/*! Create iterator to find input arcs for specified element
 * @param beg_type Type of begin element for input arcs, to iterate
 * @param arc_type Type of input arc to iterate (0 - all types)
 * @param el sc-addr of element to iterate input arcs
 * @return If iterator created, then return pointer to it; otherwise return null
 */
_SC_EXTERN sc_set_iterator3 * sc_set_iterator3_a_a_f_new(sc_memory_context const * ctx, sc_types beg_types, sc_types arc_types, sc_addrs end_addrs);

/*! Create iterator to find arcs between two specified elements
 * @param el_beg sc-addr of begin element
 * @param arc_type Type of arcs to iterate (0 - all types)
 * @param el_end sc-addr of end element
 * @return If iterator created, then return pointer to it; otherwise return null
 */
_SC_EXTERN sc_set_iterator3 * sc_set_iterator3_f_a_f_new(const sc_memory_context *ctx, sc_addrs beg_addrs, sc_types arc_types, sc_addrs end_addrs);

/*! Create iterator to determine edge source and target
 */
_SC_EXTERN sc_set_iterator3 * sc_set_iterator3_a_f_a_new(sc_memory_context const * ctx, sc_types beg_types, sc_addrs arc_addrs, sc_types end_types);

// Requried for clean template search algorithm
_SC_EXTERN sc_set_iterator3 * sc_set_iterator3_f_f_a_new(sc_memory_context const * ctx, sc_addrs beg_addrs, sc_addrs arc_addrs, sc_types end_types);
_SC_EXTERN sc_set_iterator3 * sc_set_iterator3_a_f_f_new(sc_memory_context const * ctx, sc_types beg_types, sc_addrs arc_addrs, sc_addrs end_addrs);
_SC_EXTERN sc_set_iterator3 * sc_set_iterator3_f_f_f_new(sc_memory_context const * ctx, sc_addrs beg_addrs, sc_addrs arc_addrs, sc_addrs end_addrs);

/*! Create new sc-iterator-3
 * @param type Iterator type (search template)
 * @param p1 First iterator parameter
 * @param p2 Second iterator parameter
 * @param p3 Third iterator parameter
 * @return Pointer to created iterator. If parameters invalid for specified iterator type, or type is not a sc-iterator-3, then return 0
 */
_SC_EXTERN sc_set_iterator3 * sc_set_iterator3_new(sc_memory_context const * ctx, sc_iterator3_type type, sc_set_iterator_param p1, sc_set_iterator_param p2, sc_set_iterator_param p3);

/*! Destroy iterator and free allocated memory
 * @param it Pointer to sc-iterator that need to be destroyed
 */
_SC_EXTERN void sc_set_iterator3_free(sc_set_iterator3 * it);

/*! Go to next iterator result
 * @param it Pointer to iterator that we need to go next result
 * @return Return SC_TRUE, if iterator moved to new results; otherwise return SC_FALSE.
 * example: while(sc_iterator_next(it)) { <your code> }
 */
_SC_EXTERN sc_bool sc_set_iterator3_next(sc_set_iterator3 * it);

/*! Get iterator value
 * @param it Pointer to iterator for getting value
 * @param vid Value id (can't be more that 3 for sc-iterator3)
 * @return Return sc-addr of search result value
 */
_SC_EXTERN sc_addr sc_set_iterator3_value(sc_set_iterator3 * it, sc_uint vid);

/*! Check if specified element type passed into
 * iterator selection.
 * @param el_type Compared element type
 * @param it_type Template type from iterator parameter
 * @return If el_type passed checking, then return SC_TRUE, else return SC_FALSE
 */
_SC_EXTERN sc_bool in_sc_types(sc_type el_type, sc_types types);

_SC_EXTERN sc_bool in_sc_addrs(sc_addr addr, sc_addrs addrs);

_SC_EXTERN sc_bool sc_addrs_is_empty(sc_addrs addrs);

_SC_EXTERN void check_sc_elements(sc_elements * elems);

_SC_EXTERN sc_set_iterator_param init_addrs(sc_addrs addrs);

_SC_EXTERN sc_set_iterator_param init_types(sc_types types);

#endif
