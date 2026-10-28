#!/bin/bash

# Without explicit deployment targets the metallibs require the host SDK OS version and fail to load on older systems
xcrun -sdk macosx metal -std=macos-metal2.3 -mmacosx-version-min=11.0 yup_RenderShader.metal -o yup_RenderShader_mac.metallib
xxd -i -n yup_RenderShader_data yup_RenderShader_mac.metallib yup_RenderShader_mac.c
rm yup_RenderShader_mac.metallib

xcrun -sdk iphoneos metal -std=ios-metal2.2 -mios-version-min=13.0 yup_RenderShader.metal -o yup_RenderShader_ios.metallib
xxd -i -n yup_RenderShader_data yup_RenderShader_ios.metallib yup_RenderShader_ios.c
rm yup_RenderShader_ios.metallib

xcrun -sdk iphonesimulator metal -std=ios-metal2.2 -miphonesimulator-version-min=13.0 yup_RenderShader.metal -o yup_RenderShader_iossim.metallib
xxd -i -n yup_RenderShader_data yup_RenderShader_iossim.metallib yup_RenderShader_iossim.c
rm yup_RenderShader_iossim.metallib

glslangValidator -V --target-env vulkan1.1 --vn yup_PresentShader_vulkan_vert -o yup_PresentShader_vulkan_vert.h yup_PresentShader_vulkan.vert
glslangValidator -V --target-env vulkan1.1 --vn yup_PresentShader_vulkan_frag -o yup_PresentShader_vulkan_frag.h yup_PresentShader_vulkan.frag
