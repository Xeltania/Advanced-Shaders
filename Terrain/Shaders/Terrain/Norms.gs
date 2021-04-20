#version 330 core
layout(triangles) in ;
layout(triangle_strip, max_vertices = 3) out ;
vec3 getNormal() ;

in vec3 posES[] ; 
in vec3 normES[] ;
in float vis[] ;

out vec3 gNormals ;
out vec3 gWorldPos_FS_in ;
out float fog ;

uniform mat4 view;
uniform mat4 projection;

out float clipSpaceZ;
void main()
{
  
  fog = vis[0];
  
   for(int i = 0 ; i < 3; i++)
   {
      gl_Position = projection * view * vec4(posES[i], 1.0) ;
	  clipSpaceZ = gl_Position.z;
      gWorldPos_FS_in = posES[i];
      gNormals = normES[i];    
      EmitVertex() ;
	  
  }
     EndPrimitive() ;

}


vec3 getNormal()
{
    vec3 a = vec3(gl_in[1].gl_Position) - vec3(gl_in[0].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[0].gl_Position);
    return normalize(cross(b, a));
}