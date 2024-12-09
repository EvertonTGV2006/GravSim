#version 460

layout(binding = 1) uniform usampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 colour;
layout(location = 2) flat in uint mode;

layout(location = 0) out vec4 outColour;

void main(){
    if (mode == 0){
        vec4 texVal = texture(texSampler, fragTexCoord);
        float alpha = 0.0f;
        if(texVal.x != 0.0f){
            alpha = 1.0f;
        }
        outColour = vec4(colour.xyz, alpha) * texVal.x;
    }
    else if (mode==1){
        outColour = vec4(colour.xyz, 1.0f);
    }
    //outColour = vec4(outColour.x, outColour.y, 0.0, 1.0);
    //outColour = vec4(fragTexCoord.x, 1.0 - fragTexCoord.y, 0.0, 1.0);
    //outColour = vec4(fragTexCoord.x, fragTexCoord.y * 0, 0.0, 1.0);
    //outColour = vec4(1.0, 1.0, 1.0, 0.0);
}