#!/usr/bin/env sh

if [ ! -f ./CMakeLists.txt ]
then
    echo "This script should be launched from the project root"
    exit 1
fi

convert_paths() {
    for i in "$@"
    do
        echo "Parsing $i:"
        cat "$i"
        echo "******************* Result of parsing *******************"
        out="$i.c"
        ./debug/pytoc -f "$i" -o "$out" && echo "Result written in $out"
        echo
    done
}

convert_paths samples/*.py
convert_paths elif-samples/*.py
convert_paths while-samples/*.py
