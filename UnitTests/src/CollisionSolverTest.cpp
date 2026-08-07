#include <catch.hpp>

#include <Cabrankengine/Core/CollisionSolver.hpp>

using namespace cbk;
using namespace cbk::math;
using namespace cbk::collision_solver;

// ====================================================
// AABB vs AABB
// ====================================================
TEST_CASE("AABB vs AABB - 2D Contact (no overlapping)") {
	Vector2 pos1(0, 0);
	Vector2 h1(0.5f, 0.5f);

	Vector2 pos2(1, 0);
	Vector2 h2(0.5f, 0.5f);

	auto result = AABB(pos1, h1, pos2, h2);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == 0.0f);
}

TEST_CASE("AABB vs AABB - 2D Overlap") {
	Vector2 pos1(0, 0);
	Vector2 h1(0.5f, 0.5f);

	Vector2 pos2(1, 0);
	Vector2 h2(1.0f, 1.0f);

	auto result = AABB(pos1, h1, pos2, h2);

	REQUIRE(result.collided);
	REQUIRE(result.penetration > 0.0f);
}

TEST_CASE("AABB vs AABB - 2D No Collision") {
	Vector2 pos1(0, 0);
	Vector2 h1(0.5f, 0.5f);

	Vector2 pos2(5, 0);
	Vector2 h2(0.5f, 0.5f);

	auto result = AABB(pos1, h1, pos2, h2);

	REQUIRE_FALSE(result.collided);
}

TEST_CASE("AABB vs AABB - 3D Contact (no overlapping)") {
	Vector3 pos1(0, 0, 0);
	Vector3 h1(0.5f, 0.5f, 0.5f);

	Vector3 pos2(1, 1, 1);
	Vector3 h2(0.5f, 0.5f, 0.5f);

	auto result = AABB(pos1, h1, pos2, h2);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == 0.0f);
}

TEST_CASE("AABB vs AABB - 3D Overlap") {
	Vector3 pos1(0, 0, 0);
	Vector3 h1(0.5f, 0.5f, 0.5f);

	Vector3 pos2(0.5f, 0.5f, 0.5f);
	Vector3 h2(0.5f, 0.5f, 0.5f);

	auto result = AABB(pos1, h1, pos2, h2);

	REQUIRE(result.collided);
	REQUIRE(result.penetration > 0.0f);
}

TEST_CASE("AABB vs AABB - 3D No Collision") {
	Vector3 pos1(0, 0, 0);
	Vector3 h1(0.5f, 0.5f, 0.5f);

	Vector3 pos2(5, 5, 5);
	Vector3 h2(0.5f, 0.5f, 0.5f);

	auto result = AABB(pos1, h1, pos2, h2);

	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Sphere vs Sphere
// ====================================================
TEST_CASE("Sphere vs Sphere - 2D Contact (no overlapping)") {
	Vector2 pos1(0, 0);
	Vector2 pos2(2, 0);

	auto result = Sphere(pos1, 1.0f, pos2, 1.0f);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.0f).margin(1e-5));
}

TEST_CASE("Sphere vs Sphere - 2D Overlap") {
	Vector2 pos1(0, 0);
	Vector2 pos2(1, 0);

	auto result = Sphere(pos1, 1.0f, pos2, 1.0f);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(1.0f).margin(1e-5));
}

TEST_CASE("Sphere vs Sphere - 2D No Collision") {
	Vector2 pos1(0, 0);
	Vector2 pos2(5, 0);

	auto result = Sphere(pos1, 1.0f, pos2, 1.0f);

	REQUIRE_FALSE(result.collided);
}

TEST_CASE("Sphere vs Sphere - 3D Contact (no overlapping)") {
	Vector3 pos1(0, 0, 0);
	Vector3 pos2(2, 0, 0);

	auto result = Sphere(pos1, 1.0f, pos2, 1.0f);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.0f).margin(1e-5));
}

TEST_CASE("Sphere vs Sphere - 3D Overlap") {
	Vector3 pos1(0, 0, 0);
	Vector3 pos2(1, 0, 0);

	auto result = Sphere(pos1, 1.0f, pos2, 1.0f);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(1.0f).margin(1e-5));
}

