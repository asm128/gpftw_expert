#include "application.h"
#include "llc_bitmap_target.h"

					::llc::error_t										drawPixelGND									
	( ::SRenderCache															& renderCache
	, ::llc::SColorBGRA															& targetColorCell
	, const ::llc::STriangleWeights<double>										& pixelWeights	
	, const ::llc::STriangle2D<float>											& uvs
	, const ::llc::grid_view<::llc::SColorBGRA>									& textureColors
	, int32_t																	iTriangle
	, const ::llc::SCoord3<double>												& lightDir	

	) {	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	const ::llc::SColorBGRA														ambientColor								= {0xF, 0xF, 0xF, 0xFF};	//(((::llc::DARKRED * pixelWeights.A) + (::llc::DARKGREEN * pixelWeights.B) + (::llc::DARKBLUE * pixelWeights.C))) * .1;
	const ::llc::SColorFloat													interpolatedColor							= {1, 1, 1, 1}; //((::llc::RED * pixelWeights.A) + (::llc::GREEN * pixelWeights.B) + (::llc::BLUE * pixelWeights.C));
	::llc::STriangle3D<float>													weightedNormals								= renderCache.TransformedNormalsVertex[iTriangle]; //((::llc::RED * pixelWeights.A) + (::llc::GREEN * pixelWeights.B) + (::llc::BLUE * pixelWeights.C));
	weightedNormals.A														*= pixelWeights.A;
	weightedNormals.B														*= pixelWeights.B;
	weightedNormals.C														*= pixelWeights.C;
	const ::llc::SCoord3<double>												interpolatedNormal							= (weightedNormals.A + weightedNormals.B + weightedNormals.C).Cast<double>().Normalize();
	//const ::llc::SCoord3<double>												interpolatedNormal							= weightedNormals.B.Cast<double>().Normalize();//(weightedNormals.A + weightedNormals.B + weightedNormals.C).Cast<double>().Normalize();
	const ::llc::SColorFloat													finalColor									= interpolatedColor * interpolatedNormal.Dot(lightDir);
	const ::llc::SCoord2<uint32_t>												textureMetrics								= textureColors.metrics();
	
	::llc::SCoord2<double>														uv											= 
		{ uvs.A.x * pixelWeights.A + uvs.B.x * pixelWeights.B + uvs.C.x * pixelWeights.C
		, uvs.A.y * pixelWeights.A + uvs.B.y * pixelWeights.B + uvs.C.y * pixelWeights.C
		};
	::llc::SColorBGRA															interpolatedBGRA;
	if( 0 == textureMetrics.x
	 ||	0 == textureMetrics.y
	 ) 
		interpolatedBGRA													= finalColor;// + ambientColor;
	else {
		const ::llc::SCoord2<int32_t>														uvcoords									= 
			{ (int32_t)((uint32_t)(uv.x * textureMetrics.x) % textureMetrics.x)
			, (int32_t)((uint32_t)(uv.y * textureMetrics.y) % textureMetrics.y)
			};
		const ::llc::SColorBGRA														& srcTexel									= textureColors[uvcoords.y][uvcoords.x];
		//if(srcTexel == ::llc::SColorBGRA{0xFF, 0, 0xFF, 0xFF}) {
		//	return 1;
		//}
		//interpolatedBGRA														= finalColor + ambientColor;
		interpolatedBGRA														= ::llc::SColorFloat(srcTexel) * finalColor + ambientColor;
	}
	if( targetColorCell == interpolatedBGRA ) 
		return 1;

	targetColorCell															= interpolatedBGRA;
	return 0;
}

