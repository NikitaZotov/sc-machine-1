# **Implementing agent in C++**

!!! note
    This is correct for only versions of sc-machine that >= 0.10.0.
--- 

All agents in C++ represent some classes in C++. To implement an agent in C++, you need to perform the following common steps:

1. Write input (initial) construction and output (result) construction of your future agent in SC-code.
2. Generate folder with source and header files for sc-agent implementation.
3. Also you need write `CMakeLists.txt` file. We use cmake to build projects in C++.
4. In header file, define a class in C++ for this agent and specifies in it at least class of actions that this agent performs and its program. In such class you can also specify primary initiation condition, initiation condition, and result condition.
5. In source file, implement all declared methods of agent's class. You can also implement your own methods and use them in an agent program. You can use all C++ and OOP tools as much as possible. 
6. Generate file and implement class for keynodes used by implemented agent.
7. Implement class for module for subscribing implemented agent.
8. Write tests for implemented agent.

---

Let's consider an example of implementing an agent to count power of the given set:

### **1. Specify inputs and outputs of your future agent.**

The initial construction of agent might look like this:

```scs
..action
<- action_calculate_set_power;
<- action_initiated;
-> rrel_1: ..some_set;;

..some_set
-> ..element_1;
-> ..element_2;
-> ..element_3;;
```

<image src="../images/agents/agent_calculate_set_power_input_construction.png"></image>

The result construction of agent might look like this:

```scs
..some_action
=> nrel_result: [*
  ..some_set => nrel_set_power: [3];;
*];;
```

<image src="../images/agents/agent_calculate_set_power_output_construction.png"></image>

In addition to agents that initiate actions themselves and then perform these actions, there is a need to implement agents that perform actions initiated by other agents. For this class of agents, it is much easier to generate a initial initiation construction in the knowledge base. See [**C++ Actions API**](../extended/agents/actions.md) to learn more about actions.

--- 

### **2. Generate folder with source and header files for agent and `CMakeLists.txt.` file.**

You should get something like this structure:

```plaintext
set-agents-module/
├── CMakeLists.txt
├── agent/
│   ├── sc_agent_calculate_set_power.hpp
│   └── sc_agent_calculate_set_power.сpp
```

---

### **3. Write `CMakeLists.txt` file.**
  
The `CMakeLists.txt` file should describe a process of building your agent code. It should contain various instructions and parameters needed to compile and link agent source code to sc-machine libraries.

**CMakeLists.txt**

```cmake
file(GLOB SOURCES CONFIGURE_DEPENDS
    "*.cpp" "*.hpp"
    "agent/*.cpp" "agent/*.hpp"
)

# Generate and link your library with using libraries.
add_library(set-agents SHARED ${SOURCES})
target_link_libraries(set-agents LINK_PUBLIC sc-memory)
target_include_directories(set-agents PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Set output path for your extension `set-agents.so`. 
# Make sure that variable `SC_EXTENSIONS_DIRECTORY` has value.
set_target_properties(set-agents 
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${SC_EXTENSIONS_DIRECTORY}
)
```

---

### **4. Define a class in C++ for this agent and specifies class of actions that this agent performs and its program.**

**sc_agent_calculate_set_power.hpp**

```cpp
#pragma once

#include <sc-memory/sc_agent.hpp>

class ScAgentCalculateSetPower : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
```

An agent's class to be implemented must comply with the following requirements:

* It must inherit one of the common classes for implementing agents: 
  
  - `template <class TScEvent> class ScAgent`,
  - or `class ScActionInitiatedAgent`.

The base class `ScAgent` contains API to implement agents that react to any sc-events. The base class `ScActionInitiatedAgent` inherits base class `ScAgent` and provides API to implement agents that react to sc-events of initiating sc-action.

* It must override at least methods `ScAddr GetAction() const` and `ScResult DoProgram(ScActionInitiatedEvent const & event, ScAction & action)`.

* Override methods must be public. Otherwise, you won't be able to build your code because the sc-machine won't be able to call methods on your agent class.

