#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Shader.h"
#include "Camera.h"

#include <Model.h>
#include "Terrain.h"

#include<string>
#include <iostream>
#include <numeric>
// Shadows
#include "CSM.h"
#include "Render.h"


// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;
// Large shadow texture : cascaded shadow maps can fix this ( the smaller this is the lower res the shadows become, leaving artifacts )
const unsigned int SHADOW_W = 5120;
const unsigned int SHADOW_H = 5120;
// Directional Light 
//glm::vec3 dirLightPos(0.1f, 1.0f, 0.2);
glm::vec3 dirLightPos(1.2f, 1.5f, 1.0f);
glm::mat4 lightProjection, lightView; // View-Projection matrices for shadow mapping
glm::mat4 lightSpaceMatrix; // Light Space Matrix : contains lightPos, targetPos, and 'up' vector.
// Buffer textures
unsigned int textureColourBuffer;
unsigned int textureDepthBuffer;
//arrays
unsigned int terrainVAO, terrainVBO;
unsigned int quadVAO, quadVBO;
unsigned int planeVAO, planeVBO;
unsigned int depthFBO, colourFBO;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(char const * path);
void updateTerrain();
//unsigned int loadTexture2(char const * path);
//
void setVAO(vector <float> vertices);
void setFBOColour();
void setFBODepth();
//
void renderQuad();
void renderScene(const Shader &shader);
void renderTrees(Shader modelShader, Model m);
//
// camera
Camera camera(glm::vec3(260, 100, 300));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Booleans to toggle features [ use key presses to swap these ]
	// Debugging Shader Toggles //
bool showNormals = false; // Bind to the 1 key
bool shadeNormals = false; // Bind to Tab key
	// 
bool useFog = false;
	// Colour / Depth Buffer
bool toggleDepth = false;
bool toggleHDR = false;
bool toggleShadowMapping = false;
bool toggleInvert = false;
bool pixelation = false;

bool moveTerrain = false;
//
glm::mat4 model = glm::mat4(1.0f);

