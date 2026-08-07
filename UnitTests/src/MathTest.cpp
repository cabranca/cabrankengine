#include <catch.hpp>
#include <Common/Math/MatrixFactory.h>

using namespace cbk::math;

// ====================================================
// Vector3fS
// ====================================================
TEST_CASE("Vector3 Arithmetic Operations") {
	Vector3 v{ 3, 4, 0 };

	SECTION("Binary Addition") {
		auto result = Vector3::Right + Vector3::Up;
		REQUIRE(result.x == 1.f);
		REQUIRE(result.y == 1.f);
		REQUIRE(result.z == 0.f);
	}

	SECTION("Unary Addition") {
		v += Vector3::Backward;
		REQUIRE(v.x == 3.f);
		REQUIRE(v.y == 4.f);
		REQUIRE(v.z == 1.f);
	}

	SECTION("Binary Subtraction") {
		auto result = Vector3::Right - Vector3::Up;
		REQUIRE(result.x == 1.f);
		REQUIRE(result.y == -1.f);
		REQUIRE(result.z == 0.f);
	}

	SECTION("Unary Subtraction") {
		v -= Vector3::Backward;
		REQUIRE(v.x == 3.f);
		REQUIRE(v.y == 4.f);
		REQUIRE(v.z == -1.f);
	}
}

TEST_CASE("Vector3 Scalar Multiplication") {
	Vector3 v{ 1, -2, 3 };

	SECTION("Binary multiplication scales all components") {
		auto result = v * 2.f;
		REQUIRE(result.x == Approx(2.f).margin(k_Epsilon));
		REQUIRE(result.y == Approx(-4.f).margin(k_Epsilon));
		REQUIRE(result.z == Approx(6.f).margin(k_Epsilon));
	}

	SECTION("Unary multiplication assignment updates in place") {
		v *= 0.5f;
		REQUIRE(v.x == Approx(0.5f).margin(k_Epsilon));
		REQUIRE(v.y == Approx(-1.f).margin(k_Epsilon));
		REQUIRE(v.z == Approx(1.5f).margin(k_Epsilon));
	}

	SECTION("Multiplying by zero gives zero Vector3") {
		auto result = v * 0.f;
		REQUIRE(result.x == Approx(0.f).margin(k_Epsilon));
		REQUIRE(result.y == Approx(0.f).margin(k_Epsilon));
		REQUIRE(result.z == Approx(0.f).margin(k_Epsilon));
	}

	SECTION("Multiplying by -1 inverts direction") {
		auto result = v * -1.f;
		REQUIRE(result.x == Approx(-v.x).margin(k_Epsilon));
		REQUIRE(result.y == Approx(-v.y).margin(k_Epsilon));
		REQUIRE(result.z == Approx(-v.z).margin(k_Epsilon));
	}

	SECTION("Scalar multiplication is distributive") {
		Vector3 a{ 1, 2, 3 };
		Vector3 b{ 4, 5, 6 };
		float s = 2.f;
		auto left = (a + b) * s;
		auto right = (a * s) + (b * s);
		REQUIRE(left.x == Approx(right.x).margin(k_Epsilon));
		REQUIRE(left.y == Approx(right.y).margin(k_Epsilon));
		REQUIRE(left.z == Approx(right.z).margin(k_Epsilon));
	}
}

TEST_CASE("Vector3 Unary Negation") {
	Vector3 v{ 1, -2, 3 };

	SECTION("Negation flips all signs") {
		auto neg = -v;
		REQUIRE(neg.x == Approx(-1.f).margin(k_Epsilon));
		REQUIRE(neg.y == Approx(2.f).margin(k_Epsilon));
		REQUIRE(neg.z == Approx(-3.f).margin(k_Epsilon));
	}

	SECTION("Double negation returns original Vector3") {
		auto neg = -(-v);
		REQUIRE(neg.x == Approx(v.x).margin(k_Epsilon));
		REQUIRE(neg.y == Approx(v.y).margin(k_Epsilon));
		REQUIRE(neg.z == Approx(v.z).margin(k_Epsilon));
	}

	SECTION("Negation preserves length") {
		auto neg = -v;
		REQUIRE(v.length() == Approx(neg.length()).margin(k_Epsilon));
	}

	SECTION("Negating zero Vector3 stays zero") {
		Vector3 zero{ 0, 0, 0 };
		auto neg = -zero;
		REQUIRE(neg.x == 0.f);
		REQUIRE(neg.y == 0.f);
		REQUIRE(neg.z == 0.f);
	}
}

