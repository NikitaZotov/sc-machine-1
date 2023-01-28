/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_template.hpp"

#include "sc_debug.hpp"
#include "sc_memory.hpp"

#include <algorithm>
#include <utility>
#include <future>

namespace std
{

template <class Type, class HashFunc = std::hash<Type>, class Compare = std::equal_to<Type>>
class concurrent_unordered_set
{
private:
  std::unordered_set<Type, HashFunc, Compare> m_set;
  std::mutex m_mutex;

public:
  typedef typename std::unordered_set<Type, HashFunc, Compare>::iterator iterator;

  concurrent_unordered_set() = default;

  concurrent_unordered_set(concurrent_unordered_set<Type, HashFunc, Compare> const & otherSet)
    : m_set(otherSet.m_set)
  {
    m_set.reserve(16);
  }

  concurrent_unordered_set<Type, HashFunc, Compare> & operator=(concurrent_unordered_set<Type, HashFunc, Compare> const & otherSet)
  {
    m_set = otherSet.m_set;
    return *this;
  }

  std::pair<iterator, bool> insert(Type const & val)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_set.insert(val);
  }

  auto find(Type const & object)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_set.find(object);
  }

  bool contains(Type const & object)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_set.find(object) != m_set.cend();
  }

  void erase(Type const & object)
  {
    m_set.erase(object);
  }

  size_t size()
  {
    return m_set.size();
  }

  auto cend()
  {
    return m_set.cend();
  }
};

}

class ScTemplateSearch
{
public:
  ScTemplateSearch(ScTemplate & templ, ScMemoryContext & context, ScAddr const & structure)
    : m_template(templ)
    , m_context(context)
    , m_structure(structure)
  {
    PrepareSearch();
  }

  ScTemplateSearch(
      ScTemplate & templ,
      ScMemoryContext & context,
      ScAddr const & structure,
      ScTemplateSearchResultCallback callback,
      ScTemplateSearchResultCheckCallback checkCallback = {})
    : m_template(templ)
    , m_context(context)
    , m_structure(structure)
    , m_callback(std::move(callback))
    , m_checkCallback(std::move(checkCallback))
  {
    PrepareSearch();
  }

  ScTemplateSearch(
      ScTemplate & templ,
      ScMemoryContext & context,
      ScAddr const & structure,
      ScTemplateSearchResultCallbackWithRequest callback,
      ScTemplateSearchResultCheckCallback checkCallback = {})
    : m_template(templ)
    , m_context(context)
    , m_structure(structure)
    , m_callbackWithRequest(std::move(callback))
    , m_checkCallback(std::move(checkCallback))
  {
    PrepareSearch();
  }

