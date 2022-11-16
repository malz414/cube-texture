
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>


#include "Shader.h"
#include "Camera.h"
#include "Renderer.h"

#include<string>
#include <iostream>
#include <numeric>




// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
int main();
void processInput(GLFWwindow *window);



void setUniforms(Shader& shader, Shader& shader2, Shader& lightShader);
void updatePerFrameUniforms(Shader& cubeShader, Shader& floorShader, Camera camera, bool DL, bool SL, bool PL);
unsigned int loadTexture(char const* path);

void setFBOcd();
void FBOblur();
void setFBOdepth();
// camera
Camera camera(glm::vec3(0, 0, 9));


float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

unsigned int myFBOcd, myFBOdepth;

////********SSSSSSSSSHHHlightHHHHHHADDDDDDDDDDDDDDDDDDDOOOOOOOOOOWWWWWWWWWWW MMMAAAPPP


const unsigned int SHADOW_WIDTH = 1028;
const unsigned int SHADOW_HEIGHT = 1028;
unsigned int depthMap;
unsigned int FBOshadow;
//
//
unsigned int blurFBO;
unsigned int colourAttachment[3], depthAttachment, blurredTexture;

bool DL = true;
bool SL = false;
bool PL = false;
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


glm::vec3 lightDirection = glm::vec3(0.0, -1.0, 0.40);
glm::vec3 lightColor = glm::vec3(1.0, 1.0, 1.0);





int main() {
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


	//renderer
	Renderer renderer(SCR_WIDTH, SCR_HEIGHT);

	// simple vertex and fragment shader 
	Shader cubeShader("..\\shaders\\plainVert.vs", "..\\shaders\\plainFrag.fs");
	Shader floorShader("..\\shaders\\floorFrag.vs", "..\\shaders\\floorFrag.fs");
	Shader lightShader("..\\shaders\\light.vs", "..\\shaders\\light.fs");
	Shader ppShader("..\\shaders\\pp.vs", "..\\shaders\\pp.fs");
	Shader ppDepth("..\\shaders\\pp.vs", "..\\shaders\\ppDepth.fs");
	Shader blur("..\\shaders\\pp.vs", "..\\shaders\\blur.fs");
	Shader bloom("..\\shaders\\pp.vs", "..\\shaders\\bloom.fs");
	Shader shadowMap("..\\shaders\\SM.vs", "..\\shaders\\SM.fs");
	Shader skyboxShader("..\\shaders\\SB.vs", "..\\shaders\\SB.fs");
	///Shader Refraction("..\\shaders\\Refraction.vs", "..\\shaders\\Refraction.fs");
	


	setUniforms(cubeShader, floorShader, lightShader);



	ppShader.use();
	ppShader.setInt("image", 0);
	ppDepth.use();
	ppDepth.setInt("image", 0);
	blur.use();
	blur.setInt("image", 0);
	bloom.use();
	bloom.setInt("image", 0);
	bloom.setInt("bloomBlur", 1);
	//
	setFBOcd();
	FBOblur();
	setFBOdepth();
	float orthoSize = 10;

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		float time = (float)(currentFrame * 0.5);
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		glBindFramebuffer(GL_FRAMEBUFFER, FBOshadow);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		/*updatePerFrameUniforms(cubeShader, floorShader, camera, DL, SL, PL);
		renderer.renderScene(cubeShader, floorShader, skyboxShader, camera);*/

		glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, -orthoSize, 2 * orthoSize);
		glm::mat4 lightView = glm::lookAt(lightDirection*glm::vec3(-0.60f), glm::vec3(0.10f), glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;
		shadowMap.use();
		shadowMap.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		renderer.renderCubes(shadowMap);


		glBindFramebuffer(GL_FRAMEBUFFER, myFBOcd);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cubeShader.use();
		cubeShader.setInt("depthMap", 4);
		cubeShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		floorShader.use();
		floorShader.setInt("depthMap", 4);
		floorShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, depthMap);

	
			//renderer.renderQuad(ppDepth, depthMap);


		

		updatePerFrameUniforms(cubeShader, floorShader, camera, DL, SL, PL);
		renderer.renderScene(cubeShader, floorShader, skyboxShader, camera);
		
		/*
		float orthoSize = 10;


			glBindFramebuffer(GL_FRAMEBUFFER, FBOshadow);
			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT,);
			glEnable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			updatePerFrameUniforms(cubeShader, floorShader, camera, DL, SL, PL);
			renderer.renderLights(lightShader, camera);

			glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, orthoSize, -orthoSize, -orthoSize, 2*orthoSize);
			glm::mat4 lightView = glm:::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
			glm::mat4 lightSpaceMatrix = lightProjection * lightView;
			shadowMap.use();
			shadowMap.setMat4("lightSpaceMatrix", lightSpaceMatrix);


			renderer.drawCube(shadowMap, time);
			renderer.drawFloor(shadowMap);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST);
				glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			cubeShader.use();
			cuberShader.setInt("depthMap", 4);
			cubeShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
			floorShader.use();
			floorShader.setInt("depthMap", 4);
			floorShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GK_TEXTURE_2D, depthMap);

			renderer.renderScene(cubeShader, floorShader, camera, time);

			*/


		glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
		glDisable(GL_DEPTH_TEST);

		blur.use();
		blur.setInt("hor", 1);
		renderer.renderQuad(blur, colourAttachment[0]);
		blur.setInt("hor", 0);
		renderer.renderQuad(blur, blurredTexture);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//renderer.renderQuad(ppDepth, depthAttachment);
		//renderer.renderQuad(ppShader, colourAttachment[0]);
		renderer.renderQuad(ppShader, colourAttachment[0]);
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			renderer.renderQuad(blur, blurredTexture);

		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			renderer.renderQuad(bloom, colourAttachment[0], blurredTexture);
		}
		

		glfwSwapBuffers(window);
		glfwPollEvents();
	}





	glfwTerminate();
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

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		if (DL == true) DL = false;
		else DL = true;
	}

	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		if (PL == true) PL = false;
		else PL = true;
	}
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
		if (SL == true) SL = false;
		else SL = true;
	}

}
//******* ADDD INPUT TO TURN OFF AND ONN LIGHTS LECTURE 5.3*******
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


