#!/bin/bash

usage() { echo "Usage: $0 [-h] [-s] [-p]" 1>&2; exit 1; }

while getopts "hsp" opt; do
    case "${opt}" in
        s)
            shallow=1
            ;;
        p)
            preserve=1
            ;;
        h|*)
            usage
            ;;
    esac
done

build=${BUILD_DIR:-build}

if [ -z "${shallow}" ]; then
    if [ -z "${preserve}" ]; then
        git submodule update
    fi

    parent=$(echo $build | sed s/[^\/]*/../g)

    cd deps/CommonGameKit
    sh setup.sh $@
    cd "../../"

    cd deps/glfw
    rm -rf "$build"
    mkdir -p "$build" && cd "$build"
    cmake -G Ninja "$parent"
    ninja
    cd "$parent/../../"
fi

rm -rf "$build"
ruby deps/CommonGameKit/build.rb \
--framework="deps/CommonGameKit/$build/CommonGameKit" \
--header=deps/CommonGameKit/deps/CommonC \
--header=deps/CommonGameKit/deps/stdatomic \
--header=deps/CommonGameKit/deps/threads \
--header=deps/CommonGameKit/deps/tinycthread/source \
--static="deps/glfw/$build/src/libglfw3.a",deps/glfw/include \
--lib=win32 \
--lib=cocoa
cd "$build"
ninja
