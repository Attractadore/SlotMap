#pragma once
#include <algorithm>
#include <cassert>
#include <utility>

namespace Attractadore::SlotMapNameSpace {
template<typename T1, typename T2>
struct cpp23_pair: public std::pair<T1, T2> {
    using typename std::pair<T1, T2>::first_type;
    using typename std::pair<T1, T2>::second_type;
    using std::pair<T1, T2>::first;
    using std::pair<T1, T2>::second;
    using std::pair<T1, T2>::pair;

    template<typename U1, typename U2>
    constexpr cpp23_pair(std::pair<U1, U2>& p):
        std::pair<T1, T2>::pair(p.first, p.second) {}
    template<typename U1, typename U2>
    constexpr cpp23_pair(const std::pair<U1, U2>&& p):
        std::pair<T1, T2>::pair(std::forward<const U1>(p.first), std::forward<const U2>(p.second)) {}

    constexpr const cpp23_pair& operator=(const std::pair<T1, T2>& other) const requires
        std::is_copy_assignable_v<const T1> and
        std::is_copy_assignable_v<const T2>
    {
        first = other.first;
        second = other.second;
        return *this;
    }

    template<typename U1, typename U2>
    constexpr const cpp23_pair& operator=(const std::pair<U1, U2>& other) const requires
        std::is_assignable_v<const T1&, const U1&> and
        std::is_assignable_v<const T2&, const U2&>
    {
        first = other.first;
        second = other.second;
        return *this;
    }

    constexpr const cpp23_pair& operator=(std::pair<T1, T2>&& other) const requires
        std::is_assignable_v<const T1&, T1> and
        std::is_assignable_v<const T2&, T2>
    {
        first = other.first;
        second = other.second;
        return *this;
    }

    template<typename U1, typename U2>
    constexpr const cpp23_pair& operator=(std::pair<U1, U2>&& other) const requires
        std::is_assignable_v<const T1&, U1> and
        std::is_assignable_v<const T2&, U2>
    {
        first = std::forward<U1>(other.first);
        second = std::forward<U2>(other.second);
        return *this;
    }

    constexpr void swap(const std::pair<T1, T2>& other) const noexcept (
        std::is_nothrow_swappable_v<const first_type> and
        std::is_nothrow_swappable_v<const second_type>
    ) requires
        std::is_swappable_v<const T1> and
        std::is_swappable_v<const T2>
    {
        using std::swap;
        swap(first, other.first);
        swap(second, other.second);
    }
};

template<typename T1, typename T2>
constexpr void swap(cpp23_pair<T1,T2>& l, cpp23_pair<T1,T2>& r) noexcept (
    noexcept(l.swap(r))
) requires
    std::is_swappable_v<typename cpp23_pair<T1, T2>::first_type> and
    std::is_swappable_v<typename cpp23_pair<T1, T2>::second_type>
{
    l.swap(r);
}

template<typename T1, typename T2>
constexpr void swap(const cpp23_pair<T1,T2>& l, const cpp23_pair<T1,T2>& r) noexcept (
    noexcept(l.swap(r))
) requires
    std::is_swappable_v<const typename cpp23_pair<T1,T2>::first_type> and
    std::is_swappable_v<const typename cpp23_pair<T1,T2>::second_type>
{
    l.swap(r);
}
}

template<
    typename T1, typename T2,
    typename U1, typename U2,
    template<typename> typename TQual,
    template<typename> typename UQual
> requires requires {
    typename Attractadore::SlotMapNameSpace::cpp23_pair<
        std::common_reference_t<TQual<T1>, UQual<U1>>,
        std::common_reference_t<TQual<T2>, UQual<U2>>>;
}
struct std::basic_common_reference<
    Attractadore::SlotMapNameSpace::cpp23_pair<T1, T2>,
    Attractadore::SlotMapNameSpace::cpp23_pair<U1, U2>,
    TQual,
    UQual>
{
    using type = Attractadore::SlotMapNameSpace::cpp23_pair<
        std::common_reference_t<TQual<T1>, UQual<U1>>,
        std::common_reference_t<TQual<T2>, UQual<U2>>>;
};

namespace Attractadore::SlotMapNameSpace {
template<std::random_access_iterator Iter1, std::random_access_iterator Iter2>
class ZipIterator {
    Iter1 it1;
    Iter2 it2;

