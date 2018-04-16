#include "application.h"
#include "llc_bitmap_target.h"

					::llc::error_t										drawBoxes									(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	::llc::STexture<uint32_t>													& offscreenDepth							= framework.OffscreenDepthStencil;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	::memset(offscreen		.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel)	* offscreen			.Texels.size());	// Clear target.
	//::memset(offscreenDepth	.Texels.begin(), -1, sizeof(uint32_t)								* offscreenDepth	.Texels.size());	// Clear target.
	::memset(offscreenDepth	.Texels.begin(), -1, sizeof(uint32_t)								* offscreenDepth	.Texels.size());	// Clear target.

	::SRenderCache																& renderCache								= applicationInstance.RenderCache;

	const ::llc::SMatrix4<float>												& projection								= applicationInstance.Scene.Transforms.FinalProjection	;
	const ::llc::SMatrix4<float>												& viewMatrix								= applicationInstance.Scene.Transforms.View				;

	::llc::SMatrix4<float>														xViewProjection								= viewMatrix * projection;
	::llc::SMatrix4<float>														xWorld										= {};
	::llc::SMatrix4<float>														xRotation									= {};
	const double																& fFar										= applicationInstance.Scene.Camera.Range.Far	;
	const double																& fNear										= applicationInstance.Scene.Camera.Range.Near	;
	int32_t																		& pixelsDrawn								= applicationInstance.RenderCache.PixelsDrawn	= 0;
	int32_t																		& pixelsSkipped								= applicationInstance.RenderCache.PixelsSkipped	= 0;
	renderCache.WireframePixelCoords.clear();
	renderCache.TrianglesDrawn												= 0;
	const ::llc::SCoord2<int32_t>												offscreenMetricsI							= offscreenMetrics.Cast<int32_t>();
	const ::llc::SCoord3<float>													screenCenter								= {offscreenMetricsI.x / 2.0f, offscreenMetricsI.y / 2.0f, };
	for(uint32_t iBox = 0; iBox < 10; ++iBox) {
		applicationInstance.BoxPivot.Position.x									= (float)iBox;
		applicationInstance.BoxPivot.Scale.z									= iBox / 2.5f + .1f;
		applicationInstance.BoxPivot.Scale.y									= iBox / 10.0f + .1f; //powf(2.0f, (float)iBox / 2) * .0125f;
		xWorld		.Scale			(applicationInstance.BoxPivot.Scale, true);
		xRotation	.SetOrientation	((applicationInstance.BoxPivot.Orientation + ::llc::SQuaternion<float>{0, (float)(iBox / ::llc::math_2pi), 0, 0}).Normalize());
		xWorld																	= xWorld * xRotation;
		xWorld		.SetTranslation	(applicationInstance.BoxPivot.Position, false);
		::llc::clear
			( renderCache.Triangle3dToDraw		
			, renderCache.Triangle2dToDraw		
			, renderCache.Triangle3dIndices		
			);
		//const ::llc::SCoord3<float>													& cameraFront								= applicationInstance.Scene.Camera.Vectors.Front;
		const ::llc::SMatrix4<float>												finalTransform								= xWorld * xViewProjection;
		for(uint32_t iTriangle = 0, triCount = applicationInstance.Box.Positions.size(); iTriangle < triCount; ++iTriangle) {
			::llc::STriangle3D<float>													transformedTriangle3D						= applicationInstance.Box.Positions[iTriangle];
			::llc::transform(transformedTriangle3D, xWorld);
			::llc::transform(transformedTriangle3D, viewMatrix);
			if( transformedTriangle3D.A.z >= fFar
			 && transformedTriangle3D.B.z >= fFar	
			 && transformedTriangle3D.C.z >= fFar) 
				continue;
			if( transformedTriangle3D.A.z <= (fNear + .01) 	
			 || transformedTriangle3D.B.z <= (fNear + .01) 	
			 || transformedTriangle3D.C.z <= (fNear + .01) ) 
				continue;

			float																		oldzA										= transformedTriangle3D.A.z;
			float																		oldzB										= transformedTriangle3D.B.z;
			float																		oldzC										= transformedTriangle3D.C.z;

			::llc::transform(transformedTriangle3D, projection);
			transformedTriangle3D.A.z												= oldzA;
			transformedTriangle3D.B.z												= oldzB;
			transformedTriangle3D.C.z												= oldzC;


			//::llc::translate(transformedTriangle3D, screenCenter);

			// Check against far and near planes
			// Check against screen limits
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
			llc_necall(renderCache.Triangle3dToDraw		.push_back(transformedTriangle3D)	, "Out of memory?");
			llc_necall(renderCache.Triangle3dIndices	.push_back((int16_t)iTriangle)		, "Out of memory?");
			llc_necall(renderCache.Triangle2dToDraw		.push_back(
				{ {(int32_t)transformedTriangle3D.A.x, (int32_t)transformedTriangle3D.A.y}
				, {(int32_t)transformedTriangle3D.B.x, (int32_t)transformedTriangle3D.B.y}
				, {(int32_t)transformedTriangle3D.C.x, (int32_t)transformedTriangle3D.C.y}
				}), "Out of memory?");
		}
		llc_necall(::llc::resize(renderCache.Triangle2dToDraw.size()
			, renderCache.TransformedNormalsTriangle
			, renderCache.TransformedNormalsVertex		
			, renderCache.Triangle3dColorList			
			), "Out of memory?");
		const ::llc::SCoord3<float>													& lightDir									= applicationInstance.LightDirection;
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dToDraw.size(); iTriangle < triCount; ++iTriangle) { // transform normals
			::llc::SCoord3<float>														& transformedNormalTri						= renderCache.TransformedNormalsTriangle[iTriangle];
			transformedNormalTri													= xWorld.TransformDirection(applicationInstance.Box.NormalsTriangle[iTriangle]).Normalize();
			const double																lightFactor									= transformedNormalTri.Dot(lightDir);
			renderCache.Triangle3dColorList[iTriangle]								= ((0 == (iBox % 2)) ? ::llc::GREEN : ::llc::MAGENTA) * lightFactor;
			::llc::SCoord3<float>														& transformedNormalVer0						= renderCache.TransformedNormalsVertex[iTriangle].A;
			::llc::SCoord3<float>														& transformedNormalVer1						= renderCache.TransformedNormalsVertex[iTriangle].B;
			::llc::SCoord3<float>														& transformedNormalVer2						= renderCache.TransformedNormalsVertex[iTriangle].C;
			transformedNormalVer0													= xWorld.TransformDirection(applicationInstance.Box.NormalsVertex[iTriangle].A).Normalize();
			transformedNormalVer1													= xWorld.TransformDirection(applicationInstance.Box.NormalsVertex[iTriangle].B).Normalize();
			transformedNormalVer2													= xWorld.TransformDirection(applicationInstance.Box.NormalsVertex[iTriangle].C).Normalize();
		}

		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dToDraw.size(); iTriangle < triCount; ++iTriangle) { // 
			//const double																cameraFactor								= renderCache.TransformedNormalsTriangle[iTriangle].Dot(cameraFront);
			//if(cameraFactor > .65)
			//	continue;
			++renderCache.TrianglesDrawn;
			//error_if(errored(::llc::drawTriangle(offscreen.View, renderCache.Triangle3dColorList[iTriangle], renderCache.Triangle2dToDraw[iTriangle])), "Not sure if these functions could ever fail");
			renderCache.TrianglePixelCoords.clear();
			renderCache.TrianglePixelWeights.clear();
			error_if(errored(::llc::drawTriangle(offscreenDepth.View, fFar, fNear, renderCache.Triangle3dToDraw[iTriangle], renderCache.TrianglePixelCoords, renderCache.TrianglePixelWeights)), "Not sure if these functions could ever fail");
			for(uint32_t iPixel = 0, pixCount = renderCache.TrianglePixelCoords.size(); iPixel < pixCount; ++iPixel) {
				const ::llc::SCoord2<int32_t>												& pixelCoord								= renderCache.TrianglePixelCoords[iPixel];
				if( offscreen.View[pixelCoord.y][pixelCoord.x] != renderCache.Triangle3dColorList[iTriangle] ) {
					offscreen.View[pixelCoord.y][pixelCoord.x]								= renderCache.Triangle3dColorList[iTriangle];
					++pixelsDrawn;
				}
				else
					++pixelsSkipped;
			}
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine2D<int32_t>{renderCache.Triangle2dToDraw[iTriangle].A, renderCache.Triangle2dToDraw[iTriangle].B}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine2D<int32_t>{renderCache.Triangle2dToDraw[iTriangle].B, renderCache.Triangle2dToDraw[iTriangle].C}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine2D<int32_t>{renderCache.Triangle2dToDraw[iTriangle].C, renderCache.Triangle2dToDraw[iTriangle].A}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
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