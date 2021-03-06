// Tip: Hold Left ALT + SHIFT while tapping or holding the arrow keys in order to select multiple columns and write on them at once. 
//		Also useful for copy & paste operations in which you need to copy a bunch of variable or function names and you can't afford the time of copying them one by one.
#include "application.h"

#include "llc_bitmap_target.h"
#include "llc_bitmap_file.h"
#include "llc_gui_text.h"

#include "llc_app_impl.h"
#include "llc_ro_rsm.h"
#include "llc_ro_rsw.h"
#include "llc_encoding.h"

#include <process.h>

static constexpr	const uint32_t										ASCII_SCREEN_WIDTH							= 132	;
static constexpr	const uint32_t										ASCII_SCREEN_HEIGHT							= 50	;

LLC_DEFINE_APPLICATION_ENTRY_POINT(::SApplication);	

					::SApplication										* g_ApplicationInstance						= 0;

static				::llc::error_t										updateSizeDependentResources				(::SApplication& applicationInstance)											{ 
	const ::llc::SCoord2<uint32_t>												newSize										= applicationInstance.Framework.MainDisplay.Size;// / 2; 
	::llc::STexture<::llc::SColorBGRA>											& offscreen									= applicationInstance.Framework.Offscreen;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	if(newSize != offscreenMetrics) {
		::llc::SMatrix4<float>														& finalProjection							= applicationInstance.Scene.Transforms.FinalProjection	;
		::llc::SMatrix4<float>														& fieldOfView								= applicationInstance.Scene.Transforms.FieldOfView		;
		::llc::SMatrix4<float>														& viewport									= applicationInstance.Scene.Transforms.Viewport			;
		::llc::SMatrix4<float>														& viewportInverse							= applicationInstance.Scene.Transforms.ViewportInverse	;
		::llc::SMatrix4<float>														& viewportInverseCentered					= applicationInstance.Scene.Transforms.ViewportInverse	;
		fieldOfView.FieldOfView(applicationInstance.Scene.Camera.Range.Angle * ::llc::math_pi, newSize.x / (double)newSize.y, applicationInstance.Scene.Camera.Range.Near, applicationInstance.Scene.Camera.Range.Far);
		viewport.Viewport(newSize, applicationInstance.Scene.Camera.Range.Far, applicationInstance.Scene.Camera.Range.Near);
		viewportInverse															= viewport.GetInverse();
		const ::llc::SCoord2<int32_t>												screenCenter								= {(int32_t)newSize.x / 2, (int32_t)newSize.y / 2};
		viewportInverseCentered													= viewportInverse;
		viewportInverseCentered._41												+= screenCenter.x;
		viewportInverseCentered._42												+= screenCenter.y;
		finalProjection															= fieldOfView * viewportInverseCentered;
		applicationInstance.Scene.Transforms.FinalProjectionInverse				= finalProjection.GetInverse();
	}
	llc_necall(::llc::updateSizeDependentTarget(applicationInstance.Framework.Offscreen				, newSize), "??");
	llc_necall(::llc::updateSizeDependentTarget(applicationInstance.Framework.OffscreenDepthStencil	, newSize), "??");
	
	return 0;
}
// --- Cleanup application resources.
					::llc::error_t										cleanup										(::SApplication& applicationInstance)											{
	::llc::SDisplayPlatformDetail												& displayDetail								= applicationInstance.Framework.MainDisplay.PlatformDetail;
	if(displayDetail.WindowHandle) {
		error_if(0 == ::DestroyWindow(displayDetail.WindowHandle), "Not sure why would this fail.");
		error_if(errored(::llc::displayUpdate(applicationInstance.Framework.MainDisplay)), "Not sure why this would fail");
	}
	::UnregisterClass(displayDetail.WindowClassName, displayDetail.WindowClass.hInstance);
	bool																		waiting										= true;
	for(uint32_t iThread = 0, threadCount = ::llc::size(applicationInstance.Threads.Handles); iThread < threadCount; ++iThread) 
		applicationInstance.Threads.States[iThread].RequestedClose				= true;
	while(waiting) {
		waiting																	= false;
		for(uint32_t iThread = 0, threadCount = ::llc::size(applicationInstance.Threads.Handles); iThread < threadCount; ++iThread) {
			if(false == applicationInstance.Threads.States[iThread].Closed) {
				waiting																	= true;
				break;
			}
		}
	}

	g_ApplicationInstance													= 0;
	return 0;
}

