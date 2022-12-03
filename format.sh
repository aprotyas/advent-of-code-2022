find . -iname *.cpp -o -iname *.hpp -o -path ./build -prune | xargs clang-format -i --style=Google --verbose
