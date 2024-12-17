/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#include <gtest/gtest.h>

#include <sstream>

#include "test_scs_utils.hpp"


TEST(scs_common, ElementHandle)
{
  scs::ElementHandle handle_err;
  EXPECT_FALSE(handle_err.IsValid());
  EXPECT_FALSE(handle_err.IsLocal());

  scs::ElementHandle handle_ok(1);
  EXPECT_TRUE(handle_ok.IsValid());
  EXPECT_FALSE(handle_ok.IsLocal());

  scs::ElementHandle handle_local(0, scs::Visibility::Local);
  EXPECT_TRUE(handle_local.IsValid());
  EXPECT_TRUE(handle_local.IsLocal());
}

TEST(scs_common, parser_error)
{
  char const * data = "a -> b;;\nc ->";

  scs::Parser parser;

  EXPECT_FALSE(parser.Parse(data));
  SC_LOG_WARNING(parser.GetParseError());
}

TEST(scs_common, parser_triple_1)
{
  scs::Parser parser;
  char const * data = "a -> b;;";
  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 1u);

  SPLIT_TRIPLE(triples[0]);

  EXPECT_EQ(src.GetType(), ScType::ConstNode);
  EXPECT_EQ(connector.GetType(), ScType::ConstPermPosArc);
  EXPECT_EQ(trg.GetType(), ScType::ConstNode);

  EXPECT_EQ(src.GetIdtf(), "a");
  EXPECT_EQ(trg.GetIdtf(), "b");

  EXPECT_EQ(src.GetVisibility(), scs::Visibility::System);
  EXPECT_EQ(trg.GetVisibility(), scs::Visibility::System);
}

TEST(scs_common, parser_reversed_1)
{
  scs::Parser parser;
  char const * data = "a <- b;;";
  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 1u);

  SPLIT_TRIPLE(triples[0]);

  EXPECT_EQ(src.GetIdtf(), "b");
  EXPECT_EQ(trg.GetIdtf(), "a");
  EXPECT_EQ(connector.GetType(), ScType::ConstPermPosArc);
}

TEST(scs_common, sentences_1)
{
  scs::Parser parser;
  char const * data = "a <- b;; r => x;;";
  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 2u);

  {
    auto const & t = triples[0];
    auto const & source = parser.GetParsedElement(t.m_source);
    auto const & target = parser.GetParsedElement(t.m_target);

    EXPECT_EQ(source.GetIdtf(), "b");
    EXPECT_EQ(target.GetIdtf(), "a");
  }

  {
    SPLIT_TRIPLE(triples[1]);

    EXPECT_EQ(src.GetIdtf(), "r");
    EXPECT_EQ(trg.GetIdtf(), "x");
    EXPECT_EQ(connector.GetType(), ScType::ConstCommonArc);
  }
}

TEST(scs_common, scs_comments)
{
  scs::Parser parser;

  char const * data =
      "//Level1\n"
      "a -> b;;/* example */\n"
      "c <> d;;";

  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 2u);

  {
    SPLIT_TRIPLE(triples[0]);

    EXPECT_EQ(src.GetIdtf(), "a");
    EXPECT_EQ(trg.GetIdtf(), "b");

    EXPECT_EQ(src.GetType(), ScType::ConstNode);
    EXPECT_EQ(trg.GetType(), ScType::ConstNode);
    EXPECT_EQ(connector.GetType(), ScType::ConstPermPosArc);
  }

  {
    SPLIT_TRIPLE(triples[1]);

    EXPECT_EQ(src.GetIdtf(), "c");
    EXPECT_EQ(trg.GetIdtf(), "d");

    EXPECT_EQ(src.GetType(), ScType::ConstNode);
    EXPECT_EQ(trg.GetType(), ScType::ConstNode);
    EXPECT_EQ(connector.GetType(), ScType::CommonEdge);
  }
}

