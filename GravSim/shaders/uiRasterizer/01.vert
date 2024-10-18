#version 460

const uint CHAR_COUNT = 126 - 34;
const uint CHAR_START = 34;

layout(binding = 0) uniform UniformBufferObject{
    int[CHAR_COUNT] stringContents;
} ubo;

layout(push_constant) uniform pc {
    ivec2 charDimensions;
    vec2 screenPosition;
    vec2 screenDimensions;
    uint  charAdvance;
    uint renderStage;       

};


layout(location = 0) in ivec2 inPosition;


layout(location = 0) out vec2 fragTexCoord;


void main() {
    int i = gl_InstanceIndex;

    if(renderStage == 0){
        //draw blank boxes if renderstage is 0
        vec2 inPosition2 = vec2(0, 0);
        inPosition2.x = (inPosition.x > 0) ? 1 : 0; //normalize coords to 1
        inPosition2.y = (inPosition.y > 0) ? 1 : 0;
        fragTexCoord = vec2(0, 0);
        gl_Position = vec4(screenPosition.x + inPosition2.x * screenDimensions.x, screenPosition.y + inPosition2.y * screenDimensions.y, 0.0, 1.0);
    }

    if(renderStage == 1){
        //draw character boxes if renderStage is 1

        //Unpack data from UBO int array to character;
        uint uboIndex = uint(floor(float(i) / 4));
        uint uboShift = 8 * (i % 4);
        uint uboMask = 31;

        uint char = (ubo.stringContents[uboIndex] >> uboShift) & uboMask;
        char = char - CHAR_START;

        fragTexCoord = vec2(inPosition.x + char * charDimensions.x, inPosition.y);
        //work out texture position for the current char being rendered;

        //now work out out position based on instance ID and char Advance
        vec2 stringPosition = vec2(screenDimensions.x * (charAdvance * i + inPosition.x), screenDimensions.y * inPosition.y);
        gl_Position = vec4(screenPosition.x + stringPosition.x, screenPosition.y + stringPosition.y, 0.0, 1.0);
    }




    
}