static				void												myThread									(void* _applicationThreads)														{
	::SThreadArgs																& threadArgs								= *(::SThreadArgs*)_applicationThreads;
	::SApplicationThreads														& applicationThreads						= *threadArgs.ApplicationThreads;
	int32_t																		threadId									= threadArgs.ThreadId;
	while(false == applicationThreads.States[threadId].RequestedClose) {
		Sleep(10);
	}
	applicationThreads.States[threadId].Closed								= true;
}

static				::llc::error_t										setupThreads								(::SApplication& applicationInstance)													{
	for(uint32_t iThread = 0, threadCount = ::llc::size(applicationInstance.Threads.Handles); iThread < threadCount; ++iThread) {
		applicationInstance.Threads.States	[iThread]							= {true,};									
		applicationInstance.Threads.Handles	[iThread]							= _beginthread(myThread, 0, &(applicationInstance.ThreadArgs[iThread] = {&applicationInstance.Threads, (int32_t)iThread}));
	}
	return 0;
}

static				::llc::error_t										bmpOrBmgLoad								(::llc::view_string bmpFileName, ::llc::STexture<::llc::SColorBGRA>& loaded)		{
	::llc::view_const_string													bmpFileNameC								= {bmpFileName.begin(), bmpFileName.size()};
	if(errored(::llc::bmpFileLoad(bmpFileNameC, loaded))) {
		error_printf("Failed to load bitmap from file: %s.", bmpFileNameC.begin());
		bmpFileName[bmpFileName.size() - 2]										= 'g';
		llc_necall(::llc::bmgFileLoad(bmpFileNameC, loaded), "Failed to load bitmap from file: %s.", bmpFileNameC.begin());
	}
	::llc::STexture<::llc::SColorBGRA>											original									= loaded;
	for(uint32_t y = 0; y < original.View.metrics().y; ++y)
		memcpy(loaded[y].begin(), original[original.View.metrics().y - 1 - y].begin(), sizeof(::llc::SColorBGRA) * original.View.metrics().x);
	return 0;
}

					::llc::error_t										mainWindowCreate							(::llc::SDisplay& mainWindow, HINSTANCE hInstance);

