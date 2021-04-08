#pragma once
#include <glm/glm.hpp>

class Light
{
public:
	Light() {}
	// Setters
	void setLookingAt(glm::vec3 pos);
	void setLightDir(glm::vec3 dir) { lightDir = dir; }
	// Getter
	glm::mat4 getLightView() { return lightView; }
	glm::mat4 getOrthoProjection(float r);
	//
	void makeLightView();

private:

	glm::mat4 lightView;
	glm::vec3 lightDir;
	glm::vec3 lookingAt;

};