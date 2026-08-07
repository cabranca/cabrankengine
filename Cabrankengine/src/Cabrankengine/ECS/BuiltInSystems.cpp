#include <pch.h>
#include "BuiltInSystems.h"

#include "Components.h"
#include <Cabrankengine/Core/Input.h>
#include <Cabrankengine/Renderer/Renderer2D.h>
#include <Cabrankengine/Renderer/TextRenderer.h>

namespace cbk::ecs {

	using namespace math;
	using namespace rendering;
	using namespace scene;

	void CameraSystem::update(Registry& reg, float dt) {
		for (auto& e: m_Entities) {
			auto transform = reg.getComponent<CTransform>(e).value();
			auto camera = reg.getComponent<CCamera>(e).value();
			if (m_WindowResized) {
				camera->AspectRatio = m_AspectRatio;
				m_WindowResized = false;
			}
			if (camera->IsActive) {
				Mat4 projectionMatrix;
				float halfHeight = 0.f, halfWidth = 0.f;
				switch (camera->Type) {
					case ProjectionType::Orthographic:
						halfHeight = camera->OrthoSize / 2.f;
						halfWidth = halfHeight * camera->AspectRatio;
						projectionMatrix = math::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, camera->Near, camera->Far);
						break;
					case ProjectionType::Perspective:
						projectionMatrix = math::perspective(camera->FovY, camera->AspectRatio, camera->Near, camera->Far);
						break;
				}
				auto viewMatrix = inverseAffine(fromTransform(transform->Position, transform->Rotation, transform->Scale));
				m_ViewProjectionMatrix = viewMatrix * projectionMatrix;
				m_CameraPos = transform->Position;
				break;
			}
		}
	}

	const Mat4& CameraSystem::getViewProjectionMatrix() const {
		return m_ViewProjectionMatrix;
	}

	const Vector3& CameraSystem::getCameraWorldPosition() const {
		return m_CameraPos;
	}

	bool CameraSystem::onWindowResize(WindowResizeEvent& e) {
		m_AspectRatio = static_cast<float>(e.getWidth()) / e.getHeight();
		m_WindowResized = true;
		return true;
	}

	void CameraControllerSystem::update(Registry& reg, float dt) {
		for (auto& e: m_Entities) {
			auto transform = reg.getComponent<CTransform>(e).value();
			auto controller = reg.getComponent<CCameraController>(e).value();

			auto [mouseX, mouseY] = Input::getMousePosition();
			const bool captureMouse = Input::isMouseButtonPressed(Mouse::ButtonRight);

			if (captureMouse) {
				Input::setInputMode(true, true);

				if (controller->MouseCaptured) {
					const float deltaX = mouseX - controller->LastMouseX;
					const float deltaY = mouseY - controller->LastMouseY;

					controller->Yaw -= deltaX * controller->MouseSensitivity;
					controller->Pitch -= deltaY * controller->MouseSensitivity;
					controller->Pitch = std::clamp(controller->Pitch, -89.0f, 89.0f);
				}

				controller->MouseCaptured = true;
			} else {
				Input::setInputMode(false, false);
				controller->MouseCaptured = false;
			}

			controller->LastMouseX = mouseX;
			controller->LastMouseY = mouseY;

			const float yawRadians = radians(controller->Yaw);
			const float pitchRadians = radians(controller->Pitch);

			Vector3 forward{ -std::sin(yawRadians) * std::cos(pitchRadians), std::sin(pitchRadians),
				             -std::cos(yawRadians) * std::cos(pitchRadians) };
			forward.normalized();

			// In free-flight, W/D follow the full look direction; otherwise movement
			// is projected onto the horizontal plane (FPS-style).
			Vector3 moveForward;
			if (controller->FreeFlight) {
				moveForward = forward;
			} else {
				Vector3 flatForward{ forward.x, 0.f, forward.z };
				flatForward.normalized();
				if (flatForward.lengthSquared() == 0.f)
					flatForward = Vector3::Forward;
				moveForward = flatForward;
			}

			const Vector3 moveRight = cross(moveForward, Vector3::Up).normalize();

			Vector3 movement{};

			if (Input::isKeyPressed(Key::W))
				movement += moveForward;
			if (Input::isKeyPressed(Key::S))
				movement -= moveForward;
			if (Input::isKeyPressed(Key::A))
				movement -= moveRight;
			if (Input::isKeyPressed(Key::D))
				movement += moveRight;
			if (Input::isKeyPressed(Key::Space))
				movement += Vector3::Up;
			if (Input::isKeyPressed(Key::LeftShift))
				movement += Vector3::Down;

			if (movement.lengthSquared() > 0.f) {
				movement.normalized();
				transform->Position += movement * controller->TranslationSpeed * dt;
			}

			transform->Rotation = { controller->Pitch, controller->Yaw, 0.f };
		}
	}

	void SpriteRenderSystem::update(Registry& reg, float dt) {
		for (auto& e: m_Entities) {
			auto transform = reg.getComponent<CTransform>(e).value();
			auto sprite = reg.getComponent<CSprite>(e).value();
			auto pos = transform->Position;
			math::Vector3 size{ transform->Scale };
			math::Vector3 rot{ transform->Rotation };
			rendering::Renderer2D::drawRotatedQuad(pos, { size.x, size.y }, rot.z, sprite->Texture, sprite->TilingFactor, sprite->Tint);
		}
	}

	void ModelRenderSystem::update(Registry& reg, float dt) {
		for (auto& e: m_Entities) {
			auto transform = reg.getComponent<CTransform>(e).value();
			auto model = reg.getComponent<CModel>(e).value();
			if (model->Res)
				model->Res->draw(fromTransform(transform->Position, transform->Rotation, transform->Scale));
			else
				CBK_CORE_DEBUG("Entity with id {0} doesn't have a set resource", e);
		}
	}

	void TextRenderSystem::update(Registry& reg, float dt) {
		for (auto& e: m_Entities) {
			auto transform = reg.getComponent<CTransform>(e).value();
			auto text = reg.getComponent<CText>(e).value();
			auto pos = transform->Position;
			math::Vector3 size{ transform->Scale };
			math::Vector3 rot{ transform->Rotation };
			TextRenderer::drawText(text->Text, transform->Position, text->FontScale, text->Color);
		}
	}

	void initDefaultSystems(Registry& reg) {
		reg.registerSystem<CameraSystem>();
		reg.registerSystem<CameraControllerSystem>();
		reg.registerSystem<DirectionalLightSystem>();
		reg.registerSystem<PointLightSystem>();
		reg.registerSystem<SpriteRenderSystem>();
		reg.registerSystem<ModelRenderSystem>();
		reg.registerSystem<TextRenderSystem>();

		Signature sig;
		sig.set(reg.getComponentType<CTransform>());
		sig.set(reg.getComponentType<CCamera>());
		reg.setSystemSignature<CameraSystem>(sig);

		sig.reset();
		sig.set(reg.getComponentType<CTransform>());
		sig.set(reg.getComponentType<CCameraController>());
		reg.setSystemSignature<CameraControllerSystem>(sig);

		sig.reset();
		sig.set(reg.getComponentType<CDirectionalLight>());
		reg.setSystemSignature<DirectionalLightSystem>(sig);

		sig.reset();
		sig.set(reg.getComponentType<CTransform>());
		sig.set(reg.getComponentType<CPointLight>());
		reg.setSystemSignature<PointLightSystem>(sig);

		sig.reset();
		sig.set(reg.getComponentType<CTransform>());
		sig.set(reg.getComponentType<CSprite>());
		reg.setSystemSignature<SpriteRenderSystem>(sig);

		sig.reset();
		sig.set(reg.getComponentType<CTransform>());
		sig.set(reg.getComponentType<CModel>());
		reg.setSystemSignature<ModelRenderSystem>(sig);

		sig.reset();
		sig.set(reg.getComponentType<CTransform>());
		sig.set(reg.getComponentType<CText>());
		reg.setSystemSignature<TextRenderSystem>(sig);
	}
} // namespace cbk::ecs