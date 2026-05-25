#include <pch.h>
#include "SceneSerializer.h"
#include "ComponentSerialization.h"

#include <fstream>

namespace cbk::scene {

	static nlohmann::json serializeComponents(const Scene& scene, ecs::Entity e) {
		const ecs::Registry* reg = scene.getRegistry();
		nlohmann::json comps = nlohmann::json::object();

		if (auto c = reg->getComponent<ecs::CTransform>(e))
			comps["CTransform"] = **c;
		if (auto c = reg->getComponent<ecs::CCamera>(e))
			comps["CCamera"] = **c;
		if (auto c = reg->getComponent<ecs::CCameraController>(e))
			comps["CCameraController"] = **c;
		if (auto c = reg->getComponent<ecs::CSprite>(e))
			comps["CSprite"] = **c;
		if (auto c = reg->getComponent<ecs::CDirectionalLight>(e))
			comps["CDirectionalLight"] = **c;
		if (auto c = reg->getComponent<ecs::CPointLight>(e))
			comps["CPointLight"] = **c;
		if (auto c = reg->getComponent<ecs::CModel>(e))
			comps["CModel"] = **c;
		if (auto c = reg->getComponent<ecs::CText>(e))
			comps["CText"] = **c;

		return comps;
	}

	static nlohmann::json::array_t serializeEntities(const Scene& scene) {
		nlohmann::json entities = nlohmann::json::array();
		for (ecs::Entity e: scene.getAllEntities()) {
			nlohmann::json node;
			node["name"] = scene.getEntityName(e);
			node["components"] = serializeComponents(scene, e);
			entities.push_back(std::move(node));
		}
		return entities;
	}

	static nlohmann::json toJson(const Scene& scene) {
		const auto& meta = scene.getMetadata();
		nlohmann::json root;
		root["version"] = 1;
		root["metadata"] = {
			{ "name", meta.Name },
			{ "background_color", meta.BackgroundColor },
			{ "ambient_color", meta.AmbientColor },
		};

		auto entities = serializeEntities(scene);
		root["entities"] = entities;
		return root;
	}

	static void deserializeComponents(Scene& scene, ecs::Entity e, const nlohmann::json& comps) {
		ecs::Registry* reg = scene.getRegistry();

		if (comps.contains("CTransform"))
			reg->addComponent(e, comps["CTransform"].get<ecs::CTransform>());
		if (comps.contains("CCamera"))
			reg->addComponent(e, comps["CCamera"].get<ecs::CCamera>());
		if (comps.contains("CCameraController"))
			reg->addComponent(e, comps["CCameraController"].get<ecs::CCameraController>());
		if (comps.contains("CSprite"))
			reg->addComponent(e, comps["CSprite"].get<ecs::CSprite>());
		if (comps.contains("CDirectionalLight"))
			reg->addComponent(e, comps["CDirectionalLight"].get<ecs::CDirectionalLight>());
		if (comps.contains("CPointLight"))
			reg->addComponent(e, comps["CPointLight"].get<ecs::CPointLight>());
		if (comps.contains("CModel"))
			reg->addComponent(e, comps["CModel"].get<ecs::CModel>());
		if (comps.contains("CText"))
			reg->addComponent(e, comps["CText"].get<ecs::CText>());
	}

	static Scene fromJson(const nlohmann::json& root) {
		SceneMetadata meta;
		const auto& m = root["metadata"];
        m["name"].get_to(meta.Name);
        m["background_color"].get_to(meta.BackgroundColor);
        m["ambient_color"].get_to(meta.AmbientColor);

		Scene scene(std::move(meta));

		if (root.contains("entities")) {
			for (const auto& node: root["entities"]) {
				auto e = scene.createEntity(node["name"].get<std::string>());
				if (node.contains("components"))
					deserializeComponents(scene, e, node["components"]);
			}
		}

		return scene;
	}

	void SceneSerializer::serialize(const Scene& scene, std::string_view path) {
		std::ofstream file{ path.data() };
		CBK_CORE_ASSERT(file.is_open(), "SceneSerializer: failed to open file for writing");
		file << toJson(scene).dump(4);
	}

	Scene SceneSerializer::deserialize(std::string_view path) {
		std::ifstream file{ std::string(path) };
		CBK_CORE_ASSERT(file.is_open(), "SceneSerializer: failed to open file for reading");
		return fromJson(nlohmann::json::parse(file));
	}

	std::string SceneSerializer::serializeToString(const Scene& scene) {
		return toJson(scene).dump();
	}

	Scene SceneSerializer::deserializeFromString(std::string_view json) {
		return fromJson(nlohmann::json::parse(json));
	}

} // namespace cbk::scene
