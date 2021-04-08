#pragma once

#include "Light.h"
#include "Orthographic.h"
#include <vector>

class Frustum
{
public:
	Frustum() {}
	// Setters
	void setValues(float FOV, float ar, float near, float far);
	void setLight(Light l) { light = l; }
	void setcameraView(glm::mat4 view) { cameraView = view; }
	void setCascadeEnds(float a, float b, float c, float d);
	void transformToWorldSpace();
	// Getters
	std::vector<glm::mat4> getProjectionMatrices() { return lightProjection; }
	std::vector<glm::mat4> getViewMatrices() {return lightViews;}
	//
	void updateCascades(int n);
	void findCorners(float near, float far);
	void findCentre();
	Orthographic findMinMax();
	//
	float cascadeEnds[4]; // Cascade ends ( limited to 3 cascades )

private:
	//
	Light light;
	glm::mat4 cameraView;
	glm::vec3 cascadeCentre;
	std::vector<glm::mat4> lightViews;
	std::vector<glm::mat4> lightProjection;
	std::vector<glm::vec4> corners;
	//
	float fov, nearPlane, farPlane, aspectR;
	float tanHalfHFOV, tanHalfVFOV;
	
};