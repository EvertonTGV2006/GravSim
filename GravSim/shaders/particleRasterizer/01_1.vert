#version 460



struct LineInfo{
    float eccentricity;
    float apoapsis;
    float periapsis;
    float cost;
};

layout(constant_id = 0) const uint MAX_PLANETS_ARRAY_SIZE = 2;
layout(constant_id = 1) const uint SATELLITE_COUNT = 256;
layout(constant_id = 2) const uint LINE_VERTEX_COUNT = 1024;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 models[MAX_PLANETS_ARRAY_SIZE];
} ubo;



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
    const float velMax = 15;
    const float velMin = 0;
    gl_PointSize = 1.0;
    uint i = gl_InstanceIndex;

    //vec3 Color = {1.0f, 1.0f, 1.0f};
    //float mass = inVelocity.w;
    gl_Position = ubo.proj*ubo.view*ubo.models[i]*vec4(inPosition, 1.0);
    fragPos = (ubo.models[i]*vec4(inPosition, 1.0)).xyz;
    //float velMod = length(inVelocity);
    //velMod = clamp(velMod, velMin, velMax);
    //float velModAdj = (velMod - velMin) * 360 / (velMax - velMin); 

    //fragColor = HSVtoRGB(velModAdj, 1.0f, 1.0f);

    fragColor = inColor;
    //fragColor = normalize(inVelocity.xyz) + inColor * 0.3;
    fragNormal = (ubo.models[i]*vec4(inPosition.xyz, 0.0)).xyz;

    
    
    fragTexCoord = inTexCoord;
    
}



