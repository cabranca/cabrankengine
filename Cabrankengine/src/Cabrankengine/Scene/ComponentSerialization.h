#pragma once

#include <nlohmann/json.hpp>

#include <Cabrankengine/ECS/Components.h>

// nlohmann ADL serialization — functions live in the same namespace as their types.

namespace cbk::math {

inline void to_json(nlohmann::json& j, const Vector3& v) {
    j = { v.x, v.y, v.z };
}

inline void from_json(const nlohmann::json& j, Vector3& v) {
    v.x = j.at(0).get<float>();
    v.y = j.at(1).get<float>();
    v.z = j.at(2).get<float>();
}

inline void to_json(nlohmann::json& j, const Vector4& v) {
    j = { v.x, v.y, v.z, v.w };
}

inline void from_json(const nlohmann::json& j, Vector4& v) {
    v.x = j.at(0).get<float>();
    v.y = j.at(1).get<float>();
    v.z = j.at(2).get<float>();
    v.w = j.at(3).get<float>();
}

} // namespace cbk::math

namespace cbk::ecs {

NLOHMANN_JSON_SERIALIZE_ENUM(ProjectionType, {
    { ProjectionType::Perspective,  "perspective"  },
    { ProjectionType::Orthographic, "orthographic" },
})

inline void to_json(nlohmann::json& j, const CTransform& c) {
    j = {
        { "position", c.Position },
        { "rotation", c.Rotation },
        { "scale",    c.Scale    },
    };
}

inline void from_json(const nlohmann::json& j, CTransform& c) {
    j.at("position").get_to(c.Position);
    j.at("rotation").get_to(c.Rotation);
    j.at("scale").get_to(c.Scale);
}

inline void to_json(nlohmann::json& j, const CCamera& c) {
    j = {
        { "type",       c.Type      },
        { "is_active",  c.IsActive  },
        { "fov_y",      c.FovY      },
        { "near",       c.Near      },
        { "far",        c.Far       },
        { "ortho_size", c.OrthoSize },
        // AspectRatio is omitted — set by RenderLayer on window resize
    };
}

inline void from_json(const nlohmann::json& j, CCamera& c) {
    j.at("type").get_to(c.Type);
    j.at("is_active").get_to(c.IsActive);
    j.at("fov_y").get_to(c.FovY);
    j.at("near").get_to(c.Near);
    j.at("far").get_to(c.Far);
    j.at("ortho_size").get_to(c.OrthoSize);
}

inline void to_json(nlohmann::json& j, const CCameraController& c) {
    j = {
        { "translation_speed", c.TranslationSpeed },
        { "mouse_sensitivity", c.MouseSensitivity },
        { "yaw",               c.Yaw              },
        { "pitch",             c.Pitch            },
    };
    // LastMouseX, LastMouseY, MouseCaptured are runtime bookkeeping — not persisted
}

inline void from_json(const nlohmann::json& j, CCameraController& c) {
    j.at("translation_speed").get_to(c.TranslationSpeed);
    j.at("mouse_sensitivity").get_to(c.MouseSensitivity);
    j.at("yaw").get_to(c.Yaw);
    j.at("pitch").get_to(c.Pitch);
    c.LastMouseX    = 0.f;
    c.LastMouseY    = 0.f;
    c.MouseCaptured = false;
}

inline void to_json(nlohmann::json& j, const CSprite& c) {
    j = {
        { "path",          c.Path         },
        { "tint",          c.Tint         },
        { "tiling_factor", c.TilingFactor },
        // Texture ref is omitted — recreated from Path on load
    };
}

inline void from_json(const nlohmann::json& j, CSprite& c) {
    j.at("path").get_to(c.Path);
    j.at("tint").get_to(c.Tint);
    j.at("tiling_factor").get_to(c.TilingFactor);
}

inline void to_json(nlohmann::json& j, const CPhongModel& c) {
    j = { { "path", c.Path } };
    // Model Ref is omitted — recreated from Path on load
}

inline void from_json(const nlohmann::json& j, CPhongModel& c) {
    j.at("path").get_to(c.Path);
}

inline void to_json(nlohmann::json& j, const CPBRModel& c) {
    j = { { "path", c.Path } };
    // Model Ref is omitted — recreated from Path on load
}

inline void from_json(const nlohmann::json& j, CPBRModel& c) {
    j.at("path").get_to(c.Path);
}

inline void to_json(nlohmann::json& j, const CText& c) {
    j = {
        { "text",       c.Text      },
        { "font_scale", c.FontScale },
        { "color",      c.Color     },
    };
}

inline void from_json(const nlohmann::json& j, CText& c) {
    j.at("text").get_to(c.Text);
    j.at("font_scale").get_to(c.FontScale);
    j.at("color").get_to(c.Color);
}

inline void to_json(nlohmann::json& j, const CDirectionalLight& c) {
    j = {
        { "direction", c.Direction },
        { "radiance",  c.Radiance  },
    };
}

inline void from_json(const nlohmann::json& j, CDirectionalLight& c) {
    j.at("direction").get_to(c.Direction);
    j.at("radiance").get_to(c.Radiance);
}

inline void to_json(nlohmann::json& j, const CPointLight& c) {
    j = {
        { "radiance",  c.Radiance  },
        { "constant",  c.Constant  },
        { "linear",    c.Linear    },
        { "quadratic", c.Quadratic },
    };
}

inline void from_json(const nlohmann::json& j, CPointLight& c) {
    j.at("radiance").get_to(c.Radiance);
    j.at("constant").get_to(c.Constant);
    j.at("linear").get_to(c.Linear);
    j.at("quadratic").get_to(c.Quadratic);
}

} // namespace cbk::ecs
