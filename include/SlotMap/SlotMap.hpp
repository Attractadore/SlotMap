#pragma once
#include <utility>
#include <vector>

namespace Attractadore {
namespace Detail {
template<
    unsigned IndexBits,
    unsigned GenerationBits
>
struct Key {
    using pack_type = unsigned;
    using index_type = unsigned;
    using generation_type = unsigned;
    static constexpr auto max_index = (1 << IndexBits) - 1;
    static constexpr auto max_generation = 1 << GenerationBits;
    unsigned gen: GenerationBits;
    unsigned idx: IndexBits;
};

template<typename C>
concept HasReserve = requires (C c) {
    c.reserve(std::declval<typename C::size_type>());
};

template<typename C>
concept HasCapacity = requires (C c) {
    c.capacity();
};

template<typename C>
concept HasShrinkToFit = requires (C c) {
    c.shrink_to_fit();
};
}

template<
    typename T,
    unsigned IndexBits = 24,
    unsigned GenerationBits = 8
>
class SlotMap {
    static constexpr auto idx_bits = IndexBits;
    static constexpr auto gen_bits = GenerationBits;
    using Key       = Detail::Key<idx_bits, gen_bits>;
    using IndexT    = typename Key::index_type;
    static constexpr IndexT free_list_null = Key::max_index;

    using ObjectContainer   = std::vector<T>;
    using IndexContainer    = std::vector<Key>;
    using KeyContainer      = std::vector<IndexT>;

    ObjectContainer objects;
    IndexContainer  indices;
    KeyContainer    keys;

    // TODO: how to determine list end?
    IndexT              free_list_head = free_list_null;

public:
    using const_iterator    = ObjectContainer::const_iterator;
    using iterator          = ObjectContainer::iterator;
    using size_type         = IndexT;
    using value_type        = T;
    using key_type          = Key;

    constexpr const_iterator cbegin() const noexcept { return objects.cbegin(); }
    constexpr const_iterator cend() const noexcept { return objects.cend(); }
    constexpr const_iterator begin() const noexcept { return objects.begin(); }
    constexpr const_iterator end() const noexcept { return objects.end(); }
    constexpr iterator begin() noexcept { return objects.begin(); }
    constexpr iterator end() noexcept { return objects.end(); }

    constexpr bool empty() const noexcept { return begin() == end(); }
    constexpr size_type size() const noexcept { return std::distance(begin(), end()); }
    constexpr size_type max_size() const noexcept { return Key::max_index - 1; }
    template<typename = void> requires
        Detail::HasReserve<ObjectContainer> and
        Detail::HasReserve<IndexContainer> and
        Detail::HasReserve<KeyContainer>
    constexpr void reserve(size_type sz);
    template<typename = void> requires
        Detail::HasCapacity<ObjectContainer> and
        Detail::HasCapacity<IndexContainer> and
        Detail::HasCapacity<KeyContainer>
    constexpr size_type capacity() const noexcept;
    template<typename = void> requires
        Detail::HasShrinkToFit<ObjectContainer> and
        Detail::HasShrinkToFit<IndexContainer> and
        Detail::HasShrinkToFit<KeyContainer>
    constexpr void shrink_to_fit();

    constexpr void clear() noexcept;
    // TODO: insert for > 1?
    [[nodiscard]] constexpr key_type insert(const T& val);
    // [[nodiscard]] constexpr key_type insert(T&& val);
    template<typename... Args>
    [[nodiscard]] constexpr key_type emplace(Args... args);
    constexpr void erase(iterator it) noexcept;
    constexpr void erase(key_type k) noexcept;
    constexpr void swap(SlotMap& other) noexcept;