* You can implement other methods in agent's class.

To learn more about opportunities and restrictions for implementing agents, see [**C++ Agents API**](../extended/agents/agents.md).

---

### **5. Implement all declared methods of agent's class.**

**sc_agent_calculate_set_power.cpp**

```cpp
#include "sc_agent_calculate_set_power.hpp"

#include <sc-memory/sc_memory_headers.hpp>

ScAddr ScAgentCalculateSetPower::GetActionClass() const
{
  return m_context.SearchElementBySystemIdentifier("action_calculate_set_power");
  // You have to make sure that this class is in the knowledge base.
}
// You must specify valid action class. In other case, the agent can’t be 
// subscribed to sc-event.

ScResult ScAgentCalculateSetPower::DoProgram(ScAction & action)
{
  // `ScAction` class encapsulates information about sc-action. 
  // The provided action is action that the given agent performs right now. 
  // It belongs to class action_calculate_set_power`. 
  // Actions are copyable and movable. ScAction is inherited from ScAddr.

  auto const & [setAddr] = action.GetArguments<1>(); 
  // This method finds construction `action -> rrel_1: setAddr`.
  // Here the 1 is number of arguments which action must have. In step 1, 
  // we specified that an action should have a set as its the first and 
  // only one argument. But the one who calls this agent may not specify 
  // argument for the action. So we need to check that the action has argument. 
  if (!m_context.IsElement(setAddr))
  {
    SC_AGENT_LOG_ERROR("Action does not have argument."); 
    // output: "ScAgentCalculateSetPower: Action does not have argument."
    return action.FinishWithError();
  }
  // There may be a situation where someone is trying to specify a number of 
  // arguments more then it is needed. You can also check this by specifying, 
  // for example, number 2 instead of number 1. But you don't always need 
  // to do this.

  // To calculate power of the set, we can traverse all accessory constant 
  // positive permanents arcs from the set and count number of these arcs. 
  // But, in any problem, the presence of NON-factors must be considered, 
  // but this is omitted here.
  size_t setPower = 0;
  ScIterator3Ptr const it3 = m_context.CreateIterator3( 
    setAddr,
    ScType::ConstPermPosArc,
    ScType::ConstNode
  );
  while (it3->Next())
    ++setPower;

  ScAddr const & setPowerAddr = m_context.GenerateLink(ScType::ConstNodeLink);
  m_context.SetLinkContent(setPowerAddr, setPower);
  ScAddr const & arcCommonAddr 
    = m_context.GenerateConnector(ScType::ConstCommonArc, setAddr, setPowerAddr);
  ScAddr const & nrelSetPowerAddr 
    = m_context.SearchElementBySystemIdentifier("nrel_set_power");
  // You have to make sure that this non-role relation is in the knowledge base.
  ScAddr const & arcAccessAddr = m_context.GenerateConnector(
    ScType::ConstPermPosArc, nrelSetPowerAddr, arcCommonAddr);

  action.FormResult(
    setAddr, arcCommonAddr, setPowerAddr, arcAccessAddr, nrelSetPowerAddr);
  SC_AGENT_LOG_DEBUG("Set power was counted: " << setPower << ".");

  // At the end of the agent's program, you must call one of three methods 
  // (`FinishSuccessfully`, `FinishUnsuccessfully`, `FinishWithError`) 
  // to indicate that the agent's performing of action is complete:
  // - Method `FinishSuccessfully` indicates that action was performed 
  // by agent successfully (sets class `action_finished_successfully`). 
  // It means that the agent solved specified problem.
  // - Method `FinishUnsuccessfully` indicates that action was performed 
  // by agent unsuccessfully (sets class `action_finished_unsuccessfully`). 
  // It means that the agent didn't solved specified problem.
  // - Method `FinishWithError` indicates that action was performed by agent
  // with error (sets class `action_finished_with_error`). 
  // It means that some incorrect situation was occurred in knowledge base.
  // All these methods return objects of `ScResult` class. 
  // You can’t generate object of ScResult via constructor, because it is private.
  return action.FinishSuccessfully();
}
```

---

### **6. Define keynodes for implemented agent and integrate their in agent program.**

For each agent, you can specify key sc-elements that this agent uses during the execution of its program. These key sc-elements are sc-elements that agent does not generate, but uses in the process of searching for or creating connections between entities in knowledge base. Key sc-elements are named keynodes. You can find these keynodes by its system identifiers (method `SearchElementBySystemIdentifier`) if they have such identifiers. Also, you can use class `ScKeynode` to define keynodes as static objects and use them in agents.

```diff
set-agents-module/
 ├── CMakeLists.txt
 ├── agent/
 │   ├── sc_agent_calculate_set_power.hpp
 │   └── sc_agent_calculate_set_power.сpp
