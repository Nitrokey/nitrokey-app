#!/bin/bash


# -i4: 4 spaces indetation
# -nut: don't use tabs
# -ts4: 1 tab = 4 spaces
# -pcs: space after procedure calls (eg: function (args) )
#                                                ^
# -npsl: put the function type in front and note above (eg: int main() )
# -bad: blank lines after declerations
# -cdw: cuddle 'while' at a do construct
# -bli0: zero space indent between 'if' and it's body braces
# -cli4: 4 spaces indent for 'case' at switch
# -cbi0: 0 space indent for 'case' braces
# -cs: space after cast operators
# -lp: if '(','[','{' is not closed at the same line, align the next line(s) to the start of the '(','[','{'
# -cdb -sc: prepend star '*" on multiline comments. Comments with /* */ are splitted to multiple
# -fca: format comments that appear after column 0
# -fc1: format comments that appear at column 0
# -dj -c0 -cp0 -cd0: right side commends indentation
# -pal: pointer align left

INDENT=/home/george/Desktop/indent-2.2.10/build/bin/indent
find . -name '*.[ch]' -print | xargs -n1 $INDENT \
                                                $1 \
						-l150 \
                                                -i4 \
                                                -nut \
                                                -ts4 \
                                                -pcs \
                                                -npsl \
                                                -cdw \
                                                -bli0 \
                                                -cli4 \
                                                -cbi0 \
                                                -cs \
                                                -lp \
                                                -fca \
                                                -fc1 \
                                                -dj0 -c0 -cp0 \
                                                --pointer-align-right

find . -name '*.cpp' -print | xargs -n1 $INDENT \
                                                $1 \
						-l150 \
                                                -i4 \
                                                -nut \
                                                -ts4 \
                                                -pcs \
                                                -npsl \
                                                -cdw \
                                                -bli0 \
                                                -cli4 \
                                                -cbi0 \
                                                -cs \
                                                -lp \
                                                -fca \
                                                -fc1 \
                                                -dj0 -c0 -cp0 \
                                                --pointer-align-right

                                            #    -cdb -sc \

# Trailing spaces
find . -name '*.cpp' -print | xargs -n1 sed -i -r 's/[ \t]+$//g' $1
find . -name '*.[ch]' -print | xargs -n1 sed -i -r 's/[ \t]+$//g' $1
