#!/bin/sh
set -eu

extension_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
repo_dir=$(CDPATH= cd -- "$extension_dir/../../../.." && pwd)
compiler_path=${YDSP_COMPILER:-$repo_dir/cmake/tools/ydsp_compiler/build/yup_dsp_compiler}

if [ "$(uname -s)" = "Darwin" ]; then
    platform=darwin
elif [ "$(uname -s)" = "Linux" ]; then
    platform=linux
else
    platform=win32
fi

case "$(uname -m)" in
    arm64|aarch64) architecture=arm64 ;;
    x86_64|amd64) architecture=x64 ;;
    *) echo "Unsupported compiler architecture: $(uname -m)" >&2; exit 2 ;;
esac

if [ ! -x "$compiler_path" ]; then
    echo "YDSP compiler not found: $compiler_path" >&2
    echo "Build it with: just dsp_compiler" >&2
    exit 2
fi

destination="$extension_dir/server/$platform-$architecture"
if [ "$platform" = "win32" ]; then
    compiler_name=yup_dsp_compiler.exe
else
    compiler_name=yup_dsp_compiler
fi
mkdir -p "$destination"
cp "$compiler_path" "$destination/$compiler_name"
chmod 755 "$destination/$compiler_name"
