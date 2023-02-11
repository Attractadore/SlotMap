#include "Attractadore/SlotMap.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>

namespace Attractadore {
std::ostream &operator<<(std::ostream &os, SlotMapKey key) {
  struct Hack {
    uint32_t index;
    uint32_t generation;
  };
  auto hack = std::bit_cast<Hack>(key);
  os << "idx: " << hack.index << ", gen: " << hack.generation;
  return os;
}
} // namespace Attractadore

using Attractadore::SlotMap;
template class Attractadore::SlotMap<int>;

using TestIterator = SlotMap<int>::iterator;
static_assert(std::input_iterator<TestIterator>);
static_assert(std::forward_iterator<TestIterator>);
static_assert(std::bidirectional_iterator<TestIterator>);
static_assert(std::random_access_iterator<TestIterator>);

TEST(TestIsEmpty, Empty) {
  SlotMap<int> s;
  EXPECT_TRUE(s.empty());
}

TEST(TestIsEmpty, NotEmpty) {
  SlotMap<int> s;
  auto k = s.insert(1);
  EXPECT_FALSE(s.empty());
}

TEST(TestSize, InsertAndDelete) {
  SlotMap<int> s;
  std::vector<decltype(s)::key_type> keys;
  int i = 0;
  for (; i < 16; i++) {
    EXPECT_EQ(s.size(), i);
    auto k = s.insert(i);
    keys.push_back(k);
  }
  for (; i > 0; i--) {
    EXPECT_EQ(s.size(), i);
    auto k = keys.back();
    keys.pop_back();
    auto it = s.find(k);
    ASSERT_NE(it, s.end());
    s.erase(it);
  }
  EXPECT_EQ(s.size(), i);
}

TEST(TestCapacity, Fresh) {
  SlotMap<int> s;
  ASSERT_EQ(s.capacity(), 0);
}

TEST(TestCapacity, AfterReserve) {
  SlotMap<int> s;
  s.reserve(10);
  ASSERT_GE(s.capacity(), 10);
}

TEST(TestCapacity, AfterShrink) {
  SlotMap<int> s;
  s.reserve(10);
  auto cnt = s.size();
  s.shrink_to_fit();
  ASSERT_LE(s.capacity(), cnt);
}

TEST(TestClear, Clear) {
  SlotMap<int> s;
  for (size_t i = 0; i < 16; i++) {
    auto k = s.insert(i);
  }
  s.clear();
  EXPECT_TRUE(s.empty());
}

TEST(TestClear, ValueAfterInsertAfterClear) {
  SlotMap<int> s;
  for (size_t i = 0; i < 3; i++) {
    auto k = s.insert(i);
  }
  s.clear();
  EXPECT_TRUE(s.empty());
  std::vector values = {0, 1, 2, 3, 4};
  for (auto v : values) {
    auto k = s.insert(v);
    auto it = s.find(k);
    ASSERT_NE(it, s.end()) << "Iterator for value " << v << " and key (" << k
                           << ") is end()";
    EXPECT_EQ(it->first, k);
    EXPECT_EQ(it->second, v);
  }
}

TEST(TestInsert, ValueAfterInsert) {
  SlotMap<int> s;
  std::vector values = {0, 1, 2, 3, 4};
  for (auto v : values) {
    auto k = s.insert(v);
    auto it = s.find(k);
    ASSERT_NE(it, s.end()) << "Iterator for value " << v << " and key (" << k
                           << ") is end()";
    EXPECT_EQ(it->first, k);
    EXPECT_EQ(it->second, v);
  }
}

TEST(TestInsert, ValueAfterAllInsert) {
  SlotMap<int> s;
  std::vector values = {0, 1, 2, 3, 4};
  std::vector<decltype(s)::key_type> keys;
  for (auto v : values) {
    keys.push_back(s.insert(v));
  }
  for (size_t i = 0; i < values.size(); i++) {
    auto k = keys[i];
    auto v = values[i];
    auto it = s.find(k);
    ASSERT_NE(it, s.end()) << "Iterator for value " << v << " and key (" << k
                           << ") is end()";
    EXPECT_EQ(it->first, k);
    EXPECT_EQ(it->second, v);
  }
}

