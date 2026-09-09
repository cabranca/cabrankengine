#include <pch.h>
#include "Renderer.h"

#include <Common/BinaryFormats.h>

#include <Cabrankengine/Scene/DefaultLibrary.h>

#include "GeometryDescriptor.h"
#include "Materials/Material.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "TextRenderer.h"

namespace cbk::rendering {

	using namespace math;
	using namespace scene;

	namespace {

		// One submission, held until endScene() replays it.
		struct DrawEntry {
			Ref<Material> Mat;
			Ref<GeometryDescriptor> Desc;
			Mat4 Xf;
		};

		// One queue per material kind, so draws leave here grouped and each backend pipeline
		// gets bound as a single contiguous run instead of once per mesh. Bucketing rather
		// than sorting a flat list: the key domain is fixed at compile time, so this is a
		// counting sort — no comparator, no key stored per entry, and submission order is
		// preserved within a kind for free.
		std::array<std::vector<DrawEntry>, common::k_MaterialKindCount> s_Queues;

	} // namespace

	void Renderer::init(const Window& window, const RendererSpec& spec) {
		CBK_PROFILE_FUNCTION();

		RenderCommand::init(window, spec);
		// DefaultLibrary::init();
		// Renderer2D::init();
		// TextRenderer::init();
	}

	void Renderer::shutdown() {
		// The queues hold Refs to GPU-owning materials and geometry. They are empty in
		// practice (endScene clears every frame), but a frame abandoned mid-submission
		// would otherwise keep those alive past vmaDestroyAllocator — see KnownIssues.md #1.
		for (auto& queue: s_Queues)
			queue.clear();

		// DefaultLibrary::shutdown();
		ShaderLibrary::shutdown(); // This should be in the same class that initialize it.
		// TextRenderer::shutdown();
		// Renderer2D::shutdown();

		RenderCommand::shutdown();
	}

	void Renderer::waitIdle() {
		RenderCommand::waitIdle();
	}

	void Renderer::beginScene(const SceneData& sceneData) {
		RenderCommand::beginScene(sceneData);
	}

	void Renderer::endScene() {
		// Kind order is the enumerator order of common::MaterialKind. Nothing depends on
		// which kind goes first — only on each one being contiguous.
		for (auto& queue: s_Queues) {
			for (const auto& entry: queue)
				RenderCommand::drawIndexed(entry.Mat, entry.Desc, entry.Xf, entry.Desc->getIndexCount());

			// clear(), not shrink: the capacity is worth keeping across frames.
			queue.clear();
		}
	}

	void Renderer::submit(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const Mat4& transform) {
		const auto kind = static_cast<uint32_t>(material->getKind());
		CBK_CORE_ASSERT(kind < common::k_MaterialKindCount, "Renderer::submit(): material kind out of range");
		s_Queues[kind].push_back({ material, desc, transform });
	}

	void Renderer::onWindowResize(uint32_t width, uint32_t height) {
		RenderCommand::setViewport(0, 0, width, height);
	}
} // namespace cbk::rendering
