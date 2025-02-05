#version 460


layout (binding = 2) uniform utexture2D tex[128];
layout (binding = 3) uniform sampler samp;

layout(location = 0) in vec2 tCoord;
layout(location = 1) flat in uint tInd;
layout(location = 2) flat in uint data;

layout(location = 0) out vec4 outColour;

void main(){
    float col = float(texture(usampler2D(tex[tInd], samp), tCoord))/256.0f;
    outColour = vec4(col, col, col, col);
    //outColour = vec4(1,1,1,1);
}