static				::llc::error_t										blendGNDNormals								(const ::llc::grid_view<::llc::STileGeometryGND> &tileGeometryView, const ::llc::array_view<::llc::STileSkinGND>& lstTileSkinData, const ::llc::grid_view<::llc::STileMapping>& tileMappingView, ::llc::array_view<::llc::SModelNodeGND> & gndModelNodes)																						{
	for(uint32_t y = 0; y < tileGeometryView.metrics().y - 1; ++y) {
		const ::llc::array_view<const ::llc::STileGeometryGND>						rowTileGeometry								= tileGeometryView	[y];
		const ::llc::array_view<const ::llc::STileMapping	   >					rowTileMapping								= tileMappingView	[y];
		const ::llc::array_view<const ::llc::STileGeometryGND>						rowNextTileGeometry							= tileGeometryView	[y + 1];
		const ::llc::array_view<const ::llc::STileMapping	   >					rowNextTileMapping							= tileMappingView	[y + 1];
		for(uint32_t x = 0; x < tileGeometryView.metrics().x - 1; ++x) {
			const ::llc::STileGeometryGND												& tileGeometry0								= rowTileGeometry		[x];
			const ::llc::STileGeometryGND												& tileGeometry1								= rowTileGeometry		[x + 1];
			const ::llc::STileGeometryGND												& tileGeometry2								= rowNextTileGeometry	[x];
			const ::llc::STileGeometryGND												& tileGeometry3								= rowNextTileGeometry	[x + 1];

			const ::llc::STileMapping													& tileMapping0								= rowTileMapping	[x + 0];
			const ::llc::STileMapping													& tileMapping1								= rowTileMapping	[x + 1];
			const ::llc::STileMapping													& tileMapping2								= rowNextTileMapping[x + 0];
			const ::llc::STileMapping													& tileMapping3								= rowNextTileMapping[x + 1];

			const bool																	processTile0								= (tileGeometry0.SkinMapping.SkinIndexTop != -1);// && (applicationInstance.GNDData.lstTileTextureData[tileGeometry0.SkinMapping.SkinIndexTop].TextureIndex != -1);
			const bool																	processTile1								= (tileGeometry1.SkinMapping.SkinIndexTop != -1) && tileGeometry0.SkinMapping.SkinIndexFront == -1;// && (applicationInstance.GNDData.lstTileTextureData[tileGeometry1.SkinMapping.SkinIndexTop].TextureIndex != -1);
			const bool																	processTile2								= (tileGeometry2.SkinMapping.SkinIndexTop != -1) && tileGeometry0.SkinMapping.SkinIndexRight == -1;// && (applicationInstance.GNDData.lstTileTextureData[tileGeometry2.SkinMapping.SkinIndexTop].TextureIndex != -1);
			const bool																	processTile3								= (tileGeometry3.SkinMapping.SkinIndexTop != -1) && (tileGeometry0.SkinMapping.SkinIndexFront == -1 && tileGeometry0.SkinMapping.SkinIndexRight == -1);// && (applicationInstance.GNDData.lstTileTextureData[tileGeometry3.SkinMapping.SkinIndexTop].TextureIndex != -1);
			const int32_t																texIndex0									= processTile0 ? lstTileSkinData[tileGeometry0.SkinMapping.SkinIndexTop].TextureIndex : -1;
			const int32_t																texIndex1									= processTile1 ? lstTileSkinData[tileGeometry1.SkinMapping.SkinIndexTop].TextureIndex : -1;
			const int32_t																texIndex2									= processTile2 ? lstTileSkinData[tileGeometry2.SkinMapping.SkinIndexTop].TextureIndex : -1;
			const int32_t																texIndex3									= processTile3 ? lstTileSkinData[tileGeometry3.SkinMapping.SkinIndexTop].TextureIndex : -1;
			::llc::SCoord3<float>														normal										= {};
			uint32_t																	divisor										= 0;
			if(processTile0) { ++divisor; ::llc::SModelNodeGND & gndNode0 = gndModelNodes[texIndex0]; normal += gndNode0.Normals[tileMapping0.VerticesTop[3]]; }
			if(processTile1) { ++divisor; ::llc::SModelNodeGND & gndNode1 = gndModelNodes[texIndex1]; normal += gndNode1.Normals[tileMapping1.VerticesTop[2]]; }
			if(processTile2) { ++divisor; ::llc::SModelNodeGND & gndNode2 = gndModelNodes[texIndex2]; normal += gndNode2.Normals[tileMapping2.VerticesTop[1]]; }
			if(processTile3) { ++divisor; ::llc::SModelNodeGND & gndNode3 = gndModelNodes[texIndex3]; normal += gndNode3.Normals[tileMapping3.VerticesTop[0]]; }
			if(divisor) {
				(normal /= divisor).Normalize();
				if(processTile0) { ::llc::SModelNodeGND	& gndNode0 = gndModelNodes[texIndex0]; gndNode0.Normals[tileMapping0.VerticesTop[3]] = normal; }
				if(processTile1) { ::llc::SModelNodeGND	& gndNode1 = gndModelNodes[texIndex1]; gndNode1.Normals[tileMapping1.VerticesTop[2]] = normal; }
				if(processTile2) { ::llc::SModelNodeGND	& gndNode2 = gndModelNodes[texIndex2]; gndNode2.Normals[tileMapping2.VerticesTop[1]] = normal; }
				if(processTile3) { ::llc::SModelNodeGND	& gndNode3 = gndModelNodes[texIndex3]; gndNode3.Normals[tileMapping3.VerticesTop[0]] = normal; }
			}
		}
	}
	return 0;
}

