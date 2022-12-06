#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stack>
#include <type_traits>

class Instruction {
 public:
  Instruction(std::size_t amount, std::size_t src, std::size_t dst)
      : amount_{amount}, src_{--src}, dst_{--dst} {}

  std::size_t amount() const { return amount_; }
  std::size_t src() const { return src_; }
  std::size_t dst() const { return dst_; }

  friend std::ostream& operator<<(std::ostream& os,
                                  const Instruction& instruction);

 private:
  std::size_t amount_;
  std::size_t src_;
  std::size_t dst_;
};

std::ostream& operator<<(std::ostream& os, const Instruction& instruction) {
  os << "Amount: " << instruction.amount_ << ", Source: " << instruction.src_
     << ", Destination: " << instruction.dst_;
  return os;
}

template <typename T>
void flip_stack(std::stack<T>& stack) {
  std::stack<T> flipped_stack;
  while (!stack.empty()) {
    flipped_stack.push(stack.top());
    stack.pop();
  }
  stack = flipped_stack;
}

namespace impl {
void execute_single(const auto& instruction, auto& stacks) {
  for (std::size_t idx{}; idx < instruction.amount(); ++idx) {
    stacks.at(instruction.dst()).push(stacks.at(instruction.src()).top());
    stacks.at(instruction.src()).pop();
  }
}

void execute_multi(const auto& instruction, auto& stacks) {
  std::stack<char> tmp_stack;
  for (std::size_t idx{}; idx < instruction.amount(); ++idx) {
    tmp_stack.push(stacks.at(instruction.src()).top());
    stacks.at(instruction.src()).pop();
  }

  while (!tmp_stack.empty()) {
    stacks.at(instruction.dst()).push(tmp_stack.top());
    tmp_stack.pop();
  }
}
}  // namespace impl

struct single_move;
struct multi_move;

template <typename ExecutionPolicy>
void execute(const auto& instruction, auto& stacks) {
  if constexpr (std::is_same_v<ExecutionPolicy, single_move>) {
    impl::execute_single(instruction, stacks);
  } else if constexpr (std::is_same_v<ExecutionPolicy, multi_move>) {
    impl::execute_multi(instruction, stacks);
  } else {
    static_assert(true,
                  "Please specify either single_move or multi_move policy");
  }
}

template <typename ExecutionPolicy>
void execute_and_show(const auto& instructions, auto& stacks,
                      const std::string_view prefix_msg) {
  auto tmp_stacks = stacks;
  std::for_each(instructions.cbegin(), instructions.cend(),
                [&tmp_stacks](const auto& instruction) {
                  execute<ExecutionPolicy>(instruction, tmp_stacks);
                });
  std::cout << prefix_msg;
  std::transform(tmp_stacks.begin(), tmp_stacks.end(),
                 std::ostream_iterator<char>(std::cout, ""),
                 [](const auto& stack) { return stack.top(); });
  std::cout << '\n';
}

int main() {
  std::vector<std::stack<char>> stacks;
  std::vector<Instruction> instructions;
  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    std::string line;
    std::getline(ifs, line);

    // Create the stacks
    const constexpr std::size_t item_width = 4UL;
    const constexpr char item_start = '[';
    stacks = std::vector<std::stack<char>>(line.length() / item_width + 1);
    do {
      for (auto iter = std::find(line.cbegin(), line.cend(), item_start);
           iter != line.cend();
           iter = std::find(iter + 1, line.cend(), item_start)) {
        std::size_t idx = std::distance(line.cbegin(), iter);
        stacks.at(idx / item_width).push(*std::next(iter));
      }
    } while (std::getline(ifs, line) && !line.empty());
    std::for_each(stacks.begin(), stacks.end(), flip_stack<char>);

    // Parse the instructions
    for (; std::getline(ifs, line);) {
      std::istringstream instruction{line};
      std::string move, from, to;
      std::size_t amount, src, dst;
      instruction >> move >> amount >> from >> src >> to >> dst;
      instructions.emplace_back(amount, src, dst);
    }
  }

  execute_and_show<single_move>(instructions, stacks, "Problem 1: ");
  execute_and_show<multi_move>(instructions, stacks, "Problem 2: ");

  return 0;
}
