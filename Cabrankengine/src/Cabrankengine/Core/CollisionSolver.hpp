#pragma once

#include <pch.h>
#include <cmath>
#include <cstdint>
#include <array>
#include <utility>

#include <Common/Math/Common.h>
#include <Common/Math/VecTraits.h>

namespace cbk {

	namespace collision_solver {

		// ============================================================
		// Result type
		// ============================================================

		template <typename VecType>
		struct Collision {
			VecType normal;    // Direction of highest penetration value from the first object to the second
			float penetration; // How much the objects are colliding
			bool collided;     // True if there is collision
		};

		// ============================================================
		// Bitmask filter
		// ============================================================

		inline constexpr bool shouldCollide(uint32_t catA, uint32_t maskA, uint32_t catB, uint32_t maskB) noexcept {
			return (catA & maskB) != 0 && (catB & maskA) != 0;
		}

		// ============================================================
		// Internal helpers
		// ============================================================

		namespace detail {

			// Dimension count
			template <typename VecType>
			inline constexpr int dimensions() {
				return math::VecTraits<VecType>::Dimensions;
			}

			// Component-wise absolute value
			template <typename VecType>
			inline VecType vecAbs(const VecType& v) {
				VecType result{};
				for (int i = 0; i < dimensions<VecType>(); ++i)
					result[i] = std::abs(v[i]);
				return result;
			}

			// Dot product (loop-based, works for any subscriptable vector)
			template <typename VecType>
			inline float vecDot(const VecType& a, const VecType& b) {
				float sum = 0.0f;
				for (int i = 0; i < dimensions<VecType>(); ++i)
					sum += a[i] * b[i];
				return sum;
			}

			// Closest point on a line segment (center + dir * halfLen) to a point
			template <typename VecType>
			inline VecType closestPointOnSegment(const VecType& segCenter, const VecType& segDir, float segHalfLen, const VecType& point) {
				VecType diff = point - segCenter;
				float t = vecDot(diff, segDir);
				t = std::clamp(t, -segHalfLen, segHalfLen);
				return segCenter + segDir * t;
			}

			// Closest pair of points between two line segments
			template <typename VecType>
			inline std::pair<VecType, VecType> closestPointsSegmentSegment(const VecType& c1, const VecType& d1, float h1,
			                                                               const VecType& c2, const VecType& d2, float h2) {

				VecType r = c1 - c2;
				float a = vecDot(d1, d1);
				float e = vecDot(d2, d2);
				float f = vecDot(d2, r);
				float b = vecDot(d1, d2);
				float c = vecDot(d1, r);

				float denom = a * e - b * b;

				float s, t;
				if (denom < math::EPSILON) {
					s = 0.0f;
					t = f / e;
				} else {
					s = (b * f - c * e) / denom;
					t = (a * f - b * c) / denom;
				}

				s = std::clamp(s, -h1, h1);
				t = std::clamp(t, -h2, h2);

				float tN = vecDot(d2, (c1 + d1 * s) - c2);
				t = std::clamp(tN, -h2, h2);
				float sN = vecDot(d1, (c2 + d2 * t) - c1);
				s = std::clamp(sN, -h1, h1);

				VecType p1 = c1 + d1 * s;
				VecType p2 = c2 + d2 * t;
				return { p1, p2 };
			}

			// Closest point on an AABB to a point
			template <typename VecType>
			inline VecType closestPointOnAABB(const VecType& boxPos, const VecType& boxHalf, const VecType& point) {
				VecType result{};
				for (int i = 0; i < dimensions<VecType>(); ++i) {
					float lo = boxPos[i] - boxHalf[i];
					float hi = boxPos[i] + boxHalf[i];
					result[i] = std::clamp(point[i], lo, hi);
				}
				return result;
			}

			// Get OBB local axes from orientation
			template <typename VecType>
			inline auto getOBBAxes(const typename math::VecTraits<VecType>::OrientationType& orientation) {
				constexpr int N = dimensions<VecType>();
				if constexpr (N == 2) {
					float c = std::cos(orientation);
					float s = std::sin(orientation);
					std::array<VecType, 2> axes{};
					axes[0][0] = c;
					axes[0][1] = s;
					axes[1][0] = -s;
					axes[1][1] = c;
					return axes;
				} else {
					return orientation.toAxes();
				}
			}