  using ScTemplateGroupedTriples = ScTemplate::ScTemplateGroupedTriples;

private:
  void PrepareSearch()
  {
    auto const & SortTriplesWithConstBeginElement = [this]() {
      auto triplesWithConstBeginElement = m_template.m_orderedTriples[(size_t)ScTemplateTripleType::FAE];
      if (triplesWithConstBeginElement.empty())
      {
        triplesWithConstBeginElement = m_template.m_orderedTriples[(size_t)ScTemplateTripleType::FAN];
      }

      sc_int32 priorityTripleIdx = -1;
      sc_int32 minOutputArcsCount = -1;
      for (size_t const tripleIdx : triplesWithConstBeginElement)
      {
        ScTemplateTriple const * triple = m_template.m_triples[tripleIdx];
        auto const count = (sc_int32)m_context.GetElementOutputArcsCount(triple->GetValues()[0].m_addrValue);

        if (minOutputArcsCount == -1 || count < minOutputArcsCount)
        {
          priorityTripleIdx = (sc_int32)tripleIdx;
          minOutputArcsCount = (sc_int32)count;
        }
      }

      if (priorityTripleIdx != -1)
      {
        m_template.m_orderedTriples[(size_t)ScTemplateTripleType::PFAE].insert(priorityTripleIdx);
      }
    };

    auto const & SaveDependenciesBetweenTriples = [this]() {
      for (ScTemplateTriple const * triple : m_template.m_triples)
      {
        ScTemplateItemValue const & item1 = (*triple)[0];
        ScTemplateItemValue const & item2 = (*triple)[1];
        ScTemplateItemValue const & item3 = (*triple)[2];

        for (ScTemplateTriple * otherTriple : m_template.m_triples)
        {
          ScTemplateItemValue const & otherItem1 = (*otherTriple)[0];
          ScTemplateItemValue const & otherItem2 = (*otherTriple)[1];
          ScTemplateItemValue const & otherItem3 = (*otherTriple)[2];

          if (triple->m_index == otherTriple->m_index)
            continue;

          auto const & TryAddDependence = [this, &triple, &otherTriple](
                                              ScTemplateItemValue const & item,
                                              ScTemplateItemValue const & otherItem1,
                                              ScTemplateItemValue const & otherItem2,
                                              ScTemplateItemValue const & otherItem3) {
            if (item.m_replacementName.empty())
              return;

            sc_bool const withItem1Equal = item.m_replacementName == otherItem1.m_replacementName;
            sc_bool const withItem2Equal = item.m_replacementName == otherItem2.m_replacementName;
            sc_bool const withItem3Equal = item.m_replacementName == otherItem3.m_replacementName;

            if (withItem1Equal || withItem2Equal || withItem3Equal)
            {
              std::string const & key = GetKey(triple, item);
              auto const & found = m_itemsToTriples.find(key);
              if (found == m_itemsToTriples.cend())
                m_itemsToTriples.insert({key, {otherTriple->m_index}});
              else
                found->second.insert(otherTriple->m_index);
            }
          };

          TryAddDependence(item1, otherItem1, otherItem2, otherItem3);
          TryAddDependence(item2, otherItem1, otherItem2, otherItem3);
          TryAddDependence(item3, otherItem1, otherItem2, otherItem3);
        }
      }
    };

    auto const & RemoveCycledDependenciesBetweenTriples = [this]() {
      auto const & faeTriples =  m_template.m_orderedTriples[(size_t)ScTemplateTripleType::FAN];

      for (ScTemplateTriple const * triple : m_template.m_triples)
      {
        auto const & item1 = (*triple)[0];

        bool isFound = false;
        if (item1.IsAddr() && faeTriples.find(triple->m_index) != faeTriples.cend())
        {
          ScTemplateGroupedTriples checkedTriples;
          FindCycleWithFAATriple(item1, triple, triple, checkedTriples, isFound);
        }

        if (isFound)
        {
          std::string const & key = GetKey(triple, item1);
          m_cycledTriples.insert(triple->m_index);
        }
      }

      for (size_t const idx : m_cycledTriples)
      {
        ScTemplateTriple * triple = m_template.m_triples[idx];
        std::string const & key = GetKey(triple, (*triple)[0]);

        auto const & found = m_itemsToTriples.find(key);
        if (found != m_itemsToTriples.cend())
        {
          for (size_t const otherIdx : m_cycledTriples)
          {
            found->second.erase(otherIdx);
          }
        }
      }
    };

    {
      SortTriplesWithConstBeginElement();
      SaveDependenciesBetweenTriples();
      RemoveCycledDependenciesBetweenTriples();
    }
  }

  static std::string GetKey(ScTemplateTriple const * triple, ScTemplateItemValue const & item)
  {
    std::ostringstream stream;
    stream << item.m_replacementName << triple->m_index;

    return stream.str();
  }

  void FindCycleWithFAATriple(
      ScTemplateItemValue const & item,
      ScTemplateTriple const * triple,
      ScTemplateTriple const * tripleToFind,
      ScTemplateGroupedTriples & checkedTriples,
      bool & isFound)
  {
    if (isFound)
    {
      return;
    }

    auto const & FindCycleWithFAATripleByTripleItem = [this, &tripleToFind, &checkedTriples, &isFound](
                                                          ScTemplateItemValue const & item,
                                                          ScTemplateTriple const * triple,
                                                          ScTemplateItemValue const & previousItem) {
      if (!item.m_replacementName.empty() && item.m_replacementName == previousItem.m_replacementName)
      {
        return;
      }

      if (item.m_addrValue.IsValid() && item.m_addrValue == previousItem.m_addrValue)
      {
        return;
      }

      FindCycleWithFAATriple(item, triple, tripleToFind, checkedTriples, isFound);
    };

    ScTemplateGroupedTriples nextTriples;
    FindDependedTriple(item, triple, nextTriples);

    for (size_t const otherTripleIdx : nextTriples)
    {
      if ((otherTripleIdx == tripleToFind->m_index && item.m_replacementName != (*tripleToFind)[0].m_replacementName) ||
          isFound)
      {
        isFound = true;
        break;
      }

      if (checkedTriples.find(otherTripleIdx) != checkedTriples.cend())
      {
        continue;
      }

      {
        ScTemplateTriple const * otherTriple = m_template.m_triples[otherTripleIdx];
        auto const & items = otherTriple->GetValues();
        checkedTriples.insert(otherTripleIdx);

        FindCycleWithFAATripleByTripleItem(items[0], otherTriple, item);
        FindCycleWithFAATripleByTripleItem(items[1], otherTriple, item);
        FindCycleWithFAATripleByTripleItem(items[2], otherTriple, item);
      }
    }
  }

