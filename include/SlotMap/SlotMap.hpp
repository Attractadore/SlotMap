#pragma once
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

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

template<typename C>
concept HasData = requires (C c, const C cc) {
    c.data();
    cc.data();
};

template<typename SlotMap>
concept SlotMapHasData = HasData<typename SlotMap::ObjC>;
}

template<
    typename T,
    unsigned IdxBits = 24,
    unsigned GenBits = 8
>
class SlotMap {
    using Key   = Detail::Key<IdxBits, GenBits>;
    using IdxT  = typename Key::idx_type;
    using GenT  = typename Key::gen_type;

    using ObjC  = std::vector<T>;
    using IdxC  = std::vector<Key>;
    using KeyC  = std::vector<IdxT>;

    static constexpr IdxT free_list_null = Key::max_idx;

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

    template<std::copy_constructible = value_type>
    [[nodiscard]] constexpr key_type insert(const value_type& val) { return EmplaceImpl(val); }
    template<std::move_constructible = value_type>
    [[nodiscard]] constexpr key_type insert(value_type&& val) { return EmplaceImpl(std::move(val)); }
    template<typename... Args>
        requires std::constructible_from<value_type, Args&&...>
    [[nodiscard]] constexpr emplace_result emplace(Args&&... args);
    constexpr iterator erase(iterator it) noexcept;
    constexpr void erase(key_type k) noexcept { EraseImpl(Index(k)); }
    constexpr void swap(SlotMap& other) noexcept;

    constexpr const_iterator find(key_type k) const noexcept;
    constexpr iterator find(key_type k) noexcept;
    constexpr const T& operator[](key_type k) const noexcept { return objects[Index(k)]; }
    constexpr T& operator[](key_type k) noexcept { return objects[Index(k)]; }
    constexpr bool contains(key_type k) const noexcept { return find(k) != end(); };
    template<Detail::SlotMapHasData = SlotMap>
    constexpr const value_type* data() const noexcept { return objects.data(); }
    template<Detail::SlotMapHasData = SlotMap>
    constexpr value_type* data() noexcept { return objects.data(); }

private:
    template<typename... Args>
        requires std::constructible_from<T, Args&&...>
    [[nodiscard]] constexpr key_type EmplaceImpl(Args&&... args);

    constexpr void EraseImpl(IdxT erase_obj_idx) noexcept;

    constexpr IdxT Index(key_type k) const noexcept {
        assert(k.idx < indices.size());
        auto idx = indices[k.idx].idx;
        assert(idx < objects.size());
        return idx;
    }
};

template<typename T, unsigned I, unsigned G>
template<typename... Args>
    requires std::constructible_from<T, Args&&...>
constexpr auto SlotMap<T, I, G>::EmplaceImpl(Args&&... args) -> key_type {
    IdxT obj_idx = objects.size();

    // Reserve space in key-index map
    if (free_list_head == free_list_null) {
        IdxT free_list_next = indices.size();
        indices.emplace_back().idx =
            std::exchange(free_list_head, free_list_next);
    }

    // Reserve space in index-key map
    keys.resize(obj_idx + 1);

    auto idx_idx = free_list_head;
    auto [gen, free_list_next] = indices[idx_idx];

    // Insert object into object array
    objects.emplace_back(std::forward<Args>(args)...);
    // Insert object into index-key map
    keys.back() = idx_idx;
    // Insert object into key-index map
    indices[idx_idx].idx = obj_idx;
    // Update free list
    free_list_head = free_list_next;

    return {
        .gen = gen,
        .idx = idx_idx,
    };
}

template<typename T, unsigned I, unsigned G>
constexpr auto SlotMap<T, I, G>::find(key_type k) const noexcept -> const_iterator {
    auto [key_gen, idx_idx] = k;
    if (idx_idx < indices.size()) {
        auto [gen, obj_idx] = indices[idx_idx];
        if (key_gen == gen) {
            auto it = std::next(begin(), obj_idx);
            assert(it < end());
            return it;
        }
    }
    return end();
}

template<typename T, unsigned I, unsigned G>
constexpr auto SlotMap<T, I, G>::find(key_type k) noexcept -> iterator {
    auto [key_gen, idx_idx] = k;
    if (idx_idx < indices.size()) {
        auto [gen, obj_idx] = indices[idx_idx];
        if (key_gen == gen) {
            auto it = std::next(begin(), obj_idx);
            assert(it < end());
            return it;
        }
    }
    return end();
}

template<typename T, unsigned I, unsigned G>
constexpr void SlotMap<T, I, G>::EraseImpl(IdxT erase_obj_idx) noexcept {
    using std::swap;
    auto back_idx_idx = keys.back();

    // Erase object from object array
    swap(objects[erase_obj_idx], objects.back());
    objects.pop_back();

    // Erase object from index-key map
    auto erase_idx_idx =
        std::exchange(keys[erase_obj_idx], back_idx_idx);
    keys.pop_back();

    // Update key-index map for back object
    indices[back_idx_idx].idx = erase_obj_idx;

    // Erase object from key-index map and update free list
    GenT gen = indices[erase_idx_idx].gen + 1;
    indices[erase_idx_idx] = {
        .gen = gen,
        .idx = std::exchange(free_list_head, erase_idx_idx),
    };
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
constexpr void SlotMap<T, I, G>::swap(SlotMap<T, I, G>& other) noexcept {
    using std::swap;
    swap(objects, other.objects);
    swap(indices, other.indices);
    swap(keys, other.keys);
    swap(free_list_head, other.free_list_head);
}

template<typename T, unsigned I, unsigned G>
constexpr void swap(SlotMap<T, I, G>& l, SlotMap<T, I, G>& r) noexcept {
    l.swap(r);
}
}