			// Transform a point from world space into OBB local space
			template <typename VecType, int N>
			inline VecType worldToOBBLocal(const VecType& point, const VecType& obbPos, const std::array<VecType, N>& axes) {
				VecType diff = point - obbPos;
				VecType local{};
				for (int i = 0; i < N; ++i)
					local[i] = vecDot(diff, axes[i]);
				return local;
			}

			// Transform a direction from OBB local space to world space
			template <typename VecType, int N>
			inline VecType obbLocalToWorldDir(const VecType& localDir, const std::array<VecType, N>& axes) {
				VecType world{};
				for (int i = 0; i < N; ++i) {
					for (int j = 0; j < N; ++j)
						world[j] += localDir[i] * axes[i][j];
				}
				return world;
			}

		} // namespace detail

		// ============================================================
		// AABB vs AABB
		// ============================================================

		template <typename VecType>
		Collision<VecType> AABB(const VecType& pos1, const VecType& halfExtents1, const VecType& pos2, const VecType& halfExtents2) {
			constexpr int N = detail::dimensions<VecType>();
			VecType delta = detail::vecAbs(pos1 - pos2);
			VecType overlap = halfExtents1 + halfExtents2 - delta;

			for (int i = 0; i < N; ++i) {
				if (overlap[i] < 0.0f)
					return { VecType(0.0f), 0.0f, false };
			}

			int minAxis = 0;
			float minOverlap = overlap[0];
			for (int i = 1; i < N; ++i) {
				if (overlap[i] < minOverlap) {
					minOverlap = overlap[i];
					minAxis = i;
				}
			}

			VecType normal(0.0f);
			normal[minAxis] = (pos2[minAxis] > pos1[minAxis]) ? 1.0f : -1.0f;

			return { normal, minOverlap, true };
		}

		// ============================================================
		// Sphere vs Sphere
		// ============================================================

		template <typename VecType>
		Collision<VecType> Sphere(const VecType& pos1, float radius1, const VecType& pos2, float radius2) {
			VecType delta = pos2 - pos1;
			float dist2 = detail::vecDot(delta, delta);
			float radiusSum = radius1 + radius2;

			if (dist2 > radiusSum * radiusSum)
				return { VecType(0.0f), 0.0f, false };

			float dist = std::sqrt(dist2);
			VecType normal = (dist > math::EPSILON) ? (delta / dist) : VecType(0.0f);
			float penetration = radiusSum - dist;

			return { normal, penetration, true };
		}

		// ============================================================
		// Sphere vs AABB
		// ============================================================

		template <typename VecType>
		Collision<VecType> SphereAABB(const VecType& spherePos, float radius, const VecType& boxPos, const VecType& boxHalf) {
			constexpr int N = detail::dimensions<VecType>();
			VecType closestPoint = detail::closestPointOnAABB(boxPos, boxHalf, spherePos);

			VecType delta = spherePos - closestPoint;
			float dist2 = detail::vecDot(delta, delta);

			if (dist2 > radius * radius)
				return { VecType(0.0f), 0.0f, false };

			float dist = std::sqrt(dist2);

			VecType normal{};
			if (dist > math::EPSILON) {
				normal = delta / dist;
			} else {
				VecType overlap = boxHalf - detail::vecAbs(spherePos - boxPos);
				int minAxis = 0;
				float minOverlap = overlap[0];
				for (int i = 1; i < N; ++i) {
					if (overlap[i] < minOverlap) {
						minOverlap = overlap[i];
						minAxis = i;
					}
				}
				normal = VecType(0.0f);
				normal[minAxis] = (spherePos[minAxis] > boxPos[minAxis]) ? 1.0f : -1.0f;
				dist = 0.0f;
			}

			return { normal, radius - dist, true };
		}

		// ============================================================
		// OBB vs OBB  (Separating Axis Theorem)
		// ============================================================

