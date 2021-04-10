#pragma once
#include "Terrain.h"
#include "Light.h"
#include "Frustum.h"
#include "Shader.h"

// Cascading Shadow Map class

class CSM
{
public :
	CSM() {};
	CSM(unsigned int shadowWidth, unsigned int shadowHeight, unsigned int *depthMapArray, unsigned int *depthMapFBOArray, int cascades);
	CSM(int w, int h, unsigned int *depthMapArray, unsigned int *depthMapArrayFBO, glm::vec3 direction, float fieldOfView, float ar, float nearPlane, float farPlane);
	void setDepthFBO();
	void firstPassFillShadowMaps(Terrain terrain, Shader shader, int VAO);
	void updateFrustra(glm::mat4 view);
	float transformCascadeEnds(float end);

private :
	int n; // Number of cascades (default 3)
	int shadowW, shadowH;
	unsigned int* depthMap;
	unsigned int *depthMapFBO;
	//
	Light light;
	Frustum frustum;
	//
	float FOV, aspectR, near, far;
	glm::mat4 cameraProj;

};