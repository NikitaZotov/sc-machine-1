#include <gtest/gtest.h>

#include "sc-memory/sc_memory.hpp"

#include "sc_test.hpp"
#include "sc-memory/sc_scs_helper.hpp"
#include "dummy_file_interface.hpp"

TEST_F(ScMemoryTest, AccessRights)
{
  SCsHelper helper(*m_ctx, std::make_shared<DummyFileInterface>());
  EXPECT_TRUE(helper.GenerateBySCsText(
      "action_in_sc_memory -> add_action; search_action; remove_action;;"
      "action_actor -> manager;;"
      "writer -> manager;;"
      "writer => nrel_accessible_action_in_sc_memory: add_action; search_action;;"
      "manager -> manager_1;;"
  ));

  ScAddr const actorAddr = m_ctx->HelperFindBySystemIdtf("manager_1");
  EXPECT_TRUE(actorAddr.IsValid());

  ScMemoryContext ctx{actorAddr};

  ScAddr const node = ctx.CreateLink();
  EXPECT_TRUE(node.IsValid());

  ScAddr const link = ctx.CreateLink();
  EXPECT_TRUE(link.IsValid());

  ScAddr const edge = ctx.CreateEdge(ScType::EdgeAccessConstPosPerm, node, link);
  EXPECT_TRUE(edge.IsValid());

  EXPECT_FALSE(ctx.EraseElement(node));
  EXPECT_TRUE(ctx.IsElement(node));

  EXPECT_TRUE(helper.GenerateBySCsText(
      "writer => nrel_accessible_action_in_sc_memory: remove_action;;"
  ));

  EXPECT_TRUE(ctx.EraseElement(node));
  EXPECT_FALSE(ctx.IsElement(node));
}