		template <typename VecType>
		Collision<VecType> OBB(const VecType& pos1, const VecType& halfExtents1,
		                       const typename math::VecTraits<VecType>::OrientationType& orient1, const VecType& pos2,
		                       const VecType& halfExtents2, const typename math::VecTraits<VecType>::OrientationType& orient2) {
			constexpr int N = detail::dimensions<VecType>();
			auto axes1 = detail::getOBBAxes<VecType>(orient1);
			auto axes2 = detail::getOBBAxes<VecType>(orient2);

			VecType d = pos2 - pos1;

			float minPen = std::numeric_limits<float>::max();
			VecType minNormal{};

			auto testAxis = [&](const VecType& axis) -> bool {
				float axisLen2 = detail::vecDot(axis, axis);
				if (axisLen2 < math::EPSILON)
					return true;

				float invLen = 1.0f / std::sqrt(axisLen2);
				VecType normAxis{};
				for (int i = 0; i < N; ++i)
					normAxis[i] = axis[i] * invLen;

				float proj1 = 0.0f;
				for (int i = 0; i < N; ++i)
					proj1 += halfExtents1[i] * std::abs(detail::vecDot(axes1[i], normAxis));

				float proj2 = 0.0f;
				for (int i = 0; i < N; ++i)
					proj2 += halfExtents2[i] * std::abs(detail::vecDot(axes2[i], normAxis));

				float dist = std::abs(detail::vecDot(d, normAxis));
				float overlap = proj1 + proj2 - dist;

				if (overlap < 0.0f)
					return false;

				if (overlap < minPen) {
					minPen = overlap;
					minNormal = normAxis;
					if (detail::vecDot(d, normAxis) < 0.0f) {
						for (int i = 0; i < N; ++i)
							minNormal[i] = -minNormal[i];
					}
				}
				return true;
			};

			for (int i = 0; i < N; ++i) {
				if (!testAxis(axes1[i]))
					return { VecType(0.0f), 0.0f, false };
			}

			for (int i = 0; i < N; ++i) {
				if (!testAxis(axes2[i]))
					return { VecType(0.0f), 0.0f, false };
			}

			if constexpr (N == 3) {
				for (int i = 0; i < 3; ++i) {
					for (int j = 0; j < 3; ++j) {
						VecType crossAxis{};
						crossAxis[0] = axes1[i][1] * axes2[j][2] - axes1[i][2] * axes2[j][1];
						crossAxis[1] = axes1[i][2] * axes2[j][0] - axes1[i][0] * axes2[j][2];
						crossAxis[2] = axes1[i][0] * axes2[j][1] - axes1[i][1] * axes2[j][0];
						if (!testAxis(crossAxis))
							return { VecType(0.0f), 0.0f, false };
					}
				}
			}

			return { minNormal, minPen, true };
		}

		// ============================================================
		// OBB vs Sphere
		// ============================================================

		template <typename VecType>
		Collision<VecType> OBBSphere(const VecType& obbPos, const VecType& obbHalf,
		                             const typename math::VecTraits<VecType>::OrientationType& orient, const VecType& spherePos,
		                             float radius) {
			constexpr int N = detail::dimensions<VecType>();
			auto axes = detail::getOBBAxes<VecType>(orient);

			VecType local = detail::worldToOBBLocal<VecType, N>(spherePos, obbPos, axes);

			VecType closest{};
			for (int i = 0; i < N; ++i)
				closest[i] = std::clamp(local[i], -obbHalf[i], obbHalf[i]);

			VecType localDelta = local - closest;
			float dist2 = detail::vecDot(localDelta, localDelta);

			if (dist2 > radius * radius)
				return { VecType(0.0f), 0.0f, false };

			float dist = std::sqrt(dist2);

			VecType normal{};
			if (dist > math::EPSILON) {
				VecType localNorm{};
				for (int i = 0; i < N; ++i)
					localNorm[i] = localDelta[i] / dist;
				normal = detail::obbLocalToWorldDir<VecType, N>(localNorm, axes);
			} else {
				VecType overlap{};
				for (int i = 0; i < N; ++i)
					overlap[i] = obbHalf[i] - std::abs(local[i]);

				int minAxis = 0;
				float minOverlap = overlap[0];
				for (int i = 1; i < N; ++i) {
					if (overlap[i] < minOverlap) {
						minOverlap = overlap[i];
						minAxis = i;
					}
				}
				VecType localNorm(0.0f);
				localNorm[minAxis] = (local[minAxis] > 0.0f) ? 1.0f : -1.0f;
				normal = detail::obbLocalToWorldDir<VecType, N>(localNorm, axes);
				dist = 0.0f;
			}

			return { normal, radius - dist, true };
		}

