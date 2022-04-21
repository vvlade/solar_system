#include <iostream>
#include <cmath>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image.h"

#include "shader.h"
#include "camera.h"
#include "model.h"


struct Orb {
    const glm::vec3 LightPosition = glm::vec3(0.0f);
    glm::vec3 Position = glm::vec3(1.0f);
    glm::vec3 Ambient = glm::vec3(0.1f);
    glm::vec3 Diffuse = glm::vec3(0.6f);
    glm::vec3 Specular = glm::vec3(0.0f);
    glm::vec3 RotationAngle;                                    // axial tilt
    glm::vec3 RevolutionCenter = glm::vec3(0.0f);         // the point around which the orb revolves
    glm::vec3 Size;
    glm::vec2 RevolutionRadius;
    float OrbitalInclination;
    float RotationSpeed;
    float RevolutionSpeed;
    bool Revolve = true;                                        // does the orb revolve around something
};

struct PointLight {
    float Const;
    float Linear;
    float Quadratic;
};

Camera cam;

int SCR_WIDTH = 800, SCR_HEIGHT = 600;
float prevX = SCR_WIDTH / 2.0;
float prevY = SCR_HEIGHT / 2.0;

glm::vec3 viewPos = cam.Position;


// callbacks and other functions
void processInput(GLFWwindow* window);
void cursorPositionCallback(GLFWwindow* window, double posX, double posY);
void scrollCallback(GLFWwindow* window, double offsetX, double offsetY);
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void setUpPlanetData(Orb& o, Shader& s, PointLight& pl);

