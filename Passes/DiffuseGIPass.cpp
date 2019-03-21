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
}


bool DiffuseGIPass::initialize(RenderContext * pRenderContext, ResourceManager::SharedPtr pResManager)
{
	mpResManager = pResManager;
	mpResManager->requestTextureResources({ kWPositionBuffer, 
		kWNormalBuffer, 
		kDiffuseBuffer,
		kSpecBuffer,
		ResourceManager::kEnvironmentMap,
		ResourceManager::kOutputChannel });

	mpRtShader = RayLaunch::create(kRtShaderFile, kRayGen);
	mpRtShader->addMissShader(kRtShaderFile, kShadowMiss);
	mpRtShader->addHitShader(kRtShaderFile, "", kShadowAnyHit);
	mpRtShader->compileRayProgram();
	if (mpScene) { mpRtShader->setScene(mpScene); }

	// Create a tri-linear sampler for the Environment Map
	Sampler::Desc desc;
	desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(8);
	mpLinearSampler = Sampler::create(desc);

	// Seed the RNG with the current time
	// auto currentTime = std::chrono::high_resolution_clock::now();
	// auto timeInMillisec = std::chrono::time_point_cast<std::chrono::milliseconds>(currentTime);
	// mRng = std::mt19937(uint32_t(timeInMillisec.time_since_epoch().count()));

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
	auto dstTexture = mpResManager->getClearedTexture(ResourceManager::kOutputChannel, vec4(0.f, 0.f, 0.f, 0.f));

	if (!dstTexture || !mpRtShader || !mpRtShader->readyToRender()) { return; }

	// Pass variables to the Miss shader
	// auto missVars = mpRtShader->getMissVars(0);
	// missVars["gDiffuseCol"] = wDiffuseTexture;
	// missVars["gEnvMap"] = mpResManager->getEnvironmentMap();
	// missVars["gEnvSampler"] = mpLinearSampler;

	// Pass variables to the Hit shaders for each geometry instance
	// for (auto hitVars : mpRtShader->getHitVars(0))
	// {
	// 	hitVars["gWorldPos"] = wPosTexture;
	// 	hitVars["gWorldNorm"] = wNormTexture;
	// 	hitVars["gDiffuseCol"] = wDiffuseTexture;
	// }

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

	//mpScene->getActiveCamera()->setJitter(xOff / float(wPosTexture->getWidth()), yOff / float(wPosTexture->getHeight()));

	mpRtShader->execute(pRenderContext, mpResManager->getScreenSize());
}