		// ============================================================
		// OBB vs AABB  (treats AABB as identity-oriented OBB)
		// ============================================================

		template <typename VecType>
		Collision<VecType> OBBAABB(const VecType& obbPos, const VecType& obbHalf,
		                           const typename math::VecTraits<VecType>::OrientationType& orient, const VecType& aabbPos,
		                           const VecType& aabbHalf) {
			typename math::VecTraits<VecType>::OrientationType identity{};
			return OBB(obbPos, obbHalf, orient, aabbPos, aabbHalf, identity);
		}

		// ============================================================
		// Plane vs Sphere
		// ============================================================

		template <typename VecType>
		Collision<VecType> PlaneSphere(const VecType& planeNormal, float planeDist, const VecType& spherePos, float radius) {
			float signedDist = detail::vecDot(planeNormal, spherePos) - planeDist;
			float absDist = std::abs(signedDist);

			if (absDist > radius)
				return { VecType(0.0f), 0.0f, false };

			VecType normal = (signedDist >= 0.0f) ? planeNormal : (planeNormal * -1.0f);
			float penetration = radius - absDist;

			return { normal, penetration, true };
		}

		// ============================================================
		// Plane vs AABB
		// ============================================================

		template <typename VecType>
		Collision<VecType> PlaneAABB(const VecType& planeNormal, float planeDist, const VecType& boxPos, const VecType& boxHalf) {
			constexpr int N = detail::dimensions<VecType>();

			float projRadius = 0.0f;
			for (int i = 0; i < N; ++i)
				projRadius += boxHalf[i] * std::abs(planeNormal[i]);

			float signedDist = detail::vecDot(planeNormal, boxPos) - planeDist;
			float absDist = std::abs(signedDist);

			if (absDist > projRadius)
				return { VecType(0.0f), 0.0f, false };

			VecType normal = (signedDist >= 0.0f) ? planeNormal : (planeNormal * -1.0f);
			float penetration = projRadius - absDist;

			return { normal, penetration, true };
		}

		// ============================================================
		// Plane vs OBB
		// ============================================================

		template <typename VecType>
		Collision<VecType> PlaneOBB(const VecType& planeNormal, float planeDist, const VecType& obbPos, const VecType& obbHalf,
		                            const typename math::VecTraits<VecType>::OrientationType& orient) {
			constexpr int N = detail::dimensions<VecType>();
			auto axes = detail::getOBBAxes<VecType>(orient);

			float projRadius = 0.0f;
			for (int i = 0; i < N; ++i)
				projRadius += obbHalf[i] * std::abs(detail::vecDot(axes[i], planeNormal));

			float signedDist = detail::vecDot(planeNormal, obbPos) - planeDist;
			float absDist = std::abs(signedDist);

			if (absDist > projRadius)
				return { VecType(0.0f), 0.0f, false };

			VecType normal = (signedDist >= 0.0f) ? planeNormal : (planeNormal * -1.0f);
			float penetration = projRadius - absDist;

			return { normal, penetration, true };
		}

		// ============================================================
		// Capsule vs Capsule
		// ============================================================

		template <typename VecType>
		Collision<VecType> CapsuleCapsule(const VecType& pos1, const VecType& dir1, float halfH1, float r1, const VecType& pos2,
		                                  const VecType& dir2, float halfH2, float r2) {
			auto [p1, p2] = detail::closestPointsSegmentSegment(pos1, dir1, halfH1, pos2, dir2, halfH2);
			return Sphere(p1, r1, p2, r2);
		}

		// ============================================================
		// Capsule vs Sphere
		// ============================================================

		template <typename VecType>
		Collision<VecType> CapsuleSphere(const VecType& capPos, const VecType& capDir, float capHalfH, float capRadius,
		                                 const VecType& spherePos, float sphereRadius) {
			VecType closest = detail::closestPointOnSegment(capPos, capDir, capHalfH, spherePos);
			return Sphere(closest, capRadius, spherePos, sphereRadius);
		}

		// ============================================================
		// Capsule vs AABB
		// ============================================================