TEST(TestInsert, InsertAfterErase) {
  SlotMap<int> s;

  auto base_k = s.insert(0);
  auto old_k = s.insert(1);
  s.erase(old_k);
  auto new_k = s.insert(2);
  ASSERT_NE(new_k, base_k);
  ASSERT_NE(new_k, old_k);

  auto base_it = s.find(base_k);
  auto old_it = s.find(old_k);
  auto new_it = s.find(new_k);

  ASSERT_NE(base_it, s.end());
  ASSERT_EQ(old_it, s.end());
  ASSERT_NE(new_it, s.end());

  EXPECT_EQ(base_it->second, 0);
  EXPECT_EQ(new_it->second, 2);
}

TEST(TestEmplace, Emplace) {
  SlotMap<int> s;
  std::vector values = {0, 1, 2, 3, 4};
  for (auto v : values) {
    auto &&[k, ref] = *s.emplace(v);
    EXPECT_EQ(ref, v);
  }
}

TEST(TestEmplace, EmplaceAfterErase) {
  SlotMap<int> s;

  auto base_k = s.emplace(0)->first;
  auto old_k = s.emplace(1)->first;
  s.erase(old_k);
  auto new_k = s.emplace(2)->first;

  auto base_it = s.find(base_k);
  auto old_it = s.find(old_k);
  auto new_it = s.find(new_k);

  ASSERT_NE(base_it, s.end());
  ASSERT_EQ(old_it, s.end());
  ASSERT_NE(new_it, s.end());

  EXPECT_EQ(base_it->second, 0);
  EXPECT_EQ(new_it->second, 2);
}

TEST(TestErase, EraseIterator) {
  SlotMap<int> s;

  auto k = s.insert(0);
  auto it = s.find(k);
  ASSERT_NE(it, s.end());

  s.erase(it);
  EXPECT_EQ(s.size(), 0);
  EXPECT_EQ(s.find(k), s.end());
}

TEST(TestErase, EraseAllIterator) {
  SlotMap<int> s;
  std::vector values = {0, 1, 2, 3, 4};
  for (auto v : values) {
    auto k = s.insert(v);
  }
  for (auto it = s.begin(); it != s.end();) {
    it = s.erase(it);
  }
  EXPECT_TRUE(s.empty());
}

TEST(TestErase, EraseAllIteratorBackward) {
  SlotMap<int> s;
  std::vector values = {0, 1, 2, 3, 4};
  for (auto v : values) {
    auto k = s.insert(v);
  }
  while (not s.empty()) {
    s.erase(s.end() - 1);
  }
  EXPECT_TRUE(s.empty());
}

TEST(TestErase, EraseKey) {
  SlotMap<int> s;

  auto k = s.insert(0);

  s.erase(k);
  EXPECT_EQ(s.size(), 0);
  EXPECT_EQ(s.find(k), s.end());
}

TEST(TestTryErase, TryErase) {
  SlotMap<int> s;

  auto k = s.insert(0);

  EXPECT_TRUE(s.try_erase(k));
  EXPECT_FALSE(s.try_erase(k));
}

TEST(TestSwap, Swap) {
  SlotMap<int> s1, s2;
  using key_type = decltype(s1)::key_type;
  std::vector<key_type> keys1;
  std::vector<key_type> keys2;
  std::array vals = {0, 1, 2, 3, 4, 5};

  int i = 0;
  for (; i < 4; i++) {
    keys1.push_back(s1.insert(vals[i]));
  }
  for (; i < vals.size(); i++) {
    keys2.push_back(s2.insert(vals[i]));
  }

  swap(s1, s2);

  i = 0;
  for (; i < 4; i++) {
    auto k = keys1[i];
    EXPECT_EQ(s2[k], vals[i]);
  }
  for (; i < vals.size(); i++) {
    auto k = keys2[i - 4];
    EXPECT_EQ(s1[k], vals[i]);
  }
}

