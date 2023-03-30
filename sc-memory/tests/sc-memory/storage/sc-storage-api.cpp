#include <gtest/gtest.h>

extern "C"
{
#include "sc-core/sc-store/sc_storage.h"
#include "sc-core/sc-store/sc-base/sc_allocator.h"
#include "sc-core/sc-store/sc_iterator3.h"
}

#define SC_DICTIONARY_FS_MEMORY_PATH "storage"

TEST(ScStorageTest, sc_storage_init_shutdown)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, 1), SC_STORAGE_OK);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_STORAGE_OK);
}

TEST(ScDictionaryFSMemoryTest, sc_storage_init_shutdown_no_path)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, nullptr, 0, SC_TRUE), SC_STORAGE_WRONG_PATH);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_STORAGE_NO);
}

TEST(ScDictionaryFSMemoryTest, sc_storage_init_shutdown_no_exist_path)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, "", 0, SC_TRUE), SC_STORAGE_WRONG_PATH);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_STORAGE_NO);
}

TEST(ScStorageTest, sc_storage_add_elements)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, 1), SC_STORAGE_OK);

  sc_addr const class_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const | sc_type_node_class);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(class_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, class_addr));

  sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(node_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, node_addr));

  sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(arc_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_STORAGE_OK);
}

TEST(ScStorageTest, sc_storage_add_elements_get)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, 1), SC_STORAGE_OK);

  sc_addr const class_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const | sc_type_node_class);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(class_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, class_addr));
  sc_type type;
  EXPECT_EQ(sc_storage_get_element_type(storage, class_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const | sc_type_node_class);

  sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(node_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, node_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const);

  sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(arc_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, arc_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_arc_pos_const_perm);
  sc_addr beg_addr, end_addr;
  EXPECT_EQ(sc_storage_get_arc_info(storage, arc_addr, &beg_addr, &end_addr), SC_STORAGE_OK);
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(beg_addr, class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(end_addr, node_addr));

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_STORAGE_OK);
}

TEST(ScStorageTest, sc_storage_add_elements_get_by_iterator)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, 1), SC_STORAGE_OK);

  sc_addr const class_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const | sc_type_node_class);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(class_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, class_addr));
  sc_type type;
  EXPECT_EQ(sc_storage_get_element_type(storage, class_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const | sc_type_node_class);

  sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(node_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, node_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const);

  sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(arc_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, arc_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_arc_pos_const_perm);
  sc_addr beg_addr, end_addr;
  EXPECT_EQ(sc_storage_get_arc_info(storage, arc_addr, &beg_addr, &end_addr), SC_STORAGE_OK);
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(beg_addr, class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(end_addr, node_addr));

  sc_iterator3 * it = sc_iterator3_a_a_f_new(storage, sc_type_node | sc_type_const | sc_type_node_class, sc_type_arc_pos_const_perm, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_a_a_f_new(storage, sc_type_node | sc_type_const, sc_type_arc_pos_const_perm, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_STORAGE_OK);
}

TEST(ScStorageTest, sc_storage_add_elements_multiple_iterator)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, 1), SC_STORAGE_OK);

  sc_addr const class_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const | sc_type_node_class);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(class_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, class_addr));
  sc_type type;
  EXPECT_EQ(sc_storage_get_element_type(storage, class_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const | sc_type_node_class);

  sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(node_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, node_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const);

  sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(arc_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, arc_addr, &type), SC_STORAGE_OK);
  EXPECT_EQ(type, sc_type_arc_pos_const_perm);
  sc_addr beg_addr, end_addr;
  EXPECT_EQ(sc_storage_get_arc_info(storage, arc_addr, &beg_addr, &end_addr), SC_STORAGE_OK);
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(beg_addr, class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(end_addr, node_addr));

  sc_iterator3 * it = sc_iterator3_a_a_f_new(storage, sc_type_node | sc_type_const | sc_type_node_class, sc_type_arc_pos_const_perm, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_STORAGE_OK);
}
