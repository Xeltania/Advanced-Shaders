

#version 330 core
// normal phong shader
float calcShadow(vec4 fragPosLightSpace);
out vec4 FragColor;

in vec2 TexCoordsTE;
in vec3 gNormals ;
in vec3 posES ;
in vec4 FragPosLightSpace ;

struct DirLight {
    vec3 direction;  // direction of light
    vec3 ambient;    // ambient , spsc, and diffuse values
    vec3 diffuse;
    vec3 specular;
}; 

uniform sampler2D texture1;  // texture for objects
uniform sampler2D shadowMap;   // shadow map texture
uniform DirLight dirLight;
uniform vec3 viewPos ;


void main()
{   
     float shine = 0.5f ;
	 vec3 norms = gNormals;
     vec3 viewDir = normalize(viewPos - posES);

	 vec3 ambient = dirLight.ambient * texture(texture1, TexCoordsTE).rgb;     
     vec3 lightDir = normalize(-dirLight.direction);
	 vec3 reflectDir = reflect(-dirLight.direction, norms);

    // diffuse shading
    float diff = max(dot(norms, dirLight.direction), 0.0);
	vec3 diffuse  = dirLight.diffuse  * diff * vec3(texture(texture1, TexCoordsTE));
    // specular shading
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shine);
    vec3 specular = dirLight.specular * spec * vec3(texture(texture1, TexCoordsTE));

	// so far, so familiar. Next line is new - call the shadowCalc() function
	// this returns a float for how much a fragement is in shadow
	float shadow = calcShadow(FragPosLightSpace); 
	//combine
	// 1-shadow - how much a fragement is NOT in shadow
    FragColor = vec4(ambient + (1.0-shadow)*(diffuse + specular),1.0f);
	//FragColor = vec4(vec3(shadow),1.0) ;
	FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}


float calcShadow(vec4 fragPosLightSpace)  //incomplete
{
    float shadow = 0.0 ; 
    // perform perspective divide values in range [-1,1]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // sample from shadow map  (returns a float; call it closestDepth)
	float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective ( call it current depth)
 float currentDepth = projcoords.z;
    // check whether current frag pos is in shadow
	if(currentDepth > closestDepth)
		shadow = 1;
	

    return shadow;
}
