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
	CSM(int w, int h, glm::vec3 direction, float fieldOfView, float ar, float nearPlane, float farPlane, int cascades, glm::mat4 proj);
	void setDepthFBO();
	std::vector<unsigned int> getDepthMap() { return depthMap; }
	std::vector<unsigned int> getDepthFBO() { return depthMapFBO; }
	void firstPassFillShadowMaps(Terrain terrain, Shader shader, int VAO, glm::vec3 lightDir);
	void updateFrustra(glm::mat4 view);
	float transformCascadeEnds(float end);
	void updateProjection(glm::mat4 proj) { cameraProj = proj; }

private :
	int n; // Number of cascades (default 3)
	int shadowW, shadowH;
	std::vector<unsigned int> depthMap;
	std::vector<unsigned int> depthMapFBO;
	//
	Frustum frustum;
	//
	float FOV, aspectR, near, far;
	glm::mat4 cameraProj;
	glm::vec3 lightDir;

};