/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#pragma once

class TestTypesIterator : public TestMemory
{
public:
  void Run()
  {
    ScIterator3Ptr iter3 = m_ctx->Iterator3(
          node,
          ScType::EdgeDCommonConst,
          ScType::NodeConst);
    while (iter3->Next());

    iter3 = m_ctx->Iterator3(
          node,
          ScType::EdgeDCommonConst,
          ScType::LinkConst);
    while (iter3->Next());
  }

  void Setup(size_t constrCount) override
  {
    node = m_ctx->CreateNode(ScType::NodeConst);
    for (uint32_t i = 0; i < constrCount; ++i)
    {
      ScAddr const trg1 = m_ctx->CreateNode(ScType::NodeConst);
      ScAddr const trg2 = m_ctx->CreateLink();
      m_ctx->CreateEdge(ScType::EdgeDCommonConst, node, trg1);
      ScAddr edge = m_ctx->CreateEdge(ScType::EdgeDCommonConst, node, trg2);
    }
  }

private:
  ScAddr node;
};
