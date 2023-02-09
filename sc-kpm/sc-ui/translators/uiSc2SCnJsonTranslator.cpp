/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "uiPrecompiled.h"
#include "uiSc2SCnJsonTranslator.h"

#include "uiTranslators.h"
#include "uiKeynodes.h"
#include "uiUtils.h"
#include <iostream>
#include <algorithm>
// #include <string_view>

// constexpr std::string_view SCN_TRANSLATOR_CONSTANTS [] ={

// };

uiSc2SCnJsonTranslator::uiSc2SCnJsonTranslator()
  : mScElementsInfoPool(0)
{
}

uiSc2SCnJsonTranslator::~uiSc2SCnJsonTranslator()
{
  if (mScElementsInfoPool)
    delete[] mScElementsInfoPool;
}

void uiSc2SCnJsonTranslator::runImpl()
{
  // get command arguments
  sc_iterator5 * it5 = sc_iterator5_a_a_f_a_f_new(
      s_default_ctx,
      sc_type_node | sc_type_const,
      sc_type_arc_common | sc_type_const,
      mInputConstructionAddr,
      sc_type_arc_pos_const_perm,
      keynode_question_nrel_answer);
  if (sc_iterator5_next(it5) == SC_TRUE)
  {
    sc_iterator3 * it3 =
        sc_iterator3_f_a_a_new(s_default_ctx, sc_iterator5_value(it5, 0), sc_type_arc_pos_const_perm, 0);
    while (sc_iterator3_next(it3) == SC_TRUE)
      mKeywordsList.push_back(sc_iterator3_value(it3, 2));
    sc_iterator3_free(it3);
  }
  sc_iterator5_free(it5);

  collectScElementsInfo();

  sc_json result = sc_json::array();

  tScAddrList::iterator it, itEnd = mKeywordsList.end();
  for (it = mKeywordsList.begin(); it != itEnd; ++it)
  {
    result.insert(result.end(), json(mScElementsInfo[*it], 0, false));
  }
  mOutputData = result.dump();
}

void uiSc2SCnJsonTranslator::collectScElementsInfo()
{
  sc_uint32 elementsCount = (sc_uint32)mObjects.size();
  mScElementsInfoPool = new sScElementInfo[elementsCount];

  sc_uint32 poolUsed = 0;
  // first of all collect information about elements
  tScAddrToScTypeMap::const_iterator it, itEnd = mObjects.end();
  for (it = mObjects.begin(); it != itEnd; ++it)
  {
    if (mScElementsInfo.find(it->first) != mScElementsInfo.end())
      continue;

    mScElementsInfoPool[poolUsed].addr = it->first;
    mScElementsInfoPool[poolUsed].type = it->second;
    mScElementsInfo[it->first] = &(mScElementsInfoPool[poolUsed]);
    poolUsed++;
  }

  // now we need to itarete all arcs and collect output/input arcs info
  sc_type elType;
  sc_addr begAddr, endAddr, arcAddr;
  for (it = mObjects.begin(); it != itEnd; ++it)
  {
    arcAddr = it->first;
    elType = it->second;

    sScElementInfo * elInfo = mScElementsInfo[arcAddr];
    elInfo->isInTree = false;

    // skip nodes and links
    if (!(elType & sc_type_arc_mask))
      continue;

    // get begin/end addrs
    if (sc_memory_get_arc_begin(s_default_ctx, arcAddr, &begAddr) != SC_RESULT_OK)
      continue;  // @todo process errors
    if (isNeedToTranslate(begAddr) == false)
      continue;
    if (sc_memory_get_arc_end(s_default_ctx, arcAddr, &endAddr) != SC_RESULT_OK)
      continue;  // @todo process errors
    if (isNeedToTranslate(endAddr) == false)
      continue;

    elInfo->srcAddr = begAddr;
    elInfo->trgAddr = endAddr;

    sScElementInfo * begInfo = mScElementsInfo[begAddr];
    sScElementInfo * endInfo = mScElementsInfo[endAddr];

    elInfo->source = begInfo;
    elInfo->target = endInfo;

    // check if arc is not broken
    if (begInfo == 0 || endInfo == 0)
      continue;
    endInfo->inputArcs.push_back(mScElementsInfo[arcAddr]);
    begInfo->outputArcs.push_back(mScElementsInfo[arcAddr]);
  }

  // find structures elements
  tScAddrList::const_iterator keywordIt, keywordItEnd = mKeywordsList.end();
  sScElementInfo::tScElementInfoList::const_iterator elIt, elItEnd, structureElementIt;
  sScElementInfo * elInfo;
  for (keywordIt = mKeywordsList.begin(); keywordIt != keywordItEnd; ++keywordIt)
  {
    elInfo = mScElementsInfo[*keywordIt];
    if (elInfo->type & sc_type_node_struct)
    {
      elItEnd = elInfo->outputArcs.end();
      for(elIt = elInfo->outputArcs.begin(); elIt != elItEnd;)
      {
        if ((*elIt)->type & sc_type_arc_pos_const_perm && (*elIt)->inputArcs.empty())
        {
          structureElementIt = std::find((*elIt)->target->inputArcs.begin(),(*elIt)->target->inputArcs.end(), *elIt);
          if (structureElementIt != (*elIt)->target->inputArcs.end())
          {
            elInfo->structureElements.push_back((*elIt)->target);
            (*elIt)->target->inputArcs.erase(structureElementIt);
            elIt = elInfo->outputArcs.erase(elIt);
          }
          else
          {
            ++elIt;
          }
        }
        else
        {
          ++elIt;
        }
      }
      elInfo->structKeyword = findStructKeyword(elInfo->structureElements);
    }
  }
}

