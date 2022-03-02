/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_set_iterator3.h"
#include "sc_element.h"
#include "sc_storage.h"
#include "../sc_memory_private.h"

#include <glib.h>

const sc_uint32 s_set_max_iterator_lock_attempts = 10;

sc_set_iterator3 * sc_set_iterator3_f_a_a_new(
      const sc_memory_context * ctx,
      sc_addrs beg_addrs,
      sc_types arc_types,
      sc_types end_types)
{
  sc_set_iterator_param p1, p2, p3;

  p1 = init_addrs(beg_addrs);
  p2 = init_types(arc_types);
  p3 = init_types(end_types);

  return sc_set_iterator3_new(ctx, sc_iterator3_f_a_a, p1, p2, p3);
}

sc_set_iterator3 * sc_set_iterator3_a_a_f_new(
      const sc_memory_context * ctx,
      sc_types beg_types,
      sc_types arc_types,
      sc_addrs end_addrs)
{
  sc_set_iterator_param p1, p2, p3;

  p1 = init_types(beg_types);
  p2 = init_types(arc_types);
  p3 = init_addrs(end_addrs);

  return sc_set_iterator3_new(ctx, sc_iterator3_a_a_f, p1, p2, p3);
}

sc_set_iterator3 * sc_set_iterator3_f_a_f_new(
      const sc_memory_context * ctx,
      sc_addrs beg_addrs,
      sc_types arc_types,
      sc_addrs end_addrs)
{
  sc_set_iterator_param p1, p2, p3;

  p1 = init_addrs(beg_addrs);
  p2 = init_types(arc_types);
  p3 = init_addrs(end_addrs);

  return sc_set_iterator3_new(ctx, sc_iterator3_f_a_f, p1, p2, p3);
}

sc_set_iterator3 * sc_set_iterator3_a_f_a_new(
      sc_memory_context const * ctx,
      sc_types beg_types,
      sc_addrs arc_addrs,
      sc_types end_types)
{
  sc_set_iterator_param p1, p2, p3;

  p1 = init_types(beg_types);
  p2 = init_addrs(arc_addrs);
  p3 = init_types(end_types);

  return sc_set_iterator3_new(ctx, sc_iterator3_a_f_a, p1, p2, p3);
}

sc_set_iterator3 * sc_set_iterator3_f_f_a_new(
      sc_memory_context const * ctx,
      sc_addrs beg_addrs,
      sc_addrs arc_addrs,
      sc_types end_types)
{
  sc_set_iterator_param p1, p2, p3;

  p1 = init_addrs(beg_addrs);
  p2 = init_addrs(arc_addrs);
  p3 = init_types(end_types);

  return sc_set_iterator3_new(ctx, sc_iterator3_f_f_a, p1, p2, p3);
}

sc_set_iterator3 * sc_set_iterator3_a_f_f_new(
      sc_memory_context const * ctx,
      sc_types beg_types,
      sc_addrs arc_addrs,
      sc_addrs end_addrs)
{
  sc_set_iterator_param p1, p2, p3;

  p1 = init_types(beg_types);
  p2 = init_addrs(arc_addrs);
  p3 = init_addrs(end_addrs);

  return sc_set_iterator3_new(ctx, sc_iterator3_a_f_f, p1, p2, p3);
}

sc_set_iterator3 * sc_set_iterator3_f_f_f_new(
      sc_memory_context const * ctx,
      sc_addrs beg_addrs,
      sc_addrs arc_addrs,
      sc_addrs end_addrs)
{
  sc_set_iterator_param p1, p2, p3;

  p1 = init_addrs(beg_addrs);
  p2 = init_addrs(arc_addrs);
  p3 = init_addrs(end_addrs);

  return sc_set_iterator3_new(ctx, sc_iterator3_f_f_f, p1, p2, p3);
}

