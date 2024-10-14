#version 460

//#extension GL_EXT_fragment_shader_barycentric : enable

layout(binding = 1) uniform sampler2D texSampler;




layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 pos;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColour;

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

    outColour = vec4(fragColour, 0.0);

}