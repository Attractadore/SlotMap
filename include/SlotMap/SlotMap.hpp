#pragma once
#include <limits>
#include <vector>

namespace SlotMap {
template<typename T, typename KeyT = unsigned>
class SlotMap {
    static constexpr auto index_bits = 24;
    static constexpr auto geneneration_bits = 8;
    static constexpr KeyT free_list_null = std::numeric_limits<KeyT>::max();

    std::vector<T>      objects;
    std::vector<KeyT>   indices;
    std::vector<KeyT>   keys;

    // TODO: how to determine list end?
    KeyT                free_list_head = free_list_null;

public:
    constexpr KeyT insert(const T& val);
    constexpr void erase(KeyT key);

    static constexpr KeyT GetIndex(KeyT key) noexcept {
        return key & ((1 << index_bits) - 1);
    }

    static constexpr KeyT GetGen(KeyT key) noexcept {
        return key >> index_bits;
    }

    static constexpr KeyT MakeKey(KeyT index, KeyT generation) noexcept {
        return (generation << index_bits) | index;
    }
};

template<typename T, typename KeyT>
constexpr KeyT SlotMap<T, KeyT>::insert(const T& val) {
    KeyT index, gen;
    if (free_list_head == free_list_null) {
        index = indices.size();
        indices.push_back(objects.size());
        gen = 0;
    } else {
        auto next_and_gen = indices[free_list_head];
        index = free_list_head;
        gen = GetGen(next_and_gen);
        free_list_head = GetIndex(next_and_gen);
    }
    keys.push_back(index);
    objects.push_back(val);
    return MakeKey(index, gen);
}


template<typename T, typename KeyT>
constexpr void SlotMap<T, KeyT>::erase(KeyT key) {
    auto key_idx = GetIndex(key);
    auto gen = GetGen(key);
    auto idx = indices[key_idx];
    if (gen != GetGen(idx)) {
        return;
    }
    idx = GetIndex(key);
}
}
