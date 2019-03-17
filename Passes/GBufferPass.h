#pragma once

#include "../SharedUtils/RenderPass.h"
#include "../SharedUtils/RayLaunch.h"


class GBufferPass : public ::RenderPass, inherit_shared_from_this<::RenderPass, GBufferPass>
{
public:
	using SharedPtr = std::shared_ptr<GBufferPass>;
	
	static SharedPtr create() { return SharedPtr(new GBufferPass()); }
	virtual ~GBufferPass() = default;

protected:
	GBufferPass() : ::RenderPass("Ray Traced G-Buffer", "G-Buffer Options") {}

	// Implementation of RenderPass interface
	bool initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager) override;
	void initScene(RenderContext* pRenderContext, Scene::SharedPtr pScene) override;
	void renderGui(Gui* pGui) override;
	void execute(RenderContext* pRenderContext) override;

	// Override RenderPass methods
	bool requiresScene() override { return true; }
	bool usesRayTracing() override { return true; }
	bool usesEnvironmentMap() override { return true; }

	// Internal state
	RayLaunch::SharedPtr		mpRtShader;
	RtScene::SharedPtr			mpScene;

	// Environment Map sampler
	Sampler::SharedPtr			mpLinearSampler;
};