  inline bool IsStructureValid()
  {
    return m_structure.IsValid();
  }

  inline bool IsInStructure(ScAddr const & addr)
  {
    return m_context.HelperCheckEdge(m_structure, addr, ScType::EdgeAccessConstPosPerm);
  }

  ScAddr const & ResolveAddr(
      ScTemplateItemValue const & item,
      ScAddrVector const & resultAddrs,
      ScTemplateSearchResult & result) const
  {
    switch (item.m_itemType)
    {
    case ScTemplateItemValue::Type::Addr:
    {
      return item.m_addrValue;
    }

    case ScTemplateItemValue::Type::Replace:
    {
      auto const & it = result.m_replacements.equal_range(item.m_replacementName);
      for (auto curIt = it.first; curIt != it.second; ++curIt)
      {
        ScAddr const & addr = resultAddrs[curIt->second];
        if (addr.IsValid())
          return addr;
      }

      auto const & addrsIt = m_template.m_namesToAddrs.find(item.m_replacementName);
      if (addrsIt != m_template.m_namesToAddrs.cend())
      {
        return addrsIt->second;
      }

      return ScAddr::Empty;
    }

    case ScTemplateItemValue::Type::Type:
    {
      if (!item.m_replacementName.empty())
      {
        auto const & it = result.m_replacements.equal_range(item.m_replacementName);
        for (auto curIt = it.first; curIt != it.second; ++curIt)
        {
          ScAddr const & addr = resultAddrs[curIt->second];
          if (addr.IsValid())
            return addr;
        }

        return ScAddr::Empty;
      }
    }

    default:
    {
      return ScAddr::Empty;
    }
    }
  }

  ScIterator3Ptr CreateIterator(
      ScTemplateItemValue const & item1,
      ScTemplateItemValue const & item2,
      ScTemplateItemValue const & item3,
      ScAddrVector const & resultAddrs,
      ScTemplateSearchResult & result)
  {
    ScAddr const addr1 = ResolveAddr(item1, resultAddrs, result);
    ScAddr const addr2 = ResolveAddr(item2, resultAddrs, result);
    ScAddr const addr3 = ResolveAddr(item3, resultAddrs, result);

    auto const PrepareType = [](ScType const & type) {
      if (type.HasConstancyFlag())
        return type.UpConstType();

      return type;
    };

    if (addr1.IsValid())
    {
      if (!addr2.IsValid())
      {
        if (addr3.IsValid())  // F_A_F
        {
          return m_context.Iterator3(addr1, PrepareType(item2.m_typeValue), addr3);
        }
        else  // F_A_A
        {
          return m_context.Iterator3(addr1, PrepareType(item2.m_typeValue), PrepareType(item3.m_typeValue));
        }
      }
      else
      {
        if (addr3.IsValid())  // F_F_F
        {
          return m_context.Iterator3(addr1, addr2, addr3);
        }
        else  // F_F_A
        {
          return m_context.Iterator3(addr1, addr2, PrepareType(item3.m_typeValue));
        }
      }
    }
    else if (addr3.IsValid())
    {
      if (addr2.IsValid())  // A_F_F
      {
        return m_context.Iterator3(PrepareType(item1.m_typeValue), addr2, addr3);
      }
      else  // A_A_F
      {
        return m_context.Iterator3(PrepareType(item1.m_typeValue), PrepareType(item2.m_typeValue), addr3);
      }
    }
    else if (addr2.IsValid() && !addr3.IsValid())  // A_F_A
    {
      return m_context.Iterator3(PrepareType(item1.m_typeValue), addr2, PrepareType(item3.m_typeValue));
    }

    return {};
  }