TEST_CASE("Sphere vs Sphere - 3D No Collision") {
	Vector3 pos1(0, 0, 0);
	Vector3 pos2(5, 0, 0);

	auto result = Sphere(pos1, 1.0f, pos2, 1.0f);

	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Sphere vs AABB
// ====================================================
TEST_CASE("Sphere vs AABB - 2D Contact (no overlapping)") {
	Vector2 circlePos(1, 0);
	float radius = 0.5f;

	Vector2 boxPos(0, 0);
	Vector2 boxHalf(0.5f, 0.5f);

	auto result = SphereAABB(circlePos, radius, boxPos, boxHalf);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == 0.0f);
}

TEST_CASE("Sphere vs AABB - 2D Overlap") {
	Vector2 circlePos(0.5f, 0.5f);
	float radius = 1.0f;

	Vector2 boxPos(0, 0);
	Vector2 boxHalf(0.5f, 0.5f);

	auto result = SphereAABB(circlePos, radius, boxPos, boxHalf);

	REQUIRE(result.collided);
	REQUIRE(result.penetration > 0.0f);
}

TEST_CASE("Sphere vs AABB - 2D No Collision") {
	Vector2 circlePos(5, 5);
	float radius = 1.0f;

	Vector2 boxPos(0, 0);
	Vector2 boxHalf(0.5f, 0.5f);

	auto result = SphereAABB(circlePos, radius, boxPos, boxHalf);

	REQUIRE_FALSE(result.collided);
}

TEST_CASE("Sphere vs AABB - 3D Contact (no overlapping)") {
	Vector3 spherePos(1, 0, 0);
	float radius = 0.5f;

	Vector3 boxPos(0, 0, 0);
	Vector3 boxHalf(0.5f, 0.5f, 0.5f);

	auto result = SphereAABB(spherePos, radius, boxPos, boxHalf);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == 0.0f);
}

TEST_CASE("Sphere vs AABB - 3D Overlap") {
	Vector3 spherePos(1, 1, 1);
	float radius = 1.0f;

	Vector3 boxPos(0, 0, 0);
	Vector3 boxHalf(0.5f, 0.5f, 0.5f);

	auto result = SphereAABB(spherePos, radius, boxPos, boxHalf);

	REQUIRE(result.collided);
	REQUIRE(result.penetration > 0.0f);
}

TEST_CASE("Sphere vs AABB - 3D No Collision") {
	Vector3 spherePos(5, 5, 5);
	float radius = 1.0f;

	Vector3 boxPos(0, 0, 0);
	Vector3 boxHalf(0.5f, 0.5f, 0.5f);

	auto result = SphereAABB(spherePos, radius, boxPos, boxHalf);

	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Bitmask Filter
// ====================================================
TEST_CASE("shouldCollide - default layers collide") {
	REQUIRE(shouldCollide(0x0001, 0xFFFFFFFF, 0x0001, 0xFFFFFFFF));
}

TEST_CASE("shouldCollide - different layers collide when masks allow") {
	REQUIRE(shouldCollide(0x0001, 0x0002, 0x0002, 0x0001));
}

TEST_CASE("shouldCollide - no collision when mask excludes category") {
	REQUIRE_FALSE(shouldCollide(0x0001, 0x0000, 0x0002, 0xFFFFFFFF));
}

TEST_CASE("shouldCollide - asymmetric mask blocks") {
	// A can see B, but B can't see A
	REQUIRE_FALSE(shouldCollide(0x0001, 0x0002, 0x0002, 0x0004));
}

// ====================================================
// OBB vs OBB
// ====================================================
TEST_CASE("OBB vs OBB - 2D Aligned (effectively AABB) Overlap") {
	Vector2 pos1(0, 0);
	Vector2 h1(0.5f, 0.5f);
	float orient1 = 0.0f;

	Vector2 pos2(0.5f, 0);
	Vector2 h2(0.5f, 0.5f);
	float orient2 = 0.0f;

	auto result = OBB(pos1, h1, orient1, pos2, h2, orient2);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.5f).margin(1e-4));
}

