/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef _sc_iterator_types_h_
#define _sc_iterator_types_h_

#include "sc_defines.h"
#include "sc_types.h"
#include "sc_element.h"


//! sc-iterator types
typedef enum
{
  sc_iterator3_f_a_a = 0, // outgoing edges
  sc_iterator3_a_a_f,     // ingoing edges
  sc_iterator3_f_a_f,     // edge between source and target
  sc_iterator3_a_f_a,		  // find source/target elements of edge
  // just for clean template search
  sc_iterator3_f_f_a,
  sc_iterator3_a_f_f,
  sc_iterator3_f_f_f,
  sc_iterator3_count

} sc_iterator3_type;

#endif