  void FindDependedTriple(
      ScTemplateItemValue const & item,
      ScTemplateTriple const * triple,
      ScTemplateGroupedTriples & nextTriples)
  {
    if (item.m_replacementName.empty())
      return;

    std::string const & key = GetKey(triple, item);
    auto const & found = m_itemsToTriples.find(key);
    if (found != m_itemsToTriples.cend())
    {
      nextTriples = found->second;
    }
  }

  bool IsTriplesEqual(
      ScTemplateTriple const * triple,
      ScTemplateTriple const * otherTriple,
      std::string const & itemName = "")
  {
    if (triple->m_index == otherTriple->m_index)
    {
      return true;
    }

    auto const & tripleValues = triple->GetValues();
    auto const & otherTripleValues = otherTriple->GetValues();

    auto const & IsTriplesItemsEqual =
        [this](ScTemplateItemValue const & item, ScTemplateItemValue const & otherItem) -> bool {
      bool isEqual = item.m_typeValue == otherItem.m_typeValue;
      if (!isEqual)
      {
        auto found = m_template.m_namesToTypes.find(item.m_replacementName);
        if (found == m_template.m_namesToTypes.cend())
        {
          found = m_template.m_namesToTypes.find(item.m_replacementName);
          if (found != m_template.m_namesToTypes.cend())
          {
            isEqual = item.m_typeValue == found->second;
          }
        }
        else
        {
          isEqual = found->second == otherItem.m_typeValue;
        }
      }

      if (isEqual)
      {
        isEqual = item.m_addrValue == otherItem.m_addrValue;
      }

      if (!isEqual)
      {
        auto found = m_template.m_namesToAddrs.find(item.m_replacementName);
        if (found == m_template.m_namesToAddrs.cend())
        {
          found = m_template.m_namesToAddrs.find(item.m_replacementName);
          if (found != m_template.m_namesToAddrs.cend())
          {
            isEqual = item.m_addrValue == found->second;
          }
        }
        else
        {
          isEqual = found->second == otherItem.m_addrValue;
        }
      }

      return isEqual;
    };

    return IsTriplesItemsEqual(tripleValues[0], otherTripleValues[0]) &&
           IsTriplesItemsEqual(tripleValues[1], otherTripleValues[1]) &&
           IsTriplesItemsEqual(tripleValues[2], otherTripleValues[2]) &&
           ((tripleValues[0].m_replacementName == otherTripleValues[0].m_replacementName &&
             (itemName.empty() || otherTripleValues[0].m_replacementName == itemName)) ||
            (tripleValues[2].m_replacementName == otherTripleValues[2].m_replacementName &&
             (itemName.empty() || otherTripleValues[0].m_replacementName == itemName)));
  };

  using UsedEdges = std::concurrent_unordered_set<ScAddr, ScAddrHashFunc<uint32_t>>;
  using ScTriplesOrderCheckedEdges = std::vector<UsedEdges>;

  void DoIterationOnNextEqualTriples(
      ScTemplateGroupedTriples const & triples,
      std::string const & itemName,
      std::concurrent_unordered_set<size_t> & checkedTriples,
      ScTemplateGroupedTriples const & currentIterableTriples,
      ScAddrVector & resultAddrs,
      ScTemplateSearchResult & result,
      bool & isFinished,
      bool & isLast)
  {
    isLast = true;
    isFinished = true;

    std::unordered_set<size_t> iteratedTriples;
    for (size_t const idx : triples)
    {
      ScTemplateTriple * triple = m_template.m_triples[idx];
      if (iteratedTriples.find(triple->m_index) != iteratedTriples.cend())
        continue;

      ScTemplateGroupedTriples equalTriples;
      for (ScTemplateTriple * otherTriple : m_template.m_triples)
      {
        // check if iterable triple is equal to current, not checked and not iterable with previous
        if (!checkedTriples.contains(otherTriple->m_index) &&
            std::find_if(
                currentIterableTriples.cbegin(),
                currentIterableTriples.cend(),
                [&](size_t const otherIdx) {
                  return idx == otherIdx;
                }) == currentIterableTriples.cend() &&
            IsTriplesEqual(triple, otherTriple, itemName))
        {
          equalTriples.insert(otherTriple->m_index);
          iteratedTriples.insert(otherTriple->m_index);
        }
      }

      if (!equalTriples.empty())
      {
        isLast = false;
        DoDependenceIteration(equalTriples, checkedTriples, resultAddrs, result, isFinished);
      }
    }
  }