TEST_CASE("Vector3 Length and Normalization") {
	Vector3 v{ 3, 4, 0 };
	REQUIRE(v.lengthSquared() == Approx(25.f).margin(k_Epsilon));

	Vector3 zero = Vector3::Zero;
	REQUIRE(zero.lengthSquared() == Approx(0.f).margin(k_Epsilon));

	SECTION("Length of standard axes") {
		REQUIRE(Vector3::Right.length() == Approx(1.f).margin(k_Epsilon));
		REQUIRE(Vector3::Up.length() == Approx(1.f).margin(k_Epsilon));
		REQUIRE(Vector3::Backward.length() == Approx(1.f).margin(k_Epsilon));
		REQUIRE(Vector3::Zero.length() == Approx(0.f).margin(k_Epsilon));
	}

	SECTION("Length of arbitrary Vector3") {
		Vector3 v{ 3, 4, 0 };
		REQUIRE(v.length() == Approx(5.f).margin(k_Epsilon));
	}

	SECTION("Normalize non-zero Vector3") {
		Vector3 v{ 3, 4, 0 };
		v.normalized();
		REQUIRE(v.length() == Approx(1.f).margin(k_Epsilon));
		REQUIRE(v.x == Approx(0.6f).margin(k_Epsilon));
		REQUIRE(v.y == Approx(0.8f).margin(k_Epsilon));
	}

	SECTION("Normalize zero Vector3 is no-op") {
		Vector3 v{ 0, 0, 0 };
		v.normalize();
		REQUIRE(v.x == 0.f);
		REQUIRE(v.y == 0.f);
		REQUIRE(v.z == 0.f);
	}

	SECTION("Double normalization keeps same value") {
		Vector3 v{ 1, 2, 3 };
		v.normalize();
		Vector3 before = v;
		v.normalize();
		REQUIRE(v.x == Approx(before.x).margin(k_Epsilon));
		REQUIRE(v.y == Approx(before.y).margin(k_Epsilon));
		REQUIRE(v.z == Approx(before.z).margin(k_Epsilon));
	}
}

TEST_CASE("Vector3 Dot Product") {
	SECTION("Orthogonal vectors return true") {
		REQUIRE(perpendicular(Vector3::Right, Vector3::Up));
		REQUIRE(perpendicular(Vector3::Up, Vector3::Forward));
		REQUIRE(perpendicular(Vector3::Forward, Vector3::Right));
	}

	SECTION("Non-orthogonal vectors return false") {
		REQUIRE_FALSE(perpendicular(Vector3{ 1, 1, 0 }, Vector3::Up));
	}

	SECTION("Parallel Vector3fs give product of magnitudes") {
		REQUIRE(dot({ 2, 0, 0 }, { 3, 0, 0 }) == Approx(6.f).margin(k_Epsilon));
	}

	SECTION("Opposite Vector3fs give negative result") {
		REQUIRE(dot({ 1, 0, 0 }, { -1, 0, 0 }) == Approx(-1.f).margin(k_Epsilon));
	}
}

TEST_CASE("Vector3 Cross Product") {
	SECTION("Cross product of orthogonal basis Vector3fs") {
		auto result = cross(Vector3::Right, Vector3::Up);
		REQUIRE(result.x == Approx(0.f).margin(k_Epsilon));
		REQUIRE(result.y == Approx(0.f).margin(k_Epsilon));
		REQUIRE(result.z == Approx(1.f).margin(k_Epsilon));
	}

	SECTION("Cross product is anti-commutative") {
		REQUIRE(cross(Vector3::Right, Vector3::Up).z == Approx(1.f).margin(k_Epsilon));
		REQUIRE(cross(Vector3::Up, Vector3::Right).z == Approx(-1.f).margin(k_Epsilon));
	}

	SECTION("Cross product of parallel Vector3fs is zero") {
		auto result = cross(Vector3::Right, Vector3::Right);
		REQUIRE(result.length() == Approx(0.f).margin(k_Epsilon));
	}

	SECTION("Result is orthogonal to both operands") {
		Vector3 a{ 1, 2, 3 };
		Vector3 b{ 4, 5, 6 };
		Vector3 c = cross(a, b);
		REQUIRE(dot(c, a) == Approx(0.f).margin(k_Epsilon));
		REQUIRE(dot(c, b) == Approx(0.f).margin(k_Epsilon));
	}
}

TEST_CASE("Vector3 Normalization Check") {
	SECTION("Normalized Vector3 reports true") {
		Vector3 v{ 1, 0, 0 };
		REQUIRE(v.isNormalized());
	}

	SECTION("Non-normalized Vector3 reports false") {
		Vector3 v{ 2, 0, 0 };
		REQUIRE_FALSE(v.isNormalized());
	}

	SECTION("Normalized after normalize()") {
		Vector3 v{ 123456, 6547849, 0 };
		v.normalized();
		REQUIRE(v.isNormalized());
	}
}

