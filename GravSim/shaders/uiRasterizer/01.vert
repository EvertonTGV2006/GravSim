#version 460

const uint CHAR_COUNT = 128 - 32;
const uint CHAR_START = 32;
const uint STRING_LENGTH = 4096;

layout(binding = 0) uniform UniformBufferObject{
    uvec4[STRING_LENGTH / 16] stringContents;
} ubo;

layout(push_constant) uniform pc {
    vec2 charDimensions;
    vec2 screenPosition;
    vec2 texDimensions;
    float texAdvance;
    uint renderStage;  
    int instanceOffset;     
};


layout(location = 0) in vec2 inPosition;


layout(location = 0) out vec2 fragTexCoord;

int characters[11] = int[](72, 101, 108, 108, 111, 32, 87, 111, 114, 108, 100);

void main() {
    uint i = gl_InstanceIndex;

    if(renderStage == 0){
        //draw blank boxes if renderstage is 0
        vec2 inPosition2 = vec2(0, 0);
        inPosition2.x = (inPosition.x > 0) ? 1 : 0; //normalize coords to 1
        inPosition2.y = (inPosition.y > 0) ? 1 : 0;
        fragTexCoord = vec2(0, 0);
        gl_Position = vec4(screenPosition.x + inPosition2.x * texDimensions.x, screenPosition.y + inPosition2.y * texDimensions.y, 0.0, 1.0);
    }

    if(renderStage == 1){
        //draw character boxes if renderStage is 1

        //Unpack data from UBO int array to character;

        uint uboVecIndex = i >> 4;
        uint uboValIndex = (i >> 2) & 3;
        uint uboShiftIndex = i & 3;
        uint uboShiftValue = 8 * uboShiftIndex;
        uint uboMask = 127;
        
        uvec4 uboVec = ubo.stringContents[uboVecIndex];
        uint uboVal = uboVec[uboValIndex];
        uint char = (uboVal >> uboShiftValue) & uboMask;

        

        char = char - CHAR_START;

        //char = characters[i] - CHAR_START;

        fragTexCoord = vec2(inPosition.x + char * charDimensions.x, inPosition.y);

        //float newChar = float(char) / 50;
        
        //fragTexCoord = vec2(newChar, newChar);
        //work out texture position for the current char being rendered;

        //now work out out position based on instance ID and instanceOffset and char Advance
        uint j = i - instanceOffset;
        vec2 stringPosition = vec2(texDimensions.x * (texAdvance * j + inPosition.x),texDimensions.y * inPosition.y);
        gl_Position = vec4(screenPosition.x + stringPosition.x, screenPosition.y + stringPosition.y, 0.0, 1.0);
    }
    if(renderStage == 2){
        vec2 positions[3] = vec2[](
            vec2(0.0, -0.5),
            vec2(0.5, 0.5),
            vec2(-0.5, 0.5)
        );
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    }
    if (renderStage==3){
        i = i + instanceOffset;
        uint uboVecIndex = i >> 4;
        uint uboValIndex = (i >> 2) & 3;
        uint uboShiftIndex = i & 3;
        uint uboShiftValue = 8 * uboShiftIndex;
        uint uboMask = 127;
        
        uvec4 uboVec = ubo.stringContents[uboVecIndex];
        uint uboVal = uboVec[uboValIndex];
        uint char = (uboVal >> uboShiftValue) & uboMask;

        

        char = char - CHAR_START;

        //char = characters[i] - CHAR_START;

        vec2 boxPosition = vec2(inPosition.x * charDimensions.x + gl_InstanceIndex * texAdvance + screenPosition.x, inPosition.y * charDimensions.y + screenPosition.y);
        //vec2 texPosition = vec2(float(char & 127) / 128.0f, inPosition.y * texDimensions.y);
        vec2 texPosition = vec2(inPosition.x * texDimensions.x + float(char) * texDimensions.x, inPosition.y * texDimensions.y);
        vec2 transofmedPosition = (boxPosition - vec2(0.5f,0.5f) ) * 2;
        gl_Position = vec4(transofmedPosition.xy, 0.0, 1.0);
        fragTexCoord = texPosition;
    }




    
}



