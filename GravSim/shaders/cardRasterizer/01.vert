#version 460

layout(binding = 0) uniform UniformBufferObject{
    mat4 cardMat[52];
} ubo;

layout(push_constant) uniform pc {
    vec2 charDimensions;
    vec2 screenPosition;
    vec2 texDimensions;
    float texAdvance;
    uint renderStage;  
    vec4 inColour;  
    int instanceOffset;   
    
};


layout(location = 0) in vec4 inPosition;


layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 colour;
layout(location = 2) out uint mode;

int characters[11] = int[](72, 101, 108, 108, 111, 32, 87, 111, 114, 108, 100);

void main() {
    uint i = gl_InstanceIndex;

    uint rank = i % 13;
    uint suit = (i - rank) / 4;

    vec3 cardDimensions = vec3(0.5f, 0.5f * 95.0f / 71.0f, 0.015f);
    vec2 texDimensions = vec2(1.0f / 13.0f, 1.0f / 4.0f);

    vec2 backCardLoc = vec2(6, 2);
    vec2 backCardDim = vec2(1.0f / 7.0f, 1.0f / 5.0f);

    if (inPosition.w == 1.0f){
        //card is top surface, so calculate texCoords accordingly
        fragTexCoord.x = (rank - 1 + inPosition.x) * texDimensions.x;
        fragTexCoord.y = (suit + inPosition.y) * texDimensions.y;

        gl_Position = vec4((ubo.cardMat[i] * vec4(cardDimensions.x * inPosition.x, cardDimensions.y * inPosition.y, cardDimensions.z * inPosition.z, 1.0f)).xyz, 1.0f);
        mode = 1;
        //colour = vec3(inPosition.x*0.6f, inPosition.y * 0.4f, 0.2f);
        colour = vec3(1);
    }
    if(inPosition.w==0.0f){
        fragTexCoord.x = (backCardLoc.x + inPosition.x) * backCardDim.x;
        fragTexCoord.y = (backCardLoc.y + inPosition.y) * backCardDim.y;
        gl_Position = vec4((ubo.cardMat[i] * vec4(cardDimensions.x * inPosition.x, cardDimensions.y * inPosition.y, cardDimensions.z * inPosition.z, 1.0f)).xyz, 1.0f);
        mode = 0;
        colour = vec3(1);

    }
}