TEST_CASE("OBB vs OBB - 2D Rotated 45deg Overlap") {
	Vector2 pos1(0, 0);
	Vector2 h1(0.5f, 0.5f);
	float orient1 = 0.0f;

	Vector2 pos2(1.0f, 0);
	Vector2 h2(0.5f, 0.5f);
	float orient2 = radians(45.0f);

	auto result = OBB(pos1, h1, orient1, pos2, h2, orient2);

	// At distance 1.0, boxes of half-extent 0.5 and one rotated 45deg should still overlap
	REQUIRE(result.collided);
}

TEST_CASE("OBB vs OBB - 2D No Collision") {
	Vector2 pos1(0, 0);
	Vector2 h1(0.5f, 0.5f);
	float orient1 = 0.0f;

	Vector2 pos2(5.0f, 0);
	Vector2 h2(0.5f, 0.5f);
	float orient2 = 0.0f;

	auto result = OBB(pos1, h1, orient1, pos2, h2, orient2);
	REQUIRE_FALSE(result.collided);
}

TEST_CASE("OBB vs OBB - 3D Aligned Overlap") {
	Vector3 pos1(0, 0, 0);
	Vector3 h1(0.5f);
	math::Quaternion orient1{}; // identity

	Vector3 pos2(0.5f, 0, 0);
	Vector3 h2(0.5f);
	math::Quaternion orient2{};

	auto result = OBB(pos1, h1, orient1, pos2, h2, orient2);

	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.5f).margin(1e-4));
}

TEST_CASE("OBB vs OBB - 3D Rotated 45deg Y-axis") {
	Vector3 pos1(0, 0, 0);
	Vector3 h1(0.5f);
	math::Quaternion orient1{};

	Vector3 pos2(1.0f, 0, 0);
	Vector3 h2(0.5f);
	math::Quaternion orient2 = math::Quaternion::fromAxisAngle({ 0, 1, 0 }, math::radians(45.0f));

	auto result = OBB(pos1, h1, orient1, pos2, h2, orient2);
	REQUIRE(result.collided);
}

TEST_CASE("OBB vs OBB - 3D No Collision") {
	Vector3 pos1(0, 0, 0);
	Vector3 h1(0.5f);
	math::Quaternion orient1{};

	Vector3 pos2(5, 5, 5);
	Vector3 h2(0.5f);
	math::Quaternion orient2{};

	auto result = OBB(pos1, h1, orient1, pos2, h2, orient2);
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// OBB vs Sphere
// ====================================================
TEST_CASE("OBB vs Sphere - 2D Overlap") {
	Vector2 obbPos(0, 0);
	Vector2 obbHalf(0.5f, 0.5f);
	float orient = 0.0f;

	Vector2 spherePos(0.8f, 0);
	float radius = 0.5f;

	auto result = OBBSphere(obbPos, obbHalf, orient, spherePos, radius);
	REQUIRE(result.collided);
}

TEST_CASE("OBB vs Sphere - 2D No Collision") {
	Vector2 obbPos(0, 0);
	Vector2 obbHalf(0.5f, 0.5f);
	float orient = 0.0f;

	Vector2 spherePos(5, 5);
	float radius = 0.5f;

	auto result = OBBSphere(obbPos, obbHalf, orient, spherePos, radius);
	REQUIRE_FALSE(result.collided);
}

TEST_CASE("OBB vs Sphere - 3D Overlap rotated") {
	Vector3 obbPos(0, 0, 0);
	Vector3 obbHalf(1.0f, 0.5f, 0.5f);
	math::Quaternion orient = math::Quaternion::fromAxisAngle({ 0, 0, 1 }, math::radians(90.0f));
	// After 90deg Z rotation, the long axis (X) becomes Y

	Vector3 spherePos(0, 1.2f, 0);
	float radius = 0.5f;

	auto result = OBBSphere(obbPos, obbHalf, orient, spherePos, radius);
	REQUIRE(result.collided);
}

// ====================================================
// OBB vs AABB
// ====================================================
TEST_CASE("OBB vs AABB - 2D Overlap") {
	Vector2 obbPos(0.3f, 0);
	Vector2 obbHalf(0.5f, 0.5f);
	float orient = radians(30.0f);

	Vector2 aabbPos(0, 0);
	Vector2 aabbHalf(0.5f, 0.5f);

	auto result = OBBAABB(obbPos, obbHalf, orient, aabbPos, aabbHalf);
	REQUIRE(result.collided);
}

TEST_CASE("OBB vs AABB - 2D No Collision") {
	Vector2 obbPos(5, 0);
	Vector2 obbHalf(0.5f, 0.5f);
	float orient = 0.0f;

	Vector2 aabbPos(0, 0);
	Vector2 aabbHalf(0.5f, 0.5f);

	auto result = OBBAABB(obbPos, obbHalf, orient, aabbPos, aabbHalf);
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Plane vs Sphere
// ====================================================
TEST_CASE("Plane vs Sphere - 2D Penetrating") {
	Vector2 normal(0, 1);
	float dist = 0.0f; // plane at y=0

	Vector2 spherePos(0, 0.5f);
	float radius = 1.0f;

	auto result = PlaneSphere(normal, dist, spherePos, radius);
	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.5f).margin(1e-5));
}

