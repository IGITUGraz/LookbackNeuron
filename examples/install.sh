#!/usr/bin/env bash

if [ ! -d ./IzhikTestModule ]; then
	raise error "Cannot find the Module directory IzhikTestModule in the current directory `pwd`"
fi

if [ ! -d ./build_IzhikTestModule ]; then
	mkdir ./build_IzhikTestModule
fi

cd ./build_IzhikTestModule
cmake -Dwith-nest=/home/arjun/nest-bleeding-python3/bin/nest-config -DCMAKE_CXX_FLAGS='-std=c++11 -Wno-unused-variable -Wno-reorder' ../IzhikTestModule
make -j4
make install
cd ..