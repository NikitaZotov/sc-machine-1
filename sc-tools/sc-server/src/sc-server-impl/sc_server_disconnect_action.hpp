/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#pragma once

#include <utility>

#include "sc_server_action.hpp"
#include "sc_server.hpp"

class ScServerDisconnectAction : public ScServerAction
{
public:
  ScServerDisconnectAction(ScServer * server, ScServerConnectionHandle hdl)
    : ScServerAction(std::move(hdl))
    , m_server(server)
  {
  }

  void Emit() override
  {
    ScAddr const & sessionAddr = m_server->GetConnections()->at(m_hdl);
    m_server->m_context->EraseElement(sessionAddr);
    m_server->GetConnections()->erase(m_hdl);
  }

  ~ScServerDisconnectAction() override = default;

protected:
  ScServer * m_server;
};
