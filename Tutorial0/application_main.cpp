// Tip: Hold Left ALT + SHIFT while tapping or holding the arrow keys in order to select multiple columns and write on them at once. 
//		Also useful for copy & paste operations in which you need to copy a bunch of variable or function names and you can't afford the time of copying them one by one.
#include "application.h"

#include "llc_bitmap_target.h"
#include "llc_bitmap_file.h"
#include "llc_grid_copy.h"
#include "llc_grid_scale.h"
#include "llc_bit_array_view.h"
#include "llc_geometry.h"

#include "llc_app_impl.h"
#include "llc_matrix.h"

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
		viewport.Viewport(newSize, applicationInstance.CameraFar, applicationInstance.CameraNear);
		viewportInverse															= viewport.GetInverse();
		const ::llc::SCoord2<int32_t>												screenCenter								= {(int32_t)newSize.x / 2, (int32_t)newSize.y / 2};
		viewportInverse._41														+= screenCenter.x;
		viewportInverse._42														+= screenCenter.y;
		projection.FieldOfView(applicationInstance.CameraAngle * ::llc::math_pi, newSize.x / (double)newSize.y, applicationInstance.CameraNear, applicationInstance.CameraFar);
		projection																*= viewportInverse;
	}
	::llc::updateSizeDependentTarget(applicationInstance.Framework.Offscreen, newSize);
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
	static constexpr	const char												bmpFileName2	[]							= "Codepage-437-24.bmp";
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
	::llc::generateCubeGeometry(applicationInstance.Box.Positions, applicationInstance.Box.Normals, applicationInstance.Box.UVs);
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
	if(applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_ADD		])	applicationInstance.CameraAngle += frameInfo.Seconds.LastFrame * .05f;
	if(applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_SUBTRACT	])	applicationInstance.CameraAngle -= frameInfo.Seconds.LastFrame * .05f;

	applicationInstance.BoxPivot.Orientation.y								+= (float)(sinf((float)frameInfo.Seconds.Total / 2) * ::llc::math_pi);
	applicationInstance.BoxPivot.Orientation.w								= 1;
	applicationInstance.BoxPivot.Orientation.Normalize();
	return 0;
}

static				::llc::error_t										textCalcSizeLine							(const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0)	{ return (::llc::error_t)(sizeCharCell.x * (text0.size() - 1)); }
static				::llc::error_t										textDrawFixedSize							(::llc::grid_view<::llc::SColorBGRA>& bmpTarget, const ::llc::grid_view<::llc::SColorBGRA>& viewTextureFont, uint32_t characterCellsX, int32_t dstOffsetY, const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0, const ::llc::SCoord2<int32_t> dstTextOffset)	{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	for(int32_t iChar = 0, charCount = (int32_t)text0.size(); iChar < charCount; ++iChar) {
		int32_t																	coordTableX										= text0[iChar] % characterCellsX;
		int32_t																	coordTableY										= text0[iChar] / characterCellsX;
		const ::llc::SCoord2<int32_t>											coordCharTable									= {coordTableX * sizeCharCell.x, coordTableY * sizeCharCell.y};
		const ::llc::SCoord2<int32_t>											dstOffset1										= {sizeCharCell.x * iChar, dstOffsetY};
		const ::llc::SRectangle2D<int32_t>										srcRect0										= ::llc::SRectangle2D<int32_t>{{coordCharTable.x, (int32_t)viewTextureFont.height() - sizeCharCell.y - coordCharTable.y}, sizeCharCell};
		error_if(errored(::llc::grid_copy_alpha(bmpTarget, viewTextureFont, dstTextOffset + dstOffset1, srcRect0, {0xFF, 0x00, 0xFF, 0xFF})), "I believe this never fails.");
		//error_if(errored(::llc::grid_copy(bmpTarget, viewTextureFont, dstTextOffset + dstOffset1, srcRect0)), "I believe this never fails.");
	}
	return 0;
}

static				::llc::error_t										textDrawAlignedFixedSize					(::llc::grid_view<::llc::SColorBGRA>& targetView, const ::llc::grid_view<::llc::SColorBGRA>& fontAtlas, uint32_t lineOffset, const ::llc::SCoord2<uint32_t>& targetSize, const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0 )	{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	const ::llc::SCoord2<int32_t>												dstTextOffset								= {(int32_t)targetSize.x / 2 - (int32_t)textCalcSizeLine(sizeCharCell, text0) / 2, };
	uint32_t																	dstOffsetY									= (int32_t)(targetSize.y - lineOffset * sizeCharCell.y - sizeCharCell.y);
	return ::textDrawFixedSize(targetView, fontAtlas, 32, dstOffsetY, sizeCharCell, text0, dstTextOffset);
}