  void DoDependenceIteration(
      ScTemplateGroupedTriples const & triples,
      std::concurrent_unordered_set<size_t> & checkedTriples,
      ScAddrVector & resultAddrs,
      ScTemplateSearchResult & result,
      bool & isFinished)
  {
    auto const & TryDoNextDependenceIteration = [this, &result](
                                                    ScTemplateTriple const * triple,
                                                    ScTemplateItemValue const & item,
                                                    ScAddr const & itemAddr,
                                                    std::concurrent_unordered_set<size_t> & checkedTriples,
                                                    ScAddrVector & resultAddrs,
                                                    ScTemplateGroupedTriples const & currentIterableTriples,
                                                    bool & isFinished,
                                                    bool & isLast) {
      ScTemplateGroupedTriples nextTriples;
      FindDependedTriple(item, triple, nextTriples);

      DoIterationOnNextEqualTriples(
          nextTriples,
          item.m_replacementName,
          checkedTriples,
          currentIterableTriples,
          resultAddrs,
          result,
          isFinished,
          isLast);
    };

    auto const & UpdateItemResults = [&result](
                                         ScTemplateItemValue const & item,
                                         ScAddr const & addr,
                                         size_t const elementNum,
                                         ScAddrVector & resultAddrs) {
      resultAddrs[elementNum] = addr;

      if (item.m_replacementName.empty())
        return;

      result.m_replacements.insert({item.m_replacementName, elementNum});
    };

    auto const & UpdateResults = [&UpdateItemResults](
                                     ScTemplateTriple * triple,
                                     ScAddr const & addr1,
                                     ScAddr const & addr2,
                                     ScAddr const & addr3,
                                     ScAddrVector & resultAddrs) {
      size_t itemIdx = triple->m_index * 3;

      UpdateItemResults((*triple)[0], addr1, itemIdx, resultAddrs);
      UpdateItemResults((*triple)[1], addr2, ++itemIdx, resultAddrs);
      UpdateItemResults((*triple)[2], addr3, ++itemIdx, resultAddrs);
    };

    auto const & ClearResults =
        [this](size_t tripleIdx, ScAddrVector & resultAddrs, std::concurrent_unordered_set<size_t> & checkedTriples) {
          checkedTriples.erase(tripleIdx);

          tripleIdx *= 3;
          resultAddrs[tripleIdx] = ScAddr::Empty;
          m_usedEdges.erase(resultAddrs[++tripleIdx]);
          resultAddrs[tripleIdx] = ScAddr::Empty;
          resultAddrs[++tripleIdx] = ScAddr::Empty;
        };

    ScTemplateTriple * triple = m_template.m_triples[*triples.begin()];

    bool isAllChildrenFinished = false;
    bool isLast = false;

    ScTemplateItemValue item1 = (*triple)[0];
    ScTemplateItemValue item2 = (*triple)[1];
    ScTemplateItemValue item3 = (*triple)[2];

//    std::cout << "try " << triple->m_index << " = {" << item1.m_replacementName << "} ---{" <<
//        item2.m_replacementName << "}---> {" << item3.m_replacementName << "}" << std::endl;

    ScIterator3Ptr const it = CreateIterator(item1, item2, item3, resultAddrs, result);
    if (!it || !it->IsValid())
    {
      SC_THROW_EXCEPTION(utils::ExceptionInvalidState, "During search procedure has been chosen var triple");
    }

    size_t count = 0;

    std::concurrent_unordered_set<size_t> const currentCheckedTriples{checkedTriples};
    ScAddrVector const currentResultAddrs{resultAddrs};
    while (it->Next() && !isStopped)
    {
      ScAddr const & addr2 = it->Get(1);

      if (m_usedEdges.contains(addr2))
      {
        continue;
      }

      ScAddr const & addr1 = it->Get(0);
      ScAddr const & addr3 = it->Get(2);

      // check triple elements by structure belonging or predicate callback
      if ((IsStructureValid() && (!IsInStructure(addr1) || !IsInStructure(addr2) || !IsInStructure(addr3))) ||
          (m_checkCallback && !m_checkCallback(addr1, addr2, addr3)))
      {
        for (size_t const tripleIdx : triples)
        {
          m_triplesOrderUsedEdges[tripleIdx].insert(addr2);
        }
        continue;
      }

      for (size_t const tripleIdx : triples)
      {
        triple = m_template.m_triples[tripleIdx];

        // check if edge is used for other equal triple
        if (m_triplesOrderUsedEdges[tripleIdx].contains(addr2))
        {
          continue;
        }

        // check if all equal triples found to make a new search result item
        if (count == triples.size())
        {
          size_t const triplesCount = m_template.m_triples.size();
          if (isAllChildrenFinished && checkedTriples.size() != triplesCount)
          {
            size_t const prevResultCount = m_resultCount;
            while (checkedTriples.size() != triplesCount && !m_finishedResult.contains(prevResultCount))
            {
              std::cout << "wait" << std::endl;
            }
          }

          std::cout << "next" << std::endl;

          count = 0;
          m_finishedResult.insert(m_resultCount);
          ++m_resultCount;
          checkedTriples = std::concurrent_unordered_set<size_t>{currentCheckedTriples};
          resultAddrs.assign(currentResultAddrs.cbegin(), currentResultAddrs.cend());
        }

        if (checkedTriples.find(tripleIdx) != checkedTriples.cend())
        {
          continue;
        }

        item1 = (*triple)[0];
        item2 = (*triple)[1];
        item3 = (*triple)[2];

                std::cout << "iterate " << triple->m_index << " = {" << m_context.HelperGetSystemIdtf(addr1) << "} ---{" <<
                  item2.m_replacementName << "}---> {" << m_context.HelperGetSystemIdtf(addr3)  << "}" << std::endl;

        // update data
        {
          UpdateResults(triple, addr1, addr2, addr3, resultAddrs);
          checkedTriples.insert(tripleIdx);
          m_triplesOrderUsedEdges[tripleIdx].insert(addr2);
          m_usedEdges.insert(addr2);
          ++count;
        }

        // find next depended on triples and analyse result
        {
          bool isChildFinished = false;
          bool isNoChild = false;

          // first of all check triples by edge, it is more effectively
          auto branchFuture2 = std::async(std::launch::async, [&]() {
            TryDoNextDependenceIteration(
                triple, item2, addr2, checkedTriples, resultAddrs, triples, isChildFinished, isNoChild);
            isAllChildrenFinished = isChildFinished;
            isLast = isNoChild;

            if (!isChildFinished && !isLast)
            {
              ClearResults(tripleIdx, resultAddrs, checkedTriples);
            }
          });

          auto branchFuture1 = std::async(std::launch::async, [&]() {
            TryDoNextDependenceIteration(
                triple, item1, addr1, checkedTriples, resultAddrs, triples, isChildFinished, isNoChild);
            isAllChildrenFinished = isChildFinished;
            isLast = isNoChild;

            if (!isChildFinished && !isLast)
            {
              ClearResults(tripleIdx, resultAddrs, checkedTriples);
            }
          });

          auto branchFuture3 = std::async(std::launch::async, [&]() {
            TryDoNextDependenceIteration(
                triple, item3, addr3, checkedTriples, resultAddrs, triples, isChildFinished, isNoChild);
            isAllChildrenFinished = isChildFinished;
            isLast = isNoChild;

            if (!isChildFinished && !isLast)
            {
              ClearResults(tripleIdx, resultAddrs, checkedTriples);
            }
          });

          auto future = std::async(std::launch::async, [&]() {
            branchFuture2.get();
            branchFuture1.get();
            branchFuture3.get();

            // all connected triples found
            if (isAllChildrenFinished)
            {
              std::cout << "succeed " << triple->m_index << " = {" << item1.m_replacementName << "}---{"
                        << item2.m_replacementName << "}---> {" << item3.m_replacementName << "}" << std::endl;

              // current edge is busy for all equal triples
              for (size_t const idx : triples)
              {
                m_triplesOrderUsedEdges[idx].insert(addr2);
              }

              std::cout << "Found " << checkedTriples.size() << " to achieve " << m_template.m_triples.size() << std::endl;
              // there are no next triples for current triple, it is last
              if (isLast && checkedTriples.size() == m_template.m_triples.size() &&
                  !m_finishedResult.contains(m_resultCount))
              {
                m_finishedResult.insert(m_resultCount);

                std::cout << "Great " << checkedTriples.size() << " to achieve " << m_template.m_triples.size() << std::endl;

                FormResult(result, resultAddrs);
              }
            }
          });
        }
      }
    }

    isFinished = isAllChildrenFinished;
  }