//
//					::llc::error_t										drawPixel									
//	( ::SRenderCache															& renderCache
//	, ::llc::SColorBGRA															& targetColorCell
//	, const ::llc::STriangleWeights<double>										& pixelWeights	
//	, const ::llc::STriangle2D<float>											& uvGrid
//	, const ::llc::grid_view<::llc::SColorBGRA>									& textureColors
//	, int32_t																	iTriangle
//	, const ::llc::SCoord3<double>												& lightDir	
//
//	) {	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
//	const ::llc::SColorBGRA														ambientColor								= {0xF, 0xF, 0xF, 0xFF};	//(((::llc::DARKRED * pixelWeights.A) + (::llc::DARKGREEN * pixelWeights.B) + (::llc::DARKBLUE * pixelWeights.C))) * .1;
//	const ::llc::SColorFloat													interpolatedColor							= {1, 1, 1, 1}; //((::llc::RED * pixelWeights.A) + (::llc::GREEN * pixelWeights.B) + (::llc::BLUE * pixelWeights.C));
//	::llc::STriangle3D<float>													weightedNormals								= renderCache.TransformedNormalsVertex[iTriangle]; //((::llc::RED * pixelWeights.A) + (::llc::GREEN * pixelWeights.B) + (::llc::BLUE * pixelWeights.C));
//	weightedNormals.A														*= pixelWeights.A;
//	weightedNormals.B														*= pixelWeights.B;
//	weightedNormals.C														*= pixelWeights.C;
//	const ::llc::SCoord3<double>												interpolatedNormal							= (weightedNormals.A + weightedNormals.B + weightedNormals.C).Cast<double>().Normalize();
//	const ::llc::SColorFloat													finalColor									= interpolatedColor * interpolatedNormal.Dot(lightDir);
//	const ::llc::SCoord2<uint32_t>												textureMetrics								= textureColors.metrics();
//	
//	::llc::SCoord2<double>														uv											= 
//		{ uvGrid.A.x * pixelWeights.A + uvGrid.B.x * pixelWeights.B + uvGrid.C.x * pixelWeights.C
//		, uvGrid.A.y * pixelWeights.A + uvGrid.B.y * pixelWeights.B + uvGrid.C.y * pixelWeights.C
//		};
//	::llc::SColorBGRA															interpolatedBGRA;
//	if( 0 == textureMetrics.x
//	 ||	0 == textureMetrics.y
//	 ) 
//		interpolatedBGRA													= finalColor + ambientColor;
//	else {
//		const ::llc::SCoord2<int32_t>														uvcoords									= 
//			{ (int32_t)((uint32_t)(uv.x * textureMetrics.x) % textureMetrics.x)
//			, (int32_t)((uint32_t)(uv.y * textureMetrics.y) % textureMetrics.y)
//			};
//		const ::llc::SColorBGRA														& srcTexel									= textureColors[uvcoords.y][uvcoords.x];
//		if(srcTexel == ::llc::SColorBGRA{0xFF, 0, 0xFF, 0xFF}) {
//			return 1;
//		}
//		//interpolatedBGRA														= finalColor + ambientColor;
//		interpolatedBGRA														= ::llc::SColorFloat(srcTexel) * finalColor + ambientColor;
//	}
//	if( targetColorCell == interpolatedBGRA ) 
//		return 1;
//
//	targetColorCell															= interpolatedBGRA;
//	return 0;
//}
//


					::llc::error_t										drawGND										(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
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
	//::llc::SMatrix4<float>														xRotation									= {};
	//xRotation.Identity();
	const double																& fFar										= applicationInstance.Scene.Camera.Range.Far	;
	const double																& fNear										= applicationInstance.Scene.Camera.Range.Near	;
	int32_t																		& pixelsDrawn								= applicationInstance.RenderCache.PixelsDrawn	= 0;
	int32_t																		& pixelsSkipped								= applicationInstance.RenderCache.PixelsSkipped	= 0;
	renderCache.WireframePixelCoords.clear();
	renderCache.TrianglesDrawn												= 0;
	const ::llc::SCoord2<int32_t>												offscreenMetricsI							= offscreenMetrics.Cast<int32_t>();
	const ::llc::SCoord3<float>													screenCenter								= {offscreenMetricsI.x / 2.0f, offscreenMetricsI.y / 2.0f, };
	xWorld.Identity();
	::llc::STimer																timerMark									= {};
	for(uint32_t iGNDNode = 0; iGNDNode < applicationInstance.GNDData.TextureNames.size(); ++iGNDNode) {
		const SModelNodeGND															& gndNode									= applicationInstance.GNDModel.Nodes[iGNDNode];
		xWorld		.Scale			(applicationInstance.GridPivot.Scale, true);
		//xRotation	.SetOrientation	((applicationInstance.GridPivot.Orientation + ::llc::SQuaternion<float>{0, (float)(iGrid / ::llc::math_2pi), 0, 0}).Normalize());
		//xRotation	.SetOrientation	(applicationInstance.GridPivot.Orientation.Normalize());
		//xWorld																	= xWorld * xRotation;
		xWorld		.SetTranslation	(applicationInstance.GridPivot.Position, false);
		::llc::clear
			( renderCache.Triangle3dWorld
			, renderCache.Triangle3dToDraw		
			, renderCache.Triangle3dIndices		
			);
		const ::llc::SMatrix4<float>												xWV											= xWorld * viewMatrix;
		//const ::llc::SCoord3<float>													& cameraFront								= applicationInstance.Scene.Camera.Vectors.Front;
		//const ::llc::SMatrix4<float>												finalTransform								= xWorld * xViewProjection;
		for(uint32_t iTriangle = 0, triCount = gndNode.VertexIndices.size(); iTriangle < triCount; ++iTriangle) {
			const ::llc::STriangleWeights<uint32_t>										& vertexIndices								= gndNode.VertexIndices[iTriangle];
			::llc::STriangle3D<float>													triangle3DWorld								= 
				{	gndNode.Vertices[vertexIndices.A]
				,	gndNode.Vertices[vertexIndices.B]
				,	gndNode.Vertices[vertexIndices.C]
				};
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
			if(transformedTriangle3D.A.x < 0 && transformedTriangle3D.B.x < 0 && transformedTriangle3D.C.x < 0) continue;
			if(transformedTriangle3D.A.y < 0 && transformedTriangle3D.B.y < 0 && transformedTriangle3D.C.y < 0) continue;
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
			::llc::transform(triangle3DWorld, xWorld);
			llc_necall(renderCache.Triangle3dWorld		.push_back(triangle3DWorld)			, "Out of memory?");
			llc_necall(renderCache.Triangle3dIndices	.push_back(iTriangle)		, "Out of memory?");
		}
		//timerMark.Frame();
		//info_printf("First iteration: %f.", timerMark.LastTimeSeconds);
		llc_necall(::llc::resize(renderCache.Triangle3dIndices.size()
			//, renderCache.TransformedNormalsTriangle
			, renderCache.TransformedNormalsVertex		
			, renderCache.Triangle3dColorList			
			), "Out of memory?");

		const ::llc::SCoord3<float>													& lightDir									= applicationInstance.LightDirection;
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) { // transform normals
			//::llc::SCoord3<float>														& transformedNormalTri						= renderCache.TransformedNormalsTriangle[iTriangle];
			const ::llc::STriangleWeights<uint32_t>										& vertexIndices								= gndNode.VertexIndices[renderCache.Triangle3dIndices[iTriangle]];
			::llc::STriangle3D<float>													triangle3DWorldNormals						= 
				{ gndNode.Normals[vertexIndices.A]
				, gndNode.Normals[vertexIndices.B]
				, gndNode.Normals[vertexIndices.C]
				};
			//transformedNormalTri													= xWorld.TransformDirection(((triangle3DWorldNormals.A + triangle3DWorldNormals.B + triangle3DWorldNormals.C) / 3).Normalize()).Normalize();
			//const double																lightFactor									= fabs(transformedNormalTri.Dot(lightDir));
			//renderCache.Triangle3dColorList[iTriangle]								= ::llc::LIGHTGRAY * lightFactor;
			//const ::llc::STriangle3D<float>												& vertNormalsTriOrig						= applicationInstance.Grid.NormalsVertex[renderCache.Triangle3dIndices[iTriangle]];
			::llc::STriangle3D<float>													& vertNormalsTri							= renderCache.TransformedNormalsVertex[iTriangle];
			vertNormalsTri.A														= xWorld.TransformDirection(gndNode.Normals[vertexIndices.A]).Normalize();
			vertNormalsTri.B														= xWorld.TransformDirection(gndNode.Normals[vertexIndices.B]).Normalize();
			vertNormalsTri.C														= xWorld.TransformDirection(gndNode.Normals[vertexIndices.C]).Normalize();
		}
		//timerMark.Frame();
		//info_printf("Second iteration: %f.", timerMark.LastTimeSeconds);

		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) { // 
			//const double																cameraFactor								= renderCache.TransformedNormalsTriangle[iTriangle].Dot(applicationInstance.Scene.Camera.Vectors.Front);
			//if(cameraFactor > .65)
			//	continue;
			renderCache.TrianglePixelCoords.clear();
			renderCache.TrianglePixelWeights.clear();
			const ::llc::STriangle3D<float>												& tri3DToDraw								= renderCache.Triangle3dToDraw[iTriangle];
			error_if(errored(::llc::drawTriangle(offscreenDepth.View, fFar, fNear, tri3DToDraw, renderCache.TrianglePixelCoords, renderCache.TrianglePixelWeights)), "Not sure if these functions could ever fail");
			//if(0 != renderCache.TrianglePixelCoords.size())
			++renderCache.TrianglesDrawn;
			//const int32_t																iGridTri									= renderCache.Triangle3dIndices[iTriangle];
			//const ::llc::STriangle2D<float>												& uvGrid										= applicationInstance.Grid.UVs[iGridTri];
			const ::llc::STriangleWeights<uint32_t>										& vertexIndices								= gndNode.VertexIndices[renderCache.Triangle3dIndices[iTriangle]];
			::llc::STriangle2D<float>													triangle3DUVs								= 
				{ gndNode.UVs[vertexIndices.A]
				, gndNode.UVs[vertexIndices.B]
				, gndNode.UVs[vertexIndices.C]
				};
			for(uint32_t iPixel = 0, pixCount = renderCache.TrianglePixelCoords.size(); iPixel < pixCount; ++iPixel) {
				const ::llc::SCoord2<int32_t>												& pixelCoord								= renderCache.TrianglePixelCoords	[iPixel];
				const ::llc::STriangleWeights<double>										& pixelWeights								= renderCache.TrianglePixelWeights	[iPixel];
				if(0 == ::drawPixelGND(renderCache, offscreen.View[pixelCoord.y][pixelCoord.x], pixelWeights, triangle3DUVs, applicationInstance.TexturesGND[iGNDNode].View, iTriangle, lightDir.Cast<double>()))
					++pixelsDrawn;
				else
					++pixelsSkipped;
			}
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].A, renderCache.Triangle3dToDraw[iTriangle].B}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].B, renderCache.Triangle3dToDraw[iTriangle].C}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].C, renderCache.Triangle3dToDraw[iTriangle].A}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			// draw normals
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].A, renderCache.Triangle3dToDraw[iTriangle].A + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].B, renderCache.Triangle3dToDraw[iTriangle].B + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].C, renderCache.Triangle3dToDraw[iTriangle].C + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
		} 
		//timerMark.Frame();
		//info_printf("Third iteration: %f.", timerMark.LastTimeSeconds);

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

