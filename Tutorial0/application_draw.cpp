#include "application.h"
#include "llc_bitmap_target.h"

					::llc::error_t										drawBoxes									(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	::memset(offscreen.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel) * offscreen.Texels.size());	// Clear target.

	::llc::array_pod<::llc::SColorBGRA>											& triangle3dColorList						= applicationInstance.RenderCache.Triangle3dColorList	;
	::llc::array_pod<::llc::SCoord3<float>>										& transformedNormals						= applicationInstance.RenderCache.TransformedNormals	;
	::llc::array_pod<::llc::STriangle3D<float>>									& triangle3dToDraw							= applicationInstance.RenderCache.Triangle3dToDraw		;
	::llc::array_pod<::llc::STriangle2D<int32_t>>								& triangle2dToDraw							= applicationInstance.RenderCache.Triangle2dToDraw		;
	::llc::array_pod<int16_t>													& triangle3dIndices							= applicationInstance.RenderCache.Triangle3dIndices		;
	::llc::array_pod<int16_t>													& triangle2dIndices							= applicationInstance.RenderCache.Triangle2dIndices		;
	::llc::array_pod<int16_t>													& triangle2d23dIndices						= applicationInstance.RenderCache.Triangle2d23dIndices	;

	::llc::SMatrix4<float>														& projection								= applicationInstance.XProjection	;
	::llc::SMatrix4<float>														& viewMatrix								= applicationInstance.XView			;
	::llc::SMatrix4<float>														xViewProjection								= viewMatrix * projection;
	::llc::SCoord3<float>														lightPos									= {10, 5, 0};
	::llc::SFrameInfo															& frameInfo									= framework.FrameInfo;
	lightPos.RotateY(frameInfo.Microseconds.Total / 250000.0f);
	::llc::SMatrix4<float>														xWorld										= {};
	::llc::SMatrix4<float>														xRotation									= {};
	xRotation.Identity();
	lightPos.Normalize();

	const double																& fFar										= applicationInstance.CameraFar		;
	const double																& fNear										= applicationInstance.CameraNear	;
	int32_t																		pixelsDrawn									= 0;
	for(uint32_t iBox = 0; iBox < 20; ++iBox) {
		applicationInstance.BoxPivot.Position.x									= (float)iBox;
		applicationInstance.BoxPivot.Scale.z									= iBox / 10.0f;
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
		const ::llc::SCoord3<float>													& cameraFront								= applicationInstance.CameraVectors.CameraFront;
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
			if(transformedTriangle3D.A.x < 0 && transformedTriangle3D.B.x < 0 && transformedTriangle3D.C.x < 0) 
				continue;
			if(transformedTriangle3D.A.y < 0 && transformedTriangle3D.B.y < 0 && transformedTriangle3D.C.y < 0) 
				continue;
			if( transformedTriangle3D.A.x >= offscreenMetricsI.x
			 && transformedTriangle3D.B.x >= offscreenMetricsI.x
			 && transformedTriangle3D.C.x >= offscreenMetricsI.x
			 )
				continue;
			if( transformedTriangle3D.A.y >= offscreenMetricsI.y
			 && transformedTriangle3D.B.y >= offscreenMetricsI.y
			 && transformedTriangle3D.C.y >= offscreenMetricsI.y
			 )
				continue;
			const ::llc::STriangle2D<int32_t>											transformedTriangle2D						= 
				{ {(int32_t)transformedTriangle3D.A.x, (int32_t)transformedTriangle3D.A.y}
				, {(int32_t)transformedTriangle3D.B.x, (int32_t)transformedTriangle3D.B.y}
				, {(int32_t)transformedTriangle3D.C.x, (int32_t)transformedTriangle3D.C.y}
				};
			llc_necall(triangle2dToDraw		.push_back(transformedTriangle2D)		, "?");
			llc_necall(triangle2dIndices	.push_back(triangle3dIndices[iTriangle]), "?");
			llc_necall(triangle2d23dIndices	.push_back((int16_t)iTriangle)			, "?");
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
				const ::llc::SCoord2<int32_t>												& pixelCoord								= trianglePixelCoords[iPixel];
				if( offscreen.View[pixelCoord.y][pixelCoord.x] != triangle3dColorList[iTriangle] ) {
					offscreen.View[pixelCoord.y][pixelCoord.x]								= triangle3dColorList[iTriangle];
					++pixelsDrawn;
				}
			}
		}
	}
	return pixelsDrawn;
}