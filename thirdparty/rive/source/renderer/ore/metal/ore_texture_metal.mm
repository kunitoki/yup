/*
 * Copyright 2025 Rive
 */

#include "ore_texture_metal.hpp"
#include "rive/renderer/ore/ore_context_metal.hpp"
#include "rive/rive_types.hpp"

#import <Metal/Metal.h>

namespace rive::ore
{

void TextureMetal::uploadImpl(const TextureDataDesc& data)
{
    assert(m_mtlTexture != nil);
    assert(data.data != nullptr);

    MTLRegion region = MTLRegionMake3D(
        data.x, data.y, data.z, data.width, data.height, data.depth);

    // Apple's `replaceRegion:` docs require `bytesPerImage = 0` for
    // non-array 2D textures (Metal API Validation aborts on any other
    // value). For texture3D / array2D the value is the per-slice stride.
    const NSUInteger mtlBytesPerImage =
        (m_type == TextureType::texture3D || m_type == TextureType::array2D)
            ? static_cast<NSUInteger>(data.bytesPerRow) * data.rowsPerImage
            : 0;

    if (m_mtlTexture.storageMode == MTLStorageModePrivate)
    {
        assert(m_ctx != nullptr);
        if (m_format == TextureFormat::depth24plusStencil8 ||
            m_format == TextureFormat::depth32floatStencil8)
        {
            m_ctx->setLastError(
                "upload: combined depth-stencil formats are not supported "
                "on private textures");
            return;
        }
        const uint32_t bpt = textureFormatBytesPerTexel(m_format);
        if (bpt == 0)
        {
            m_ctx->setLastError("upload: block-compressed formats are not "
                                "supported on private textures");
            return;
        }
        const NSUInteger stagingSize =
            mtlBytesPerImage * (data.depth - 1) +
            static_cast<NSUInteger>(data.bytesPerRow) * (data.height - 1) +
            static_cast<NSUInteger>(data.width) * bpt;
        @autoreleasepool
        {
            id<MTLBuffer> staging =
                [m_ctx->device() newBufferWithBytes:data.data
                                             length:stagingSize
                                            options:MTLResourceStorageModeShared];
            if (staging == nil)
            {
                m_ctx->setLastError(
                    "upload: staging buffer allocation failed (size=%llu)",
                    static_cast<unsigned long long>(stagingSize));
                return;
            }
            id<MTLCommandBuffer> commandBuffer = [m_ctx->m_mtlQueue commandBuffer];
            if (commandBuffer == nil)
            {
                m_ctx->setLastError("upload: command buffer creation failed");
                return;
            }
            id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
            [blit copyFromBuffer:staging
                       sourceOffset:0
                  sourceBytesPerRow:data.bytesPerRow
                sourceBytesPerImage:mtlBytesPerImage
                         sourceSize:MTLSizeMake(
                                        data.width, data.height, data.depth)
                          toTexture:m_mtlTexture
                   destinationSlice:data.layer
                   destinationLevel:data.mipLevel
                  destinationOrigin:MTLOriginMake(data.x, data.y, data.z)];
            [blit endEncoding];
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
            if (commandBuffer.status == MTLCommandBufferStatusError)
            {
                m_ctx->setLastError(
                    "upload: blit failed: %s",
                    commandBuffer.error.localizedDescription.UTF8String
                        ?: "unknown");
            }
        }
        return;
    }

    [m_mtlTexture replaceRegion:region
                    mipmapLevel:data.mipLevel
                          slice:data.layer
                      withBytes:data.data
                    bytesPerRow:data.bytesPerRow
                  bytesPerImage:mtlBytesPerImage];
}

} // namespace rive::ore