    using difference_type1  = std::iter_difference_t<Iter1>;
    using difference_type2  = std::iter_difference_t<Iter2>;
    using reference1        = std::iter_reference_t<Iter1>;
    using reference2        = std::iter_reference_t<Iter2>;
    using rvalue_reference1 = std::iter_rvalue_reference_t<Iter1>;
    using rvalue_reference2 = std::iter_rvalue_reference_t<Iter2>;
    using value_type1       = std::iter_value_t<Iter1>;
    using value_type2       = std::iter_value_t<Iter2>;

public:
    using difference_type   = std::common_type_t<difference_type1, difference_type2>;
    using reference         = cpp23_pair<reference1, reference2>;
    using rvalue_reference  = cpp23_pair<rvalue_reference1, rvalue_reference2>;
    using value_type        = cpp23_pair<value_type1, value_type2>;

    constexpr ZipIterator(Iter1 it1 = {}, Iter2 it2 = {}) noexcept:
        it1{it1}, it2{it2} {}

    // InputIterator

    constexpr reference operator*() const noexcept {
        return {*it1, *it2};
    }

    constexpr auto operator->() const noexcept {
        struct ProxyPointer: reference {
            using reference::reference;
            const ProxyPointer* operator->() const noexcept {
                return this;
            }
        };
        return ProxyPointer{*it1, *it2};
    }

    constexpr ZipIterator& operator++() noexcept {
        ++it1;
        ++it2;
        return *this;
    }

    constexpr ZipIterator  operator++(int) const noexcept {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    // ForwardIterator

    constexpr bool operator==(const ZipIterator& other) const noexcept {
        bool is_equal1 = it1 == other.it1;
        bool is_equal2 = it2 == other.it2;
        assert(is_equal1 == is_equal2);
        return is_equal1;
    }

    // BidirectionalIterator

    constexpr ZipIterator& operator--() noexcept {
        --it1;
        --it2;
        return *this;
    }

    constexpr ZipIterator  operator--(int) const noexcept {
        auto temp = *this;
        --(*this);
        return temp;
    }

    // RandomAccessIterator

    constexpr std::weak_ordering operator<=>(const ZipIterator& other) const noexcept {
        auto ord_1 = std::weak_order(it1, other.it1);
        auto ord_2 = std::weak_order(it2, other.it2);
        assert(ord_1 == ord_2);
        return ord_1;
    }

    constexpr difference_type operator-(const ZipIterator& other) const noexcept {
        auto d1 = it1 - other.it1;
        auto d2 = it2 - other.it2;
        assert(d1 == d2);
        return d1;
    }

    constexpr ZipIterator  operator+(difference_type d) const noexcept {
        auto temp = *this;
        return temp += d;
    }

    constexpr ZipIterator  operator-(difference_type d) const noexcept {
        auto temp = *this;
        return temp -= d;
    }

    constexpr ZipIterator& operator+=(difference_type d) noexcept {
        it1 += d;
        it2 += d;
        return *this;
    }

    constexpr ZipIterator& operator-=(difference_type d) noexcept {
        it1 -= d;
        it2 -= d;
        return *this;
    }

    constexpr reference operator[](difference_type idx) const noexcept {
        return {it1[idx], it2[idx]};
    }

    friend constexpr rvalue_reference iter_move(ZipIterator it) noexcept (
        noexcept(std::ranges::iter_move(it.it1)) and
        noexcept(std::ranges::iter_move(it.it2))
    ) {
        return {std::ranges::iter_move(it.it1), std::ranges::iter_move(it.it2)};
    }

    friend constexpr void iter_swap(ZipIterator lit, ZipIterator rit) noexcept (
        noexcept(std::ranges::iter_swap(lit.it1, rit.it1)) and
        noexcept(std::ranges::iter_swap(lit.it2, rit.it2))
    ) {
        std::ranges::iter_swap(lit.it1, rit.it1);
        std::ranges::iter_swap(lit.it2, rit.it2);
    }
};

template<std::forward_iterator Iter1, std::forward_iterator Iter2>
constexpr ZipIterator<Iter1, Iter2> operator+(
    typename ZipIterator<Iter1, Iter2>::difference_type d,
    ZipIterator<Iter1, Iter2> it
) noexcept {
    return it + d;
}

template<typename Container, typename Friend>
struct ContainerView: private Container {
    friend Friend;
    using Container::begin;
    using Container::end;
    constexpr auto cbegin() const noexcept { return begin(); }
    constexpr auto cend() const noexcept { return end(); }
    constexpr bool empty() const noexcept { return begin() == end(); }
    constexpr auto size() const noexcept { return std::ranges::distance(begin(), end()); }
    constexpr decltype(auto) operator[](typename Container::size_type idx) const noexcept { return begin()[idx]; }
    constexpr decltype(auto) operator[](typename Container::size_type idx) noexcept { return begin()[idx]; }
};

template<std::unsigned_integral I>
struct Key {
    using idx_type = I;
    constexpr auto operator<=>(const Key&) const noexcept = default;
    I idx;
};

template<typename Key, typename Value>
struct EmplaceResult {
    Key     key;
    Value&  ref;
};

template<typename C>
concept HasReserve = requires (C c) {
    c.reserve(std::declval<typename C::size_type>());
};

template<typename SlotMap>
concept SlotMapCanReserve =
    HasReserve<typename SlotMap::ObjC> and
    HasReserve<typename SlotMap::KeyC>;

template<typename C>
concept HasCapacity = requires (C c) {
    c.capacity();
};

template<typename SlotMap>
concept SlotMapHasCapacity =
    HasCapacity<typename SlotMap::ObjC> and
    HasCapacity<typename SlotMap::KeyC>;

template<typename C>
concept HasShrinkToFit = requires (C c) {
    c.shrink_to_fit();
};

template<typename SlotMap>
concept SlotMapCanShrinkToFit =
    HasShrinkToFit<typename SlotMap::ObjC> and
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
    typename I,
    template <typename> typename ObjectContainer,
    template <typename> typename KeyContainer
>
concept SlotMapRequires =
    std::unsigned_integral<I> and
    std::is_nothrow_move_constructible_v<T> and
    std::is_nothrow_move_assignable_v<T> and
    requires (ObjectContainer<T> c) {
        { std::ranges::swap(c, c) } noexcept;
    } and
    requires (KeyContainer<I> c) {
        { std::ranges::swap(c, c) } noexcept;
    };

#define SLOTMAP_TEMPLATE template<\
    typename T,\
    std::unsigned_integral I,\
    template <typename> typename O,\
    template <typename> typename K\
> requires SlotMapRequires<T, I, O, K>
#define SLOTMAP SlotMap<T, I, O, K>

SLOTMAP_TEMPLATE
class SlotMapBase {
protected:
    using key_type      = Key<I>;
    using index_type    = typename key_type::idx_type;

