#version 410 core

out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

//Light
struct LightAttribs
{
	vec3 lightPos;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};  
uniform LightAttribs light;


// Interface block
in vec3 gWorldPos_FS_in; // Normal
in vec3 gNormals; // geo shader normals
in vec2 TexCoordsTE; // Texture coordinates
in vec3 posES; // Fragment position
in float fog ; // Fog Visibility

uniform vec3 camPos; // View Position
uniform float scale;
uniform vec3 sky ; // Clear colour of the window

vec3 colour;
vec3 tint;

// Uniform textures to use instead of tinting?
// uniform sampler2D grassTex;


void main()
{    


	float height = gWorldPos_FS_in.y / scale;
	vec4 green = vec4(0.3, 0.35, 0.15, 0.0);
	vec4 grey = vec4(0.5, 0.4, 0.5, 0.0);
	vec4 blue = vec4(0.0, 0.4, 1, 0.0);
	vec4 white = vec4(1,1,1,0);
	
	
	// Light Normal and Dir
	vec3 normal = gNormals;
	vec3 lightDir = normalize(light.lightPos - gWorldPos_FS_in);
	//Camera distance
	
	
	
	//Check height of the point to determine what to colour it:
	colour = vec3(0.6,0.3,0.25);
	
		if (height > 1.0)
		{
		colour = vec3(mix(grey, white, smoothstep(0.3,0.4,height)).rgb);
		}
		else if (height > 0.5)
		{
		colour = vec3(mix(blue, grey, smoothstep(0.2, 0.8,height)).rgb);
		}
		else if(height > 0) 
		{
		colour = vec3(mix(green, blue, smoothstep(0.5,0.4,height)).rgb); 
		}
	// 


	
	// Ambient
	vec3 ambient = colour * light.ambient; //* vec3(texture(texture_diffuse1, TexCoordsTE));
	
	// Diffuse
	float diff = max(0.0, dot(normal, lightDir));
	vec3 diffuse = (diff *colour) * light.diffuse; //* vec3(texture(texture_diffuse1, TexCoordsTE));
	
	// Light values for specular
	
	vec3 viewDir = normalize(camPos - gWorldPos_FS_in);
	vec3 reflectDir = reflect(-lightDir, normal);
	
	// Specular
	
	vec3 halfDir = normalize(lightDir + viewDir); // Blinn-Phong
	
	float spec = pow(max(0, dot(halfDir, viewDir)), 0.9);
	vec3 specular = (spec *colour)* light.specular;// * vec3(texture(texture_specular1, TexCoordsTE));
	
	// Regular FragCol
	vec3 blinnPhong = (ambient + diffuse + specular);

	FragColor = vec4( blinnPhong ,1.0);
	// Mix Fog visibility
	FragColor = mix(vec4(sky, 1.0), FragColor, fog);

}