TEST(scs_common, const_var)
{
  char const * data = "_a _-> b;;";
  scs::Parser parser;

  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 1u);

  {
    SPLIT_TRIPLE(triples[0]);

    EXPECT_EQ(src.GetType(), ScType::VarNode);
    EXPECT_EQ(connector.GetType(), ScType::VarPermPosArc);
    EXPECT_EQ(trg.GetType(), ScType::ConstNode);

    EXPECT_EQ(src.GetIdtf(), "_a");
    EXPECT_EQ(trg.GetIdtf(), "b");
  }
}

class TestScType : public ScType
{
public:
  constexpr TestScType(sc_type type)
    : ScType(type)
  {
  }

  TestScType BitOr(ScType const & other) const
  {
    return *this | other;
  }
};

TEST(scs_common, SCsNodeKeynodes)
{
  std::vector<ScType> const & nodeTypes = {
    ScType::Node,
    ScType::NodeClass,
    ScType::NodeRole,
    ScType::NodeNonRole,
    ScType::NodeTuple,
    ScType::NodeStructure,
    ScType::NodeSuperclass,
    ScType::NodeLink,
    ScType::NodeLinkClass,
  };

  std::stringstream stream;
  for (ScType const & nodeType : nodeTypes)
  {
    stream << ".._node_" << std::string(nodeType) << " -> " << "..node_" << std::string(nodeType) << ";;\n" << std::endl;
    stream << nodeType.GetSCsElementKeynode() << " -> " << "..node_" << std::string(nodeType) << ";;\n" << std::endl;
    stream << nodeType.GetSCsElementKeynode() << " -> " << ".._node_" << std::string(nodeType) << ";;\n" << std::endl;
  }

  scs::Parser parser;
  EXPECT_TRUE(parser.Parse(stream.str()));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), nodeTypes.size() * 3);

  auto const GetSourceNodeType = [&triples, &parser](size_t index) -> ScType
  {
    EXPECT_TRUE(index < triples.size());
    return parser.GetParsedElement(triples[index].m_source).GetType();
  };

  auto const GetTargetNodeType = [&triples, &parser](size_t index) -> ScType
  {
    EXPECT_TRUE(index < triples.size());
    return parser.GetParsedElement(triples[index].m_target).GetType();
  };

  for (size_t i = 0, j = 0; j < nodeTypes.size(); i += 3, ++j)
  {
    EXPECT_EQ(GetSourceNodeType(i), TestScType(nodeTypes[j]).BitOr(ScType::Var));
    EXPECT_EQ(GetTargetNodeType(i), TestScType(nodeTypes[j]).BitOr(ScType::Const));
  }
}

TEST(scs_common, Links)
{
  std::string const data =
      "a -> \"file://data.txt\";;"
      "b -> [x];;"
      "c -> _[];;"
      "d -> [];;"
      "e -> ![lala]!;;"
      "f -> ![]!;;"
      "g -> [tra! tra!];;";
  scs::Parser parser;

  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();

  EXPECT_EQ(triples.size(), 7u);

  EXPECT_EQ(parser.GetParsedElement(triples[0].m_target).GetType(), ScType::ConstNodeLink);
  EXPECT_TRUE(parser.GetParsedElement(triples[0].m_target).IsURL());
  EXPECT_EQ(parser.GetParsedElement(triples[1].m_target).GetType(), ScType::ConstNodeLink);
  EXPECT_FALSE(parser.GetParsedElement(triples[2].m_target).IsURL());
  EXPECT_EQ(parser.GetParsedElement(triples[2].m_target).GetType(), ScType::VarNodeLink);
  EXPECT_EQ(parser.GetParsedElement(triples[3].m_target).GetType(), ScType::ConstNodeLink);
  EXPECT_FALSE(parser.GetParsedElement(triples[3].m_target).IsURL());
  EXPECT_EQ(parser.GetParsedElement(triples[4].m_target).GetType(), ScType::ConstNodeLinkClass);
  EXPECT_EQ(parser.GetParsedElement(triples[4].m_target).GetValue(), "lala");
  EXPECT_EQ(parser.GetParsedElement(triples[5].m_target).GetType(), ScType::ConstNodeLinkClass);
  EXPECT_EQ(parser.GetParsedElement(triples[5].m_target).GetValue(), "");
  EXPECT_EQ(parser.GetParsedElement(triples[6].m_target).GetType(), ScType::ConstNodeLink);
  EXPECT_EQ(parser.GetParsedElement(triples[6].m_target).GetValue(), "tra! tra!");
}

