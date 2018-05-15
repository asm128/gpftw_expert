// Tip: Hold Left ALT + SHIFT while tapping or holding the arrow keys in order to select multiple columns and write on them at once. 
//		Also useful for copy & paste operations in which you need to copy a bunch of variable or function names and you can't afford the time of copying them one by one.
#include "application.h"

#include "llc_bitmap_target.h"
#include "llc_bitmap_file.h"
#include "llc_gui_text.h"

#include "llc_app_impl.h"

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

					void												myThread									(void* _applicationThreads)														{
	::SThreadArgs																& threadArgs								= *(::SThreadArgs*)_applicationThreads;
	::SApplicationThreads														& applicationThreads						= *threadArgs.ApplicationThreads;
	int32_t																		threadId									= threadArgs.ThreadId;
	while(false == applicationThreads.States[threadId].RequestedClose) {
		Sleep(10);
	}
	applicationThreads.States[threadId].Closed								= true;
}

					::llc::error_t										setupThreads								(::SApplication& applicationInstance)													{
	for(uint32_t iThread = 0, threadCount = ::llc::size(applicationInstance.Threads.Handles); iThread < threadCount; ++iThread) {
		applicationInstance.Threads.States	[iThread]							= {true,};									
		applicationInstance.Threads.Handles	[iThread]							= _beginthread(myThread, 0, &(applicationInstance.ThreadArgs[iThread] = {&applicationInstance.Threads, (int32_t)iThread}));
	}
	return 0;
}

					::llc::error_t										bmpOrBmgLoad								(::llc::view_string bmpFileName, ::llc::STexture<::llc::SColorBGRA>& loaded)		{
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

					::llc::error_t										generateModelFromHeights					(const ::llc::grid_view<::STileHeights<float>> & tileHeights, ::llc::SModelGeometry<float> & generated)																											{
	for(uint32_t y = 0, yMax = tileHeights.metrics().y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = tileHeights.metrics().x; x < xMax; ++x) {
		float																		fx											= (float)x;
		float																		fy											= (float)y;
		const ::STileHeights<float>													& curTile									= tileHeights[y][x];
		const ::llc::STriangle3D<float>												tri1										= {{fx, curTile.Heights[0], fy}, {fx, curTile.Heights[2], fy + 1}, {fx + 1, curTile.Heights[3], fy + 1}};
		const ::llc::STriangle3D<float>												tri2										= {{fx, curTile.Heights[0], fy}, {fx + 1, curTile.Heights[3], fy + 1}, {fx + 1, curTile.Heights[1], fy}};
		generated.Positions.push_back(tri1);
		generated.Positions.push_back(tri2);
		const ::llc::SCoord3<float>													normal1										= (tri1.B - tri1.A).Cross(tri1.C - tri1.A);
		const ::llc::SCoord3<float>													normal2										= (tri1.B - tri1.A).Cross(tri1.C - tri1.A);
		generated.NormalsVertex		.push_back({normal1, normal1, normal1});
		generated.NormalsVertex		.push_back({normal2, normal2, normal2});
		generated.NormalsTriangle	.push_back(normal1);
		generated.NormalsTriangle	.push_back(normal2);
	}

	const ::llc::SCoord2<double>											gridUnit										= {1.0 / tileHeights.metrics().x, 1.0 / tileHeights.metrics().y};
	const ::llc::SCoord2<double>											gridMetricsF									= tileHeights.metrics().Cast<double>();
	for(uint32_t z = 0; z < tileHeights.metrics().y; ++z) 
	for(uint32_t x = 0; x < tileHeights.metrics().x; ++x) { 
		const ::llc::SCoord2<double>											gridCell										= {x / gridMetricsF.x, z / gridMetricsF.y};
		const ::llc::SCoord2<double>											gridCellFar										= gridCell + gridUnit;
		generated.UVs.push_back({{(float)gridCell.x, (float)gridCell.y}, {(float)gridCell.x			, (float)gridCellFar.y}	, gridCellFar.Cast<float>()					}); 
		generated.UVs.push_back({{(float)gridCell.x, (float)gridCell.y}, gridCellFar.Cast<float>()	, {(float)gridCellFar.x, (float)gridCell.y}	}); 
	}

	generated;
	return 0;
}


					::llc::error_t										mainWindowCreate							(::llc::SDisplay& mainWindow, HINSTANCE hInstance);
					::llc::error_t										setup										(::SApplication& applicationInstance)													{
	g_ApplicationInstance													= &applicationInstance;
	error_if(errored(setupThreads(applicationInstance)), "Unknown.");
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	error_if(errored(::mainWindowCreate(mainWindow, applicationInstance.Framework.RuntimeValues.PlatformDetail.EntryPointArgs.hInstance)), "Failed to create main window why?????!?!?!?!?");
	char																		bmpFileName1	[]							= "test.bmp";//"pow_core_0.bmp";
	char																		bmpFileName2	[]							= "Codepage-437-24.bmp";
	error_if(errored(::bmpOrBmgLoad(bmpFileName1, applicationInstance.TextureGrid)), "");
	error_if(errored(::bmpOrBmgLoad(bmpFileName2, applicationInstance.TextureFont)), "");
	const ::llc::SCoord2<uint32_t>												& textureFontMetrics						= applicationInstance.TextureFont.View.metrics();
	applicationInstance.TextureFontMonochrome.resize(textureFontMetrics);
	for(uint32_t y = 0, yMax = textureFontMetrics.y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = textureFontMetrics.x; x < xMax; ++x) {
		const ::llc::SColorBGRA														& srcColor									= applicationInstance.TextureFont.View[y][x];
		applicationInstance.TextureFontMonochrome.View[y * textureFontMetrics.x + x]	
			=	0 != srcColor.r
			||	0 != srcColor.g
			||	0 != srcColor.b
			;
	}


	//llc_necall(::gndFileLoad(applicationInstance.GNDData, ".\\data\\prt_fild00.gnd"), "Error");
	//SModelNodeGND																modelNode;
	//applicationInstance.TexturesGND.resize(applicationInstance.GNDData.TextureNames.size());
	//for(uint32_t iTex = 0; iTex < applicationInstance.GNDData.TextureNames.size(); ++ iTex) {
	//	static constexpr	const char												respath			[]							= "..\\data\\texture";
	//	char																		filepathinal	[512]						= {};
	//	sprintf_s(filepathinal, "%s\\%s", respath, applicationInstance.GNDData.TextureNames[iTex].c_str());
	//	error_if(errored(::llc::bmpFileLoad((::llc::view_const_string)filepathinal, applicationInstance.TexturesGND[iTex])), "Not found? %s.", filepathinal);
	//}

	ree_if(errored(::updateSizeDependentResources(applicationInstance)), "Cannot update offscreen and textures and this could cause an invalid memory access later on.");

	// Load and pretransform our cube geometry.
	const ::llc::grid_view<::llc::SColorBGRA>									& textureGridView								= applicationInstance.TextureGrid.View;
	const ::llc::SCoord2<uint32_t>												& textureGridMetrics							= textureGridView.metrics();
	applicationInstance.TileHeights.resize(textureGridView.metrics());
	for(uint32_t y = 0, yMax = textureGridMetrics.y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = textureGridMetrics.x; x < xMax; ++x) {
		::STileHeights<float>														& curTile									= applicationInstance.TileHeights[y][x];
		float																		curHeight									= textureGridView[y][x].g / 255.0f;
		curTile.Heights[0]														= curHeight;
		curTile.Heights[1]														= curHeight;
		curTile.Heights[2]														= curHeight;
		curTile.Heights[3]														= curHeight;
	}
	for(uint32_t y = 0, yMax = (textureGridMetrics.y - 1); y < yMax; ++y)
	for(uint32_t x = 0, xMax = (textureGridMetrics.x - 1); x < xMax; ++x) {
		::STileHeights<float>														& curTile									= applicationInstance.TileHeights[y		][x		];
		::STileHeights<float>														& frontTile									= applicationInstance.TileHeights[y		][x + 1	];
		::STileHeights<float>														& leftTile									= applicationInstance.TileHeights[y + 1	][x		];
		::STileHeights<float>														& flTile									= applicationInstance.TileHeights[y + 1	][x + 1	];
		float																		averageHeight								= 
			( curTile	.Heights[3]
			+ frontTile	.Heights[2]
			+ leftTile	.Heights[1]
			+ flTile	.Heights[0]
			) / 4;
		curTile									.Heights[3]						= averageHeight;
		frontTile								.Heights[2]						= averageHeight;	
		leftTile								.Heights[1]						= averageHeight;	
		flTile									.Heights[0]						= averageHeight;
	}
	
	::llc::SModelGeometry<float>												& geometryGrid								= applicationInstance.Grid;
	::generateModelFromHeights(applicationInstance.TileHeights.View, geometryGrid);
	::llc::generateGridGeometry(textureGridMetrics, geometryGrid);
	
	const ::llc::SCoord3<float>													gridCenter									= {textureGridMetrics.x / 2.0f, 0, textureGridMetrics.y / 2.0f};
	
	for(uint32_t iTriangle = 0; iTriangle < geometryGrid.Positions.size(); ++iTriangle) {
		::llc::STriangle3D<float>													& transformedTriangle						= geometryGrid.Positions	[iTriangle];
		transformedTriangle.A													-= gridCenter;
		transformedTriangle.B													-= gridCenter;
		transformedTriangle.C													-= gridCenter;
	}

	applicationInstance.GridPivot.Orientation								= {0,0,0,1}; 

	::llc::array_view<::llc::SCoord3<float>>										positions									= {(::llc::SCoord3<float>*)geometryGrid.Positions		.begin(), geometryGrid.Positions		.size() * 3};
	::llc::array_view<::llc::SCoord3<float>>										normals										= {(::llc::SCoord3<float>*)geometryGrid.NormalsVertex	.begin(), geometryGrid.NormalsVertex	.size() * 3};
	//
	for(uint32_t iBlend = 0; iBlend < 10; ++iBlend) 
	for(uint32_t iPosition = 0; iPosition < positions.size(); ++iPosition) {
		const ::llc::SCoord3<float>														& curPos									= positions	[iPosition];
		::llc::SCoord3<float>															& curNormal									= normals	[iPosition];
		for(uint32_t iPosOther = iPosition + 1; iPosOther < positions.size(); ++iPosOther) {
			const ::llc::SCoord3<float>														& otherPos									= positions	[iPosOther];
			if((curPos - otherPos).LengthSquared() < (0.001 * .001)) {
				::llc::SCoord3<float>															& otherNormal								= normals	[iPosOther];
				otherNormal = curNormal														= ((otherNormal + curNormal) / 2).Normalize();
				break;
			}
		}
	}

	applicationInstance.Scene.Camera.Points.Position						= {20, 30, 0};
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
	//viewMatrix.LookAt(camera.Position, {0, 0, 0}, {0, 1, 0});

	//------------------------------------------------ Lights
	::llc::SCoord3<float>														& lightDir									= applicationInstance.LightDirection;
	lightDir.RotateY(frameInfo.Microseconds.LastFrame / 1000000.0f);
	lightDir.Normalize();

	//------------------------------------------------ 
	applicationInstance.GridPivot.Scale										= {2.f, 4.f, 2.f};
	//applicationInstance.GridPivot.Scale										= {1, -1, 1};
	//applicationInstance.GridPivot.Orientation.y								= (float)(sinf((float)(frameInfo.Seconds.Total / 2)) * ::llc::math_2pi);
	//applicationInstance.GridPivot.Orientation.w								= 1;
	//applicationInstance.GridPivot.Orientation.Normalize();
	//applicationInstance.GridPivot.Position									= {applicationInstance.GNDData.Metrics.Size.x / 2.0f * -1, 0, applicationInstance.GNDData.Metrics.Size.y / 2.0f * -1};
	applicationInstance.GridPivot.Position									= {};
	return 0;
}

					::llc::error_t										drawGrides									(::SApplication& applicationInstance);
					::llc::error_t										drawGND										(::SApplication& applicationInstance);

					::llc::error_t										draw										(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	int32_t 																	pixelsDrawn0								= drawGrides(applicationInstance); error_if(errored(pixelsDrawn0), "??");
	static constexpr const ::llc::SCoord2<int32_t>								sizeCharCell								= {9, 16};
	uint32_t																	lineOffset									= 0;
	static constexpr const char													textLine2	[]								= "Press ESC to exit.";
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	::llc::grid_view<::llc::SColorBGRA>											& offscreenView								= offscreen.View;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	const ::llc::grid_view<::llc::SColorBGRA>									& fontAtlasView								= applicationInstance.TextureFont.View;
	const ::llc::SColorBGRA														textColor									= {0, framework.FrameInfo.FrameNumber % 0xFFU, 0, 0xFFU};
	::llc::textLineDrawAlignedFixedSizeLit(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), lineOffset = offscreenMetrics.y / sizeCharCell.y - 1, offscreenMetrics, sizeCharCell, textLine2, textColor);
	::llc::STimer																& timer										= applicationInstance.Framework.Timer;
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	char																		buffer		[256]							= {};
	uint32_t																	
	textLen																	= (uint32_t)sprintf_s(buffer, "[%u x %u]. FPS: %g. Last frame seconds: %g.", mainWindow.Size.x, mainWindow.Size.y, 1 / timer.LastTimeSeconds, timer.LastTimeSeconds);	::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangles drawn: %u. Pixels drawn: %u. Pixels skipped: %u.", (uint32_t)applicationInstance.RenderCache.TrianglesDrawn, (uint32_t)applicationInstance.RenderCache.PixelsDrawn, (uint32_t)applicationInstance.RenderCache.PixelsSkipped);	::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//textLen																	= (uint32_t)sprintf_s(buffer, "Triangle3dColorList  cache size: %u.", applicationInstance.RenderCache.Triangle3dColorList	.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//textLen																	= (uint32_t)sprintf_s(buffer, "TransformedNormals   cache size: %u.", applicationInstance.RenderCache.TransformedNormals	.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//textLen																	= (uint32_t)sprintf_s(buffer, "Triangle3dToDraw     cache size: %u.", applicationInstance.RenderCache.Triangle3dToDraw		.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//textLen																	= (uint32_t)sprintf_s(buffer, "Triangle2dToDraw     cache size: %u.", applicationInstance.RenderCache.Triangle2dToDraw		.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//textLen																	= (uint32_t)sprintf_s(buffer, "Triangle3dIndices    cache size: %u.", applicationInstance.RenderCache.Triangle3dIndices		.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//textLen																	= (uint32_t)sprintf_s(buffer, "Triangle2dIndices    cache size: %u.", applicationInstance.RenderCache.Triangle2dIndices		.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//textLen																	= (uint32_t)sprintf_s(buffer, "Triangle2d23dIndices cache size: %u.", applicationInstance.RenderCache.Triangle2d23dIndices	.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//::textDrawAlignedFixedSize(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen}, textColor);
	return 0;																																																
}
	