    using ObjC          = O<T>;
    using IdxC          = K<index_type>;
    using KeyC          = K<key_type>;
    using ObjV          = ContainerView<ObjC, SlotMapBase>;
    using IdxV          = ContainerView<IdxC, SlotMapBase>;
    using KeyV          = ContainerView<KeyC, SlotMapBase>;

    ObjC m_objects;
    IdxC m_indices;
    KeyC m_keys;

public:
    constexpr const KeyV& keys() const noexcept {
        return static_cast<const KeyV&>(m_keys);
    }
    constexpr const ObjV& values() const noexcept {
        return static_cast<const ObjV&>(m_objects);
    }
    constexpr ObjV& values() noexcept {
        return static_cast<ObjV&>(m_objects);
    }
};

template<
    typename T,
    std::unsigned_integral KeyType,
    template <typename> typename ObjectContainer,
    template <typename> typename KeyContainer = ObjectContainer
> requires SlotMapRequires<T, KeyType, ObjectContainer, KeyContainer>
class SlotMap: public SlotMapBase<T, KeyType, ObjectContainer, KeyContainer> {
    using Base = SlotMapBase<T, KeyType, ObjectContainer, KeyContainer>;
    using typename Base::index_type;
    using Base::m_objects;
    using Base::m_indices;
    using Base::m_keys;

    using const_key_view        = decltype(std::declval<const Base*>()->keys());
    using const_value_view      = decltype(std::declval<const Base*>()->values());
    using value_view            = decltype(std::declval<Base*>()->values());
    static_assert(std::ranges::borrowed_range<const_key_view>);
    static_assert(std::ranges::borrowed_range<const_value_view>);
    static_assert(std::ranges::borrowed_range<value_view>);
    static_assert(std::ranges::random_access_range<const_key_view>);
    static_assert(std::ranges::random_access_range<const_value_view>);
    static_assert(std::ranges::random_access_range<value_view>);

    using const_key_iterator    = std::ranges::iterator_t<const_key_view>;

