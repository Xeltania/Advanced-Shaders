#include "CSM.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include "Shader.h"

CSM::CSM( unsigned int shadowWidth, unsigned int shadowHeight, unsigned int *depthMapArray, unsigned int *depthMapFBOArray, int cascades)
{
	// Assign member values
	n = cascades;
	shadowW = shadowWidth;
	shadowH = shadowHeight;
	//depthMap = depthMapArray;
	//depthMapFBO = depthMapFBOArray;
	// Set The Depth FBO
	setDepthFBO();

}

CSM::CSM(int w, int h, glm::vec3 direction, float fieldOfView, float ar, float nearPlane, float farPlane, int cascades)
{
	n = cascades;
	shadowW = w;
	shadowH = h;
	depthMap.resize(n);
	depthMapFBO.resize(n);
	FOV = fieldOfView;
	aspectR = ar;
	near = nearPlane;
	far = farPlane;
	//
	setDepthFBO();
	frustum.setLightDir(direction);
	frustum.setCascadeEnds(nearPlane, farPlane / 10, farPlane / 4, farPlane);
	frustum.setValues(FOV, ar, 0, 0);

}



void CSM::setDepthFBO()
{
	// Gen Frame Buffer
	glGenFramebuffers(n, depthMapFBO.data());
	// Create Depth Texture
	glGenTextures(n, depthMap.data());
	// Set up each frame buffer
	for (GLuint i = 0; i < n; i++)
	{
		glBindTexture(GL_TEXTURE_2D, depthMap[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowW, shadowH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		//
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		//
		float borderColour[] = { 1.0,1.0,1.0,1.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);
		// Attach depth texture as the FBO depth buffer
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap[i], 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

}

void CSM::firstPassFillShadowMaps(Terrain terrain, Shader shader, int VAO, glm::vec3 lightDir)
{
	frustum.setLightDir(lightDir);
	// Update view and projection at each iteration 
	std::vector<glm::mat4> views;
	views.resize(3);
	std::vector<glm::mat4>projections;
	projections.resize(3);
	//
	views = frustum.getViewMatrices();
	projections = frustum.getProjectionMatrices();

	shader.use();
	// LSM
	shader.setMat4("lightSpaceMatrix[0]", projections.at(0)*views.at(0));
	shader.setMat4("lightSpaceMatrix[1]", projections.at(1)*views.at(1));
	shader.setMat4("lightSpaceMatrix[2]", projections.at(2)*views.at(2));
	// Cascade Ends
	shader.setFloat("cascadeEnds[0]", transformCascadeEnds(frustum.cascadeEnds[0]));
	shader.setFloat("cascadeEnds[1]", transformCascadeEnds(frustum.cascadeEnds[1]));
	shader.setFloat("cascadeEnds[2]", transformCascadeEnds(frustum.cascadeEnds[2]));
	shader.setFloat("cascadeEnds[3]", transformCascadeEnds(frustum.cascadeEnds[3]));
	
	for (int i = 0; i < n; i++)
	{

		shader.setMat4("projection", projections.at(i));
		shader.setMat4("view", views.at(i));
		//
		glViewport(0, 0, shadowW, shadowH);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO[i]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(VAO);
		glDrawArrays(GL_PATCHES, 0, terrain.getSize());

	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void CSM::updateFrustra(glm::mat4 view)
{
	frustum.setcameraView(view);
	frustum.updateCascades(n);
}

float CSM::transformCascadeEnds(float end) // transform cascade ends to world space
{
	glm::vec4 tmp = glm::vec4(0, 0, end, 1.0);
	tmp = cameraProj * tmp;
	return tmp.z;
}
