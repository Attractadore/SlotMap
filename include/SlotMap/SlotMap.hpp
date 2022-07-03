#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

namespace Attractadore {
namespace Detail {
template<unsigned Bits> requires (Bits <= 64)
auto MinWideTypeImpl() {
    if constexpr (Bits <= 8) {
        return uint8_t();
    } else if constexpr (Bits <= 16) {
        return uint16_t();
    } else if constexpr (Bits <= 32) {
        return uint32_t();
    } else if constexpr (Bits <= 64) {
        return uint64_t();
    }
}

template<unsigned Bits>
using MinWideType = decltype(MinWideTypeImpl<Bits>());

template<unsigned IdxBits, unsigned GenBits>
    requires (IdxBits + GenBits <= 64) and (IdxBits > 0) and (GenBits > 0)
struct Key {
    using pack_type = MinWideType<IdxBits + GenBits>;
    using idx_type  = MinWideType<IdxBits>;
    using gen_type  = MinWideType<GenBits>;

    static constexpr auto idx_bits = IdxBits;
    static constexpr auto gen_bits = GenBits;
    static constexpr auto max_idx = (1ull << IdxBits) - 1u;
    static constexpr auto max_gen = (1ull << GenBits) - 1u;

    pack_type gen: GenBits;
    pack_type idx: IdxBits;

    constexpr auto operator<=>(const Key& other) const noexcept {
        auto idx_order = std::strong_order(idx, other.idx);
        if (idx_order == std::strong_ordering::equal) {
            return std::strong_order(gen, other.gen);
        } else {
            return idx_order;
        }
    }
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

template<
    typename T,
    unsigned IdxBits,
    unsigned GenBits,
    template <typename> typename ObjectContainer,
    template <typename> typename KeyContainer
>
concept SlotMapRequires =
    std::is_nothrow_move_constructible_v<T> and
    (IdxBits + GenBits <= 64) and
    (IdxBits > 0) and
    (GenBits > 0) and
    requires (ObjectContainer<T> c) {
        { std::ranges::swap(c, c) } noexcept;
    } and
    requires (KeyContainer<typename Detail::Key<IdxBits, GenBits>> c) {
        { std::ranges::swap(c, c) } noexcept;
    } and
    requires (KeyContainer<typename Detail::Key<IdxBits, GenBits>::idx_type> c) {
        { std::ranges::swap(c, c) } noexcept;
    };

template<typename T>
using StdVector = std::vector<T>;
}

#define SLOTMAP_TEMPLATE_DECL template<\
    typename T,\
    unsigned I,\
    unsigned G,\
    template <typename> typename O,\
    template <typename> typename K\
> requires Detail::SlotMapRequires<T, I, G, O, K>
#define SLOTMAP SlotMap<T, I, G, O, K>

template<
    typename T,
    unsigned IdxBits = 24,
    unsigned GenBits = 8,
    template <typename> typename ObjectContainer = Detail::StdVector,
    template <typename> typename KeyContainer    = Detail::StdVector
> requires Detail::SlotMapRequires<T, IdxBits, GenBits, ObjectContainer, KeyContainer>
class SlotMap {
    using Key   = Detail::Key<IdxBits, GenBits>;
    using IdxT  = typename Key::idx_type;
    using GenT  = typename Key::gen_type;

    using ObjC  = ObjectContainer<T>;
    using IdxC  = KeyContainer<Key>;
    using KeyC  = KeyContainer<IdxT>;

    static constexpr IdxT free_list_null = Key::max_idx;

    ObjC objects;
    IdxC indices;
    KeyC keys;
    IdxT free_list_head = free_list_null;

public:
    using key_type          = Key;
    using value_type        = T;
    using const_reference   = const T&;
    using reference         = T&;
    using const_iterator    = typename ObjC::const_iterator;
    using iterator          = typename ObjC::iterator;
    using size_type         = IdxT;
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
    [[nodiscard]] constexpr key_type insert(const value_type& val) { return emplace(val).key; }
    template<std::move_constructible = value_type>
    [[nodiscard]] constexpr key_type insert(value_type&& val) { return emplace(std::move(val)).key; }
    template<typename... Args>
        requires std::constructible_from<value_type, Args&&...>
    [[nodiscard]] constexpr emplace_result emplace(Args&&... args);
    constexpr iterator erase(iterator it) noexcept;
    constexpr void erase(key_type k) noexcept { erase_impl(index(k)); }
    constexpr void swap(SlotMap& other) noexcept;

