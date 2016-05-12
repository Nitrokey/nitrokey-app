#!/bin/bash

#style is defined in .clang-format
cat <(find . -name '*.cpp') <(find . -name '*.h') | xargs clang-format-3.8 -i

