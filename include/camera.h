#ifndef SOLAR_SYSTEM_CAMERA_H
#define SOLAR_SYSTEM_CAMERA_H


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

enum Direction {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera {
    void updateCameraVectors() {
        float x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        float y = sin(glm::radians(pitch));
        float z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
        Front = glm::normalize(glm::vec3(x, y, z));
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
public:
    // default values
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 20.0f);
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right = glm::normalize(glm::cross(Front, Up));

    float cameraSpeed = 0.3f;
    float mouseSensitivity = 0.2f;
    float yaw = -90.0f;
    float pitch = 0.0f;
    float Zoom = 45.0f;

    Camera() {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void processKeyboard(Direction direction) {
        switch (direction) {
            case FORWARD: {
                Position += cameraSpeed * Front;
            }
                break;
            case BACKWARD: {
                Position -= cameraSpeed * Front;
            }
                break;
            case RIGHT: {
                Position += cameraSpeed * Right;
            }
                break;
            case LEFT: {
                Position -= cameraSpeed * Right;
            }break;
        }
    }

    void processMouse(double xOffset, double yOffset, bool constrainPitch = true) {
        xOffset *= mouseSensitivity;
        yOffset *= mouseSensitivity;

        yaw += xOffset;
        pitch += yOffset;

        if(constrainPitch) {
            pitch = std::min(pitch, 90.0f);
            pitch = std::max(pitch, -90.0f);
        }

        updateCameraVectors();
    }

    void processScroll(double yOffset) {
        Zoom -= (float)yOffset;
        if(Zoom < 1.0f) {
            Zoom = 1.0f;
        }
        if(Zoom > 45.0f) {
            Zoom = 45.0f;
        }
    }
};

#endif