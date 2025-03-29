#version 460

layout(binding = 1) uniform sampler2D[2] texSampler;

layout(location = 0) in vec2 cardBaseCoord;
layout(location = 1) in vec2 cardBaseBlendCoord;
layout(location = 2) in vec2 cardValCoord;
layout(location = 3) in float blendConstant;
layout(location = 4) flat in uint mode;
layout(location = 5) in vec3 normal;
layout(location = 6) in vec3 fragPos;

layout(location = 0) out vec4 finalColour;


layout(push_constant) uniform pc{
    mat4 viewMat;
    mat4 viewProjMat;
    vec4 pos;
    vec4 dir;
    vec4 colour;
    vec4 eyePos;
};

float cornerBlendRadius;
float cardHeight;

void main(){
    vec4 outColour;
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



        //outColour = vec4(sampleColor.a, 0.0f, 0.0f, 1.0f);

        //outColour = vec4(sampleColor.xyz, 1.0f);
        //outColour = vec4(fragTexCoord.x, fragTexCoord.y, 0.0f, 1.0f);

        //outColour = sampleColor;

        //outColour = vec4(fragTexCoord, 0.0f, 1.0f);
    }
    else if(mode==2){
        outColour = vec4(colour.xyz, 1.0f);
    }
    if(outColour.a == 0.0f){
        discard;
    }
    //now we do lighting calculations;
    vec3 fragEye = eyePos.xyz - fragPos.xyz;
    vec3 lightFrag = fragPos.xyz - pos.xyz;

    //diffuse
    float diff = dot(normalize(lightFrag), normalize(normal));
    
    vec4 diffColour = outColour * max(diff, 0.0f);
    
    //ambient
    float amb = 0.1f;
    vec4 ambColour = outColour * amb;

    //specular
    vec3 halfwayVector = normalize(normalize(-fragEye) + normalize(lightFrag));
    float spec = pow(dot(normalize(normal), halfwayVector), pos.a);
    vec4 specColour = outColour * max(spec, 0.0f);

    float ambCoeff = 1.0f;
    float diffCoeff = 0.5f;
    float specCoeff = 0.5f;

    finalColour = (specCoeff * specColour + ambCoeff * ambColour + diffCoeff * diffColour);
    
    //finalColour = ambColour;
    //finalColour = outColour * (amb + spec + diff);

    finalColour.a = outColour.a;
    //finalColour = vec4(normalize(lightFrag), 1.0f);
    //finalColour = diffColour;
    //finalColour = specColour;
    //finalColour = vec4(halfwayVector, 1.0f);
    //finalColour = vec4(normal, 1.0f);
    //outColour = vec4(normal, 1.0f);



    //outColour = vec4(outColour.x, outColour.y, 0.0, 1.0);
    //outColour = vec4(fragTexCoord.x, 1.0 - fragTexCoord.y, 0.0, 1.0);
    //outColour = vec4(fragTexCoord.x, fragTexCoord.y * 0, 0.0, 1.0);
    //outColour = vec4(1.0, 1.0, 1.0, 0.0);
}