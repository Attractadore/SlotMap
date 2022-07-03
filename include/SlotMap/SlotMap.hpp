#pragma once
#include <utility>
#include <vector>
#include <cstdint>

namespace Attractadore {
namespace Detail {
template<unsigned Bits> 
auto MinWideTypeImpl() {
    if constexpr (Bits <= 8) {
        return uint8_t();
    } else if constexpr (Bits <= 16) {
        return uint16_t();
    } else if constexpr (Bits <= 32) {
        return uint32_t();
    } else if constexpr (Bits <= 64) {
        return uint64_t();
    } else throw "Too many bits to pack into a standard type";
}

template<unsigned Bits> 
using MinWideType = decltype(MinWideTypeImpl<Bits>());

template<unsigned IdxBits, unsigned GenBits>
struct Key {
    using pack_type = MinWideType<IdxBits + GenBits>;
    using idx_type  = MinWideType<IdxBits>;
    using gen_type  = MinWideType<GenBits>;

    static constexpr auto idx_bits = IdxBits;
    static constexpr auto gen_bits = GenBits;
    static constexpr auto max_idx = (1u << IdxBits) - 1u;
    static constexpr auto max_gen = (1u << GenBits) - 1u;

    pack_type gen: GenBits;
    pack_type idx: IdxBits;
};

template<typename Key, typename Value>
struct EmplaceResult {
    Key key;
    Value& reference;
};

template<typename C>
concept HasReserve = requires (C c) {
    c.reserve(std::declval<typename C::size_type>());
};

template<typename SlotMap>
concept SlotMapHasReserve =
    HasReserve<typename SlotMap::ObjC> and
    HasReserve<typename SlotMap::IdxC> and
    HasReserve<typename SlotMap::KeyC>;

template<typename C>
concept HasCapacity = requires (C c) {
    c.capacity();
};

template<typename SlotMap>
concept SlotMapHasCapacity =
    HasCapacity<typename SlotMap::ObjC> and
    HasCapacity<typename SlotMap::IdxC> and
    HasCapacity<typename SlotMap::KeyC>;

template<typename C>
concept HasShrinkToFit = requires (C c) {
    c.shrink_to_fit();
};

template<typename SlotMap>
concept SlotMapHasShrinkToFit =
    HasShrinkToFit<typename SlotMap::ObjC> and
    HasShrinkToFit<typename SlotMap::IdxC> and
    HasShrinkToFit<typename SlotMap::KeyC>;
}

template<
    typename T,
    unsigned IdxBits = 24,
    unsigned GenBits = 8
>
class SlotMap {
    using Key   = Detail::Key<IdxBits, GenBits>;
    using IdxT  = typename Key::idx_type;
    static constexpr IdxT free_list_null = Key::max_idx;

    using ObjC  = std::vector<T>;
    using IdxC  = std::vector<Key>;
    using KeyC  = std::vector<IdxT>;

    ObjC objects;
    IdxC indices;
    KeyC keys;
    IdxT free_list_head = free_list_null;

public:
    using const_iterator    = typename ObjC::const_iterator;
    using iterator          = typename ObjC::iterator;
    using size_type         = IdxT;
    using value_type        = T;
    using key_type          = Key;
    using emplace_result    = Detail::EmplaceResult<key_type, value_type>;

    constexpr const_iterator cbegin() const noexcept { return objects.cbegin(); }
    constexpr const_iterator cend() const noexcept { return objects.cend(); }
    constexpr const_iterator begin() const noexcept { return objects.begin(); }
    constexpr const_iterator end() const noexcept { return objects.end(); }
    constexpr iterator begin() noexcept { return objects.begin(); }
    constexpr iterator end() noexcept { return objects.end(); }

    constexpr bool empty() const noexcept { return begin() == end(); }
    constexpr size_type size() const noexcept { return std::distance(begin(), end()); }
    static constexpr size_type max_size() noexcept { return Key::max_idx - 1; }
    template<Detail::SlotMapHasReserve = SlotMap>
    constexpr void reserve(size_type sz);
    template<Detail::SlotMapHasCapacity = SlotMap>
    constexpr size_type capacity() const noexcept;
    template<Detail::SlotMapHasShrinkToFit = SlotMap>
    constexpr void shrink_to_fit();

    constexpr void clear() noexcept;