TEST(scs_common, LinkAssigns)
{
  std::string const data =
      "_a = \"file://data.txt\";;"
      "_b = _[x];;"
      "_c = _[];;"
      "_d = _[];;"
      "_e = _![lala]!;;"
      "_f = _![]!;;";
  scs::Parser parser;

  EXPECT_TRUE(parser.Parse(data));

  parser.ForEachParsedElement([](scs::ParsedElement const & element) -> void {
    EXPECT_EQ(element.GetType() & ScType::VarNodeLink, ScType::VarNodeLink);
  });

  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(0)).GetValue(), "file://data.txt");
  EXPECT_TRUE(parser.GetParsedElement(scs::ElementHandle(0)).IsURL());
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(0)).GetIdtf(), "_a");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(1)).GetValue(), "x");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(1)).GetIdtf(), "_b");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(2)).GetValue(), "");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(2)).GetIdtf(), "_c");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(3)).GetValue(), "");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(3)).GetIdtf(), "_d");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(4)).GetValue(), "lala");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(4)).GetType(), ScType::VarNodeLinkClass);
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(4)).GetIdtf(), "_e");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(5)).GetValue(), "");
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(5)).GetType(), ScType::VarNodeLinkClass);
  EXPECT_EQ(parser.GetParsedElement(scs::ElementHandle(5)).GetIdtf(), "_f");
}

TEST(scs_common, DeprecatedSCsKeynodes)
{
  std::string const data = "a <- c;; a <- sc_node_not_relation;; b <- c;; b <- sc_node_not_binary_tuple;; c <- sc_node_struct;; a <- d;; d <- sc_node_norole_relation;;";
  scs::Parser parser;

  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 7u);

  EXPECT_EQ(parser.GetParsedElement(triples[0].m_source).GetType(), ScType::ConstNodeStructure);
  EXPECT_EQ(parser.GetParsedElement(triples[0].m_target).GetType(), ScType::ConstNodeClass);
  EXPECT_EQ(parser.GetParsedElement(triples[2].m_source).GetType(), ScType::ConstNodeStructure);
  EXPECT_EQ(parser.GetParsedElement(triples[2].m_target).GetType(), ScType::ConstNodeTuple);
  EXPECT_EQ(parser.GetParsedElement(triples[5].m_source).GetType(), ScType::ConstNodeNonRole);
}

TEST(scs_common, DirectConnectors)
{
  auto const & connectorTypes = ScType::GetConnectorTypes();

  std::vector<ScType> connectorTypesVec{connectorTypes.cbegin(), connectorTypes.cend()};
  std::stringstream stream;
  for (ScType const & connectorType : connectorTypesVec)
    stream << "x " << connectorType.GetDirectSCsConnector() << " y;;\n" << std::endl;

  scs::Parser parser;
  EXPECT_TRUE(parser.Parse(stream.str()));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), connectorTypes.size());

  auto const GetConnectorType = [&triples, &parser](size_t index) -> ScType
  {
    EXPECT_TRUE(index < triples.size());
    return parser.GetParsedElement(triples[index].m_connector).GetType();
  };

  for (size_t i = 0; i < connectorTypesVec.size(); ++i)
    EXPECT_EQ(GetConnectorType(i), connectorTypesVec[i]);
}

TEST(scs_common, ReverseConnectors)
{
  auto const & connectorTypes = ScType::GetConnectorTypes();

  std::vector<ScType> connectorTypesVec{connectorTypes.cbegin(), connectorTypes.cend()};
  std::stringstream stream;
  for (ScType const & connectorType : connectorTypesVec)
    stream << "x " << connectorType.GetReverseSCsConnector() << " y;;\n" << std::endl;

  scs::Parser parser;
  EXPECT_TRUE(parser.Parse(stream.str()));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), connectorTypes.size());

  auto const GetConnectorType = [&triples, &parser](size_t index) -> ScType
  {
    EXPECT_TRUE(index < triples.size());
    return parser.GetParsedElement(triples[index].m_connector).GetType();
  };

  for (size_t i = 0; i < connectorTypesVec.size(); ++i)
    EXPECT_EQ(GetConnectorType(i), connectorTypesVec[i]);
}

