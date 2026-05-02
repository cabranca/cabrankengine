#pragma once
#include <pch.h>

// For use by Cabrankengine Application
#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/AudioEngine.h>
#include <Cabrankengine/Core/Layer.h>
#include <Cabrankengine/Core/Logger.h>
#include <Cabrankengine/Core/Timestep.h>

#include <Cabrankengine/Core/Input.h>
#include <Cabrankengine/Core/KeyCodes.h>
#include <Cabrankengine/Core/MouseCodes.h>

#include <Cabrankengine/Debug/Instrumentator.h>

#include <Cabrankengine/ECS/ECS.h>

#include <Cabrankengine/Events/KeyEvent.h>
#include <Cabrankengine/Events/MouseEvent.h>

#include <Cabrankengine/ImGui/ImGuiLayer.h>

#include <Cabrankengine/Math/MatrixFactory.h>


// --- Renderer ---
#include <Cabrankengine/Renderer/Materials/PBRMaterial.h>
#include <Cabrankengine/Renderer/Materials/PhongMaterial.h>
#include <Cabrankengine/Renderer/Renderer.h>
#include <Cabrankengine/Renderer/Renderer2D.h>
#include <Cabrankengine/Renderer/RenderCommand.h>
#include <Cabrankengine/Renderer/Buffer.h>
#include <Cabrankengine/Renderer/FrameBuffer.h>
#include <Cabrankengine/Renderer/Shader.h>
#include <Cabrankengine/Renderer/TextRenderer.h>
#include <Cabrankengine/Renderer/Texture.h>
#include <Cabrankengine/Renderer/VertexArray.h>

// --- Scene ---
#include <Cabrankengine/Scene/DefaultLibrary.h>
#include <Cabrankengine/Scene/Model.h>
#include <Cabrankengine/Scene/SceneSerializer.h>
#include <Cabrankengine/Scene/Archetypes/Archetypes.h>