sScElementInfo * uiSc2SCnJsonTranslator::findStructKeyword(sScElementInfo::tScElementInfoList structureElements)
{
  sScElementInfo::tScElementInfoList structures;
  sScElementInfo::tScElementInfoList elements = structureElements;
  std::copy_if(
      structureElements.begin(),
      structureElements.end(),
      std::back_inserter(structures),
      [](sScElementInfo * el){return el->type & sc_type_node_struct;}
    );
  if (!structures.empty())
  {
    elements = structures;
  }
  sScElementInfo::tScElementInfoList::const_iterator result = std::max_element(
    elements.begin(),
    elements.end(),
    [](sScElementInfo * left, sScElementInfo * right) {
      return (left->outputArcs.size() + left->inputArcs.size()) < (right->outputArcs.size() + right->inputArcs.size());
    });
  if (result != elements.end())
    return *result;
  return nullptr;
}

sc_json uiSc2SCnJsonTranslator::json(sScElementInfo * elInfo, int level, bool isStruct)
{
  sc_json result;
  String content;
  bool content_exists;
  bool isFullLinkedNodes = true;

  result = getBaseInfo(elInfo);
  if (!elInfo->structureElements.empty())
  {
    structureElements = elInfo->structureElements;
    result["struct"] = json(elInfo->structKeyword, 0, true);
  }
  // if node is arc
  if (elInfo->type & sc_type_arc_mask)
  {
    result["sourceNode"] = getBaseInfo(elInfo->source);
    result["targetNode"] = getBaseInfo(elInfo->target);
  }
  //if node is link
  if (elInfo->type & sc_type_link)
  {
    isFullLinkedNodes = false;
    sc_addr format;
    String content;
    sc_helper_resolve_system_identifier(s_default_ctx,"format_html", &format);
    if (sc_helper_check_arc(s_default_ctx, elInfo->addr, format, sc_type_arc_common | sc_type_const) == SC_TRUE)
    {
      result["contentType"] = "format_html";
    }
    sc_helper_resolve_system_identifier(s_default_ctx,"format_github_source_link", &format);
    if (sc_helper_check_arc(s_default_ctx, elInfo->addr, format, sc_type_arc_common | sc_type_const) == SC_TRUE)
    {
      result["contentType"] = "format_github_source_link";
    }
    sc_helper_resolve_system_identifier(s_default_ctx,"format_pdf", &format);
    if (sc_helper_check_arc(s_default_ctx, elInfo->addr, format, sc_type_arc_common | sc_type_const) == SC_TRUE)
    {
      result["contentType"] = "format_pdf";
    }
    sc_helper_resolve_system_identifier(s_default_ctx,"format_png", &format);
    if (sc_helper_check_arc(s_default_ctx, elInfo->addr, format, sc_type_arc_common | sc_type_const) == SC_TRUE)
    {
      result["contentType"] = "format_png";
    }
    if (result["contentType"].is_null())
    {
      content_exists = getLinkContent(elInfo->addr, content);
      if (content_exists)
      {
        result["content"] = content;
      }
      else
      {
        result["content"] = sc_json();
      }
      result["contentType"] = "format_txt";
    }
  }
  if (level < maxLevel || (elInfo->type & sc_type_node && elInfo->type & sc_type_node_tuple))
  {
    result["children"] = getChildrens(elInfo, isFullLinkedNodes, level + 1, isStruct);
  }

  return result;
}

