#version 460

//#extension GL_EXT_fragment_shader_barycentric : enable

layout(binding = 1) uniform sampler2D texSampler;




layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 pos;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColour;

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
    //outColor = texture(texSampler, fragTexCoord);
    float ambientStrength = 0.1;
    float specularStrength = 0.5;

    //vec3 normal = normalize(fragNormal);
    //vec3 lightRay = normalize(constants.lightPos.xyz - pos);
    //vec3 viewRay = normalize(constants.cameraPos.xyz-pos);
    //vec3 reflectRay = reflect(-lightRay, normal);

    
    //vec3 ambient = ambientStrength * constants.lightColour.xyz;

    //vec3 diffuse = max(dot(normal, lightRay), 0.0) * constants.lightColour.xyz;

    //vec3 specular = pow(max(dot(viewRay, reflectRay), 0.0), 64) * specularStrength * constants.lightColour.xyz;

    //vec3 result = (ambient +diffuse + specular) * fragColour;

    //vec3 baryCoord = gl_BaryCoordEXT;
    
    //float Edge = min(baryCoord.x, min(baryCoord.y, baryCoord.z));

    //float wireFrame = smoothstep(0, 0.1, Edge);

    //result *= wireFrame;

    //outColour = vec4(result, 0.0);

    outColour = vec4(normalize(fragNormal).xyz, 0.4);

    float hue = (pos.z) / (1e8f / -360.0f);
    outColour = vec4(HSVtoRGB(hue, 1, 1), 0.4f) * min(1, (9e16f) / pow(pos.z, 2.0f));


    if(normalize(fragNormal).z > 0.999999f){
        outColour = vec4(1, 1, 1, 1);
    }
    if(mod(pos.z, 2e6) < 2e5){
        outColour = vec4(0, 0, 0, 1.0);
    }
    
    

}