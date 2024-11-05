/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_config_test.hpp"

#include <sc-config/sc_config.hpp>
#include <sc-config/sc_memory_config.hpp>

TEST_F(ScConfigTest, ValidConfig)
{
  ScConfig config{SC_CONFIG_INI};

  ScMemoryConfig memoryConfig{config, {}};

  sc_memory_params const params = memoryConfig.GetParams();
  EXPECT_EQ(params.max_loaded_segments, 1000u);
  EXPECT_EQ(params.dump_memory, SC_TRUE);
  EXPECT_EQ(params.dump_memory_period, 4u);
  EXPECT_EQ(params.dump_memory_statistics, SC_TRUE);
  EXPECT_EQ(params.dump_memory_statistics_period, 4u);
  EXPECT_EQ(std::string(params.log_type), "Console");
  EXPECT_EQ(std::string(params.log_file), "");
  EXPECT_EQ(std::string(params.log_level), "Debug");
  EXPECT_EQ(std::string(params.storage), "sc-machine-test-kb-bin");
  EXPECT_EQ(std::string(params.extensions), "bin/extensions");
}

TEST_F(ScConfigTest, RemoveSavePeriod)
{
  ScConfig config{SC_CONFIGS + "/removed-save-period.ini"};

  ScMemoryConfig memoryConfig{config, {}};

  EXPECT_THROW(memoryConfig.GetParams(), utils::ExceptionInvalidParams);
}

TEST_F(ScConfigTest, RemoveUpdatePeriod)
{
  ScConfig config{SC_CONFIGS + "/removed-update-period.ini"};

  ScMemoryConfig memoryConfig{config, {}};

  EXPECT_THROW(memoryConfig.GetParams(), utils::ExceptionInvalidParams);
}

TEST_F(ScConfigTest, DeprecatedRepoPath)
{
  ScConfig config{SC_CONFIGS + "/deprecated-repo-path.ini"};

  ScMemoryConfig memoryConfig{config, {}};

  sc_memory_params const params = memoryConfig.GetParams();
  EXPECT_EQ(params.max_loaded_segments, 1000u);
  EXPECT_EQ(params.dump_memory, SC_TRUE);
  EXPECT_EQ(params.dump_memory_period, 4u);
  EXPECT_EQ(params.dump_memory_statistics, SC_TRUE);
  EXPECT_EQ(params.dump_memory_statistics_period, 4u);
  EXPECT_EQ(std::string(params.log_type), "Console");
  EXPECT_EQ(std::string(params.log_file), "");
  EXPECT_EQ(std::string(params.log_level), "Debug");
  EXPECT_EQ(std::string(params.storage), "sc-machine-test-kb-bin");
  EXPECT_EQ(std::string(params.extensions), "bin/extensions");
}

TEST_F(ScConfigTest, DeprecatedExtensionsPath)
{
  ScConfig config{SC_CONFIGS + "/deprecated-extensions-path.ini"};

  ScMemoryConfig memoryConfig{config, {}};

  sc_memory_params const params = memoryConfig.GetParams();
  EXPECT_EQ(params.max_loaded_segments, 1000u);
  EXPECT_EQ(params.dump_memory, SC_TRUE);
  EXPECT_EQ(params.dump_memory_period, 4u);
  EXPECT_EQ(params.dump_memory_statistics, SC_TRUE);
  EXPECT_EQ(params.dump_memory_statistics_period, 4u);
  EXPECT_EQ(std::string(params.log_type), "Console");
  EXPECT_EQ(std::string(params.log_file), "");
  EXPECT_EQ(std::string(params.log_level), "Debug");
  EXPECT_EQ(std::string(params.storage), "sc-machine-test-kb-bin");
  EXPECT_EQ(std::string(params.extensions), "bin/extensions");
}