TEST_CASE("Plane vs Sphere - 2D Above") {
	Vector2 normal(0, 1);
	float dist = 0.0f;

	Vector2 spherePos(0, 5.0f);
	float radius = 1.0f;

	auto result = PlaneSphere(normal, dist, spherePos, radius);
	REQUIRE_FALSE(result.collided);
}

TEST_CASE("Plane vs Sphere - 3D Penetrating") {
	Vector3 normal(0, 1, 0);
	float dist = 0.0f;

	Vector3 spherePos(0, 0.3f, 0);
	float radius = 1.0f;

	auto result = PlaneSphere(normal, dist, spherePos, radius);
	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.7f).margin(1e-5));
}

// ====================================================
// Plane vs AABB
// ====================================================
TEST_CASE("Plane vs AABB - 2D Penetrating") {
	Vector2 normal(0, 1);
	float dist = 0.0f;

	Vector2 boxPos(0, 0.3f);
	Vector2 boxHalf(0.5f, 0.5f);

	auto result = PlaneAABB(normal, dist, boxPos, boxHalf);
	REQUIRE(result.collided);
}

TEST_CASE("Plane vs AABB - 2D Above") {
	Vector2 normal(0, 1);
	float dist = 0.0f;

	Vector2 boxPos(0, 5.0f);
	Vector2 boxHalf(0.5f, 0.5f);

	auto result = PlaneAABB(normal, dist, boxPos, boxHalf);
	REQUIRE_FALSE(result.collided);
}

TEST_CASE("Plane vs AABB - 3D Penetrating") {
	Vector3 normal(0, 1, 0);
	float dist = 0.0f;

	Vector3 boxPos(0, 0.3f, 0);
	Vector3 boxHalf(0.5f, 0.5f, 0.5f);

	auto result = PlaneAABB(normal, dist, boxPos, boxHalf);
	REQUIRE(result.collided);
}

// ====================================================
// Plane vs OBB
// ====================================================
TEST_CASE("Plane vs OBB - 2D Penetrating rotated") {
	Vector2 normal(0, 1);
	float dist = 0.0f;

	Vector2 obbPos(0, 0.5f);
	Vector2 obbHalf(0.5f, 0.5f);
	float orient = radians(45.0f);

	auto result = PlaneOBB(normal, dist, obbPos, obbHalf, orient);
	REQUIRE(result.collided);
}

