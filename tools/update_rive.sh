#!/bin/sh

pushd build
#rm -rf rive-runtime
#git clone https://github.com/rive-app/rive-runtime.git

pushd rive-runtime
pushd renderer/src/shaders

python -m venv cooker
./cooker/bin/python3 -m pip install ply
#gsed -i -e "s/python3/cooker\/bin\/python3/g" Makefile

MINIFY_FLAGS="--ply-path=cooker/lib/python3.11/site-packages"

make FLAGS="${MINIFY_FLAGS}"
make FLAGS="${MINIFY_FLAGS}" rive_pls_macosx_metallib
make FLAGS="${MINIFY_FLAGS}" rive_pls_ios_metallib
make FLAGS="${MINIFY_FLAGS}" rive_pls_ios_simulator_metallib
make FLAGS="${MINIFY_FLAGS}" spirv
make FLAGS="${MINIFY_FLAGS}" spirv-binary

popd # renderer/src/shaders
popd # rive-runtime
popd # build
