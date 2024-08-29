/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_agent_context.hpp"

#include <algorithm>

#include "sc_event_subscription.hpp"

#include "sc_action.hpp"
#include "sc_keynodes.hpp"

ScAgentContext::ScAgentContext() noexcept
  : ScAgentContext(ScAddr::Empty)
{
}

ScAgentContext::ScAgentContext(sc_memory_context * context) noexcept
  : ScMemoryContext(context)
{
}

ScAgentContext::ScAgentContext(ScAddr const & userAddr) noexcept
  : ScMemoryContext(userAddr)
{
}

ScAgentContext::ScAgentContext(ScAgentContext && other) noexcept
  : ScMemoryContext(std::move(other)) {};

ScAgentContext & ScAgentContext::operator=(ScAgentContext && other) noexcept
{
  if (this == &other)
    return *this;

  ScMemoryContext::operator=(std::move(other));
  return *this;
}

std::shared_ptr<ScElementaryEventSubscription<ScElementaryEvent>> ScAgentContext::CreateElementaryEventSubscription(
    ScAddr const & eventClassAddr,
    ScAddr const & subscriptionElementAddr,
    std::function<void(ScElementaryEvent const &)> const & eventCallback) noexcept(false)
{
  ValidateEventElements(eventClassAddr, subscriptionElementAddr, "elementary sc-event subscription");

  return std::shared_ptr<ScElementaryEventSubscription<ScElementaryEvent>>(
      new ScElementaryEventSubscription<ScElementaryEvent>(
          *this, eventClassAddr, subscriptionElementAddr, eventCallback));
}

std::shared_ptr<ScWaiter> ScAgentContext::CreateEventWaiter(
    ScAddr const & eventClassAddr,
    ScAddr const & subscriptionElementAddr,
    std::function<void(void)> const & initiateCallback) noexcept(false)
{
  ValidateEventElements(eventClassAddr, subscriptionElementAddr, "sc-event waiter");

  auto eventWait = std::shared_ptr<ScEventWaiter<ScElementaryEvent>>(
      new ScEventWaiter<ScElementaryEvent>(*this, eventClassAddr, subscriptionElementAddr));
  eventWait->SetOnWaitStartDelegate(initiateCallback);
  return eventWait;
}

std::shared_ptr<ScWaiter> ScAgentContext::CreateConditionWaiter(
    ScAddr const & eventClassAddr,
    ScAddr const & subscriptionElementAddr,
    std::function<void(void)> const & initiateCallback,
    std::function<bool(ScElementaryEvent const &)> const & checkCallback) noexcept(false)
{
  ValidateEventElements(eventClassAddr, subscriptionElementAddr, "condition waiter");

  auto eventWait = std::shared_ptr<ScConditionWaiter<ScElementaryEvent>>(
      new ScConditionWaiter<ScElementaryEvent>(*this, eventClassAddr, subscriptionElementAddr, checkCallback));
  eventWait->SetOnWaitStartDelegate(initiateCallback);
  return eventWait;
}

std::shared_ptr<ScWaiter> ScAgentContext::CreateConditionWaiter(
    ScAddr const & eventClassAddr,
    ScAddr const & subscriptionElementAddr,
    std::function<bool(ScElementaryEvent const &)> const & checkCallback) noexcept(false)
{
  return CreateConditionWaiter(eventClassAddr, subscriptionElementAddr, {}, checkCallback);
}

ScAction ScAgentContext::CreateAction(ScAddr const & actionClassAddr) noexcept(false)
{
  ScAddr const & actionAddr = CreateNode(ScType::NodeConst);
  CreateEdge(ScType::EdgeAccessConstPosPerm, actionClassAddr, actionAddr);
  ScAction action{this, actionAddr};
  return action;
}

ScAction ScAgentContext::ConvertToAction(ScAddr const & actionAddr) noexcept(false)
{
  ScAction action{this, actionAddr};
  return action;
}

ScSet ScAgentContext::CreateSet()
{
  ScAddr const & setAddr = CreateNode(ScType::NodeConst);
  ScSet set{this, setAddr};
  return set;
}

ScSet ScAgentContext::ConvertToSet(ScAddr const & setAddr) noexcept(false)
{
  ScSet set{this, setAddr};
  return set;
}

ScStructure ScAgentContext::CreateStructure()
{
  ScAddr const & structureAddr = CreateNode(ScType::NodeConstStruct);
  ScStructure structure{this, structureAddr};
  return structure;
}

ScStructure ScAgentContext::ConvertToStructure(ScAddr const & structureAddr) noexcept(false)
{
  ScStructure structure{this, structureAddr};
  return structure;
}

void ScAgentContext::ValidateEventElements(
    ScAddr const & eventClassAddr,
    ScAddr const & subscriptionElementAddr,
    std::string const & validatorName)
{
  if (!IsElement(eventClassAddr))
    SC_THROW_EXCEPTION(
        utils::ExceptionInvalidParams,
        "Not able to create " << validatorName << " because sc-event class is not valid.");

  if (!HelperCheckEdge(ScKeynodes::sc_event, eventClassAddr, ScType::EdgeAccessConstPosPerm))
    SC_THROW_EXCEPTION(
        utils::ExceptionInvalidParams,
        "Not able to create " << validatorName
                              << " because sc-event class is not belongs to "
                                 "`sc_event`.");

  if (!IsElement(subscriptionElementAddr))
    SC_THROW_EXCEPTION(
        utils::ExceptionInvalidParams,
        "Not able to create " << validatorName << " because subscription sc-element is not valid.");

  if (eventClassAddr == ScKeynodes::sc_event_before_change_link_content
      && !GetElementType(subscriptionElementAddr).IsLink())
    SC_THROW_EXCEPTION(
        utils::ExceptionInvalidParams,
        "Not able to create " << validatorName
                              << " of changing link content because subscription sc-element is "
                                 "not sc-link.");
}