sc_json uiSc2SCnJsonTranslator::getChildrens(sScElementInfo * elInfo, bool isFullLinkedNodes, int level, bool isStruct)
{
  sc_json childrens = sc_json::array();
  sScElementInfo::tScElementInfoList outputArcs = elInfo->outputArcs;
  sScElementInfo::tScElementInfoList inputArcs = elInfo->inputArcs;

  sc_json rightChildrens = getChildrensByDirection(outputArcs, "right", isStruct);
  childrens.insert(childrens.end(), rightChildrens.begin(), rightChildrens.end());

  sc_json leftChildrens = getChildrensByDirection(inputArcs, "left", isStruct);
  childrens.insert(childrens.end(), leftChildrens.begin(), leftChildrens.end());

  childrens = groupChildrensByModifier(childrens);
  if (isFullLinkedNodes)
    childrens = getJsonOfLinkedNodes(childrens, level, isStruct);
  return childrens;
}

sc_json uiSc2SCnJsonTranslator::getChildrensByDirection(sScElementInfo::tScElementInfoList arcs, String direction, bool isStruct)
{
  sc_json childrens = sc_json::array();
  sScElementInfo::tScElementInfoList::const_iterator it, itEnd = arcs.end();
  for (it = arcs.begin(); it != itEnd; ++it)
  {
    sc_json children;
    if ((*it)->isInTree)
      continue;
    if (isStruct && (std::find(structureElements.begin(), structureElements.end(), *it) == structureElements.end()))
      continue;
    (*it)->isInTree = true;

    sScElementInfo * linkedNode;
    if (direction.compare("right") == 0)
    {
      linkedNode = (*it)->target;
    }
    else if (direction.compare("left") == 0)
    {
      linkedNode = (*it)->source;
    }

    sc_json arc = getBaseInfo(*it);
    arc["direction"] = direction;

    sScElementInfo::tScElementInfoList modifiersList = (*it)->inputArcs;

    sScElementInfo::tScElementInfoList::const_iterator modifierIt = modifiersList.begin();
    sc_json modifier = sc_json();
    if (modifierIt != modifiersList.end() && !((*modifierIt)->isInTree))
    {
        modifier = getBaseInfo((*modifierIt)->source);
        modifier["modifierArc"] = getBaseInfo(*modifierIt);
        // modifier["modifierArc"]["content"] = sc_json();
        (*modifierIt)->isInTree = true;
    }

    children["arc"] = arc;
    children["modifier"] = modifier;
    children["linkedNode"] = getBaseInfo(linkedNode);

    childrens.push_back(children);
  }
  return childrens;
}

sc_json uiSc2SCnJsonTranslator::groupChildrensByModifier(sc_json childrens)
{
  sc_json groupedChildrens, visited, arcs, modifiers, linkedNodes, filtered = sc_json::array();
  sc_json children, groupedChildren, modifier, modifierAddr;
  for (sc_json::iterator it = childrens.begin(); it != childrens.end(); ++it)
  {
    // check is element visited
    children = *it;
    if (std::find(visited.begin(), visited.end(), children) != visited.end())
      continue;

    groupedChildren = sc_json();
    arcs = sc_json::array();
    modifiers = sc_json::array();
    linkedNodes = sc_json::array();

    //get modifier
    if (children["modifier"].is_null())
    {
      groupedChildren["arcs"][0] = children["arc"];
      groupedChildren["modifiers"] = sc_json();
      groupedChildren["linkedNodes"][0] = children["linkedNode"];

      groupedChildrens.push_back(groupedChildren);
      visited.push_back(children);
      continue;
    }
    std::cout << children["modifier"].dump() << std::endl;
    modifier = children["modifier"];
    modifierAddr = modifier["addr"];
    modifier["modifierArcs"] = sc_json::array();

    // get elements with same modifiers
    filtered = sc_json::array();
    std::copy_if(
      childrens.begin(),
      childrens.end(),
      std::back_inserter(filtered),
      [modifierAddr](sc_json c){ return modifierAddr == c["modifier"]["addr"];});

    for (sc_json::iterator j = filtered.begin(); j != filtered.end(); ++j)
    {
      //group childrens
      sc_json fc = *j;
      arcs.push_back(fc["arc"]);
      modifier["modifierArcs"].push_back(fc["modifier"]["modifierArc"]);
      modifier.erase("modifierArc");
      linkedNodes.push_back(fc["linkedNode"]);

      // set element as visited
      visited.push_back(fc);
    }
    modifiers.push_back(modifier);

    groupedChildren["arcs"] = arcs;
    groupedChildren["modifiers"] = modifiers;
    groupedChildren["linkedNodes"] = linkedNodes;

    groupedChildrens.push_back(groupedChildren);
  }

  return groupedChildrens;
}

