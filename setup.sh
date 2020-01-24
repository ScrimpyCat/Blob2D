git submodule update

cd deps/CommonGameKit
sh setup.sh

cd ../glfw
rm -rf build
mkdir build && cd build
cmake -G Ninja ..
ninja

cd ../../../
rm -rf build
ruby deps/CommonGameKit/build.rb \
--framework=deps/CommonGameKit/build/CommonGameKit \
--header=deps/CommonGameKit/deps/CommonC \
--header=deps/CommonGameKit/deps/stdatomic \
--header=deps/CommonGameKit/deps/threads \
--header=deps/CommonGameKit/deps/tinycthread/source \
--static=deps/glfw/build/src/libglfw3.a,deps/glfw/include \
--lib=win32 \
--lib=cocoa
cd build
ninja