sc_bool sc_iterator_ref_element(const sc_memory_context * ctx, sc_addr addr)
{
  sc_element * el = 0;
  sc_uint16 a = 0;

  while (a < 1000)
  {
    STORAGE_CHECK_CALL(sc_storage_element_lock(addr, &el));
    if (el != null_ptr &&
        sc_element_is_request_deletion(el) == SC_FALSE &&
        el->flags.type != 0)
    {
      sc_storage_element_ref(addr);
      STORAGE_CHECK_CALL(sc_storage_element_unlock(addr));
      return SC_TRUE;
    }
    STORAGE_CHECK_CALL(sc_storage_element_unlock(addr));

    a++;
    g_usleep(1000);
  }

  return SC_FALSE;
}

sc_bool sc_iterator_ref_elements(const sc_memory_context * ctx, sc_addrs addrs)
{
  sc_uint8 i = 0;
  for (; i < addrs.size; i++)
  {
    if (!sc_iterator_ref_element(ctx, addrs.addrs[i]))
      return SC_FALSE;
  }
  return SC_TRUE;
}

sc_set_iterator3 * sc_set_iterator3_new(
      const sc_memory_context * ctx,
      sc_iterator3_type type,
      sc_set_iterator_param p1,
      sc_set_iterator_param p2,
      sc_set_iterator_param p3)
{
  // check types
  if (type >= sc_iterator3_count)
    return (sc_set_iterator3*) null_ptr;

  // check params with template
  switch (type)
  {
  case sc_iterator3_f_a_a:
    if (p1.types.size || !p2.types.size || !p3.types.size ||
        sc_iterator_ref_elements(ctx, p1.addrs) != SC_TRUE)
    {
      return (sc_set_iterator3*) null_ptr;
    }
    break;

  case sc_iterator3_a_a_f:
    if (!p1.types.size || !p2.types.size || p3.types.size ||
        sc_iterator_ref_elements(ctx, p3.addrs) != SC_TRUE)
    {
      return (sc_set_iterator3*) null_ptr;
    }
    break;

  case sc_iterator3_f_a_f:
    if (p1.types.size || !p2.types.size || p3.types.size ||
        sc_iterator_ref_elements(ctx, p1.addrs) != SC_TRUE ||
        sc_iterator_ref_elements(ctx, p3.addrs) != SC_TRUE)
    {
      g_print("%s", "size");
      return (sc_set_iterator3*) null_ptr;
    }
    break;

  case sc_iterator3_a_f_a:
    if (!p1.types.size || p2.types.size || !p3.types.size ||
        sc_iterator_ref_elements(ctx, p2.addrs) != SC_TRUE)
    {
      return (sc_set_iterator3*) null_ptr;
    }
    break;

  case sc_iterator3_f_f_a:
    if (p1.types.size || p2.types.size || !p3.types.size ||
        sc_iterator_ref_elements(ctx, p1.addrs) != SC_TRUE ||
        sc_iterator_ref_elements(ctx, p2.addrs) != SC_TRUE)
    {
      return (sc_set_iterator3*)0;
    }
    break;

  case sc_iterator3_a_f_f:
    if (!p1.types.size || p2.types.size || p3.types.size ||
        sc_iterator_ref_elements(ctx, p2.addrs) != SC_TRUE ||
        sc_iterator_ref_elements(ctx, p3.addrs) != SC_TRUE)
    {
      return (sc_set_iterator3*) null_ptr;
    }
    break;

  case sc_iterator3_f_f_f:
    if (p1.types.size || p2.types.size || p3.types.size ||
        sc_iterator_ref_elements(ctx, p1.addrs) != SC_TRUE ||
        sc_iterator_ref_elements(ctx, p2.addrs) != SC_TRUE ||
        sc_iterator_ref_elements(ctx, p3.addrs) != SC_TRUE)
    {
      return (sc_set_iterator3*) null_ptr;
    }
    break;

  default:
    break;
  }

  sc_set_iterator3 * it = g_new0(sc_set_iterator3, 1);

  it->params[0] = p1;
  it->params[1] = p2;
  it->params[2] = p3;

  it->type = type;
  it->ctx = ctx;
  it->current_constr = 0;
  it->finished = SC_FALSE;

  return it;
}

