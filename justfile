
default:
  @just --list

update:
  mkdir -p build
  cmake -G Xcode -B build

open:
  @just update
  open build/yup.xcodeproj

generate:
  rm -Rf build
  @just update

make:
  @just update
  cmake --build build

#run:
#  @just make
#  ./build/app/app