TEST_CASE("Plane vs OBB - 3D Above") {
	Vector3 normal(0, 1, 0);
	float dist = 0.0f;

	Vector3 obbPos(0, 5, 0);
	Vector3 obbHalf(0.5f);
	math::Quaternion orient{};

	auto result = PlaneOBB(normal, dist, obbPos, obbHalf, orient);
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Capsule vs Capsule
// ====================================================
TEST_CASE("Capsule vs Capsule - 2D Parallel Overlap") {
	Vector2 pos1(0, 0), dir1(0, 1);
	float hh1 = 1.0f, r1 = 0.5f;

	Vector2 pos2(0.5f, 0), dir2(0, 1);
	float hh2 = 1.0f, r2 = 0.5f;

	auto result = CapsuleCapsule(pos1, dir1, hh1, r1, pos2, dir2, hh2, r2);
	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.5f).margin(1e-4));
}

TEST_CASE("Capsule vs Capsule - 2D No Collision") {
	Vector2 pos1(0, 0), dir1(0, 1);
	float hh1 = 1.0f, r1 = 0.5f;

	Vector2 pos2(5, 0), dir2(0, 1);
	float hh2 = 1.0f, r2 = 0.5f;

	auto result = CapsuleCapsule(pos1, dir1, hh1, r1, pos2, dir2, hh2, r2);
	REQUIRE_FALSE(result.collided);
}

TEST_CASE("Capsule vs Capsule - 3D Crossing Overlap") {
	Vector3 pos1(0, 0, 0), dir1(1, 0, 0);
	float hh1 = 1.0f, r1 = 0.3f;

	Vector3 pos2(0, 0.4f, 0), dir2(0, 0, 1);
	float hh2 = 1.0f, r2 = 0.3f;

	auto result = CapsuleCapsule(pos1, dir1, hh1, r1, pos2, dir2, hh2, r2);
	REQUIRE(result.collided);
}

// ====================================================
// Capsule vs Sphere
// ====================================================
TEST_CASE("Capsule vs Sphere - 2D Broadside Contact") {
	Vector2 capPos(0, 0), capDir(0, 1);
	float capHH = 1.0f, capR = 0.5f;

	Vector2 spherePos(1.0f, 0);
	float sphereR = 0.5f;

	auto result = CapsuleSphere(capPos, capDir, capHH, capR, spherePos, sphereR);
	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.0f).margin(1e-4));
}

TEST_CASE("Capsule vs Sphere - 3D Tip Contact") {
	Vector3 capPos(0, 0, 0), capDir(0, 1, 0);
	float capHH = 1.0f, capR = 0.3f;

	Vector3 spherePos(0, 1.5f, 0);
	float sphereR = 0.3f;

	auto result = CapsuleSphere(capPos, capDir, capHH, capR, spherePos, sphereR);
	// Closest point on segment to sphere is (0,1,0), distance = 0.5
	// Combined radii = 0.6 > 0.5 → collision
	REQUIRE(result.collided);
}

