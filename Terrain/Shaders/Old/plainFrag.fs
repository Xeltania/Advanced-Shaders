#version 330 core
out vec4 FragColor ;


in vec2 gNormals ;
in vec3 FragPos ;
in vec3 gWorldPos_FS_in ;

void main()
{
    FragColor = vec4(1.0,0.0, 0.0,1.0) ;
}
	