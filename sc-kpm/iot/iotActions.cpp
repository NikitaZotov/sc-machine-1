/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#include "iotKeynodes.hpp"
#include "iotUtils.hpp"

#include <time.h>

#include "sc-memory/cpp/sc_stream.hpp"

#include "iotActions.hpp"

extern "C"
{
#include "glib.h"
}

GThread * gPeriodicalThread = 0;
GMutex gManagerLock;

struct Locker
{
  Locker()
  {
    g_mutex_lock(&gManagerLock);
  }

  ~Locker()
  {
    g_mutex_unlock(&gManagerLock);
  }
};

namespace iot
{

// thread loop
gpointer periodical_task_loop(gpointer data)
{
  ActionManager * taskManager = ActionManager::getInstance();
  while (taskManager->isRunning())
  {
    taskManager->tick();
    g_usleep(1000000);	// one second sleep
  }

  return 0;
}


/// TODO: move to define (autogenerated)
ScAddr ActionManager::msActionPeriodical;
ScAddr ActionManager::msActionTimeSpecified;

ActionManager * ActionManager::msInstance = 0;

ActionManager * ActionManager::getInstance()
{
  SC_ASSERT(msInstance, ());
  return msInstance;
}


ActionManager::ActionManager()
  : mIsInitialized(false)
  , mIsRunning(false)
{
  SC_ASSERT(msInstance == nullptr, ());
  msInstance = this;
}

ActionManager::~ActionManager()
{
  delete mMemoryCtx;
}

void ActionManager::initialize()
{
  SC_ASSERT(!mIsInitialized, ());
  mIsInitialized = true;

  mMemoryCtx = new ScMemoryContext(sc_access_lvl_make_min, "ActionManager");

  // try to find all periodical actions
  ScIterator3Ptr iterActions = mMemoryCtx->Iterator3(
        msActionPeriodical,
        ScType::EdgeAccessConstPosPerm,
        ScType::NodeConst);

  while (iterActions->Next())
    addPeriodicalAction(iterActions->Get(2));

  // try to find all time based actions
  iterActions = mMemoryCtx->Iterator3(
        msActionTimeSpecified,
        ScType::EdgeAccessConstPosPerm,
        ScType::NodeConst);
  while (iterActions->Next())
  {
    // TODO: skip finished
    addTimeSpecifiedAction(iterActions->Get(2));
  }

  // run thread
  mIsRunning = true;
  SC_ASSERT(gPeriodicalThread == 0, ());
  gPeriodicalThread = g_thread_new("PeriodicalTaskThread", periodical_task_loop, 0);
}

void ActionManager::shutdown()
{
  mIsRunning = false;
  g_thread_join(gPeriodicalThread);
  gPeriodicalThread = 0;

  delete mMemoryCtx;
  mMemoryCtx = 0;

  mIsInitialized = false;
}

void ActionManager::addPeriodicalAction(ScAddr const & actionAddr)
{
  ScTemplate periodTempl;

  periodTempl
      .TripleWithRelation(
        actionAddr >> "action",
        ScType(sc_type_arc_common | sc_type_var),
        ScType(sc_type_node | sc_type_var) >> "value",
        ScType(sc_type_arc_pos_var_perm),
        Keynodes::nrel_period)
      .TripleWithRelation(
        "value",
        ScType(sc_type_arc_pos_var_perm),
        ScType(sc_type_link | sc_type_var) >> "link",
        ScType(sc_type_arc_pos_var_perm),
        Keynodes::rrel_seconds)
      .Triple(
        Keynodes::binary_int32,
        ScType(sc_type_arc_pos_var_perm),
        "link");

  ScTemplateSearchResult searchResult;

  if (mMemoryCtx->HelperSearchTemplate(periodTempl, searchResult))
  {
    SC_ASSERT(searchResult.Size() > 0, ());
    ScTemplateSearchResultItem const item = searchResult[0];

    int32_t period = 0;
    ScStream stream;

    if (mMemoryCtx->GetLinkContent(item["link"], stream) && (stream.Size() == sizeof(period)))
    {
      stream.ReadType(period);
      appendActionPeriodical(item["action"], period);
    }
  }
}

void ActionManager::addTimeSpecifiedAction(ScAddr const & actionAddr)
{
  ScTemplate timeTempl;

  timeTempl
      .TripleWithRelation(
        actionAddr >> "action",
        ScType(sc_type_arc_common | sc_type_var),
        ScType(sc_type_node | sc_type_var) >> "value",
        ScType(sc_type_arc_pos_var_perm),
        Keynodes::nrel_time)
      .TripleWithRelation(
        "value",
        ScType(sc_type_arc_pos_var_perm),
        ScType(sc_type_link | sc_type_var) >> "link",
        ScType(sc_type_arc_pos_var_perm),
        Keynodes::rrel_epoch)
      .Triple(
        Keynodes::binary_int64,
        ScType(sc_type_arc_pos_var_perm),
        "link"
        );

  ScTemplateSearchResult searchResult;

  if (mMemoryCtx->HelperSearchTemplate(timeTempl, searchResult))
  {
    SC_ASSERT(searchResult.Size() > 0, ());
    ScTemplateSearchResultItem const item = searchResult[0];

    int64_t time = 0;
    ScStream stream;

    if (mMemoryCtx->GetLinkContent(item["link"], stream) && (stream.Size() == sizeof(time)))
    {
      stream.ReadType(time);
      appendAction(item["action"], time);
    }
  }
}

void ActionManager::appendActionPeriodical(ScAddr const & action, uint32_t period)
{
  time_t tm;
  time(&tm);

  Locker lock;
  mTaskSet.insert(Task(action, period, tm + period));
}

void ActionManager::appendAction(ScAddr const & action, uint64_t runTime)
{
  Locker lock;
  mTaskSet.insert(Task(action, 0, runTime));
}

void ActionManager::tick()
{
  time_t tm;
  time(&tm);

  Locker lock;
  tTaskSet::iterator it = mTaskSet.begin();
  while (it != mTaskSet.end())
  {
    Task task = *it;

    if (task.nextRunTime <= (uint64_t)tm)
    {
      // run action
      ScTemplate initTempl;

      initTempl.Triple(
            Keynodes::command_initiated,
            ScType(sc_type_arc_pos_const_perm),
            task.action);

      ScTemplateSearchResult searchResult;
      if (!mMemoryCtx->HelperSearchTemplate(initTempl, searchResult))
      {
        ScTemplateGenResult genResult;
        mMemoryCtx->HelperGenTemplate(initTempl, genResult);

        mTaskSet.erase(*it);

        // re-insert periodical task
        if (task.period > 0)
        {
          task.nextRunTime += task.period;
          mTaskSet.insert(task);
        }

        it = mTaskSet.begin();
      }
    }
    else
    {
      break;
    }

    // didn't erase first element
    ++it;
  }
}

bool ActionManager::isRunning() const
{
  return mIsRunning;
}

// ---------------

SC_AGENT_IMPLEMENTATION(ANewPeriodicalActionAgent)
{
  return SC_RESULT_ERROR;
}

} // namespace iot
