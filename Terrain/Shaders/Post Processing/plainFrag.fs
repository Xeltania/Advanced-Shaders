#version 330 core
out vec4 FragColor ;


in vec2 TexCoords;

// Uniform of the terrain scene to be passed to the colour buffer
uniform sampler2D scene;
// Unifrom near and far plane for depth buffer
uniform float nearPlane;
uniform float farPlane;

float linearizeDepth(float depth);

void main()
{
	 float depth = texture(scene, TexCoords).r;
   // FragColor = texture(scene, TexCoords) ;
    FragColor = vec4(vec3(linearizeDepth(depth) / farPlane), 1.0);
	FragColor = vec4(vec3(depth),1.0);
}
	
float linearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}