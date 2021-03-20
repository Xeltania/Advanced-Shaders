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


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(char const * path);
//unsigned int loadTexture2(char const * path);
void setVAO(vector <float> vertices);

// camera
Camera camera(glm::vec3(260,100,300));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//arrays
unsigned int terrainVAO;

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
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);


	// simple vertex and fragment shader - add your own tess and geo shader
	Shader terrainShader("..\\shaders\\Terrain\\plainVert.vs", "..\\shaders\\Terrain\\normFrag.fs", "..\\shaders\\Terrain\\Norms.gs", "..\\shaders\\Terrain\\tessControlShader.tcs", "..\\shaders\\Terrain\\tessEvaluationShader.tes");
	Shader drawNormal("..\\shaders\\Normals\\plainVert.vs", "..\\shaders\\Normals\\plainFrag.fs", "..\\shaders\\Normals\\normalGeometry.gs", "..\\shaders\\Normals\\tessControlShader.tcs", "..\\shaders\\Normals\\tessEvaluationShader.tes");
	//Load HeightMap Texture
	unsigned int heightMap = 0;// = loadTexture("..\\resources\\heightMap.jpg");
	//Bind

	int gridSize = 10;

	//Terrain Constructor ; number of grids in width, number of grids in height, gridSize
	Terrain terrain(50, 50, gridSize);
	terrainVAO = terrain.getVAO();

	// Clear Colour
	const float clearR = 0.40f;
	const float clearG = 0.40f;
	const float clearB = 0.80f;


	while (!glfwWindowShouldClose(window))
	{

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(clearR, clearG, clearB, 1.0f);
		
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1200.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
	    terrainShader.use();
	    terrainShader.setMat4("projection", projection);
		terrainShader.setMat4("view", view);
		terrainShader.setMat4("model", model);
		terrainShader.setVec3("camPos", camera.Position);
		terrainShader.setInt("heightMap", 0);
		terrainShader.setFloat("scale", 90);
		terrainShader.setInt("octaves", 50);
		terrainShader.setInt("gridSize", gridSize);
		// Lighting
		GLint lightPos, ambient, diffuse, specular;

		lightPos = glGetUniformLocation(terrainShader.ID, "light.lightPos");
		ambient = glGetUniformLocation(terrainShader.ID, "light.ambient");
		diffuse = glGetUniformLocation(terrainShader.ID, "light.diffuse");
		specular = glGetUniformLocation(terrainShader.ID, "light.specular");
		glUniform3f (lightPos, 0.1f,-1.0f, 0.2f);
		//glUniform3f(lightPos, camera.Position.x, camera.Position.y + 20.0, camera.Position.z - 2.0);
		glUniform3f(ambient, 0.2f, 0.2f, 0.2f);
		glUniform3f(diffuse, 0.55f, 0.55f, 0.55f);
		glUniform3f(specular, 0.3f, 0.3f, 0.3f);

		// Fog
		terrainShader.setVec3("sky", glm::vec3(clearR, clearG, clearB));
		//terrainShader.setFloat("fogGradient", 0.5f);
	
	

		glBindVertexArray(terrainVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, heightMap);
		if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else 		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);



		glDrawArrays(GL_PATCHES, 0, terrain.getSize());


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
		std::cout << "Loaded texture at path: " << path << " width " << width << " id " << textureID <<  std::endl;

	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
		
	}

	return textureID;
}





