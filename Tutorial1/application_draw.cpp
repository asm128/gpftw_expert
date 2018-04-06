#include "application.h"
#include "llc_bitmap_target.h"

					::llc::error_t										drawBoxes									(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	::memset(offscreen.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel) * offscreen.Texels.size());	// Clear target.

	::SRenderCache																& renderCache								= applicationInstance.RenderCache;

	const ::llc::SMatrix4<float>												& projection								= applicationInstance.XProjection	;
	const ::llc::SMatrix4<float>												& viewMatrix								= applicationInstance.XView			;

	::llc::SMatrix4<float>														xViewProjection								= viewMatrix * projection;
	::llc::SMatrix4<float>														xWorld										= {};
	::llc::SMatrix4<float>														xRotation									= {};
	const double																& fFar										= applicationInstance.Camera.Range.Far	;
	const double																& fNear										= applicationInstance.Camera.Range.Near	;
	int32_t																		& pixelsDrawn								= applicationInstance.RenderCache.PixelsDrawn	= 0;
	int32_t																		& pixelsSkipped								= applicationInstance.RenderCache.PixelsSkipped	= 0;
	renderCache.WireframePixelCoords.clear();
	for(uint32_t iBox = 0; iBox < 20; ++iBox) {
		applicationInstance.BoxPivot.Position.x									= (float)iBox;
		applicationInstance.BoxPivot.Scale.z									= iBox / 10.0f + .1f;
		applicationInstance.BoxPivot.Scale.y									= iBox / 20.0f + .1f;
		xWorld		.Scale			(applicationInstance.BoxPivot.Scale, true);
		xRotation	.SetOrientation	((applicationInstance.BoxPivot.Orientation + ::llc::SQuaternion<float>{0, (float)(iBox / ::llc::math_2pi), 0, 0}).Normalize());
		xWorld																	= xWorld * xRotation;
		xWorld		.SetTranslation(applicationInstance.BoxPivot.Position, false);
		::llc::clear
			( renderCache.Triangle3dToDraw		
			, renderCache.Triangle2dToDraw		
			, renderCache.Triangle3dIndices		
			, renderCache.Triangle2dIndices		
			, renderCache.Triangle2d23dIndices	
			);
		const ::llc::SCoord2<int32_t>												offscreenMetricsI							= offscreenMetrics.Cast<int32_t>();
		const ::llc::SCoord3<float>													& cameraFront								= applicationInstance.Camera.Vectors.Front;
		for(uint32_t iTriangle = 0; iTriangle < 12; ++iTriangle) {
			::llc::STriangle3D<float>													transformedTriangle3D						= applicationInstance.Box.Positions[iTriangle];
			::llc::transform(transformedTriangle3D, xWorld * xViewProjection);
			if(transformedTriangle3D.A.z >= fFar	|| transformedTriangle3D.B.z >= fFar	|| transformedTriangle3D.C.z >= fFar ) 
				continue;
			if(transformedTriangle3D.A.z <= fNear	|| transformedTriangle3D.B.z <= fNear	|| transformedTriangle3D.C.z <= fNear) 
				continue;
			llc_necall(renderCache.Triangle3dToDraw		.push_back(transformedTriangle3D)	, "Out of memory?");
			llc_necall(renderCache.Triangle3dIndices	.push_back((int16_t)iTriangle)		, "Out of memory?");
		}
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) {
			const ::llc::STriangle3D<float>												& transformedTriangle3D						= renderCache.Triangle3dToDraw[iTriangle];
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
			llc_necall(renderCache.Triangle2dToDraw		.push_back(transformedTriangle2D)		, "Out of memory?");
			llc_necall(renderCache.Triangle2dIndices	.push_back(renderCache.Triangle3dIndices[iTriangle]), "Out of memory?");
			llc_necall(renderCache.Triangle2d23dIndices	.push_back((int16_t)iTriangle)			, "Out of memory?");
		}
		llc_necall(renderCache.TransformedNormals	.resize(renderCache.Triangle2dToDraw.size()), "Out of memory?");
		llc_necall(renderCache.Triangle3dColorList	.resize(renderCache.Triangle2dToDraw.size()), "Out of memory?");
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle2dToDraw.size(); iTriangle < triCount; ++iTriangle) // transform normals
			renderCache.TransformedNormals[iTriangle]											= xWorld.TransformDirection(applicationInstance.Box.Normals[renderCache.Triangle3dIndices[renderCache.Triangle2d23dIndices[iTriangle]]]).Normalize();

		const ::llc::SCoord3<float>													& lightPos									= applicationInstance.LightPosition;
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle2dToDraw.size(); iTriangle < triCount; ++iTriangle) { // calculate lighting 
			const double																lightFactor									= renderCache.TransformedNormals[iTriangle].Dot(lightPos);
			renderCache.Triangle3dColorList[iTriangle]											= ((0 == (iBox % 2)) ? ::llc::GREEN : ::llc::MAGENTA) * lightFactor;
		}

		renderCache.TrianglePixelCoords.clear();
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle2dIndices.size(); iTriangle < triCount; ++iTriangle) { // 
			const double																cameraFactor								= renderCache.TransformedNormals[iTriangle].Dot(cameraFront);
			if(cameraFactor > .35)
				continue;
			//error_if(errored(::llc::drawTriangle(offscreen.View, triangle3dColorList[iTriangle], triangle2dToDraw[iTriangle])), "Not sure if these functions could ever fail");
			renderCache.TrianglePixelCoords.clear();
			error_if(errored(::llc::drawTriangle(offscreenMetrics, renderCache.Triangle2dToDraw[iTriangle], renderCache.TrianglePixelCoords)), "Not sure if these functions could ever fail");
			for(uint32_t iPixel = 0, pixCount = renderCache.TrianglePixelCoords.size(); iPixel < pixCount; ++iPixel) {
				const ::llc::SCoord2<int32_t>												& pixelCoord								= renderCache.TrianglePixelCoords[iPixel];
				if( offscreen.View[pixelCoord.y][pixelCoord.x] != renderCache.Triangle3dColorList[iTriangle] ) {
					offscreen.View[pixelCoord.y][pixelCoord.x]								= renderCache.Triangle3dColorList[iTriangle];
					++pixelsDrawn;
				}
				else
					++pixelsSkipped;
			}
			error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine2D<int32_t>{renderCache.Triangle2dToDraw[iTriangle].A, renderCache.Triangle2dToDraw[iTriangle].B}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine2D<int32_t>{renderCache.Triangle2dToDraw[iTriangle].B, renderCache.Triangle2dToDraw[iTriangle].C}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine2D<int32_t>{renderCache.Triangle2dToDraw[iTriangle].C, renderCache.Triangle2dToDraw[iTriangle].A}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
		}
	}
	static constexpr const ::llc::SColorBGRA									color										= ::llc::YELLOW;
	for(uint32_t iPixel = 0, pixCount = renderCache.WireframePixelCoords.size(); iPixel < pixCount; ++iPixel) {
		const ::llc::SCoord2<int32_t>												& pixelCoord								= renderCache.WireframePixelCoords[iPixel];
		if( offscreen.View[pixelCoord.y][pixelCoord.x] != color ) {
			offscreen.View[pixelCoord.y][pixelCoord.x]								= color;
			++pixelsDrawn;
		}
		else
			++pixelsSkipped;
	}
	return pixelsDrawn;
}