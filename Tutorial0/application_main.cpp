// Tip: Hold Left ALT + SHIFT while tapping or holding the arrow keys in order to select multiple columns and write on them at once. 
//		Also useful for copy & paste operations in which you need to copy a bunch of variable or function names and you can't afford the time of copying them one by one.
#include "application.h"

#include "llc_bitmap_target.h"
#include "llc_bitmap_file.h"
#include "llc_gui_text.h"

#include "llc_app_impl.h"

static constexpr	const uint32_t										ASCII_SCREEN_WIDTH							= 132	;
static constexpr	const uint32_t										ASCII_SCREEN_HEIGHT							= 50	;

LLC_DEFINE_APPLICATION_ENTRY_POINT(::SApplication);	

					::SApplication										* g_ApplicationInstance						= 0;

static				::llc::error_t										updateSizeDependentResources				(::SApplication& applicationInstance)											{ 
	const ::llc::SCoord2<uint32_t>												& newSize									= applicationInstance.Framework.MainDisplay.Size; 
	::llc::STexture<::llc::SColorBGRA>											& offscreen									= applicationInstance.Framework.Offscreen;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	if(newSize != offscreenMetrics) {
		::llc::SMatrix4<float>														& projection								= applicationInstance.XProjection		;
		::llc::SMatrix4<float>														& viewport									= applicationInstance.XViewport			;
		::llc::SMatrix4<float>														& viewportInverse							= applicationInstance.XViewportInverse	;
		viewport.Viewport(newSize, applicationInstance.Camera.Range.Far, applicationInstance.Camera.Range.Near);
		viewportInverse															= viewport.GetInverse();
		const ::llc::SCoord2<int32_t>												screenCenter								= {(int32_t)newSize.x / 2, (int32_t)newSize.y / 2};
		viewportInverse._41														+= screenCenter.x;
		viewportInverse._42														+= screenCenter.y;
		projection.FieldOfView(applicationInstance.Camera.Range.Angle * ::llc::math_pi, newSize.x / (double)newSize.y, applicationInstance.Camera.Range.Near, applicationInstance.Camera.Range.Far);
		projection																*= viewportInverse;
	}
	llc_necall(::llc::updateSizeDependentTarget(applicationInstance.Framework.Offscreen, newSize), "??");
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
	g_ApplicationInstance													= 0;
	return 0;
}

					::llc::error_t										mainWindowCreate							(::llc::SDisplay& mainWindow, HINSTANCE hInstance);
					::llc::error_t										setup										(::SApplication& applicationInstance)											{ 
	g_ApplicationInstance													= &applicationInstance;
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	error_if(errored(::mainWindowCreate(mainWindow, applicationInstance.Framework.RuntimeValues.PlatformDetail.EntryPointArgs.hInstance)), "Failed to create main window why?????!?!?!?!?");
	static constexpr	const char												bmpFileName2	[]							= "..\\gpk_data\\images\\Codepage-437-24.bmp";
	error_if(errored(::llc::bmpFileLoad((::llc::view_const_string)bmpFileName2, applicationInstance.TextureFont				)), "Failed to load bitmap from file: %s.", bmpFileName2);
	const ::llc::SCoord2<uint32_t>												& textureFontMetrics						= applicationInstance.TextureFont.View.metrics();
	applicationInstance.TextureFontMonochrome.resize(textureFontMetrics);
	for(uint32_t y = 0, yMax = textureFontMetrics.y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = textureFontMetrics.x; x < xMax; ++x)
		applicationInstance.TextureFontMonochrome.View[y * textureFontMetrics.x + x]	
		=	0 != applicationInstance.TextureFont.View[y][x].r
		||	0 != applicationInstance.TextureFont.View[y][x].g
		||	0 != applicationInstance.TextureFont.View[y][x].b
		;

	// Load and pretransform our cube geometry.
	static constexpr	const ::llc::SCoord3<float>								cubeCenter									= {0.5f, 0.5f, 0.5f};
	::llc::generateCubeGeometry(applicationInstance.Box);
	for(uint32_t iTriangle = 0; iTriangle < 12; ++iTriangle) {
		::llc::STriangle3D<float>													& transformedTriangle						= applicationInstance.Box.Positions[iTriangle];
		transformedTriangle.A													-= cubeCenter;
		transformedTriangle.B													-= cubeCenter;
		transformedTriangle.C													-= cubeCenter;
	}
	ree_if(errored(::updateSizeDependentResources(applicationInstance)), "Cannot update offscreen and textures and this could cause an invalid memory access later on.");
	applicationInstance.BoxPivot.Orientation								= {0,0,0,1}; 
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
	bool																		updateProjection							= false;
	if(applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_ADD		])	{ updateProjection = true; applicationInstance.Camera.Range.Angle += frameInfo.Seconds.LastFrame * .05f; }
	if(applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_SUBTRACT	])	{ updateProjection = true; applicationInstance.Camera.Range.Angle -= frameInfo.Seconds.LastFrame * .05f; }
	if(updateProjection) {
		::llc::SMatrix4<float>														& projection								= applicationInstance.XProjection		;
		const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= framework.Offscreen.View.metrics();
		projection.FieldOfView(applicationInstance.Camera.Range.Angle * ::llc::math_pi, offscreenMetrics.x / (double)offscreenMetrics.y, applicationInstance.Camera.Range.Near, applicationInstance.Camera.Range.Far);
		::llc::SMatrix4<float>														& viewportInverse							= applicationInstance.XViewportInverse	;
		projection																*= viewportInverse;
	}

	//------------------------------------------------ Camera
	::llc::SCameraPoints														& camera									= applicationInstance.Camera.Points;
	camera.Position.RotateY(framework.Input.MouseCurrent.Deltas.x / 20.0f);
	if(framework.Input.MouseCurrent.Deltas.z) {
		::llc::SCoord3<float>														zoomVector									= camera.Position;
		zoomVector.Normalize();
		const double																zoomWeight									= framework.Input.MouseCurrent.Deltas.z / 240.;
		camera.Position															+= zoomVector * zoomWeight;
	}
	::llc::SMatrix4<float>														& viewMatrix								= applicationInstance.XView			;
	::llc::SCameraVectors														& cameraVectors								= applicationInstance.Camera.Vectors;
	cameraVectors.Up														= {0, 1, 0};
	cameraVectors.Front														= (camera.Target - camera.Position).Normalize();
	cameraVectors.Right														= cameraVectors.Up		.Cross(cameraVectors.Front).Normalize();
	cameraVectors.Up														= cameraVectors.Front	.Cross(cameraVectors.Right).Normalize();
	viewMatrix.View3D(camera.Position, cameraVectors.Right, cameraVectors.Up, cameraVectors.Front);
	//viewMatrix.LookAt(camera.Position, camera.Target, cameraUp);

	//------------------------------------------------ Lights
	::llc::SCoord3<float>														& lightDir									= applicationInstance.LightDirection;
	lightDir.RotateY(frameInfo.Microseconds.LastFrame / 250000.0f);
	lightDir.Normalize();

	//------------------------------------------------ 
	applicationInstance.BoxPivot.Scale										= {.5f, 1.0f, 2.5f};
	applicationInstance.BoxPivot.Orientation.y								+= (float)(sinf((float)frameInfo.Seconds.Total / 2) * ::llc::math_pi);
	applicationInstance.BoxPivot.Orientation.w								= 1;
	applicationInstance.BoxPivot.Orientation.Normalize();
	applicationInstance.BoxPivot.Position									= {0, 0, 0};
	return 0;
}

					::llc::error_t										drawBoxes									(::SApplication& applicationInstance);

					::llc::error_t										draw										(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	int32_t 																	pixelsDrawn									= drawBoxes(applicationInstance);
	error_if(errored(pixelsDrawn), "??");
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
	textLen																	= (uint32_t)sprintf_s(buffer, "Pixels drawn: %u. Pixels skipped: %u.", (uint32_t)applicationInstance.RenderCache.PixelsDrawn, (uint32_t)applicationInstance.RenderCache.PixelsSkipped);	::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangle3dColorList  cache size: %u.", applicationInstance.RenderCache.Triangle3dColorList	.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "TransformedNormals   cache size: %u.", applicationInstance.RenderCache.TransformedNormals	.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangle3dToDraw     cache size: %u.", applicationInstance.RenderCache.Triangle3dToDraw		.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangle2dToDraw     cache size: %u.", applicationInstance.RenderCache.Triangle2dToDraw		.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangle3dIndices    cache size: %u.", applicationInstance.RenderCache.Triangle3dIndices		.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangle2dIndices    cache size: %u.", applicationInstance.RenderCache.Triangle2dIndices		.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	textLen																	= (uint32_t)sprintf_s(buffer, "Triangle2d23dIndices cache size: %u.", applicationInstance.RenderCache.Triangle2d23dIndices	.size()); ::llc::textLineDrawAlignedFixedSizeRGBA(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	//::textDrawAlignedFixedSize(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen}, textColor);
	return 0;																																																
}
	