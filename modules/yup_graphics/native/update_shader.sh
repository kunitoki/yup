#!/bin/bash

xcrun -sdk macosx metal yup_RenderShader.metal -o yup_RenderShader_mac.metallib
xxd -i -n yup_RenderShader_data yup_RenderShader_mac.metallib yup_RenderShader_mac.c
rm yup_RenderShader_mac.metallib

xcrun -sdk iphoneos metal yup_RenderShader.metal -o yup_RenderShader_ios.metallib
xxd -i -n yup_RenderShader_data yup_RenderShader_ios.metallib yup_RenderShader_ios.c
rm yup_RenderShader_ios.metallib

xcrun -sdk iphonesimulator metal yup_RenderShader.metal -o yup_RenderShader_iossim.metallib
xxd -i -n yup_RenderShader_data yup_RenderShader_iossim.metallib yup_RenderShader_iossim.c
rm yup_RenderShader_iossim.metallib

glslangValidator -V --target-env vulkan1.1 --vn yup_PresentShader_vulkan_vert -o yup_PresentShader_vulkan_vert.h yup_PresentShader_vulkan.vert
glslangValidator -V --target-env vulkan1.1 --vn yup_PresentShader_vulkan_frag -o yup_PresentShader_vulkan_frag.h yup_PresentShader_vulkan.frag
