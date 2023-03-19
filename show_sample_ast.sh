#!/usr/bin/env sh

if [ ! -f ./CMakeLists.txt ]
then
    echo "This script should be launched from the project root"
    exit 1
fi

for i in samples/*
do
    echo "Parsing $i:"
    cat "$i"
    echo "******************* Result of parsing *******************"
    ./debug/parser -f "$i"
    echo
done


