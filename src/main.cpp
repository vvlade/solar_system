#include <iostream>
#include <cmath>
#include <string>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image.h"

#include "shader.h"
#include "camera.h"
#include "model.h"

// FIXME: point shadows
// i have to activeate GL_TEXTURE0 jer sam stavio u mashu da krece od 1 i da se aktivira od 1

struct Orb {
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Ambient = glm::vec3(0.1f);
    glm::vec3 Diffuse = glm::vec3(0.6f);
    glm::vec3 Specular = glm::vec3(0.0f);
    glm::vec3 RotationAngle = glm::vec3(0.0f, 1.0f, 0.0f);                                                // axial tilt
    glm::vec3 RevolutionCenter = glm::vec3(0.0f);                     // the point around which the orb revolves
    glm::vec3 Size = glm::vec3(1.0f);
    glm::vec2 RevolutionRadius;
    float OrbitalInclination = 0.0f;
    float RotationSpeed = 0.0f;
    float RevolutionSpeed = 0.0f;
    bool Revolves = true;                                                    // does the orb revolve around something
};

struct PointLight {
    glm::vec3 Position = glm::vec3(0.0f);
    float Const = 1.0;
    float Linear = 0.0014;
    float Quadratic = 0.000007;
};

Camera cam;

int SCR_WIDTH = 800, SCR_HEIGHT = 600;
float prevX = SCR_WIDTH / 2.0;
float prevY = SCR_HEIGHT / 2.0;

glm::vec3 issPos;

// callbacks and other functions
void processInput(GLFWwindow* window);
void cursorPositionCallback(GLFWwindow* window, double posX, double posY);
void scrollCallback(GLFWwindow* window, double offsetX, double offsetY);
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void setUpOrbData(Orb& o, Shader& s, PointLight& pl, bool depth);
unsigned loadTexture(const char* path);
unsigned int loadSkybox(std::vector<std::string> &faces);
unsigned setUpTheISS();
unsigned setUpTheSkybox();
void renderDepthScene(Shader &s, std::vector<Orb> &os, std::vector<Model> &ms, PointLight &l);


