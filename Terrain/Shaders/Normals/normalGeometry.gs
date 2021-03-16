#version 330 core

layout (triangles) in ;
layout (line_strip, max_vertices = 2) out;


uniform float length;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

// Pass through normal and frag pos
in vec3 normES[]; // Takes in the vertex normals
in vec3 posES[];

out vec3 gNormals ;
out vec3 gWorldPos_FS_in ;

vec3 getNormal();

void main()
{

	
// Iterate over triangles points
	for(int i =0; i < 3; i++)
	{
		gNormals = normES[i];
		gWorldPos_FS_in = posES[i];
		//
		gl_Position = projection * view * vec4(gWorldPos_FS_in, 1.0);
		EmitVertex();
		gl_Position = projection * view * (vec4(gWorldPos_FS_in, 1.0) + vec4(gNormals * length, 0.0)); 
		EmitVertex();
		EndPrimitive();
	}
}
vec3 getNormal()
{
    vec3 a = vec3(gl_in[1].gl_Position) - vec3(gl_in[0].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[0].gl_Position);
    return normalize(cross(b, a));
}