void sc_set_iterator3_free(sc_set_iterator3 *it)
{
  if (it == null_ptr)
    return;

  if ((it->finished == SC_FALSE) && SC_ADDR_IS_EMPTY(it->results[1]))
  {
    sc_element *el = 0;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->results[1], &el));
    g_assert(el != null_ptr);
    sc_storage_element_unref(it->results[1]);
    STORAGE_CHECK_CALL(sc_storage_element_unlock(it->results[1]));
  }
  switch (it->type)
  {
  case sc_iterator3_f_a_a:
    sc_storage_elements_unref(it->params[0].addrs);
    break;

  case sc_iterator3_a_a_f:
    sc_storage_elements_unref(it->params[2].addrs);
    break;

  case sc_iterator3_f_a_f:
    sc_storage_elements_unref(it->params[0].addrs);
    sc_storage_elements_unref(it->params[2].addrs);
    break;

  case sc_iterator3_a_f_a:
    sc_storage_elements_unref(it->params[1].addrs);
    break;

  case sc_iterator3_f_f_a:
    sc_storage_elements_unref(it->params[0].addrs);
    sc_storage_elements_unref(it->params[1].addrs);
    break;

  case sc_iterator3_a_f_f:
    sc_storage_elements_unref(it->params[1].addrs);
    sc_storage_elements_unref(it->params[2].addrs);
    break;

  case sc_iterator3_f_f_f:
    sc_storage_elements_unref(it->params[0].addrs);
    sc_storage_elements_unref(it->params[1].addrs);
    sc_storage_elements_unref(it->params[2].addrs);
    break;

  default:
    break;
  }

  g_free(it);
}

sc_bool sc_set_iterator_param_compare(sc_element * el, sc_addr addr, sc_set_iterator_param param)
{
  g_assert(el != 0);

  if (param.types.size)
    return in_sc_types(el->flags.type, param.types);
  else
    return in_sc_addrs(addr, param.addrs);
}


sc_bool _sc_set_iterator3_f_a_a_next(sc_set_iterator3 *it)
{
  sc_addr arc_addr;
  SC_ADDR_MAKE_EMPTY(arc_addr);

  it->results[0] = it->params[0].addrs.addrs[it->current_constr];

  // try to find first output arc
  if (SC_ADDR_IS_EMPTY(it->results[1]))
  {
    sc_element *elem = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->results[0], &elem));
    g_assert(elem != null_ptr);
    arc_addr = elem->first_out_arc;
    STORAGE_CHECK_CALL(sc_storage_element_unlock(it->results[0]));
  }
  else
  {
    sc_element *elem = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->results[1], &elem));
    g_assert(elem != null_ptr);
    arc_addr = elem->arc.next_out_arc;
    sc_storage_element_unref(it->results[1]);
    STORAGE_CHECK_CALL(sc_storage_element_unlock(it->results[1]));
  }

  // iterate through output arcs
  while (SC_ADDR_IS_NOT_EMPTY(arc_addr))
  {
    sc_element *el = null_ptr;
    // lock required elements to prevent deadlock with deletion
    while (el == null_ptr)
      STORAGE_CHECK_CALL(sc_storage_element_lock_try(arc_addr, s_set_max_iterator_lock_attempts, &el));

    sc_storage_element_ref(arc_addr);

    sc_addr next_out_arc = el->arc.next_out_arc;
    if (sc_element_is_request_deletion(el) == SC_FALSE)
    {
      sc_addr arc_end = el->arc.end;
      sc_type arc_type = el->flags.type;
      sc_access_levels arc_access = el->flags.access_levels;
      sc_access_levels end_access;
      if (sc_storage_get_access_levels(it->ctx, arc_end, &end_access) != SC_RESULT_OK)
        end_access = sc_access_lvl_make_max;

      STORAGE_CHECK_CALL(sc_storage_element_unlock(arc_addr));

      sc_type el_type;
      sc_storage_get_element_type(it->ctx, arc_end, &el_type);

      if (in_sc_types(arc_type, it->params[1].types) &&
          in_sc_types(el_type, it->params[2].types) &&
          sc_access_lvl_check_read(it->ctx->access_levels, arc_access) &&
          sc_access_lvl_check_read(it->ctx->access_levels, end_access))
      {
        // store found result
        it->results[1] = arc_addr;
        it->results[2] = arc_end;

        return SC_TRUE;
      }
    } else
    {
      sc_storage_element_unref(arc_addr);
      STORAGE_CHECK_CALL(sc_storage_element_unlock(arc_addr));
    }

    // go to next arc
    arc_addr = next_out_arc;
  }

