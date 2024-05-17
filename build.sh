#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

pushd "$SCRIPT_DIR" > /dev/null
rm -rf build

mkdir build
cd build
cmake ../
make -j8

popd > /dev/null