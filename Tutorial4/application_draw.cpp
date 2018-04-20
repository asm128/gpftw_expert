#include "application.h"
#include "llc_bitmap_target.h"

					::llc::error_t										drawBoxes									(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	::llc::STexture<uint32_t>													& offscreenDepth							= framework.OffscreenDepthStencil;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	::memset(offscreen		.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel)	* offscreen			.Texels.size());	// Clear target.
	for(uint32_t y = 0, yMax = offscreenMetrics.y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = offscreenMetrics.x; x < xMax; ++x)
		offscreen.View[y][x]													= {(uint8_t)(y / 10), 0, 0, 0xFF};

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
	for(uint32_t iBox = 0; iBox < 1; ++iBox) {
		applicationInstance.BoxPivot.Position.x									= (float)iBox;
		//applicationInstance.BoxPivot.Scale.z									= iBox / 12.5f + .1f;
		//applicationInstance.BoxPivot.Scale.y									= 1; //iBox / 10.0f + .1f; //powf(2.0f, (float)iBox / 2) * .0125f;
		xWorld		.Scale			(applicationInstance.BoxPivot.Scale, true);
		//xRotation	.SetOrientation	((applicationInstance.BoxPivot.Orientation + ::llc::SQuaternion<float>{0, (float)(iBox / ::llc::math_2pi), 0, 0}).Normalize());
		xRotation	.SetOrientation	(applicationInstance.BoxPivot.Orientation.Normalize());
		xWorld																	= xWorld * xRotation;
		xWorld		.SetTranslation	(applicationInstance.BoxPivot.Position, false);
		::llc::clear
			( renderCache.Triangle3dWorld
			, renderCache.Triangle3dToDraw		
			, renderCache.Triangle3dIndices		
			);
		const ::llc::SMatrix4<float>												xWV											= xWorld * viewMatrix;
		//const ::llc::SCoord3<float>													& cameraFront								= applicationInstance.Scene.Camera.Vectors.Front;
		//const ::llc::SMatrix4<float>												finalTransform								= xWorld * xViewProjection;
		for(uint32_t iTriangle = 0, triCount = applicationInstance.Box.Positions.size(); iTriangle < triCount; ++iTriangle) {
			::llc::STriangle3D<float>													triangle3DWorld								= applicationInstance.Box.Positions[iTriangle];
			::llc::STriangle3D<float>													transformedTriangle3D						= triangle3DWorld;
			::llc::transform(transformedTriangle3D, xWV);
			if( transformedTriangle3D.A.z >= fFar
			 && transformedTriangle3D.B.z >= fFar	
			 && transformedTriangle3D.C.z >= fFar) 
				continue;
			if( (transformedTriangle3D.A.z <= fNear) // && transformedTriangle3D.B.z <= (fNear + .01) )  	
			 || (transformedTriangle3D.B.z <= fNear) // && transformedTriangle3D.C.z <= (fNear + .01) )  	
			 || (transformedTriangle3D.C.z <= fNear) // && transformedTriangle3D.A.z <= (fNear + .01) )  	
			 ) 
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
			::llc::transform(triangle3DWorld, xWorld);
			llc_necall(renderCache.Triangle3dIndices	.push_back((int16_t)iTriangle)		, "Out of memory?");
			llc_necall(renderCache.Triangle3dWorld		.push_back(triangle3DWorld)	, "Out of memory?");
			llc_necall(renderCache.Triangle3dToDraw		.push_back(transformedTriangle3D)	, "Out of memory?");
		}
		llc_necall(::llc::resize(renderCache.Triangle3dIndices.size()
			, renderCache.TransformedNormalsTriangle
			, renderCache.TransformedNormalsVertex		
			, renderCache.Triangle3dColorList			
			), "Out of memory?");

		const ::llc::SCoord3<float>													& lightDir									= applicationInstance.LightDirection;
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) { // transform normals
			::llc::SCoord3<float>														& transformedNormalTri						= renderCache.TransformedNormalsTriangle[iTriangle];
			transformedNormalTri													= xWorld.TransformDirection(applicationInstance.Box.NormalsTriangle[renderCache.Triangle3dIndices[iTriangle]]).Normalize();
			const double																lightFactor									= fabs(transformedNormalTri.Dot(lightDir));
			renderCache.Triangle3dColorList[iTriangle]								= ::llc::LIGHTGRAY * lightFactor;
			const ::llc::STriangle3D<float>												& vertNormalsTriOrig						= applicationInstance.Box.NormalsVertex[renderCache.Triangle3dIndices[iTriangle]];
			::llc::STriangle3D<float>													& vertNormalsTri							= renderCache.TransformedNormalsVertex[iTriangle];
			vertNormalsTri.A														= xWorld.TransformDirection(vertNormalsTriOrig.A).Normalize();
			vertNormalsTri.B														= xWorld.TransformDirection(vertNormalsTriOrig.B).Normalize();
			vertNormalsTri.C														= xWorld.TransformDirection(vertNormalsTriOrig.C).Normalize();
		}

		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) { // 
			const double																cameraFactor								= renderCache.TransformedNormalsTriangle[iTriangle].Dot(applicationInstance.Scene.Camera.Vectors.Front);
			if(cameraFactor > .65)
				continue;
			renderCache.TrianglePixelCoords.clear();
			renderCache.TrianglePixelWeights.clear();
			error_if(errored(::llc::drawTriangle(offscreenDepth.View, fFar, fNear, renderCache.Triangle3dToDraw[iTriangle], renderCache.TrianglePixelCoords, renderCache.TrianglePixelWeights)), "Not sure if these functions could ever fail");
			//if(0 != renderCache.TrianglePixelCoords.size())
			++renderCache.TrianglesDrawn;
			const int32_t																iBoxTri										= renderCache.Triangle3dIndices[iTriangle];
			const ::llc::STriangle2D<float>												& uvBox										= applicationInstance.Box.UVs[iBoxTri];
			::llc::SColorFloat															bleh										= renderCache.Triangle3dColorList[iTriangle];
			for(uint32_t iPixel = 0, pixCount = renderCache.TrianglePixelCoords.size(); iPixel < pixCount; ++iPixel) {
				const ::llc::SCoord2<int32_t>												& pixelCoord								= renderCache.TrianglePixelCoords	[iPixel];
				const ::llc::STriangleWeights<double>										& pixelWeights								= renderCache.TrianglePixelWeights	[iPixel];
				const ::llc::SColorBGRA														ambientColor								= {0xF, 0xF, 0xF, 0xFF};	//(((::llc::DARKRED * pixelWeights.A) + (::llc::DARKGREEN * pixelWeights.B) + (::llc::DARKBLUE * pixelWeights.C))) * .1;
				const ::llc::SColorFloat													interpolatedColor							= {1, 1, 1, 1}; //((::llc::RED * pixelWeights.A) + (::llc::GREEN * pixelWeights.B) + (::llc::BLUE * pixelWeights.C));
				::llc::STriangle3D<float>													weightedNormals								= renderCache.TransformedNormalsVertex[iTriangle]; //((::llc::RED * pixelWeights.A) + (::llc::GREEN * pixelWeights.B) + (::llc::BLUE * pixelWeights.C));
				weightedNormals.A														*= pixelWeights.A;
				weightedNormals.B														*= pixelWeights.B;
				weightedNormals.C														*= pixelWeights.C;
				const ::llc::SCoord3<double>												interpolatedNormal							= (weightedNormals.A + weightedNormals.B + weightedNormals.C).Cast<double>().Normalize();
				bleh																	= interpolatedColor * interpolatedNormal.Dot(lightDir.Cast<double>());
				::llc::SCoord2<double>														uv											= 
					{ uvBox.A.x * pixelWeights.A + uvBox.B.x * pixelWeights.B + uvBox.C.x * pixelWeights.C
					, uvBox.A.y * pixelWeights.A + uvBox.B.y * pixelWeights.B + uvBox.C.y * pixelWeights.C
					};
				::llc::SColorBGRA															interpolatedBGRA;
				if( 0 == applicationInstance.TextureBox.View.metrics().x
				 ||	0 == applicationInstance.TextureBox.View.metrics().y
				 ) 
					interpolatedBGRA														= bleh + ambientColor;
				else {
					::llc::SCoord2<int32_t>														uvcoords									= 
						{ (int32_t)((uint32_t)(uv.x * applicationInstance.TextureBox.View.metrics().x) % applicationInstance.TextureBox.View.metrics().x)
						, (int32_t)((uint32_t)(uv.y * applicationInstance.TextureBox.View.metrics().y) % applicationInstance.TextureBox.View.metrics().y)
						};
					::llc::SColorBGRA															& srcTexel									= applicationInstance.TextureBox[uvcoords.y][uvcoords.x];
					if(srcTexel == ::llc::SColorBGRA{0xFF, 0, 0xFF, 0xFF}) {
						++pixelsSkipped;
						continue;
					}
					interpolatedBGRA														= ::llc::SColorFloat(srcTexel) * bleh + ambientColor;
				}
				::llc::SColorBGRA															& targetColorCell							= offscreen.View[pixelCoord.y][pixelCoord.x];
				if( targetColorCell == interpolatedBGRA ) 
					++pixelsSkipped;
				else {
					targetColorCell															= interpolatedBGRA;
					++pixelsDrawn;
				}
			}
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].A, renderCache.Triangle3dToDraw[iTriangle].B}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].B, renderCache.Triangle3dToDraw[iTriangle].C}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].C, renderCache.Triangle3dToDraw[iTriangle].A}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			// draw normals
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].A, renderCache.Triangle3dToDraw[iTriangle].A + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].B, renderCache.Triangle3dToDraw[iTriangle].B + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].C, renderCache.Triangle3dToDraw[iTriangle].C + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
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