//
float exposure = 1.0f; // Passed into HDR to change exposure level in runtime
float pixels = 512.0f;
//

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "IMAT3907", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	//
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Shaders
		// Main Terrain Shader
	Shader terrainShader("..\\shaders\\Terrain\\plainVert.vs", "..\\shaders\\Terrain\\normFrag.fs", "..\\shaders\\Terrain\\Norms.gs", "..\\shaders\\Terrain\\tessControlShader.tcs", "..\\shaders\\Terrain\\tessEvaluationShader.tes");
	// Normal Debugging Shader
	Shader drawNormal("..\\shaders\\Normals\\plainVert.vs", "..\\shaders\\Normals\\plainFrag.fs", "..\\shaders\\Normals\\normalGeometry.gs", "..\\shaders\\Normals\\tessControlShader.tcs", "..\\shaders\\Normals\\tessEvaluationShader.tes");
	// Post Processing
	Shader postProcessor("..\\shaders\\Post Processing\\plainVert.vs", "..\\shaders\\Post Processing\\plainFrag.fs", 0, 0, 0);
	//

	//Load HeightMap Texture
	//unsigned int heightMap = 0;// loadTexture("..\\resources\\heightMap.jpg");
	//Bind

	int gridSize = 10;

	//Terrain Constructor ; number of grids in width, number of grids in height, gridSize
	Terrain terrain(50, 50, gridSize);
	terrainVAO = terrain.getVAO();

	// Clear Colour
	float clearR = 0.5f;
	float clearG = 0.60f;
	float clearB = 0.80f;

	// Lighting 
	GLint lightPos, ambient, diffuse, specular;

	// Frame Buffers

	//setFBODepth();
	setFBOColour();

	//
	float timestep=0;

	dirLightPos = glm::vec3(100.f, 250.f, 100.f);
	// 100 10 300
	float shadowBias = 0.001f;

	// CSM 
	const int numCascades = 3;
	float nearPlane = 0.1f, farPlane = 1000.f;
	terrainShader.setInt("cascades", numCascades);
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, nearPlane, farPlane);
	CSM csm(SHADOW_W, SHADOW_H, -dirLightPos, glm::radians(camera.Zoom),SCR_WIDTH/SCR_HEIGHT, nearPlane, farPlane, numCascades, projection);
	Render renderer;

	terrainShader.use();
	terrainShader.setInt("shadowMap[0]", csm.getDepthMap()[0]);
	terrainShader.setInt("shadowMap[1]", csm.getDepthMap()[1]);
	terrainShader.setInt("shadowMap[2]", csm.getDepthMap()[2]);

	while (!glfwWindowShouldClose(window))
	{

		updateTerrain();

		// set the textures for the CSM uniforms
		renderer.setTextures(terrainShader);
		//
		/*
		timestep = (timestep / 2);
		if ( timestep > 2) {
			clearR -= 0.01f;
			clearG -= 0.01f;
			clearB -= 0.01f;
		}
		else
		{
			clearR += 0.01f;
			clearG += 0.01f;
			clearB += 0.01f;

			
		}*/

		timestep = glfwGetTime();

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		// global view settings
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(clearR, clearG, clearB, 1.0f);
		//
	
		glm::mat4 view = camera.GetViewMatrix();
		csm.updateFrustra(view);
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, nearPlane, farPlane);
		csm.updateProjection(projection);

		glm::vec3 targetPos(260, 100, 900);

		// Fog
		terrainShader.setVec3("sky", glm::vec3(clearR, clearG, clearB));
		//
					// Lighting
		//dirLightPos.z += sin(glfwGetTime()) * 10;
		dirLightPos.y += sin(glfwGetTime()) * 5;

		// Setup shader defaults
		postProcessor.use();
		postProcessor.setBool("depth", toggleDepth);
		postProcessor.setBool("hdr", toggleHDR);
		postProcessor.setBool("inverted", toggleInvert);
		postProcessor.setBool("pixelation", pixelation);
		postProcessor.setFloat("exposure", exposure);
		postProcessor.setFloat("pixels", pixels);
		//
		terrainShader.use();
		terrainShader.setFloat("bias", shadowBias);
		terrainShader.setMat4("model", model);
		terrainShader.setVec3("camPos", camera.Position);
		terrainShader.setInt("heightMap", 0);
		terrainShader.setFloat("scale", 90);
		terrainShader.setInt("octaves", 50);
		terrainShader.setInt("gridSize", gridSize);
		terrainShader.setBool("fogEnabled", useFog);
		terrainShader.setBool("shadows", toggleShadowMapping);
		lightPos = glGetUniformLocation(terrainShader.ID, "light.lightPos");
		ambient = glGetUniformLocation(terrainShader.ID, "light.ambient");
		diffuse = glGetUniformLocation(terrainShader.ID, "light.diffuse");
		specular = glGetUniformLocation(terrainShader.ID, "light.specular");
		glUniform3f(lightPos, dirLightPos.x, dirLightPos.y, dirLightPos.z);
		glUniform3f(ambient, 0.2f, 0.2f, 0.2f);
		glUniform3f(diffuse, 0.55f, 0.55f, 0.55f);
		glUniform3f(specular, 0.3f, 0.3f, 0.3f);

		if (toggleShadowMapping)
		{
			// CSM
			// Created FBO attachments, now take the first pass of the scene. Bind each framebuffer adn fill with depth values
			csm.firstPassFillShadowMaps(terrain, terrainShader, terrainVAO, -dirLightPos);

			//now render to screen
			glBindFramebuffer(GL_FRAMEBUFFER, colourFBO);
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);  // viewport for whole screen
			terrainShader.use();
			terrainShader.setMat4("projection", projection);
			terrainShader.setMat4("view", view);
			//

			glEnable(GL_DEPTH_TEST);
			glClearColor(clearR, clearG, clearB, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			// Set textures to active and bind them 
			glActiveTexture(GL_TEXTURE1 );
			glBindTexture(GL_TEXTURE_2D, csm.getDepthMap()[0]);  // attach texture as shadow map
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, csm.getDepthMap()[1]);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, csm.getDepthMap()[2]);
			//
			glDrawArrays(GL_PATCHES, 0, terrain.getSize());

			//

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST);
			postProcessor.use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureColourBuffer);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			renderQuad();
		}
		else 
		{
			
			// Terrain Drawing
			terrainShader.use();
			terrainShader.setMat4("projection", projection);
			terrainShader.setMat4("view", view);
			glEnable(GL_DEPTH_TEST);
			//Main Viewport
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
			glBindVertexArray(terrainVAO);

			if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawArrays(GL_PATCHES, 0, terrain.getSize());

			//



		}
		// testing different parts of terrain Z -> all in close / far shadow cascade :( 
		
		if (toggleDepth)
		{
			terrainShader.setMat4("projection", projection);
			terrainShader.setMat4("view", view);
			csm.firstPassFillShadowMaps(terrain, terrainShader, terrainVAO, -dirLightPos);

			//
			for (int i = 0;i <3; i ++)
			{			
			int offset = i * 250;
			glViewport(0 + offset, 600, 250, 250);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST); // Disable depth test : only rendering a 2D image.
			postProcessor.use(); // Use the post-processing shader (all post processing effects will go in here)
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, csm.getDepthMap()[i]); // Bind Colour or Depth buffer
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			renderQuad(); // render our quad.
			//
			}
		}
		// Normals Shader
		drawNormal.use();
		drawNormal.setMat4("projection", projection);
		drawNormal.setMat4("view", view);
		drawNormal.setMat4("model", model);
		drawNormal.setVec3("camPos", camera.Position);
		drawNormal.setInt("heightMap", 0);
		drawNormal.setFloat("scale", 90);
		drawNormal.setInt("octaves", 50);
		//
		if (showNormals)
		{
			// Draw Normal
			drawNormal.setFloat("length", 1.f);
			drawNormal.setInt("gridSize", gridSize);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_PATCHES, 0, terrain.getSize());
		}
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}


	glfwTerminate();
	system("pause");
	return 0;
}

