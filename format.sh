find . -iname *.cpp -o -iname *.hpp -o -path ./build -o -path ./8/include -prune | xargs clang-format -i --style=Google --verbose
