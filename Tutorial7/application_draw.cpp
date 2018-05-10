#include "application.h"
#include "llc_bitmap_target.h"
#include "llc_ro_rsw.h"

					::llc::error_t										drawPixelGND									
	( ::SRenderCache															& renderCache
	, ::llc::SColorBGRA															& targetColorCell
	, const ::llc::STriangleWeights<double>										& pixelWeights	
	, const ::llc::STriangle3D<float>											& positions
	, const ::llc::STriangle2D<float>											& uvs
	, const ::llc::grid_view<::llc::SColorBGRA>									& textureColors
	, int32_t																	iTriangle
	, const ::llc::SCoord3<double>												& lightDir
	, const ::llc::SColorFloat													& diffuseColor								
	, const ::llc::SColorFloat													& ambientColor								
	, const ::llc::array_view<::llc::SLightInfoRSW>								& lights
	) {	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SColorFloat															lightColor									= {0, 0, 0, 1}; //((::llc::RED * pixelWeights.A) + (::llc::GREEN * pixelWeights.B) + (::llc::BLUE * pixelWeights.C));
	const ::llc::STriangle3D<float>												& normals									= renderCache.TransformedNormalsVertex[iTriangle];
	::llc::STriangle3D<double>													weightedNormals								= 
		{ normals.A.Cast<double>() * pixelWeights.A
		, normals.B.Cast<double>() * pixelWeights.B
		, normals.C.Cast<double>() * pixelWeights.C
		};
	const ::llc::SCoord3<double>												interpolatedNormal							= (weightedNormals.A + weightedNormals.B + weightedNormals.C).Normalize();
	::llc::SColorFloat															directionalColor							= diffuseColor * interpolatedNormal.Dot(lightDir);
	const ::llc::SCoord2<uint32_t>												textureMetrics								= textureColors.metrics();
	::llc::SCoord2<double>														uv											= 
		{ uvs.A.x * pixelWeights.A + uvs.B.x * pixelWeights.B + uvs.C.x * pixelWeights.C
		, uvs.A.y * pixelWeights.A + uvs.B.y * pixelWeights.B + uvs.C.y * pixelWeights.C
		};
	::llc::STriangle3D<double>													weightedPositions							= 
		{ positions.A.Cast<double>() * pixelWeights.A//((::llc::RED * pixelWeights.A) + (::llc::GREEN * pixelWeights.B) + (::llc::BLUE * pixelWeights.C));
		, positions.B.Cast<double>() * pixelWeights.B
		, positions.C.Cast<double>() * pixelWeights.C
		};

	const ::llc::SCoord3<double>												interpolatedPosition						= weightedPositions.A + weightedPositions.B + weightedPositions.C;
	::llc::SColorBGRA															interpolatedBGRA;
	if( 0 == textureMetrics.x
	 ||	0 == textureMetrics.y
	 //||	0 != textureMetrics.y
	 ) {
		for(uint32_t iLight = 0; iLight < lights.size(); ++iLight) {
			const ::llc::SLightInfoRSW													& rswLight									= lights[iLight];
			::llc::SCoord3<float>														rswColor									= rswLight.Color * (1.0 - (rswLight.Position.Cast<double>() - interpolatedPosition).Length() / 10.0);
			lightColor																+= ::llc::SColorFloat(rswColor.x, rswColor.y, rswColor.z, 1.0f) / 2.0;
		}
		interpolatedBGRA														= directionalColor + lightColor + ambientColor;
	}
	else {
		const ::llc::SCoord2<int32_t>												uvcoords									= 
			{ (int32_t)((uint32_t)(uv.x * textureMetrics.x) % textureMetrics.x)
			, (int32_t)((uint32_t)(uv.y * textureMetrics.y) % textureMetrics.y)
			};
		const ::llc::SColorFloat													& srcTexel									= textureColors[uvcoords.y][uvcoords.x];
		if(srcTexel == ::llc::SColorBGRA{0xFF, 0, 0xFF, 0xFF}) 
			return 1;
		//interpolatedBGRA														= finalColor + ambientColor;
		for(uint32_t iLight = 0, lightCount = lights.size(); iLight < lightCount; ++iLight) {
			const ::llc::SLightInfoRSW													& rswLight									= lights[rand() % lights.size()];
			double																		distFactor									= 1.0 - (rswLight.Position.Cast<double>() - interpolatedPosition).Length() / 10.0;
			if(distFactor > 0) {
				::llc::SCoord3<float>														rswColor									= rswLight.Color * distFactor;
				lightColor																+= srcTexel * ::llc::SColorFloat(rswColor.x, rswColor.y, rswColor.z, 1.0f) / 2.0;
			}
		}
		interpolatedBGRA														= srcTexel * directionalColor + lightColor + srcTexel * ambientColor;
	}
	if( targetColorCell == interpolatedBGRA ) 
		return 1;

	targetColorCell															= interpolatedBGRA;
	return 0;
}

