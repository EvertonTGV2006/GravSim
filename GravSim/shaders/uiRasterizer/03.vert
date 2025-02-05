#version 460

const uint CHAR_COUNT = 128 - 32;
const uint CHAR_START = 32;
const uint STRING_LENGTH = 4096;

struct UIChar{
    int c;
    float s;
    vec2 sP;
    vec2 sD;
};


layout(binding = 0) uniform UniformBufferObject{
    UIChar strData[STRING_LENGTH];
} ubo;

layout (push_constant) uniform pc{
    float aspect;
};


layout(location = 0) in vec2 inPos;

layout(location = 0) out vec2 tCoord;
layout(location = 1) flat out uint tInd;
layout(location = 2) flat out uint data;



void main() {
    uint i = gl_InstanceIndex;

    UIChar wChar = ubo.strData[i];

    vec2 vPos = vec2(inPos.x*wChar.sD.y, inPos.y*wChar.sD.y);
    vec2 sPos = vPos + wChar.sP;
    vec2 tPos = 2.0f * (sPos - vec2(0.5f, 0.5f));

    gl_Position = vec4(tPos, 0.0, 1.0);
    //gl_Position = vec4(inPos * 0.5f, 0.0, 1.0);
    tCoord = inPos;
    tInd = wChar.c;
    data = 0;


    
}