    using const_value_iterator  = std::ranges::iterator_t<const_value_view>;
    using value_iterator        = std::ranges::iterator_t<value_view>;

    static constexpr index_type free_list_null = std::numeric_limits<KeyType>::max();
    index_type m_free_list_head = free_list_null;

public:
    using typename Base::key_type;
    using value_type        = T;
    using const_reference   = const T&;
    using reference         = T&;

    using const_iterator    = ZipIterator<const_key_iterator, const_value_iterator>;
    using iterator          = ZipIterator<const_key_iterator, value_iterator>;

    using difference_type   = std::iter_difference_t<iterator>;
    using size_type         = std::make_signed_t<difference_type>;

    using emplace_result    = EmplaceResult<key_type, value_type>;

    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }
    constexpr const_iterator begin() const noexcept {
        return { keys().begin(), values().begin() };
    }
    constexpr const_iterator end() const noexcept {
        return { keys().end(), values().end() };
    }
    constexpr iterator begin() noexcept { return {
        keys().begin(), values().begin() };
    }
    constexpr iterator end() noexcept { return {
        keys().end(), values().end() };
    }

    constexpr bool empty() const noexcept { return begin() == end(); }
    constexpr size_type size() const noexcept { return std::ranges::distance(begin(), end()); }
    static constexpr size_type max_size() noexcept { return free_list_null - 1; }

    constexpr void reserve(size_type sz)
        requires SlotMapCanReserve<SlotMap>
    {
        m_objects.reserve(sz);
        m_indices.reserve(sz);
        m_keys.reserve(sz);
    }

    constexpr size_type capacity() const noexcept
        requires SlotMapHasCapacity<SlotMap>
    {
        auto cap = max_size();
        auto obj_cap = m_objects.capacity();
        if (obj_cap < cap) {
            cap = obj_cap;
        }
        auto idx_cap = m_indices.capacity();
        if (idx_cap < cap) {
            cap = idx_cap;
        }
        auto key_cap = m_keys.capacity();
        if (key_cap < cap) {
            cap = key_cap;
        }
        return cap;
    }

    constexpr void shrink_to_fit()
        requires SlotMapCanShrinkToFit<SlotMap>
    {
        m_objects.shrink_to_fit();
        m_indices.shrink_to_fit();
        m_keys.shrink_to_fit();
    }

    constexpr void clear() noexcept;

    [[nodiscard]] constexpr key_type insert(const value_type& val)
        requires std::copy_constructible<value_type> { return emplace(val).key; }
    [[nodiscard]] constexpr key_type insert(value_type&& val)
        requires std::move_constructible<value_type> { return emplace(std::move(val)).key; }
    template<typename... Args> requires std::constructible_from<value_type, Args&&...>
    [[nodiscard]] constexpr emplace_result emplace(Args&&... args);
    constexpr iterator erase(iterator it) noexcept;
    constexpr void erase(key_type k) noexcept { erase_impl(index(k)); }
    constexpr value_type pop(key_type k) noexcept;
    constexpr void swap(SlotMap& other) noexcept;

    constexpr const_iterator access(key_type k) const noexcept { return std::ranges::next(begin(), index(k)); }
    constexpr iterator access(key_type k) noexcept { return std::ranges::next(begin(), index(k)); };
    constexpr const_iterator find(key_type k) const noexcept;
    constexpr iterator find(key_type k) noexcept;
    constexpr const T& operator[](key_type k) const noexcept { return m_objects[index(k)]; }
    constexpr T& operator[](key_type k) noexcept { return m_objects[index(k)]; }
    constexpr bool contains(key_type k) const noexcept { return find(k) != end(); };
    constexpr const value_type* data() const noexcept
        requires SlotMapHasData<SlotMap> { return m_objects.data(); }
    constexpr value_type* data() noexcept
        requires SlotMapHasData<SlotMap> { return m_objects.data(); }
    using Base::keys;
    using Base::values;

    constexpr bool operator==(const SlotMap& other) const noexcept;

private:
    constexpr index_type index(key_type k) const noexcept {
        assert(k.idx < m_indices.size());
        auto idx = m_indices[k.idx];
        assert(idx < m_objects.size());
        return idx;
    }

