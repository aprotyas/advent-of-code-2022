#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <locale>
#include <map>
#include <numeric>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class Rucksack {
 public:
  Rucksack(const std::string_view items)
      : size_{items.length() / 2},
        compartments_{std::make_pair(std::string{items, 0, size_},
                                     std::string{items, size_, size_})},
        common_item_{*std::find_first_of(
            compartments_.first.cbegin(), compartments_.first.cend(),
            compartments_.second.cbegin(), compartments_.second.cend())} {}
  const std::pair<std::string, std::string>& compartments() const {
    return compartments_;
  }
  char common_item() const { return common_item_; }
  std::size_t size() const { return size_; }
  std::string items() const {
    return std::apply(std::plus<std::string>{}, compartments_);
  }
  friend std::ostream& operator<<(std::ostream& os, const Rucksack& rucksack);

 private:
  std::size_t size_;
  std::pair<std::string, std::string> compartments_{};
  char common_item_;
};

std::ostream& operator<<(std::ostream& os, const Rucksack& rucksack) {
  os << rucksack.compartments_.first << ", " << rucksack.compartments_.second
     << ", " << rucksack.common_item_;
  return os;
}

std::size_t priority(const char ch) {
  constexpr std::size_t alphabet_size{static_cast<std::size_t>('z' - 'a') + 1};

  if (std::islower(ch, std::locale::classic())) {
    return static_cast<std::size_t>(ch - 'a') + 1;
  }
  return static_cast<std::size_t>(ch - 'A') + alphabet_size + 1;
}

template <std::size_t... Indices>
struct indices {
  using next = indices<Indices..., sizeof...(Indices)>;
};
template <std::size_t N>
struct build_indices {
  using type = typename build_indices<N - 1>::type::next;
};
template <>
struct build_indices<0> {
  using type = indices<>;
};
template <std::size_t N>
using BuildIndices = typename build_indices<N>::type;

template <typename Iterator>
using ValueType = typename std::iterator_traits<Iterator>::value_type;

// internal overload with indices tag
template <
    std::size_t... I, typename RandomAccessIterator,
    typename Array = std::array<ValueType<RandomAccessIterator>, sizeof...(I)>>
Array make_array(RandomAccessIterator first, indices<I...>) {
  return Array{{first[I]...}};
}

// externally visible interface
template <std::size_t N, typename RandomAccessIterator>
std::array<ValueType<RandomAccessIterator>, N> make_array(
    RandomAccessIterator first, RandomAccessIterator last) {
  return make_array(first, BuildIndices<N>{});
}

constexpr std::size_t group_size = 3;
using Group = std::array<Rucksack, group_size>;

char common_item(const Group& group) {
  std::map<char, std::size_t> seen;
  for (const auto& r : group) {
    std::map<char, std::size_t> items;
    for (const auto ch : r.items()) {
      items.try_emplace(ch, 1);
    }

    for (const auto [ch, _] : items) {
      if (seen.contains(ch)) {
        ++seen[ch];
      } else {
        seen.insert({ch, 1});
      }
    }
  }

  std::multimap<std::size_t, char> freqs;
  std::transform(seen.cbegin(), seen.cend(),
                 std::inserter(freqs, freqs.begin()),
                 [](const std::pair<char, std::size_t>& p) {
                   return std::pair<std::size_t, char>{p.second, p.first};
                 });
  return freqs.find(group_size)->second;
}

int main() {
  std::vector<Rucksack> rucksacks;
  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    for (std::string line; std::getline(ifs, line);) {
      rucksacks.emplace_back(line);
    }
  }

  std::size_t priority_sum =
      std::accumulate(rucksacks.cbegin(), rucksacks.cend(), 0UL,
                      [](const std::size_t init, const Rucksack& r) {
                        return init + priority(r.common_item());
                      });
  std::cout << "Problem 1: " << priority_sum << '\n';

  std::vector<Group> groups;
  for (std::size_t idx{}; idx < rucksacks.size(); idx += group_size) {
    groups.push_back(make_array<group_size>(
        std::make_move_iterator(rucksacks.cbegin() + idx),
        std::make_move_iterator(rucksacks.cbegin() + idx + group_size)));
  }

  std::size_t group_priority_sum =
      std::accumulate(groups.cbegin(), groups.cend(), 0UL,
                      [](const std::size_t init, const Group& group) {
                        return init + priority(common_item(group));
                      });
  std::cout << "Problem 2: " << group_priority_sum << '\n';
}
