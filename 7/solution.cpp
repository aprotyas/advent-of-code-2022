#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "frozen/string.h"
#include "frozen/unordered_map.h"

namespace std {
[[noreturn]] inline void unreachable() {
#ifdef __GNUC__
  __builtin_unreachable();
#elif defined _MSC_VER
  __assume(false);
#endif
}
}  // namespace std

class Command {
 public:
  const static constexpr std::string_view prefix{"$"};

  enum class Type {
    cd,
    ls,
  };

  constexpr static frozen::unordered_map<frozen::string, Type, 2> str_to_type{
      {"cd", Type::cd},
      {"ls", Type::ls},
  };

  constexpr static frozen::unordered_map<Type, frozen::string, 2> type_to_str{
      {Type::cd, "cd"},
      {Type::ls, "ls"},
  };

  Command(const std::vector<std::string>& command_info) {
    // Simple situation: characters [2, 4) are always the types
    const auto cmd_line = command_info.front();
    std::string type_str = cmd_line.substr(2, 2);
    type_ = str_to_type.at(frozen::string(type_str));

    switch (type_) {
      case Type::cd:
        // For cd, characters [5, size()) always form the argument
        argument_ = cmd_line.substr(5);
        break;
      case Type::ls:
        // For ls, all remainder lines are output, so copy those out
        std::copy(std::next(command_info.cbegin()), command_info.cend(),
                  std::back_insert_iterator(output_));
        break;
      default:
        std::unreachable();
    }
  }

  Type type() const { return type_; }
  const std::optional<std::string>& maybe_argument() const { return argument_; }
  const std::vector<std::string>& output() const { return output_; }

 private:
  Type type_;
  std::optional<std::string> argument_{};
  std::vector<std::string> output_{};
};

std::ostream& operator<<(std::ostream& os, const frozen::string& frozen_str) {
  std::copy(frozen_str.begin(), frozen_str.end(),
            std::ostream_iterator<char>(os, ""));
  return os;
}

std::ostream& operator<<(std::ostream& os, const Command& command) {
  os << Command::type_to_str.at(command.type()) << ": ";
  switch (command.type()) {
    case Command::Type::cd:
      os << command.maybe_argument().value_or("");
      break;
    case Command::Type::ls:
      std::copy(command.output().cbegin(), command.output().cend(),
                std::ostream_iterator<std::string>(os, "\n"));
      break;
    default:
      std::unreachable();
  }
  return os;
}

struct File {
  std::string name;
  std::size_t size;
};

class Directory {
 public:
  Directory(const std::string& name) : name_{name} {}
  std::size_t size() const {
    return std::accumulate(files_.cbegin(), files_.cend(), 0,
                           [](const std::size_t init, const auto& file) {
                             return init + file->size;
                           }) +
           std::accumulate(child_dirs_.cbegin(), child_dirs_.cend(), 0,
                           [](const std::size_t init, const auto child_dir) {
                             return init + child_dir->size();
                           });
  }
  std::string name() const { return name_; }
  std::shared_ptr<Directory> parent() const { return parent_; }
  std::vector<std::shared_ptr<Directory>>& child_dirs() { return child_dirs_; }
  const std::vector<std::shared_ptr<Directory>>& child_dirs() const {
    return child_dirs_;
  }
  std::vector<std::unique_ptr<File>>& files() { return files_; }
  const std::vector<std::unique_ptr<File>>& files() const { return files_; }

  void set_parent(std::shared_ptr<Directory> parent) { parent_ = parent; }

 private:
  std::string name_;
  std::shared_ptr<Directory> parent_{nullptr};
  std::vector<std::shared_ptr<Directory>> child_dirs_{};
  std::vector<std::unique_ptr<File>> files_{};
};

constexpr const std::size_t dir_size_limit = 100000;
constexpr const std::size_t filesystem_space = 70000000;
constexpr const std::size_t unused_space = 30000000;

std::vector<std::size_t> find_dir_sizes_around_limit(
    const Directory* root, const std::size_t dir_size_limit,
    std::function<bool(const std::size_t, const std::size_t)> cmp) {
  std::vector<std::size_t> dir_sizes;
  if (cmp(root->size(), dir_size_limit)) {
    dir_sizes.push_back(root->size());
  }
  for (const auto& child : root->child_dirs()) {
    std::vector<std::size_t> child_large_dir_sizes =
        find_dir_sizes_around_limit(child.get(), dir_size_limit, cmp);
    dir_sizes.insert(dir_sizes.end(), child_large_dir_sizes.begin(),
                     child_large_dir_sizes.end());
  }
  return dir_sizes;
}

int main() {
  std::vector<Command> commands;
  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    std::vector<std::string> command_info;
    for (std::string line; std::getline(ifs, line);) {
      // Start of a command
      if (line.starts_with(Command::prefix)) {
        if (!command_info.empty()) {
          commands.emplace_back(command_info);
        }
        command_info.clear();
      }
      command_info.push_back(line);
    }
  }

  constexpr const std::string_view dir_entry_prefix{"dir"};
  constexpr const std::string_view prev_dir_name{".."};
  constexpr const std::string_view root_dir_name{"/"};
  const auto root = std::make_shared<Directory>(
      commands.cbegin()->maybe_argument().value_or(std::string{root_dir_name}));
  auto current = root;
  // Skipping the first command since it is `cd /`, which has been accounted for
  // in root construction
  for (const auto& command : commands | std::views::drop(1)) {
    switch (command.type()) {
      case Command::Type::cd: {
        const std::string new_dir_name = command.maybe_argument().value();
        if (new_dir_name == root_dir_name) {
          current = root;
        } else if (new_dir_name == prev_dir_name) {
          current = current->parent();
        } else {
          current = *std::find_if(current->child_dirs().begin(),
                                  current->child_dirs().end(),
                                  [&new_dir_name](const auto child_dir) {
                                    return child_dir->name() == new_dir_name;
                                  });
        }
      } break;
      case Command::Type::ls:
        for (const auto& line : command.output()) {
          if (line.starts_with(dir_entry_prefix)) {
            // Dir
            const std::string dir_name =
                line.substr(dir_entry_prefix.length() + 1);
            current->child_dirs().push_back(
                std::make_shared<Directory>(dir_name));
            current->child_dirs().back()->set_parent(current);
          } else {
            // File
            const std::size_t space_idx = line.find(" ");
            const std::size_t size = std::stoi(line.substr(0, space_idx));
            const std::string file_name = line.substr(space_idx + 1);
            current->files().push_back(std::make_unique<File>(file_name, size));
          }
        }
        break;
      default:
        std::unreachable();
    }
  }

  const auto dir_sizes_under_limit = find_dir_sizes_around_limit(
      root.get(), dir_size_limit, std::less_equal<std::size_t>{});
  const std::size_t dir_sizes_sum = std::accumulate(
      dir_sizes_under_limit.cbegin(), dir_sizes_under_limit.cend(), 0,
      [](const std::size_t init, const std::size_t dir_size) {
        return init + dir_size;
      });
  std::cout << "Problem 1: " << dir_sizes_sum << '\n';

  const std::size_t free_req = unused_space - (filesystem_space - root->size());
  const auto dir_sizes_over_limit = find_dir_sizes_around_limit(
      root.get(), free_req, std::greater_equal<std::size_t>{});
  const auto min_biggest_dir_size = *std::min_element(
      dir_sizes_over_limit.cbegin(), dir_sizes_over_limit.cend());
  std::cout << "Problem 2: " << min_biggest_dir_size << '\n';
}
