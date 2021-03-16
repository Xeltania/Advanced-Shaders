#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormals;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos ; 
out vec3 normals ;

void main()
{
        normals = aNormals ; 
		fragPos = (model * vec4(aPos, 1.0)).xyz; 
		gl_Position = model *vec4(aPos, 1.0); 
}

