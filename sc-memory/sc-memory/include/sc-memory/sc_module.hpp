/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#pragma once

#include "sc_object.hpp"
#include "sc_addr.hpp"
#include "sc_agent.hpp"

/*!
 * @interface An interface for keynodes and agent modules. It's like a complex component that contains linked agents.
 * Implement your own module class.
 * @example
 * File sc_nlp_module.hpp:
 * \code
 * #include "sc-memory/kpm/sc_module.hpp"
 *
 * #include "nlp-module/keynodes/sc_nlp_keynodes.hpp"
 * #include "nlp-module/agents/sc_syntactic_analysis_agent.hpp"
 * #include "nlp-module/agents/sc_semantic_analysis_agent.hpp"
 *
 * SC_MODULE(ScNLPModule)
 *   ->Keynodes(new nlp::ScNLPKeynodes())
 *   ->Agent({
 *      new nlp::ScSyntacticAnalysisAgent<ScEvent::Type::AddOutputEdge>(),
 *      nlp::ScNLPKeynodes::kSyntacticAnalysisAction,
 *    })
 *   ->Agent({
 *      new nlp::ScSemanticAnalysisAgent<ScEvent::Type::AddOutputEdge>(),
 *      nlp::ScNLPKeynodes::kSemanticAnalysisAction
 *    });
 * \endcode
 * File sc_nlp_module.cpp:
 * \code
 * #include "nlp-module/sc_nlp_module.hpp"
 *
 * SC_MODULE_REGISTER(SC_NLP_MODULE);
 * \endcode
 * @note Not recommended to use interface API to implement module classes. Use example defines instead of.
 */
class _SC_EXTERN ScModule : public ScObject
{
public:
  _SC_EXTERN ~ScModule() override = default;

  _SC_EXTERN std::string GetName() override
  {
    return "ScModule";
  }

  _SC_EXTERN static ScModule * Create(ScModule * module)
  {
    return module;
  }

  /*! Reminds keynodes to register it with module after.
   * @param keynodes A pointer to dynamically created keynodes instance
   * @returns Pointer to module instance
   */
  _SC_EXTERN virtual ScModule * Keynodes(ScKeynodes * keynodes) final;

  /*! Reminds agent and it initiation condition to register it with module after.
   * @param agentInfo A pointer to dynamically created agent instance and a vector of subscription addrs
   * @returns Pointer to module instance
   */
  _SC_EXTERN virtual ScModule * Agent(std::pair<ScAgentAbstract *, ScAddrVector> const & agentInfo) final;

  /*! Reminds agent and it initiation condition to register it with module after.
   * @param agentInfo A pointer to dynamically created agent instance and a subscription addr
   * @returns Pointer to module instance
   */
  _SC_EXTERN virtual ScModule * Agent(std::pair<ScAgentAbstract *, ScAddr> const & agentInfo) final;

  /*! Registers all module keynodes and agents
   * @returns Result of initializing
   */
  _SC_EXTERN sc_result Register(ScMemoryContext * ctx, ScAddr const & initMemoryGeneratedStructureAddr);

  /*! Unregisters all module keynodes and agents
   * @returns Result of shutdown
   */
  _SC_EXTERN sc_result Unregister(ScMemoryContext * ctx);

protected:
  /// Registered keynodes
  std::list<ScKeynodes *> m_keynodes;
  /// Registered agents
  std::list<std::pair<ScAgentAbstract *, ScAddrVector>> m_agents;
};

/// Implements module class and create it instance
#define SC_MODULE_BODY(__ModuleName__) \
  static ScModule * m_instance; \
\
  _SC_EXTERN std::string GetName() override \
  { \
    return #__ModuleName__; \
  }

/// Registers module class instance
#define SC_MODULE_REGISTER(__ModuleName__) \
  extern "C" \
  { \
  _SC_EXTERN sc_result \
  sc_module_initialize_with_init_memory_generated_structure(sc_addr const init_memory_generated_structure_addr) \
  { \
    __ModuleName__::m_instance->Register(ScMemory::ms_globalContext, init_memory_generated_structure_addr); \
    return SC_RESULT_OK; \
  } \
\
  _SC_EXTERN sc_result sc_module_shutdown() \
  { \
    __ModuleName__::m_instance->Unregister(ScMemory::ms_globalContext); \
    return SC_RESULT_OK; \
  } \
  } \
  ScModule * __ModuleName__::m_instance = __ModuleName__::Create(new __ModuleName__())

#define SC_MODULE_INITIALIZE(__ModuleName__) \
  sc_result __ModuleName__::Initialize(ScMemoryContext * ctx, ScAddr const & initMemoryGeneratedStructureAddr) \
  { \
    return SC_RESULT_OK; \
  } \
  sc_result __ModuleName__::Shutdown(ScMemoryContext * ctx) \
  { \
    return SC_RESULT_OK; \
  } \
  struct __ModuleName__##Dummy \
  { \
  }