#include <gtest/gtest.h>

#include "sc_test.hpp"
#include "dummy_file_interface.hpp"

#include "sc-memory/sc_memory.hpp"
#include "sc-memory/sc_scs_helper.hpp"

#include "sc-agents-common/keynodes/coreKeynodes.hpp"
#include "sc-agents-common/utils/GenerationUtils.hpp"
#include "sc-agents-common/utils/AgentUtils.hpp"

#include "tests/test-agents/FinishActionTestAgent.hpp"

using namespace scUtilsTestAgents;

TEST_F(ScMemoryTest, wrapInOrientedSetBySequenceRelation)
{
  std::string const data =
      "concept_message"
      "  -> first_message;"
      "  -> second_message; "
      "  -> third_message;; ";

  SCsHelper helper(*m_ctx, std::make_shared<DummyFileInterface>());
  EXPECT_TRUE(helper.GenerateBySCsText(data));

  ScAddrVector suggestions;
  ScAddr const firstMessage = m_ctx->HelperFindBySystemIdtf("first_message");
  EXPECT_TRUE(firstMessage.IsValid());
  ScAddr const secondMessage = m_ctx->HelperFindBySystemIdtf("second_message");
  EXPECT_TRUE(secondMessage.IsValid());
  ScAddr const thirdMessage = m_ctx->HelperFindBySystemIdtf("third_message");
  EXPECT_TRUE(thirdMessage.IsValid());
  suggestions.insert(suggestions.end(), {firstMessage, secondMessage, thirdMessage});

  ScAddr orientedSet = utils::GenerationUtils::wrapInOrientedSetBySequenceRelation(&*m_ctx, suggestions);

  std::string const FIRST_MESSAGE_EDGE_ALIAS = "_first_message_edge";
  std::string const SECOND_MESSAGE_EDGE_ALIAS = "_second_message_edge";
  std::string const THIRD_MESSAGE_EDGE_ALIAS = "_third_message_edge";

  ScTemplate findTemplate;
  findTemplate.Quintuple(
      orientedSet,
      ScType::EdgeAccessVarPosPerm >> FIRST_MESSAGE_EDGE_ALIAS,
      firstMessage,
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::rrel_1);
  findTemplate.Quintuple(
      FIRST_MESSAGE_EDGE_ALIAS,
      ScType::EdgeDCommonVar,
      ScType::EdgeAccessVarPosPerm >> SECOND_MESSAGE_EDGE_ALIAS,
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::nrel_basic_sequence);
  findTemplate.Triple(orientedSet, SECOND_MESSAGE_EDGE_ALIAS, secondMessage);
  findTemplate.Quintuple(
      SECOND_MESSAGE_EDGE_ALIAS,
      ScType::EdgeDCommonVar,
      ScType::EdgeAccessVarPosPerm >> THIRD_MESSAGE_EDGE_ALIAS,
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::nrel_basic_sequence);
  findTemplate.Triple(orientedSet, THIRD_MESSAGE_EDGE_ALIAS, thirdMessage);

  ScTemplateSearchResult searchResult;
  m_ctx->HelperSearchTemplate(findTemplate, searchResult);
  EXPECT_TRUE(searchResult.Size() == 1);
}

TEST_F(ScMemoryTest, formActionNode)
{
  ScAddr actionClass = m_ctx->CreateNode(ScType::NodeConstClass);
  ScAddrVector params = {m_ctx->CreateNode(ScType::NodeConst), m_ctx->CreateNode(ScType::NodeConst)};

  ScAddr const & actionNode = utils::AgentUtils::formActionNode(m_ctx.get(), actionClass, params);
  EXPECT_TRUE(actionNode.IsValid());

  std::string const actionAlias = "_action";

  ScTemplate generatedActionTemplate;
  generatedActionTemplate.Triple(actionClass, ScType::EdgeAccessVarPosPerm, ScType::NodeVar >> actionAlias);
  generatedActionTemplate.Quintuple(
      actionAlias,
      ScType::EdgeAccessVarPosPerm,
      params[0],
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::rrel_1);
  generatedActionTemplate.Quintuple(
      actionAlias,
      ScType::EdgeAccessVarPosPerm,
      params[1],
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::rrel_2);
  ScTemplateSearchResult generatedActionSearchResult;
  m_ctx->HelperSearchTemplate(generatedActionTemplate, generatedActionSearchResult);

  EXPECT_TRUE(generatedActionSearchResult.Size() == 1);
}

