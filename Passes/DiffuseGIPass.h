#pragma once

#include "../SharedUtils/RenderPass.h"
#include "../SharedUtils/RayLaunch.h"
#include <random>


class DiffuseGIPass : public ::RenderPass, inherit_shared_from_this<::RenderPass, DiffuseGIPass>
{
public:
	using SharedPtr = std::shared_ptr<DiffuseGIPass>;
	
	static SharedPtr create() { return SharedPtr(new DiffuseGIPass()); }
	virtual ~DiffuseGIPass() = default;

protected:
	DiffuseGIPass() : ::RenderPass("Diffuse GI", "Diffuse GI Options") {}

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
	RayLaunch::SharedPtr					mpRtShader;
	RtScene::SharedPtr						mpScene;

	// Environment Map sampler
	Sampler::SharedPtr						mpLinearSampler;

	// State for random number generator to calculate pixel jitter
	// std::uniform_real_distribution<float>	mUniformDist;
	// std::mt19937							mRng;

	// Toggles for shadow and GI rays
	bool									mShadowRays = true;
	bool									mGIRays = true;

	// Frame count to seed the RNG for shadow and GI rays
	uint32_t								mFrameCount = 0xdeadbeef;
};
