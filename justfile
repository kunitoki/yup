alias g := generate
alias u := update
alias o := open

default:
  @just --list

[confirm("Are you sure you want to clean the build folder?")]
clean:
  rm -Rf build/*

update:
  cmake -G Xcode -B build -DYUP_ENABLE_PROFILING=ON

generate: clean update

make: update
  cmake --build build

open: update
  -open build/yup.xcodeproj

ios:
  cmake -G Xcode -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM=OS64

emscripten CONFIG="Debug":
  emcc -v
  emcmake cmake -G "Ninja Multi-Config" -B build
  cmake --build build --config {{CONFIG}}
  python3 -m http.server -d .
  #python3 serve.py -p 8000 -d .
