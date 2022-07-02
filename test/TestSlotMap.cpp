#include "SlotMap/SlotMap.hpp"

#include <gtest/gtest.h>

namespace Attractadore::Detail {
template<unsigned I, unsigned G>
std::ostream& operator<<(std::ostream& os, typename Attractadore::Detail::Key<I, G> key) {
    os << "idx: " << key.idx << ", gen: " << key.gen;
    return os;
}
}

using Attractadore::SlotMap;

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
    for (;i > 0; i--) {
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

TEST(TestInsert, ValueAfterInsert) {
    SlotMap<int> s;
    std::vector values = {0, 1, 2, 3, 4};
    for (auto v: values) {
        auto k = s.insert(v);
        auto it = s.find(k);
        ASSERT_NE(it, s.end()) <<
            "Iterator for value " << v << " and key (" << k << ") is end()";
        EXPECT_EQ(*it, v);
    }
}

TEST(TestInsert, ValueAfterAllInsert) {
    SlotMap<int> s;
    std::vector values = {0, 1, 2, 3, 4};
    std::vector<decltype(s)::key_type> keys;
    for (auto v: values) {
        keys.push_back(s.insert(v));
    }
    for (size_t i = 0; i < values.size(); i++) {
        auto k = keys[i];
        auto v = values[i];
        auto it = s.find(k);
        ASSERT_NE(it, s.end()) <<
            "Iterator for value " << v << " and key (" << k << ") is end()";
        EXPECT_EQ(*it, v);
    }
}

TEST(TestInsert, InsertAfterErase) {
    SlotMap<int> s;

    auto base_k = s.insert(0);
    auto old_k = s.insert(1);
    s.erase(s.find(old_k));
    auto new_k = s.insert(2);

    auto base_it = s.find(base_k);
    auto old_it = s.find(old_k);
    auto new_it = s.find(new_k);

    ASSERT_NE(base_it, s.end());
    ASSERT_EQ(old_it, s.end());
    ASSERT_NE(new_it, s.end());

    EXPECT_EQ(*base_it, 0);
    EXPECT_EQ(*new_it, 2);
}
