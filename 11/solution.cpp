#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace std {
[[noreturn]] inline void unreachable() {
#ifdef __GNUC__
  __builtin_unreachable();
#elif defined _MSC_VER
  __assume(false);
#endif
}
}  // namespace std

class Monkey {
 public:
  using ItemType = int64_t;

  static Monkey create_from_description(
      const std::vector<std::string>& monkey_description) {
    const std::size_t id =
        static_cast<std::size_t>(*std::prev(monkey_description[0].cbegin(), 2));

    std::deque<ItemType> items;
    {
      const std::string items_str = monkey_description[1].substr(18);
      constexpr const std::string_view comma_delimiter = ",";
      std::size_t start = 0;
      std::size_t end = items_str.find(comma_delimiter);
      while (end != std::string::npos) {
        items.push_back(std::stoll(items_str.substr(start, end - start)));
        start = end + comma_delimiter.length();
        end = items_str.find(comma_delimiter, start);
      }
      items.push_back(std::stoi(items_str.substr(start)));
    }

    std::function<ItemType(ItemType)> operation;
    {
      std::optional<int> argument{};
      try {
        argument = std::stoi(monkey_description[2].substr(25));
      } catch (const std::invalid_argument&) {
      }

      const char op_token = monkey_description[2][23];
      switch (op_token) {
        case '+':
          operation = [argument](const ItemType item) {
            if (argument) {
              return std::plus<ItemType>{}(item, argument.value());
            }
            return std::plus<ItemType>{}(item, item);
          };
          break;
        case '*':
          operation = [argument](const ItemType item) {
            if (argument) {
              return std::multiplies<ItemType>{}(item, argument.value());
            }
            return std::multiplies<ItemType>{}(item, item);
          };
          break;
        default:
          std::unreachable();
      }
    }

    const std::size_t test_factor =
        std::stoul(monkey_description[3].substr(21));

    const std::size_t true_test_effect = static_cast<std::size_t>(
        *std::prev(monkey_description[4].cend()) - '0');
    const std::size_t false_test_effect = static_cast<std::size_t>(
        *std::prev(monkey_description[5].cend()) - '0');
    const std::pair<std::size_t, std::size_t> test_effect =
        std::make_pair(true_test_effect, false_test_effect);

    return Monkey(id, items, operation, test_factor, test_effect);
  }

  Monkey(const std::size_t id, const std::deque<ItemType>& items,
         const std::function<ItemType(ItemType)> operation,
         std::size_t test_factor,
         const std::pair<std::size_t, std::size_t> test_effect)
      : id_{id},
        items_{items},
        operation_{operation},
        test_factor_{test_factor},
        test_effect_{test_effect} {}

  std::pair<std::vector<ItemType>, std::vector<ItemType>> inspect() {
    std::vector<ItemType> pass_items;
    std::vector<ItemType> fail_items;
    std::transform(items_.cbegin(), items_.cend(), items_.begin(),
                   [&pass_items, &fail_items, operation = operation_,
                    test = test_, cooldown = cooldown_](const ItemType item) {
                     const ItemType new_item = cooldown(operation(item));
                     if (test(new_item)) {
                       pass_items.push_back(new_item);
                     } else {
                       fail_items.push_back(new_item);
                     }
                     return new_item;
                   });
    items_inspected_ += items_.size();
    return std::make_pair(pass_items, fail_items);
  }

  void receive(const std::vector<ItemType>& rx_items) {
    items_.insert(items_.cend(), rx_items.cbegin(), rx_items.cend());
  }

  std::size_t id() const { return id_; }
  const std::deque<ItemType>& items() const { return items_; }
  std::deque<ItemType>& items() { return items_; }
  std::function<ItemType(ItemType)> operation() const { return operation_; }
  std::function<bool(ItemType)> test() const { return test_; }
  std::function<ItemType(ItemType)>& cooldown() { return cooldown_; }
  std::pair<std::size_t, std::size_t> test_effect() const {
    return test_effect_;
  }
  std::size_t items_inspected() const { return items_inspected_; }
  std::size_t test_factor() const { return test_factor_; }

 private:
  std::size_t id_;
  std::deque<ItemType> items_;
  std::function<ItemType(ItemType)> operation_;
  std::size_t test_factor_{};
  std::function<bool(ItemType)> test_{
      [test_factor = test_factor_](const ItemType item) {
        return std::modulus<ItemType>{}(item, test_factor) == 0;
      }};
  std::function<ItemType(ItemType)> cooldown_{
      [](const int item) { return std::divides<int>{}(item, 3); }};
  std::pair<std::size_t, std::size_t> test_effect_;
  std::size_t items_inspected_{};
};

std::ostream& operator<<(std::ostream& os, const Monkey& monkey) {
  os << "Monkey " << monkey.id() << ":\n\tStarting items: ";
  for (const auto item : monkey.items()) {
    os << item << ' ';
  }
  return os;
}

void show_monkey_business(const auto& monkeys, const bool show_all = false) {
  if (show_all) {
    std::transform(
        monkeys.cbegin(), monkeys.cend(),
        std::ostream_iterator<Monkey::ItemType>(std::cout, "  "),
        [](const Monkey& monkey) { return monkey.items_inspected(); });
    std::cout << '\n';
  } else {
    std::vector<std::size_t> activities;
    std::transform(
        monkeys.cbegin(), monkeys.cend(), std::back_inserter(activities),
        [](const Monkey& monkey) { return monkey.items_inspected(); });

    std::partial_sort(activities.begin(), std::next(activities.begin(), 2),
                      activities.end(), std::greater<std::size_t>{});
    const std::size_t monkey_business = activities[0] * activities[1];
    std::cout << "Monkey business: " << monkey_business << '\n';
  }
}

int main() {
  std::vector<Monkey> monkeys;

  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    std::vector<std::string> monkey_description;
    for (std::string line; std::getline(ifs, line);) {
      if (line.empty()) {
        monkeys.push_back(Monkey::create_from_description(monkey_description));
        monkey_description.clear();
      } else {
        monkey_description.push_back(line);
      }
    }
    monkeys.push_back(Monkey::create_from_description(monkey_description));
  }

  const std::size_t supermod =
      std::accumulate(monkeys.cbegin(), monkeys.cend(), 1,
                      [](const std::size_t init, const Monkey& monkey) {
                        return init * monkey.test_factor();
                      });
  for (auto& monkey : monkeys) {
    monkey.cooldown() = [supermod](const Monkey::ItemType item) {
      return std::modulus<Monkey::ItemType>{}(
          item, static_cast<Monkey::ItemType>(supermod));
    };
  }

  constexpr const std::size_t num_rounds = 10000;
  for (std::size_t round{}; round < num_rounds; ++round) {
    for (auto& monkey : monkeys) {
      const auto [pass_items, fail_items] = monkey.inspect();
      const auto [pass_dest, fail_dest] = monkey.test_effect();
      monkeys[pass_dest].receive(pass_items);
      monkeys[fail_dest].receive(fail_items);
      monkey.items().clear();
    }
  }

  show_monkey_business(monkeys);

  return 0;
}
