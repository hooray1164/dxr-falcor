#include "GBufferPass.h"


namespace
{
	const std::string kRtShaderFile = "DXRBasic/Shaders/rtGBuffer.hlsl";
	const std::string kEnvironmentMapFile = "MonValley_G_DirtRoad_3k.hdr";
	const std::string kSceneFile = "DXRBasic/BasicScene.fscene";

	const std::string kWPositionBuffer = "WorldPosition";
	const std::string kWNormalBuffer = "WorldNormal";
	const std::string kDiffuseBuffer = "DiffuseColor";

	const std::string kPrimaryRayGen = "GBufferRayGen";
	const std::string kPrimaryMiss = "PrimaryMiss";
	const std::string kPrimaryClosestHit = "PrimaryClosestHit";
}


bool GBufferPass::initialize(RenderContext * pRenderContext, ResourceManager::SharedPtr pResManager)
{
	mpResManager = pResManager;
	mpResManager->requestTextureResources({ kWPositionBuffer, kWNormalBuffer, kDiffuseBuffer });

	mpResManager->updateEnvironmentMap(kEnvironmentMapFile);
	mpResManager->setDefaultSceneName(kSceneFile);

	mpRtShader = RayLaunch::create(kRtShaderFile, kPrimaryRayGen, 1);
	mpRtShader->addMissShader(kRtShaderFile, kPrimaryMiss);
	mpRtShader->addHitShader(kRtShaderFile, kPrimaryClosestHit, "");
	mpRtShader->compileRayProgram();
	if (mpScene) { mpRtShader->setScene(mpScene); }

	Sampler::Desc desc;
	desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(8);
	mpLinearSampler = Sampler::create(desc);

	return true;
}

void GBufferPass::initScene(RenderContext * pRenderContext, Scene::SharedPtr pScene)
{
	mpScene = std::dynamic_pointer_cast<RtScene>(pScene);
	if (mpRtShader) { mpRtShader->setScene(mpScene); }
}

void GBufferPass::renderGui(Gui * pGui)
{
}

void GBufferPass::execute(RenderContext * pRenderContext)
{
}
