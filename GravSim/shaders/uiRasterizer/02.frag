#version 460

layout(binding = 1) uniform usampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColour;

void main(){
    outColour = texture(texSampler, fragTexCoord);
    outColour = vec4(1.0, 1.0, 1.0, 0.0);
}