int main() {
    // init
    //------------------------
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

    // flipping loaded textures on the y axis
    //stbi_set_flip_vertically_on_load(true);


    // callbacks
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glEnable(GL_DEPTH_TEST);

    // shaders
    Shader shader("resources/shaders/someVS.vs", "resources/shaders/someFS.fs");

    // models
    //resources/objects/Sunn/Sun.max
    //
    Model sunModel("resources/objects/Sun/Sun.obj");
    sunModel.SetShaderTextureNamePrefix("material.");

    Model moonModel("resources/objects/Moon/Moon.obj");
    moonModel.SetShaderTextureNamePrefix("material.");


    //resources/objects/11-1earth-made-by-elite3d/1Earth-made-by-ELITE3d/Model/Globe.obj
    Model earthModel("resources/objects/Earth/Earth.obj");
    earthModel.SetShaderTextureNamePrefix("material.");

    Model mercuryModel("resources/objects/Mercury/65-mercury_1k/Mercury 1K.obj");
    mercuryModel.SetShaderTextureNamePrefix("material.");

    // FIXME: find a model
    //Model venusModel("resources/objects/planet-venus/source/model/venus.blend");
    //venusModel.SetShaderTextureNamePrefix("material.");

    Model marsModel("resources/objects/Mars-Photorealistic-3D-Model/Mars 2K.obj");
    marsModel.SetShaderTextureNamePrefix("material.");

    // setting up light properties
    // ------------------------------------
    PointLight pl;
    pl.Const = 1.0f;
    pl.Linear = 0.0014f;
    pl.Quadratic = 0.0000007f;


    // setting up the planets and suns data
    // SUN
    //---------------------------------------
    Orb sun;
    sun.Position = glm::vec3(0.0f);
    sun.Ambient = glm::vec3(1.0f);
    sun.Diffuse = glm::vec3(1.0f);
    sun.Specular = glm::vec3(0.0f);
    sun.RotationAngle = glm::vec3(0.08f, 1.0f, 0.0f);
    sun.Size = glm::vec3(0.5f);
    sun.OrbitalInclination = 0.0f;
    // MERCURY
    //---------------------------------------
    Orb mercury;
    mercury.Position = glm::vec3(15.0969, 0.0, 0.0);
    mercury.RotationAngle = glm::vec3(0.02, 1.0, 0.0);
    mercury.RevolutionRadius = glm::vec2(mercury.Position.x);
    mercury.Size = glm::vec3(1.0);
    mercury.OrbitalInclination = 2;
    // VENUS
    //---------------------------------------
    /*Orb venus;
    venus.Position = glm::vec3(28.2097, 0.0f, 0.0f);
    venus.RotationAngle = glm::vec3(1.971111111, 1.0f, 0.0f);
    venus.RevolutionRadius = glm::vec2(venus.Position.x);
    venus.Size = glm::vec3(1.0);
    venus.OrbitalInclination = 1;*/
    // EARTH
    //---------------------------------------
    Orb earth;
    earth.Position = glm::vec3(39.0f, 0.0f, 0.0f);
    earth.RotationAngle = glm::vec3(0.2611, 1.0f, 0.0f);
    earth.RevolutionRadius = glm::vec2(earth.Position.x);
    earth.Size = glm::vec3(0.04f);
    earth.OrbitalInclination = 0.0f;
    // MOON
    //---------------------------------------
    Orb moon;
    moon.Position = glm::vec3(earth.Position.x+7, earth.Position.y, earth.Position.z);
    moon.RotationAngle = glm::vec3(0.017f, 1.0f, 0.0f);
    moon.RevolutionRadius = glm::vec2(3.0f);
    moon.Size = glm::vec3(0.01);
    moon.OrbitalInclination = 2.4;
    // MARS
    //---------------------------------------
    Orb mars;
    mars.Position = glm::vec3(59.5647, 0.0, 0.0);
    mars.RotationAngle = glm::vec3(0.277777778, 1.0, 0.0);
    mars.RevolutionRadius = glm::vec2(mars.Position.x);
    mars.Size = glm::vec3(0.02);
    mars.OrbitalInclination = 0.0;

    // rendering loop
    //------------------------
    while(!glfwWindowShouldClose(window)) {
        // poll events
        //------------------------

        glfwPollEvents();
        processInput(window);

        // update world state
        //------------------------
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // setting up the backpack
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(cam.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);

        shader.setMat4("view", cam.getViewMatrix());



        // SUN
        //---------------------------------------------------------
        sun.RotationSpeed = glfwGetTime()*6.759259259;
        sun.Revolve = false;
        setUpPlanetData(sun, shader, pl);
        sunModel.Draw(shader);


        // MERCURY
        //---------------------------------------------------------
        mercury.RotationSpeed = glfwGetTime()*3.093220339;
        mercury.RevolutionSpeed = glfwGetTime()*2.074570876;
        mercury.RevolutionCenter = sun.Position;
        setUpPlanetData(mercury, shader, pl);
        mercuryModel.Draw(shader);

        // VENUS
        //---------------------------------------------------------
        /*venus.RotationSpeed = glfwGetTime()*0.718538647;
        venus.RevolutionSpeed = glfwGetTime();
        venus.RevolutionCenter = sun.Position;
        setUpPlanetData(venus, shader, pl);
        venusModel.Draw(shader);*/

        // EARTH
        //---------------------------------------------------------
        earth.RotationSpeed = glfwGetTime()*182.5;
        earth.RevolutionSpeed = glfwGetTime()*0.5;
        setUpPlanetData(earth, shader, pl);
        earthModel.Draw(shader);


        // MOON
        //---------------------------------------------------------
        moon.RotationSpeed = glfwGetTime()*(-6.684981685);
        moon.RevolutionSpeed = glfwGetTime()*6.186440678;
        moon.RevolutionCenter = earth.Position;
        setUpPlanetData(moon, shader, pl);
        moonModel.Draw(shader);

        // MARS
        //---------------------------------------------------------
        mars.RotationSpeed = glfwGetTime()*182.5;
        mars.RevolutionSpeed = glfwGetTime()*0.25;
        mars.RevolutionCenter = sun.Position;
        setUpPlanetData(mars, shader, pl);
        marsModel.Draw(shader);


        // render image
        //------------------------
        glfwSwapBuffers(window);
    }

    // de-init
    //------------------------
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

    cam.cameraSpeed = 0.1f;
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
// setting up the shader to draw planets and the sun
void setUpPlanetData(Orb& o, Shader& s, PointLight& pl) {
    s.setVec3("light.position", o.LightPosition);

    s.setVec3("light.ambient", o.Ambient);
    s.setVec3("light.diffuse", o.Diffuse);
    s.setVec3("light.specular", o.Specular);
    s.setFloat("light.constant", pl.Const);
    s.setFloat("light.linear", pl.Linear);
    s.setFloat("light.quadratic", pl.Quadratic);

    s.setFloat("material.shininess", 32.0f);

    s.setVec3("ViewPos", cam.Position);

    // transformations
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, o.Position);
    model = glm::rotate(model, glm::radians(o.RotationSpeed), o.RotationAngle);
    model = glm::scale(model, o.Size);
    s.setMat4("model", model);
    if(o.Revolve) {
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