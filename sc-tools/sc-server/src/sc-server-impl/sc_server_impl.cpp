/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_server_impl.hpp"

#include "sc_server_action_defines.hpp"

extern "C"
{
#include "sc-core/sc-store/sc_storage.h"
#include "sc-core/sc-store/sc_storage_private.h"
#include "sc-core/sc-store/sc-base/sc_thread.h"
}

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8090

ScServerImpl::ScServerImpl(sc_memory_params const & params)
  : ScServerImpl(DEFAULT_HOST, DEFAULT_PORT, SC_FALSE, params)
{
}

ScServerImpl::ScServerImpl(
    std::string const & host,
    ScServerPort port,
    sc_bool syncActions,
    sc_memory_params const & params)
  : ScServer(host, port, params)
  , m_syncActions(syncActions)
  , m_actionsRun(SC_TRUE)
  , m_actions(new ScServerActions())
{
}

void ScServerImpl::Initialize()
{
  auto const & onProcessAuthorized =
      [this](ScAddr const & addr, ScAddr const & edgeAddr, ScAddr const & otherAddr) -> sc_bool {
    {
      ScServerLock guard(m_actionLock);
      m_authorizedUserProcesses.insert(otherAddr);
    }
    m_actionCond.notify_one();

    return SC_TRUE;
  };

  auto const & onProcessUnauthorized =
      [this](ScAddr const & addr, ScAddr const & edgeAddr, ScAddr const & otherAddr) -> sc_bool {
    {
      ScServerLock guard(m_actionLock);
      m_authorizedUserProcesses.erase(otherAddr);
    }
    m_actionCond.notify_one();

    return SC_TRUE;
  };

  m_authorizeUserProcessSubscription =
      new ScEventAddOutputEdge(*m_context, ScKeynodes::kAuthorizedUserProcess, onProcessAuthorized);
  m_unauthorizeUserProcessSubscription =
      new ScEventRemoveOutputEdge(*m_context, ScKeynodes::kAuthorizedUserProcess, onProcessUnauthorized);

  m_instance->set_open_handler(bind(&ScServerImpl::OnOpen, this, ::_1));
  m_instance->set_close_handler(bind(&ScServerImpl::OnClose, this, ::_1));
  m_instance->set_message_handler(bind(&ScServerImpl::OnMessage, this, ::_1, ::_2));
}

void ScServerImpl::AfterInitialize()
{
  while (m_actions->empty() == SC_FALSE)
    ;

  m_actionsRun = SC_FALSE;
  m_actionCond.notify_one();

  delete m_authorizeUserProcessSubscription;
  delete m_unauthorizeUserProcessSubscription;
}

void ScServerImpl::EmitActions()
{
  while (m_actionsRun == SC_TRUE)
  {
    ScServerUniqueLock lock(m_actionLock);

    while (m_actions->empty() && m_actionsRun)
      m_actionCond.wait(lock);

    if (m_actionsRun == SC_FALSE)
      break;

    ScServerAction * action = m_actions->front();
    m_actions->pop();

    lock.unlock();

    ScServerLock guard(m_connectionLock);

    try
    {
      action->Emit();
    }
    catch (std::exception const & e)
    {
      LogMessage(ScServerErrorLevel::error, e.what());
    }
    delete action;
  }
}

sc_bool ScServerImpl::IsWorkable()
{
  return m_actions->empty() == SC_FALSE;
}

sc_bool ScServerImpl::CheckIfUserProcessAuthorized(ScServerUserProcessId const & userProcessId)
{
  sc_bool isAuthorized;
  {
    ScServerLock guard(m_actionLock);
    ScAddr const & sessionAddr = m_connections->at(userProcessId);
    isAuthorized = m_authorizedUserProcesses.find(sessionAddr) != m_authorizedUserProcesses.cend();
  }
  m_actionCond.notify_one();

  return isAuthorized;
}

void ScServerImpl::OnOpen(ScServerUserProcessId const & userProcessId)
{
  {
    ScServerLock guard(m_actionLock);
    m_actions->push(new ScServerConnectAction(this, userProcessId));
  }
  m_actionCond.notify_one();
}

void ScServerImpl::OnClose(ScServerUserProcessId const & userProcessId)
{
  {
    ScServerLock guard(m_actionLock);
    m_actions->push(new ScServerDisconnectAction(this, userProcessId));
  }
  m_actionCond.notify_one();
}

void ScServerImpl::OnMessage(ScServerUserProcessId const & userProcessId, ScServerMessage const & msg)
{
  if (m_syncActions == SC_TRUE)
  {
    {
      ScServerLock guard(m_actionLock);
      m_actions->push(new ScServerMessageAction(this, userProcessId, msg));
    }
    m_actionCond.notify_one();
  }
  else
  {
    sc_storage_start_new_process();

    ScServerMessageAction(this, userProcessId, msg).Emit();

    sc_storage_end_new_process();
  }
}

void ScServerImpl::OnEvent(ScServerUserProcessId const & userProcessId, std::string const & msg)
{
  {
    ScServerLock guard(m_actionLock);
    m_actions->push(new ScServerEventCallbackAction(this, userProcessId, msg));
  }
  m_actionCond.notify_one();
}

ScServerImpl::~ScServerImpl()
{
  delete m_actions;
}