TEST(TestFind, FindPresent) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  auto it = s.find(k);
  ASSERT_NE(it, s.end());
  EXPECT_EQ(it->second, val);
}

TEST(TestFind, FindNotPresent) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  s.erase(k);
  auto it = s.find(k);
  EXPECT_EQ(it, s.end());
}

TEST(TestFind, FindAfterClear) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  s.clear();
  auto it = s.find(k);
  EXPECT_EQ(it, s.end());
}

TEST(TestGet, GetPresent) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  auto *ptr = s.get(k);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, val);
}

TEST(TestGet, GetNotPresent) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  s.erase(k);
  auto *ptr = s.get(k);
  EXPECT_EQ(ptr, nullptr);
}

TEST(TestAccess, Access) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  EXPECT_EQ(s[k], val);
}

TEST(TestContains, ContainsPresent) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  EXPECT_TRUE(s.contains(k));
}

TEST(TestContains, ContainsNotPresent) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  s.erase(k);
  EXPECT_FALSE(s.contains(k));
}

TEST(TestContains, ContainsAfterClear) {
  SlotMap<int> s;
  auto val = 0xffdead;
  auto k = s.insert(val);
  s.clear();
  EXPECT_FALSE(s.contains(k));
}

TEST(TestCompare, Self) {
  SlotMap<int> s1;
  EXPECT_EQ(s1, s1);
}

TEST(TestCompare, Copy) {
  SlotMap<int> s1;
  auto s2 = s1;
  EXPECT_EQ(s1, s2);
}

TEST(TestCompare, DifferentEmpty) {
  SlotMap<int> s1;
  SlotMap<int> s2;
  EXPECT_EQ(s1, s2);
}

TEST(TestCompare, DifferentNotEmpty) {
  SlotMap<int> s1;
  auto k = s1.insert(0);
  SlotMap<int> s2;
  EXPECT_NE(s1, s2);
}

TEST(TestCompare, DifferentSame) {
  SlotMap<int> s1;
  for (int i = 0; i < 16; i++) {
    auto k = s1.insert(i);
  }
  SlotMap<int> s2;
  for (int i = 0; i < 16; i++) {
    EXPECT_NE(s1, s2);
    auto k = s2.insert(i);
  }
  EXPECT_EQ(s1, s2);
}

TEST(TestPop, Pop) {
  SlotMap<int> s;

  auto k = s.insert(0);

  auto v = s.pop(k);
  EXPECT_EQ(s.size(), 0);
  EXPECT_EQ(s.find(k), s.end());
  EXPECT_EQ(v, 0);
}

TEST(TestPop, PopAll) {
  SlotMap<int> s;
  using Key = decltype(s)::key_type;

  std::vector values = {0, 1, 2, 3, 4};
  std::vector<Key> keys;

  for (auto v : values) {
    auto k = s.insert(v);
    keys.push_back(k);
  }
  for (size_t i = 0; i < values.size(); i++) {
    auto k = keys[i];
    EXPECT_EQ(s.size(), values.size() - i);
    auto v = s.pop(k);
    EXPECT_EQ(v, values[i]);
  }
  EXPECT_TRUE(s.empty());
}

TEST(TestTryPop, TryPop) {
  SlotMap<int> s;

  auto val = 0;
  auto key = s.insert(val);

  EXPECT_EQ(s.try_pop(key), val);
  EXPECT_EQ(s.try_pop(key), std::nullopt);
}

TEST(TestKeysValues, KeysValues) {
  SlotMap<int> s;
  std::vector values = {0, 1, 2, 3, 4};
  std::vector<decltype(s)::key_type> keys;
  for (auto v : values) {
    keys.push_back(s.insert(v));
  }
  EXPECT_TRUE(std::ranges::is_permutation(s.keys(), keys));
  EXPECT_TRUE(std::ranges::is_permutation(s.values(), values));
}