  void FormResult(ScTemplateSearchResult & result, ScAddrVector const & resultAddrs)
  {
    if (m_callback)
    {
      m_callback(ScTemplateSearchResultItem(&resultAddrs, &result.m_replacements));
    }
    else if (m_callbackWithRequest)
    {
      ScTemplateSearchRequest const & request =
          m_callbackWithRequest(ScTemplateSearchResultItem(&resultAddrs, &result.m_replacements));
      switch (request)
      {
      case ScTemplateSearchRequest::STOP:
      {
        isStopped = true;
        break;
      }
      case ScTemplateSearchRequest::ERROR:
      {
        SC_THROW_EXCEPTION(utils::ExceptionInvalidState, "Requested error state during search");
      }
      default:
        break;
      }
    }
    else
    {
      result.m_results.emplace_back(resultAddrs);
    }
  }

  void DoIterations(ScTemplateSearchResult & result)
  {
    if (m_template.m_triples.empty())
      return;

    auto const & DoStartIteration = [this, &result](ScTemplateGroupedTriples const & triples) {
      std::vector<ScAddr> resultAddrs;
      resultAddrs.resize(CalculateOneResultSize());
      std::concurrent_unordered_set<size_t> checkedTriples;

      bool isFinished = false;
      bool isLast = false;

      DoIterationOnNextEqualTriples(triples, "", checkedTriples, {}, resultAddrs, result, isFinished, isLast);
    };

    auto const & triples = m_template.m_orderedTriples;
    for (ScTemplateGroupedTriples const & equalTriples : triples)
    {
      if (!equalTriples.empty())
      {
        DoStartIteration(equalTriples);

        // TODO: Provide logic for template with more connectivity components than 1 in sc-template
        break;
      }
    }
  }

public:
  ScTemplate::Result operator()(ScTemplateSearchResult & result)
  {
    result.Clear();
    m_triplesOrderUsedEdges.resize(CalculateOneResultSize());

    DoIterations(result);

    return ScTemplate::Result(result.Size() > 0);
  }