    // TODO: insert for > 1?
    template<std::copy_constructible = value_type>
    [[nodiscard]] constexpr key_type insert(const value_type& val) { return EmplaceImpl(val); }
    template<std::move_constructible = value_type>
    [[nodiscard]] constexpr key_type insert(value_type&& val) { return EmplaceImpl(std::move(val)); }
    template<typename... Args>
        requires std::constructible_from<value_type, Args&&...>
    [[nodiscard]] constexpr emplace_result emplace(Args&&... args);

    constexpr iterator erase(iterator it) noexcept;
    constexpr void erase(key_type k) noexcept { EraseImpl(Index(k)); }
    // TODO: is std::swap good enough?
    constexpr void swap(SlotMap& other) noexcept(noexcept(std::swap(*this, other))) { std::swap(*this, other); }

    constexpr const_iterator find(key_type k) const noexcept;
    constexpr iterator find(key_type k) noexcept;
    constexpr const T& operator[](key_type k) const noexcept { return objects[Index(k)]; }
    constexpr T& operator[](key_type k) noexcept { return objects[Index(k)]; }
    constexpr bool contains(key_type k) const noexcept { return find(k) != end(); };
    constexpr const T* data() const noexcept { return objects.data(); }
    constexpr T* data() noexcept { return objects.data(); }

private:
    template<typename... Args>
        requires std::constructible_from<T, Args&&...>
    [[nodiscard]] constexpr key_type EmplaceImpl(Args&&... args);

    constexpr void EraseImpl(IdxT erase_obj_idx) noexcept;

    constexpr IdxT Index(key_type k) const noexcept { return indices[k.idx].idx; }
};

template<typename T, unsigned I, unsigned G>
template<typename... Args>
    requires std::constructible_from<T, Args&&...>
constexpr auto SlotMap<T, I, G>::EmplaceImpl(Args&&... args) -> key_type {
    if (free_list_head == free_list_null) {
        IdxT idx = indices.size();
        objects.emplace_back(std::forward<Args>(args)...);
        keys.push_back(idx);
        indices.push_back({.idx = idx});
        return {
            .idx = idx,
        };
    } else {
        auto& [gen, next] = indices[free_list_head];
        auto idx = free_list_head;
        IdxT obj_idx = objects.size();
        objects.emplace_back(std::forward<Args>(args)...);
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
    if (k.idx >= indices.size()) {
        return end();
    }
    auto [gen, obj_idx] = indices[k.idx];
    if (gen != k.gen) {
        return end();
    } else {
        return std::next(begin(), obj_idx);
    }
}

template<typename T, unsigned I, unsigned G>
constexpr void SlotMap<T, I, G>::EraseImpl(IdxT erase_obj_idx) noexcept {
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

    // Update erased object's generation
    indices[erase_obj_idx_idx].gen++;

    // Update free list
    indices[erase_obj_idx_idx].idx =
        std::exchange(free_list_head, erase_obj_idx_idx);
}

template<typename T, unsigned I, unsigned G>
template<Detail::SlotMapHasReserve>
constexpr void SlotMap<T, I, G>::reserve(size_type sz) {
    objects.reserve(sz);
    indices.reserve(sz);
    keys.reserve(sz);
}

template<typename T, unsigned I, unsigned G>
template<Detail::SlotMapHasCapacity>
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
template<Detail::SlotMapHasShrinkToFit>
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
    free_list_head = free_list_null;
}

template<typename T, unsigned I, unsigned G>
template<typename... Args>
    requires std::constructible_from<T, Args&&...>
constexpr auto SlotMap<T, I, G>::emplace(Args&&... args) -> emplace_result {
    // TODO: optimize reference retrieval or don't do it
    auto key = EmplaceImpl(std::forward<Args>(args)...);
    return { key, (*this)[key] };
}

template<typename T, unsigned I, unsigned G>
constexpr auto SlotMap<T, I, G>::erase(iterator it) noexcept -> iterator {
    auto erase_obj_idx = &*it - objects.data();
    EraseImpl(erase_obj_idx);
    return it;
}

template<typename T, unsigned I, unsigned G>
constexpr void swap(SlotMap<T, I, G>& l, SlotMap<T, I, G>& r) noexcept(noexcept(l.swap(l, r))) {
    l.swap(r);
}
}
