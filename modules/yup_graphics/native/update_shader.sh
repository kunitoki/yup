#!/bin/bash

xcrun -sdk macosx metal yup_RenderShader.metal -o yup_RenderShader_mac.metallib
xxd -i -n yup_RenderShader_data yup_RenderShader_mac.metallib yup_RenderShader_mac.c
rm yup_RenderShader_mac.metallib

xcrun -sdk iphoneos metal yup_RenderShader.metal -o yup_RenderShader_ios.metallib
xxd -i -n yup_RenderShader_data yup_RenderShader_ios.metallib yup_RenderShader_ios.c
rm yup_RenderShader_ios.metallib