		template <typename VecType>
		Collision<VecType> CapsuleAABB(const VecType& capPos, const VecType& capDir, float capHalfH, float capRadius, const VecType& boxPos,
		                               const VecType& boxHalf) {
			// Iterative closest-point: find closest spine point to box, then closest box point to spine point
			VecType endA = capPos + capDir * capHalfH;
			VecType endB = capPos - capDir * capHalfH;

			VecType closestA = detail::closestPointOnAABB(boxPos, boxHalf, endA);
			VecType closestB = detail::closestPointOnAABB(boxPos, boxHalf, endB);
			VecType closestC = detail::closestPointOnAABB(boxPos, boxHalf, capPos);

			auto dist2 = [](const VecType& a, const VecType& b) {
				VecType d = a - b;
				return detail::vecDot(d, d);
			};

			float dA = dist2(endA, closestA);
			float dB = dist2(endB, closestB);
			float dC = dist2(capPos, closestC);

			VecType bestBox;
			if (dA <= dB && dA <= dC)
				bestBox = closestA;
			else if (dB <= dC)
				bestBox = closestB;
			else
				bestBox = closestC;

			VecType spineClosest = detail::closestPointOnSegment(capPos, capDir, capHalfH, bestBox);
			VecType boxClosest = detail::closestPointOnAABB(boxPos, boxHalf, spineClosest);

			VecType delta = spineClosest - boxClosest;
			float d2 = detail::vecDot(delta, delta);

			if (d2 > capRadius * capRadius)
				return { VecType(0.0f), 0.0f, false };

			float d = std::sqrt(d2);
			VecType normal = (d > math::EPSILON) ? (delta / d) : VecType(0.0f);

			return { normal, capRadius - d, true };
		}

		// ============================================================
		// Capsule vs Plane
		// ============================================================

		template <typename VecType>
		Collision<VecType> CapsulePlane(const VecType& capPos, const VecType& capDir, float capHalfH, float capRadius,
		                                const VecType& planeNormal, float planeDist) {
			VecType endA = capPos + capDir * capHalfH;
			VecType endB = capPos - capDir * capHalfH;

			float distA = detail::vecDot(planeNormal, endA) - planeDist;
			float distB = detail::vecDot(planeNormal, endB) - planeDist;

			float minDist = (std::abs(distA) < std::abs(distB)) ? distA : distB;
			float absDist = std::abs(minDist);

			if (absDist > capRadius)
				return { VecType(0.0f), 0.0f, false };

			VecType normal = (minDist >= 0.0f) ? planeNormal : (planeNormal * -1.0f);
			float penetration = capRadius - absDist;

			return { normal, penetration, true };
		}

		// ============================================================
		// Capsule vs OBB
		// ============================================================

		template <typename VecType>
		Collision<VecType> CapsuleOBB(const VecType& capPos, const VecType& capDir, float capHalfH, float capRadius, const VecType& obbPos,
		                              const VecType& obbHalf, const typename math::VecTraits<VecType>::OrientationType& orient) {
			constexpr int N = detail::dimensions<VecType>();
			auto axes = detail::getOBBAxes<VecType>(orient);

			VecType localCapPos = detail::worldToOBBLocal<VecType, N>(capPos, obbPos, axes);
			VecType localCapDir{};
			for (int i = 0; i < N; ++i)
				localCapDir[i] = detail::vecDot(capDir, axes[i]);

			VecType zero(0.0f);
			Collision<VecType> localResult = CapsuleAABB(localCapPos, localCapDir, capHalfH, capRadius, zero, obbHalf);

			if (!localResult.collided)
				return localResult;

			VecType worldNormal = detail::obbLocalToWorldDir<VecType, N>(localResult.normal, axes);
			return { worldNormal, localResult.penetration, true };
		}

		// ============================================================
		// Cylinder collisions (3D only, Y-axis-aligned)
		// ============================================================

