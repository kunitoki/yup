alias g := generate
alias u := update
alias o := open

default:
  @just --list

update:
  mkdir -p build
  cmake -G Xcode -B build -DYUP_ENABLE_PROFILING=ON

clean:
  rm -Rf build/*

generate:
  @just clear
  @just update

open:
  @just update
  -open build/yup.xcodeproj

make:
  @just update
  cmake --build build

ios:
  mkdir -p build
  cmake -G Xcode -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM=OS64

emscripten:
  emcc -v
  emcmake cmake -G "Ninja Multi-Config" -B build
  cmake --build build --config Debug
  python3 -m http.server -d .
  #python3 serve.py -p 8000 -d .

#run:
#  @just make
#  ./build/app/app
