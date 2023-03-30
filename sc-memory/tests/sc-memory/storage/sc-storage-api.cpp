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
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, SC_TRUE), SC_RESULT_OK);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_RESULT_OK);
}

TEST(ScDictionaryFSMemoryTest, sc_storage_init_shutdown_no_path)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, nullptr, 0, SC_TRUE), SC_RESULT_ERROR_IO);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_RESULT_NO);
}

TEST(ScDictionaryFSMemoryTest, sc_storage_init_shutdown_no_exist_path)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, "", 0, SC_TRUE), SC_RESULT_ERROR_IO);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_RESULT_NO);
}

TEST(ScStorageTest, sc_storage_add_elements)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, SC_TRUE), SC_RESULT_OK);

  sc_addr const class_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const | sc_type_node_class);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(class_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, class_addr));

  sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(node_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, node_addr));

  sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(arc_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_RESULT_OK);
}

TEST(ScStorageTest, sc_storage_add_elements_get)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, SC_TRUE), SC_RESULT_OK);

  sc_addr const class_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const | sc_type_node_class);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(class_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, class_addr));
  sc_type type;
  EXPECT_EQ(sc_storage_get_element_type(storage, class_addr, &type), SC_RESULT_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const | sc_type_node_class);

  sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(node_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, node_addr, &type), SC_RESULT_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const);

  sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(arc_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, arc_addr, &type), SC_RESULT_OK);
  EXPECT_EQ(type, sc_type_arc_pos_const_perm);
  sc_addr beg_addr, end_addr;
  EXPECT_EQ(sc_storage_get_arc_info(storage, arc_addr, &beg_addr, &end_addr), SC_RESULT_OK);
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(beg_addr, class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(end_addr, node_addr));

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_RESULT_OK);
}

TEST(ScStorageTest, sc_storage_add_elements_get_by_iterator)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, SC_TRUE), SC_RESULT_OK);

  sc_addr const class_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const | sc_type_node_class);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(class_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, class_addr));
  sc_type type;
  EXPECT_EQ(sc_storage_get_element_type(storage, class_addr, &type), SC_RESULT_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const | sc_type_node_class);

  sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(node_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, node_addr, &type), SC_RESULT_OK);
  EXPECT_EQ(type, sc_type_node | sc_type_const);

  sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
  EXPECT_FALSE(SC_ADDR_IS_EMPTY(arc_addr));
  EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
  EXPECT_EQ(sc_storage_get_element_type(storage, arc_addr, &type), SC_RESULT_OK);
  EXPECT_EQ(type, sc_type_arc_pos_const_perm);
  sc_addr beg_addr, end_addr;
  EXPECT_EQ(sc_storage_get_arc_info(storage, arc_addr, &beg_addr, &end_addr), SC_RESULT_OK);
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(beg_addr, class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(end_addr, node_addr));

  sc_iterator3 * it = sc_iterator3_a_a_f_new_ext(storage, sc_type_node | sc_type_const | sc_type_node_class, sc_type_arc_pos_const_perm, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_a_a_f_new_ext(storage, sc_type_node | sc_type_const, sc_type_arc_pos_const_perm, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_a_a_f_new_ext(storage, sc_type_node | sc_type_const, 0, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_a_a_f_new_ext(storage, sc_type_node | sc_type_const, sc_type_arc_access, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_a_a_f_new_ext(storage, sc_type_node | sc_type_const, sc_type_const, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_f_a_a_new_ext(storage, class_addr, sc_type_arc_pos_const_perm, sc_type_node | sc_type_const);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_f_a_a_new_ext(storage, class_addr, sc_type_arc_pos_const_perm, sc_type_node);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_f_a_a_new_ext(storage, class_addr, 0, sc_type_node | sc_type_const);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_f_a_a_new_ext(storage, class_addr, sc_type_arc_access, sc_type_node | sc_type_const);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_f_a_a_new_ext(storage, class_addr, sc_type_const, sc_type_node | sc_type_const);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  it = sc_iterator3_f_a_f_new_ext(storage, class_addr, sc_type_arc_pos_const_perm, node_addr);
  EXPECT_TRUE(sc_iterator3_next(it));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 0), class_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 1), arc_addr));
  EXPECT_TRUE(SC_ADDR_IS_EQUAL(sc_iterator3_value(it, 2), node_addr));
  EXPECT_FALSE(sc_iterator3_next(it));
  sc_iterator3_free(it);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_RESULT_OK);
}

TEST(ScStorageTest, sc_storage_add_elements_multiple_iterator)
{
  sc_storage * storage;
  EXPECT_EQ(sc_storage_initialize(&storage, SC_DICTIONARY_FS_MEMORY_PATH, 0, SC_TRUE), SC_RESULT_OK);

  sc_addr const class_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const | sc_type_node_class);

  sc_uint16 arcs_access_count_1 = 128;
  for (sc_uint16 i = 0; i < arcs_access_count_1; ++i)
  {
    sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
    EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
    sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
    EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
  }

  sc_uint16 j = 0;
  sc_iterator3 * it = sc_iterator3_f_a_a_new_ext(storage, class_addr, sc_type_arc_pos_const_perm, sc_type_node | sc_type_const);;
  while (sc_iterator3_next(it))
  {
    sc_addr const arc_addr = sc_iterator3_value(it, 1);
    EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
    sc_addr const node_addr = sc_iterator3_value(it, 2);
    EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
    ++j;
  }
  sc_iterator3_free(it);
  EXPECT_EQ(j, arcs_access_count_1);

  sc_uint16 arcs_common_count = 512;
  for (sc_uint16 i = 0; i < arcs_common_count; ++i)
  {
    sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
    EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
    sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_common | sc_type_const, class_addr, node_addr);
    EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
  }

  sc_uint16 arcs_access_count_2 = 512;
  for (sc_uint16 i = 0; i < arcs_access_count_2; ++i)
  {
    sc_addr const node_addr = sc_storage_node_new(storage, sc_type_node | sc_type_const);
    EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
    sc_addr arc_addr = sc_storage_connector_new(storage, sc_type_arc_pos_const_perm, class_addr, node_addr);
    EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
  }

  j = 0;
  it = sc_iterator3_f_a_a_new_ext(storage, class_addr, sc_type_arc_pos_const_perm, sc_type_node | sc_type_const);;
  while (sc_iterator3_next(it))
  {
    sc_addr const arc_addr = sc_iterator3_value(it, 1);
    EXPECT_TRUE(sc_storage_is_element(storage, arc_addr));
    sc_addr const node_addr = sc_iterator3_value(it, 2);
    EXPECT_TRUE(sc_storage_is_element(storage, node_addr));
    ++j;
  }
  sc_iterator3_free(it);
  EXPECT_EQ(j, arcs_access_count_1 + arcs_access_count_2);

  EXPECT_EQ(sc_storage_shutdown(storage, SC_TRUE), SC_RESULT_OK);
}
