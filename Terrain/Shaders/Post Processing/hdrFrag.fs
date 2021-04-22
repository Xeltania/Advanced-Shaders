#version 330 core
out vec4 colour;
  
in vec2 TexCoords;

uniform sampler2D u_colourTexture;

void main()
{             
    //vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    //FragColor = vec4(hdrColor, 1.0);
	colour = vec4(vec3(1.0 - texture(u_colourTexture, TexCoords)), 1.0);
}  