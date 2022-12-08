#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/unique.hpp>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

struct Tree {
  using type = Tree;

  template <bool visibility>
  struct set_visibility {
    void operator()(Tree& tree) {
      if (!tree.maybe_visible.has_value()) {
        tree.maybe_visible = visibility;
      } else {
        tree.maybe_visible = tree.maybe_visible.value() | visibility;
      }
    }
  };

  bool operator>(const Tree& other) const { return height > other.height; }
  bool operator==(const Tree& other) const { return height == other.height; }
  bool operator<(const Tree& other) const { return height < other.height; }

  std::size_t height;
  std::optional<bool> maybe_visible{};
};

std::ostream& operator<<(std::ostream& os, const Tree& tree) {
  os << tree.height;
  return os;
}

template <typename T>
void mark_visible_impl(std::vector<T>& range) {
  using BaseT = std::remove_cvref_t<typename T::type>;

  std::size_t max_height;
  if constexpr (std::is_same_v<T, std::reference_wrapper<BaseT>>) {
    max_height = range.front().get().height;
  } else {
    max_height = range.front().height;
  }
  std::unordered_set<std::size_t> visible_heights{max_height};
  for (auto it = std::next(range.cbegin()); it != range.cend();
       it = std::next(it)) {
    std::size_t height;
    if constexpr (std::is_same_v<T, std::reference_wrapper<BaseT>>) {
      height = it->get().height;
    } else {
      height = it->height;
    }
    if (height > max_height) {
      max_height = height;
      visible_heights.insert(max_height);
    }
  }
  auto last_visible = std::find_if(
      range.cbegin(), range.cend(),
      [max_height](const BaseT& tree) { return tree.height == max_height; });

  std::size_t num_visible;
  if (last_visible == range.cend()) {
    num_visible = range.size();
  } else {
    num_visible = std::distance(range.cbegin(), std::next(last_visible));
  }

  ranges::for_each(
      range | ranges::views::take(num_visible) |
          ranges::views::filter([&visible_heights](const BaseT& tree) {
            if (!visible_heights.contains(tree.height)) {
              return false;
            }
            visible_heights.erase(std::find(
                visible_heights.cbegin(), visible_heights.cend(), tree.height));
            return true;
          }) |
          ranges::views::unique(std::equal_to<BaseT>{}),
      Tree::set_visibility<true>{});
  ranges::for_each(range | ranges::views::drop(num_visible),
                   Tree::set_visibility<false>{});
}

template <typename T>
void mark_visible(std::vector<T>& range) {
  using BaseT = std::remove_cvref_t<typename T::type>;

  std::vector<std::reference_wrapper<BaseT>> reversed_range;
  std::copy(range.rbegin(), range.rend(), std::back_inserter(reversed_range));

  mark_visible_impl(range);
  mark_visible_impl(reversed_range);
}

int main() {
  std::vector<std::vector<Tree>> grid;

  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    for (std::string line; std::getline(ifs, line);) {
      std::vector<Tree> row;
      std::transform(line.cbegin(), line.cend(), std::back_inserter(row),
                     [](const auto height) -> Tree {
                       constexpr const char zero = '0';
                       return {static_cast<std::size_t>(height - zero)};
                     });
      grid.push_back(row);
    }
  }

  // rows - from left, from right
  for (auto& row : grid) {
    mark_visible(row);
  }

  // cols - from up, from down
  for (std::size_t col_idx{}; col_idx < grid[0].size(); ++col_idx) {
    std::vector<std::reference_wrapper<Tree>> col;
    for (std::size_t row_idx{}; row_idx < grid.size(); ++row_idx) {
      col.push_back(grid[row_idx][col_idx]);
    }
    mark_visible(col);
  }

  const std::size_t visible_trees = std::accumulate(
      grid.cbegin(), grid.cend(), 0,
      [](const std::size_t init, const auto& row) {
        return init + std::accumulate(
                          row.cbegin(), row.cend(), 0,
                          [](const std::size_t init, const auto& tree) {
                            return init +
                                   (tree.maybe_visible.value_or(false) ? 1 : 0);
                          });
      });
  std::cout << "Problem 1: " << visible_trees << '\n';

  std::vector<std::size_t> scenic_scores;
  for (int row{1}; row < static_cast<int>(grid.size()) - 1; ++row) {
    for (int col{1}; col < static_cast<int>(grid[row].size()) - 1; ++col) {
      std::size_t height{grid[row][col].height};

      // up
      std::size_t up_score{1};
      for (int it_row{row - 1};
           it_row >= 0 && grid[it_row][col].height < height;
           --it_row, ++up_score) {
        if (it_row == 0) {
          --up_score;
        }
      }

      // down
      std::size_t down_score{1};
      for (int it_row{row + 1}; it_row < static_cast<int>(grid.size()) &&
                                grid[it_row][col].height < height;
           ++it_row, ++down_score) {
        if (it_row == static_cast<int>(grid.size()) - 1) {
          --down_score;
        }
      }

      // left
      std::size_t left_score{1};
      for (int it_col{col - 1};
           it_col >= 0 && grid[row][it_col].height < height;
           --it_col, ++left_score) {
        if (it_col == 0) {
          --left_score;
        }
      }

      // right
      std::size_t right_score{1};
      for (int it_col{col + 1}; it_col < static_cast<int>(grid[row].size()) &&
                                grid[row][it_col].height < height;
           ++it_col, ++right_score) {
        if (it_col == static_cast<int>(grid[row].size()) - 1) {
          --right_score;
        }
      }

      const std::size_t scenic_score =
          up_score * down_score * left_score * right_score;
      scenic_scores.push_back(scenic_score);
    }
  }

  const auto max_scenic_score =
      *std::max_element(scenic_scores.cbegin(), scenic_scores.cend());
  std::cout << "Problem 2: " << max_scenic_score << '\n';
}
