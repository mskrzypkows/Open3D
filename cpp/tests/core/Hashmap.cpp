// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "open3d/core/hashmap/Hashmap.h"

#include <unordered_map>

#include "open3d/core/Device.h"
#include "open3d/core/Indexer.h"
#include "open3d/core/MemoryManager.h"
#include "open3d/core/SizeVector.h"
#include "tests/UnitTest.h"
#include "tests/core/CoreTest.h"

namespace open3d {
namespace tests {

class HashmapPermuteDevices : public PermuteDevices {};
INSTANTIATE_TEST_SUITE_P(Hashmap,
                         HashmapPermuteDevices,
                         testing::ValuesIn(PermuteDevices::TestCases()));

TEST_P(HashmapPermuteDevices, Init) {
    core::Device device = GetParam();

    int n = 5;
    std::vector<int> keys_val = {100, 300, 500, 700, 900};
    std::vector<int> values_val = {1, 3, 5, 7, 9};

    core::Tensor keys(keys_val, {5}, core::Dtype::Int32, device);
    core::Tensor values(values_val, {5}, core::Dtype::Int32, device);

    int init_capacity = n * 2;
    core::Hashmap hashmap(init_capacity, core::Dtype::Int32, core::Dtype::Int32,
                          device);

    core::Tensor masks({n}, core::Dtype::Bool, device);
    core::Tensor addrs({n}, core::Dtype::Int32, device);
    hashmap.Insert(keys.GetDataPtr(), values.GetDataPtr(),
                   static_cast<core::addr_t *>(addrs.GetDataPtr()),
                   static_cast<bool *>(masks.GetDataPtr()), n);
    EXPECT_TRUE(masks.All());
    EXPECT_EQ(hashmap.Size(), 5);
}

TEST_P(HashmapPermuteDevices, Find) {
    core::Device device = GetParam();

    int n = 5;
    std::vector<int> keys_val = {100, 300, 500, 700, 900};
    std::vector<int> values_val = {1, 3, 5, 7, 9};

    core::Tensor keys(keys_val, {n}, core::Dtype::Int32, device);
    core::Tensor values(values_val, {n}, core::Dtype::Int32, device);

    int init_capacity = n * 2;
    core::Hashmap hashmap(init_capacity, core::Dtype::Int32, core::Dtype::Int32,
                          device);

    core::Tensor masks({n}, core::Dtype::Bool, device);
    core::Tensor addrs({n}, core::Dtype::Int32, device);

    hashmap.Insert(keys.GetDataPtr(), values.GetDataPtr(),
                   static_cast<core::addr_t *>(addrs.GetDataPtr()),
                   static_cast<bool *>(masks.GetDataPtr()), n);
    hashmap.Find(keys.GetDataPtr(),
                 static_cast<core::addr_t *>(addrs.GetDataPtr()),
                 static_cast<bool *>(masks.GetDataPtr()), n);
    EXPECT_TRUE(masks.All());

    std::vector<int> keys_query_val = {100, 500, 800, 900, 1000};
    core::Tensor keys_query(keys_query_val, {n}, core::Dtype::Int32, device);
    hashmap.Find(keys_query.GetDataPtr(),
                 static_cast<core::addr_t *>(addrs.GetDataPtr()),
                 static_cast<bool *>(masks.GetDataPtr()), n);
    EXPECT_EQ(masks.ToFlatVector<bool>(),
              std::vector<bool>({true, true, false, true, false}));

    core::Tensor active_indices =
            addrs.IndexGet({masks}).To(core::Dtype::Int64);
    std::vector<core::Tensor> ai({active_indices});

    core::Tensor keys_buffer = core::Hashmap::ReinterpretBufferTensor(
            hashmap.GetKeyTensor(), {init_capacity}, core::Dtype::Int32);
    core::Tensor keys_valid = keys_buffer.IndexGet(ai);
    EXPECT_EQ(keys_valid.ToFlatVector<int>(),
              std::vector<int>({100, 500, 900}));

    core::Tensor values_buffer = core::Hashmap::ReinterpretBufferTensor(
            hashmap.GetValueTensor(), {init_capacity}, core::Dtype::Int32);
    core::Tensor values_valid = values_buffer.IndexGet(ai);
    EXPECT_EQ(values_valid.ToFlatVector<int>(), std::vector<int>({1, 5, 9}));
}

TEST_P(HashmapPermuteDevices, Insert) {
    core::Device device = GetParam();

    int n = 5;
    std::vector<int> keys_val = {100, 300, 500, 700, 900};
    std::vector<int> values_val = {1, 3, 5, 7, 9};

    core::Tensor keys(keys_val, {5}, core::Dtype::Int32, device);
    core::Tensor values(values_val, {5}, core::Dtype::Int32, device);

    int init_capacity = n * 2;
    core::Hashmap hashmap(init_capacity, core::Dtype::Int32, core::Dtype::Int32,
                          device);

    core::Tensor masks({n}, core::Dtype::Bool, device);
    core::Tensor addrs({n}, core::Dtype::Int32, device);

    hashmap.Insert(keys.GetDataPtr(), values.GetDataPtr(),
                   static_cast<core::addr_t *>(addrs.GetDataPtr()),
                   static_cast<bool *>(masks.GetDataPtr()), n);

    std::vector<int> keys_insert_val = {100, 500, 800, 900, 1000};
    std::vector<int> values_insert_val = {1, 5, 8, 9, 10};
    core::Tensor keys_insert(keys_insert_val, {5}, core::Dtype::Int32, device);
    core::Tensor values_insert(values_insert_val, {5}, core::Dtype::Int32,
                               device);
    hashmap.Insert(keys_insert.GetDataPtr(), values_insert.GetDataPtr(),
                   static_cast<core::addr_t *>(addrs.GetDataPtr()),
                   static_cast<bool *>(masks.GetDataPtr()), n);

    EXPECT_EQ(hashmap.Size(), 7);
    EXPECT_EQ(masks.ToFlatVector<bool>(),
              std::vector<bool>({false, false, true, false, true}));

    n = hashmap.Size();
    core::Tensor addrs_all({n}, core::Dtype::Int32, device);
    hashmap.GetActiveIndices(
            static_cast<core::addr_t *>(addrs_all.GetDataPtr()));
    core::Tensor indices_all = addrs_all.To(core::Dtype::Int64);
    core::Tensor keys_all = hashmap.GetKeyTensor().IndexGet({indices_all});
    core::Tensor values_all = hashmap.GetValueTensor().IndexGet({indices_all});

    std::unordered_map<int, int> key_value_all = {
            {100, 1}, {300, 3}, {500, 5},   {700, 7},
            {800, 8}, {900, 9}, {1000, 10},
    };
    for (int64_t i = 0; i < n; ++i) {
        int k = keys_all[i].Item<int>();
        int v = values_all[i].Item<int>();

        auto it = key_value_all.find(k);
        EXPECT_TRUE(it != key_value_all.end());
        EXPECT_EQ(it->first, k);
        EXPECT_EQ(it->second, v);
    }
}

TEST_P(HashmapPermuteDevices, Erase) {
    core::Device device = GetParam();

    int n = 5;
    std::vector<int> keys_val = {100, 300, 500, 700, 900};
    std::vector<int> values_val = {1, 3, 5, 7, 9};

    core::Tensor keys(keys_val, {5}, core::Dtype::Int32, device);
    core::Tensor values(values_val, {5}, core::Dtype::Int32, device);

    int init_capacity = n * 2;
    core::Hashmap hashmap(init_capacity, core::Dtype::Int32, core::Dtype::Int32,
                          device);

    core::Tensor masks({n}, core::Dtype::Bool, device);
    core::Tensor addrs({n}, core::Dtype::Int32, device);
    hashmap.Insert(keys.GetDataPtr(), values.GetDataPtr(),
                   static_cast<core::addr_t *>(addrs.GetDataPtr()),
                   static_cast<bool *>(masks.GetDataPtr()), n);

    std::vector<int> keys_erase_val = {100, 500, 800, 900, 1000};
    core::Tensor keys_erase(keys_erase_val, {5}, core::Dtype::Int32, device);
    hashmap.Erase(keys_erase.GetDataPtr(),
                  static_cast<bool *>(masks.GetDataPtr()), n);
    EXPECT_EQ(hashmap.Size(), 2);
    EXPECT_EQ(masks.ToFlatVector<bool>(),
              std::vector<bool>({true, true, false, true, false}));

    n = hashmap.Size();
    core::Tensor addrs_all({n}, core::Dtype::Int32, device);
    hashmap.GetActiveIndices(
            static_cast<core::addr_t *>(addrs_all.GetDataPtr()));
    core::Tensor indices_all = addrs_all.To(core::Dtype::Int64);
    core::Tensor keys_all = hashmap.GetKeyTensor().IndexGet({indices_all});
    core::Tensor values_all = hashmap.GetValueTensor().IndexGet({indices_all});

    std::unordered_map<int, int> key_value_all = {
            {300, 3},
            {700, 7},
    };
    for (int64_t i = 0; i < n; ++i) {
        int k = keys_all[i].Item<int>();
        int v = values_all[i].Item<int>();

        auto it = key_value_all.find(k);
        EXPECT_TRUE(it != key_value_all.end());
        EXPECT_EQ(it->first, k);
        EXPECT_EQ(it->second, v);
    }
}

TEST_P(HashmapPermuteDevices, Rehash) {
    core::Device device = GetParam();

    int n = 5;
    std::vector<int> keys_val = {100, 300, 500, 700, 900};
    std::vector<int> values_val = {1, 3, 5, 7, 9};

    core::Tensor keys(keys_val, {5}, core::Dtype::Int32, device);
    core::Tensor values(values_val, {5}, core::Dtype::Int32, device);

    int init_capacity = n * 2;
    core::Hashmap hashmap(init_capacity, core::Dtype::Int32, core::Dtype::Int32,
                          device);

    core::Tensor masks({n}, core::Dtype::Bool, device);
    core::Tensor addrs({n}, core::Dtype::Int32, device);

    hashmap.Insert(keys.GetDataPtr(), values.GetDataPtr(),
                   static_cast<core::addr_t *>(addrs.GetDataPtr()),
                   static_cast<bool *>(masks.GetDataPtr()), n);

    hashmap.Rehash(10);
    n = hashmap.Size();
    EXPECT_EQ(n, 5);

    core::Tensor addrs_all({n}, core::Dtype::Int32, device);
    hashmap.GetActiveIndices(
            static_cast<core::addr_t *>(addrs_all.GetDataPtr()));
    core::Tensor indices_all = addrs_all.To(core::Dtype::Int64);
    core::Tensor keys_all = hashmap.GetKeyTensor().IndexGet({indices_all});
    core::Tensor values_all = hashmap.GetValueTensor().IndexGet({indices_all});

    std::unordered_map<int, int> key_value_all = {
            {100, 1}, {300, 3}, {500, 5}, {700, 7}, {900, 9}};
    for (int64_t i = 0; i < n; ++i) {
        int k = keys_all[i].Item<int>();
        int v = values_all[i].Item<int>();

        auto it = key_value_all.find(k);
        EXPECT_TRUE(it != key_value_all.end());
        EXPECT_EQ(it->first, k);
        EXPECT_EQ(it->second, v);
    }
}

}  // namespace tests
}  // namespace open3d
