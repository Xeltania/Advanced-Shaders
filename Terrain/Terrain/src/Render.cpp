#include "Render.h"

void Render::setTextures(Shader & shader)
{
	shader.use();
	shader.setInt("heightMap", 0);

	shader.setInt("shadowMap[0]", 1);
	shader.setInt("shadowMap[1]", 2);
	shader.setInt("shadowMap[2]", 3);

}