#include <catch.hpp>

#include <Cabrankengine/ECS/ECS.h>

using namespace cbk::ecs;

// --------------------------
// Dummy components
// --------------------------
struct Position {
	float x, y;
};
struct Velocity {
	float dx, dy;
};

// Dummy system
class MovementSystem : public ISystem {
  public:
	virtual void update(Registry& registry, float dt) override {
		for (const auto& e: m_Entities) {
			auto pos = registry.getComponent<Position>(e).value();
			auto vel = registry.getComponent<Velocity>(e).value();
			pos->x += vel->dx * dt;
			pos->y += vel->dy * dt;
		}
	}
};

// ============================================================
// EntityManager tests
// ============================================================
TEST_CASE("EntityManager - create and destroy entities") {
	EntityManager em;

	Entity e1 = em.createEntity();
	Entity e2 = em.createEntity();

	REQUIRE(e1 != e2);
	REQUIRE(e1 < MAX_ENTITIES);
	REQUIRE(e2 < MAX_ENTITIES);

	Signature sig;
	sig.set(0);
	em.setSignature(e1, sig);

	sig = em.getSignature(e1);
	REQUIRE(sig.test(0));

	// Destroy should reset signature
	em.destroyEntity(e1);
	sig = em.getSignature(e1);
	REQUIRE(sig.none());
}

// ============================================================
// ComponentManager tests
// ============================================================
TEST_CASE("ComponentManager - add, get, remove") {
	ComponentManager cm;
	cm.registerComponent<Position>();

	Entity e = 0;
	cm.addComponent<Position>(e, Position{ 1.f, 2.f });

	auto pos = cm.getComponent<Position>(e);
	REQUIRE(pos.has_value());
	REQUIRE(pos.value()->x == Approx(1.f));
	REQUIRE(pos.value()->y == Approx(2.f));

	pos.value()->x = 5.f;
	REQUIRE(cm.getComponent<Position>(e).value()->x == Approx(5.f));

	cm.removeComponent<Position>(e);
	auto removedComponent = cm.getComponent<Position>(e);
	REQUIRE_FALSE(removedComponent.has_value());
}

TEST_CASE("ComponentManager - get missing component returns empty optional") {
	ComponentManager cm;
	cm.registerComponent<Position>();

	auto missingComponent = cm.getComponent<Position>(0);
	REQUIRE_FALSE(missingComponent.has_value());
}

// ============================================================
// SystemManager tests
// ============================================================
TEST_CASE("SystemManager - entity filtering by signature") {
	ComponentManager cm;
	EntityManager em;
	SystemManager sm;

	cm.registerComponent<Position>();
	cm.registerComponent<Velocity>();

	auto moveSys = sm.RegisterSystem<MovementSystem>();

	Signature moveSig;
	moveSig.set(cm.getComponentType<Position>());
	moveSig.set(cm.getComponentType<Velocity>());
	sm.SetSignature<MovementSystem>(moveSig);

	// Entity with only Position
	Entity e1 = em.createEntity();
	em.setSignature(e1, Signature{}.set(cm.getComponentType<Position>()));
	sm.EntitySignatureChanged(e1, em.getSignature(e1));
	REQUIRE(moveSys->getEntities().empty());

	// Entity with Position + Velocity
	Entity e2 = em.createEntity();
	Signature sig2;
	sig2.set(cm.getComponentType<Position>());
	sig2.set(cm.getComponentType<Velocity>());
	em.setSignature(e2, sig2);
	sm.EntitySignatureChanged(e2, sig2);
	REQUIRE(moveSys->getEntities().contains(e2));

	// Removing component should remove from system
	sig2.reset(cm.getComponentType<Velocity>());
	em.setSignature(e2, sig2);
	sm.EntitySignatureChanged(e2, sig2);
	REQUIRE_FALSE(moveSys->getEntities().contains(e2));
}

// ============================================================
// Registry integration tests
// ============================================================
TEST_CASE("Registry - component add/remove updates signature") {
	Registry reg;
	reg.registerComponent<Position>();

	Entity e = reg.createEntity();
	reg.addComponent(e, Position{ 10.f, 20.f });

	auto sig = reg.getSignature(e);
	REQUIRE(sig.any());

	reg.removeComponent<Position>(e);
	sig = reg.getSignature(e);
	REQUIRE(sig.none());
}

TEST_CASE("Registry + SystemManager integration") {
	Registry reg;
	reg.registerComponent<Position>();
	reg.registerComponent<Velocity>();

	auto moveSys = reg.registerSystem<MovementSystem>();

	Signature sig;
	sig.set(reg.getComponentType<Position>());
	sig.set(reg.getComponentType<Velocity>());
	reg.setSystemSignature<MovementSystem>(sig);

	Entity e = reg.createEntity();
	reg.addComponent(e, Position{ 0.f, 0.f });
	reg.addComponent(e, Velocity{ 1.f, 1.f });

	REQUIRE(moveSys->getEntities().contains(e));

	moveSys->update(reg, 1.0f);

	auto pos = reg.getComponent<Position>(e).value();
	REQUIRE(pos->x == Approx(1.f));
	REQUIRE(pos->y == Approx(1.f));
}
