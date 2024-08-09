/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc-core/sc-container/sc_string.h"

#include <glib.h>

#include "sc-core/sc_types.h"

sc_bool sc_str_has_prefix(sc_char const * str, sc_char const * prefix)
{
  return g_str_has_prefix(str, prefix);
}
