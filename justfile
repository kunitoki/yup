alias c := clean

gtest_filter := "*"

[doc("list available recipes")]
default:
  @just --list

[confirm("Are you sure you want to clean the build/{{PLATFORM}} folder? [y/N]")]
[doc("clean single platform build artifacts")]
clean PLATFORM="mac":
  rm -Rf build/{{PLATFORM}}/*

[confirm("Are you sure you want to clean the build folder? [y/N]")]
[doc("clean project build artifacts")]
cleanall:
  rm -Rf build/*

[doc("build project using cmake")]
build PLATFORM="mac" CONFIG="Debug" TARGET="yup_tests":
  cmake --build build/{{PLATFORM}} --config {{CONFIG}} --target {{TARGET}}

[doc("execute unit tests using cmake")]
[macos]
test CONFIG="Debug":
  cmake -G Xcode -B build/mac
  cmake --build build/mac --target yup_tests --config {{CONFIG}}
  build/mac/tests/{{CONFIG}}/yup_tests.app/Contents/MacOS/yup_tests --gtest_filter={{gtest_filter}}

[doc("generate and open project in macOS using Xcode")]
[macos]
mac PROFILING="OFF":
  cmake -G Xcode -B build/mac -DYUP_ENABLE_PROFILING={{PROFILING}}
  -open build/mac/yup.xcodeproj

[doc("generate and open project using Ninja multi config")]
ninja PROFILING="OFF":
  cmake -G "Ninja Multi-Config" -B build/ninja -DYUP_ENABLE_PROFILING={{PROFILING}}

[doc("generate and open project in Windows using Visual Studio")]
[windows]
win PROFILING="OFF":
  cmake -G "Visual Studio 18 2026" -B build/win -DYUP_ENABLE_PROFILING={{PROFILING}}
  -start build/win/yup.slnx

[doc("generate project in Linux using Ninja")]
[linux]
linux PROFILING="OFF":
  @just ninja {{PROFILING}}

[doc("generate and open project for iOS using Xcode")]
[macos]
ios IOS_PLATFORM="OS64":
  cmake -G Xcode -B build/ios -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM={{IOS_PLATFORM}}
  -open build/ios/yup.xcodeproj

[doc("generate and open project for iOS Simulator macOS using Xcode")]
[macos]
ios_simulator IOS_PLATFORM="SIMULATORARM64":
  cmake -G Xcode -B build/ios_sim -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/ios.cmake -DPLATFORM={{IOS_PLATFORM}}
  -open build/ios_sim/yup.xcodeproj

[doc("generate and open project for Android using Android Studio (macos)")]
[macos]
android:
  cmake -G Xcode -B build/android -DYUP_TARGET_ANDROID=ON
  -open -a /Applications/Android\ Studio.app build/android/examples/graphics

[doc("generate and open project for Android using Android Studio (windows)")]
[windows]
android:
  cmake -G "Visual Studio 18 2026" -B build/android -DYUP_TARGET_ANDROID=ON

[doc("generate and open project for Android using Android Studio (linux)")]
[linux]
android:
  cmake -G "Unix Makefiles" -B build/android -DYUP_TARGET_ANDROID=ON

[doc("generate and build project for WASM")]
emscripten CONFIG="Debug" TARGET="yup_tests":
  emcc -v
  emcmake cmake -G "Ninja Multi-Config" -B build/emscripten
  @just build emscripten {{CONFIG}} {{TARGET}}

[doc("run Debug tests for WASM")]
[working-directory: 'build/emscripten/tests/Debug/']
emscripten_test_debug:
  @just build emscripten Debug
  node yup_tests.js --gtest_filter={{gtest_filter}}

[doc("run Release tests for WASM")]
[working-directory: 'build/emscripten/tests/Release/']
emscripten_test_release:
  @just build emscripten Release
  node yup_tests.js --gtest_filter={{gtest_filter}}

[doc("serve project for WASM")]
emscripten_serve:
  #uv run python -m http.server -d .
  uv run python tools/serve.py -p 8000 -d .

[doc("generate python wheel for yup_python bindings")]
[working-directory: 'python']
python_wheel:
  uv venv --allow-existing
  uv pip install build
  uv run python -m build --wheel
  @just python_install
  @just python_test

[doc("install python wheel for yup_python bindings")]
[working-directory: 'python']
python_install:
  uv pip install --force-reinstall dist/yup-*.whl

[doc("uninstall python wheel for yup_python bindings")]
[working-directory: 'python']
python_uninstall:
  uv pip uninstall -y yup

[doc("run tests for yup_python bindings")]
[working-directory: 'python']
python_test *TEST_OPTS:
  uv sync --group test
  uv run --group test python -m pytest -s {{TEST_OPTS}}

[doc("compile and invoke yup_shader_bundler tool")]
[working-directory: 'cmake/tools/shader_bundler']
shader_bundler *COMPILE_ARGS:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=
  cmake --build build --config Release -j4
  build/yup_shader_bundler {{COMPILE_ARGS}}

[doc("compile and invoke yup_dsp_compiler tool")]
[working-directory: 'cmake/tools/ydsp_compiler']
dsp_compiler *COMPILE_ARGS:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=
  cmake --build build --config Release -j4
  build/yup_dsp_compiler {{COMPILE_ARGS}}

[doc("build and install the YDSP VSCode extension")]
[working-directory: 'modules/yup_dsp_jit/tools/vscode-ydsp']
vscode:
  @just dsp_compiler --help
  sh tools/package-server.sh
  npm install
  npm run compile
  npx --yes @vscode/vsce package -o vscode-ydsp.vsix
  npx --yes @vscode/vsce ls --tree
  code --install-extension vscode-ydsp.vsix --force

[doc("fetch missing coverage lines for a pull request")]
fetch_coverage PR:
  uv run python tools/print_uncovered_lines.py --pr {{PR}}

[doc("update rive runtime")]
rive_update REF="runtime-v0.1.62":
  uv run python tools/rive_update.py --rive-ref {{REF}} --allow-dirty --keep-work-dir

[doc("update rive shaders")]
rive_shaders_update:
  uv venv .venv --clear
  source .venv/bin/activate
  uv pip install ply
  uv run make -C thirdparty/rive/source/renderer/shaders -j 8
  cp -R thirdparty/rive/source/renderer/shaders/out/generated/* thirdparty/rive/source/renderer/generated/shaders/
  rm -Rf thirdparty/rive/source/renderer/shaders/out
  .venv/bin/deactivate

[doc("update the example graphics demo")]
update_emscripten_example NAME DEST DEMOPATH:
  sed -i '' -e 's/YUP_EXAMPLE_GRAPHICS_DEMO:STRING=.*/YUP_EXAMPLE_GRAPHICS_DEMO:STRING={{NAME}}/g' build/emscripten/CMakeCache.txt
  @just emscripten Release example_graphics
  cp -R build/emscripten/examples/graphics/Release/* "{{DEMOPATH}}/{{DEST}}"

[doc("update the example graphics demos")]
update_emscripten_examples DEMOPATH="../yup-demos/demos":
  @just emscripten Release example_graphics
  @just update_emscripten_example Component3D component-3d {{DEMOPATH}}
  @just update_emscripten_example ComponentEffects component-effects {{DEMOPATH}}
  @just update_emscripten_example Filter filter {{DEMOPATH}}
  @just update_emscripten_example FluidSimulation fluid-simulation {{DEMOPATH}}
  @just update_emscripten_example Lottie lottie {{DEMOPATH}}
  @just update_emscripten_example Pbr pbr {{DEMOPATH}}
  @just update_emscripten_example SpectrumAnalyzer spectrum-analyzer {{DEMOPATH}}
  @just update_emscripten_example Svg svg {{DEMOPATH}}
  @just update_emscripten_example TouchTrails touch-trails {{DEMOPATH}}
  @just update_emscripten_example Widgets widgets {{DEMOPATH}}
  @just update_emscripten_example YdspSynths ydsp-synths {{DEMOPATH}}
