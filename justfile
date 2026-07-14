alias c := clean

gtest_filter := "*"

[doc("list available recipes")]
default:
  @just --list

[confirm("Are you sure you want to clean the build folder? [y/N]")]
[doc("clean project build artifacts")]
clean:
  rm -Rf build/*

[doc("build project using cmake")]
build CONFIG="Debug" TARGET="yup_tests":
  cmake --build build --config {{CONFIG}} --target {{TARGET}}

[doc("execute unit tests using cmake")]
[macos]
test CONFIG="Debug":
  cmake -G Xcode -B build
  cmake --build build --target yup_tests --config {{CONFIG}}
  build/tests/{{CONFIG}}/yup_tests.app/Contents/MacOS/yup_tests --gtest_filter={{gtest_filter}}

[doc("generate and open project in macOS using Xcode")]
[macos]
mac PROFILING="OFF":
  cmake -G Xcode -B build -DYUP_ENABLE_PROFILING={{PROFILING}}
  -open build/yup.xcodeproj

[doc("generate and open project using Ninja multi config")]
ninja PROFILING="OFF":
  cmake -G "Ninja Multi-Config" -B build -DYUP_ENABLE_PROFILING={{PROFILING}}

[doc("generate and open project in Windows using Visual Studio")]
[windows]
win PROFILING="OFF":
  cmake -G "Visual Studio 18 2026" -B build -DYUP_ENABLE_PROFILING={{PROFILING}}
  -start build/yup.slnx

[doc("generate project in Linux using Ninja")]
[linux]
linux PROFILING="OFF":
  @just ninja {{PROFILING}}

[doc("generate and open project for iOS using Xcode")]
[macos]
ios PLATFORM="OS64":
  cmake -G Xcode -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM={{PLATFORM}}
  -open build/yup.xcodeproj

[doc("generate and open project for iOS Simulator macOS using Xcode")]
[macos]
ios_simulator PLATFORM="SIMULATORARM64":
  @just ios {{PLATFORM}}

[doc("generate and open project for Android using Android Studio (macos)")]
[macos]
android:
  cmake -G Xcode -B build -DYUP_TARGET_ANDROID=ON
  -open -a /Applications/Android\ Studio.app build/examples/graphics

[doc("generate and open project for Android using Android Studio (windows)")]
[windows]
android:
  cmake -G "Visual Studio 18 2026" -B build -DYUP_TARGET_ANDROID=ON

[doc("generate and open project for Android using Android Studio (linux)")]
[linux]
android:
  cmake -G "Unix Makefiles" -B build -DYUP_TARGET_ANDROID=ON

[doc("generate and build project for WASM")]
emscripten CONFIG="Debug" TARGET="yup_tests":
  emcc -v
  emcmake cmake -G "Ninja Multi-Config" -B build
  @just build {{CONFIG}} {{TARGET}}

[doc("run tests for WASM")]
[working-directory: 'build/tests/Debug/']
emscripten_test:
  @just build Debug
  node yup_tests.js --gtest_filter={{gtest_filter}}

[doc("serve project for WASM")]
emscripten_serve:
  #python3 -m http.server -d .
  python3 tools/serve.py -p 8000 -d .

[working-directory: 'python']
python_wheel:
  python -m build --wheel
  @just python_install
  @just python_test

[working-directory: 'python']
python_install:
  python -m pip install --force-reinstall dist/yup-*.whl

[working-directory: 'python']
python_uninstall:
  python -m pip uninstall -y yup

[working-directory: 'python']
python_test *TEST_OPTS:
  python -m pytest -s {{TEST_OPTS}}

[working-directory: 'cmake/tools/shader_bundler']
shader_bundler *COMPILE_ARGS:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=
  cmake --build build --config Release -j4
  build/yup_shader_bundler {{COMPILE_ARGS}}

rive_update REF="runtime-v0.1.62":
  uv run python tools/rive_update.py --rive-ref {{REF}} --allow-dirty --keep-work-dir