TEST_F(ScMemoryTest, getActionResultIfExistForGeneratedAction)
{
  SC_AGENT_REGISTER(FinishActionTestAgent, scAgentsCommon::CoreKeynodes::question_initiated);

  ScAddr actionClass = m_ctx->CreateNode(ScType::NodeConstClass);
  ScAddrVector params = {m_ctx->CreateNode(ScType::NodeConst), m_ctx->CreateNode(ScType::NodeConst)};

  ScAddr const & answerNode = utils::AgentUtils::applyActionAndGetResultIfExists(m_ctx.get(), actionClass, params);
  EXPECT_TRUE(answerNode.IsValid());

  std::string const actionAlias = "_action";

  ScTemplate generatedActionTemplate;
  generatedActionTemplate.Triple(actionClass, ScType::EdgeAccessVarPosPerm, ScType::NodeVar >> actionAlias);
  generatedActionTemplate.Triple(
      scAgentsCommon::CoreKeynodes::question_finished, ScType::EdgeAccessVarPosPerm, actionAlias);
  generatedActionTemplate.Quintuple(
      actionAlias,
      ScType::EdgeDCommonVar,
      answerNode,
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::nrel_answer);
  ScTemplateSearchResult generatedActionSearchResult;
  m_ctx->HelperSearchTemplate(generatedActionTemplate, generatedActionSearchResult);

  EXPECT_TRUE(generatedActionSearchResult.Size() == 1);

  SC_AGENT_UNREGISTER(FinishActionTestAgent);
}

TEST_F(ScMemoryTest, getActionResultIfExistForExistingAction)
{
  SC_AGENT_REGISTER(FinishActionTestAgent, scAgentsCommon::CoreKeynodes::question_initiated);

  ScAddr actionNode = m_ctx->CreateNode(ScType::NodeConst);

  ScAddr const & answerNode = utils::AgentUtils::applyActionAndGetResultIfExists(m_ctx.get(), actionNode);
  EXPECT_TRUE(answerNode.IsValid());

  ScTemplate generatedActionTemplate;
  generatedActionTemplate.Quintuple(
      actionNode,
      ScType::EdgeDCommonVar,
      answerNode,
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::nrel_answer);
  generatedActionTemplate.Triple(
      scAgentsCommon::CoreKeynodes::question_finished, ScType::EdgeAccessVarPosPerm, actionNode);
  ScTemplateSearchResult generatedActionSearchResult;
  m_ctx->HelperSearchTemplate(generatedActionTemplate, generatedActionSearchResult);

  EXPECT_TRUE(generatedActionSearchResult.Size() == 1);

  SC_AGENT_UNREGISTER(FinishActionTestAgent);
}

TEST_F(ScMemoryTest, applyGeneratedAction)
{
  SC_AGENT_REGISTER(FinishActionTestAgent, scAgentsCommon::CoreKeynodes::question_initiated);

  ScAddr actionClass = m_ctx->CreateNode(ScType::NodeConstClass);
  ScAddrVector params = {m_ctx->CreateNode(ScType::NodeConst), m_ctx->CreateNode(ScType::NodeConst)};

  EXPECT_TRUE(utils::AgentUtils::applyAction(m_ctx.get(), actionClass, params));

  std::string const actionAlias = "_action";

  ScTemplate generatedActionTemplate;
  generatedActionTemplate.Triple(actionClass, ScType::EdgeAccessVarPosPerm, ScType::NodeVar >> actionAlias);
  generatedActionTemplate.Triple(
      scAgentsCommon::CoreKeynodes::question_finished, ScType::EdgeAccessVarPosPerm, actionAlias);
  generatedActionTemplate.Quintuple(
      actionAlias,
      ScType::EdgeAccessVarPosPerm,
      params[0],
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::rrel_1);
  generatedActionTemplate.Quintuple(
      actionAlias,
      ScType::EdgeAccessVarPosPerm,
      params[1],
      ScType::EdgeAccessVarPosPerm,
      scAgentsCommon::CoreKeynodes::rrel_2);
  ScTemplateSearchResult generatedActionSearchResult;
  m_ctx->HelperSearchTemplate(generatedActionTemplate, generatedActionSearchResult);

  EXPECT_TRUE(generatedActionSearchResult.Size() == 1);

  SC_AGENT_UNREGISTER(FinishActionTestAgent);
}

TEST_F(ScMemoryTest, applyExistingAction)
{
  SC_AGENT_REGISTER(FinishActionTestAgent, scAgentsCommon::CoreKeynodes::question_initiated);

  ScAddr actionNode = m_ctx->CreateNode(ScType::NodeConst);

  EXPECT_TRUE(utils::AgentUtils::applyAction(m_ctx.get(), actionNode));

  EXPECT_TRUE(m_ctx->HelperCheckEdge(
      scAgentsCommon::CoreKeynodes::question_finished, actionNode, ScType::EdgeAccessConstPosPerm));

  SC_AGENT_UNREGISTER(FinishActionTestAgent);
}
