
default:
  @just --list

update:
  mkdir -p build
  cmake -G Xcode -B build -DYUP_ENABLE_PROFILING=ON

ios:
  mkdir -p build
  cmake -G Xcode -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM=OS64

clear:
  rm -Rf build/*

generate:
  @just clear
  @just update

open:
  @just update
  open build/yup.xcodeproj

make:
  @just update
  cmake --build build

#run:
#  @just make
#  ./build/app/app
