alias c := clean

default:
  @just --list

[confirm("Are you sure you want to clean the build folder? [y/N]")]
clean:
  rm -Rf build/*

osx PROFILING="OFF":
  cmake -G Xcode -B build -DYUP_ENABLE_PROFILING={{PROFILING}}
  -open build/yup.xcodeproj

ios:
  cmake -G Xcode -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM=OS64
  -open build/yup.xcodeproj

ios_simulator:
  cmake -G Xcode -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM=SIMULATORARM64
  -open build/yup.xcodeproj

android:
  cmake -G "Unix Makefiles" -B build -DYUP_TARGET_ANDROID=ON
  -open -a /Applications/Android\ Studio.app build/examples/render

emscripten CONFIG="Debug":
  emcc -v
  emcmake cmake -G "Ninja Multi-Config" -B build
  cmake --build build --config {{CONFIG}}
  python3 -m http.server -d .
  #python3 tools/serve.py -p 8000 -d .
