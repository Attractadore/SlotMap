#pragma once
#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace Attractadore::detail {
template <typename T1, typename T2>
struct Cpp23Pair : public std::pair<T1, T2> {
  using typename std::pair<T1, T2>::first_type;
  using typename std::pair<T1, T2>::second_type;
  using std::pair<T1, T2>::first;
  using std::pair<T1, T2>::second;
  using std::pair<T1, T2>::pair;

  template <typename U1, typename U2>
  constexpr Cpp23Pair(std::pair<U1, U2> &p)
      : std::pair<T1, T2>::pair(p.first, p.second) {}
  template <typename U1, typename U2>
  constexpr Cpp23Pair(const std::pair<U1, U2> &&p)
      : std::pair<T1, T2>::pair(std::forward<const U1>(p.first),
                                std::forward<const U2>(p.second)) {}

  constexpr const Cpp23Pair &operator=(const std::pair<T1, T2> &other) const
    requires std::is_copy_assignable_v<const T1> and
             std::is_copy_assignable_v<const T2>
  {
    first = other.first;
    second = other.second;
    return *this;
  }

  template <typename U1, typename U2>
  constexpr const Cpp23Pair &operator=(const std::pair<U1, U2> &other) const
    requires std::is_assignable_v<const T1 &, const U1 &> and
             std::is_assignable_v<const T2 &, const U2 &>
  {
    first = other.first;
    second = other.second;
    return *this;
  }

  constexpr const Cpp23Pair &operator=(std::pair<T1, T2> &&other) const
    requires std::is_assignable_v<const T1 &, T1> and
             std::is_assignable_v<const T2 &, T2>
  {
    first = other.first;
    second = other.second;
    return *this;
  }

  template <typename U1, typename U2>
  constexpr const Cpp23Pair &operator=(std::pair<U1, U2> &&other) const
    requires std::is_assignable_v<const T1 &, U1> and
             std::is_assignable_v<const T2 &, U2>
  {
    first = std::forward<U1>(other.first);
    second = std::forward<U2>(other.second);
    return *this;
  }

  constexpr void swap(const std::pair<T1, T2> &other) const
      noexcept(std::is_nothrow_swappable_v<const first_type>
                   and std::is_nothrow_swappable_v<const second_type>)
    requires std::is_swappable_v<const T1> and std::is_swappable_v<const T2>
  {
    using std::swap;
    swap(first, other.first);
    swap(second, other.second);
  }
};

template <typename T1, typename T2>
constexpr void swap(Cpp23Pair<T1, T2> &l,
                    Cpp23Pair<T1, T2> &r) noexcept(noexcept(l.swap(r)))
  requires std::is_swappable_v<typename Cpp23Pair<T1, T2>::first_type> and
           std::is_swappable_v<typename Cpp23Pair<T1, T2>::second_type>
{
  l.swap(r);
}

template <typename T1, typename T2>
constexpr void swap(const Cpp23Pair<T1, T2> &l,
                    const Cpp23Pair<T1, T2> &r) noexcept(noexcept(l.swap(r)))
  requires std::is_swappable_v<const typename Cpp23Pair<T1, T2>::first_type> and
           std::is_swappable_v<const typename Cpp23Pair<T1, T2>::second_type>
{
  l.swap(r);
}
} // namespace Attractadore::detail

template <typename T1, typename T2, typename U1, typename U2,
          template <typename> typename TQual,
          template <typename> typename UQual>
  requires requires {
             typename Attractadore::detail::Cpp23Pair<
                 std::common_reference_t<TQual<T1>, UQual<U1>>,
                 std::common_reference_t<TQual<T2>, UQual<U2>>>;
           }
struct std::basic_common_reference<Attractadore::detail::Cpp23Pair<T1, T2>,
                                   Attractadore::detail::Cpp23Pair<U1, U2>,
                                   TQual, UQual> {
  using type = Attractadore::detail::Cpp23Pair<
      std::common_reference_t<TQual<T1>, UQual<U1>>,
      std::common_reference_t<TQual<T2>, UQual<U2>>>;
};

namespace Attractadore {
namespace detail {

template <std::random_access_iterator Iter1, std::random_access_iterator Iter2>
class ZipIterator {
  Iter1 it1;
  Iter2 it2;

