#version 330 core
out vec4 FragColor ;


in vec2 TexCoords;

// Uniform of the terrain scene to be passed to the colour buffer
uniform sampler2D scene;
// Unifrom near and far plane for depth buffer
uniform float nearPlane;
uniform float farPlane;

float linearizeDepth(float depth);


// Bools for post processing
uniform bool depth;
uniform bool hdr;
uniform bool inverted;
uniform bool pixelation;

uniform float exposure;
uniform float pixels;

void main()
{

	FragColor = texture(scene, TexCoords) ;
	if(pixelation)
	{
		float dx = 15.0 * (1/pixels);
		float dy = 10.0 * (1/pixels);
		vec2 newCoord = vec2(dx * floor(TexCoords.x / dx),
							dy * floor(TexCoords.y / dy));
		FragColor = texture(scene, newCoord);
	}
	if(inverted)
	{
		vec4 colour = vec4(vec3(1.0 - texture(scene, TexCoords)), 1.0);
		FragColor = colour;
	}
	if(hdr)
	{
		const float gamma = 2.5;
		vec3 hdrColor = texture(scene, TexCoords).rgb;
		hdrColor = texture(scene, TexCoords).rgb;
  
		// reinhard tone mapping
		vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
		// gamma correction 
		mapped = pow(mapped, vec3(1.0 / gamma));
  
		FragColor = vec4(mapped, 1.0);
	}
	if(depth)
	{
		float depth = texture(scene, TexCoords).r;
		FragColor = vec4(vec3(linearizeDepth(depth) / farPlane), 1.0);
		FragColor = vec4(vec3(depth),1.0);
	}
}
	
float linearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}