		template <typename VecType>
		Collision<VecType> CylinderSphere(const VecType& cylPos, float cylHalfH, float cylRadius, const VecType& spherePos,
		                                  float sphereRadius) {
			static_assert(math::VecTraits<VecType>::Dimensions == 3, "CylinderSphere is 3D only");

			float dy = spherePos[1] - cylPos[1];
			float clampedY = std::clamp(dy, -cylHalfH, cylHalfH);

			float dx = spherePos[0] - cylPos[0];
			float dz = spherePos[2] - cylPos[2];
			float radialDist2 = dx * dx + dz * dz;

			VecType closest = cylPos;
			closest[1] += clampedY;

			float radialDist = std::sqrt(radialDist2);
			if (radialDist > math::EPSILON) {
				float clampedR = std::min(radialDist, cylRadius);
				closest[0] += dx / radialDist * clampedR;
				closest[2] += dz / radialDist * clampedR;
			}

			VecType delta = spherePos - closest;
			float dist2 = detail::vecDot(delta, delta);

			if (dist2 > sphereRadius * sphereRadius)
				return { VecType(0.0f), 0.0f, false };

			float dist = std::sqrt(dist2);
			VecType normal = (dist > math::EPSILON) ? (delta / dist) : VecType(0.0f);

			return { normal, sphereRadius - dist, true };
		}

		// Cylinder vs AABB (3D only, Y-axis-aligned cylinder)
		template <typename VecType>
		Collision<VecType> CylinderAABB(const VecType& cylPos, float cylHalfH, float cylRadius, const VecType& boxPos,
		                                const VecType& boxHalf) {
			static_assert(math::VecTraits<VecType>::Dimensions == 3, "CylinderAABB is 3D only");

			// Y-axis overlap
			float cylMinY = cylPos[1] - cylHalfH;
			float cylMaxY = cylPos[1] + cylHalfH;
			float boxMinY = boxPos[1] - boxHalf[1];
			float boxMaxY = boxPos[1] + boxHalf[1];

			float yOverlap = std::min(cylMaxY, boxMaxY) - std::max(cylMinY, boxMinY);
			if (yOverlap < 0.0f)
				return { VecType(0.0f), 0.0f, false };

			// XZ-plane: circle vs rectangle
			float closestX = std::clamp(cylPos[0], boxPos[0] - boxHalf[0], boxPos[0] + boxHalf[0]);
			float closestZ = std::clamp(cylPos[2], boxPos[2] - boxHalf[2], boxPos[2] + boxHalf[2]);

			float dx = cylPos[0] - closestX;
			float dz = cylPos[2] - closestZ;
			float radialDist2 = dx * dx + dz * dz;

			if (radialDist2 > cylRadius * cylRadius)
				return { VecType(0.0f), 0.0f, false };

			float radialDist = std::sqrt(radialDist2);
			float radialPen = cylRadius - radialDist;

			VecType normal(0.0f);
			float pen;
			if (yOverlap < radialPen) {
				pen = yOverlap;
				normal[1] = (cylPos[1] > boxPos[1]) ? 1.0f : -1.0f;
			} else {
				pen = radialPen;
				if (radialDist > math::EPSILON) {
					normal[0] = dx / radialDist;
					normal[2] = dz / radialDist;
				} else {
					normal[0] = 1.0f;
				}
			}

			return { normal, pen, true };
		}

		// Cylinder vs Plane (3D only, Y-axis-aligned cylinder)
		template <typename VecType>
		Collision<VecType> CylinderPlane(const VecType& cylPos, float cylHalfH, float cylRadius, const VecType& planeNormal,
		                                 float planeDist) {
			static_assert(math::VecTraits<VecType>::Dimensions == 3, "CylinderPlane is 3D only");

			VecType top = cylPos;
			top[1] += cylHalfH;
			VecType bottom = cylPos;
			bottom[1] -= cylHalfH;

			float topDist = detail::vecDot(planeNormal, top) - planeDist;
			float botDist = detail::vecDot(planeNormal, bottom) - planeDist;

			VecType discCenter;
			float discDist;
			if (std::abs(topDist) < std::abs(botDist)) {
				discCenter = top;
				discDist = topDist;
			} else {
				discCenter = bottom;
				discDist = botDist;
			}

			float nx = planeNormal[0];
			float nz = planeNormal[2];
			float xzLen = std::sqrt(nx * nx + nz * nz);
			float effectiveRadius = cylRadius * xzLen;

			float minDist = discDist - effectiveRadius;

			if (minDist > 0.0f)
				return { VecType(0.0f), 0.0f, false };

			VecType normal = (discDist >= 0.0f) ? planeNormal : (planeNormal * -1.0f);
			float penetration = std::abs(minDist);

			return { normal, penetration, true };
		}

	} // namespace collision_solver

} // namespace cbk