  using difference_type1 = std::iter_difference_t<Iter1>;
  using difference_type2 = std::iter_difference_t<Iter2>;
  using reference1 = std::iter_reference_t<Iter1>;
  using reference2 = std::iter_reference_t<Iter2>;
  using rvalue_reference1 = std::iter_rvalue_reference_t<Iter1>;
  using rvalue_reference2 = std::iter_rvalue_reference_t<Iter2>;
  using value_type1 = std::iter_value_t<Iter1>;
  using value_type2 = std::iter_value_t<Iter2>;

public:
  using difference_type =
      std::common_type_t<difference_type1, difference_type2>;
  using reference = Cpp23Pair<reference1, reference2>;
  using rvalue_reference = Cpp23Pair<rvalue_reference1, rvalue_reference2>;
  using value_type = Cpp23Pair<value_type1, value_type2>;

  constexpr ZipIterator(Iter1 it1 = {}, Iter2 it2 = {}) noexcept
      : it1{it1}, it2{it2} {}

  // InputIterator

  constexpr reference operator*() const noexcept { return {*it1, *it2}; }

  constexpr auto operator->() const noexcept {
    struct ProxyPointer : reference {
      using reference::reference;
      const ProxyPointer *operator->() const noexcept { return this; }
    };
    return ProxyPointer{*it1, *it2};
  }

  constexpr ZipIterator &operator++() noexcept {
    ++it1;
    ++it2;
    return *this;
  }

  constexpr ZipIterator operator++(int) const noexcept {
    auto temp = *this;
    ++(*this);
    return temp;
  }

  // ForwardIterator

  constexpr bool operator==(const ZipIterator &other) const noexcept {
    bool is_equal1 = it1 == other.it1;
    bool is_equal2 = it2 == other.it2;
    assert(is_equal1 == is_equal2);
    return is_equal1;
  }

  // BidirectionalIterator

  constexpr ZipIterator &operator--() noexcept {
    --it1;
    --it2;
    return *this;
  }

  constexpr ZipIterator operator--(int) const noexcept {
    auto temp = *this;
    --(*this);
    return temp;
  }

  // RandomAccessIterator

  constexpr std::weak_ordering
  operator<=>(const ZipIterator &other) const noexcept {
    auto ord = std::weak_order(it1, other.it1);
    assert(ord == std::weak_order(it2, other.it2));
    return ord;
  }

  constexpr difference_type operator-(const ZipIterator &other) const noexcept {
    auto d = it1 - other.it1;
    assert(d == (it2 - other.it2));
    return d;
  }

  constexpr ZipIterator operator+(difference_type d) const noexcept {
    auto temp = *this;
    return temp += d;
  }

  constexpr ZipIterator operator-(difference_type d) const noexcept {
    auto temp = *this;
    return temp -= d;
  }

  constexpr ZipIterator &operator+=(difference_type d) noexcept {
    it1 += d;
    it2 += d;
    return *this;
  }

  constexpr ZipIterator &operator-=(difference_type d) noexcept {
    it1 -= d;
    it2 -= d;
    return *this;
  }

  constexpr reference operator[](difference_type idx) const noexcept {
    return {it1[idx], it2[idx]};
  }

  friend constexpr rvalue_reference
  iter_move(ZipIterator it) noexcept(noexcept(std::ranges::iter_move(
      it.it1)) and noexcept(std::ranges::iter_move(it.it2))) {
    return {std::ranges::iter_move(it.it1), std::ranges::iter_move(it.it2)};
  }

