#!/bin/sh

#echo "converting..."
# inkscape movi20.pdf --export-plain-svg=movi20.svg

echo "compiling..."
c++ -std=c++17 -O2 sh_insns.cpp -o sh_insns

echo "executing..."
./sh_insns > sh_insns.html

echo "done"