static				::llc::error_t										transformTriangles							
	( const ::llc::array_view<::llc::STriangleWeights<uint32_t>>	& vertexIndexList
	, const ::llc::array_view<::llc::SCoord3<float>>				& vertices
	, double														fFar
	, double														fNear
	, const ::llc::SMatrix4<float>									& xWorld
	, const ::llc::SMatrix4<float>									& xWV
	, const ::llc::SMatrix4<float>									& xProjection
	, const ::llc::SCoord2<int32_t>									& targetMetrics
	, ::SRenderCache												& out_transformed
	) {	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	for(uint32_t iTriangle = 0, triCount = vertexIndexList.size(); iTriangle < triCount; ++iTriangle) {
		const ::llc::STriangleWeights<uint32_t>										& vertexIndices								= vertexIndexList[iTriangle];
		::llc::STriangle3D<float>													triangle3DWorld								= 
			{	vertices[vertexIndices.A]
			,	vertices[vertexIndices.B]
			,	vertices[vertexIndices.C]
			};
		::llc::STriangle3D<float>													transformedTriangle3D						= triangle3DWorld;
		::llc::transform(transformedTriangle3D, xWV);
		// Check against far and near planes
		if( transformedTriangle3D.A.z >= fFar
		 && transformedTriangle3D.B.z >= fFar	
		 && transformedTriangle3D.C.z >= fFar) 
			continue;
		if( (transformedTriangle3D.A.z <= fNear)
		 || (transformedTriangle3D.B.z <= fNear)
		 || (transformedTriangle3D.C.z <= fNear)
		 ) 
		 continue;

		float																		oldzA										= transformedTriangle3D.A.z;
		float																		oldzB										= transformedTriangle3D.B.z;
		float																		oldzC										= transformedTriangle3D.C.z;

		::llc::transform(transformedTriangle3D, xProjection);
		transformedTriangle3D.A.z												= oldzA;
		transformedTriangle3D.B.z												= oldzB;
		transformedTriangle3D.C.z												= oldzC;
		// Check against screen limits
		if(transformedTriangle3D.A.x < 0 && transformedTriangle3D.B.x < 0 && transformedTriangle3D.C.x < 0) continue;
		if(transformedTriangle3D.A.y < 0 && transformedTriangle3D.B.y < 0 && transformedTriangle3D.C.y < 0) continue;
		if( transformedTriangle3D.A.x >= targetMetrics.x
		 && transformedTriangle3D.B.x >= targetMetrics.x
		 && transformedTriangle3D.C.x >= targetMetrics.x
		 )
			continue;
		if( transformedTriangle3D.A.y >= targetMetrics.y
		 && transformedTriangle3D.B.y >= targetMetrics.y
		 && transformedTriangle3D.C.y >= targetMetrics.y
		 )
			continue;
		llc_necall(out_transformed.Triangle3dToDraw		.push_back(transformedTriangle3D)	, "Out of memory?");
		::llc::transform(triangle3DWorld, xWorld);
		llc_necall(out_transformed.Triangle3dWorld		.push_back(triangle3DWorld)			, "Out of memory?");
		llc_necall(out_transformed.Triangle3dIndices	.push_back(iTriangle)				, "Out of memory?");
	}
	return 0;
}

static				::llc::error_t										transformNormals
	( const ::llc::array_view<::llc::STriangleWeights<uint32_t>>	& vertexIndexList
	, const ::llc::array_view<::llc::SCoord3<float>>				& normals
	, const ::llc::SMatrix4<float>									& xWorld
	, ::SRenderCache												& renderCache
	) {	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) { // transform normals
			const ::llc::STriangleWeights<uint32_t>										& vertexIndices								= vertexIndexList[renderCache.Triangle3dIndices[iTriangle]];
			::llc::STriangle3D<float>													triangle3DWorldNormals						= 
				{ normals[vertexIndices.A]
				, normals[vertexIndices.B]
				, normals[vertexIndices.C]
				};
			::llc::STriangle3D<float>													& vertNormalsTri							= renderCache.TransformedNormalsVertex[iTriangle];
			vertNormalsTri.A														= xWorld.TransformDirection(normals[vertexIndices.A]).Normalize(); // gndNode.Normals[vertexIndices.A]; //
			vertNormalsTri.B														= xWorld.TransformDirection(normals[vertexIndices.B]).Normalize(); // gndNode.Normals[vertexIndices.B]; //
			vertNormalsTri.C														= xWorld.TransformDirection(normals[vertexIndices.C]).Normalize(); // gndNode.Normals[vertexIndices.C]; //
			//vertNormalsTri.A.y *= -1;
			//vertNormalsTri.B.y *= -1;
			//vertNormalsTri.C.y *= -1;
		}
	
	return 0;
}

