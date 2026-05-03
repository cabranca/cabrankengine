#include <pch.h>
#include "EditorCamera.h"

#include <Cabrankengine/Core/Input.h>
#include <Cabrankengine/Math/Common.h>
#include <Cabrankengine/Math/MatrixFactory.h>

namespace cbk::rendering {

using namespace math;

void EditorCamera::update(float dt) {
    auto [mouseX, mouseY] = Input::getMousePosition();
    const bool captureMouse = Input::isMouseButtonPressed(Mouse::ButtonRight);

    if (captureMouse) {
        Input::setInputMode(true, true);
        if (m_MouseCaptured) {
            Yaw   -= (mouseX - m_LastMouseX) * MouseSensitivity;
            Pitch -= (mouseY - m_LastMouseY) * MouseSensitivity;
            Pitch  = std::clamp(Pitch, -89.f, 89.f);
        }
        m_MouseCaptured = true;
    } else {
        Input::setInputMode(false, false);
        m_MouseCaptured = false;
    }

    m_LastMouseX = mouseX;
    m_LastMouseY = mouseY;

    const float yawRad   = radians(Yaw);
    const float pitchRad = radians(Pitch);

    Vector3 forward{ -std::sin(yawRad) * std::cos(pitchRad),
                      std::sin(pitchRad),
                     -std::cos(yawRad) * std::cos(pitchRad) };
    forward.normalized();

    Vector3 flatForward{ forward.x, 0.f, forward.z };
    flatForward.normalized();
    if (flatForward.lengthSquared() == 0.f)
        flatForward = Vector3::Forward;

    const Vector3 flatRight = cross(flatForward, Vector3::Up).normalize();

    Vector3 movement{};
    if (Input::isKeyPressed(Key::W))            movement += flatForward;
    if (Input::isKeyPressed(Key::S))            movement -= flatForward;
    if (Input::isKeyPressed(Key::A))            movement -= flatRight;
    if (Input::isKeyPressed(Key::D))            movement += flatRight;
    if (Input::isKeyPressed(Key::Space))        movement += Vector3::Up;
    if (Input::isKeyPressed(Key::LeftShift))    movement += Vector3::Down;

    if (movement.lengthSquared() > 0.f) {
        movement.normalized();
        Position += movement * TranslationSpeed * dt;
    }

    if (Input::isKeyPressed(Key::E)) Roll -= RollSpeed * dt;
    if (Input::isKeyPressed(Key::Q)) Roll += RollSpeed * dt;

    recomputeVP();
}

void EditorCamera::onWindowResize(float aspectRatio) {
    AspectRatio = aspectRatio;
    recomputeVP();
}

const Mat4& EditorCamera::getViewProjectionMatrix() const {
    return m_ViewProjectionMatrix;
}

const Vector3& EditorCamera::getWorldPosition() const {
    return Position;
}

void EditorCamera::recomputeVP() {
    const Vector3 rotation{ Pitch, Yaw, Roll };
    const Mat4 view = inverseAffine(fromTransform(Position, rotation, Vector3::One));
    const Mat4 proj = perspective(FovY, AspectRatio, Near, Far);
    m_ViewProjectionMatrix = view * proj;
}

} // namespace cbk::rendering
