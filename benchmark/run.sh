#!/bin/bash

cmake . -DCMAKE_BUILD_TYPE=Release
[[ $? != 0 ]] && { echo "cmake configure error!"; exit 1; }
cmake --build .
[[ $? != 0 ]] && { echo "cmake build error!"; exit 1; }

echo "assets/canada.json" && ./benchmark ../assets/canada.json
echo

echo "assets/citm_catalog.json" && ./benchmark ../assets/citm_catalog.json
echo

echo "assets/jeopardy.json" && ./benchmark ../assets/jeopardy.json
echo

echo "assets/twitter.json" && ./benchmark ../assets/twitter.json

