#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

class Packet : public std::variant<std::vector<Packet>, int> {
 private:
  using base = std::variant<std::vector<Packet>, int>;

 public:
  using base::base;
  Packet() = default;
  Packet(const std::string& line) {}
};

std::ostream& operator<<(std::ostream& os, const Packet& packet) {
  if (const std::vector<Packet>* as_vector =
          std::get_if<std::vector<Packet>>(&packet)) {
    os << '[';
    for (const auto& elem : *as_vector) {
      os << elem;
    }
    os << ']';
  } else if (const int* as_int = std::get_if<int>(&packet)) {
    os << *as_int;
  }
  return os;
}

int main() {
  std::vector<std::pair<Packet, Packet>> packet_pairs;

  if (std::ifstream ifs{"input"}; ifs.is_open()) {
    std::array<Packet, 2> packets;
    std::size_t idx{};
    for (std::string line; std::getline(ifs, line);) {
      if (line.empty()) {
        idx = 0;
        packet_pairs.emplace_back(packets.at(0), packets.at(1));
        continue;
      }
      packets.at(idx++) = Packet{line};
    }
  }

  for (const auto& packet_pair : packet_pairs) {
    std::ignore = packet_pair;
  }
}
