#version 460

layout(binding = 1) uniform sampler2D[2] texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 colour;
layout(location = 2) flat in uint mode;

layout(location = 0) out vec4 outColour;

void main(){
    if (mode == 0){
        outColour = texture(texSampler[0], fragTexCoord);
    }
    else if (mode==1){
        vec4 sampleColor = texture(texSampler[1], fragTexCoord);
        //outColour = sampleColor * sampleColor.w + vec4(colour, 1.0f) * (1-sampleColor.w);
        //outColour = (sampleColor * sampleColor.a) + (vec4(colour, 1.0f) * 0.0f * (0.0f-sampleColor.a));
        outColour = vec4(sampleColor.xyz * sampleColor.a + colour * (1.0f - sampleColor.a), 1.0f);
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