void setUniforms(Shader& shader, Shader& floorShader, Shader& lightShader) {

	float BloomMinBrightness = 0.10f;
	shader.use();
	// directional 

	shader.setVec3("lightCol", lightColor);
	shader.setVec3("lightDir", lightDirection);

	shader.setInt("diffuseTexture", 0);
	shader.setInt("specTexture", 1);
	shader.setInt("normalMap", 2);
	shader.setInt("dispMap", 3);
	shader.setFloat("bloomBrightness", BloomMinBrightness);


	//point light

	glm::vec3 pLightPos = glm::vec3(2.0, 3.0, 4.0);
	glm::vec3 pLightCol = glm::vec3(1.0, 1.0, 1.0);
	float Kc = 1.0;
	float Kl = 0.22f;
	float Ke = 0.2f;

	shader.setVec3("pLight.position", pLightPos);
	shader.setVec3("pLight.color", pLightCol);
	shader.setFloat("pLight.Kc", Kc);
	shader.setFloat("pLight.Kl", Kl);
	shader.setFloat("pLight.Ke", Ke);
	//spot


	shader.setVec3("sLight.color", glm::vec3(1.0f, 1.0f, 1.0f));
	shader.setFloat("sLight.Kc", 1.0f);
	shader.setFloat("sLight.Kl", 0.027f);
	shader.setFloat("sLight.Ke", 0.0028f);
	shader.setFloat("sLight.innerRad", glm::cos(glm::radians(12.5f)));
	shader.setFloat("sLight.outerRad", glm::cos(glm::radians(17.5f)));



	// floor Shader

	//light dir
	floorShader.use();

	floorShader.setVec3("lightCol", lightColor);
	floorShader.setVec3("lightDir", lightDirection);
	//textures
	floorShader.setInt("diffuseTexture", 0);
	floorShader.setInt("specTexture", 1);
	floorShader.setInt("normalMap", 2);
	floorShader.setInt("dispMap", 3);
	floorShader.setFloat("PXscale", .0150);
	floorShader.setFloat("bloomBrightness", BloomMinBrightness);

	//point
	floorShader.setVec3("pLight.position", pLightPos);
	floorShader.setVec3("pLight.color", pLightCol);
	floorShader.setFloat("pLight.Kc", Kc);
	floorShader.setFloat("pLight.Kl", Kl);
	floorShader.setFloat("pLight.Ke", Ke);
	//spot

	floorShader.setVec3("sLight.color", glm::vec3(1.0f, 1.0f, 1.0f));
	floorShader.setFloat("sLight.Kc", 1.0f);
	floorShader.setFloat("sLight.Kl", 0.027f);
	floorShader.setFloat("sLight.Ke", 0.0028f);
	floorShader.setFloat("sLight.innerRad", glm::cos(glm::radians(12.5f)));
	floorShader.setFloat("sLight.outerRad", glm::cos(glm::radians(17.5f)));

}



