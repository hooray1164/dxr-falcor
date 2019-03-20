#include "GBufferPass.h"


namespace
{
	const std::string kRtShaderFile = "DXRBasic\\Shaders\\rtGBuffer.hlsl";
	const std::string kEnvironmentMapFile = "MonValley_G_DirtRoad_3k.hdr";
	const std::string kSceneFile = "Data/DXRBasic/BasicScene.fscene";
	//const std::string kSceneFile = "Data/pink_room/pink_room.fscene";

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

	mpRtShader = RayLaunch::create(kRtShaderFile, kPrimaryRayGen);
	mpRtShader->addMissShader(kRtShaderFile, kPrimaryMiss);
	mpRtShader->addHitShader(kRtShaderFile, kPrimaryClosestHit, "");
	mpRtShader->compileRayProgram();
	if (mpScene) { mpRtShader->setScene(mpScene); }

	// Create a tri-linear sampler for the Environment Map
	Sampler::Desc desc;
	desc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(8);
	mpLinearSampler = Sampler::create(desc);

	// Seed the RNG with the current time
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto timeInMillisec = std::chrono::time_point_cast<std::chrono::milliseconds>(currentTime);
	mRng = std::mt19937(uint32_t(timeInMillisec.time_since_epoch().count()));

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
	if (!mpRtShader || !mpRtShader->readyToRender()) { return; }

	auto blackRGB = vec4(1.f, 0.f, 0.f, 0.f);

	auto wPosTexture = mpResManager->getClearedTexture(kWPositionBuffer, blackRGB);
	auto wNormTexture = mpResManager->getClearedTexture(kWNormalBuffer, blackRGB);
	auto wDiffuseTexture = mpResManager->getClearedTexture(kDiffuseBuffer, blackRGB);

	// Pass variables to the Miss shader
	auto missVars = mpRtShader->getMissVars(0);
	missVars["gDiffuseCol"] = wDiffuseTexture;
	missVars["gEnvMap"] = mpResManager->getEnvironmentMap();
	missVars["gEnvSampler"] = mpLinearSampler;

	// Pass variables to the Hit shaders for each geometry instance
	for (auto hitVars : mpRtShader->getHitVars(0))
	{
		hitVars["gWorldPos"] = wPosTexture;
		hitVars["gWorldNorm"] = wNormTexture;
		hitVars["gDiffuseCol"] = wDiffuseTexture;
	}

	float xOff, yOff;
	xOff = mUniformDist(mRng);
	yOff = mUniformDist(mRng);

	// Pass variables to the RayGen shader
	auto rayGenVars = mpRtShader->getRayGenVars();
	rayGenVars["RayGenCB"]["gPixelJitter"] = vec2(xOff, yOff);
	//mpScene->getActiveCamera()->setJitter(xOff / float(wPosTexture->getWidth()), yOff / float(wPosTexture->getHeight()));

	mpRtShader->execute(pRenderContext, mpResManager->getScreenSize());
}