TEST(scs_common, ReversedConnectors)
{
  std::string const data =
      "x"
      "< _y; <=_ y1; <-_ y2;"
      "<|-_ y3; </_ y4; <~_ y5; <|~_ y6;"
      "</_ y7;;";

  scs::Parser parser;

  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 8u);
  {
    auto const CheckEdgeType = [&triples, &parser](size_t index, ScType type) -> bool {
      EXPECT_TRUE(index < triples.size());
      return (parser.GetParsedElement(triples[index].m_connector).GetType() == type);
    };

    EXPECT_TRUE(CheckEdgeType(0, ScType::CommonArc));
    EXPECT_TRUE(CheckEdgeType(1, ScType::VarCommonArc));
    EXPECT_TRUE(CheckEdgeType(2, ScType::VarPermPosArc));
    EXPECT_TRUE(CheckEdgeType(3, ScType::VarPermNegArc));
    EXPECT_TRUE(CheckEdgeType(4, ScType::VarFuzArc));
    EXPECT_TRUE(CheckEdgeType(5, ScType::VarActualTempPosArc));
    EXPECT_TRUE(CheckEdgeType(6, ScType::VarActualTempNegArc));
    EXPECT_TRUE(CheckEdgeType(7, ScType::VarFuzArc));
  }
}

TEST(scs_common, VarConnectorsAndNodes)
{
  std::string const data =
      "x"
      "<-_y; <-_ y1; <- _y2; <-_ _y3;;";

  std::string const errorData =
      "x <- _ y3;;";

  scs::Parser parser;

  EXPECT_TRUE(parser.Parse(data));
  EXPECT_FALSE(parser.Parse(errorData));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 4u);
  {
    auto const CheckEdgeType = [&triples, &parser](size_t index, ScType type) -> bool {
      EXPECT_TRUE(index < triples.size());
      return (parser.GetParsedElement(triples[index].m_connector).GetType() == type);
    };

    EXPECT_TRUE(CheckEdgeType(0, ScType::VarPermPosArc));
    EXPECT_EQ(parser.GetParsedElement(triples[0].m_source).GetType(), ScType::ConstNode);

    EXPECT_TRUE(CheckEdgeType(1, ScType::VarPermPosArc));
    EXPECT_EQ(parser.GetParsedElement(triples[1].m_source).GetType(), ScType::ConstNode);

    EXPECT_TRUE(CheckEdgeType(2, ScType::ConstPermPosArc));
    EXPECT_EQ(parser.GetParsedElement(triples[2].m_source).GetType(), ScType::VarNode);

    EXPECT_TRUE(CheckEdgeType(3, ScType::VarPermPosArc));
    EXPECT_EQ(parser.GetParsedElement(triples[3].m_source).GetType(), ScType::VarNode);
  }
}

TEST(scs_common, type_error)
{
  std::string const data = "a <- sc_node_material;; a <- sc_node_role_relation;;";

  scs::Parser parser;
  EXPECT_FALSE(parser.Parse(data));
}

TEST(scs_common, file_url)
{
  std::string const data = "scs_file_url -> \"file://html/faq.html\";;";

  scs::Parser parser;

  EXPECT_TRUE(parser.Parse(data));

  auto const & triples = parser.GetParsedTriples();
  EXPECT_EQ(triples.size(), 1u);

  SPLIT_TRIPLE(triples[0]);

  EXPECT_EQ(trg.GetType(), ScType::ConstNodeLink);
  EXPECT_EQ(trg.GetValue(), "file://html/faq.html");
  EXPECT_TRUE(trg.IsURL());
}
