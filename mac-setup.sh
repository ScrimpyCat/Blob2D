git submodule init
git submodule update

cd deps/CommonGameKit
sh mac-setup.sh

cd ../glfw
rm -rf build
mkdir build && cd build
cmake -G Xcode ..
sed -i -e 's/SDKROOT/MACOSX_DEPLOYMENT_TARGET = 10.10; SDKROOT/' glfw.xcodeproj/project.pbxproj