    constexpr const_iterator find(key_type k) const noexcept;
    constexpr iterator find(key_type k) noexcept;
    constexpr const T& operator[](key_type k) const noexcept { return objects[index(k)]; }
    constexpr T& operator[](key_type k) noexcept { return objects[index(k)]; }
    constexpr bool contains(key_type k) const noexcept { return find(k) != end(); };
    template<Detail::SlotMapHasData = SlotMap>
    constexpr const value_type* data() const noexcept { return objects.data(); }
    template<Detail::SlotMapHasData = SlotMap>
    constexpr value_type* data() noexcept { return objects.data(); }

    constexpr bool operator==(const SlotMap& other) const noexcept;
    constexpr bool operator!=(const SlotMap& other) const noexcept { return not (*this == other); }

private:
    constexpr void erase_impl(IdxT erase_obj_idx) noexcept;

    constexpr IdxT index(key_type k) const noexcept {
        assert(k.idx < indices.size());
        auto idx = indices[k.idx].idx;
        assert(idx < objects.size());
        return idx;
    }
};

SLOTMAP_TEMPLATE_DECL
template<Detail::SlotMapHasReserve>
constexpr void SLOTMAP::reserve(size_type sz) {
    objects.reserve(sz);
    indices.reserve(sz);
    keys.reserve(sz);
}

SLOTMAP_TEMPLATE_DECL
template<Detail::SlotMapHasCapacity>
constexpr auto SLOTMAP::capacity() const noexcept -> size_type {
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

SLOTMAP_TEMPLATE_DECL
template<Detail::SlotMapHasShrinkToFit>
constexpr void SLOTMAP::shrink_to_fit() {
    objects.shrink_to_fit();
    indices.shrink_to_fit();
    keys.shrink_to_fit();
}

SLOTMAP_TEMPLATE_DECL
constexpr void SLOTMAP::clear() noexcept {
    objects.clear();
    indices.clear();
    keys.clear();
    free_list_head = free_list_null;
}

SLOTMAP_TEMPLATE_DECL
template<typename... Args>
    requires std::constructible_from<T, Args&&...>
constexpr auto SLOTMAP::emplace(Args&&... args) -> emplace_result {
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
    auto& ref = objects.emplace_back(std::forward<Args>(args)...);
    // Insert object into index-key map
    keys.back() = idx_idx;
    // Insert object into key-index map
    indices[idx_idx].idx = obj_idx;
    // Update free list
    free_list_head = free_list_next;

    return {
        .key = {
            .gen = gen,
            .idx = idx_idx,
        },
        .reference = ref,
    };
}

SLOTMAP_TEMPLATE_DECL
constexpr auto SLOTMAP::erase(iterator it) noexcept -> iterator {
    auto erase_obj_idx = std::distance(begin(), it);
    erase_impl(erase_obj_idx);
    return std::next(begin(), erase_obj_idx);
}

SLOTMAP_TEMPLATE_DECL
constexpr void SLOTMAP::erase_impl(IdxT erase_obj_idx) noexcept {
    auto back_idx_idx = keys.back();

    // Erase object from object array
    std::ranges::swap(objects[erase_obj_idx], objects.back());
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

SLOTMAP_TEMPLATE_DECL
constexpr void SLOTMAP::swap(SLOTMAP& other) noexcept {
    std::ranges::swap(objects, other.objects);
    std::ranges::swap(indices, other.indices);
    std::ranges::swap(keys, other.keys);
    std::ranges::swap(free_list_head, other.free_list_head);
}

#define SLOTMAP_FIND_DEF(k) \
    auto [key_gen, idx_idx] = k;\
    if (idx_idx < indices.size()) {\
        auto [gen, obj_idx] = indices[idx_idx];\
        if (key_gen == gen) {\
            auto it = std::next(begin(), obj_idx);\
            assert(it < end());\
            return it;\
        }\
    }\
    return end();

SLOTMAP_TEMPLATE_DECL
constexpr auto SLOTMAP::find(key_type k) const noexcept -> const_iterator {
    SLOTMAP_FIND_DEF(k);
}

SLOTMAP_TEMPLATE_DECL
constexpr auto SLOTMAP::find(key_type k) noexcept -> iterator {
    SLOTMAP_FIND_DEF(k);
}
#undef SLOTMAP_FIND_DEF

SLOTMAP_TEMPLATE_DECL
constexpr bool SLOTMAP::operator==(const SlotMap& other) const noexcept {
    return std::is_permutation(
        objects.begin(), objects.end(),
        other.objects.begin(), other.objects.end());
}

SLOTMAP_TEMPLATE_DECL
constexpr void swap(SLOTMAP& l, SLOTMAP& r) noexcept {
    l.swap(r);
}

#undef SLOTMAP_TEMPLATE_DECL
#undef SLOTMAP
}