//					::llc::error_t										drawGrides									(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
//	::llc::SFramework															& framework									= applicationInstance.Framework;
//	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
//	::llc::STexture<uint32_t>													& offscreenDepth							= framework.OffscreenDepthStencil;
//	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
//	::memset(offscreen		.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel)	* offscreen			.Texels.size());	// Clear target.
//	for(uint32_t y = 0, yMax = offscreenMetrics.y; y < yMax; ++y)
//	for(uint32_t x = 0, xMax = offscreenMetrics.x; x < xMax; ++x)
//		offscreen.View[y][x]													= {(uint8_t)(y / 10), 0, 0, 0xFF};
//
//	//::memset(offscreenDepth	.Texels.begin(), -1, sizeof(uint32_t)								* offscreenDepth	.Texels.size());	// Clear target.
//	::memset(offscreenDepth	.Texels.begin(), -1, sizeof(uint32_t)								* offscreenDepth	.Texels.size());	// Clear target.
//
//	::SRenderCache																& renderCache								= applicationInstance.RenderCache;
//
//	const ::llc::SMatrix4<float>												& projection								= applicationInstance.Scene.Transforms.FinalProjection	;
//	const ::llc::SMatrix4<float>												& viewMatrix								= applicationInstance.Scene.Transforms.View				;
//
//	::llc::SMatrix4<float>														xViewProjection								= viewMatrix * projection;
//	::llc::SMatrix4<float>														xWorld										= {};
//	::llc::SMatrix4<float>														xRotation									= {};
//	const double																& fFar										= applicationInstance.Scene.Camera.Range.Far	;
//	const double																& fNear										= applicationInstance.Scene.Camera.Range.Near	;
//	int32_t																		& pixelsDrawn								= applicationInstance.RenderCache.PixelsDrawn	= 0;
//	int32_t																		& pixelsSkipped								= applicationInstance.RenderCache.PixelsSkipped	= 0;
//	renderCache.WireframePixelCoords.clear();
//	renderCache.TrianglesDrawn												= 0;
//	const ::llc::SCoord2<int32_t>												offscreenMetricsI							= offscreenMetrics.Cast<int32_t>();
//	const ::llc::SCoord3<float>													screenCenter								= {offscreenMetricsI.x / 2.0f, offscreenMetricsI.y / 2.0f, };
//	for(uint32_t iGrid = 0; iGrid < 1; ++iGrid) {
//		xWorld		.Scale			(applicationInstance.GridPivot.Scale, true);
//		//xRotation	.SetOrientation	((applicationInstance.GridPivot.Orientation + ::llc::SQuaternion<float>{0, (float)(iGrid / ::llc::math_2pi), 0, 0}).Normalize());
//		xRotation	.SetOrientation	(applicationInstance.GridPivot.Orientation.Normalize());
//		xWorld																	= xWorld * xRotation;
//		xWorld		.SetTranslation	(applicationInstance.GridPivot.Position, false);
//		::llc::clear
//			( renderCache.Triangle3dWorld
//			, renderCache.Triangle3dToDraw		
//			, renderCache.Triangle3dIndices		
//			);
//		const ::llc::SMatrix4<float>												xWV											= xWorld * viewMatrix;
//		//const ::llc::SCoord3<float>													& cameraFront								= applicationInstance.Scene.Camera.Vectors.Front;
//		//const ::llc::SMatrix4<float>												finalTransform								= xWorld * xViewProjection;
//		for(uint32_t iTriangle = 0, triCount = applicationInstance.Grid.Positions.size(); iTriangle < triCount; ++iTriangle) {
//			::llc::STriangle3D<float>													triangle3DWorld								= applicationInstance.Grid.Positions[iTriangle];
//			::llc::STriangle3D<float>													transformedTriangle3D						= triangle3DWorld;
//			::llc::transform(transformedTriangle3D, xWV);
//			if( transformedTriangle3D.A.z >= fFar
//			 && transformedTriangle3D.B.z >= fFar	
//			 && transformedTriangle3D.C.z >= fFar) 
//				continue;
//			if( (transformedTriangle3D.A.z <= fNear) // && transformedTriangle3D.B.z <= (fNear + .01) )  	
//			 || (transformedTriangle3D.B.z <= fNear) // && transformedTriangle3D.C.z <= (fNear + .01) )  	
//			 || (transformedTriangle3D.C.z <= fNear) // && transformedTriangle3D.A.z <= (fNear + .01) )  	
//			 ) 
//				continue;
//
//			float																		oldzA										= transformedTriangle3D.A.z;
//			float																		oldzB										= transformedTriangle3D.B.z;
//			float																		oldzC										= transformedTriangle3D.C.z;
//
//			::llc::transform(transformedTriangle3D, projection);
//			transformedTriangle3D.A.z												= oldzA;
//			transformedTriangle3D.B.z												= oldzB;
//			transformedTriangle3D.C.z												= oldzC;
//			//::llc::translate(transformedTriangle3D, screenCenter);
//			// Check against far and near planes
//			// Check against screen limits
//			if(transformedTriangle3D.A.x < 0 && transformedTriangle3D.B.x < 0 && transformedTriangle3D.C.x < 0) continue;
//			if(transformedTriangle3D.A.y < 0 && transformedTriangle3D.B.y < 0 && transformedTriangle3D.C.y < 0) continue;
//			if( transformedTriangle3D.A.x >= offscreenMetricsI.x
//			 && transformedTriangle3D.B.x >= offscreenMetricsI.x
//			 && transformedTriangle3D.C.x >= offscreenMetricsI.x
//			 )
//				continue;
//			if( transformedTriangle3D.A.y >= offscreenMetricsI.y
//			 && transformedTriangle3D.B.y >= offscreenMetricsI.y
//			 && transformedTriangle3D.C.y >= offscreenMetricsI.y
//			 )
//				continue;
//			llc_necall(renderCache.Triangle3dToDraw		.push_back(transformedTriangle3D)	, "Out of memory?");
//			::llc::transform(triangle3DWorld, xWorld);
//			llc_necall(renderCache.Triangle3dWorld		.push_back(triangle3DWorld)			, "Out of memory?");
//			llc_necall(renderCache.Triangle3dIndices	.push_back(iTriangle)		, "Out of memory?");
//		}
//		llc_necall(::llc::resize(renderCache.Triangle3dIndices.size()
//			, renderCache.TransformedNormalsTriangle
//			, renderCache.TransformedNormalsVertex		
//			, renderCache.Triangle3dColorList			
//			), "Out of memory?");
//
//		const ::llc::SCoord3<float>													& lightDir									= applicationInstance.LightDirection;
//		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) { // transform normals
//			::llc::SCoord3<float>														& transformedNormalTri						= renderCache.TransformedNormalsTriangle[iTriangle];
//			transformedNormalTri													= xWorld.TransformDirection(applicationInstance.Grid.NormalsTriangle[renderCache.Triangle3dIndices[iTriangle]]).Normalize();
//			const double																lightFactor									= fabs(transformedNormalTri.Dot(lightDir));
//			renderCache.Triangle3dColorList[iTriangle]								= ::llc::LIGHTGRAY * lightFactor;
//			const ::llc::STriangle3D<float>												& vertNormalsTriOrig						= applicationInstance.Grid.NormalsVertex[renderCache.Triangle3dIndices[iTriangle]];
//			::llc::STriangle3D<float>													& vertNormalsTri							= renderCache.TransformedNormalsVertex[iTriangle];
//			vertNormalsTri.A														= xWorld.TransformDirection(vertNormalsTriOrig.A).Normalize();
//			vertNormalsTri.B														= xWorld.TransformDirection(vertNormalsTriOrig.B).Normalize();
//			vertNormalsTri.C														= xWorld.TransformDirection(vertNormalsTriOrig.C).Normalize();
//		}
//
//		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) { // 
//			const double																cameraFactor								= renderCache.TransformedNormalsTriangle[iTriangle].Dot(applicationInstance.Scene.Camera.Vectors.Front);
//			if(cameraFactor > .65)
//				continue;
//			renderCache.TrianglePixelCoords.clear();
//			renderCache.TrianglePixelWeights.clear();
//			error_if(errored(::llc::drawTriangle(offscreenDepth.View, fFar, fNear, renderCache.Triangle3dToDraw[iTriangle], renderCache.TrianglePixelCoords, renderCache.TrianglePixelWeights)), "Not sure if these functions could ever fail");
//			//if(0 != renderCache.TrianglePixelCoords.size())
//			++renderCache.TrianglesDrawn;
//			const int32_t																iGridTri										= renderCache.Triangle3dIndices[iTriangle];
//			const ::llc::STriangle2D<float>												& uvGrid										= applicationInstance.Grid.UVs[iGridTri];
//			for(uint32_t iPixel = 0, pixCount = renderCache.TrianglePixelCoords.size(); iPixel < pixCount; ++iPixel) {
//				const ::llc::SCoord2<int32_t>												& pixelCoord								= renderCache.TrianglePixelCoords	[iPixel];
//				const ::llc::STriangleWeights<double>										& pixelWeights								= renderCache.TrianglePixelWeights	[iPixel];
//				if(0 == ::drawPixel(renderCache, offscreen.View[pixelCoord.y][pixelCoord.x], pixelWeights, uvGrid, applicationInstance.TextureGrid.View, iTriangle, lightDir.Cast<double>()))
//					++pixelsDrawn;
//				else
//					++pixelsSkipped;
//			}
//			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].A, renderCache.Triangle3dToDraw[iTriangle].B}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
//			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].B, renderCache.Triangle3dToDraw[iTriangle].C}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
//			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].C, renderCache.Triangle3dToDraw[iTriangle].A}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
//			// draw normals
//			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].A, renderCache.Triangle3dToDraw[iTriangle].A + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
//			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].B, renderCache.Triangle3dToDraw[iTriangle].B + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
//			//error_if(errored(::llc::drawLine(offscreenMetrics, ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].C, renderCache.Triangle3dToDraw[iTriangle].C + (renderCache.TransformedNormalsTriangle[iTriangle])}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
//		} 
//	}
//	static constexpr const ::llc::SColorBGRA									color										= ::llc::YELLOW;
//	for(uint32_t iPixel = 0, pixCount = renderCache.WireframePixelCoords.size(); iPixel < pixCount; ++iPixel) {
//		const ::llc::SCoord2<int32_t>												& pixelCoord								= renderCache.WireframePixelCoords[iPixel];
//		if( offscreen.View[pixelCoord.y][pixelCoord.x] != color ) {
//			offscreen.View[pixelCoord.y][pixelCoord.x]								= color;
//			++pixelsDrawn;
//		}
//		else
//			++pixelsSkipped;
//	}
//	return pixelsDrawn;
//}
//
