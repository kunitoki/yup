/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

//==============================================================================
/** A metallic-roughness PBR material, following the glTF 2.0 material model.

    Every factor multiplies its texture, and a missing texture reads as white (or as a flat
    normal for the normal texture). Factors are linear values; the base color and emissive
    textures are sRGB encoded.

    Materials are shared between meshes: changing one changes every primitive that uses it.

    @see Mesh, MaterialNode, Texture
*/
class YUP_API Material : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<Material>;

    /** How the alpha channel of the base color is used. */
    enum class AlphaMode
    {
        opaque, ///< Alpha is ignored and the surface is fully opaque.
        mask,   ///< Fragments with alpha below alphaCutoff are discarded, the rest are opaque.
        blend   ///< The surface is alpha blended over what is behind it.
    };

    //==============================================================================
    /** Creates a white, fully rough, fully metallic material, like the glTF defaults. */
    Material() = default;

    //==============================================================================
    String name;                                         ///< The name of the material.

    std::array<float, 4> baseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f }; ///< Linear RGBA base color.
    Texture::Ptr baseColorTexture;                       ///< sRGB base color and alpha.

    float metallicFactor = 1.0f;                         ///< Metalness, 0 for dielectrics.
    float roughnessFactor = 1.0f;                        ///< Perceptual roughness.
    Texture::Ptr metallicRoughnessTexture;               ///< Roughness in green, metalness in blue.

    Texture::Ptr normalTexture;                          ///< Tangent space normals.
    float normalScale = 1.0f;                            ///< Scales the x and y of the sampled normals.

    Texture::Ptr occlusionTexture;                       ///< Ambient occlusion in red.
    float occlusionStrength = 1.0f;                      ///< How much of the occlusion is applied.

    std::array<float, 3> emissiveFactor { 0.0f, 0.0f, 0.0f }; ///< Linear RGB emitted light.
    Texture::Ptr emissiveTexture;                        ///< sRGB emitted light.

    AlphaMode alphaMode = AlphaMode::opaque;             ///< How alpha is used.
    float alphaCutoff = 0.5f;                            ///< Threshold for AlphaMode::mask.
    bool doubleSided = false;                            ///< True to draw back faces too, with flipped normals.

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Material)
};

} // namespace yup
