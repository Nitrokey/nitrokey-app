#!/bin/bash

#style is defined in .clang-format

# run for all files
#cat <(find . -name '*.cpp') <(find . -name '*.h') | xargs -n1 clang-format-3.8 -i

#run only on not committed files
git status | sed 's|new file|new_file|g' | grep -e modified -e new_file | awk '{print $2}' | xargs clang-format-3.8 -i

