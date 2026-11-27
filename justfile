alias c := clean

[doc("list available recipes")]
default:
  @just --list

#==================================================================================================
# Global commands
#==================================================================================================

[confirm("Are you sure you want to clean the build/{{PLATFORM}} folder? [y/N]")]
[doc("clean single platform build artifacts")]
clean PLATFORM="mac":
  rm -Rf build/{{PLATFORM}}/*

[confirm("Are you sure you want to clean the build folder? [y/N]")]
[doc("clean project build artifacts")]
cleanall:
  rm -Rf build/*

#==================================================================================================
# Generic operations
#==================================================================================================

[doc("build project using cmake")]
build PLATFORM="mac" CONFIG="Debug" TARGET="yup_tests":
  cmake --build build/{{PLATFORM}} --config {{CONFIG}} --target {{TARGET}}

[doc("generate and open project using Ninja multi config")]
ninja PROFILING="OFF" VULKAN="OFF":
  cmake -G "Ninja Multi-Config" -B build/ninja -DYUP_ENABLE_PROFILING={{PROFILING}} -DYUP_ENABLE_VULKAN={{VULKAN}}

[doc("execute unit tests using cmake and ctest (MODULE selects a single module, e.g. yup_core)")]
[macos]
test CONFIG="Debug" GTEST_FILTER="*" MODULE="":
  cmake -G Xcode -B build/mac
  cmake --build build/mac --target yup_tests --config {{CONFIG}}
  GTEST_FILTER="{{GTEST_FILTER}}" ctest --test-dir build/mac/tests -C {{CONFIG}} --output-on-failure -R "^{{MODULE}}"

#==================================================================================================
# Platform configuration
#==================================================================================================

[doc("generate and open project in macOS using Xcode")]
[macos]
mac PROFILING="OFF":
  cmake -G Xcode -B build/mac -DYUP_ENABLE_PROFILING={{PROFILING}}
  -open build/mac/yup.xcodeproj

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

#==================================================================================================
# Emscripten specific
#==================================================================================================

[doc("run Debug tests for WASM (MODULE selects a single module, e.g. yup_core)")]
emscripten_test_debug GTEST_FILTER="*" MODULE="":
  @just build emscripten Debug
  GTEST_FILTER="{{GTEST_FILTER}}" ctest --test-dir build/emscripten/tests -C Debug --output-on-failure -R "^{{MODULE}}"

[doc("run Release tests for WASM (MODULE selects a single module, e.g. yup_core)")]
emscripten_test_release GTEST_FILTER="*" MODULE="":
  @just build emscripten Release
  GTEST_FILTER="{{GTEST_FILTER}}" ctest --test-dir build/emscripten/tests -C Release --output-on-failure -R "^{{MODULE}}"

[doc("serve project for WASM")]
emscripten_serve INTERFACE="localhost":
  #uv run python -m http.server -d .
  uv run python tools/serve.py -i {{INTERFACE}} -p 8000 -d .

#==================================================================================================
# Python operations
#==================================================================================================

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
  uv pip install --force-reinstall dist/yup_audio-*.whl

[doc("uninstall python wheel for yup_python bindings")]
[working-directory: 'python']
python_uninstall:
  uv pip uninstall yup-audio

[doc("run tests for yup_python bindings")]
[working-directory: 'python']
python_test *TEST_OPTS:
  uv sync --group test
  uv run --group test python -m pytest -s {{TEST_OPTS}}

#==================================================================================================
# YSL and 3D tooling
#==================================================================================================

[doc("compile and invoke yup_shader_bundler tool")]
[working-directory: 'cmake/tools/shader_bundler']
shader_bundler *COMPILE_ARGS:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=
  cmake --build build --config Release -j4
  build/yup_shader_bundler {{COMPILE_ARGS}}

[doc("fit the yup_renderer GGX area light tables into modules/yup_renderer/core/yup_LtcTables.inc")]
[working-directory: 'cmake/tools/ltc_fitter']
ltc_tables:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=
  cmake --build build --config Release -j4
  build/yup_ltc_fitter --output "{{justfile_directory()}}/modules/yup_renderer/core/yup_LtcTables.inc"

[doc("time the Scene3D target benchmarks on the GPU: ACTION is run, record (the baseline) or compare (with it)")]
[macos]
perf_3d ACTION="run" CONFIG="Release" FILTER="SceneRendererBenchmarkTests.DISABLED_*TargetFrameTimes":
  uv run python tools/perf.py --just "{{just_executable()}}" {{ACTION}} {{CONFIG}} "{{FILTER}}"

[doc("time the 2D painting, restyling and document benchmarks: ACTION is run, record (the baseline) or compare (with it)")]
[macos]
perf_2d ACTION="run" CONFIG="Release" FILTER="PaintBenchmarkTests.DISABLED_*":
  uv run python tools/perf.py --just "{{just_executable()}}" --suite 2d {{ACTION}} {{CONFIG}} "{{FILTER}}"

#==================================================================================================
# YDSP tooling
#==================================================================================================

[doc("compile and invoke yup_dsp_compiler tool")]
[working-directory: 'cmake/tools/ydsp_compiler']
dsp_compiler *COMPILE_ARGS:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=
  cmake --build build --config Release -j4
  build/yup_dsp_compiler {{COMPILE_ARGS}}

[doc("build and install the YDSP VSCode extension")]
[working-directory: 'modules/yup_dsp_jit/tools/vscode-ydsp']
vscode_ydsp:
  @just dsp_compiler --help
  sh tools/package-server.sh
  npm install
  npm run compile
  npx --yes @vscode/vsce package -o vscode-ydsp.vsix
  npx --yes @vscode/vsce ls --tree
  code --install-extension vscode-ydsp.vsix --force

#==================================================================================================
# YUI tooling
#==================================================================================================

[doc("compile and invoke yup_yui_tool tool")]
[working-directory: 'cmake/tools/yui_tool']
yui_tool *COMPILE_ARGS:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=
  cmake --build build --config Release -j4
  build/yup_yui_tool {{COMPILE_ARGS}}

[doc("compile yup_yui_tool and run its command line tests")]
[working-directory: 'cmake/tools/yui_tool']
yui_tool_test:
  @just yui_tool --help
  uv run python tests/test_cli.py build/yup_yui_tool

[doc("compile yup_yui_tool for the web, as the YUI VSCode extension runs it (needs the Emscripten SDK activated)")]
[working-directory: 'cmake/tools/yui_tool']
yui_tool_wasm CONFIG="Release" *CMAKE_ARGS:
  emcmake cmake -G Ninja -S . -B build_wasm -DCMAKE_BUILD_TYPE={{CONFIG}} {{CMAKE_ARGS}}
  cmake --build build_wasm

[doc("compile yup_yui_tool for the web and run its tests")]
[working-directory: 'cmake/tools/yui_tool']
yui_tool_wasm_test:
  @just yui_tool_wasm
  node --test tests/test_web.js

[doc("check the types of the YUI VSCode extension and run its tests that need no editor")]
[working-directory: 'modules/yup_gui_markup/tools/vscode-yui']
vscode_yui_test:
  npm install
  npm run check
  npm test

[doc("build and install the YUI VSCode extension (needs the Emscripten SDK activated)")]
[working-directory: 'modules/yup_gui_markup/tools/vscode-yui']
vscode_yui:
  @just yui_tool_wasm
  sh tools/package-tool.sh
  npm install
  npm run compile
  npx --no-install vsce package -o vscode-yui.vsix
  npx --no-install vsce ls --tree
  code --install-extension vscode-yui.vsix --force

#==================================================================================================
# Developer tooling
#==================================================================================================

[doc("fetch missing coverage lines for a pull request")]
fetch_coverage PR:
  uv run python tools/print_uncovered_lines.py --pr {{PR}}

[doc("sample a running process (example_graphics by default) and print the call graph")]
[macos]
sample SECONDS="5" NAME="example_graphics" PID="":
  uv run python tools/sample_process.py --name {{NAME}} --seconds {{SECONDS}} {{ if PID != "" { "--pid " + PID } else { "" } }}

[doc("update rive runtime")]
rive_update REF="runtime-v0.1.700":
  uv run python tools/rive_update.py --rive-ref {{REF}} --allow-dirty --keep-work-dir

[doc("check that every rive manifest patch is present in thirdparty/rive")]
rive_check_patches:
  uv run python tools/rive_check_patches.py

[doc("update rive shaders")]
rive_shaders_update:
  uv venv .venv --clear
  source .venv/bin/activate
  uv pip install ply
  uv run make -C thirdparty/rive/source/renderer/shaders -j 8
  cp -R thirdparty/rive/source/renderer/shaders/out/generated/* thirdparty/rive/source/renderer/generated/shaders/
  rm -Rf thirdparty/rive/source/renderer/shaders/out
  .venv/bin/deactivate

[doc("regenerate rive d3d shaders (needs make, python3 and the Windows SDK fxc on PATH)")]
[windows]
rive_shaders_d3d:
  @command -v fxc >/dev/null || { echo "fxc not found: run from a Developer Command Prompt or add the Windows SDK bin folder to PATH"; exit 1; }
  python3 -m pip install --quiet --target build/rive-shaders-ply ply
  make -C thirdparty/rive/source/renderer/shaders d3d FLAGS=--ply-path=../../../../../build/rive-shaders-ply
  cp thirdparty/rive/source/renderer/shaders/out/generated/d3d/*.h thirdparty/rive/source/renderer/generated/shaders/d3d/
  rm -Rf thirdparty/rive/source/renderer/shaders/out build/rive-shaders-ply

[doc("build the home page demos into website/src/public/yup")]
website_demos CONFIG="Release":
  @just emscripten {{CONFIG}} website_demos
  cp build/emscripten/website/src/demos/{{CONFIG}}/website_demos.js website/src/public/yup/
  cp build/emscripten/website/src/demos/{{CONFIG}}/website_demos.wasm website/src/public/yup/

[doc("develop website")]
[working-directory: 'website']
website: website_demos
  npm install
  npm run dev

[doc("update the example graphics demo")]
update_emscripten_example NAME DEST DEMOPATH:
  sed -i '' -e 's/YUP_EXAMPLE_GRAPHICS_DEMO:STRING=.*/YUP_EXAMPLE_GRAPHICS_DEMO:STRING={{NAME}}/g' build/emscripten/CMakeCache.txt
  @just emscripten Release example_graphics
  mkdir -p "{{DEMOPATH}}/{{DEST}}"
  cp -R build/emscripten/examples/graphics/Release/* "{{DEMOPATH}}/{{DEST}}"

[doc("update the example graphics demos")]
update_emscripten_examples DEMOPATH="../yup-demos/demos":
  @just emscripten Release example_graphics
  @just update_emscripten_example CodeEditor code-editor {{DEMOPATH}}
  @just update_emscripten_example Component3D component-3d {{DEMOPATH}}
  @just update_emscripten_example ComponentEffects component-effects {{DEMOPATH}}
  @just update_emscripten_example ComputeParticles compute-particles {{DEMOPATH}}
  @just update_emscripten_example Emoji emoji {{DEMOPATH}}
  @just update_emscripten_example Filter filter {{DEMOPATH}}
  @just update_emscripten_example FluidSimulation fluid-simulation {{DEMOPATH}}
  @just update_emscripten_example ListBox list-box {{DEMOPATH}}
  @just update_emscripten_example Lottie lottie {{DEMOPATH}}
  @just update_emscripten_example Images image-mesh {{DEMOPATH}}
  @just update_emscripten_example Paths paths {{DEMOPATH}}
  @just update_emscripten_example Pbr pbr {{DEMOPATH}}
  @just update_emscripten_example Scene3D scene3d {{DEMOPATH}}
  @just update_emscripten_example Shaders shaders {{DEMOPATH}}
  @just update_emscripten_example SpectrumAnalyzer spectrum-analyzer {{DEMOPATH}}
  @just update_emscripten_example Svg svg {{DEMOPATH}}
  @just update_emscripten_example TouchTrails touch-trails {{DEMOPATH}}
  @just update_emscripten_example Themes themes {{DEMOPATH}}
  @just update_emscripten_example TreeView tree-view {{DEMOPATH}}
  @just update_emscripten_example Widgets widgets {{DEMOPATH}}
  @just update_emscripten_example Yui3D yui-3d {{DEMOPATH}}
  @just update_emscripten_example YuiDrawing yui-drawing {{DEMOPATH}}
  @just update_emscripten_example YdspSynths ydsp-synths {{DEMOPATH}}

[doc("render the documentation figures of examples/docsimg into docs/_static/images (FIGURE is a wildcard on their ids)")]
[macos]
docs_images FIGURE="*" CONFIG="Release":
  @[ build/mac/examples/docsimg/example_docsimg.xcodeproj/project.pbxproj -nt examples/docsimg/CMakeLists.txt ] || cmake -G Xcode -B build/mac
  cmake --build build/mac --config {{CONFIG}} --target example_docsimg -- -quiet
  build/mac/examples/docsimg/{{CONFIG}}/example_docsimg.app/Contents/MacOS/example_docsimg --out docs/_static/images --figure "{{FIGURE}}"

[doc("build the documentation into build/docs")]
docs:
  uv run --with-requirements docs/requirements.txt python -m sphinx -b html -q docs build/docs