+├── keynodes/
+│   └── sc_set_keynodes.hpp
```

**CMakeLists.txt**

```diff
file(GLOB SOURCES CONFIGURE_DEPENDS
    "*.cpp" "*.hpp"
    "agent/*.cpp" "agent/*.hpp"
+   "keynodes/*.hpp"
)
```

**sc_set_keynodes.hpp**

```cpp
#include <sc-memory/sc_keynodes.hpp>

// This class unites keynodes that used by agents of one module 
// (with one sense). You can implement hierarchies of keynodes.
class ScSetKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const action_calculate_set_power{
    "action_calculate_set_power", ScType::ConstNodeClass};
  static inline ScKeynode const nrel_set_power{
    "nrel_set_power", ScType::ConstNodeNonRole};
  // Here the first argument in constructor is system identifier of 
  // sc-keynode and the second argument is sc-type of this sc-keynode.
  // If there is no sc-keynode with such system identifier in 
  // knowledge base, then the one with specified sc-type will be generated.
  // You don't should specify type of sc-keynode here, be default it is 
  // `ScType::ConstNode`. But you sure that your code will use this 
  // keynode with type `ScType::ConstNode` correctly.
};
```

!!! warning
    You can't specify sc-keynode with empty system identifier. It can be invalid.

!!! warning
    All keynodes must be static objects. You can define keynodes as static objects everywhere (not only in classes).

`ScKeynodes` class is base class for all classes with keynodes. It contains core keynodes, that can be used in each agent. See [**C++ Keynodes API**](../extended/agents/keynodes.md) to learn more about keynodes.

**sc_agent_calculate_set_power.cpp**

```diff
#include "sc_agent_calculate_set_power.hpp"

#include <sc-memory/sc_memory_headers.hpp>

+ #include "keynodes/sc_set_keynodes.hpp"

ScAddr ScAgentCalculateSetPower::GetActionClass() const
{
- return m_context.SearchElementBySystemIdentifier("action_calculate_set_power");
+ return ScSetKeynodes::action_calculate_set_power;
}

ScResult ScAgentCalculateSetPower::DoProgram(ScAction & action)
{
  ...

- ScAddr const & nrelSetPowerAddr 
-   = m_context.SearchElementBySystemIdentifier("nrel_set_power");
- ScAddr const & arcAccessAddr = m_context.GenerateConnector(
-   ScType::ConstPermPosArc, nrelSetPowerAddr, arcCommonAddr);
+ ScAddr const & arcAccessAddr = m_context.GenerateConnector(
+   ScType::ConstPermPosArc, 
+   ScSetKeynodes::nrel_set_power, 
+   arcCommonAddr);
- action.FormResult(
-   setAddr, arcCommonAddr, setPowerAddr, arcAccessAddr, nrelSetPowerAddr);
+ action.FormResult(
+   setAddr, 
+   arcCommonAddr,
+   setPowerAddr,
+   arcAccessAddr, 
+   ScSetKeynodes::nrel_set_power);
  ...
}
```

---

### **7. Implement module class to subscribe your agent to specified sc-event.**

Someone should subscribe your agent to event. It can be other agent, or any code at all. You can implement class, that allows subscribing agents. This class is named sc-module. Each sc-module should subscribe agents with common sense.

```diff
 set-agents-module/
 ├── CMakeLists.txt
 ├── agent/
 │   ├── sc_agent_calculate_set_power.hpp
 │   └── sc_agent_calculate_set_power.сpp
 ├── keynodes/
 │   └── sc_set_keynodes.hpp