static				::llc::error_t										drawTriangles
	( const ::llc::array_view	<::llc::STriangleWeights<uint32_t>>	& vertexIndexList
	, const ::llc::array_view<::llc::SCoord3<float>>				& vertices
	, const ::llc::array_view	<::llc::SCoord2<float>>				& uvs
	, const ::llc::grid_view	<::llc::SColorBGRA>					& textureView
	, double														fFar
	, double														fNear
	, const ::llc::SCoord3<float>									& lightDir
	, ::SRenderCache												& renderCache
	, ::llc::grid_view	<uint32_t>									& targetDepthView
	, ::llc::grid_view	<::llc::SColorBGRA>							& targetView
	, const ::llc::SColorFloat										& diffuseColor								
	, const ::llc::SColorFloat										& ambientColor								
	, const ::llc::array_view<::llc::SLightInfoRSW>					& lights
	, uint32_t														* pixelsDrawn
	, uint32_t														* pixelsSkipped
	) {	// --- 
		//const ::llc::SCoord3<float>													& lightDir									= applicationInstance.LightDirection;
		for(uint32_t iTriangle = 0, triCount = renderCache.Triangle3dIndices.size(); iTriangle < triCount; ++iTriangle) { // 
			renderCache.TrianglePixelCoords.clear();
			renderCache.TrianglePixelWeights.clear();
			const ::llc::STriangle3D<float>												& tri3DToDraw								= renderCache.Triangle3dToDraw[iTriangle];
			error_if(errored(::llc::drawTriangle(targetDepthView, fFar, fNear, tri3DToDraw, renderCache.TrianglePixelCoords, renderCache.TrianglePixelWeights)), "Not sure if these functions could ever fail");
			++renderCache.TrianglesDrawn;
			const ::llc::STriangleWeights<uint32_t>										& vertexIndices								= vertexIndexList[renderCache.Triangle3dIndices[iTriangle]];
			const ::llc::STriangle3D<float>												triangle3DPositions							= 
				{ vertices[vertexIndices.A]
				, vertices[vertexIndices.B]
				, vertices[vertexIndices.C]
				};
			const ::llc::STriangle2D<float>												triangle3DUVs								= 
				{ uvs[vertexIndices.A]
				, uvs[vertexIndices.B]
				, uvs[vertexIndices.C]
				};
			//if(vertexIndices.C == -1) {
				for(uint32_t iPixel = 0, pixCount = renderCache.TrianglePixelCoords.size(); iPixel < pixCount; ++iPixel) {
					const ::llc::SCoord2<int32_t>												& pixelCoord								= renderCache.TrianglePixelCoords	[iPixel];
					const ::llc::STriangleWeights<double>										& pixelWeights								= renderCache.TrianglePixelWeights	[iPixel];
					if(0 == ::drawPixelGND(renderCache, targetView[pixelCoord.y][pixelCoord.x], pixelWeights, triangle3DPositions, triangle3DUVs, textureView, iTriangle, lightDir.Cast<double>(), diffuseColor, ambientColor, lights))
						++*pixelsDrawn;
					else
						++*pixelsSkipped;
				}
			//}
			//error_if(errored(::llc::drawLine(targetView.metrics(), ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].A, renderCache.Triangle3dToDraw[iTriangle].B}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(targetView.metrics(), ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].B, renderCache.Triangle3dToDraw[iTriangle].C}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
			//error_if(errored(::llc::drawLine(targetView.metrics(), ::llc::SLine3D<float>{renderCache.Triangle3dToDraw[iTriangle].C, renderCache.Triangle3dToDraw[iTriangle].A}, renderCache.WireframePixelCoords)), "Not sure if these functions could ever fail");
		} 
	return 0;
}

					::llc::error_t										drawGND										(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	::llc::STexture<uint32_t>													& offscreenDepth							= framework.OffscreenDepthStencil;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	::memset(offscreenDepth	.Texels.begin(), -1, sizeof(uint32_t)								* offscreenDepth	.Texels.size());	// Clear target.
	::memset(offscreen		.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel)	* offscreen			.Texels.size());	// Clear target.
	for(uint32_t y = 0, yMax = offscreenMetrics.y; y < yMax; ++y) {
		const uint8_t																colorHeight									= (uint8_t)(y / 10);
		for(uint32_t x = 0, xMax = offscreenMetrics.x; x < xMax; ++x)
			offscreen.View[y][x]													= {colorHeight, 0, 0, 0xFF};
	}

	::SRenderCache																& renderCache								= applicationInstance.RenderCache;

	const ::llc::SMatrix4<float>												& projection								= applicationInstance.Scene.Transforms.FinalProjection	;
	const ::llc::SMatrix4<float>												& viewMatrix								= applicationInstance.Scene.Transforms.View				;

	//::llc::SMatrix4<float>														xViewProjection								= viewMatrix * projection;
	//::llc::SMatrix4<float>														xRotation									= {};
	//xRotation.Identity();
	::llc::SMatrix4<float>														xWorld										= {};
	xWorld.Identity();
	const double																& fFar										= applicationInstance.Scene.Camera.Range.Far	;
	const double																& fNear										= applicationInstance.Scene.Camera.Range.Near	;
	uint32_t																	& pixelsDrawn								= applicationInstance.RenderCache.PixelsDrawn	= 0;
	uint32_t																	& pixelsSkipped								= applicationInstance.RenderCache.PixelsSkipped	= 0;
	renderCache.WireframePixelCoords.clear();
	renderCache.TrianglesDrawn												= 0;
	const ::llc::SCoord2<int32_t>												offscreenMetricsI							= offscreenMetrics.Cast<int32_t>();
	const ::llc::SCoord3<float>													screenCenter								= {offscreenMetricsI.x / 2.0f, offscreenMetricsI.y / 2.0f, };
	const ::llc::SColorFloat													ambient										= 
		{ applicationInstance.RSWData.Light.Ambient.x
		, applicationInstance.RSWData.Light.Ambient.y
		, applicationInstance.RSWData.Light.Ambient.z
		, 1
		};
	const ::llc::SColorFloat													diffuse										= 
		{ applicationInstance.RSWData.Light.Diffuse.x
		, applicationInstance.RSWData.Light.Diffuse.y
		, applicationInstance.RSWData.Light.Diffuse.z
		, 1
		};
	::llc::STimer																timerMark									= {};
	for(uint32_t iGNDTexture = 0; iGNDTexture < applicationInstance.GNDData.TextureNames.size(); ++iGNDTexture) {
		for(uint32_t iFacingDirection = 0; iFacingDirection < 6; ++iFacingDirection) {
			const ::llc::grid_view<::llc::SColorBGRA>									& gndNodeTexture							= applicationInstance.TexturesGND	[iGNDTexture].View;
			const ::llc::SModelNodeGND													& gndNode									= applicationInstance.GNDModel.Nodes[applicationInstance.GNDData.TextureNames.size() * iFacingDirection + iGNDTexture];
			xWorld		.Scale			(applicationInstance.GridPivot.Scale, true);
			//xRotation	.SetOrientation	(applicationInstance.GridPivot.Orientation.Normalize());
			//xWorld																	= xWorld * xRotation;
			xWorld		.SetTranslation	(applicationInstance.GridPivot.Position, false);
			::llc::clear
				( renderCache.Triangle3dWorld
				, renderCache.Triangle3dToDraw		
				, renderCache.Triangle3dIndices		
				);
			const ::llc::SMatrix4<float>												xWV											= xWorld * viewMatrix;
			transformTriangles(gndNode.VertexIndices, gndNode.Vertices, fFar, fNear, xWorld, xWV, projection, offscreenMetricsI, renderCache);
			//timerMark.Frame(); info_printf("First iteration: %f.", timerMark.LastTimeSeconds);
			llc_necall(renderCache.TransformedNormalsVertex.resize(renderCache.Triangle3dIndices.size()), "Out of memory?");
			transformNormals(gndNode.VertexIndices, gndNode.Normals, xWorld, renderCache);
			//timerMark.Frame(); info_printf("Second iteration: %f.", timerMark.LastTimeSeconds);
			const ::llc::SCoord3<float>													& lightDir									= applicationInstance.LightDirection;
			drawTriangles(gndNode.VertexIndices, gndNode.Vertices, gndNode.UVs, gndNodeTexture, fFar, fNear, lightDir, renderCache, offscreenDepth.View, offscreen.View, diffuse, ambient, applicationInstance.RSWData.RSWLights, &pixelsDrawn, &pixelsSkipped);
			//timerMark.Frame(); info_printf("Third iteration: %f.", timerMark.LastTimeSeconds);
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
	return (::llc::error_t)pixelsDrawn;
}