    constexpr void erase_impl(index_type erase_obj_idx) noexcept;
    constexpr void erase_index_and_key(index_type erase_obj_idx) noexcept;
};

SLOTMAP_TEMPLATE
constexpr void SLOTMAP::clear() noexcept {
    m_objects.clear();
    // Push all objects into free list
    for (auto key: m_keys) {
        m_indices[key.idx] =
            std::exchange(m_free_list_head, key.idx);
    }
    m_keys.clear();
}

SLOTMAP_TEMPLATE
template<typename... Args>
    requires std::constructible_from<T, Args&&...>
constexpr auto SLOTMAP::emplace(Args&&... args) -> emplace_result {
    index_type obj_idx = m_objects.size();

    // Reserve space in key-index map
    if (m_free_list_head == free_list_null) {
        index_type free_list_next = m_indices.size();
        m_indices.emplace_back() =
            std::exchange(m_free_list_head, free_list_next);
    }

    // Reserve space in index-key map
    m_keys.resize(obj_idx + 1);

    auto idx_idx = m_free_list_head;
    auto free_list_next = m_indices[idx_idx];

    // Insert object into object array
    auto& ref = m_objects.emplace_back(std::forward<Args>(args)...);
    // Insert object into index-key map
    auto key = m_keys.back() = { .idx = idx_idx };
    // Insert object into key-index map
    m_indices[idx_idx] = obj_idx;
    // Update free list
    m_free_list_head = free_list_next;

    return {
        .key = key,
        .ref = ref,
    };
}

SLOTMAP_TEMPLATE
constexpr auto SLOTMAP::erase(iterator it) noexcept -> iterator {
    auto erase_obj_idx = std::ranges::distance(begin(), it);
    erase_impl(erase_obj_idx);
    return std::ranges::next(begin(), erase_obj_idx);
}

SLOTMAP_TEMPLATE
constexpr auto SLOTMAP::pop(key_type k) noexcept -> value_type {
    auto erase_obj_idx = index(k);
    // Erase object from object array
    auto temp = std::exchange(m_objects[erase_obj_idx], m_objects.back());
    m_objects.pop_back();
    erase_index_and_key(erase_obj_idx);
    return temp;
}

SLOTMAP_TEMPLATE
constexpr void SLOTMAP::erase_impl(index_type erase_obj_idx) noexcept {
    // Erase object from object array
    std::ranges::swap(m_objects[erase_obj_idx], m_objects.back());
    m_objects.pop_back();
    erase_index_and_key(erase_obj_idx);
}

SLOTMAP_TEMPLATE
constexpr void SLOTMAP::erase_index_and_key(index_type erase_obj_idx) noexcept {
    // Erase object from index-key map
    auto back_key = m_keys.back();
    auto erase_key = std::exchange(m_keys[erase_obj_idx], back_key);
    m_keys.pop_back();

    // Update key-index map for back object
    m_indices[back_key.idx] = erase_obj_idx;

    // Erase object from key-index map and update free list
    m_indices[erase_key.idx] =
        std::exchange(m_free_list_head, erase_key.idx);
}

SLOTMAP_TEMPLATE
constexpr void SLOTMAP::swap(SLOTMAP& other) noexcept {
    std::ranges::swap(m_objects, other.m_objects);
    std::ranges::swap(m_indices, other.m_indices);
    std::ranges::swap(m_keys, other.m_keys);
    std::ranges::swap(m_free_list_head, other.m_free_list_head);
}

#define SLOTMAP_FIND(k) \
    if (k.idx < m_indices.size()) {\
        auto obj_idx = m_indices[k.idx];\
        auto it = std::ranges::next(begin(), obj_idx);\
        if (it < end() and it->first == k) {\
            return it;\
        }\
    }\
    return end();

SLOTMAP_TEMPLATE
constexpr auto SLOTMAP::find(key_type k) const noexcept -> const_iterator {
    SLOTMAP_FIND(k);
}

SLOTMAP_TEMPLATE
constexpr auto SLOTMAP::find(key_type k) noexcept -> iterator {
    SLOTMAP_FIND(k);
}
#undef SLOTMAP_FIND

SLOTMAP_TEMPLATE
constexpr bool SLOTMAP::operator==(const SlotMap& other) const noexcept {
    return std::ranges::is_permutation(*this, other);
}

SLOTMAP_TEMPLATE
constexpr void swap(SLOTMAP& l, SLOTMAP& r) noexcept {
    l.swap(r);
}

#undef SLOTMAP_TEMPLATE
#undef SLOTMAP
}

namespace Attractadore {
using SlotMapNameSpace::SlotMap;
}