void updatePerFrameUniforms(Shader& cubeShader, Shader& floorShader, Camera camera, bool DL, bool SL, bool PL) {

	cubeShader.use();
	cubeShader.setVec3("sLight.position", camera.Position);
	cubeShader.setVec3("sLight.direction", (camera.Front));
	cubeShader.setBool("DL", DL);
	cubeShader.setBool("SL", SL);
	cubeShader.setBool("PL", PL);

	floorShader.use();
	floorShader.setVec3("sLight.position", camera.Position);
	floorShader.setVec3("sLight.direction", (camera.Front));
	floorShader.setBool("DL", DL);
	floorShader.setBool("SL", SL);
	floorShader.setBool("PL", PL);

}
//****MAKE FRAMEBUFFER CLASS******
  void setFBOcd() {
  glGenFramebuffers(1, &myFBOcd);
  glBindFramebuffer(GL_FRAMEBUFFER, myFBOcd);
  
  glGenTextures(2, colourAttachment);
  
  for (int i = 0; i < 3; i++) {
  	glBindTexture(GL_TEXTURE_2D, colourAttachment[i]);
  
  	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, colourAttachment[i], 0);
  }
  //*********WRITE DEPTH INFO TO TEXTURE ?? LEXTURE 7.4******************
  
  
  glGenTextures(1, &depthAttachment);
  glBindTexture(GL_TEXTURE_2D, depthAttachment);
  //glGenFramebuffers(1, &myFBOdepth);
  //glBindFramebuffer(GL_FRAMEBUFFER, myFBOdepth);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthAttachment, 0);
  
  unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, attachments);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  
  }
  
  	
//void setFBOdepth() {
//		glGenFramebuffers(1, &myFBOdepth);
//		glBindFramebuffer(GL_FRAMEBUFFER, myFBOdepth);
//		glGenTextures(1, &depthAttachment);
//		glBindTexture(GL_TEXTURE_2D, depthAttachment);
//
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthAttachment, 0);
//		glDrawBuffer(GL_NONE);
//		glReadBuffer(GL_NONE);
//		glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//
//
//}

void FBOblur() {
	glGenFramebuffers(1, &blurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
	glGenTextures(1, &blurredTexture);
	glBindTexture(GL_TEXTURE_2D, blurredTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurredTexture, 0);

	//*********WRITE DEPTH INFO TO TEXTURE ?? LEXTURE 7.4******************
}



void setFBOdepth() {
	glGenFramebuffers(1, &FBOshadow);
	glBindFramebuffer(GL_FRAMEBUFFER, FBOshadow);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);



}
