/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_template.hpp"

#include "sc_debug.hpp"
#include "sc_memory.hpp"

#include <algorithm>
#include <stack>

class ScTemplateSearch
{
public:
  friend class ScTemplateSearchResult;

  ScTemplateSearch(ScTemplate const & templ, ScMemoryContext & context, ScAddr const & scStruct)
    : m_template(templ)
    , m_context(context)
    , m_struct(scStruct)
  {
    UpdateSearchCache();
  }

  void UpdateSearchCache()
  {
    if (m_template.m_isForceOrder)
    {
      m_template.m_searchCachedOrder.resize(m_template.m_constructions.size());
      for (size_t i = 0; i < m_template.m_constructions.size(); ++i)
        m_template.m_searchCachedOrder[i] = i;
    }
    else if (!m_template.IsSearchCacheValid() && !m_template.m_constructions.empty())
    {
      // update it
      ScTemplate::ProcessOrder preCache(m_template.m_constructions.size());
      for (size_t i = 0; i < preCache.size(); ++i)
        preCache[i] = i;

      static const size_t kScoreEdge = 5;
      static const size_t kScoreOther = 2;
      static const size_t kScorePoweredElement = 1;

      auto const CalculateScore = [this](ScTemplateConstr3 const & constr) {
        uint8_t score = 0;
        auto const & values = constr.GetValues();
        if (values[1].IsAddr() && values[0].IsAssign() && values[2].IsAssign())
          score += kScoreEdge;
        else if (values[0].IsAddr() && values[1].IsAssign() && values[2].IsAddr())
          score += kScoreOther * 2;  // should be a sum of (f_a_a and a_a_f)
        else if (values[0].IsAddr() || values[2].IsAddr())
          score += kScoreOther;
        else
        {
          for (auto const & other : m_template.m_constructions)
          {
            auto const & otherValues = other.GetValues();
            if (values[1].m_replacementName == otherValues[2].m_replacementName ||
                values[2].m_replacementName == otherValues[0].m_replacementName)
            {
              score += kScorePoweredElement;
              break;
            }
          }
        }

        return score;
      };

      /** First of all we need to calculate scores for all triples
       * (more scores - should be search first).
       * Also need to store all replacements that need to be resolved
       */
      std::vector<uint8_t> tripleScores(m_template.m_constructions.size());
      std::unordered_map<std::string, std::vector<size_t>> replDependMap;
      for (size_t i = 0; i < m_template.m_constructions.size(); ++i)
      {
        ScTemplateConstr3 const & triple = m_template.m_constructions[i];
        tripleScores[i] = CalculateScore(triple);
        // doesn't add edges into depend map
        auto const TryAppendRepl = [&](ScTemplateItemValue const & value, size_t idx) {
          SC_ASSERT(idx < 3, ());
          if (!value.IsAddr() && !value.m_replacementName.empty())
            replDependMap[value.m_replacementName].push_back((i << 2) + idx);
        };

        // do not use loop, to make it faster (not all compilers will make a linear code)
        TryAppendRepl(triple.GetValues()[0], 0);
        TryAppendRepl(triple.GetValues()[1], 1);
        TryAppendRepl(triple.GetValues()[2], 2);
      }

      // sort by scores
      std::sort(preCache.begin(), preCache.end(), [&](size_t a, size_t b) {
        return (tripleScores[a] > tripleScores[b]);
      });

      // now we need to append triples, in order, when previous resolve replacement for a next one
      ScTemplate::ProcessOrder & cache = m_template.m_searchCachedOrder;
      cache.resize(preCache.size());

      std::vector<bool> isTripleCached(cache.size(), false);
      std::unordered_set<std::string> usedReplacements;

      size_t preCacheIdx = 1;
      size_t orderIdx = 1;
      cache[0] = preCache[0];
      isTripleCached[cache[0]] = true;
      while (orderIdx < cache.size())
      {
        size_t const curTripleIdx = cache[orderIdx - 1];
        auto const & triple = m_template.m_constructions[curTripleIdx];

        // get resolved replacements by the last triple
        std::vector<std::string> resolvedReplacements;
        resolvedReplacements.reserve(3);
        for (auto const & value : triple.GetValues())
        {
          if (!value.m_replacementName.empty())
            resolvedReplacements.emplace_back(value.m_replacementName);
        }

        // collect triples, that depend on resolved replacements and update their scores
        std::unordered_set<size_t> resolvedTriples;
        for (auto const & name : resolvedReplacements)
        {
          if (usedReplacements.find(name) != usedReplacements.end())
            continue;

          auto const it = replDependMap.find(name);
          if (it != replDependMap.end())
            resolvedTriples.insert(it->second.begin(), it->second.end());

          usedReplacements.insert(name);
        }

        // find next non used triple
        while (isTripleCached[preCache[preCacheIdx]])
          preCacheIdx++;

        size_t bestTripleIdx = preCache[preCacheIdx];
        // now update scores of resolved triples and find best one scores
        for (size_t idx : resolvedTriples)
        {
          // unpack it
          size_t const tripleIdx = idx >> 2;

          if (isTripleCached[tripleIdx])
            continue;

          // for a constants see initial calculation
          if (idx == 1)
            tripleScores[tripleIdx] += kScoreEdge;
          else
            tripleScores[tripleIdx] += kScoreOther;

          // check if it better
          if (tripleScores[bestTripleIdx] < tripleScores[tripleIdx])
            bestTripleIdx = tripleIdx;
        }

        // append new best triple
        cache[orderIdx++] = bestTripleIdx;
        isTripleCached[bestTripleIdx] = true;
      }

      m_template.m_isSearchCacheValid = true;
    }
  }

  ScTemplate::Result operator()(ScTemplateSearchResult & result)
  {
    result.Clear();

    result.m_context = &m_context;
    result.m_template = &m_template;

    return ScTemplate::Result(result.Size() > 0);
  }

  size_t CalculateOneResultSize() const
  {
    return m_template.m_constructions.size() * 3;
  }

private:
  ScTemplate const & m_template;
  ScMemoryContext & m_context;
  ScAddr const m_struct;

  using StructCache = std::unordered_set<ScAddr, ScAddrHashFunc<uint32_t>>;
  StructCache m_structCache;
};

ScTemplate::Result ScTemplate::Search(ScMemoryContext & ctx, ScTemplateSearchResult & result) const
{
  ScTemplateSearch search(*this, ctx, ScAddr());
  return search(result);
}

ScTemplate::Result ScTemplate::SearchInStruct(
    ScMemoryContext & ctx,
    ScAddr const & scStruct,
    ScTemplateSearchResult & result) const
{
  ScTemplateSearch search(*this, ctx, scStruct);
  return search(result);
}
