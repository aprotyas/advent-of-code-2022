#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

constexpr std::string_view dash{"-"};

class Assignment {
 public:
  Assignment(const std::string& range)
      : start_{std::stoul(range.substr(0, range.find(dash)))},
        end_{std::stoul(range.substr(range.find(dash) + 1))} {}

  uint64_t start() const { return start_; }
  uint64_t end() const { return end_; }

  bool contains(const Assignment& other) const {
    return other.start() >= start_ && other.end() <= end_;
  }
  bool overlaps(const Assignment& other) const {
    return other.start() <= end_ && other.end() >= start_;
  }

  friend std::ostream& operator<<(std::ostream& os, const Assignment& a);

 private:
  uint64_t start_;
  uint64_t end_;
};

using AssignmentPair = std::pair<Assignment, Assignment>;

std::ostream& operator<<(std::ostream& os, const Assignment& a) {
  os << a.start() << "-" << a.end();
  return os;
}

namespace std {
[[noreturn]] inline void unreachable() {
#ifdef __GNUC__
  __builtin_unreachable();
#elif defined _MSC_VER
  __assume(false);
#endif
}
}  // namespace std

struct full_overlap;
struct any_overlap;

template <typename OverlapPolicy>
class OverlapCounter {
 public:
  void operator()(const AssignmentPair& assignment_pair) {
    const auto& [assignment1, assignment2] = assignment_pair;

    if constexpr (std::is_same_v<OverlapPolicy, full_overlap>) {
      if (assignment1.contains(assignment2) ||
          assignment2.contains(assignment1)) {
        ++count_;
      }
    } else if constexpr (std::is_same_v<OverlapPolicy, any_overlap>) {
      if (assignment1.overlaps(assignment2) ||
          assignment2.overlaps(assignment1)) {
        ++count_;
      }
    } else {
      static_assert(true, "Wrong overlap policy chosen");
    }
  }

  std::size_t count() const { return count_; }

 private:
  std::size_t count_{};
};

int main() {
  std::vector<AssignmentPair> assignment_pairs;
  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    for (std::string line; std::getline(ifs, line);) {
      constexpr std::string_view comma{","};
      const auto comma_idx = line.find(comma);
      assignment_pairs.emplace_back(
          std::piecewise_construct,
          std::forward_as_tuple(line.substr(0, comma_idx)),
          std::forward_as_tuple(line.substr(comma_idx + 1)));
    }
  }

  const std::size_t contains_count =
      std::for_each(assignment_pairs.cbegin(), assignment_pairs.cend(),
                    OverlapCounter<full_overlap>{})
          .count();
  std::cout << "Problem 1: " << contains_count << '\n';

  const std::size_t overlap_count =
      std::for_each(assignment_pairs.cbegin(), assignment_pairs.cend(),
                    OverlapCounter<any_overlap>{})
          .count();
  std::cout << "Problem 2: " << overlap_count << '\n';
  return 0;
}