  void operator()()
  {
    ScTemplateSearchResult result;
    m_triplesOrderUsedEdges.resize(CalculateOneResultSize());

    DoIterations(result);
  }

  size_t CalculateOneResultSize() const
  {
    return m_template.m_triples.size() * 3;
  }

private:
  ScTemplate & m_template;
  ScMemoryContext & m_context;

  bool isStopped = false;
  ScAddr const m_structure;
  ScTemplateSearchResultCallback m_callback;
  ScTemplateSearchResultCallbackWithRequest m_callbackWithRequest;
  ScTemplateSearchResultCheckCallback m_checkCallback;

  std::map<std::string, ScTemplateGroupedTriples> m_itemsToTriples;
  ScTemplateGroupedTriples m_cycledTriples;
  UsedEdges m_usedEdges;
  ScTriplesOrderCheckedEdges m_triplesOrderUsedEdges;

  std::atomic<size_t> m_resultCount = { 0 };
  std::concurrent_unordered_set<size_t> m_finishedResult;
};

ScTemplate::Result ScTemplate::Search(ScMemoryContext & ctx, ScTemplateSearchResult & result) const
{
  ScTemplateSearch search(const_cast<ScTemplate &>(*this), ctx, ScAddr::Empty);
  return search(result);
}

void ScTemplate::Search(
    ScMemoryContext & ctx,
    ScTemplateSearchResultCallback const & callback,
    ScTemplateSearchResultCheckCallback const & checkCallback) const
{
  ScTemplateSearch search(const_cast<ScTemplate &>(*this), ctx, ScAddr::Empty, callback, checkCallback);
  search();
}

void ScTemplate::Search(
    ScMemoryContext & ctx,
    ScTemplateSearchResultCallbackWithRequest const & callback,
    ScTemplateSearchResultCheckCallback const & checkCallback) const
{
  ScTemplateSearch search(const_cast<ScTemplate &>(*this), ctx, ScAddr::Empty, callback, checkCallback);
  search();
}

ScTemplate::Result ScTemplate::SearchInStruct(
    ScMemoryContext & ctx,
    ScAddr const & scStruct,
    ScTemplateSearchResult & result) const
{
  ScTemplateSearch search(const_cast<ScTemplate &>(*this), ctx, scStruct);
  return search(result);
}
