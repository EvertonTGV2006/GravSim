#version 460

layout(binding = 1) uniform sampler2D[2] texSampler;

layout(location = 0) in vec2 cardBaseCoord;
layout(location = 1) in vec2 cardBaseBlendCoord;
layout(location = 2) in vec2 cardValCoord;
layout(location = 3) in float blendConstant;
layout(location = 4) flat in uint mode;

layout(location = 0) out vec4 outColour;

float cornerBlendRadius;
float cardHeight;

void main(){
    if (mode == 0){
        outColour = texture(texSampler[0], cardBaseCoord);
    }
    else if (mode==1){
        vec4 sampleColor = texture(texSampler[1], cardValCoord);
        vec4 baseColorA = texture(texSampler[0], cardBaseCoord);
        vec4 baseColorB = texture(texSampler[0], cardBaseBlendCoord);
        vec4 baseColor = (1.0f - blendConstant) * baseColorA + blendConstant * baseColorB;


        //outColour = sampleColor * sampleColor.w + vec4(colour, 1.0f) * (1-sampleColor.w);
        //outColour = (sampleColor * sampleColor.a) + (vec4(colour, 1.0f) * 0.0f * (0.0f-sampleColor.a));
        outColour = vec4(sampleColor.xyz * sampleColor.a + baseColor.xyz * (1.0f - sampleColor.a), baseColor.a);

        if(baseColor.a == 0.0f){
            discard;
        }

        //outColour = vec4(sampleColor.a, 0.0f, 0.0f, 1.0f);

        //outColour = vec4(sampleColor.xyz, 1.0f);
        //outColour = vec4(fragTexCoord.x, fragTexCoord.y, 0.0f, 1.0f);

        //outColour = sampleColor;

        //outColour = vec4(fragTexCoord, 0.0f, 1.0f);
    }
    //outColour = vec4(outColour.x, outColour.y, 0.0, 1.0);
    //outColour = vec4(fragTexCoord.x, 1.0 - fragTexCoord.y, 0.0, 1.0);
    //outColour = vec4(fragTexCoord.x, fragTexCoord.y * 0, 0.0, 1.0);
    //outColour = vec4(1.0, 1.0, 1.0, 0.0);
}