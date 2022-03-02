/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#pragma once

class TestTypesSetIterator : public TestMemory
{
public:
  void Run()
  {
    ScTypeVector param2 = { ScType::EdgeDCommonConst };
    ScTypeVector param3 = { ScType::NodeConst, ScType::LinkConst };
    ScSetIterator3Ptr setIter3 = m_ctx->SetIterator3(node, param2, param3);
    while (setIter3->Next());
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