+├── sc_set_module.hpp
+└── sc_set_module.cpp
```

**sc_set_module.hpp**

```cpp
#pragma once

#include <sc-memory/sc_module.hpp>

class ScSetModule : public ScModule
{
  // Here class is empty. You doesn't need to implement any methods. 
  // `ScModule` class contains all necessary API to subscribe your 
  // agents as separate sc-module.
};
```

**sc_set_module.cpp**

```cpp
#include "sc_set_module.hpp"

#include "agent/sc_agent_calculate_set_power.hpp"

SC_MODULE_REGISTER(ScSetModule)
  ->Agent<ScAgentCalculateSetPower>();
  // This method pointers to module that agent class `ScAgentCalculateSetPower`
  // should be subscribed to sc-event of adding outgoing sc-arc from sc-element
  // `action_initiated`. It is default parameter in these method if you want to
  // subscribe agent class inherited from `ScActionInitiatedAgent`.

// This way of subscribing agents makes it easier to write code. 
// You don't have to think about unsubscribing agents after 
// the system shutdown - your module will do it all by itself.
```

If you want to initialize something else in your module besides agents, you can override methods `Initialize(ScMemoryContext * context) override;` and `Shutdown(ScMemoryContext * context) override;`.

**sc_set_module.hpp**

```diff
class ScSetModule : public ScModule
{
+ void Initialize(ScMemoryContext * context) override;
+ void Shutdown(ScMemoryContext * context) override;
};
```

**sc_set_module.cpp**

```diff
SC_MODULE_REGISTER(ScSetModule)
  ->Agent<ScAgentCalculateSetPower>();

+ // This method will be called once. 
+ void ScSetModule::Initialize(ScMemoryContext * context)
+ {
+   // Implement initialize of your objects here.
+ }
+ // This method will be called once. 
+ void ScSetModule::Shutdown(ScMemoryContext * context)
+ {
+   // Implement shutdown of your objects here.
+ }
```

You can find all modules functionality in the [**C++ Modules API**](../extended/agents/modules.md).

### **8. Write tests for implemented agent. Check agent logic.**

To make sure how your agent works it is best to generate tests and cover in them all possible cases that your agent has to handle. For this, generate a separate file with test cases and implement them.

```diff
 set-agents-module/
 ├── CMakeLists.txt
 ├── agent/
 │   ├── sc_agent_calculate_set_power.hpp
 │   └── sc_agent_calculate_set_power.сpp
 ├── keynodes/
 │   └── sc_set_keynodes.hpp
+├── tests/
+│   └── test_sc_agent_calculate_set_power.cpp
 ├── sc_set_module.hpp
 └── sc_set_module.cpp
```

**CMakeLists.txt**

```diff
file(GLOB SOURCES CONFIGURE_DEPENDS
    "*.cpp" "*.hpp"
    "agent/*.cpp" "agent/*.hpp"
    "keynodes/*.hpp"
)

add_library(set-agents SHARED ${SOURCES})
target_link_libraries(set-agents LINK_PUBLIC sc-machine::sc-memory)
target_include_directories(set-agents PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(set-agents 
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${SC_EXTENSIONS_DIRECTORY}
)

+file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
+    "tests/*.cpp"
+)

# Generate executable for tests.
+add_executable(set-agents-tests ${TEST_SOURCES})
+target_link_libraries(set-agents-tests LINK_PRIVATE set-agents)
+target_include_directories(set-agents-tests 
+    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
+)

+gtest_discover_tests(set-agents-tests
+    TEST_LIST ${TEST_SOURCES}
+    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
+)
```

**test_sc_agent_calculate_set_power.cpp**

```cpp
#include <sc-memory/test/sc_test.hpp>

#include <sc-memory/sc_memory_headers.hpp>

