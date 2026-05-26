#include <pch.h>

#include <string>

#include <emscripten/bind.h>

#include <nlohmann/json.hpp>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/EntryPoint.h>
#include <Cabrankengine/Renderer/RenderLayer.h>
#include <Cabrankengine/Scene/Scene.h>
#include <Cabrankengine/Scene/SceneSerializer.h>
#include <Cabrankengine/Scene/ComponentSerialization.h>
#include <Cabrankengine/ECS/Components.h>

using namespace cbk;
using namespace emscripten;

// EntryPoint.h provides main(); this supplies the Application factory it expects.
// JS clients drive the live scene via the bindings below.
namespace cbk {
    Application* createApplication() {
        return new Application();
    }
}

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

static void newScene(const std::string& name) {
    scene::SceneMetadata meta;
    meta.Name = name;
    Application::get().loadScene(scene::Scene(meta));
}

static std::string serializeScene() {
    return scene::SceneSerializer::serializeToString(Application::get().getScene());
}

static void deserializeScene(const std::string& json) {
    Application::get().loadScene(scene::SceneSerializer::deserializeFromString(json));
}

// ---------------------------------------------------------------------------
// Mode
// ---------------------------------------------------------------------------

static void setGameMode()   { rendering::RenderLayer::setEditorMode(false); }
static void setEditorMode() { rendering::RenderLayer::setEditorMode(true); }

// ---------------------------------------------------------------------------
// Entities
// ---------------------------------------------------------------------------

static int createEntity(const std::string& name) {
    ecs::Entity e = Application::get().getScene().createEntity(name);
    return static_cast<int>(e);
}

static void destroyEntity(int e) {
    Application::get().getScene().destroyEntity(e);
}

// ---------------------------------------------------------------------------
// Component getters — return JSON string, or "" if component is absent
// ---------------------------------------------------------------------------

template<typename C>
static std::string getComponent(int entityId) {
    auto opt = Application::get().getScene().getRegistry()->getComponent<C>(static_cast<ecs::Entity>(entityId));
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
// Component add — accepts JSON string; logs and returns silently on bad JSON
// ---------------------------------------------------------------------------

template<typename C>
static void addComponent(int entityId, const std::string& json) {
    auto j = nlohmann::json::parse(json, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        CBK_CORE_ERROR("addComponent: invalid JSON for entity {0}", entityId);
        return;
    }
    Application::get().getScene().getRegistry()->addComponent(static_cast<ecs::Entity>(entityId), j.get<C>());
}

static void addTransform(int e, const std::string& json)        { addComponent<ecs::CTransform>(e, json); }
static void addCamera(int e, const std::string& json)           { addComponent<ecs::CCamera>(e, json); }
static void addCameraController(int e, const std::string& json) { addComponent<ecs::CCameraController>(e, json); }
static void addDirectionalLight(int e, const std::string& json) { addComponent<ecs::CDirectionalLight>(e, json); }
static void addPointLight(int e, const std::string& json)       { addComponent<ecs::CPointLight>(e, json); }
static void addText(int e, const std::string& json)             { addComponent<ecs::CText>(e, json); }
static void addPhongModel(int e, const std::string& json)       { addComponent<ecs::CPhongModel>(e, json); }

// ---------------------------------------------------------------------------
// Component setters — overwrites existing component; logs and returns silently on bad JSON or missing component
// ---------------------------------------------------------------------------

template<typename C>
static void setComponent(int entityId, const std::string& json) {
    auto opt = Application::get().getScene().getRegistry()->getComponent<C>(static_cast<ecs::Entity>(entityId));
    if (!opt) {
        CBK_CORE_ERROR("setComponent: entity {0} has no component of this type", entityId);
        return;
    }
    auto j = nlohmann::json::parse(json, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        CBK_CORE_ERROR("setComponent: invalid JSON for entity {0}", entityId);
        return;
    }
    **opt = j.get<C>();
}

static void setTransform(int e, const std::string& json)        { setComponent<ecs::CTransform>(e, json); }
static void setCamera(int e, const std::string& json)           { setComponent<ecs::CCamera>(e, json); }
static void setCameraController(int e, const std::string& json) { setComponent<ecs::CCameraController>(e, json); }
static void setDirectionalLight(int e, const std::string& json) { setComponent<ecs::CDirectionalLight>(e, json); }
static void setPointLight(int e, const std::string& json)       { setComponent<ecs::CPointLight>(e, json); }
static void setText(int e, const std::string& json)             { setComponent<ecs::CText>(e, json); }

// ---------------------------------------------------------------------------
// Component remove
// ---------------------------------------------------------------------------

template<typename C>
static void removeComponent(int entityId) {
    Application::get().getScene().getRegistry()->removeComponent<C>(static_cast<ecs::Entity>(entityId));
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
    function("newScene",               &newScene);
    function("serializeScene",         &serializeScene);
    function("deserializeScene",       &deserializeScene);
    function("setGameMode",            &setGameMode);
    function("setEditorMode",          &setEditorMode);
    function("createEntity",           &createEntity);
    function("destroyEntity",          &destroyEntity);
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
    function("addPhongModel",          &addPhongModel);
    function("removeTransform",        &removeTransform);
    function("removeCamera",           &removeCamera);
    function("removeCameraController", &removeCameraController);
    function("removeDirectionalLight", &removeDirectionalLight);
    function("removePointLight",       &removePointLight);
    function("removeText",             &removeText);
}
