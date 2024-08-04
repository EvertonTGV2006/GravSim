#version 450

struct Particle {
	vec4 position;
	vec4 velocity;
    uint cell;
    uint newIndex;
};



layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(std140, binding = 1) readonly buffer ParticleSSBOIn {
   Particle particlesIn[ ];
};

layout(push_constant) uniform pc {
    mat4 model;
                    
} constants;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec2 fragTexCoord;

vec3 HSVtoRGB(float hue, float sat, float val){
    //float hueAdj = hue / 60;
    //float chr = val * sat;
    //float xin = chr * ( 1 - abs(mod(hueAdj, 2) - 1));
    //vec3 RGBraw = vec3(0.0f, 0.0f, 0.0f);
    //if (0 < hueAdj < 1){RGBraw = vec3(chr, xin, 0.0f)};
    //else if (1 < hueAdj < 2){RGBraw = vec3(xin, chr, 0.0f)};
    //else if (2 < hueAdj < 3){RGBraw = vec3(0.0f, chr, xin)};
    //else if (3 < hueAdj < 4){RGBraw = vec3(0.0f, xin, chr)};
    //else if (4 < hueAdj < 5){RGBraw = vec3(xin, 0.0f, chr)};
    //else if (5 < hueAdj < 6){RGBraw = vec3(chr, 0.0f, xin)};
    //float madj = val - chr;
    //vec3 RGB = RGBraw + vec3(madj, madj, madj);
    //return RGB
    
    float k = mod(5 + hue/60, 6);
    float r = val - val * sat * max(0, min(k, min(4-k, 1)));
    k = mod(3 + hue/60, 6);
    float g = val - val * sat * max(0, min(k, min(4-k, 1)));
    k = mod(1 + hue/60, 6);
    float b = val - val * sat * max(0, min(k, min(4-k, 1)));

    return vec3(r, g, b);


}





void main() {
    Particle particleIn = particlesIn[gl_InstanceIndex];
    const float velMax = 15;
    const float velMin = 0;


    uint mode = 1;

    if (mode==0){
        gl_Position = ubo.proj*ubo.view*constants.model*vec4(inPosition, 1.0);    
        fragPos = (constants.model*vec4(inPosition, 1.0)).xyz;
        fragColor = inColor;
        fragNormal = vec3(0.0, 0.0, 1.0);
    }
    else if (mode==1){
    vec3 Color = {1.0f, 1.0f, 1.0f};
    float mass = particleIn.velocity.w;
        gl_Position = ubo.proj*ubo.view*constants.model*vec4((inPosition*mass+particleIn.position.xzy), 1.0);
        fragPos = (constants.model*vec4(inPosition+vec3(particleIn.position.xzy), 1.0)).xyz;
        float velMod = length(particleIn.velocity.xyz);
        velMod = clamp(velMod, velMin, velMax);
        float velModAdj = (velMod - velMin) * 360 / (velMax - velMin); 

        fragColor = HSVtoRGB(velModAdj, 1.0f, 1.0f);

        //fragColor = inColor;
        //fragColor = normalize(particleIn.velocity.xyz) + inColor * 0.3;
        fragNormal = (constants.model*vec4(inPosition, 0.0)).xyz;
    }
    
    
    fragTexCoord = inTexCoord;
    
}



