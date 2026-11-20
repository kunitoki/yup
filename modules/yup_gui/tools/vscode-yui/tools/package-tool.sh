#!/bin/sh
set -eu

extension_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
repo_dir=$(CDPATH= cd -- "$extension_dir/../../../.." && pwd)
build_dir=${YUI_TOOL:-$repo_dir/cmake/tools/yui_tool/build_wasm}
destination="$extension_dir/tool"

if [ ! -f "$build_dir/yup_yui_tool.js" ] || [ ! -f "$build_dir/yup_yui_tool.wasm" ]; then
    echo "YUI tool for the web not found in: $build_dir" >&2
    echo "Build it with: just yui_tool_wasm" >&2
    exit 2
fi

rm -rf "$destination"
mkdir -p "$destination"
cp "$build_dir/yup_yui_tool.js" "$build_dir/yup_yui_tool.wasm" "$destination/"
