# PassCurses

## Requirements
Put single-include header json.hpp from https://github.com/nlohmann/json into the includes folder

## Compile with:
> __g++ -std=c++17 -o <*name-of-your-choice*> main.cpp -lncurses -lstdc++fs__

## Or with CMake:
> __cmake .__

then

> __make__

You can:
* add new custom passwords
* generate new random passwords
* delete passwords
* search for existing passwords
