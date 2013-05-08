#!/bin/sh
# g++-4.7 -std=c++11 -O2 s-exprpp.cpp -o s-exprpp

#g++-4.7 -std=c++11 -D__gen__ -E sh_insns.cpp | ./s-exprpp > sh_insns.ii
#g++-4.7 -std=c++11 -D__gen__ -O2 sh_insns.ii -lboost_system -o sh_insns

#echo "converting..."
# inkscape movi20.pdf --export-plain-svg=movi20.svg

echo "preprocessing..."
g++-4.7 -std=c++11 -D__gen__ -E sh_insns.cpp | ./s-exprpp > sh_insns.ii

echo "compiling..."
g++-4.7 -std=c++11 -D__gen__ -O2 sh_insns.ii -o sh_insns

echo "executing..."
./sh_insns > sh_insns.html

echo "done"

