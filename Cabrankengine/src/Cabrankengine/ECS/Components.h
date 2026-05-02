#pragma once

#include <cstdint>

#include <Cabrankengine/Math/VecTraits.h>
#include <Cabrankengine/Renderer/Materials/PBRMaterial.h>
#include <Cabrankengine/Renderer/Materials/PhongMaterial.h>
#include <Cabrankengine/Renderer/Texture.h>
#include <Cabrankengine/Scene/Model.h>

namespace cbk::ecs {

	struct CTransform {
		math::Vector3 Position;
		math::Vector3 Rotation;
		math::Vector3 Scale{ 1.f };
	};

	enum class ProjectionType : uint8_t { Perspective, Orthographic };

	// Can this be done with two components? Variants?
	struct CCamera {
		ProjectionType Type = ProjectionType::Perspective;
		bool IsActive = true;

		float FovY = cbk::math::PI / 4.f;
		float AspectRatio = 16.f / 9.f;
		float Near = 0.1f;
		float Far = 100.f;

		float OrthoSize = 900.f;
	};

	struct CCameraController {
		float TranslationSpeed = 10.f;
		float MouseSensitivity = 0.1f;
		float Yaw = 0.f;
		float Pitch = 0.f;
		float LastMouseX = 0.f;
		float LastMouseY = 0.f;
		bool MouseCaptured = false;
	};

	struct CSprite {
		std::string Path;
		Ref<rendering::Texture2D> Texture;
		math::Vector4 Tint{ 1.f };
		float TilingFactor{ 1.f };
	};

	struct CPhongModel {
		std::string Path;
		Ref<scene::Model<rendering::PhongMaterial>> Res;
	};

	struct CPBRModel {
		std::string Path;
		Ref<scene::Model<rendering::PBRMaterial>> Res;
	};

	struct CText {
		std::string Text;
		float FontScale = 0.05f;
		math::Vector4 Color{ 1.f };
	};

	struct CDirectionalLight {
		math::Vector3 Direction{ 0.f, -1.f, 0.f };
		math::Vector3 Radiance{ 1.f };
	};

	struct CPointLight {
		math::Vector3 Radiance{ 1.f };

		float Constant{ 1.f };
		float Linear{ 0.09f };
		float Quadratic{ 0.032f };
	};

	// Bitmask-based collision filter: category says what I am, mask says what I collide with
	struct CCollisionFilter {
		uint32_t CategoryBits = 0x0001;
		uint32_t MaskBits = 0xFFFFFFFF;
	};

	// Axis-Aligned Bounding Box — centered, stores half-extents
	template <typename VecType>
	struct CAABBCollider {
		VecType HalfExtents{ 0.5f };

		CAABBCollider() = default;
		explicit CAABBCollider(const VecType& size) : HalfExtents(size / 2.0f) {}
	};

	// Kept for backwards compatibility
	template <typename VecType>
	using CCollisionBox = CAABBCollider<VecType>;

	// Sphere (2D circle) collider — centered, stores radius
	template <typename VecType>
	struct CSphereCollider {
		float Radius = 0.5f;

		CSphereCollider() = default;
		explicit CSphereCollider(float radius) : Radius(radius) {}
	};

	// Oriented Bounding Box — centered, stores half-extents + orientation
	// 2D: orientation is a float angle in radians
	// 3D: orientation is a Quaternion
	template <typename VecType>
	struct COBBCollider {
		VecType HalfExtents{ 0.5f };
		typename math::VecTraits<VecType>::OrientationType Orientation{};

		COBBCollider() = default;
		COBBCollider(const VecType& halfExtents, const typename math::VecTraits<VecType>::OrientationType& orientation)
		    : HalfExtents(halfExtents), Orientation(orientation) {}
	};

	// Plane (2D line) collider — defined by normal and signed distance from origin
	template <typename VecType>
	struct CPlaneCollider {
		VecType Normal{};
		float Distance = 0.0f;

		CPlaneCollider() = default;
		CPlaneCollider(const VecType& normal, float distance) : Normal(normal), Distance(distance) {}
	};

	// Capsule collider — centered, defined by normalized direction, half-height along that direction, and radius
	template <typename VecType>
	struct CCapsuleCollider {
		VecType Direction{}; // Normalized axis of the capsule spine
		float HalfHeight = 0.5f;
		float Radius = 0.25f;

		CCapsuleCollider() = default;
		CCapsuleCollider(const VecType& direction, float halfHeight, float radius)
		    : Direction(direction), HalfHeight(halfHeight), Radius(radius) {}
	};

	// Y-axis-aligned cylinder collider (3D only) — centered, stores half-height and radius
	struct CCylinderCollider {
		float HalfHeight = 0.5f;
		float Radius = 0.5f;

		CCylinderCollider() = default;
		CCylinderCollider(float halfHeight, float radius) : HalfHeight(halfHeight), Radius(radius) {}
	};

} // namespace cbk::ecs
