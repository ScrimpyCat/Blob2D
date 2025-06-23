git submodule init
git submodule update

cd deps/CommonGameKit
sh mac-setup.sh

cd ../glfw
rm -rf build
mkdir build && cd build
cmake -G Xcode ..
sed -i '' -e 's/SDKROOT[^;]*/MACOSX_DEPLOYMENT_TARGET = "$(RECOMMENDED_MACOSX_DEPLOYMENT_TARGET)"; SDKROOT = macosx/' glfw.xcodeproj/project.pbxproj

# Universal builds
sed -i '' -e 's/ARCHS = "$(NATIVE_ARCH_ACTUAL)";/ARCHS = "$(ARCHS_STANDARD)";/' glfw.xcodeproj/project.pbxproj
sed -i '' -e ':a' -e 'N' -e '$!ba' -e 's/\(\/\* Release \*\/ = {\n[[:space:]]*isa = XCBuildConfiguration;\n[[:space:]]*buildSettings = {\n[[:space:]]*ARCHS = "$(ARCHS_STANDARD)";\n[[:space:]]*ONLY_ACTIVE_ARCH\) = YES;/\1 = NO;/' glfw.xcodeproj/project.pbxproj
