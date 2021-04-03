#version 330 core
//only care about xyz dont need uv, normals etc 
layout (location = 0) in vec3 aPos;

// pass in matrix to transform point to light space
// remember we are looking at the scene from the lights point of view

uniform mat4 lightSpaceMatrix;  


void main()
{
    // like regular transform but projection is orthographic and view is simply the lights poistion looking at the origin
    gl_Position = lightSpaceMatrix * vec4(aPos, 1.0);
}