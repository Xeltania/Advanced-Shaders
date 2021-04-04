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



// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;
// Large shadow texture : cascaded shadow maps can fix this ( the smaller this is the lower res the shadows become, leaving artifacts )
const unsigned int SHADOW_W = 5120;
const unsigned int SHADOW_H = 5120;
// Directional Light 
//glm::vec3 dirLightPos(0.1f, 1.0f, 0.2);
glm::vec3 dirLightPos(1000.f, 1000.f, 1100.f);
glm::mat4 lightProjection, lightView; // View-Projection matrices for shadow mapping
glm::mat4 lightSpaceMatrix; // Light Space Matrix : contains lightPos, targetPos, and 'up' vector.
// Buffer textures
unsigned int textureColourBuffer;
unsigned int textureDepthBuffer;
//arrays
unsigned int terrainVAO, terrainVBO;
unsigned int quadVAO, quadVBO;
unsigned int planeVAO, planeVBO;
unsigned int FBO;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(char const * path);
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
bool usePerlin = false; // Bind to 2 key : need to toggle between how cdm is calculated in TES
bool useCDM = false; // Swap between the getNormal() and CDM calculated normals
bool useFog = false;
	// Colour / Depth Buffer
bool toggleBuffers = false;
bool toggleShadowMapping = false;



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
	Shader depthShader("..\\shaders\\Post Processing\\depthVert.vs", "..\\shaders\\Post Processing\\depthFrag.fs", 0, 0, 0);
	//Shader shadowShader("..\\shaders\\Post Processing\\shadowVert.vs", "..\\shaders\\Terrain\\normFrag.fs", "..\\shaders\\Terrain\\Norms.gs", "..\\shaders\\Terrain\\tessControlShader.tcs", "..\\shaders\\Terrain\\tessEvaluationShader.tes"); // seperate shader for shadow mapping?
	//
		// texture for floor and cubes
	unsigned int metal = loadTexture("..\\Resources\\metal.jpg");

	//Load HeightMap Texture
	unsigned int heightMap = 2;// loadTexture("..\\resources\\heightMap.jpg");
	//Bind

	int gridSize = 10;

	//Terrain Constructor ; number of grids in width, number of grids in height, gridSize
	Terrain terrain(50, 50, gridSize);
	terrainVAO = terrain.getVAO();

	// Clear Colour
	const float clearR = 0.40f;
	const float clearG = 0.40f;
	const float clearB = 0.80f;

	// Lighting 
	GLint lightPos, ambient, diffuse, specular;

	// Frame Buffers
	//setFBOColour();
	setFBODepth();
	//
	terrainShader.use();
	terrainShader.setInt("texture1", 1);
	terrainShader.setInt("shadowMap", 0);
	while (!glfwWindowShouldClose(window))
	{

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		// global view settings
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(clearR, clearG, clearB, 1.0f);
		//
		float nearPlane = 1.f, farPlane = 2000.f;
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, nearPlane, farPlane);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		// Light Space Matrix 
		lightProjection = glm::ortho(-350.0f, 350.0f, -350.0f, 350.0f, nearPlane, farPlane);
		glm::vec3 wLightPos(800, 600, 900);
		glm::vec3 targetPos(400, 50, 400);
		lightView = glm::lookAt(dirLightPos, glm::vec3(0), glm::vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;
		//
					// Lighting
		dirLightPos.z -= sin(glfwGetTime()) * 20;
		dirLightPos.y += sin(glfwGetTime()) * 5;

		if (toggleShadowMapping)
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, heightMap);			
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, metal);
			// render scene from light's point of view
			depthShader.use();
			depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
			// change viewport
			glViewport(0, 0, SHADOW_W, SHADOW_H);
			glBindFramebuffer(GL_FRAMEBUFFER, FBO);  // bind FBO
			glClear(GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureDepthBuffer);
			depthShader.use();
			renderScene(depthShader);  // render using depthShader - renders to FBO not screen


			//now render to screen
			// bind default framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);  // viewport for whole screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			terrainShader.use();
			terrainShader.setMat4("projection", projection);
			terrainShader.setMat4("view", view);
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
			//glUniform3f(lightPos, camera.Position.x, camera.Position.y + 20.0, camera.Position.z - 2.0);
			glUniform3f(ambient, 0.2f, 0.2f, 0.2f);
			glUniform3f(diffuse, 0.55f, 0.55f, 0.55f);
			glUniform3f(specular, 0.3f, 0.3f, 0.3f);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, metal);  // attach texture for objects
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureDepthBuffer);  // attach texture as shadow map
			glDrawArrays(GL_PATCHES, 0, terrain.getSize());
		}
		else {
			// Terrain Drawing
			terrainShader.use();
			terrainShader.setMat4("projection", projection);
			terrainShader.setMat4("view", view);
			terrainShader.setMat4("model", model);
			terrainShader.setVec3("camPos", camera.Position);
			terrainShader.setInt("heightMap", 0);
			terrainShader.setFloat("scale", 90);
			terrainShader.setInt("octaves", 50);
			terrainShader.setInt("gridSize", gridSize);
			terrainShader.setBool("fogEnabled", useFog);

			lightPos = glGetUniformLocation(terrainShader.ID, "light.lightPos");
			ambient = glGetUniformLocation(terrainShader.ID, "light.ambient");
			diffuse = glGetUniformLocation(terrainShader.ID, "light.diffuse");
			specular = glGetUniformLocation(terrainShader.ID, "light.specular");
			glUniform3f(lightPos, dirLightPos.x, dirLightPos.y, dirLightPos.z);
			//glUniform3f(lightPos, camera.Position.x, camera.Position.y + 20.0, camera.Position.z - 2.0);
			glUniform3f(ambient, 0.2f, 0.2f, 0.2f);
			glUniform3f(diffuse, 0.55f, 0.55f, 0.55f);
			glUniform3f(specular, 0.3f, 0.3f, 0.3f);

			// Fog
			terrainShader.setVec3("sky", glm::vec3(clearR, clearG, clearB));
			//terrainShader.setFloat("fogGradient", 0.5f);
			//


			//Main Viewport
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

			glBindVertexArray(terrainVAO);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, heightMap);
			// Colour the terrain in wireframe or fill depending on key press event
			if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			//

			glDrawArrays(GL_PATCHES, 0, terrain.getSize());

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
		
		if (toggleBuffers)
		{
			// First pass frame buffer
			glBindFramebuffer(GL_FRAMEBUFFER, FBO);
			terrainShader.use();
			terrainShader.setMat4("projection", lightProjection);
			terrainShader.setMat4("view", lightView);
			terrainShader.setMat4("model", model);
			terrainShader.setVec3("camPos", camera.Position);
			terrainShader.setInt("heightMap", 0);
			terrainShader.setFloat("scale", 90);
			terrainShader.setInt("octaves", 50);
			terrainShader.setInt("gridSize", gridSize);
			terrainShader.setBool("fogEnabled", useFog);
			lightPos = glGetUniformLocation(terrainShader.ID, "light.lightPos");
			ambient = glGetUniformLocation(terrainShader.ID, "light.ambient");
			diffuse = glGetUniformLocation(terrainShader.ID, "light.diffuse");
			specular = glGetUniformLocation(terrainShader.ID, "light.specular");
			glUniform3f(lightPos, dirLightPos.x, dirLightPos.y, dirLightPos.z);
			//glUniform3f(lightPos, camera.Position.x, camera.Position.y + 20.0, camera.Position.z - 2.0);
			glUniform3f(ambient, 0.2f, 0.2f, 0.2f);
			glUniform3f(diffuse, 0.55f, 0.55f, 0.55f);
			glUniform3f(specular, 0.3f, 0.3f, 0.3f);
			glEnable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBindVertexArray(terrainVAO);
			glDrawArrays(GL_PATCHES, 0, terrain.getSize());

			// renderScene(terrainShader, modelShader, tree); // Use this to render a scene with trees in it.

			// Second pass to render to screen
		//	glViewport(900, 500, SCR_WIDTH, SCR_HEIGHT); // Draw depth attachment to this window
			//
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST); // Disable depth test : only rendering a 2D image.
			postProcessor.use(); // Use the post-processing shader (all post processing effects will go in here)
			postProcessor.setFloat("nearPlane", nearPlane);
			postProcessor.setFloat("farPlane", farPlane);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureDepthBuffer); // Bind Colour or Depth buffer
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			renderQuad(); // render our quad.
			//
		}
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}


	glfwTerminate();
	system("pause");
	return 0;
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
		usePerlin = !usePerlin; // Swap to perlin/height mapped
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		useCDM = !useCDM; // Swap to getNormal/CDM
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		useFog = !useFog; // Toggle Fog
	if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		toggleBuffers = !toggleBuffers; // Toggle BUffers
	if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) 
		toggleShadowMapping = !toggleShadowMapping; // Toggle BUffers
		if(toggleShadowMapping)std::cout << "Toggled Shadows" << std::endl;
	


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
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	// Create colour attachment texture
	glGenTextures(1, &textureColourBuffer);
	glBindTexture(GL_TEXTURE_2D, textureColourBuffer);
	// Parameters for sampling ( set to null because we aren't using an image - screen space which we fill with our scene )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
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
	glGenFramebuffers(1, &FBO);
	// Create depth texture
	glGenTextures(1, &textureDepthBuffer);
	glBindTexture(GL_TEXTURE_2D, textureDepthBuffer);
	// Parameters for sampling ( null texture )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//
	float borderColour[] = { 1.0,1.0,1.0,1.0 }; // Border colour
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);
	// Attach depth texture as FBO depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
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

