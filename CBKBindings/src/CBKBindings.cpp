#include <string>

#include <emscripten/bind.h>

#include <nlohmann/json.hpp>

#include <Cabrankengine/Scene/Scene.h>
#include <Cabrankengine/Scene/SceneSerializer.h>
#include <Cabrankengine/Scene/ComponentSerialization.h>
#include <Cabrankengine/ECS/Components.h>

using namespace cbk;
using namespace emscripten;

static scene::Scene g_Scene;

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

static void sceneNew(const std::string& name) {
    scene::SceneMetadata meta;
    meta.Name = name;
    g_Scene = scene::Scene(meta); // This isn't propagated to the Application
}

static std::string serializeScene() {
    return scene::SceneSerializer::serializeToString(g_Scene);
}

static void deserializeScene(const std::string& json) {
    g_Scene = scene::SceneSerializer::deserializeFromString(json); // This isn't propagated to the Application
}

// ---------------------------------------------------------------------------
// Entities
// ---------------------------------------------------------------------------

static int entityCreate(const std::string& name) {
    ecs::Entity e = g_Scene.createEntity(name);
    return static_cast<int>(e);
}

// Have to define the exact API the front wants
static std::string getAllEntities() {
    return scene::SceneSerializer::serializeToString(g_Scene);
}

static void entityDestroy(int e) {
    g_Scene.destroyEntity(e);
}

// ---------------------------------------------------------------------------
// Component getters — return JSON string, or "" if component is absent
// ---------------------------------------------------------------------------

template<typename C>
static std::string getComponent(int entityId) {
    auto opt = g_Scene.getRegistry()->getComponent<C>(static_cast<ecs::Entity>(entityId));
    if (!opt) return "";
    nlohmann::json j = **opt;
    return j.dump();
}

static std::string getTransform(int e)         { return getComponent<ecs::CTransform>(e); }
static std::string getCamera(int e)            { return getComponent<ecs::CCamera>(e); }
static std::string getCameraController(int e)  { return getComponent<ecs::CCameraController>(e); }
static std::string getDirectionalLight(int e)  { return getComponent<ecs::CDirectionalLight>(e); }
static std::string getPointLight(int e)        { return getComponent<ecs::CPointLight>(e); }
static std::string getText(int e)              { return getComponent<ecs::CText>(e); }

// ---------------------------------------------------------------------------
// Component add — accepts JSON string, returns false on bad JSON
// ---------------------------------------------------------------------------

template<typename C>
static bool addComponent(int entityId, const std::string& json) {
    try {
        g_Scene.getRegistry()->addComponent(static_cast<ecs::Entity>(entityId),
                                            nlohmann::json::parse(json).get<C>());
        return true;
    } catch (...) {
        return false;
    }
}

static bool addTransform(int e, const std::string& json)        { return addComponent<ecs::CTransform>(e, json); }
static bool addCamera(int e, const std::string& json)           { return addComponent<ecs::CCamera>(e, json); }
static bool addCameraController(int e, const std::string& json) { return addComponent<ecs::CCameraController>(e, json); }
static bool addDirectionalLight(int e, const std::string& json) { return addComponent<ecs::CDirectionalLight>(e, json); }
static bool addPointLight(int e, const std::string& json)       { return addComponent<ecs::CPointLight>(e, json); }
static bool addText(int e, const std::string& json)             { return addComponent<ecs::CText>(e, json); }

// ---------------------------------------------------------------------------
// Component setters — overwrites existing component, returns false if absent
// ---------------------------------------------------------------------------

template<typename C>
static bool setComponent(int entityId, const std::string& json) {
    auto opt = g_Scene.getRegistry()->getComponent<C>(static_cast<ecs::Entity>(entityId));
    if (!opt) return false;
    try {
        **opt = nlohmann::json::parse(json).get<C>();
        return true;
    } catch (...) {
        return false;
    }
}

static bool setTransform(int e, const std::string& json)        { return setComponent<ecs::CTransform>(e, json); }
static bool setCamera(int e, const std::string& json)           { return setComponent<ecs::CCamera>(e, json); }
static bool setCameraController(int e, const std::string& json) { return setComponent<ecs::CCameraController>(e, json); }
static bool setDirectionalLight(int e, const std::string& json) { return setComponent<ecs::CDirectionalLight>(e, json); }
static bool setPointLight(int e, const std::string& json)       { return setComponent<ecs::CPointLight>(e, json); }
static bool setText(int e, const std::string& json)             { return setComponent<ecs::CText>(e, json); }

// ---------------------------------------------------------------------------
// Component remove
// ---------------------------------------------------------------------------

template<typename C>
static void removeComponent(int entityId) {
    g_Scene.getRegistry()->removeComponent<C>(static_cast<ecs::Entity>(entityId));
}

static void removeTransform(int e)        { removeComponent<ecs::CTransform>(e); }
static void removeCamera(int e)           { removeComponent<ecs::CCamera>(e); }
static void removeCameraController(int e) { removeComponent<ecs::CCameraController>(e); }
static void removeDirectionalLight(int e) { removeComponent<ecs::CDirectionalLight>(e); }
static void removePointLight(int e)       { removeComponent<ecs::CPointLight>(e); }
static void removeText(int e)             { removeComponent<ecs::CText>(e); }

// ---------------------------------------------------------------------------
// Bindings
// ---------------------------------------------------------------------------

EMSCRIPTEN_BINDINGS(cbk) {
    function("sceneNew",              &sceneNew);
    function("serializeScene",        &serializeScene);
    function("deserializeScene",      &deserializeScene);
    function("entityCreate",          &entityCreate);
    function("getAllEntities",         &getAllEntities);
    function("entityDestroy",          &entityDestroy);
    function("getTransform",           &getTransform);
    function("getCamera",              &getCamera);
    function("getCameraController",    &getCameraController);
    function("getDirectionalLight",    &getDirectionalLight);
    function("getPointLight",          &getPointLight);
    function("getText",                &getText);
    function("setTransform",           &setTransform);
    function("setCamera",              &setCamera);
    function("setCameraController",    &setCameraController);
    function("setDirectionalLight",    &setDirectionalLight);
    function("setPointLight",          &setPointLight);
    function("setText",                &setText);
    function("addTransform",           &addTransform);
    function("addCamera",              &addCamera);
    function("addCameraController",    &addCameraController);
    function("addDirectionalLight",    &addDirectionalLight);
    function("addPointLight",          &addPointLight);
    function("addText",                &addText);
    function("removeTransform",        &removeTransform);
    function("removeCamera",           &removeCamera);
    function("removeCameraController", &removeCameraController);
    function("removeDirectionalLight", &removeDirectionalLight);
    function("removePointLight",       &removePointLight);
    function("removeText",             &removeText);
}