///---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static				::llc::error_t										textDrawFixedSize							(::llc::grid_view<::llc::SColorBGRA>& bmpTarget, const ::llc::bit_array_view<uint32_t>& viewTextureFont, const ::llc::SCoord2<uint32_t> & viewMetrics, uint32_t characterCellsX, int32_t dstOffsetY, const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0, const ::llc::SCoord2<int32_t> dstTextOffset, const ::llc::SColorBGRA& color)	{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::array_pod<::llc::SCoord2<uint32_t>>								dstCoords;
	for(int32_t iChar = 0, charCount = (int32_t)text0.size(); iChar < charCount; ++iChar) {
		const int32_t															coordTableX										= text0[iChar] % characterCellsX;
		const int32_t															coordTableY										= text0[iChar] / characterCellsX;
		const ::llc::SCoord2<int32_t>											coordCharTable									= {coordTableX * sizeCharCell.x, coordTableY * sizeCharCell.y};
		const ::llc::SCoord2<int32_t>											dstOffset1										= {sizeCharCell.x * iChar, dstOffsetY};
		const ::llc::SRectangle2D<int32_t>										srcRect0										= ::llc::SRectangle2D<int32_t>{{coordCharTable.x, (int32_t)viewMetrics.y - sizeCharCell.y - coordCharTable.y}, sizeCharCell};
		//error_if(errored(::llc::grid_copy_alpha_bit(bmpTarget, viewTextureFont, dstTextOffset + dstOffset1, viewMetrics, color, srcRect0)), "I believe this never fails.");
		dstCoords.clear();
		error_if(errored(::llc::grid_raster_alpha_bit(bmpTarget, viewTextureFont, dstTextOffset + dstOffset1, viewMetrics, srcRect0, dstCoords)), "I believe this never fails.");
		for(uint32_t iCoord = 0; iCoord < dstCoords.size(); ++iCoord)
			::llc::drawPixelLight(bmpTarget, dstCoords[iCoord], color, 0.05f, 0.75);
	}
	return 0;
}

static				::llc::error_t										textDrawAlignedFixedSize					(::llc::grid_view<::llc::SColorBGRA>& targetView, const ::llc::bit_array_view<uint32_t>& fontAtlas, const ::llc::SCoord2<uint32_t> & viewMetrics, uint32_t lineOffset, const ::llc::SCoord2<uint32_t>& targetSize, const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0, const ::llc::SColorBGRA& color)	{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	const ::llc::SCoord2<int32_t>												dstTextOffset								= {(int32_t)targetSize.x / 2 - (int32_t)textCalcSizeLine(sizeCharCell, text0) / 2, };
	uint32_t																	dstOffsetY									= (int32_t)(targetSize.y - lineOffset * sizeCharCell.y - sizeCharCell.y);
	return ::textDrawFixedSize(targetView, fontAtlas, viewMetrics, 32, dstOffsetY, sizeCharCell, text0, dstTextOffset, color);
}

