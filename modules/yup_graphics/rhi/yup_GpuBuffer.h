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

class GraphicsContext;

//==============================================================================
/** Identifies the intended usage of a GpuBuffer. */
enum class GpuBufferType : uint8_t
{
    vertex,  ///< Per-vertex attribute data, bound via GpuProgram::setVertexBuffer().
    index,   ///< Index data, bound via GpuProgram::setIndexBuffer().
    uniform, ///< Uniform (constant) data.
};

//==============================================================================
/** A reference-counted GPU buffer handle.

    Wraps a backend-native ore buffer holding vertex, index, or uniform data.
    Create one via GpuBuffer::create() and bind it to a GpuProgram for indexed
    or non-indexed geometry rendering. The underlying GPU resource lives for as
    long as at least one GpuBuffer::Ptr exists.

    Buffers are immutable by default: the data provided at creation time is
    uploaded once and cannot be updated afterwards.

    Requires the GraphicsContext to have been created with
    Options::enableOreContext = true.

    @see GpuProgram, GraphicsContext::Options
*/
class YUP_API GpuBuffer : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuBuffer>;

    //==============================================================================
    ~GpuBuffer();

    //==============================================================================
    /** Returns the buffer usage type. */
    GpuBufferType getType() const noexcept;

    /** Returns the size of the buffer in bytes. */
    size_t getSizeInBytes() const noexcept;

    /** Returns true if this buffer holds a valid GPU resource. */
    bool isValid() const noexcept;

    //==============================================================================
    /** Creates a GPU buffer and uploads the given data.

        @param ctx        A GraphicsContext created with enableOreContext = true.
        @param type       The intended usage of the buffer.
        @param data       Pointer to the source data to upload (must be non-null).
        @param byteSize   Number of bytes to upload (must be greater than zero).

        @returns A valid GpuBuffer, or nullptr on failure (ore unavailable or
                 allocation failed).
    */
    static GpuBuffer::Ptr create (GraphicsContext& ctx,
                                  GpuBufferType type,
                                  const void* data,
                                  size_t byteSize);

private:
    friend class GpuProgram;

    GpuBuffer() = default;

    /** Returns the underlying ore buffer pointer, or nullptr. Internal use. */
    rive::ore::Buffer* oreBufferHandle() const noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuBuffer)
};

} // namespace yup
