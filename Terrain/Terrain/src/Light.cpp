#include "..\include\Light.h"

#include "Camera.h"

void Light::setLookingAt(glm::vec3 pos)
{
	lookingAt = pos;
	makeLightView();
}

glm::mat4 Light::getOrthoProjection(float r)
{
	return glm::ortho(-r, r, -r, r, -r, r);
}

void Light::makeLightView()
{
	lightView = glm::lookAt(lookingAt, glm::normalize(lightDir) + lookingAt, UP);
}
