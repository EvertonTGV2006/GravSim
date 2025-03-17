#version 460

struct CardMat{
    mat4 tr;
    uint card;
	uint mod1;
	uint mod2;
	uint mod3;
}


layout(binding = 0) uniform UniformBufferObject{
    CardMat data[64];
} ubo;

layout(push_constant) uniform pc {
    mat4 viewMat;
    mat4 viewProjMat;
    vec4 pos;
    vec4 dir;
    vec4 colour;
    vec4 eyePos;
};
float aspectRatio = 4.0f / 3.0f;

layout(location = 0) in vec4 inPosition;


layout(location = 0) out vec2 cardBaseCoord;
layout(location = 1) out vec2 cardBaseBlendCoord;
layout(location = 2) out vec2 cardValCoord;
layout(location = 3) out float blendConstant;
layout(location = 4) flat out uint mode;
layout(location = 5) out vec3 normal;
layout(location = 6) out vec3 fragPos;

int characters[11] = int[](72, 101, 108, 108, 111, 32, 87, 111, 114, 108, 100);

void main() {
    uint i = gl_InstanceIndex;

    CardMat card = ubo.data[i];
    float suit = floor(card.card / 16.0f);
    float rank = card.card - (suit * 16.0f);

    vec3 cardDimensions = vec3(1.0f, 1.0f * (95.0f / 71.0f), 0.0015f);
    vec2 texDimensions = vec2(1.0f / 13.0f, 1.0f / 4.0f);

    vec2 backCardLoc = vec2(6, 2);
    vec2 baseCardLoc = vec2(1, 0);
    vec2 baseCardBlendLoc = vec2(6,0);
    vec2 baseCardDim = vec2(1.0f / 7.0f, 1.0f / 5.0f);
    vec4 outPosition;
    vec4 cardNormal;

    if(inPosition.w == 0.0f){
        cardNormal = vec4(0.0f, 0.0f, 1.0f, 0.0f);

        cardBaseCoord.x = (backCardLoc.x + inPosition.x+0.5f) * baseCardDim.x;
        cardBaseCoord.y = (backCardLoc.y + inPosition.y+0.5f) * baseCardDim.y;
        cardValCoord = vec2(0.0f,0.0f);
        mode = 0;
    }
    else{
                cardNormal = vec4(0.0f, 0.0f, -1.0f, 0.0f);

        cardValCoord.x = (float(rank) + inPosition.x+0.5f) * texDimensions.x;
        cardValCoord.y = (float(suit) + inPosition.y+0.5f) * texDimensions.y;
        cardBaseCoord.x = (baseCardLoc.x + inPosition.x+0.5f) * baseCardDim.x;
        cardBaseCoord.y = (baseCardLoc.y + inPosition.y+0.5f) * baseCardDim.y;

        cardBaseBlendCoord.x = (baseCardBlendLoc.x + inPosition.x+0.5f) * baseCardDim.x;
        cardBaseBlendCoord.y = (baseCardBlendLoc.y + inPosition.y+0.5f) * baseCardDim.y;

        outPosition = vec4((cardMat* vec4(cardDimensions.x * inPosition.x, cardDimensions.y * inPosition.y, cardDimensions.z * inPosition.z, 1.0f)).xyz, 1.0f);
        
        mode = 1;
    }
    inPosition.w = 1.0f;
    outPosition = card.tr * inPosition;
    fragPos = outPosition.xyz;

    gl_Position = viewProjMat * outPosition;
}



