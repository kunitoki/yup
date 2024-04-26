#include "rive_pls_renderer.h"

#if RIVE_WEBGPU
#include "source/webgpu/em_js_handle.cpp"
#include "source/webgpu/pls_render_context_webgpu_impl.cpp"
#elif RIVE_WEBGL
#include "source/gl/pls_impl_framebuffer_fetch.cpp"
#include "source/gl/pls_impl_ext_native.cpp"
#include "source/gl/pls_render_buffer_gl_impl.cpp"
#include "source/gl/pls_impl_webgl.cpp"
#include "source/gl/load_gles_extensions.cpp"
#include "source/gl/pls_render_context_gl_impl.cpp"
#include "source/gl/pls_impl_rw_texture.cpp"
#include "source/gl/gl_utils.cpp"
#include "source/gl/load_store_actions_ext.cpp"
#include "source/gl/pls_render_target_gl.cpp"
#include "source/gl/gl_state.cpp"
#endif
