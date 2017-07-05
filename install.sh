#!/usr/bin/env bash

if [ ! -d ./build_LookbackModule ]; then
	mkdir ./build_LookbackModule
fi

cd ./build_LookbackModule
cmake -Dwith-nest=`which nest-config` -DCMAKE_CXX_FLAGS='-g -Wno-unused-variable -Wno-reorder' ..
make -j4
make install
cd ..
