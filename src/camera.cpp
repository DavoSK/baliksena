//
// Created by david on 24. 4. 2021.
//

#include "camera.hpp"
#include "input.hpp"

glm::mat4 Camera::createProjMatrix(int width, int height, float fov, float near, float far) {
    Aspect = (float)width / (float)height;
    Near = near;
    Far = far;
    Fov = fov;
    mProjMatrix = glm::perspectiveLH(glm::radians(getFOV()), Aspect, Near, Far);
    mProjSkyboxMatrix = glm::perspectiveLH(glm::radians(Fov), (float)width / (float)height, 0.01f, 2000.0f);

    updateCameraVectors();
    updateFrustum();

    return mProjMatrix;
}

void Camera::debugRender() {
    Renderer::immBegin();
    Renderer::immSetColor({1.0f, 1.0f, 1.0f});
  
    glm::mat4 inv = glm::inverse(getMatrix());
    float halfHeight = tanf(glm::radians(Fov / 2.f));
    float halfWidth = halfHeight * Aspect;

    float xn = halfWidth * Near;
    float xf = halfWidth * Far;
    float yn = halfHeight * Near;
    float yf = halfHeight * Far;

    glm::vec4 f[8u] =
    {
        // near face
        {xn, yn,    Near, 1.f},
        {-xn, yn,   Near, 1.f},
        {xn, -yn,   Near, 1.f},
        {-xn, -yn,  Near, 1.f},

        // far face
        {xf, yf,    Far, 1.f},
        {-xf, yf,   Far, 1.f},
        {xf, -yf,   Far, 1.f},
        {-xf, -yf,  Far, 1.f},
    };

    glm::vec3 v[8];
    for (int i = 0; i < 8; i++)
    {
        glm::vec4 ff = inv * f[i];
        v[i].x = ff.x / ff.w;
        v[i].y = ff.y / ff.w;
        v[i].z = ff.z / ff.w;
    }

    Renderer::immRenderLine(v[0], v[1]);
    Renderer::immRenderLine(v[0], v[2]);
    Renderer::immRenderLine(v[3], v[1]);
    Renderer::immRenderLine(v[3], v[2]);

    Renderer::immRenderLine(v[4], v[5]);
    Renderer::immRenderLine(v[4], v[6]);
    Renderer::immRenderLine(v[7], v[5]);
    Renderer::immRenderLine(v[7], v[6]);

    Renderer::immRenderLine(v[0], v[4]);
    Renderer::immRenderLine(v[1], v[5]);
    Renderer::immRenderLine(v[3], v[7]);
    Renderer::immRenderLine(v[2], v[6]);
    
    Renderer::immEnd();
}

void Camera::update(float deltaTime) {
    const auto velocity = MovementSpeed * deltaTime;
    mPos += mPosDelta.x * (Front * velocity);
    mPos += mPosDelta.y * (Up * velocity);
    mPos += mPosDelta.z * (Right * velocity);

    Yaw -= mDirDelta.x * MouseSensitivity;
    Pitch -= mDirDelta.y * MouseSensitivity;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    updateCameraVectors();
    updateFrustum();
    setMatrix(glm::lookAtLH(mPos, mPos + Front, Up));
}

void Camera::updateFrustum() {
    mFrustum.update(mProjMatrix * mTransform);
}

void Camera::updateCameraVectors() {
    const glm::vec3 front = {cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
                             sin(glm::radians(Pitch)),
                             sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))};
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