//  if (it->current_constr < it->params[0].addrs.size)
//  {
//    it->current_constr++;
//    SC_ADDR_MAKE_EMPTY(it->results[1]);
//    return _sc_set_iterator3_f_a_a_next(it);
//  }

  it->finished = SC_TRUE;

  return SC_FALSE;
}

sc_bool _sc_set_iterator3_f_a_f_next(sc_set_iterator3 *it)
{
  sc_addr arc_addr;

  SC_ADDR_MAKE_EMPTY(arc_addr);

  it->results[0] = it->params[0].addrs.addrs[it->current_constr];
  it->results[2] = it->params[2].addrs.addrs[it->current_constr];

  // try to find first input arc
  if (SC_ADDR_IS_EMPTY(it->results[1]))
  {
    sc_element *el = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->results[2], &el));
    g_assert(el != null_ptr);
    arc_addr = el->first_in_arc;
    STORAGE_CHECK_CALL(sc_storage_element_unlock(it->results[2]));
  }
  else
  {
    sc_element *el = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->results[1], &el));
    g_assert(el != null_ptr);
    arc_addr = el->arc.next_in_arc;
    sc_storage_element_unref(it->results[1]);
    STORAGE_CHECK_CALL(sc_storage_element_unlock(it->results[1]));
  }

  // trying to find input arc, that created before iterator, and wasn't deleted
  while (SC_ADDR_IS_NOT_EMPTY(arc_addr))
  {
    sc_element *el = null_ptr;
    while (el == null_ptr)
      STORAGE_CHECK_CALL(sc_storage_element_lock_try(arc_addr, s_set_max_iterator_lock_attempts, &el));

    sc_storage_element_ref(arc_addr);

    sc_addr next_in_arc = el->arc.next_in_arc;
    if (sc_element_is_request_deletion(el) == SC_FALSE)
    {
      sc_type arc_type = el->flags.type;
      sc_addr arc_begin = el->arc.begin;
      sc_access_levels arc_access = el->flags.access_levels;

      STORAGE_CHECK_CALL(sc_storage_element_unlock(arc_addr));

      if (in_sc_addrs(arc_begin, it->params[0].addrs) &&
          in_sc_types(arc_type, it->params[1].types) &&
          sc_access_lvl_check_read(it->ctx->access_levels, arc_access))
      {
        // store found result
        it->results[1] = arc_addr;
        return SC_TRUE;
      }
    } else
    {
      sc_storage_element_unref(arc_addr);
      STORAGE_CHECK_CALL(sc_storage_element_unlock(arc_addr));
    }

    // go to next arc
    arc_addr = next_in_arc;
  }

  it->finished = SC_TRUE;

  return SC_FALSE;
}