sc_json uiSc2SCnJsonTranslator::getJsonOfLinkedNodes(sc_json childrens, int level, bool isStruct)
{
  sc_json new_childrens = sc_json();
  sc_json children, linkedNode;
  sc_addr keynode_nrel_sc_text_translation, linkedNodeAddr;
  sScElementInfo * linkedNodeInfo;
  sc_helper_resolve_system_identifier(s_default_ctx,"nrel_sc_text_translation", &keynode_nrel_sc_text_translation);

  for (sc_json::const_iterator it = childrens.begin(); it != childrens.end(); ++it)
  {
    children = *it;
    if (!children["modifiers"].is_null() && children["modifiers"][0]["addr"] == SC_ADDR_LOCAL_TO_INT(keynode_nrel_sc_text_translation))
    {
      --level;
    }
    for (size_t i = 0; i < children["linkedNodes"].size(); ++i)
    {
      linkedNode = children["linkedNodes"][i];
      linkedNodeAddr.seg = SC_ADDR_LOCAL_SEG_FROM_INT(linkedNode["addr"].get<int>());
      linkedNodeAddr.offset = SC_ADDR_LOCAL_OFFSET_FROM_INT(linkedNode["addr"].get<int>());
      linkedNodeInfo = mScElementsInfo[linkedNodeAddr];
      if (!(linkedNodeInfo->type & sc_type_node_struct))
      {
        linkedNode = json(linkedNodeInfo, level, isStruct);
      }
      children["linkedNodes"][i] = linkedNode;
    }

    new_childrens.push_back(children);
  }
  return new_childrens;
}

bool uiSc2SCnJsonTranslator::getLinkContent(sc_addr link_addr, String & content)
{
  sc_stream * link_stream = nullptr;
  sc_uint32 link_length = 0;
  sc_uint32 read_bytes = 0;
  sc_bool result = SC_FALSE;
  sc_char buffer[32];

  content = "";
  if (sc_memory_get_link_content(s_default_ctx, link_addr, &link_stream) == SC_RESULT_OK)
  {
    sc_stream_get_length(link_stream, &link_length);
     while (sc_stream_eof(link_stream) == SC_FALSE)
      {
        sc_stream_read_data(link_stream, buffer, 32, &read_bytes);
        content.append(buffer, read_bytes);
      }
      sc_stream_free(link_stream);

      result = SC_TRUE;
    }

  if (result == SC_FALSE)
  {
    content = "";
    return SC_FALSE;
  }

  return result;
}

sc_json uiSc2SCnJsonTranslator::getBaseInfo(sScElementInfo * elInfo)
{
  sc_json result;
  result["addr"] = std::stoi(uiTranslateFromSc::buildId(elInfo->addr));
  String idtf;
  bool idtf_exists = ui_translate_get_identifier(elInfo->addr, mOutputLanguageAddr, idtf);
  if (idtf_exists)
  {
    result["idtf"] = idtf;
  }
  else
  {
    result["idtf"] = sc_json();
  }
  result["type"] = elInfo->type;

  return result;
}

// -------------------------------------
sc_result uiSc2SCnJsonTranslator::ui_translate_sc2scn(const sc_event * event, sc_addr arg)
{
  sc_addr cmd_addr, input_addr, format_addr, lang_addr;

  if (sc_memory_get_arc_end(s_default_ctx, arg, &cmd_addr) != SC_RESULT_OK)
    return SC_RESULT_ERROR;

  if (ui_check_cmd_type(cmd_addr, keynode_command_translate_from_sc) != SC_RESULT_OK)
    return SC_RESULT_ERROR;

  if (ui_translate_command_resolve_arguments(cmd_addr, &format_addr, &input_addr, &lang_addr) != SC_RESULT_OK)
    return SC_RESULT_ERROR;

  if (format_addr == keynode_format_scn_json)
  {
    uiSc2SCnJsonTranslator translator;
    translator.translate(input_addr, format_addr, lang_addr);
  }

  return SC_RESULT_OK;
}
