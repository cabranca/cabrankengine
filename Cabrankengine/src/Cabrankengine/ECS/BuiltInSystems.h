#pragma once

#include <Cabrankengine/Events/ApplicationEvent.h>
#include <Cabrankengine/Math/Mat4.h>

#include "Registry.hpp"
#include "SystemManager.hpp"

namespace cbk::ecs {

	class CameraSystem : public ISystem {
	  public:
		void update(Registry& registry, float dt) override;

		const math::Mat4& getViewProjectionMatrix() const;
		const math::Vector3& getCameraWorldPosition() const;

		bool onWindowResize(WindowResizeEvent& e);

	  private:
		math::Mat4 m_ViewProjectionMatrix;
		math::Vector3 m_CameraPos;
		bool m_WindowResized = false;
		float m_AspectRatio = 0.f;
	};

	class CameraControllerSystem : public ISystem {
	  public:
		void update(Registry& reg, float dt) override;
	};

	class DirectionalLightSystem : public ISystem {
	  public:
		void update(Registry& reg, float dt) override {}
	};

	class PointLightSystem : public ISystem {
	  public:
		void update(Registry& reg, float dt) override {}
	};

	class SpriteRenderSystem : public ISystem {
	  public:
		void update(Registry& reg, float dt) override;
	};

	class PhongRenderSystem : public ISystem {
	  public:
		void update(Registry& reg, float dt) override;
	};

	class PBRRenderSystem : public ISystem {
	  public:
		void update(Registry& reg, float dt) override;
	};

	class TextRenderSystem : public ISystem {
	  public:
		void update(Registry& reg, float dt) override;
	};

	void initDefaultSystems(Registry& reg);
} // namespace cbk::ecs