sc_bool _sc_set_iterator3_a_a_f_next(sc_set_iterator3 * it)
{
  sc_addr arc_addr;
  SC_ADDR_MAKE_EMPTY(arc_addr)

  it->results[2] = it->params[2].addrs.addrs[0];

  // try to find first input arc
  if (SC_ADDR_IS_EMPTY(it->results[1]))
  {
    sc_element *el = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->results[2], &el));
    g_assert(el != null_ptr);
    arc_addr = el->first_in_arc;
    STORAGE_CHECK_CALL(sc_storage_element_unlock(it->results[2]));
  }else
  {
    sc_element *el = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->results[1], &el));
    g_assert(el != null_ptr);
    arc_addr = el->arc.next_in_arc;
    sc_storage_element_unref(it->results[1]);
    STORAGE_CHECK_CALL(sc_storage_element_unlock(it->results[1]));
  }

  // trying to find input arc, that created before iterator, and wasn't deleted
  while (SC_ADDR_IS_NOT_EMPTY(arc_addr))
  {
    sc_element *el = 0;
    while (el == null_ptr)
      STORAGE_CHECK_CALL(sc_storage_element_lock_try(arc_addr, s_set_max_iterator_lock_attempts, &el));

    sc_storage_element_ref(arc_addr);

    sc_addr next_in_arc = el->arc.next_in_arc;
    if (sc_element_is_request_deletion(el) == SC_FALSE)
    {
      sc_type arc_type = el->flags.type;
      sc_addr arc_begin = el->arc.begin;
      sc_access_levels arc_access = el->flags.access_levels;
      sc_access_levels begin_access;
      if (sc_storage_get_access_levels(it->ctx, arc_begin, &begin_access) != SC_RESULT_OK)
        begin_access = sc_access_lvl_make_max;

      STORAGE_CHECK_CALL(sc_storage_element_unlock(arc_addr));

      sc_type el_type = 0;
      sc_storage_get_element_type(it->ctx, arc_begin, &el_type);

      if (in_sc_types(arc_type, it->params[1].types) &&
            in_sc_types(el_type, it->params[0].types) &&
          sc_access_lvl_check_read(it->ctx->access_levels, arc_access) &&
          sc_access_lvl_check_read(it->ctx->access_levels, begin_access)
          )
      {
        // store found result
        it->results[1] = arc_addr;
        it->results[0] = arc_begin;

        return SC_TRUE;
      }
    } else
    {
      sc_storage_element_unref(arc_addr);
      STORAGE_CHECK_CALL(sc_storage_element_unlock(arc_addr));
    }

    // go to next arc
    arc_addr = next_in_arc;
  }

  it->finished = SC_TRUE;

  return SC_FALSE;
}

sc_bool _sc_set_iterator3_a_f_a_next(sc_set_iterator3 * it)
{
  if (!it->finished)
  {
    it->finished = SC_TRUE;
    it->results[1] = it->params[1].addrs.addrs[0];

    sc_element * arc_el = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->results[1], &arc_el));
    g_assert(arc_el != null_ptr);
    it->results[0] = arc_el->arc.begin;
    it->results[2] = arc_el->arc.end;
    STORAGE_CHECK_CALL(sc_storage_elements_unlock(it->params[1].addrs));

    return SC_TRUE;
  }

  return SC_FALSE;
}

sc_bool _sc_set_iterator3_f_f_a_next(sc_set_iterator3 * it)
{
  if (!it->finished)
  {
    it->finished = SC_TRUE;
    sc_bool result = SC_FALSE;

    sc_element *edge_el = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->params[1].addrs.addrs[0], &edge_el));
    g_assert(edge_el != null_ptr);
    if (in_sc_addrs(edge_el->arc.begin, it->params[0].addrs))
    {
      result = SC_TRUE;
      it->results[0] = edge_el->arc.begin;
      it->results[1] = it->params[1].addrs.addrs[0];
      it->results[2] = edge_el->arc.end;
    }
    STORAGE_CHECK_CALL(sc_storage_elements_unlock(it->params[1].addrs));

    return result;
  }

  return SC_FALSE;
}

sc_bool _sc_set_iterator3_a_f_f_next(sc_set_iterator3 * it)
{
  if (!it->finished)
  {
    it->finished = SC_TRUE;
    sc_bool result = SC_FALSE;

    sc_element *edge_el = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->params[1].addrs.addrs[0], &edge_el));
    g_assert(edge_el != null_ptr);
    if (in_sc_addrs(edge_el->arc.end, it->params[2].addrs))
    {
      result = SC_TRUE;
      it->results[0] = edge_el->arc.begin;
      it->results[1] = it->params[1].addrs.addrs[0];
      it->results[2] = edge_el->arc.end;
    }
    STORAGE_CHECK_CALL(sc_storage_elements_unlock(it->params[1].addrs));

    return result;
  }

  return SC_FALSE;
}

