#version 450
// An image shader tinting the picture it reads, compiled into fx/tint.frag.ysl by yup_add_yui_resources
layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;
    float amount;
} params;
layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);
    color = mix (pixel, pixel * params.tint, params.amount);
}
