#pragma once

#include <Common/Math/Mat4.h>
#include <Common/Math/Vector3.h>

namespace cbk::rendering {

	class EditorCamera {
	  public:
		void update(float dt);
		void onWindowResize(float aspectRatio);

		[[nodiscard]] const math::Mat4& getViewProjectionMatrix() const;
		[[nodiscard]] const math::Vector3& getWorldPosition() const;

		math::Vector3 Position = {};
		float Yaw = 0.f;
		float Pitch = 0.f;
		float TranslationSpeed = 10.f;
		float MouseSensitivity = 0.1f;
		float Roll = 0.f;
		float RollSpeed = 45.f;
		float FovY = math::k_Pi / 4.f;
		float Near = 0.1f;
		float Far = 100.f;
		float AspectRatio = 16.f / 9.f;

	  private:
		void recomputeVP();

		float m_LastMouseX = 0.f;
		float m_LastMouseY = 0.f;
		bool m_MouseCaptured = false;
		math::Mat4 m_ViewProjectionMatrix;
	};

} // namespace cbk::rendering
