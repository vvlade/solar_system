#include <cmath>
#include <iostream>
#include <string>

#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image.h"

#include "camera.h"
#include "model.h"
#include "shader.h"

struct Orb {
  glm::vec3 Position = glm::vec3(0.0f);
  glm::vec3 Ambient = glm::vec3(0.4f);
  glm::vec3 Diffuse = glm::vec3(1.0f);
  glm::vec3 Specular = glm::vec3(0.0f);
  glm::vec3 RotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);	 // axial tilt
  glm::vec3 RevolutionCenter =
	  glm::vec3(0.0f);	// the point around which the orb revolves
  glm::vec3 RevolutionCenterSmall;
  glm::vec3 Size = glm::vec3(1.0f);
  glm::vec2 RevolutionRadiusSmall;
  glm::vec2 RevolutionRadius;
  float OrbitalInclination = 0.0f;
  float RotationSpeed = 0.0f;
  float RevolutionSpeed = 0.0f;
  float RevolutionSmallSpeed = 0.0f;
  int RevolutionNr = 0;
};

struct PointLight {
  glm::vec3 Position = glm::vec3(0.0f);
  glm::vec3 Ambient = glm::vec3(0.1f);
  glm::vec3 Diffuse = glm::vec3(1.0f);
  glm::vec3 Specular = glm::vec3(1.0f);
  float Const = 1.0;
  float Linear = 0.0014;
  float Quadratic = 0.000007;
};

struct SpotLight {
  glm::vec3 Ambient = glm::vec3(0.0f);
  glm::vec3 Diffuse = glm::vec3(1.0f);
  glm::vec3 Specular = glm::vec3(1.0f);
  float Const = 1.0;
  float Linear = 0.01;
  float Quadratic = 0.001;
  float Cutoff = 2.0f;
  float OuterCutoff = 1.0f;
};

Camera cam;

int SCR_WIDTH = 800, SCR_HEIGHT = 600;
float prevX = SCR_WIDTH / 2.0;
float prevY = SCR_HEIGHT / 2.0;
bool flashlightOn = false;

glm::vec3 issPos;

// callbacks and other functions
void processInput(GLFWwindow* window);
void cursorPositionCallback(GLFWwindow* window, double posX, double posY);
void scrollCallback(GLFWwindow* window, double offsetX, double offsetY);
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void setUpOrbData(Orb& o,
				  Shader& s,
				  PointLight& pl,
				  SpotLight& sl,
				  bool depth = false);
void keyCallback(GLFWwindow* window,
				 int key,
				 int scancode,
				 int action,
				 int mod);
auto loadTexture(const char* path) -> unsigned;
unsigned int loadSkybox(std::vector<std::string>& faces);
auto setUpTheISS() -> unsigned;
auto setUpTheSkybox() -> unsigned;
void setSpotlight(Shader& s, SpotLight& sl);