TEST_CASE("Vector3 Constants") {
	REQUIRE(Vector3::Right == Vector3{ 1, 0, 0 });
	REQUIRE(Vector3::Left == Vector3{ -1, 0, 0 });
	REQUIRE(Vector3::Up == Vector3{ 0, 1, 0 });
	REQUIRE(Vector3::Down == Vector3{ 0, -1, 0 });
	REQUIRE(Vector3::Forward == Vector3{ 0, 0, -1 });
	REQUIRE(Vector3::Backward == Vector3{ 0, 0, 1 });
	REQUIRE(Vector3::Zero == Vector3{ 0, 0, 0 });
	REQUIRE(Vector3::One == Vector3{ 1, 1, 1 });
}

TEST_CASE("Vector3 Equality and Inequality") {
	Vector3 a{ 1, 2, 3 };
	Vector3 b{ 1, 2, 3 };
	Vector3 c{ -1, 2, 3 };

	SECTION("Identical Vector3s are equal") {
		REQUIRE(a == b);
	}

	SECTION("Different Vector3s are not equal") {
		REQUIRE(a != c);
	}
}

TEST_CASE("Vector3 Angle Between") {
	SECTION("Parallel vectors have 0 radians between them") {
		REQUIRE(angleBetween(Vector3::Right, Vector3::Right) == Approx(0.f).margin(k_Epsilon));
	}

	SECTION("Opposite vectors have pi radians between them") {
		REQUIRE(angleBetween(Vector3::Right, Vector3::Left) == Approx(static_cast<float>(k_Pi)).margin(k_Epsilon));
	}

	SECTION("Perpendicular vectors have pi/2 radians between them") {
		REQUIRE(angleBetween(Vector3::Right, Vector3::Up) == Approx(static_cast<float>(k_HalfPi)).margin(k_Epsilon));
	}

	SECTION("Angle with zero vector returns 0") {
		REQUIRE(angleBetween(Vector3::Zero, Vector3::Right) == Approx(0.f).margin(k_Epsilon));
	}
}

// ====================================================
// Mat4 Constructors and Constants
// ====================================================
TEST_CASE("Mat4 Constructors and Constants") {
	SECTION("Default constructor initializes to zeros") {
		Mat4 m{};
		for (const auto& row: m.elements)
			REQUIRE(row == Vector4{ 0, 0, 0, 0 });
	}

	SECTION("Identity matrix matches expected pattern") {
		const auto& i = identityMat();
		REQUIRE(i.elements[0] == Vector4{ 1, 0, 0, 0 });
		REQUIRE(i.elements[1] == Vector4{ 0, 1, 0, 0 });
		REQUIRE(i.elements[2] == Vector4{ 0, 0, 1, 0 });
		REQUIRE(i.elements[3] == Vector4{ 0, 0, 0, 1 });
	}

	SECTION("Zero matrix has all zeros") {
		const auto& z = zeroMat();
		for (const auto& row: z.elements)
			REQUIRE(row == Vector4{ 0, 0, 0, 0 });
	}
}

