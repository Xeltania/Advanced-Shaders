
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;  // this is different - we're passing this to calculate shadow in frag shader

out vec2 TexCoords;
out vec3 fragPosVS ;
out vec4 FragPosLightSpace;  // pass it on as a vec4


void main()
{
    TexCoords = aTexCoords;  
    fragPosVS = vec3(model * vec4(aPos, 1.0)).xyz;
	//gl_Position = projection * view *vec4(fragPosVS, 1.0);
	FragPosLightSpace = lightSpaceMatrix * vec4(fragPosVS, 1.0);  // point as ight sees it

}