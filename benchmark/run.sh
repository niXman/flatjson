#!/bin/bash

cmake . -DCMAKE_BUILD_TYPE=Release
[[ $? != 0 ]] && { echo "cmake configure error!"; exit 1; }
cmake --build .
[[ $? != 0 ]] && { echo "cmake build error!"; exit 1; }

echo "canada.json" && ./benchmark canada.json
echo

echo "citm_catalog.json" && ./benchmark citm_catalog.json
echo

echo "jeopardy.json" && ./benchmark jeopardy.json
echo

echo "twitter.json" && ./benchmark twitter.json

