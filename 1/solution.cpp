#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

template <std::ranges::range T>
struct totals {
  T::value_type operator()(const T& val) {
    return std::accumulate(val.cbegin(), val.cend(), 0UL);
  }
};

constexpr std::size_t top_group_size{3UL};

int main() {
  std::vector<std::vector<uint32_t>> foods;
  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    std::vector<uint32_t> food;
    for (std::string line; std::getline(ifs, line);) {
      if (line.empty()) {
        foods.push_back(food);
        food.clear();
      } else {
        food.push_back(std::stoi(line));
      }
    }
  }

  std::vector<uint32_t> food_totals;
  std::transform(foods.cbegin(), foods.cend(), std::back_inserter(food_totals),
                 totals<decltype(foods)::value_type>{});
  std::cout << "Problem 1: "
            << *std::max_element(food_totals.cbegin(), food_totals.cend())
            << '\n';

  std::nth_element(food_totals.begin(), food_totals.begin() + top_group_size,
                   food_totals.end(), std::greater{});
  uint32_t top_group_total{std::accumulate(
      food_totals.cbegin(), food_totals.cbegin() + top_group_size, 0U)};
  std::cout << "Problem 2: " << top_group_total << '\n';
  return 0;
}
