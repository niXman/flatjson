#!/bin/bash

g++ -std=c++11 -I../include -D__FJ__CHILDS_TYPE=std::uint32_t -O2 benchmark.cpp -o benchmark
[[ $? != 0 ]] && exit 1

echo "canada.json" && ./benchmark canada.json
echo

echo "citm_catalog.json" && ./benchmark citm_catalog.json
echo

echo "jeopardy.json" && ./benchmark jeopardy.json
echo

echo "twitter.json" && ./benchmark twitter.json