struct SCamera {
						::llc::SCoord3<float>								Position, Target;
};


					::llc::error_t										draw										(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	::memset(offscreen.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel) * offscreen.Texels.size());	// Clear target.
	//------------------------------------------------
	::llc::SMatrix4<float>														& projection								= applicationInstance.XProjection	;
	::llc::SMatrix4<float>														& viewMatrix								= applicationInstance.XView			;
	::llc::SFrameInfo															& frameInfo									= framework.FrameInfo;
	::SCamera																	camera										= {{20, 5, 0}, {}};
	::llc::SCoord3<float>														lightPos									= {10, 5, 0};
	static float																cameraRotation								= 0;
	cameraRotation															+= (float)framework.Input.MouseCurrent.Deltas.x / 20.0f;
	camera.Position	.RotateY(cameraRotation);
	//camera.Position	.RotateY(frameInfo.Microseconds.Total / 1000000.0f);
	lightPos		.RotateY(frameInfo.Microseconds.Total /  250000.0f);
	::llc::SCoord3<float>														cameraUp									= {0, 1, 0};
	const ::llc::SCoord3<float>													cameraFront									= (camera.Target - camera.Position).Normalize();
	const ::llc::SCoord3<float>													cameraRight									= cameraUp.Cross(cameraFront).Normalize();
	cameraUp																= cameraFront.Cross(cameraRight).Normalize();
	viewMatrix.View3D(camera.Position, cameraRight, cameraUp, cameraFront);
	//viewMatrix.LookAt(camera.Position, camera.Target, cameraUp);
	::llc::SMatrix4<float>														xViewProjection								= viewMatrix * projection;
	lightPos.Normalize();
	::llc::SMatrix4<float>														xWorld										= {};
	::llc::SMatrix4<float>														xRotation									= {};
	xRotation.Identity();
	applicationInstance.BoxPivot.Scale										= {.5f, 1.0f, 2.5f};
	applicationInstance.BoxPivot.Position									= {0, 0, 0};

	::llc::array_pod<::llc::SColorBGRA>											& triangle3dColorList						= applicationInstance.RenderCache.Triangle3dColorList	;
	::llc::array_pod<::llc::SCoord3<float>>										& transformedNormals						= applicationInstance.RenderCache.TransformedNormals	;
	::llc::array_pod<::llc::STriangle3D<float>>									& triangle3dToDraw							= applicationInstance.RenderCache.Triangle3dToDraw		;
	::llc::array_pod<::llc::STriangle2D<int32_t>>								& triangle2dToDraw							= applicationInstance.RenderCache.Triangle2dToDraw		;
	::llc::array_pod<int16_t>													& triangle3dIndices							= applicationInstance.RenderCache.Triangle3dIndices		;
	::llc::array_pod<int16_t>													& triangle2dIndices							= applicationInstance.RenderCache.Triangle2dIndices		;
	::llc::array_pod<int16_t>													& triangle2d23dIndices						= applicationInstance.RenderCache.Triangle2d23dIndices	;

	const double																& fFar										= applicationInstance.CameraFar		;
	const double																& fNear										= applicationInstance.CameraNear	;
	for(uint32_t iBox = 0; iBox < 20; ++iBox) {
		applicationInstance.BoxPivot.Position.x								= (float)iBox;
		applicationInstance.BoxPivot.Scale.z								= iBox / 10.0f;
		xWorld		.Scale			(applicationInstance.BoxPivot.Scale, true);
		xRotation	.SetOrientation	((applicationInstance.BoxPivot.Orientation + ::llc::SQuaternion<float>{0, (float)(iBox / ::llc::math_2pi), 0, 0}).Normalize());
		xWorld																	= xWorld * xRotation;
		xWorld		.SetTranslation(applicationInstance.BoxPivot.Position, false);
		//triangle3dColorList		.clear();
		//transformedNormals		.clear();
		triangle3dToDraw		.clear();
		triangle2dToDraw		.clear();
		triangle3dIndices		.clear();
		triangle2dIndices		.clear();
		triangle2d23dIndices	.clear();
		::llc::SCoord2<int32_t>														offscreenMetricsI							= offscreenMetrics.Cast<int32_t>();
		for(uint32_t iTriangle = 0; iTriangle < 12; ++iTriangle) {
			::llc::STriangle3D<float>													transformedTriangle3D						= applicationInstance.Box.Positions[iTriangle];
			::llc::transform(transformedTriangle3D, xWorld * xViewProjection);
			if(transformedTriangle3D.A.z >= fFar	|| transformedTriangle3D.B.z >= fFar	|| transformedTriangle3D.C.z >= fFar ) 
				continue;
			if(transformedTriangle3D.A.z <= fNear	|| transformedTriangle3D.B.z <= fNear	|| transformedTriangle3D.C.z <= fNear) 
				continue;
			triangle3dToDraw	.push_back(transformedTriangle3D);
			triangle3dIndices	.push_back((int16_t)iTriangle);
		}
		for(uint32_t iTriangle = 0, triCount = triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) {
			const ::llc::STriangle3D<float>												& transformedTriangle3D						= triangle3dToDraw[iTriangle];
			::llc::STriangle2D<int32_t>													transformedTriangle2D						= {};
			transformedTriangle2D.A													= {(int32_t)transformedTriangle3D.A.x, (int32_t)transformedTriangle3D.A.y};
			transformedTriangle2D.B													= {(int32_t)transformedTriangle3D.B.x, (int32_t)transformedTriangle3D.B.y};
			transformedTriangle2D.C													= {(int32_t)transformedTriangle3D.C.x, (int32_t)transformedTriangle3D.C.y};
//			::llc::translate(transformedTriangle2D, screenCenter);
			if(transformedTriangle2D.A.x < 0 && transformedTriangle2D.B.x < 0 && transformedTriangle2D.C.x < 0) 
				continue;
			if(transformedTriangle2D.A.y < 0 && transformedTriangle2D.B.y < 0 && transformedTriangle2D.C.y < 0) 
				continue;
			if( transformedTriangle2D.A.x >= offscreenMetricsI.x
			 && transformedTriangle2D.B.x >= offscreenMetricsI.x
			 && transformedTriangle2D.C.x >= offscreenMetricsI.x
			 )
				continue;
			if( transformedTriangle2D.A.y >= offscreenMetricsI.y
			 && transformedTriangle2D.B.y >= offscreenMetricsI.y
			 && transformedTriangle2D.C.y >= offscreenMetricsI.y
			 )
				continue;
			triangle2dToDraw	.push_back(transformedTriangle2D);
			triangle2dIndices	.push_back(triangle3dIndices[iTriangle]);
			triangle2d23dIndices.push_back((int16_t)iTriangle);
		}
		transformedNormals	.resize(triangle2dToDraw.size());
		triangle3dColorList	.resize(triangle2dToDraw.size());
		for(uint32_t iTriangle = 0, triCount = triangle2dToDraw.size(); iTriangle < triCount; ++iTriangle) // transform normals
			transformedNormals[iTriangle]											= xWorld.TransformDirection(applicationInstance.Box.Normals[triangle3dIndices[triangle2d23dIndices[iTriangle]]]).Normalize();

		for(uint32_t iTriangle = 0, triCount = triangle2dToDraw.size(); iTriangle < triCount; ++iTriangle) { // calculate lighting 
			const double																lightFactor									= transformedNormals[iTriangle].Dot(lightPos);
			triangle3dColorList[iTriangle]											= ((0 == (iBox % 2)) ? ::llc::GREEN : ::llc::MAGENTA) * lightFactor;
		}

		::llc::array_pod<::llc::SCoord2<int32_t>>									trianglePixelCoords;
		::llc::array_pod<::llc::SCoord2<int16_t>>									wireframePixelCoords;
		for(uint32_t iTriangle = 0, triCount = triangle2dIndices.size(); iTriangle < triCount; ++iTriangle) { // 
			const double																cameraFactor								= transformedNormals[iTriangle].Dot(cameraFront);
			if(cameraFactor > .35)
				continue;
			//error_if(errored(::llc::drawTriangle(offscreen.View, triangle3dColorList[iTriangle], triangle2dToDraw[iTriangle])), "Not sure if these functions could ever fail");
			trianglePixelCoords.clear();
			error_if(errored(::llc::drawTriangle(offscreenMetrics, triangle2dToDraw[iTriangle], trianglePixelCoords)), "Not sure if these functions could ever fail");
			for(uint32_t iPixel = 0, pixCount = trianglePixelCoords.size(); iPixel < pixCount; ++iPixel) {
				const ::llc::SCoord2<int32_t>												pixelCoord									= trianglePixelCoords[iPixel];
				offscreen.View[pixelCoord.y][pixelCoord.x]								= triangle3dColorList[iTriangle];
			}
		}
	}
	//------------------------------------------------
	static constexpr const ::llc::SCoord2<int32_t>								sizeCharCell								= {9, 16};
	uint32_t																	lineOffset									= 0;
	static constexpr const char													textLine0	[]								= "W: Up, S: Down, A: Left, D: Right";
	static constexpr const char													textLine1	[]								= "T: Shoot. Y: Thrust. U: Handbrake.";
	static constexpr const char													textLine2	[]								= "Press ESC to exit.";
	::llc::grid_view<::llc::SColorBGRA>											& offscreenView								= applicationInstance.Framework.Offscreen.View;
	const ::llc::grid_view<::llc::SColorBGRA>									& fontAtlasView								= applicationInstance.TextureFont.View;
	::textDrawAlignedFixedSize(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), lineOffset = offscreenMetrics.y / sizeCharCell.y - 1, offscreenMetrics, sizeCharCell, textLine2, ::llc::SColorBGRA{0, framework.FrameInfo.FrameNumber % 0xFFU, 0, 0xFFU});
	::llc::STimer																& timer										= applicationInstance.Framework.Timer;
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	char																		buffer		[256]							= {};
	uint32_t																	textLen										= (uint32_t)sprintf_s(buffer, "[%u x %u]. FPS: %g. Last frame seconds: %g.", mainWindow.Size.x, mainWindow.Size.y, 1 / timer.LastTimeSeconds, timer.LastTimeSeconds);
	//::textDrawAlignedFixedSize(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen}, ::llc::SColorBGRA{0, framework.FrameInfo.FrameNumber % 0xFFU, 0, 0xFFU});;
	::textDrawAlignedFixedSize(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	return 0;																																																
}
	