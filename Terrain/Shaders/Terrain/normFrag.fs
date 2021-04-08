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
//in vec4 FragPosLightSpace ;
//
in float clipSpaceZ;
uniform vec3 camPos; // View Position
uniform float scale;
uniform vec3 sky ; // Clear colour of the window
//
uniform bool fogEnabled;
uniform bool shadows;
uniform float bias; // Uniform used to set the bias value of the shadows.
uniform int cascades; // Number of cascades for CSM.

vec3 colour;
vec3 tint;

// Uniform textures to use instead of tinting?
// uniform sampler2D grassTex;

// Calculating shadows:
float calcShadow(vec4 fragPosLightSpace, float bias);
uniform sampler2D shadowMap[cascades];   // shadow map texture
uniform mat4 lightSpaceMatrix[cascades];  // this is different - we're passing this to calculate shadow in frag shader
uniform const cascadeEnds[cascades+1];

//CSM
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;

int findAppropriateMap(float z, float ends[4]);

void main()
{    
	int map = findAppropriateMap(clipSpaceZ, cascadeEnds);
	
	vec4 FragPosLightSpace = lightSpaceMatrix[map] * vec4(gWorldPos_FS_in, 1.0);  // point as ight sees it

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
		colour = vec3(mix(white, grey, smoothstep(1.65,0.9,height)).rgb);
		}
		else if (height > 0.7)
		{
		colour = vec3(mix(grey, green, smoothstep(0.8, 0.75,height)).rgb);
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


	if(shadows)
	{

	float shadow = calcShadow(FragPosLightSpace, bias); 
	//combine
	// 1-shadow - how much a fragement is NOT in shadow
 //   FragColor = vec4(ambient + (1.0-shadow)*(diffuse + specular),1.0f);
	blinnPhong = ambient + (1.0-shadow)*(diffuse + specular);
	//FragColor = vec4(vec3(shadow),1.0) ;
	}
	
	FragColor = vec4( blinnPhong ,1.0);
	
	if(fogEnabled)
		// Mix Fog visibility
		FragColor = mix(vec4(sky, 1.0), FragColor, fog);

}


float calcShadow(vec4 fragPosLightSpace, float bias)  //incomplete
{
    float shadow = 0.0 ; 
    // perform perspective divide values in range [-1,1]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // sample from shadow map  (returns a float; call it closestDepth)
	float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective ( call it current depth)
	float currentDepth = projCoords.z;
	// Check if frag pos is within the shadow 
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
		// Bias to remove artifacted / blocky shadows
	for ( int i = 0 ; i < 2 ; i ++)
	{
		for ( int j = 0 ; j < 2 ; j ++)
		{
			float pcf = texture(shadowMap, projCoords.xy + vec2(i, j) * texelSize).r;
			if(currentDepth - bias > pcf)
				shadow += 1 ;
		}
		
	}
	shadow /= 9; // 3x3 kernel
	if(projCoords.z > 1.0)
		shadow = 0.0;
	// check whether current frag pos is in shadow
	//if(currentDepth > closestDepth)
	//	shadow = 1;
	
    return shadow;
}

int findAppropriateMap(float z, float ends[4])
{
	int map = 2; // Begin ith the largest map by default.
	for(int i = 0; i < cascades; i ++)
	{
		if (z <= ends[i+1])
		{
			map = i;
			break;
		}
	}
	return map;
}