void updateTerrain()
{

	model = glm::translate(glm::mat4(1.f), glm::vec3((camera.Position.x - 250.f), 0.0f, (camera.Position.z - 250.f)));

	
	moveTerrain = true;
}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	// Visual Keys
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		showNormals = !showNormals; // Swap to normal debugging shader visible
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		useFog = !useFog; // Toggle Fog
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) 
		toggleShadowMapping = !toggleShadowMapping; // Toggle BUffers
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		toggleDepth = !toggleDepth; // Toggle Depth Buffer
	if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		toggleHDR = !toggleHDR; // Toggle BUffers
	if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
		toggleInvert = !toggleInvert; // Toggle BUffers
	if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
		pixelation = !pixelation; // Toggle BUffers
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		exposure += 0.25f; // Toggle BUffers
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		exposure -= 0.25f; // Toggle BUffers
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		pixels -= 125.f; // Toggle BUffers
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		pixels += 125.f; // Toggle BUffers
	


}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_image_free(data);
		std::cout << "Loaded texture at path: " << path << " width " << width << " id " << textureID << std::endl;

	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);

	}

	return textureID;
}

// SET VAO

void setVAO(vector<float> vertices)
{
	glGenVertexArrays(1, &terrainVAO);
	glGenBuffers(1, &terrainVBO);
	glBindVertexArray(terrainVAO);
	glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
	glBufferData(GL_ARRAY_BUFFER, (vertices.size() * sizeof(GLfloat)), vertices.data(), GL_STATIC_DRAW);

	// Pos XYZ
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Texture
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// Normals
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


}

//FBO

void setFBOColour()
{

	// Create & bind FBO
	glGenFramebuffers(1, &colourFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, colourFBO);
	// Create colour attachment texture
	glGenTextures(1, &textureColourBuffer);
	glBindTexture(GL_TEXTURE_2D, textureColourBuffer);
	// Parameters for sampling ( set to null because we aren't using an image - screen space which we fill with our scene )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); // Updated to use HDR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Bind colour buffer to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, textureColourBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColourBuffer, 0);

	// Generate an RBO to perform depth testing
	unsigned int RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

}

void setFBODepth()
{
	// Create and bind FBO
	glGenFramebuffers(1, &depthFBO);
	// Create depth texture
	glGenTextures(1, &textureDepthBuffer);
	glBindTexture(GL_TEXTURE_2D, textureDepthBuffer);
	// Parameters for sampling ( null texture )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_W, SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//
	float borderColour[] = { 1.0f,1.0f,1.0f,1.0f }; // Border colour
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);
	// Attach depth texture as FBO depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureDepthBuffer, 0);
	// GL_NONE : Not attaching a colour buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


}


// RENDERING
void renderQuad()
{
	if (quadVAO == 0)
	{

		float quadVerts[] =
		{
			// Pos x, y, z		Tex coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f
		};

		// Set plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), &quadVerts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}




void renderScene(const Shader &shader)
{

}


void renderTrees(Shader modelShader, Model m)
{


}