#include "agent/sc_agent_calculate_set_power.hpp"
#include "keynodes/sc_set_keynodes.hpp"

using AgentTest = ScMemoryTest;

TEST_F(AgentTest, AgentCalculateSetPowerFinishedSuccessfully)
{
  // Generate action with class that your agent performs.
  ScAction action 
    = m_ctx->GenerateAction(ScSetKeynodes::action_calculate_set_power);

  // Generate set with two sc-elements.
  ScSet set = m_ctx->GenerateSet();
  ScAddr nodeAddr1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr nodeAddr2 = m_ctx->GenerateNode(ScType::ConstNode);
  set << nodeAddr1 << nodeAddr2;

  // Set generated set as argument for action.
  action.SetArgument(1, set);

  // Initiate and wait while action will be finished.
  action.InitiateAndWait();

  // Check that action is finished successfully.
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  // Get action result structure.
  ScStructure structure = action.GetResult();
  // Check that it has sc-elements.
  EXPECT_FALSE(structure.IsEmpty());

  // Check sc-constructions in result structure.
  // Check the first three element construction.
  ScIterator3Ptr it3 = m_ctx->CreateIterator3(
    structure, ScType::ConstPermPosArc, ScType::ConstCommonArc);
  EXPECT_TRUE(it3->Next());
  ScAddr arcAddr = it3->Get(2);

  ScAddr beginAddr;
  ScAddr linkAddr;
  m_ctx->GetConnectorIncidentElements(arcAddr, beginAddr, linkAddr);
  EXPECT_EQ(beginAddr, set);
  EXPECT_TRUE(m_ctx->GetElementType(linkAddr).IsLink());

  // Check that link content is 2.
  size_t setPower;
  EXPECT_TRUE(m_ctx->GetLinkContent(linkAddr, setPower));
  EXPECT_EQ(setPower, 2u);

  // Check the second three element construction.
  it3 = m_ctx->CreateIterator3(
    structure, ScType::ConstPermPosArc, ScType::ConstPermPosArc);
  EXPECT_TRUE(it3->Next());
  ScAddr arcAddr2 = it3->Get(2);

  ScAddr relationAddr;
  ScAddr targetArcAddr;
  m_ctx->GetConnectorIncidentElements(arcAddr2, relationAddr, targetArcAddr);
  EXPECT_EQ(relationAddr, ScSetKeynodes::nrel_set_power);
  EXPECT_EQ(targetArcAddr, arcAddr);
}

// Provide tests for unsuccessful and error situations.
...
```

!!! note
    A good code is a code covered by tests.

`ScMemoryTest` class includes `m_ctx` that is object of `ScAgentContext` class. You can use it to work with sc-memory. See [**C++ Core API**](../core/api.md) and [**C++ Agent context API**](../extended/agents/agents.md) to learn more about available methods for working with sc-memory.

By default, the sc-machine repository contains configuration for launching tests in vscode. If you're developing on vscode, you can use this configuration and extension [C++ TestMate](https://marketplace.visualstudio.com/items?itemName=matepek.vscode-catch2-test-adapter) for debugging code.

--- 

## **Frequently Asked Questions**

<!-- no toc -->
- [What is the correct way to write CMakeLists.txt? What is it? How to work with it?](#what-is-the-correct-way-to-write-cmakeliststxt-what-is-it-how-to-work-with-it)
- [How to write tests correctly?](#how-to-write-tests-correctly)

### **What is the correct way to write CMakeLists.txt? What is it? How to work with it?**

CMake is a widely used build system that facilitates the management of software builds, particularly for C++ projects. It allows developers to define the build process in a platform-independent manner, which can then be used to generate native build scripts for different environments. 

Use [this guide](https://cliutils.gitlab.io/modern-cmake/) to write CMakeLists.txt correctly.

### **How to write tests correctly?**

We use GoogleTest to test our code. GoogleTest, often referred to as gtest, is a C++ testing framework developed by Google. It is used for writing and running unit tests in C++ projects. 

Use [this guide](https://google.github.io/googletest/) to write good tests.