  friend constexpr void iter_swap(ZipIterator lit, ZipIterator rit) noexcept(
      noexcept(std::ranges::iter_swap(lit.it1, rit.it1)) and noexcept(
          std::ranges::iter_swap(lit.it2, rit.it2))) {
    std::ranges::iter_swap(lit.it1, rit.it1);
    std::ranges::iter_swap(lit.it2, rit.it2);
  }
};

template <typename Iter1, typename Iter2>
constexpr ZipIterator<Iter1, Iter2>
operator+(typename ZipIterator<Iter1, Iter2>::difference_type d,
          ZipIterator<Iter1, Iter2> it) noexcept {
  return it + d;
}

template <typename T> constexpr bool EnableSlotMapKey = false;

template <typename T> using StdVector = std::vector<T>;

} // namespace detail

template <typename K>
concept CSlotMapKey = detail::EnableSlotMapKey<K>;

class SlotMapKey;

template <typename T, CSlotMapKey K = SlotMapKey,
          template <typename> typename C = detail::StdVector>
class SlotMap;

#define ATTRACTADORE_DEFINE_SLOTMAP_KEY(NewKey)                                \
  class NewKey {                                                               \
    template <typename T, ::Attractadore::CSlotMapKey K,                       \
              template <typename> typename C>                                  \
    friend class ::Attractadore::SlotMap;                                      \
    uint32_t slot_index = std::numeric_limits<uint32_t>::max();                \
    uint32_t version = 0;                                                      \
                                                                               \
    explicit NewKey(uint32_t index, uint32_t version = 0)                      \
        : slot_index(index), version(version) {}                               \
                                                                               \
  public:                                                                      \
    NewKey() = default;                                                        \
    constexpr auto operator<=>(const NewKey &other) const = default;           \
                                                                               \
    constexpr bool is_null() const noexcept {                                  \
      return slot_index == std::numeric_limits<uint32_t>::max();               \
    }                                                                          \
  };                                                                           \
                                                                               \
  template <>                                                                  \
  inline constexpr bool ::Attractadore::detail::EnableSlotMapKey<NewKey> =     \
      true;                                                                    \
                                                                               \
  static_assert(::Attractadore::CSlotMapKey<NewKey>)

ATTRACTADORE_DEFINE_SLOTMAP_KEY(SlotMapKey);

namespace detail {

template <typename Container> struct ContainerView : private Container {
  template <typename T, ::Attractadore::CSlotMapKey K,
            template <typename> typename C>
  friend class ::Attractadore::SlotMap;

  using typename Container::const_iterator;
  using typename Container::difference_type;
  using typename Container::iterator;
  using typename Container::size_type;
  using typename Container::value_type;

  constexpr const_iterator cbegin() const noexcept { return begin(); }

  constexpr const_iterator cend() const noexcept { return end(); }

  using Container::begin;
  using Container::end;

  constexpr bool empty() const noexcept { return begin() == end(); }

  constexpr size_type size() const noexcept {
    return static_cast<size_type>(std::ranges::distance(begin(), end()));
  }

  constexpr difference_type ssize() const noexcept {
    return std::ranges::distance(begin(), end());
  }

  constexpr const value_type &front() const noexcept {
    assert(not empty());
    return *begin();
  }

  constexpr value_type &front() noexcept {
    assert(not empty());
    return *begin();
  }

  constexpr const value_type &back() const noexcept {
    assert(not empty());
    return *--end();
  }

  constexpr value_type &back() noexcept {
    assert(not empty());
    return *--end();
  }

  constexpr const value_type &operator[](size_type idx) const noexcept {
    return begin()[idx];
  }

  constexpr value_type &operator[](size_type idx) noexcept {
    return begin()[idx];
  }

  constexpr const value_type *cdata() const noexcept
    requires requires {
               {
                 ContainerView::data()
                 } -> std::convertible_to<const value_type *>;
             }
  {
    return ContainerView::data();
  }

  constexpr const value_type *data() const noexcept
    requires requires {
               {
                 ContainerView::data()
                 } -> std::convertible_to<const value_type *>;
             }
  {
    return ContainerView::data();
  }

  constexpr value_type *data() noexcept
    requires requires {
               { ContainerView::data() } -> std::convertible_to<value_type *>;
             }
  {
    return ContainerView::data();
  }
};

} // namespace detail

template <typename T, CSlotMapKey K, template <typename> typename C>
class SlotMap {
  static constexpr auto NULL_SLOT = std::numeric_limits<uint32_t>::max();

  struct Slot {
    union {
      uint32_t index;
      uint32_t next_free;
    };
    uint32_t version;
  };

  using Keys = C<K>;
  using Values = C<T>;
  using Slots = C<Slot>;
  using KeyView = detail::ContainerView<Keys>;
  using ValueView = detail::ContainerView<Values>;

  Keys m_keys;
  Values m_values;
  Slots m_slots;

  struct FreeHead {
    uint32_t value = NULL_SLOT;
    FreeHead() = default;
    FreeHead(const FreeHead &other) = default;
    FreeHead(FreeHead &other) noexcept
        : value(std::exchange(other.value, NULL_SLOT)) {}
    FreeHead &operator=(const FreeHead &other) = default;
    FreeHead &operator=(FreeHead &other) noexcept {
      value = other.value;
      other.value = NULL_SLOT;
      return *this;
    }
    FreeHead &operator=(uint32_t new_value) noexcept {
      value = new_value;
      return *this;
    }
    operator uint32_t() const noexcept { return value; }
  } m_free_head;