int main() {
    // init
    int initStatus = glfwInit();
    assert(initStatus == GLFW_TRUE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "A model of the Solar system", nullptr, nullptr);
    if(window == nullptr) {
        std::cout << "Could not create a window!\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // load glad functions
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Could not load GLAD functions!\n";
        return -1;
    }

    // ---- CALLBACKS ----
    //--------------------
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // ---- SHADERS ----
    //------------------
    Shader sunShader("resources/shaders/someVS.vs", "resources/shaders/someFS.fs");
    Shader issShader("resources/shaders/issVS.vs", "resources/shaders/issFS.fs");
    Shader skyboxShader("resources/shaders/skyboxVS.vs", "resources/shaders/skyboxFS.fs");
    //TODO: change/implement
    Shader orbShader("resources/shaders/mainVS.vs", "resources/shaders/mainFS.fs");
    Shader depthShader("resources/shaders/pointShadowDepthVS.vs", "resources/shaders/pointShadowDepthFS.fs", "resources/shaders/pointShadowDepthGS.gs");

    // FIXME: brightness of the skybox
    // ---- SKYBOX ----
    //-----------------
    std::vector<std::string> faces {
        "resources/textures/front.png",
        "resources/textures/back.png",
        "resources/textures/up.png",
        "resources/textures/down.png",
        "resources/textures/right.png",
        "resources/textures/left.png"
    };

    unsigned skyboxTexture = loadSkybox(faces);
    unsigned skyboxVAO = setUpTheSkybox();

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // ---- POINT LIGHT ----
    //----------------------
    PointLight sunlight;

// TODO: implement, format
    // ---- POINT SHADOWS ----
    // -----------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    // configure depth map FBO
    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    orbShader.use();
    orbShader.setInt("depthMap", 0);


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

    // FIXME: find a model that rotates correctly
    // JUPITER
    Model jupiterModel("resources/objects/Jupiter/Jupiter_v1_L3.123c7d3fa769-8754-46f9-8dde-2a1db30a7c4e/13905_Jupiter_V1_l3.obj");
    jupiterModel.SetShaderTextureNamePrefix("material.");

    // ---- ORBS ----
    //---------------
    // SUN
    Orb sun;
    sun.Ambient = glm::vec3(1.0f);
    sun.Diffuse = glm::vec3(0.0f);
    sun.Specular = glm::vec3(0.0f);
    sun.Size = glm::vec3(0.7);
    sun.Revolves = false;

    // MERCURY
    Orb mercury;
    mercury.Position = glm::vec3(20.0f, 0.0f, 0.0f);
    mercury.RevolutionRadius = glm::vec2(mercury.Position.x);
    mercury.Specular = glm::vec3(0.1f);
    mercury.Size = glm::vec3(0.01f);

    // VENUS
    Orb venus;
    venus.Position = glm::vec3(70.0f, 0.0f, 0.0f);
    venus.RevolutionRadius = glm::vec2(venus.Position.x);
    venus.Size = glm::vec3(0.1);

    // EARTH
    Orb earth;
    earth.Position = glm::vec3(39.0f, 0.0f, 0.0f);
    earth.RevolutionRadius = glm::vec2(earth.Position.x);
    earth.Size = glm::vec3(0.2);

    // MOON
    Orb moon;
    moon.Position = glm::vec3(earth.Position.x+7, earth.Position.y, earth.Position.z);
    moon.RevolutionRadius = glm::vec2(7.0f);
    moon.RevolutionCenter = earth.Position;
    moon.Size = glm::vec3(0.09);

    // MARS
    Orb mars;
    mars.Position = glm::vec3(50.0f, 0.0f, 0.0f);
    mars.RevolutionRadius = glm::vec2(mars.Position.x);
    mars.Size = glm::vec3(0.05);

    // JUPITER
    Orb jupiter;
    jupiter.Position = glm::vec3(90.0f, 0.0f, 0.0f);
    jupiter.RevolutionRadius = glm::vec2(jupiter.Position.x);
    jupiter.Size = glm::vec3(0.01f);

    // THE ISS
    unsigned issVAO = setUpTheISS();
    issPos = glm::vec3(earth.Position.x+5, earth.Position.y, earth.Position.z);

    unsigned issTexture = loadTexture("resources/textures/iss.png");

    issShader.use();
    issShader.setInt("tex0", 0);

    std::vector<Orb> orbs = {mercury, venus, earth, moon, mars, jupiter};
    std::vector<Model> models = {mercuryModel, venusModel, earthModel, moonModel, marsModel, jupiterModel};
    // rendering loop
    while(!glfwWindowShouldClose(window)) {
        // poll events
        glfwPollEvents();
        processInput(window);

        // update world state
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- POINT SHADOWS ----
        // -----------------------
        // 0. create depth cubemap transformation matrices
        // -----------------------------------------------
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(sunlight.Position, sunlight.Position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        // 1. render scene to depth cubemap
        // --------------------------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        for (unsigned int i = 0; i < 6; ++i) {
            depthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        }
        depthShader.setFloat("far_plane", far_plane);
        depthShader.setVec3("lightPos", sunlight.Position);
        renderDepthScene(depthShader, orbs, models, sunlight);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2nd render
        // --------------------------------------------------------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // ---- DRAWING THE ISS ----
        //--------------------------
//      TODO: make the iss have specular lighting; add things to the fragment shader
        issShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(cam.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        issShader.setMat4("projection", projection);

        issShader.setMat4("view", cam.getViewMatrix());

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, issPos);
        // changing the ISS's position so it rotates around the Earth: k(r*cos() + a, r*sin() + b); y: making the ISS go up and down
        issPos = glm::vec3(5*cos(glm::radians(glfwGetTime()*10)) + earth.Position.x, sin(glm::radians(glfwGetTime()*20)), 5*sin(glm::radians(glfwGetTime()*10)) + earth.Position.z);
        model = glm::scale(model, glm::vec3(0.5f));
        issShader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, issTexture);

        glDisable(GL_CULL_FACE);
        glBindVertexArray(issVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);


        // TODO: set up additional data and draw the objects on the scene
        // ---- DRAWING OBJECTS ----
        //--------------------------
        //SUN
        sunShader.use();
        sunShader.setMat4("projection", projection);
        sunShader.setMat4("view", cam.getViewMatrix());


        // depth -> do we render depth scene or do we render a normal scene
        sun.RotationSpeed = glfwGetTime() * 20;
        setUpOrbData(sun, sunShader, sunlight, false);
        sunModel.Draw(sunShader);

        // ---- PLANETS ----
        // -----------------
        orbShader.use();
        orbShader.setMat4("projection", projection);
        orbShader.setMat4("view", cam.getViewMatrix());
        orbShader.setFloat("far_plane", far_plane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

        // MERCURY
        mercury.RotationSpeed = glfwGetTime() * 20;
        mercury.RevolutionSpeed = glfwGetTime() * 10;
        setUpOrbData(mercury, orbShader, sunlight, false);
        mercuryModel.Draw(orbShader);

        // VENUS
        venus.RotationSpeed = glfwGetTime();
        venus.RevolutionSpeed = glfwGetTime();
        setUpOrbData(venus, orbShader, sunlight, false);
        venusModel.Draw(orbShader);

        // EARTH
        earth.RotationSpeed = glfwGetTime();
        earth.RevolutionSpeed = glfwGetTime();
        setUpOrbData(earth, orbShader, sunlight, false);
        earthModel.Draw(orbShader);

        // MOON
        moon.RotationSpeed = glfwGetTime() * 20;
        moon.RevolutionSpeed = glfwGetTime() * 20;
        setUpOrbData(moon, orbShader, sunlight, false);
        moonModel.Draw(orbShader);

        // MARS
        mars.RotationSpeed = glfwGetTime() * 10;
        mars.RevolutionSpeed = glfwGetTime() * 10;
        setUpOrbData(mars, orbShader, sunlight, false);
        marsModel.Draw(orbShader);


        // JUPITER
        jupiter.RotationSpeed = glfwGetTime();
        jupiter.RevolutionSpeed = glfwGetTime();
        setUpOrbData(jupiter, orbShader, sunlight, false);
        jupiterModel.Draw(orbShader);

        // ---- SKYBOX ----
        // ----------------
        glDepthFunc(GL_LEQUAL);

        skyboxShader.use();
        glm::mat4 view = glm::mat4(glm::mat3(cam.getViewMatrix()));
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
// processes all the input from the relevant keys o the keyboard
//------------------------
void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    cam.cameraSpeed = 0.3f;
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cam.processKeyboard(FORWARD);
    }
    else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cam.processKeyboard(BACKWARD);
    }
    else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cam.processKeyboard(LEFT);
    }
    else if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cam.processKeyboard(RIGHT);
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
    cam.processMouse(offsetX, offsetY, true);
}
//------------------------
// whenever the mouse scrolls, this function is called
//------------------------
void scrollCallback(GLFWwindow* window, double offsetX, double offsetY) {
    cam.processScroll(offsetY);
}
//------------------------
// setting up the shader data to draw the planets and the sun
//------------------------
void setUpOrbData(Orb& o, Shader& s, PointLight& pl, bool depth) {
    if(!depth) {
        s.setVec3("light.position", pl.Position);

        s.setVec3("light.ambient", o.Ambient);
        s.setVec3("light.diffuse", o.Diffuse);
        s.setVec3("light.specular", o.Specular);
        s.setFloat("light.constant", pl.Const);
        s.setFloat("light.linear", pl.Linear);
        s.setFloat("light.quadratic", pl.Quadratic);

        s.setFloat("material.shininess", 32.0f);

        s.setVec3("ViewPos", cam.Position);
    }

    // transformations
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, o.Position);
    model = glm::rotate(model, glm::radians(o.RotationSpeed), o.RotationAngle);
    model = glm::scale(model, o.Size);
    s.setMat4("model", model);
    if(o.Revolves) {
        // r*cos() + a
        float x = o.RevolutionRadius.x*cos(glm::radians(o.RevolutionSpeed)) + o.RevolutionCenter.x;
        float y;
        if(o.OrbitalInclination == 0) {
            y = 0.0;
        }
        else {
            y = sin(glm::radians(o.RevolutionSpeed))/o.OrbitalInclination;
        }
        // r*sin() + b
        float z = o.RevolutionRadius.y*sin(glm::radians(o.RevolutionSpeed)) + o.RevolutionCenter.z;
        o.Position = glm::vec3(x, y, z);
    }
}
//------------------------
// setting up the data and vertices to draw the ISS
//------------------------
unsigned setUpTheISS() {
    std::vector<float> vertices = {
//            position           texture
        -0.8f, -0.4f, 0.0f,    0.0f, 0.0f,  // 0
         0.8f, -0.4f, 0.0f,    1.0f, 0.0f,  // 1
        -0.8f,  0.4f, 0.0f,    0.0f, 1.0f,  // 3

         0.8f, -0.4f, 0.0f,    1.0f, 0.0f,  // 1
         0.8f,  0.4f, 0.0f,    1.0f, 1.0f,  // 2
        -0.8f,  0.4f, 0.0f,    0.0f, 1.0f   // 3
    };

    unsigned issVBO, issVAO;

    glGenBuffers(1, &issVBO);
    glGenVertexArrays(1, &issVAO);

    glBindVertexArray(issVAO);

    glBindBuffer(GL_ARRAY_BUFFER, issVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // describing positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //describing texture
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    return issVAO;
}
//------------------------
// setting up the data and vertices to draw the skybox
//------------------------
unsigned setUpTheSkybox() {
    std::vector<float> vertices = {
            // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    return skyboxVAO;
}
//------------------------
// loading a 2D texture from resources/textures
//------------------------
unsigned loadTexture(const char* path) {
    unsigned tex0;
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if(data) {
        glGenTextures(1, &tex0);
        glBindTexture(GL_TEXTURE_2D, tex0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLenum format;
        switch(nrChannels) {
            case 1: {
                format = GL_RED;
            }break;
            case 3: {
                format = GL_RGB;
            }break;
            case 4: {
                format = GL_RGBA;
            }break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture!\n";
    }
    stbi_image_free(data);

    return tex0;
}
//------------------------
// loading a 2D texture for the skybox from resources/textures
//------------------------
unsigned loadSkybox(std::vector<std::string> &faces) {
    unsigned textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
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

void renderDepthScene(Shader &s, std::vector<Orb> &os, std::vector<Model> &ms, PointLight &l) {
    for(unsigned i = 0; i < os.size(); i++) {
        os[i].RotationSpeed = glfwGetTime();
        os[i].RevolutionSpeed = glfwGetTime();
        setUpOrbData(os[i], s, l, true);
        ms[i].Draw(s);
    }
}