static				::llc::error_t										setup										(::SApplication& applicationInstance)													{
	g_ApplicationInstance													= &applicationInstance;
	
	error_if(errored(setupThreads(applicationInstance)), "Unknown.");
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	error_if(errored(::mainWindowCreate(mainWindow, applicationInstance.Framework.RuntimeValues.PlatformDetail.EntryPointArgs.hInstance)), "Failed to create main window why?????!?!?!?!?");
	char																		bmpFileName2	[]							= "..\\gpk_data\\images\\Codepage-437-24.bmp";
	error_if(errored(::bmpOrBmgLoad(bmpFileName2, applicationInstance.TextureFont)), "");
	const ::llc::SCoord2<uint32_t>												& textureFontMetrics						= applicationInstance.TextureFont.View.metrics();
	llc_necall(applicationInstance.TextureFontMonochrome.resize(textureFontMetrics), "Whou would we failt ro resize=");
	for(uint32_t y = 0, yMax = textureFontMetrics.y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = textureFontMetrics.x; x < xMax; ++x) {
		const ::llc::SColorBGRA														& srcColor									= applicationInstance.TextureFont.View[y][x];
		applicationInstance.TextureFontMonochrome.View[y * textureFontMetrics.x + x]	
			=	0 != srcColor.r
			||	0 != srcColor.g
			||	0 != srcColor.b
			;
	}

	static constexpr const char													ragnaPath	[]								= "..\\data_2006\\data\\";
	char																		temp		[512]							= {};
	::llc::SRSWFileContents														& rswData									= applicationInstance.RSWData;
	::llc::SGNDFileContents														& gndData									= applicationInstance.GNDData;
	sprintf_s(temp, "%s%s%s", ragnaPath, "", "niflheim.rsw");			llc_necall(::llc::rswFileLoad(rswData, ::llc::view_const_string(temp)), "Error");
	sprintf_s(temp, "%s%s%s", ragnaPath, "", &rswData.GNDFilename[0]);	llc_necall(::llc::gndFileLoad(gndData, ::llc::view_const_string(temp)), "Error");

	applicationInstance.RSMData.resize(rswData.RSWModels.size());
	for(uint32_t iRSM = 0; iRSM < (uint32_t)rswData.RSWModels.size(); ++iRSM){
		::llc::SRSMFileContents														& rsmData									= applicationInstance.RSMData[iRSM];
		::llc::view_const_string													rsmFilename									= rswData.RSWModels[iRSM].Filename;
		bool																		bNotLoaded									= true;
		for(uint32_t iLoaded = 0; iLoaded < iRSM; ++iLoaded)
			if(0 == strcmp(&rsmFilename[0], &rswData.RSWModels[iLoaded].Filename[0])) {
				bNotLoaded																= false;
				break;
			}
		if(bNotLoaded) {
			sprintf_s(temp, "%s%s%s", ragnaPath, "model\\", &rsmFilename[0]);	
			error_if(errored(::llc::rsmFileLoad(rsmData, ::llc::view_const_string(temp))), "Failed to load file: %s.", temp);
		}
	}
	for(uint32_t iLight = 0; iLight < rswData.RSWLights.size(); ++iLight) {
		rswData.RSWLights[iLight].Position										*= 1.0 / gndData.Metrics.TileScale;
		rswData.RSWLights[iLight].Position										+= ::llc::SCoord3<float>{gndData.Metrics.Size.x / 2.0f, 0.0f, (gndData.Metrics.Size.y / 2.0f)};
		rswData.RSWLights[iLight].Position.y									*= -1;
	}

	applicationInstance.TexturesGND.resize(gndData.TextureNames.size());
	for(uint32_t iTex = 0; iTex < gndData.TextureNames.size(); ++ iTex) {
		sprintf_s(temp, "%s%s%s", ragnaPath, "texture\\", &gndData.TextureNames[iTex][0]);	
		error_if(errored(::llc::bmpFileLoad(::llc::view_const_string(temp), applicationInstance.TexturesGND[iTex])), "Not found? %s.", temp);
	}

	applicationInstance.GNDModel.Nodes		.resize(gndData.TextureNames.size() * 6);
	applicationInstance.GNDModel.TileMapping.resize(gndData.Metrics.Size);
	// -- Generate minimap
	::llc::grid_view<::llc::STileGeometryGND>									tileGeometryView								= {gndData.lstTileGeometryData.begin(), gndData.Metrics.Size};
	::llc::SMinMax<float>														heightMinMax									= {};
	for(uint32_t iTile = 0; iTile < gndData.lstTileGeometryData.size(); ++iTile)
		if(gndData.lstTileGeometryData[iTile].SkinMapping.SkinIndexTop != -1) {
			for(uint32_t iHeight = 0; iHeight < 4; ++iHeight) {
				heightMinMax.Max												= ::llc::max(gndData.lstTileGeometryData[iTile].fHeight[iHeight] * -1, heightMinMax.Max);
				heightMinMax.Min												= ::llc::min(gndData.lstTileGeometryData[iTile].fHeight[iHeight] * -1, heightMinMax.Min);
			}
		}
	applicationInstance.TextureMinimap.resize(gndData.Metrics.Size);
	::llc::grid_view<::llc::SColorBGRA>											& minimapView									= applicationInstance.TextureMinimap.View;
	const float																	heightRange										= heightMinMax.Max - heightMinMax.Min;
	for(uint32_t y = 0, yMax = minimapView.metrics().y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = minimapView.metrics().x; x < xMax; ++x) {
		float																		fAverageHeight									= 0;
		for(uint32_t iHeight = 0; iHeight < 4; ++iHeight)
			fAverageHeight															+= tileGeometryView[y][x].fHeight[iHeight] * -1;
		fAverageHeight															*= .25f;
		float																		colorRatio										= (fAverageHeight - heightMinMax.Min) / heightRange;
		//minimapView[y][minimapView.metrics().x - 1 - x]							= ((fAverageHeight < 0) ? ::llc::WHITE : ::llc::BLUE) * colorRatio;
		minimapView[minimapView.metrics().y - 1 - y][x]							= ::llc::WHITE * colorRatio;
	}

	for(uint32_t iTex = 0, texCount = gndData.TextureNames.size(); iTex < texCount; ++iTex) {
		int32_t indexTop	= iTex + gndData.TextureNames.size() * ::llc::TILE_FACE_FACING_TOP		; llc_necall(::llc::gndGenerateFaceGeometry(gndData.lstTileTextureData, gndData.lstTileGeometryData, gndData.Metrics,::llc::TILE_FACE_FACING_TOP	, iTex, applicationInstance.GNDModel.Nodes[indexTop		], applicationInstance.GNDModel.TileMapping.View), "");
		int32_t indexFront	= iTex + gndData.TextureNames.size() * ::llc::TILE_FACE_FACING_FRONT	; llc_necall(::llc::gndGenerateFaceGeometry(gndData.lstTileTextureData, gndData.lstTileGeometryData, gndData.Metrics,::llc::TILE_FACE_FACING_FRONT	, iTex, applicationInstance.GNDModel.Nodes[indexFront	], applicationInstance.GNDModel.TileMapping.View), "");
		int32_t indexRight	= iTex + gndData.TextureNames.size() * ::llc::TILE_FACE_FACING_RIGHT	; llc_necall(::llc::gndGenerateFaceGeometry(gndData.lstTileTextureData, gndData.lstTileGeometryData, gndData.Metrics,::llc::TILE_FACE_FACING_RIGHT	, iTex, applicationInstance.GNDModel.Nodes[indexRight	], applicationInstance.GNDModel.TileMapping.View), "");
		//int32_t indexBottom	= iTex + gndData.TextureNames.size() * ::llc::TILE_FACE_FACING_BOTTOM	; llc_necall(::llc::gndGenerateFaceGeometry(gndData.lstTileTextureData, gndData.lstTileGeometryData, gndData.Metrics,::llc::TILE_FACE_FACING_BOTTOM	, iTex, applicationInstance.GNDModel.Nodes[indexBottom	], applicationInstance.GNDModel.TileMapping.View), "");
		int32_t indexBack	= iTex + gndData.TextureNames.size() * ::llc::TILE_FACE_FACING_BACK		; llc_necall(::llc::gndGenerateFaceGeometry(gndData.lstTileTextureData, gndData.lstTileGeometryData, gndData.Metrics,::llc::TILE_FACE_FACING_BACK	, iTex, applicationInstance.GNDModel.Nodes[indexBack	], applicationInstance.GNDModel.TileMapping.View), "");
		int32_t indexLeft	= iTex + gndData.TextureNames.size() * ::llc::TILE_FACE_FACING_LEFT		; llc_necall(::llc::gndGenerateFaceGeometry(gndData.lstTileTextureData, gndData.lstTileGeometryData, gndData.Metrics,::llc::TILE_FACE_FACING_LEFT	, iTex, applicationInstance.GNDModel.Nodes[indexLeft	], applicationInstance.GNDModel.TileMapping.View), "");
	}
	blendGNDNormals(tileGeometryView, gndData.lstTileTextureData, applicationInstance.GNDModel.TileMapping.View, applicationInstance.GNDModel.Nodes); // Blend normals.

	ree_if(errored(::updateSizeDependentResources(applicationInstance)), "Cannot update offscreen and textures and this could cause an invalid memory access later on.");
	applicationInstance.Scene.Camera.Points.Position						= {0, 30, -20};
	applicationInstance.Scene.Camera.Range.Far								= 1000;
	applicationInstance.Scene.Camera.Range.Near								= 0.001;
	return 0;
}

					::llc::error_t										update										(::SApplication& applicationInstance, bool systemRequestedExit)					{ 
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFrameInfo															& frameInfo									= framework.FrameInfo;
	retval_info_if(1, systemRequestedExit, "Exiting because the runtime asked for close. We could also ignore this value and just continue execution if we don't want to exit.");
	::llc::error_t																frameworkResult								= ::llc::updateFramework(applicationInstance.Framework);
	ree_if(errored(frameworkResult), "Unknown error.");
	rvi_if(1, frameworkResult == 1, "Framework requested close. Terminating execution.");
	ree_if(errored(::updateSizeDependentResources(applicationInstance)), "Cannot update offscreen and this could cause an invalid memory access later on.");

	//----------------------------------------------
	bool																		updateProjection							= false;
	if(framework.Input.KeyboardCurrent.KeyState[VK_ADD		])	{ updateProjection = true; applicationInstance.Scene.Camera.Range.Angle += frameInfo.Seconds.LastFrame * .05f; }
	if(framework.Input.KeyboardCurrent.KeyState[VK_SUBTRACT	])	{ updateProjection = true; applicationInstance.Scene.Camera.Range.Angle -= frameInfo.Seconds.LastFrame * .05f; }
	if(updateProjection) {
		::llc::STexture<::llc::SColorBGRA>											& offscreen									= framework.Offscreen;
		const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();

		::llc::SMatrix4<float>														& finalProjection							= applicationInstance.Scene.Transforms.FinalProjection	;
		::llc::SMatrix4<float>														& fieldOfView								= applicationInstance.Scene.Transforms.FieldOfView		;
		::llc::SMatrix4<float>														& viewport									= applicationInstance.Scene.Transforms.Viewport			;
		::llc::SMatrix4<float>														& viewportInverse							= applicationInstance.Scene.Transforms.ViewportInverse	;
		::llc::SMatrix4<float>														& viewportInverseCentered					= applicationInstance.Scene.Transforms.ViewportInverse	;
		fieldOfView.FieldOfView(applicationInstance.Scene.Camera.Range.Angle * ::llc::math_pi, offscreenMetrics.x / (double)offscreenMetrics.y, applicationInstance.Scene.Camera.Range.Near, applicationInstance.Scene.Camera.Range.Far);
		viewport.Viewport(offscreenMetrics, applicationInstance.Scene.Camera.Range.Far, applicationInstance.Scene.Camera.Range.Near);
		viewportInverse															= viewport.GetInverse();
		const ::llc::SCoord2<int32_t>												screenCenter								= {(int32_t)offscreenMetrics.x / 2, (int32_t)offscreenMetrics.y / 2};
		viewportInverseCentered													= viewportInverse;
		viewportInverseCentered._41												+= screenCenter.x;
		viewportInverseCentered._42												+= screenCenter.y;
		finalProjection															= fieldOfView * viewportInverseCentered;
		applicationInstance.Scene.Transforms.FinalProjectionInverse				= finalProjection.GetInverse();
	}

	//------------------------------------------------ Camera
	::llc::SCameraPoints														& camera									= applicationInstance.Scene.Camera.Points;
	camera.Position.RotateY(framework.Input.MouseCurrent.Deltas.x / 20.0f / (applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_CONTROL] ? 2.0 : 1));
	if(framework.Input.MouseCurrent.Deltas.z) {
		::llc::SCoord3<float>														zoomVector									= camera.Position;
		zoomVector.Normalize();
		const double																zoomWeight									= framework.Input.MouseCurrent.Deltas.z * (applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_SHIFT] ? 10 : 1) / 240.;
		camera.Position															+= zoomVector * zoomWeight * .5;
	}
	::llc::SMatrix4<float>														& viewMatrix								= applicationInstance.Scene.Transforms.View;
	::llc::SCameraVectors														& cameraVectors								= applicationInstance.Scene.Camera.Vectors;
	cameraVectors.Up														= {0, 1, 0};
	cameraVectors.Front														= (camera.Target - camera.Position).Normalize();
	cameraVectors.Right														= cameraVectors.Up		.Cross(cameraVectors.Front).Normalize();
	cameraVectors.Up														= cameraVectors.Front	.Cross(cameraVectors.Right).Normalize();
	viewMatrix.View3D(camera.Position, cameraVectors.Right, cameraVectors.Up, cameraVectors.Front);
	//const ::llc::SCoord2<uint32_t>												& gndDataMetrics							= applicationInstance.GNDData.Metrics.Size;
	//viewMatrix.LookAt(camera.Position, {(gndDataMetrics.x / 2.0f), 0, -(gndDataMetrics.y / 2.0f)}, {0, 1, 0});

	//------------------------------------------------ Lights
	::llc::SCoord3<float>														& lightDir									= applicationInstance.LightDirection;
	lightDir.RotateY(frameInfo.Microseconds.LastFrame / 1000000.0f);
	lightDir.Normalize();

	::llc::SRSWFileContents														& rswData									= applicationInstance.RSWData;
	const ::llc::SCoord3<float>													halfMapDir									= ::llc::SCoord3<float>{applicationInstance.GNDData.Metrics.Size.x / 2.0f, 0.0f, (applicationInstance.GNDData.Metrics.Size.y / 2.0f)};
	for(uint32_t iLight = 0; iLight < rswData.RSWLights.size(); ++iLight) {
		rswData.RSWLights[iLight].Position										-= halfMapDir;
		rswData.RSWLights[iLight].Position.RotateY(frameInfo.Seconds.LastFrame);
		rswData.RSWLights[iLight].Position										+= halfMapDir;
	}
	//------------------------------------------------ 
	//applicationInstance.GridPivot.Scale										= {2.f, 4.f, 2.f};
	applicationInstance.GridPivot.Scale										= {1, -1, -1};
	//applicationInstance.GridPivot.Orientation.y								= (float)(sinf((float)(frameInfo.Seconds.Total / 2)) * ::llc::math_2pi);
	//applicationInstance.GridPivot.Orientation.w								= 1;
	//applicationInstance.GridPivot.Orientation.Normalize();
	//applicationInstance.GridPivot.Position									= {applicationInstance.GNDData.Metrics.Size.x / 2.0f * -1, 0, applicationInstance.GNDData.Metrics.Size.y / 2.0f * -1};
	return 0;
}

					::llc::error_t										drawGND										(::SApplication& applicationInstance);

					::llc::error_t										draw										(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	::llc::STexture<uint32_t>													& offscreenDepth							= framework.OffscreenDepthStencil;
	::memset(offscreenDepth	.Texels.begin(), -1, sizeof(uint32_t)								* offscreenDepth	.Texels.size());	// Clear target.
	::memset(offscreen		.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel)	* offscreen			.Texels.size());	// Clear target.
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	for(uint32_t y = 0, yMax = offscreenMetrics.y; y < yMax; ++y) {	// Draw background gradient.
		const uint8_t																colorHeight									= (uint8_t)(y / 10);
		for(uint32_t x = 0, xMax = offscreenMetrics.x; x < xMax; ++x)
			offscreen.View[y][x]													= {colorHeight, 0, 0, 0xFF};
	}
	int32_t 																	pixelsDrawn1								= drawGND	(applicationInstance); error_if(errored(pixelsDrawn1), "??");
	static constexpr const ::llc::SCoord2<int32_t>								sizeCharCell								= {9, 16};
	uint32_t																	lineOffset									= 0;
	static constexpr const char													textLine2	[]								= "Press ESC to exit.";
	::llc::grid_view<::llc::SColorBGRA>											& offscreenView								= offscreen.View;
	const ::llc::grid_view<::llc::SColorBGRA>									& fontAtlasView								= applicationInstance.TextureFont.View;
	const ::llc::SColorBGRA														textColor									= {0, framework.FrameInfo.FrameNumber % 0xFFU, 0, 0xFFU};
	::llc::textLineDrawAlignedFixedSizeLit(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), lineOffset = offscreenMetrics.y / sizeCharCell.y - 1, offscreenMetrics, sizeCharCell, textLine2, textColor);
	::llc::STimer																& timer										= applicationInstance.Framework.Timer;
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	char																		buffer		[256]							= {};

	const ::llc::grid_view<::llc::SColorBGRA>									& minimapView									= applicationInstance.TextureMinimap.View;
	::llc::grid_copy(offscreenView, minimapView, ::llc::SCoord2<int32_t>{(int32_t)(offscreenMetrics.x - minimapView.metrics().x), (int32_t)(offscreenMetrics.y - minimapView.metrics().y)});

	const ::llc::SGNDFileContents												& gndData									= applicationInstance.GNDData;
	const ::llc::SRSWFileContents												& rswData									= applicationInstance.RSWData;
	const ::SRenderCache														& renderCache								= applicationInstance.RenderCache;
	uint32_t

	textLen																	= (uint32_t)sprintf_s(buffer, "[%u x %u]. FPS: %g. Last frame seconds: %g."					, mainWindow.Size.x, mainWindow.Size.y, 1 / timer.LastTimeSeconds, timer.LastTimeSeconds);							::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangles drawn: %u. Pixels drawn: %u. Pixels skipped: %u."	, (uint32_t)renderCache.TrianglesDrawn, (uint32_t)renderCache.PixelsDrawn, (uint32_t)renderCache.PixelsSkipped);	::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Tile grid size: {x=%u, z=%u}. Dynamic light count: %u."		, gndData.Metrics.Size.x, gndData.Metrics.Size.y, rswData.RSWLights.size());										::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//::textDrawAlignedFixedSize(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen}, textColor);
	return 0;																																																
}
		