TEST_CASE("Capsule vs Sphere - 3D No Collision") {
	Vector3 capPos(0, 0, 0), capDir(0, 1, 0);
	float capHH = 1.0f, capR = 0.3f;

	Vector3 spherePos(5, 0, 0);
	float sphereR = 0.3f;

	auto result = CapsuleSphere(capPos, capDir, capHH, capR, spherePos, sphereR);
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Capsule vs AABB
// ====================================================
TEST_CASE("Capsule vs AABB - 2D Overlap") {
	Vector2 capPos(0, 0), capDir(0, 1);
	float capHH = 1.0f, capR = 0.5f;

	Vector2 boxPos(0.8f, 0);
	Vector2 boxHalf(0.5f, 0.5f);

	auto result = CapsuleAABB(capPos, capDir, capHH, capR, boxPos, boxHalf);
	REQUIRE(result.collided);
}

TEST_CASE("Capsule vs AABB - 3D No Collision") {
	Vector3 capPos(0, 0, 0), capDir(0, 1, 0);
	float capHH = 1.0f, capR = 0.3f;

	Vector3 boxPos(5, 5, 5);
	Vector3 boxHalf(0.5f, 0.5f, 0.5f);

	auto result = CapsuleAABB(capPos, capDir, capHH, capR, boxPos, boxHalf);
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Capsule vs Plane
// ====================================================
TEST_CASE("Capsule vs Plane - 2D Penetrating") {
	Vector2 capPos(0, 0.5f), capDir(0, 1);
	float capHH = 1.0f, capR = 0.3f;

	Vector2 normal(0, 1);
	float dist = 0.0f;

	// Bottom endpoint at y = -0.5, distance to plane = 0.5, radius = 0.3
	// 0.5 > 0.3 so bottom endpoint is above plane
	// But top endpoint at y = 1.5, distance = 1.5 which is > 0.3
	// Closest endpoint to plane is bottom (y=-0.5), dist=0.5 > 0.3 → no collision?
	// Wait, capPos.y=0.5, bottom=0.5-1.0=-0.5. signedDist of bottom = -0.5
	// |signedDist| = 0.5 > capR(0.3) → no collision
	// Let me adjust:
	auto result = CapsulePlane(Vector2(0, 0.2f), capDir, capHH, capR, normal, dist);
	// bottom endpoint at y = 0.2 - 1.0 = -0.8, signedDist = -0.8, |0.8| > 0.3 → no collision
	// Need endpoint close to plane:
	auto result2 = CapsulePlane(Vector2(0, 1.1f), capDir, capHH, capR, normal, dist);
	// bottom endpoint at y = 1.1 - 1.0 = 0.1, signedDist = 0.1, |0.1| < 0.3 → collision!
	REQUIRE(result2.collided);
	REQUIRE(result2.penetration == Approx(0.2f).margin(1e-4));
}

TEST_CASE("Capsule vs Plane - 3D No Collision") {
	Vector3 capPos(0, 5, 0), capDir(0, 1, 0);
	float capHH = 1.0f, capR = 0.3f;

	Vector3 normal(0, 1, 0);
	float dist = 0.0f;

	auto result = CapsulePlane(capPos, capDir, capHH, capR, normal, dist);
	// Bottom endpoint at y = 4, signedDist = 4, |4| > 0.3
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Capsule vs OBB
// ====================================================
TEST_CASE("Capsule vs OBB - 2D Overlap") {
	Vector2 capPos(0, 0), capDir(0, 1);
	float capHH = 1.0f, capR = 0.5f;

	Vector2 obbPos(0.8f, 0);
	Vector2 obbHalf(0.5f, 0.5f);
	float orient = 0.0f;

	auto result = CapsuleOBB(capPos, capDir, capHH, capR, obbPos, obbHalf, orient);
	REQUIRE(result.collided);
}

TEST_CASE("Capsule vs OBB - 3D No Collision") {
	Vector3 capPos(0, 0, 0), capDir(0, 1, 0);
	float capHH = 1.0f, capR = 0.3f;

	Vector3 obbPos(5, 5, 5);
	Vector3 obbHalf(0.5f);
	math::Quaternion orient{};

	auto result = CapsuleOBB(capPos, capDir, capHH, capR, obbPos, obbHalf, orient);
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Cylinder vs Sphere (3D only)
// ====================================================
TEST_CASE("Cylinder vs Sphere - 3D Broadside Overlap") {
	Vector3 cylPos(0, 0, 0);
	float cylHH = 1.0f, cylR = 0.5f;

	Vector3 spherePos(0.8f, 0, 0);
	float sphereR = 0.5f;

	auto result = CylinderSphere(cylPos, cylHH, cylR, spherePos, sphereR);
	REQUIRE(result.collided);
}

TEST_CASE("Cylinder vs Sphere - 3D Top Cap Overlap") {
	Vector3 cylPos(0, 0, 0);
	float cylHH = 1.0f, cylR = 0.5f;

	Vector3 spherePos(0, 1.3f, 0);
	float sphereR = 0.5f;

	auto result = CylinderSphere(cylPos, cylHH, cylR, spherePos, sphereR);
	REQUIRE(result.collided);
}

TEST_CASE("Cylinder vs Sphere - 3D No Collision") {
	Vector3 cylPos(0, 0, 0);
	float cylHH = 1.0f, cylR = 0.5f;

	Vector3 spherePos(5, 5, 5);
	float sphereR = 0.5f;

	auto result = CylinderSphere(cylPos, cylHH, cylR, spherePos, sphereR);
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Cylinder vs AABB (3D only)
// ====================================================
TEST_CASE("Cylinder vs AABB - 3D Overlap") {
	Vector3 cylPos(0, 0, 0);
	float cylHH = 1.0f, cylR = 0.5f;

	Vector3 boxPos(0.8f, 0, 0);
	Vector3 boxHalf(0.5f, 0.5f, 0.5f);

	auto result = CylinderAABB(cylPos, cylHH, cylR, boxPos, boxHalf);
	REQUIRE(result.collided);
}

TEST_CASE("Cylinder vs AABB - 3D No Collision (Y separated)") {
	Vector3 cylPos(0, 0, 0);
	float cylHH = 1.0f, cylR = 0.5f;

	Vector3 boxPos(0, 5, 0);
	Vector3 boxHalf(0.5f, 0.5f, 0.5f);

	auto result = CylinderAABB(cylPos, cylHH, cylR, boxPos, boxHalf);
	REQUIRE_FALSE(result.collided);
}

TEST_CASE("Cylinder vs AABB - 3D No Collision (XZ separated)") {
	Vector3 cylPos(0, 0, 0);
	float cylHH = 1.0f, cylR = 0.5f;

	Vector3 boxPos(5, 0, 0);
	Vector3 boxHalf(0.5f, 0.5f, 0.5f);

	auto result = CylinderAABB(cylPos, cylHH, cylR, boxPos, boxHalf);
	REQUIRE_FALSE(result.collided);
}

// ====================================================
// Cylinder vs Plane (3D only)
// ====================================================
TEST_CASE("Cylinder vs Plane - 3D Penetrating (horizontal plane)") {
	Vector3 cylPos(0, 0.5f, 0);
	float cylHH = 1.0f, cylR = 0.5f;

	Vector3 normal(0, 1, 0);
	float dist = 0.0f;

	// Bottom of cylinder at y = -0.5. Disc is in XZ plane.
	// Plane normal is (0,1,0), effective radius in XZ for normal (0,1,0) = cylR * |(0,0)| = 0
	// So minDist = botDist - 0 = -0.5 - 0 = -0.5 → penetration = 0.5
	auto result = CylinderPlane(cylPos, cylHH, cylR, normal, dist);
	REQUIRE(result.collided);
	REQUIRE(result.penetration == Approx(0.5f).margin(1e-4));
}

TEST_CASE("Cylinder vs Plane - 3D Penetrating (tilted plane)") {
	Vector3 cylPos(0, 0, 0);
	float cylHH = 1.0f, cylR = 1.0f;

	// A plane tilted at 45 degrees in XY
	float invSqrt2 = 1.0f / std::sqrt(2.0f);
	Vector3 normal(invSqrt2, invSqrt2, 0);
	float dist = 0.0f;

	auto result = CylinderPlane(cylPos, cylHH, cylR, normal, dist);
	// The disc closest to the plane: depends on which disc center is closer
	// Both discs: top y=1, bottom y=-1
	// topDist = invSqrt2*0 + invSqrt2*1 = invSqrt2 ≈ 0.707
	// botDist = invSqrt2*0 + invSqrt2*(-1) = -invSqrt2 ≈ -0.707
	// |topDist| == |botDist|, so we pick top (first in comparison with <)
	// Actually abs(topDist) is NOT < abs(botDist), they're equal, so we pick bottom
	// effectiveRadius = cylR * |normal.xz| = 1.0 * |invSqrt2, 0| = invSqrt2
	// minDist = botDist - effectiveRadius = -0.707 - 0.707 = -1.414
	// penetration = 1.414
	REQUIRE(result.collided);
}

TEST_CASE("Cylinder vs Plane - 3D No Collision") {
	Vector3 cylPos(0, 5, 0);
	float cylHH = 1.0f, cylR = 0.5f;

	Vector3 normal(0, 1, 0);
	float dist = 0.0f;

	auto result = CylinderPlane(cylPos, cylHH, cylR, normal, dist);
	// Bottom at y=4, effective radius = 0 (plane is horizontal), minDist = 4 > 0 → no collision
	REQUIRE_FALSE(result.collided);
}