    constexpr const_iterator find(key_type k) const noexcept;
    constexpr iterator find(key_type k) noexcept;
    constexpr const T& operator[](key_type k) const noexcept;
    constexpr T& operator[](key_type k) noexcept;
    constexpr bool contains(key_type k) const noexcept;
    constexpr const T* data() const noexcept { return objects.data(); }
    constexpr T* data() noexcept { return objects.data(); }
};

template<typename T, unsigned I, unsigned G>
constexpr auto SlotMap<T, I, G>::insert(const T& val) -> key_type {
    if (free_list_head == free_list_null) {
        IndexT idx = indices.size();
        objects.push_back(val);
        keys.push_back(idx);
        indices.push_back({.idx = idx});
        return {
            .idx = idx,
        };
    } else {
        auto& [gen, next] = indices[free_list_head];
        auto idx = free_list_head;
        IndexT obj_idx = objects.size();
        objects.push_back(val);
        keys.push_back(idx);
        free_list_head = next;
        next = obj_idx;
        return {
            .gen = gen,
            .idx = idx,
        };
    };
}

template<typename T, unsigned I, unsigned G>
constexpr auto SlotMap<T, I, G>::find(key_type k) noexcept -> iterator {
    auto [gen, obj_idx] = indices[k.idx];
    if (gen != k.gen) {
        return end();
    } else {
        return std::next(begin(), obj_idx);
    }
}

template<typename T, unsigned I, unsigned G>
constexpr void SlotMap<T, I, G>::erase(SlotMap::iterator it) noexcept {
    auto erase_obj_idx = &*it - objects.data();

    // Erase object from object array
    std::swap(objects[erase_obj_idx], objects.back());
    objects.pop_back();

    // Erase object from index-key map
    auto back_obj_idx_idx = keys.back();
    auto erase_obj_idx_idx =
        std::exchange(keys[erase_obj_idx], back_obj_idx_idx);
    keys.pop_back();

    // Update key-index map for back object
    indices[back_obj_idx_idx].idx = erase_obj_idx;

    // Increment deleted object's generation
    indices[erase_obj_idx_idx].gen++;

    // Update free list
    indices[erase_obj_idx_idx].idx =
        std::exchange(free_list_head, erase_obj_idx_idx);
}

template<typename T, unsigned I, unsigned G>
template<typename> requires
    Detail::HasReserve<typename SlotMap<T, I, G>::ObjectContainer> and
    Detail::HasReserve<typename SlotMap<T, I, G>::IndexContainer> and
    Detail::HasReserve<typename SlotMap<T, I, G>::KeyContainer>
constexpr void SlotMap<T, I, G>::reserve(size_type sz) {
    objects.reserve(sz);
    indices.reserve(sz);
    keys.reserve(sz);
}

template<typename T, unsigned I, unsigned G>
template<typename> requires
    Detail::HasCapacity<typename SlotMap<T, I, G>::ObjectContainer> and
    Detail::HasCapacity<typename SlotMap<T, I, G>::IndexContainer> and
    Detail::HasCapacity<typename SlotMap<T, I, G>::KeyContainer>
constexpr auto SlotMap<T, I, G>::capacity() const noexcept -> size_type {
    auto cap = max_size();
    auto obj_cap = objects.capacity();
    if (obj_cap < cap) {
        cap = obj_cap;
    }
    auto idx_cap = indices.capacity();
    if (idx_cap < cap) {
        cap = idx_cap;
    }
    auto key_cap = keys.capacity();
    if (key_cap < cap) {
        cap = key_cap;
    }
    return cap;
}

template<typename T, unsigned I, unsigned G>
template<typename> requires
    Detail::HasShrinkToFit<typename SlotMap<T, I, G>::ObjectContainer> and
    Detail::HasShrinkToFit<typename SlotMap<T, I, G>::IndexContainer> and
    Detail::HasShrinkToFit<typename SlotMap<T, I, G>::KeyContainer>
constexpr void SlotMap<T, I, G>::shrink_to_fit() {
    objects.shrink_to_fit();
    indices.shrink_to_fit();
    keys.shrink_to_fit();
}

template<typename T, unsigned I, unsigned G>
constexpr void SlotMap<T, I, G>::clear() noexcept {
    objects.clear();
    indices.clear();
    keys.clear();
}
}
