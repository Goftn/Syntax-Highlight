#! /bin/sh

# generate the output of a formatting, using a style file
# the output is intended to be inserted in the examples.html

SOURCEHIGHLIGHT=../src/source-highlight

for arg in $*
do
    echo "<h2>$arg</h2>";
    $SOURCEHIGHLIGHT --tab=8 --input=./Hello.java --style-css=./$arg --data-dir=../src --outlang-def=xhtmltable.outlang;
done