  static_assert(std::ranges::borrowed_range<const KeyView &>);
  static_assert(std::ranges::borrowed_range<const ValueView &>);
  static_assert(std::ranges::borrowed_range<ValueView &>);
  static_assert(std::ranges::random_access_range<const KeyView &>);
  static_assert(std::ranges::random_access_range<const ValueView &>);
  static_assert(std::ranges::random_access_range<ValueView &>);

  using const_key_iterator = typename KeyView::const_iterator;

  using const_value_iterator = typename ValueView::const_iterator;
  using value_iterator = typename ValueView::iterator;

public:
  using key_type = K;
  using value_type = T;
  using const_iterator =
      detail::ZipIterator<const_key_iterator, const_value_iterator>;
  using iterator = detail::ZipIterator<const_key_iterator, value_iterator>;
  using const_reference = typename const_iterator::reference;
  using reference = typename iterator::reference;
  using difference_type = std::iter_difference_t<iterator>;
  using size_type = std::make_unsigned_t<difference_type>;

  constexpr const auto &keys() const noexcept {
    return static_cast<const KeyView &>(m_keys);
  }

  constexpr const auto &values() const noexcept {
    return static_cast<const ValueView &>(m_values);
  }

  constexpr auto &values() noexcept {
    return static_cast<ValueView &>(m_values);
  }

  constexpr const_iterator cbegin() const noexcept { return begin(); }

  constexpr const_iterator cend() const noexcept { return end(); }

  constexpr const_iterator begin() const noexcept {
    return {keys().begin(), values().begin()};
  }

  constexpr const_iterator end() const noexcept {
    return {keys().end(), values().end()};
  }

  constexpr iterator begin() noexcept {
    return {keys().begin(), values().begin()};
  }

  constexpr iterator end() noexcept { return {keys().end(), values().end()}; }

  constexpr bool empty() const noexcept { return begin() == end(); }

  static constexpr size_type max_size() noexcept { return NULL_SLOT - 1; }

  constexpr size_type size() const noexcept {
    return static_cast<size_type>(std::ranges::distance(begin(), end()));
  }

  constexpr difference_type ssize() const noexcept {
    return std::ranges::distance(begin(), end());
  }

  constexpr const_reference front() const noexcept {
    assert(not empty());
    return *begin();
  }

  constexpr reference front() noexcept {
    assert(not empty());
    return *begin();
  }

  constexpr const_reference back() const noexcept {
    assert(not empty());
    return *--end();
  }

  constexpr reference back() noexcept {
    assert(not empty());
    return *--end();
  }

  constexpr void reserve(size_type capacity)
    requires requires(size_type capacity) {
               m_keys.reserve(capacity);
               m_values.reserve(capacity);
               m_slots.reserve(capacity);
             }
  {
    m_keys.reserve(capacity);
    m_values.reserve(capacity);
    m_slots.reserve(capacity);
  }

  constexpr size_type capacity() const noexcept
    requires requires {
               { m_keys.capacity() } -> std::convertible_to<size_type>;
               { m_values.capacity() } -> std::convertible_to<size_type>;
               { m_slots.capacity() } -> std::convertible_to<size_type>;
             }
  {
    return std::min({
        static_cast<size_type>(m_keys.capacity()),
        static_cast<size_type>(m_values.capacity()),
        static_cast<size_type>(m_slots.capacity()),
    });
  }

  constexpr void shrink_to_fit() noexcept
    requires requires {
               m_keys.shrink_to_fit();
               m_values.shrink_to_fit();
               m_slots.shrink_to_fit();
             }
  {
    m_keys.shrink_to_fit();
    m_values.shrink_to_fit();
    m_slots.shrink_to_fit();
  }

  constexpr void clear() noexcept {
    // Push all objects into free list to preserve version info
    for (auto [slot_index, version] : m_keys) {
      m_slots[slot_index] = {
          .next_free = std::exchange(m_free_head, slot_index),
          .version = version + 1,
      };
    }
    m_keys.clear();
    m_values.clear();
  }

  [[nodiscard]] constexpr key_type insert(const value_type &value)
    requires std::copy_constructible<value_type>
  {
    return emplace(value)->first;
  }

  [[nodiscard]] constexpr key_type insert(value_type &&val)
    requires std::move_constructible<value_type>
  {
    return emplace(std::move(val))->first;
  }