sc_bool _sc_set_iterator3_f_f_f_next(sc_set_iterator3 * it)
{
  if (!it->finished)
  {
    it->finished = SC_TRUE;
    sc_bool result = SC_FALSE;

    sc_element *edge_el = null_ptr;
    STORAGE_CHECK_CALL(sc_storage_element_lock(it->params[1].addrs.addrs[0], &edge_el));
    g_assert(edge_el != null_ptr);
    if (in_sc_addrs(edge_el->arc.begin, it->params[0].addrs) &&
        in_sc_addrs(edge_el->arc.end, it->params[2].addrs))
    {
      result = SC_TRUE;
      it->results[0] = edge_el->arc.begin;
      it->results[1] = it->params[1].addrs.addrs[0];
      it->results[2] = edge_el->arc.end;
    }
    STORAGE_CHECK_CALL(sc_storage_elements_unlock(it->params[1].addrs));

    return result;
  }

  return SC_FALSE;
}

sc_bool sc_set_iterator3_next(sc_set_iterator3 * it)
{
  if ((it == null_ptr) || (it->finished == SC_TRUE))
    return SC_FALSE;

  switch (it->type)
  {

  case sc_iterator3_f_a_a:
    return _sc_set_iterator3_f_a_a_next(it);

  case sc_iterator3_f_a_f:
    return _sc_set_iterator3_f_a_f_next(it);

  case sc_iterator3_a_a_f:
    return _sc_set_iterator3_a_a_f_next(it);

  case sc_iterator3_a_f_a:
    return _sc_set_iterator3_a_f_a_next(it);

  case sc_iterator3_f_f_a:
    return _sc_set_iterator3_f_f_a_next(it);

  case sc_iterator3_a_f_f:
    return _sc_set_iterator3_a_f_f_next(it);

  case sc_iterator3_f_f_f:
    return _sc_set_iterator3_f_f_f_next(it);

  default:
    break;
  }

  return SC_FALSE;
}

sc_addr sc_set_iterator3_value(sc_set_iterator3 * it, sc_uint vid)
{
  g_assert(it != 0);
  g_assert(vid < 3);

  return it->results[vid];
}

sc_bool in_sc_types(sc_type el_type, sc_types types)
{
  sc_uint i = 0;
  for (; i < types.size; ++i)
  {
    if ((types.types[i] & sc_flags_remove(el_type)) == types.types[i])
      return SC_TRUE;
  }

  return SC_FALSE;
}

sc_bool in_sc_addrs(sc_addr addr, sc_addrs addrs)
{
  sc_uint i = 0;
  for (; i < addrs.size; i++)
  {
    if (SC_ADDR_IS_EQUAL(addrs.addrs[i], addr))
      return SC_TRUE;
  }

  return SC_FALSE;
}

sc_bool sc_addrs_is_empty(sc_addrs addrs)
{
  sc_uint i = 0;
  for (; i < addrs.size; i++)
  {
    if (!SC_ADDR_IS_EMPTY(addrs.addrs[i]))
      return SC_FALSE;
  }

  return SC_TRUE;
}

void check_sc_elements(sc_elements * elems)
{
  sc_uint i = 0;
  for (; i < elems->size; i++)
  {
    g_assert(elems->elements[i] != null_ptr);
  }
}

sc_set_iterator_param init_addrs(sc_addrs addrs)
{
  sc_set_iterator_param param;

  param.addrs = addrs;
  param.types.types = null_ptr;
  param.types.size = 0;

  return param;
}

sc_set_iterator_param init_types(sc_types types)
{
  sc_set_iterator_param param;

  param.addrs.addrs = null_ptr;
  param.addrs.size = 0;
  param.types = types;

  return param;
}
