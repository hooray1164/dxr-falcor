#include "DiffuseGIPass.h"


namespace
{
	const std::string kRtShaderFile = "DXRBasic\\Shaders\\rtDiffuseGI.hlsl";
	//const std::string kEnvironmentMapFile = "MonValley_G_DirtRoad_3k.hdr";
	//const std::string kSceneFile = "Data/DXRBasic/BasicScene.fscene";
	//const std::string kSceneFile = "Data/pink_room/pink_room.fscene";

	const std::string kWPositionBuffer = "WorldPosition";
	const std::string kWNormalBuffer = "WorldNormal";
	const std::string kDiffuseBuffer = "DiffuseColor";
	const std::string kSpecBuffer = "SpecColor";

	const std::string kRayGen = "DiffuseGIRayGen";
	const std::string kShadowMiss = "ShadowMiss";
	const std::string kShadowAnyHit = "ShadowHit";

	const std::string kGIMiss = "GIMiss";
	const std::string kGIClosestHit = "GIClosestHit";
	const std::string kGIAnyHit = "GIAnyHit";
}


DiffuseGIPass::DiffuseGIPass(const std::string & outputBuffer)
	: ::RenderPass("Diffuse GI", "Diffuse GI Options"), mOutputBuffer(outputBuffer)
{
}

bool DiffuseGIPass::initialize(RenderContext * pRenderContext, ResourceManager::SharedPtr pResManager)
{
	mpResManager = pResManager;
	mpResManager->requestTextureResources({ kWPositionBuffer, 
		kWNormalBuffer, 
		kDiffuseBuffer,
		kSpecBuffer,
		ResourceManager::kEnvironmentMap,
		mOutputBuffer });

	mpRtShader = RayLaunch::create(kRtShaderFile, kRayGen);

	mpRtShader->addMissShader(kRtShaderFile, kShadowMiss);
	mpRtShader->addHitShader(kRtShaderFile, "", kShadowAnyHit);

	mpRtShader->addMissShader(kRtShaderFile, kGIMiss);
	mpRtShader->addHitShader(kRtShaderFile, kGIClosestHit, kGIAnyHit);

	mpRtShader->compileRayProgram();
	if (mpScene) { mpRtShader->setScene(mpScene); }

	// Create a tri-linear sampler for the Environment Map
	Sampler::Desc desc;
	desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(8);
	mpLinearSampler = Sampler::create(desc);

	return true;
}

void DiffuseGIPass::initScene(RenderContext * pRenderContext, Scene::SharedPtr pScene)
{
	mpScene = std::dynamic_pointer_cast<RtScene>(pScene);
	if (mpRtShader) { mpRtShader->setScene(mpScene); }
}

void DiffuseGIPass::renderGui(Gui * pGui)
{
	int dirty = 0;

	dirty |= (int)pGui->addCheckBox("Shadow Rays", mShadowRays);
	dirty |= (int)pGui->addCheckBox("GI Rays", mGIRays);

	if (dirty) { setRefreshFlag(); }
}

void DiffuseGIPass::execute(RenderContext * pRenderContext)
{
	auto dstTexture = mpResManager->getClearedTexture(mOutputBuffer, vec4(0.f, 0.f, 0.f, 0.f));

	if (!dstTexture || !mpRtShader || !mpRtShader->readyToRender()) { return; }

	auto rayGenVars = mpRtShader->getRayGenVars();

	// Pass constant buffer variables to the RayGen shader
	rayGenVars["RayGenCB"]["gMinT"] = mpResManager->getMinTDist();
	rayGenVars["RayGenCB"]["gFrameCount"] = mFrameCount++;
	rayGenVars["RayGenCB"]["gShadows"] = mShadowRays;
	rayGenVars["RayGenCB"]["gGI"] = mGIRays;

	// Pass GBuffer textures to the RayGen shader
	rayGenVars["gWorldPos"] = mpResManager->getTexture(kWPositionBuffer);
	rayGenVars["gWorldNorm"] = mpResManager->getTexture(kWNormalBuffer);
	rayGenVars["gDiffuseCol"] = mpResManager->getTexture(kDiffuseBuffer);
	rayGenVars["gSpecCol"] = mpResManager->getTexture(kSpecBuffer);
	rayGenVars["gOutput"] = dstTexture;

	// Pass Environment Map variables to the GI Miss shader
	auto missVars = mpRtShader->getMissVars(1);
	missVars["gEnvMap"] = mpResManager->getEnvironmentMap();
	missVars["gEnvSampler"] = mpLinearSampler;

	//mpScene->getActiveCamera()->setJitter(xOff / float(wPosTexture->getWidth()), yOff / float(wPosTexture->getHeight()));

	mpRtShader->execute(pRenderContext, mpResManager->getScreenSize());
}
