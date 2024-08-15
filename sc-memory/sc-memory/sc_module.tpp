/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_module.hpp"

#include "sc_agent_builder.hpp"

template <class TScAgent, class... TScAddr, typename>
ScModule * ScModule::Agent(TScAddr const &... subscriptionAddrs)
{
  static_assert(
      (std::is_base_of<ScAddr, TScAddr>::value && ...), "Each element of parameter pack must have ScAddr type.");

  m_agents.push_back(
      {nullptr,
       GetAgentSubscribeCallback<TScAgent>(),
       GetAgentUnsubscribeCallback<TScAgent>(),
       {subscriptionAddrs...}});
  return this;
}

template <class TScAgent>
ScAgentBuilder<TScAgent> * ScModule::AgentBuilder(ScAddr const & agentImplementationAddr)
{
  auto * builder = new ScAgentBuilder<TScAgent>(this, agentImplementationAddr);
  m_agents.push_back({builder, GetAgentSubscribeCallback<TScAgent>(), GetAgentUnsubscribeCallback<TScAgent>(), {}});
  return builder;
}

template <class TScAgent, typename>
ScModule * ScModule::Agent()
{
  m_agents.push_back({nullptr, GetAgentSubscribeCallback<TScAgent>(), GetAgentUnsubscribeCallback<TScAgent>(), {}});
  return this;
}

template <class TScAgent>
ScModule::ScAgentSubscribeCallback ScModule::GetAgentSubscribeCallback()
{
  return [](ScMemoryContext * ctx, ScAddr const & agentImplementationAddr, ScAddrVector const & addrs)
  {
    if (agentImplementationAddr.IsValid())
      TScAgent::template Subscribe<TScAgent>(ctx, agentImplementationAddr);
    else
    {
      for (ScAddr const & addr : addrs)
        TScAgent::template Subscribe<TScAgent>(ctx, agentImplementationAddr, addr);
    }
  };
}

template <class TScAgent>
ScModule::ScAgentUnsubscribeCallback ScModule::GetAgentUnsubscribeCallback()
{
  return [](ScMemoryContext * ctx, ScAddr const & agentImplementationAddr, ScAddrVector const & addrs)
  {
    if (agentImplementationAddr.IsValid())
      TScAgent::template Unsubscribe<TScAgent>(ctx, agentImplementationAddr);
    else
    {
      for (ScAddr const & addr : addrs)
        TScAgent::template Unsubscribe<TScAgent>(ctx, agentImplementationAddr, addr);
    }
  };
}