  template <typename... Args>
    requires std::constructible_from<value_type, Args &&...>
  [[nodiscard]] constexpr iterator emplace(Args &&...args) {
    uint32_t index = m_keys.size();
    auto key = [&] {
      if (m_free_head == NULL_SLOT) {
        assert(m_keys.size() == m_slots.size());
        m_slots.push_back({.index = index});
        auto slot_index = index;
        return key_type(slot_index);
      } else {
        uint32_t slot_index = m_free_head;
        auto &slot = m_slots[slot_index];
        m_free_head = slot.next_free;
        slot.index = index;
        return key_type(slot_index, slot.version);
      }
    }();
    m_keys.push_back(key);
    m_values.emplace_back(std::forward<Args>(args)...);
    return std::ranges::next(begin(), index);
  }

  constexpr iterator erase(iterator it) noexcept {
    auto index = std::ranges::distance(begin(), it);
    erase(index);
    return std::ranges::next(begin(), index);
  }

  constexpr void erase(key_type k) noexcept { erase(index(k)); }

  [[nodiscard]] constexpr bool try_erase(key_type k) noexcept {
    auto it = find(k);
    if (it != end()) {
      erase(it);
      return true;
    }
    return false;
  }

  [[nodiscard]] constexpr value_type pop(key_type k) noexcept {
    auto erase_index = index(k);
    auto temp = std::exchange(m_values[erase_index], m_values.back());
    m_values.pop_back();
    erase_only_key(erase_index);
    return temp;
  }

  [[nodiscard]] constexpr std::optional<value_type>
  try_pop(key_type key) noexcept {
    auto it = find(key);
    if (it != end()) {
      auto erase_index = std::ranges::distance(begin(), it);
      auto temp = std::exchange(m_values[erase_index], m_values.back());
      m_values.pop_back();
      erase_only_key(erase_index);
      return std::move(temp);
    }
    return std::nullopt;
  }

  constexpr void swap(SlotMap &other) noexcept {
    std::ranges::swap(m_keys, other.m_keys);
    std::ranges::swap(m_values, other.m_values);
    std::ranges::swap(m_slots, other.m_slots);
    std::ranges::swap(m_free_head, other.m_free_head);
  }

#define attractadore_slotmap_find(k)                                           \
  auto slot_index = k.slot_index;                                              \
  assert(slot_index < m_slots.size());                                         \
  auto &slot = m_slots[slot_index];                                            \
  if (slot.version == k.version) {                                             \
    return std::ranges::next(begin(), slot.index);                             \
  }                                                                            \
  return end();

  constexpr const_iterator find(key_type k) const noexcept {
    attractadore_slotmap_find(k);
  }

  constexpr iterator find(key_type k) noexcept { attractadore_slotmap_find(k); }

#undef attractadore_slotmap_find

  constexpr const value_type *get(key_type key) const noexcept {
    auto it = find(key);
    return it != end() ? &it->second : nullptr;
  }

  constexpr value_type *get(key_type key) noexcept {
    auto it = find(key);
    return it != end() ? &it->second : nullptr;
  }

  constexpr const value_type &operator[](key_type k) const noexcept {
    return m_values[index(k)];
  }

  constexpr value_type &operator[](key_type k) noexcept {
    return m_values[index(k)];
  }

  constexpr bool contains(key_type k) const noexcept {
    return find(k) != end();
  };

  constexpr bool operator==(const SlotMap &other) const noexcept {
    return this->m_keys == other.m_keys and this->m_values == other.m_values;
  }

private:
  constexpr uint32_t index(key_type k) const noexcept {
    assert(k.slot_index < m_slots.size());
    auto slot = m_slots[k.slot_index];
    assert(slot.index < m_values.size());
    return slot.index;
  }

  constexpr void erase(uint32_t index) noexcept {
    assert(index < size());
    // Erase object from object array
    std::ranges::swap(m_values[index], m_values.back());
    m_values.pop_back();
    erase_only_key(index);
  }

  constexpr void erase_only_key(uint32_t index) noexcept {
    auto back_key = m_keys.back();
    auto erase_key = std::exchange(m_keys[index], back_key);
    m_keys.pop_back();
    // Order important for back_key = erase_key
    auto &back_slot = m_slots[back_key.slot_index];
    auto &erase_slot = m_slots[erase_key.slot_index];
    back_slot.index = index;
    erase_slot = {
        .next_free = std::exchange(m_free_head, erase_key.slot_index),
        .version = erase_key.version + 1,
    };
  }
};

template <typename T, CSlotMapKey K, template <typename> typename C>
constexpr void swap(SlotMap<T, K, C> &l, SlotMap<T, K, C> &r) noexcept {
  l.swap(r);
}

} // namespace Attractadore
