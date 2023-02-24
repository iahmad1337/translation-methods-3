#!/usr/bin/env sh

if [ ! -f ./CMakeLists.txt ]
then
    echo "This script should be launched from the project root"
    exit 1
fi

if [ ! -d ./cpputils ]
then
    # NOTE: I count on the fact that `cpputils` is one of the submodules needed
    # for development
    echo "Clone this repo with all of the submodules: git clone --depth=1"
    exit 1
fi

additional_opts=""
build_dir=""
if [ "$1" = "debug" ]
then
    additional_opts="-DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZERS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    build_dir="./debug"
elif [ "$1" = "release" ]
then
    additional_opts="-DCMAKE_BUILD_TYPE=Release"
    build_dir="./release"
else
    echo "Please pass the build type (debug or release)"
    exit 1
fi

# NOTE: the second variable is not quoted because I want it to be split
cmake -S . -B "$build_dir" $additional_opts
cp "$build_dir/compile_commands.json" .