auto main() -> int {
  // init
  int initStatus = glfwInit();
  assert(initStatus == GLFW_TRUE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // window
  GLFWwindow* window = glfwCreateWindow(
	  SCR_WIDTH, SCR_HEIGHT, "A model of the Solar system", nullptr, nullptr);
  if (window == nullptr) {
	std::cout << "Could not create a window!\n";
	glfwTerminate();
	return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // load glad functions
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
	std::cout << "Could not load GLAD functions!\n";
	return -1;
  }

  // ---- CALLBACKS ----
  //--------------------
  glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
  glfwSetCursorPosCallback(window, cursorPositionCallback);
  glfwSetScrollCallback(window, scrollCallback);
  glfwSetKeyCallback(window, keyCallback);

  // configure global opengl state
  glEnable(GL_DEPTH_TEST);

  // ---- SHADERS ----
  //------------------
  Shader sunShader("resources/shaders/sunVS.vs", "resources/shaders/sunFS.fs");
  Shader issShader("resources/shaders/issVS.vs", "resources/shaders/issFS.fs");
  Shader skyboxShader("resources/shaders/skyboxVS.vs",
					  "resources/shaders/skyboxFS.fs");
  Shader orbShader("resources/shaders/someVS.vs",
				   "resources/shaders/someFS.fs");
  //    Shader orbDepthShader("resources/shaders/orbDepthVS.vs",
  //    "resources/shaders/orbDepthFS.fs", "resources/shaders/orbDepthGS.gs");

  // ---- SKYBOX ----
  //-----------------
  std::vector<std::string> faces{
	  "resources/textures/front.png", "resources/textures/back.png",
	  "resources/textures/up.png",	  "resources/textures/down.png",
	  "resources/textures/right.png", "resources/textures/left.png"};

  unsigned skyboxTexture = loadSkybox(faces);
  unsigned skyboxVAO = setUpTheSkybox();

  skyboxShader.use();
  skyboxShader.setInt("skybox", 0);

  // ---- LIGHTS ----
  //----------------------
  PointLight sunlight;
  SpotLight flashlight;

  // ---- MODELS ----
  //-----------------
  // SUN
  Model sunModel("resources/objects/Sun/Sun.obj");
  sunModel.SetShaderTextureNamePrefix("material.");

  // MERCURY
  Model mercuryModel("resources/objects/Mercury/source/Mercury/Mercury.FBX");
  mercuryModel.SetShaderTextureNamePrefix("material.");

  // VENUS
  Model venusModel("resources/objects/Venus/Sun.obj");
  venusModel.SetShaderTextureNamePrefix("material.");

  // EARTH
  Model earthModel("resources/objects/Earth/Earth.obj");
  earthModel.SetShaderTextureNamePrefix("material.");

  // MOON
  Model moonModel("resources/objects/Moon/Moon.obj");
  moonModel.SetShaderTextureNamePrefix("material.");

  // MARS
  Model marsModel("resources/objects/MarsPlanet/MarsPlanet.obj");
  marsModel.SetShaderTextureNamePrefix("material.");

  // JUPITER
  Model jupiterModel(
	  "resources/objects/Jupiter/"
	  "Jupiter_v1_L3.123c7d3fa769-8754-46f9-8dde-2a1db30a7c4e/"
	  "13905_Jupiter_V1_l3.obj");
  jupiterModel.SetShaderTextureNamePrefix("material.");

  // ---- ORBS ----
  //---------------
  // EARTH
  Orb earth;
  earth.Size = glm::vec3(0.2);
  earth.Position = glm::vec3(0.0f, 0.0f, 10.0f);
  earth.RotationAxis = glm::vec3(0.5f, 1.0f, 0.0f);

  // MOON
  Orb moon;
  moon.Position = glm::vec3(20.0f, 0.0f, 0.0f);
  moon.RevolutionRadius = glm::vec2(moon.Position.x);
  moon.Size = glm::vec3(0.04);
  moon.RevolutionNr = 1;
  moon.RotationAxis = glm::vec3(0.11f, 1.0f, 0.0f);

  // MERCURY
  Orb mercury;
  mercury.RevolutionCenterSmall = glm::vec3(45.0f, 0.0f, 0.0f);
  mercury.RevolutionRadiusSmall = glm::vec2(10.0f);
  mercury.Position = glm::vec3(
	  mercury.RevolutionCenterSmall.x + mercury.RevolutionRadiusSmall.x,
	  mercury.RevolutionCenterSmall.y, mercury.RevolutionCenterSmall.z);
  mercury.RevolutionRadius = glm::vec2(mercury.RevolutionCenterSmall.x);
  mercury.Size = glm::vec3(0.01f);
  mercury.RevolutionNr = 2;
  mercury.RotationAxis = glm::vec3(0.08f, 1.0f, 0.0f);

  // VENUS
  Orb venus;
  venus.RevolutionCenterSmall = glm::vec3(65.0f, 0.0f, 0.0f);
  venus.RevolutionRadiusSmall = glm::vec2(15.0f);
  venus.Position =
	  glm::vec3(venus.RevolutionCenterSmall.x + venus.RevolutionRadiusSmall.x,
				venus.RevolutionCenterSmall.y, venus.RevolutionCenterSmall.z);
  venus.RevolutionRadius = glm::vec2(venus.RevolutionCenterSmall.x);
  venus.Size = glm::vec3(0.1);
  venus.RevolutionNr = 2;
  venus.RotationAxis = glm::vec3(0.09f, -1.0f, 0.0f);

  // SUN
  Orb sun;
  sun.Position = glm::vec3(120.0f, 0.0f, 0.0f);
  sun.RevolutionRadius = glm::vec2(sun.Position.x);
  sun.Size = glm::vec3(0.7);
  sun.RevolutionNr = 1;
  sun.RotationAxis = glm::vec3(0.2f, 1.0f, 0.0f);

  // MARS
  Orb mars;
  mars.RevolutionCenterSmall = glm::vec3(150.0f, 0.0f, 0.0f);
  mars.RevolutionRadiusSmall = glm::vec2(25.0f);
  mars.Position =
	  glm::vec3(mars.RevolutionCenterSmall.x + mars.RevolutionRadiusSmall.x,
				mars.RevolutionCenterSmall.y, mars.RevolutionCenterSmall.z);
  mars.RevolutionRadius = glm::vec2(mars.RevolutionCenterSmall.x);
  mars.Size = glm::vec3(0.08);
  mars.RevolutionNr = 2;
  mars.RotationAxis = glm::vec3(0.6f, 1.0f, 0.0f);

  // JUPITER
  Orb jupiter;
  jupiter.RevolutionCenterSmall = glm::vec3(200.0f, 0.0f, 0.0f);
  jupiter.RevolutionRadiusSmall = glm::vec2(30.0f);
  jupiter.Position = glm::vec3(
	  jupiter.RevolutionCenterSmall.x + jupiter.RevolutionRadiusSmall.x,
	  jupiter.RevolutionCenterSmall.y, jupiter.RevolutionCenterSmall.z);
  jupiter.RevolutionRadius = glm::vec2(jupiter.RevolutionCenterSmall.x);
  jupiter.Size = glm::vec3(0.03f);
  jupiter.RevolutionNr = 2;
  jupiter.RotationAxis = glm::vec3(0.2f, 1.0f, 0.0f);

  // THE ISS
  unsigned issVAO = setUpTheISS();
  unsigned issDiffuse = loadTexture("resources/textures/iss.png");
  unsigned issSpecular = loadTexture("resources/textures/iss_specular.png");

  issShader.use();
  issShader.setInt("material.texture_diffuse", 0);
  issShader.setInt("material.texture_specular", 1);

  // ---- ISS data ----
  // ------------------
  // pointlight properties
  issShader.setVec3("light.ambient", sunlight.Ambient);
  issShader.setVec3("light.diffuse", sunlight.Diffuse);
  issShader.setVec3("light.specular", sunlight.Specular);
  issShader.setFloat("light.constant", sunlight.Const);
  issShader.setFloat("light.linear", sunlight.Linear);
  issShader.setFloat("light.quadratic", sunlight.Quadratic);

  issShader.setVec3("material.ambient", glm::vec3(1.0f));
  issShader.setVec3("material.diffuse", glm::vec3(1.0f));
  issShader.setVec3("material.specular", glm::vec3(1.0f));
  issShader.setFloat("material.shininess", 1024.0f);

  issPos = glm::vec3(earth.Position.x + 5, earth.Position.y, earth.Position.z);

  //    // --- SHADOWS ---
  //    // configure depth map FBO
  //    // -----------------------
  //    const unsigned int SHADOW_WIDTH = 800, SHADOW_HEIGHT = 600;
  //    unsigned int depthMapFBO;
  //    glGenFramebuffers(1, &depthMapFBO);
  //    // create depth cubemap texture
  //    unsigned int depthCubemap;
  //    glGenTextures(1, &depthCubemap);
  //    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
  //    for (unsigned int i = 0; i < 6; ++i) {
  //        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
  //        GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
  //                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  //    }
  //    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  //    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  //    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
  //    GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_CUBE_MAP,
  //    GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  //    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
  //    GL_CLAMP_TO_EDGE);
  //    // attach depth texture as FBO's depth buffer
  //    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  //    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap,
  //    0); glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
  //    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //
  //    orbShader.use();
  //    orbShader.setInt("depthMap", 0);

  // rendering loop
  while (!glfwWindowShouldClose(window)) {
	// poll events
	glfwPollEvents();
	processInput(window);

	// update world state
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//        // --- DRAWING SHADOW MAPS ---
	//        // 0. create depth cubemap transformation matrices
	//        // -----------------------------------------------
	//        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f),
	//        (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, 0.1f, 1000.0f);
	//        std::vector<glm::mat4> shadowTransforms;
	//        shadowTransforms.push_back(shadowProj *
	//        glm::lookAt(sunlight.Position, sunlight.Position +
	//        glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
	//        shadowTransforms.push_back(shadowProj *
	//        glm::lookAt(sunlight.Position, sunlight.Position +
	//        glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
	//        shadowTransforms.push_back(shadowProj *
	//        glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(
	//        0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
	//        shadowTransforms.push_back(shadowProj *
	//        glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(
	//        0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
	//        shadowTransforms.push_back(shadowProj *
	//        glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(
	//        0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
	//        shadowTransforms.push_back(shadowProj *
	//        glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(
	//        0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
	//
	//        // 1. render scene to depth cubemap
	//        // --------------------------------
	//        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	//        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	//        glClear(GL_DEPTH_BUFFER_BIT);
	//        orbDepthShader.use();
	//        for (unsigned int i = 0; i < 6; i++) {
	//            orbDepthShader.setMat4("shadowMatrices[" + std::to_string(i) +
	//            "]", shadowTransforms[i]);
	//        }
	//        orbDepthShader.setVec3("lightPos", sunlight.Position);
	//
	//        // EARTH
	//        earth.RotationSpeed = glfwGetTime()*30;
	//        setUpOrbData(earth, orbDepthShader, sunlight, flashlight, true);
	//        earthModel.Draw(orbDepthShader);
	//
	//
	//        // MOON
	//        moon.RotationSpeed = glfwGetTime() * (-10);
	//        moon.RevolutionSpeed = glfwGetTime() * 10.5;
	//        setUpOrbData(moon, orbDepthShader, sunlight, flashlight, true);
	//        moonModel.Draw(orbDepthShader);

	//
	//        // MERCURY
	//        mercury.RotationSpeed = glfwGetTime() * 2;
	//        mercury.RevolutionSpeed = glfwGetTime() * 5;
	//        mercury.RevolutionSmallSpeed = glfwGetTime() * 30;
	//        setUpOrbData(mercury, orbDepthShader, sunlight, flashlight, true);
	//        mercuryModel.Draw(orbDepthShader);
	//
	//
	//        // VENUS
	//        venus.RotationSpeed = glfwGetTime() * 0.2;
	//        venus.RevolutionSpeed = glfwGetTime() * 2;
	//        venus.RevolutionSmallSpeed = glfwGetTime() * 20;
	//        setUpOrbData(venus, orbDepthShader, sunlight, flashlight, true);
	//        venusModel.Draw(orbDepthShader);
	//
	//
	//        // MARS
	//        mars.RotationSpeed = glfwGetTime() * 20;
	//        mars.RevolutionSpeed = glfwGetTime() * 3;
	//        mars.RevolutionSmallSpeed = glfwGetTime() * 25;
	//        setUpOrbData(mars, orbDepthShader, sunlight, flashlight, true);
	//        marsModel.Draw(orbDepthShader);
	//
	//
	//        // JUPITER
	//        jupiter.RotationSpeed = glfwGetTime() * 30;
	//        jupiter.RevolutionSpeed = glfwGetTime();
	//        jupiter.RevolutionSmallSpeed = glfwGetTime() * 20;
	//        setUpOrbData(jupiter, orbDepthShader, sunlight, flashlight, true);
	//        jupiterModel.Draw(orbDepthShader);

	//        glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ---- DRAWING OBJECTS ----
	//--------------------------

	// THE ISS
	issShader.use();
	issShader.setVec3("light.position", sunlight.Position);

	glm::mat4 projection =
		glm::perspective(glm::radians(cam.Zoom),
						 (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	issShader.setMat4("projection", projection);
	issShader.setMat4("view", cam.GetViewMatrix());

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, issPos);
	// changing the ISS's position so it rotates around the Earth: k(r*cos() +
	// a, r*sin() + b); y: making the ISS go up and down
	model = glm::rotate(model, glm::radians((float)glfwGetTime() * (-4.005f)),
						glm::vec3(0.0f, 1.0f, 0.0f));
	issPos =
		glm::vec3(5 * cos(glm::radians(glfwGetTime() * 5)) + earth.Position.x,
				  sin(glm::radians(glfwGetTime() * 20)),
				  5 * sin(glm::radians(glfwGetTime() * 5)) + earth.Position.z);
	model = glm::scale(model, glm::vec3(0.5f));
	issShader.setMat4("model", model);

	setSpotlight(issShader, flashlight);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, issDiffuse);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, issSpecular);

	glDisable(GL_CULL_FACE);
	glBindVertexArray(issVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// ---- PLANETS ----
	// -----------------
	// SUN
	sunShader.use();
	sunShader.setMat4("projection", projection);
	sunShader.setMat4("view", cam.GetViewMatrix());
	sunShader.setVec3("material.diffuse", glm::vec3(1.0f));

	flashlight.Ambient = glm::vec3(1.0f);
	sun.RotationSpeed = glfwGetTime() * 2;
	sun.RevolutionSpeed = glfwGetTime() * 5;
	setUpOrbData(sun, sunShader, sunlight, flashlight);
	sunModel.Draw(sunShader);
	sunlight.Position = sun.Position;

	flashlight.Ambient = glm::vec3(0.0f);

	orbShader.use();
	orbShader.setMat4("projection", projection);
	orbShader.setMat4("view", cam.GetViewMatrix());

	// EARTH
	earth.RotationSpeed = glfwGetTime() * 30;
	setUpOrbData(earth, orbShader, sunlight, flashlight);
	earthModel.Draw(orbShader);

	// MOON
	moon.RotationSpeed = glfwGetTime() * (-10);
	moon.RevolutionSpeed = glfwGetTime() * 10.5;
	setUpOrbData(moon, orbShader, sunlight, flashlight);
	moonModel.Draw(orbShader);

	// MERCURY
	mercury.RotationSpeed = glfwGetTime() * 2;
	mercury.RevolutionSpeed = glfwGetTime() * 5;
	mercury.RevolutionSmallSpeed = glfwGetTime() * 30;
	setUpOrbData(mercury, orbShader, sunlight, flashlight);
	mercuryModel.Draw(orbShader);

	// VENUS
	venus.RotationSpeed = glfwGetTime() * 0.2;
	venus.RevolutionSpeed = glfwGetTime() * 2;
	venus.RevolutionSmallSpeed = glfwGetTime() * 20;
	setUpOrbData(venus, orbShader, sunlight, flashlight);
	venusModel.Draw(orbShader);

	// MARS
	mars.RotationSpeed = glfwGetTime() * 20;
	mars.RevolutionSpeed = glfwGetTime() * 3;
	mars.RevolutionSmallSpeed = glfwGetTime() * 25;
	setUpOrbData(mars, orbShader, sunlight, flashlight);
	marsModel.Draw(orbShader);

	// JUPITER
	jupiter.RotationSpeed = glfwGetTime() * 30;
	jupiter.RevolutionSpeed = glfwGetTime();
	jupiter.RevolutionSmallSpeed = glfwGetTime() * 20;
	setUpOrbData(jupiter, orbShader, sunlight, flashlight);
	jupiterModel.Draw(orbShader);

	//        glActiveTexture(GL_TEXTURE0);
	//        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

	// ---- SKYBOX ----
	// ----------------
	glDepthFunc(GL_LEQUAL);

	skyboxShader.use();
	glm::mat4 view = glm::mat4(glm::mat3(cam.GetViewMatrix()));
	skyboxShader.setMat4("view", view);
	skyboxShader.setMat4("projection", projection);

	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	// render image
	glfwSwapBuffers(window);
  }

  // de-init
  glfwTerminate();
  return 0;
}

//------------------------
// processes all the input
//------------------------
void processInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
	glfwSetWindowShouldClose(window, true);
  }

  cam.MovementSpeed = 0.4f;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
	cam.ProcessKeyboard(FORWARD);
  } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
	cam.ProcessKeyboard(BACKWARD);
  } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
	cam.ProcessKeyboard(LEFT);
  } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
	cam.ProcessKeyboard(RIGHT);
  }
}
//------------------------
// whenever the window size changes, this function is called
//------------------------
void frameBufferSizeCallback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}
//------------------------
// whenever the mouse cursor changes it's position, this function is called
//------------------------
void cursorPositionCallback(GLFWwindow* window, double posX, double posY) {
  float offsetX = posX - prevX;
  float offsetY = prevY - posY;
  prevX = posX;
  prevY = posY;
  cam.ProcessMouseMovement(offsetX, offsetY, true);
}
//------------------------
// whenever the mouse scrolls, this function is called
//------------------------
void scrollCallback(GLFWwindow* window, double offsetX, double offsetY) {
  cam.ProcessMouseScroll(offsetY);
}
//------------------------
// setting up the shader data to draw the planets and the sun
//------------------------
void setUpOrbData(Orb& o,
				  Shader& s,
				  PointLight& pl,
				  SpotLight& sl,
				  bool depth) {
  // depth == true => we don't need/have these attributes
  if (!depth) {
	// PointLight
	s.setVec3("light.position", pl.Position);
	s.setVec3("light.ambient", pl.Ambient);
	s.setVec3("light.diffuse", pl.Diffuse);
	s.setVec3("light.specular", pl.Specular);
	s.setFloat("light.constant", pl.Const);
	s.setFloat("light.linear", pl.Linear);
	s.setFloat("light.quadratic", pl.Quadratic);

	// SpotLight
	setSpotlight(s, sl);

	s.setVec3("material.ambient", o.Ambient);
	s.setVec3("material.diffuse", o.Diffuse);
	s.setVec3("material.specular", o.Specular);
	s.setFloat("material.shininess", 32.0f);

	s.setVec3("ViewPos", cam.Position);
  }

  // transformations
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, o.Position);
  model = glm::rotate(model, glm::radians(o.RotationSpeed), o.RotationAxis);
  model = glm::scale(model, o.Size);
  s.setMat4("model", model);

  float x, y, z, xp, yp, zp;
  switch (o.RevolutionNr) {
	case 1: {
	  x = o.RevolutionRadius.x * cos(glm::radians(o.RevolutionSpeed)) +
		  o.RevolutionCenter.x;
	  if (o.OrbitalInclination == 0) {
		y = 0.0;
	  } else {
		y = sin(glm::radians(o.RevolutionSpeed)) / o.OrbitalInclination;
	  }
	  // r*sin() + b
	  z = o.RevolutionRadius.y * sin(glm::radians(o.RevolutionSpeed)) +
		  o.RevolutionCenter.z;
	  o.Position = glm::vec3(x, y, z);
	} break;
	case 2: {
	  // big revolution
	  x = o.RevolutionRadius.x * cos(glm::radians(o.RevolutionSpeed)) +
		  o.RevolutionCenter.x;
	  y = 0.0f;
	  z = o.RevolutionRadius.y * sin(glm::radians(o.RevolutionSpeed)) +
		  o.RevolutionCenter.z;
	  o.RevolutionCenterSmall = glm::vec3(x, y, z);

	  // small revolution
	  xp = o.RevolutionRadiusSmall.x *
			   cos(glm::radians(o.RevolutionSmallSpeed)) +
		   o.RevolutionCenterSmall.x;
	  yp = 0.0f;
	  // r*sin() + b
	  zp = o.RevolutionRadiusSmall.y *
			   sin(glm::radians(o.RevolutionSmallSpeed)) +
		   o.RevolutionCenterSmall.z;
	  o.Position = glm::vec3(xp, yp, zp);
	} break;
  }
}
//------------------------
// setting up the data and vertices to draw the ISS
//------------------------
auto setUpTheISS() -> unsigned {
  std::vector<float> vertices = {
	  //            position                 normals          texture
	  -0.8f, -0.4f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,	 // 0
	  0.8f,	 -0.4f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,	 // 1
	  -0.8f, 0.4f,	0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,	 // 3

	  0.8f,	 -0.4f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,	 // 1
	  0.8f,	 0.4f,	0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,	 // 2
	  -0.8f, 0.4f,	0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f	 // 3
  };

  unsigned issVBO, issVAO;

  glGenBuffers(1, &issVBO);
  glGenVertexArrays(1, &issVAO);

  glBindVertexArray(issVAO);

  glBindBuffer(GL_ARRAY_BUFFER, issVBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
			   vertices.data(), GL_STATIC_DRAW);

  // describing positions
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
						(void*)nullptr);
  glEnableVertexAttribArray(0);
  // describing normals
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
						(void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  // describing texture
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
						(void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  return issVAO;
}
//------------------------
// setting up the data and vertices to draw the skybox
//------------------------
auto setUpTheSkybox() -> unsigned {
  std::vector<float> vertices = {
	  // positions
	  -1.0f, 1.0f,	-1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
	  1.0f,	 -1.0f, -1.0f, 1.0f,  1.0f,	 -1.0f, -1.0f, 1.0f,  -1.0f,

	  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
	  -1.0f, 1.0f,	-1.0f, -1.0f, 1.0f,	 1.0f,	-1.0f, -1.0f, 1.0f,

	  1.0f,	 -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,	1.0f,  1.0f,  1.0f,
	  1.0f,	 1.0f,	1.0f,  1.0f,  1.0f,	 -1.0f, 1.0f,  -1.0f, -1.0f,

	  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,	 1.0f,	1.0f,  1.0f,  1.0f,
	  1.0f,	 1.0f,	1.0f,  1.0f,  -1.0f, 1.0f,	-1.0f, -1.0f, 1.0f,

	  -1.0f, 1.0f,	-1.0f, 1.0f,  1.0f,	 -1.0f, 1.0f,  1.0f,  1.0f,
	  1.0f,	 1.0f,	1.0f,  -1.0f, 1.0f,	 1.0f,	-1.0f, 1.0f,  -1.0f,

	  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,	1.0f,  -1.0f, -1.0f,
	  1.0f,	 -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,	1.0f,  -1.0f, 1.0f};

  unsigned int skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);

  glBindVertexArray(skyboxVAO);

  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
			   vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
						(void*)nullptr);
  glEnableVertexAttribArray(0);

  return skyboxVAO;
}
//------------------------
// loading a 2D texture from resources/textures
//------------------------
auto loadTexture(const char* path) -> unsigned {
  unsigned tex0;
  int width, height, nrChannels;
  unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

  if (data) {
	glGenTextures(1, &tex0);
	glBindTexture(GL_TEXTURE_2D, tex0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLenum format;
	switch (nrChannels) {
	  case 1: {
		format = GL_RED;
	  } break;
	  case 3: {
		format = GL_RGB;
	  } break;
	  case 4: {
		format = GL_RGBA;
	  } break;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
				 GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
  } else {
	std::cout << "Failed to load texture!\n";
  }
  stbi_image_free(data);

  return tex0;
}
//------------------------
// loading a 2D texture for the skybox from resources/textures
//------------------------
unsigned loadSkybox(std::vector<std::string>& faces) {
  unsigned textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  int width, height, nrChannels;
  for (unsigned i = 0; i < faces.size(); i++) {
	unsigned char* data =
		stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
	if (data) {
	  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB_ALPHA, width,
				   height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	  stbi_image_free(data);
	} else {
	  std::cout << "Cubemap tex failed to load at path: " << faces[i]
				<< std::endl;
	  stbi_image_free(data);
	}
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return textureID;
}
//------------------------
// processes input from relevant keys
//------------------------
void keyCallback(GLFWwindow* window,
				 int key,
				 int scancode,
				 int action,
				 int mod) {
  if (key == GLFW_KEY_F && action == GLFW_PRESS) {
	flashlightOn = !flashlightOn;
  }
}
//------------------------
// sets and updates spotlight properites
//------------------------
void setSpotlight(Shader& s, SpotLight& sl) {
  s.setVec3("spotLight.position", cam.Position);
  s.setVec3("spotLight.direction", cam.Front);
  s.setFloat("spotLight.cutoff", cos(glm::radians(sl.Cutoff)));
  s.setFloat("spotLight.outerCutoff", cos(glm::radians(sl.OuterCutoff)));
  if (flashlightOn) {
	s.setVec3("spotLight.ambient", sl.Ambient);
	s.setVec3("spotLight.diffuse", sl.Diffuse);
	s.setVec3("spotLight.specular", sl.Specular);
  } else {	// All to 0.
	s.setVec3("spotLight.ambient", glm::vec3(0.0f));
	s.setVec3("spotLight.diffuse", glm::vec3(0.0f));
	s.setVec3("spotLight.specular", glm::vec3(0.0f));
  }
  s.setFloat("spotLight.constant", sl.Const);
  s.setFloat("spotLight.linear", sl.Linear);
  s.setFloat("spotLight.quadratic", sl.Quadratic);
}