// ====================================================
// Equality and Inequality
// ====================================================
TEST_CASE("Mat4 Equality and Inequality") {
	Mat4 a{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	Mat4 b{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	Mat4 c{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

	SECTION("Identical matrices are equal") {
		REQUIRE(a == b);
		REQUIRE_FALSE(a != b);
	}

	SECTION("Different matrices are not equal") {
		REQUIRE(a != c);
		REQUIRE_FALSE(a == c);
	}
}

// ====================================================
// Scale
// ====================================================
TEST_CASE("Mat4 Scaling") {
	SECTION("Uniform scale produces diagonal matrix") {
		Mat4 s = scaleUniform(2.f);
		REQUIRE(s.elements[0] == Vector4{ 2, 0, 0, 0 });
		REQUIRE(s.elements[1] == Vector4{ 0, 2, 0, 0 });
		REQUIRE(s.elements[2] == Vector4{ 0, 0, 2, 0 });
		REQUIRE(s.elements[3] == Vector4{ 0, 0, 0, 1 });
	}

	SECTION("Non-uniform scale uses vector components") {
		Vector3 v{ 2, 3, 4 };
		Mat4 s = scaleXYZ(v);
		REQUIRE(s.elements[0] == Vector4{ 2, 0, 0, 0 });
		REQUIRE(s.elements[1] == Vector4{ 0, 3, 0, 0 });
		REQUIRE(s.elements[2] == Vector4{ 0, 0, 4, 0 });
		REQUIRE(s.elements[3] == Vector4{ 0, 0, 0, 1 });
	}
}

// ====================================================
// Translation
// ====================================================
TEST_CASE("Mat4 Translation") {
	Vector3 t{ 10, 20, 30 };
	Mat4 m = translation(t);

	SECTION("Translation appears in last row") {
		REQUIRE(m.elements[3] == Vector4{ 10, 20, 30, 1 });
	}

	SECTION("Rotation rows remain identity") {
		REQUIRE(m.elements[0] == Vector4{ 1, 0, 0, 0 });
		REQUIRE(m.elements[1] == Vector4{ 0, 1, 0, 0 });
		REQUIRE(m.elements[2] == Vector4{ 0, 0, 1, 0 });
	}
}

// ====================================================
// Rotation around single axes
// ====================================================
TEST_CASE("Mat4 Rotation around single axes") {
	SECTION("Rotation X 90° swaps Y and Z with sign") {
		Mat4 rx = rotateX(90.f);
		REQUIRE(rx.elements[1].y == Approx(0.f).margin(k_Epsilon));
		REQUIRE(rx.elements[1].z == Approx(1.f).margin(k_Epsilon));
		REQUIRE(rx.elements[2].y == Approx(-1.f).margin(k_Epsilon));
		REQUIRE(rx.elements[2].z == Approx(0.f).margin(k_Epsilon));
	}

	SECTION("Rotation Y 90° swaps X and Z") {
		Mat4 ry = rotateY(90.f);
		REQUIRE(ry.elements[0].x == Approx(0.f).margin(k_Epsilon));
		REQUIRE(ry.elements[0].z == Approx(-1.f).margin(k_Epsilon));
		REQUIRE(ry.elements[2].x == Approx(1.f).margin(k_Epsilon));
		REQUIRE(ry.elements[2].z == Approx(0.f).margin(k_Epsilon));
	}

	SECTION("Rotation Z 90° swaps X and Y") {
		Mat4 rz = rotateZ(90.f);
		REQUIRE(rz.elements[0].x == Approx(0.f).margin(k_Epsilon));
		REQUIRE(rz.elements[0].y == Approx(1.f).margin(k_Epsilon));
		REQUIRE(rz.elements[1].x == Approx(-1.f).margin(k_Epsilon));
		REQUIRE(rz.elements[1].y == Approx(0.f).margin(k_Epsilon));
	}
}

// ====================================================
// Combined Rotation (Euler)
// ====================================================
TEST_CASE("Mat4 Combined Euler Rotation") {
	SECTION("Zero rotation returns Identity") {
		Vector3 zeroAngles{ 0, 0, 0 };
		Mat4 r = rotateXYZ(zeroAngles);
		REQUIRE(r == identityMat());
	}

	SECTION("Composite rotation behaves like composition") {
		Vector3 euler{ 90.f, 0.f, 0.f };
		Mat4 composed = rotateXYZ(euler);
		Mat4 manual = rotateZ(euler.z) * rotateX(euler.x) * rotateY(euler.y);
		REQUIRE(composed == manual);
	}
}

// ====================================================
// Transpose
// ====================================================
TEST_CASE("Mat4 Transpose") {
	Mat4 m{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	Mat4 t = m.transpose();

	SECTION("Transposed swaps rows and columns") {
		REQUIRE(t.elements[0] == Vector4{ 1, 5, 9, 13 });
		REQUIRE(t.elements[1] == Vector4{ 2, 6, 10, 14 });
		REQUIRE(t.elements[2] == Vector4{ 3, 7, 11, 15 });
	}

	SECTION("Last row becomes former last column") {
		REQUIRE(t.elements[3] == Vector4{ 4, 8, 12, 16 });
	}
}

// ====================================================
// Matrix Product
// ====================================================
TEST_CASE("Mat4 Product") {
	Mat4 a{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	Mat4 b{ 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
	Mat4 expected{ 250, 260, 270, 280, 618, 644, 670, 696, 986, 1028, 1070, 1112, 1354, 1412, 1470, 1528 };
	Mat4 prod = a * b;

	SECTION("Product matches expected values") {
		REQUIRE(prod == expected);
	}

	SECTION("Multiplying by Identity returns same matrix") {
		REQUIRE((a * identityMat()) == a);
	}

	SECTION("Multiplying Identity by any matrix returns same matrix") {
		REQUIRE((identityMat() * a) == a);
	}
}
