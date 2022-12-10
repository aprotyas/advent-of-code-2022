#include <algorithm>
#include <array>
#include <boost/container_hash/hash.hpp>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace std {
[[noreturn]] inline void unreachable() {
#ifdef __GNUC__
  __builtin_unreachable();
#elif defined _MSC_VER
  __assume(false);
#endif
}
}  // namespace std

struct Point {
  int32_t x;
  int32_t y;

  bool operator==(const Point& other) const {
    return x == other.x && y == other.y;
  }
};

std::ostream& operator<<(std::ostream& os, const Point& point) {
  os << '(' << point.x << ',' << point.y << ')';
  return os;
}

template <>
struct std::hash<Point> {
  std::size_t operator()(const Point& point) const noexcept {
    std::size_t seed = 0;
    boost::hash_combine(seed, point.x);
    boost::hash_combine(seed, point.y);
    return seed;
  }
};

struct Motion {
  enum class Direction : char {
    up = 'U',
    down = 'D',
    left = 'L',
    right = 'R',
  };

  explicit Motion(const Motion::Direction direction_)
      : direction{direction_}, magnitude{unit_magnitude} {}

  static const constexpr std::size_t num_directions{4};
  static const constexpr std::size_t num_diagonal_directions{2};
  static const constexpr std::size_t unit_magnitude{1};

  Direction direction;
  std::size_t magnitude;
};

std::ostream& operator<<(std::ostream& os, const Motion& motion) {
  os << static_cast<char>(motion.direction) << motion.magnitude;
  return os;
}

bool in_touch(const Point& point_1, const Point& point_2) {
  return point_1.x >= point_2.x - 1 && point_1.x <= point_2.x + 1 &&
         point_1.y >= point_2.y - 1 && point_1.y <= point_2.y + 1;
}

bool within_l1_distance_impl(const Point& lagging, const Point& leading,
                             const std::size_t limit,
                             const Motion::Direction direction) {
  switch (direction) {
    case Motion::Direction::up:
      return (leading.y - lagging.y) == static_cast<int>(limit) &&
             (leading.x == lagging.x);
      break;
    case Motion::Direction::down:
      return (lagging.y - leading.y) == static_cast<int>(limit) &&
             (leading.x == lagging.x);
      break;
    case Motion::Direction::right:
      return (leading.x - lagging.x) == static_cast<int>(limit) &&
             (leading.y == lagging.y);
      break;
    case Motion::Direction::left:
      return (lagging.x - leading.x) == static_cast<int>(limit) &&
             (leading.y == lagging.y);
      break;
    default:
      std::unreachable();
  }
}

std::optional<Motion::Direction> direction_within_l1_distance(
    const Point& lagging, const Point& leading, const std::size_t limit) {
  const std::array<Motion::Direction, Motion::num_directions> directions{
      Motion::Direction::up, Motion::Direction::down, Motion::Direction::left,
      Motion::Direction::right};
  std::array<bool, Motion::num_directions> directions_in_distance;
  for (std::size_t index{}; index < directions.size(); ++index) {
    const auto direction = directions[index];
    const bool in_distance =
        within_l1_distance_impl(lagging, leading, limit, direction);
    directions_in_distance[index] = in_distance;
  }
  if (std::count(directions_in_distance.cbegin(), directions_in_distance.cend(),
                 true) != 1) {
    return {};
  }
  if (const auto it = std::find(directions_in_distance.cbegin(),
                                directions_in_distance.cend(), true);
      it != directions_in_distance.cend()) {
    const std::size_t index =
        std::distance(directions_in_distance.cbegin(), it);
    return directions[index];
  }
  return {};
}

std::array<Motion::Direction, Motion::num_diagonal_directions>
find_diagonal_directions(const Point& lagging, const Point& leading) {
  std::array<Motion::Direction, Motion::num_diagonal_directions>
      diagonal_directions;

  diagonal_directions[0] =
      (leading.y > lagging.y) ? Motion::Direction::up : Motion::Direction::down;
  diagonal_directions[1] = (leading.x > lagging.x) ? Motion::Direction::right
                                                   : Motion::Direction::left;

  return diagonal_directions;
}

std::vector<Motion> suggest_motion(const Point& lagging, const Point& leading) {
  if (in_touch(lagging, leading)) {
    return {};
  }

  // Not in touch, so figure out a motion that fixes this
  constexpr std::size_t l1_distance_limit{2};
  // If 2 units away in a given direction, just move 1 unit in that directiojn
  if (const auto maybe_direction{
          direction_within_l1_distance(lagging, leading, l1_distance_limit)}) {
    return {Motion{*maybe_direction}};
  }

  // Else, move diagonally
  std::vector<Motion> suggested_diagonal_motions;
  const auto diagonal_motion_directions =
      find_diagonal_directions(lagging, leading);
  std::transform(diagonal_motion_directions.cbegin(),
                 diagonal_motion_directions.cend(),
                 std::back_inserter(suggested_diagonal_motions),
                 [](const auto direction) { return Motion{direction}; });
  return suggested_diagonal_motions;
}

namespace Rope {
namespace Knot {
struct Head;
struct Middle;
struct Tail;
}  // namespace Knot

template <typename EndType>
class End {
 public:
  const Point& current_point() const { return current_; }
  const std::unordered_set<Point>& points_visited() const {
    return points_visited_;
  }

  void move(const Motion& motion, const bool should_track_visit =
                                      std::is_same_v<EndType, Knot::Head>) {
    switch (motion.direction) {
      case Motion::Direction::up:
        current_.y += motion.magnitude;
        break;
      case Motion::Direction::down:
        current_.y -= motion.magnitude;
        break;
      case Motion::Direction::right:
        current_.x += motion.magnitude;
        break;
      case Motion::Direction::left:
        current_.x -= motion.magnitude;
        break;
      default:
        std::unreachable();
    }

    if (should_track_visit) {
      visit(current_);
    }
  }

  // Following is only a valid behavior for a non-head knot
  template <typename U = void, typename = std::enable_if_t<
                                   !std::is_same_v<EndType, Knot::Head>, U>>
  void follow(const End<U>& end) {
    for (const auto& motion : suggest_motion(current_, end.current_point())) {
      move(motion);
    }
    visit(current_);
  }

 private:
  void visit(const Point& point) { points_visited_.insert(point); }

  Point current_{};
  std::unordered_set<Point> points_visited_{current_};
};

constexpr const std::size_t num_knots{10};

std::tuple<End<Knot::Head>, std::array<End<Knot::Middle>, num_knots - 2>,
           End<Knot::Tail>>
make_rope() {
  std::array<End<Knot::Middle>, num_knots - 2> middle;
  middle.fill(End<Knot::Middle>{});
  return std::make_tuple(End<Knot::Head>{}, middle, End<Knot::Tail>{});
}
}  // namespace Rope

int main() {
  std::vector<Motion> motions;
  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    for (std::string line; std::getline(ifs, line);) {
      // Index 0 is always direction and 1 is always a delimiter
      const Motion::Direction direction{line[0]};
      const std::size_t magnitude = std::stoul(line.substr(2));
      motions.insert(motions.end(), magnitude, Motion{direction});
    }
  }

  auto [head, middle, tail] = Rope::make_rope();
  for (const auto& motion : motions) {
    head.move(motion);
    middle.begin()->follow(head);
    for (auto it = std::next(middle.begin()); it != middle.end();
         it = std::next(it)) {
      it->follow(*std::prev(it));
    }
    tail.follow(*std::prev(middle.end()));
  }

  std::cout << "Problem 2: " << tail.points_visited().size() << '\n';
}

// 2651
