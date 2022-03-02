/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#pragma once

#include "sc_addr.hpp"
#include "sc_utils.hpp"
#include "sc_type.hpp"

extern "C"
{
#include "sc-core/sc_memory_headers.h"
}


class ScMemoryContext;

template <typename IterType>
class TSetIteratorBase
{
public:
  virtual ~TSetIteratorBase() = default;

  inline bool IsValid() const
  {
    return m_iterator != 0;
  }

  //! Returns false, if there are no more iterator results. It more results exists, then go to next one and returns true
  _SC_EXTERN virtual bool Next() const = 0;

  //! Returns sc-addr of specified element in iterator result
  _SC_EXTERN virtual ScAddr Get(sc_uint8 idx) const = 0;
  
  //! Short form of Get
  inline ScAddr operator [] (sc_uint8 idx) const { return Get(idx); }

protected:
  IterType * m_iterator;
};


template <typename ParamType1, typename ParamType2, typename ParamType3>
class TSetIterator3 : public TSetIteratorBase<sc_set_iterator3>
{
  friend class ScMemoryContext;

protected:
  _SC_EXTERN TSetIterator3(ScMemoryContext const & context, ParamType1 const & p1, ParamType2 const & p2, ParamType3 const & p3);

public:
  _SC_EXTERN ~TSetIterator3() override
  {
    Destroy();
  }

  TSetIterator3(TSetIterator3 const & other)
  {

  }

  TSetIterator3 & operator = (TSetIterator3 const & other)
  {
    TakeOwnership(other);
    return *this;
  }

  void Destroy()
  {
    if (m_iterator)
    {
      sc_set_iterator3_free(m_iterator);
      m_iterator = 0;
    }
  }

  _SC_EXTERN bool Next() const override
  {
    SC_ASSERT(IsValid(), ());
    return sc_set_iterator3_next(m_iterator) == SC_TRUE;
  }

  _SC_EXTERN ScAddr Get(sc_uint8 idx) const override
  {
    SC_ASSERT(idx < 3, ());
    SC_ASSERT(IsValid(), ());
    return ScAddr(sc_set_iterator3_value(m_iterator, idx));
  }
};

// ---------------------------
template <typename ParamType1, typename ParamType2, typename ParamType3, typename ParamType4, typename ParamType5>
class TSetIterator5 : public TSetIteratorBase<sc_iterator5>
{
  friend class ScMemoryContext;

protected:
  _SC_EXTERN TSetIterator5(ScMemoryContext const & context, ParamType1 const & p1, ParamType2 const & p2, ParamType3 const & p3, ParamType4 const & p4, ParamType5 const & p5);

public:
  _SC_EXTERN ~TSetIterator5() override
  {
    Destroy();
  }

  void Destroy()
  {
    if (m_iterator)
    {
      sc_iterator5_free(m_iterator);
      m_iterator = 0;
    }
  }

  _SC_EXTERN bool Next() const override
  {
    SC_ASSERT(IsValid(), ());
    return sc_iterator5_next(m_iterator) == SC_TRUE;
  }

  _SC_EXTERN ScAddr Get(sc_uint8 idx) const override
  {
    SC_ASSERT(idx < 5, ());
    SC_ASSERT(IsValid(), ());
    return ScAddr(sc_iterator5_value(m_iterator, idx));
  }

};

typedef TSetIteratorBase<sc_set_iterator3> ScSetIterator3Type;
typedef TSetIteratorBase<sc_iterator5> ScSetIterator5Type;

typedef std::shared_ptr< ScSetIterator3Type > ScSetIterator3Ptr;
typedef std::shared_ptr< ScSetIterator5Type > ScSetIterator5Ptr;

sc_types typesToArray(ScTypeVector const & typesVector);

sc_addrs addrsToArray(ScAddrVector const & addrsVector);