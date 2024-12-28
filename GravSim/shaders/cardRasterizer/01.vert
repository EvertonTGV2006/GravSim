#version 460

layout(binding = 0) uniform UniformBufferObject{
    mat4 cardMat[52];
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

    float suit = floor(i / 13.0f);
    float rank = i - (suit * 13.0f);

    vec3 cardDimensions = vec3(1.0f, 1.0f * (95.0f / 71.0f), 0.0015f);
    vec2 texDimensions = vec2(1.0f / 13.0f, 1.0f / 4.0f);

    vec2 backCardLoc = vec2(6, 2);
    vec2 baseCardLoc = vec2(1, 0);
    vec2 baseCardBlendLoc = vec2(6,0);
    vec2 baseCardDim = vec2(1.0f / 7.0f, 1.0f / 5.0f);
    vec4 outPosition;
    vec4 cardNormal;

    mat4 cardMat = ubo.cardMat[i];
    cardMat[3][3] = 1.0f;
    blendConstant = ubo.cardMat[i][3][3];
    if (blendConstant > 1.0f){
        blendConstant = blendConstant - 1.0f;
        baseCardBlendLoc = vec2(6,1);
    }

    //viewProjMat[3][3] = 1.0f;

    float scale = 0.2f;
    mat4 scaleMat = {{scale, 0, 0, 0}, {0, scale, 0, 0}, {0, 0, scale, 0}, {0, 0, 0, 1}};
    

    if (inPosition.w == 1.0f){
        //card is top surface, so calculate texCoords accordingly, normal points up in z
        cardNormal = vec4(0.0f, 0.0f, -1.0f, 0.0f);

        cardValCoord.x = (float(rank) + inPosition.x+0.5f) * texDimensions.x;
        cardValCoord.y = (float(suit) + inPosition.y+0.5f) * texDimensions.y;
        cardBaseCoord.x = (baseCardLoc.x + inPosition.x+0.5f) * baseCardDim.x;
        cardBaseCoord.y = (baseCardLoc.y + inPosition.y+0.5f) * baseCardDim.y;

        cardBaseBlendCoord.x = (baseCardBlendLoc.x + inPosition.x+0.5f) * baseCardDim.x;
        cardBaseBlendCoord.y = (baseCardBlendLoc.y + inPosition.y+0.5f) * baseCardDim.y;

        outPosition = vec4((cardMat* vec4(cardDimensions.x * inPosition.x, cardDimensions.y * inPosition.y, cardDimensions.z * inPosition.z, 1.0f)).xyz, 1.0f);
        
        mode = 1;
        //colour = vec3(inPosition.x*0.6f, inPosition.y * 0.4f, 0.2f);
        //colour = vec3(1);
        //colour = vec3(fragTexCoord.x, fragTexCoord.y, 0.0f);
    }
    if(inPosition.w==0.0f){

        cardNormal = vec4(0.0f, 0.0f, 1.0f, 0.0f);

        cardBaseCoord.x = (backCardLoc.x + inPosition.x+0.5f) * baseCardDim.x;
        cardBaseCoord.y = (backCardLoc.y + inPosition.y+0.5f) * baseCardDim.y;
        cardValCoord = vec2(0.0f,0.0f);

        outPosition = vec4((cardMat * vec4(cardDimensions.x * inPosition.x, cardDimensions.y * inPosition.y, cardDimensions.z * inPosition.z, 1.0f)).xyz, 1.0f);
        mode = 0;
        //colour = vec3(1);

    }
    normal = (cardMat * cardNormal).xyz;



    fragPos = (scaleMat * outPosition).xyz;
    gl_Position = viewProjMat * scaleMat * outPosition;
}



