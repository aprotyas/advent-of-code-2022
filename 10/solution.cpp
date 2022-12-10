#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>

namespace std {
[[noreturn]] inline void unreachable() {
#ifdef __GNUC__
  __builtin_unreachable();
#elif defined _MSC_VER
  __assume(false);
#endif
}
}  // namespace std

struct Instruction {
  enum class Type {
    ADDX,
    NOOP,
  };

  static std::string_view type_to_string(const Instruction::Type type) {
    switch (type) {
      case Type::ADDX:
        return "addx";
        break;
      case Type::NOOP:
        return "noop";
        break;
      default:
        std::unreachable();
    }
  }

  static std::size_t type_to_cycle_cost(const Instruction::Type type) {
    switch (type) {
      case Type::ADDX:
        return 2;
        break;
      case Type::NOOP:
        return 1;
        break;
      default:
        std::unreachable();
    }
  }

  Type type;
  std::optional<int> maybe_argument{};
};

std::ostream& operator<<(std::ostream& os, const Instruction& instruction) {
  os << Instruction::type_to_string(instruction.type);
  if (instruction.maybe_argument) {
    os << " " << instruction.maybe_argument.value();
  }
  return os;
}

struct CPU {
  void execute_instruction(const Instruction& instruction) {
    switch (instruction.type) {
      case Instruction::Type::ADDX:
        cpu_register += instruction.maybe_argument.value();
        break;
      case Instruction::Type::NOOP:
        break;
      default:
        std::unreachable();
    }
  }

  int cpu_register{1};
};

bool is_drawable(const int sprite_position, const std::size_t pixel) {
  return (static_cast<int>(pixel) <= (sprite_position + 1)) &&
         (static_cast<int>(pixel) >= (sprite_position - 1));
}

int main() {
  std::array<uint32_t, 6> cycles_of_interest;
  std::array<uint32_t, 6> crt_row_cycles;
  uint32_t start = -1;
  std::generate(cycles_of_interest.begin(), cycles_of_interest.end(),
                [&start] { return 20 * (start += 2); });
  start = 0;
  std::generate(crt_row_cycles.begin(), crt_row_cycles.end(),
                [&start] { return 20 * (start += 2); });

  std::vector<Instruction> instructions;
  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    for (std::string line; std::getline(ifs, line);) {
      // If no " " delimiter is found, it's a NOOP
      if (const std::size_t delim_idx = line.find(" ");
          delim_idx == std::string::npos) {
        instructions.emplace_back(Instruction::Type::NOOP);
      } else {
        const int argument = std::stoi(line.substr(delim_idx + 1));
        instructions.emplace_back(Instruction::Type::ADDX, argument);
      }
    }
  }

  int combined_signal_strength = 0;
  CPU cpu;
  auto current_instruction = instructions.cbegin();
  for (std::size_t cycle{1},
       target_cycle{cycle +
                    Instruction::type_to_cycle_cost(current_instruction->type)};
       (cycle <= *std::prev(cycles_of_interest.cend())) ||
       (current_instruction != instructions.cend());
       ++cycle) {
    if (cycle == target_cycle) {
      cpu.execute_instruction(*current_instruction);
      current_instruction = std::next(current_instruction);
      target_cycle +=
          Instruction::type_to_cycle_cost(current_instruction->type);
    }

    if (is_drawable(cpu.cpu_register, (cycle - 1) % (crt_row_cycles[0]))) {
      std::cout << '#';
    } else {
      std::cout << '.';
    }
    if (std::find(crt_row_cycles.cbegin(), crt_row_cycles.cend(), cycle) !=
        crt_row_cycles.cend()) {
      std::cout << '\n';
    }

    if (std::find(cycles_of_interest.cbegin(), cycles_of_interest.cend(),
                  cycle) != cycles_of_interest.cend()) {
      combined_signal_strength += cycle * cpu.cpu_register;
    }
  }

  std::cout << "\n\nProblem 1: " << combined_signal_strength << '\n';
}
