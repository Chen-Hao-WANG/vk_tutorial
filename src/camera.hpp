#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// A very simple Camera struct with public members
struct Camera {
    // 1. Attributes
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;

    // Euler Angles (in degrees)
    float yaw;
    float pitch;

    // Options
    float speed;
    float sensitivity;

    // 2. Connector
    Camera() {
        // Initial defaults matching your previous hardcoded values
        pos   = glm::vec3(0.0f, -4.0f, 2.0f);  // Back 4 units in Y
        front = glm::vec3(0.0f, 1.0f, 0.0f);   // Looking towards +Y
        up    = glm::vec3(0.0f, 0.0f, 1.0f);   // Z is Up

        yaw         = 90.0f;  // 90 degrees points towards +Y in standard trig
        pitch       = 0.0f;
        speed       = 0.05f;
        sensitivity = 0.1f;
    }

    // 3. Main Function: Get the View Matrix for the UBO
    glm::mat4 getViewMatrix() { return glm::lookAt(pos, pos + front, up); }

    // 4. Update Direction (Call this when mouse moves)
    void rotate(float dx, float dy) {
        yaw += dx * sensitivity;
        pitch -= dy * sensitivity;  // Subtract dy because moving mouse up (negative y-motion) should pitch up (positive angle)

        // Constrain pitch
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        // Calculate new Front vector (Z-Up Coordinate System)
        glm::vec3 newFront;
        // Basic trig converted for Z-up:
        // Yaw rotates around Z, Pitch rotates up/down from XY plane
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.z = sin(glm::radians(pitch));

        front = glm::normalize(newFront);
    }

    // 5. Basic Movement
    void moveForward() { pos += speed * front; }
    void moveBackward() { pos -= speed * front; }
    void moveLeft() { pos -= glm::normalize(glm::cross(front, up)) * speed; }
    void moveRight() { pos += glm::normalize(glm::cross(front, up)) * speed; }
    void moveUp() { pos += speed * up; }
    void moveDown() { pos -= speed * up; }
};
