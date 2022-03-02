/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_set_iterator.hpp"
#include "sc_memory.hpp"
#include "glib.h"


template<> TSetIterator3<ScAddr, ScTypeVector, ScAddr>::TSetIterator3(
    ScMemoryContext const & context,
    ScAddr const & p1, ScTypeVector const & p2, ScAddr const & p3)
{
  m_iterator = sc_set_iterator3_f_a_f_new(
        *context,
        addrsToArray({ p1 }),
        typesToArray(p2),
        addrsToArray({ p3 }));
}

template<> TSetIterator3<ScAddr, ScTypeVector, ScTypeVector>::TSetIterator3(
    ScMemoryContext const & context,
    ScAddr const & p1, ScTypeVector const & p2, ScTypeVector const & p3)
{
  m_iterator = sc_set_iterator3_f_a_a_new(
        *context,
        addrsToArray({ p1 }),
        typesToArray(p2),
        typesToArray(p3));
}

template<> TSetIterator3<ScTypeVector, ScTypeVector, ScAddr>::TSetIterator3(
    ScMemoryContext const & context,
    ScTypeVector const & p1, ScTypeVector const & p2, ScAddr const & p3)
{
  m_iterator = sc_set_iterator3_a_a_f_new(
        *context,
        typesToArray(p1),
        typesToArray(p2),
        addrsToArray({ p3 }));
}

template<> TSetIterator3<ScTypeVector, ScAddr, ScTypeVector>::TSetIterator3(
    ScMemoryContext const & context,
    ScTypeVector const & p1, ScAddr const & p2, ScTypeVector const & p3)
{
  m_iterator = sc_set_iterator3_a_f_a_new(
        *context,
        typesToArray(p1),
        addrsToArray({ p2 }),
        typesToArray(p3));
}

template<> TSetIterator3<ScAddr, ScAddr, ScTypeVector>::TSetIterator3(
  ScMemoryContext const & context,
  ScAddr const & p1, ScAddr const & p2, ScTypeVector const & p3)
{
  m_iterator = sc_set_iterator3_f_f_a_new(
        *context,
        addrsToArray({ p1 }),
        addrsToArray({ p2 }),
        typesToArray(p3));
}

template<> TSetIterator3<ScTypeVector, ScAddr, ScAddr>::TSetIterator3(
  ScMemoryContext const & context,
  ScTypeVector const & p1, ScAddr const & p2, ScAddr const & p3)
{
  m_iterator = sc_set_iterator3_a_f_f_new(
        *context,
        typesToArray(p1),
        addrsToArray({ p2 }),
        addrsToArray({ p3 }));
}

template<> TSetIterator3<ScAddr, ScAddr, ScAddr>::TSetIterator3(
  ScMemoryContext const & context,
  ScAddr const & p1, ScAddr const & p2, ScAddr const & p3)
{
  m_iterator = sc_set_iterator3_f_f_f_new(
        *context,
        addrsToArray({ p1 }),
        addrsToArray({ p2 }),
        addrsToArray({ p3 }));
}

//template<> TSetIterator5<ScAddr, ScType, ScType, ScType, ScAddr>::TSetIterator5(
//    ScMemoryContext const & context,
//    ScAddr const & p1,
//    sc_type const & p2,
//    sc_type const & p3,
//    sc_type const & p4,
//    ScAddr const & p5)
//{
//  m_iterator = sc_iterator5_f_a_a_a_f_new(*context, *p1, p2, p3, p4, *p5);
//}
//
//template<> TSetIterator5<ScType, ScType, ScAddr, ScType, ScAddr>::TSetIterator5(
//    ScMemoryContext const & context,
//    sc_type const & p1,
//    sc_type const & p2,
//    ScAddr const & p3,
//    sc_type const & p4,
//    ScAddr const & p5)
//{
//  m_iterator = sc_iterator5_a_a_f_a_f_new(*context, p1, p2, *p3, p4, *p5);
//}
//
//template<> TSetIterator5<ScAddr, ScType, ScAddr, ScType, ScAddr>::TSetIterator5(
//    ScMemoryContext const & context,
//    ScAddr const & p1,
//    sc_type const & p2,
//    ScAddr const & p3,
//    sc_type const & p4,
//    ScAddr const & p5)
//{
//  m_iterator = sc_iterator5_f_a_f_a_f_new(*context, *p1, p2, *p3, p4, *p5);
//}
//
//template<> TSetIterator5<ScAddr, ScType, ScAddr, ScType, ScType>::TSetIterator5(
//    ScMemoryContext const & context,
//    ScAddr const & p1,
//    sc_type const & p2,
//    ScAddr const & p3,
//    sc_type const & p4,
//    sc_type const & p5)
//{
//  m_iterator = sc_iterator5_f_a_f_a_a_new(*context, *p1, p2, *p3, p4, p5);
//}
//
//template<> TSetIterator5<ScAddr, ScType, ScType, ScType, ScType>::TSetIterator5(
//    ScMemoryContext const & context,
//    ScAddr const & p1,
//    sc_type const & p2,
//    sc_type const & p3,
//    sc_type const & p4,
//    sc_type const & p5)
//{
//  m_iterator = sc_iterator5_f_a_a_a_a_new(*context, *p1, p2, p3, p4, p5);
//}
//
//template<> TSetIterator5<ScType, ScType, ScAddr, ScType, ScType>::TSetIterator5(
//    ScMemoryContext const & context,
//    sc_type const & p1,
//    sc_type const & p2,
//    ScAddr const & p3,
//    sc_type const & p4,
//    sc_type const & p5)
//{
//  m_iterator = sc_iterator5_a_a_f_a_a_new(*context, p1, p2, *p3, p4, p5);
//}

sc_types typesToArray(ScTypeVector const & typesVector)
{
  sc_types typesArray;
  typesArray.size = typesVector.size();
  typesArray.types = g_new0(sc_type, typesVector.size());

  for (size_t i = 0; i < typesVector.size(); i++)
    typesArray.types[i] = typesVector.at(i);

  return typesArray;
}

sc_addrs addrsToArray(ScAddrVector const & addrsVector)
{
  sc_addrs addrsArray;
  addrsArray.size = addrsVector.size();
  addrsArray.addrs = g_new0(sc_addr, addrsVector.size());

  for (size_t i = 0; i < addrsVector.size(); i++)
    addrsArray.addrs[i] = *addrsVector.at(i);

  return addrsArray;
}
