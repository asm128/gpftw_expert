#include "llc_scene.h"	
#include "llc_camera.h"

#include "llc_framework.h"
#include "llc_gui.h"
 
#ifndef APPLICATION_H_098273498237423
#define APPLICATION_H_098273498237423

struct SRenderCache {
						::llc::array_pod<::llc::SCoord2<int32_t>>			TrianglePixelCoords;
						::llc::array_pod<::llc::SCoord2<int32_t>>			WireframePixelCoords;

						::llc::array_pod<::llc::SColorBGRA>					Triangle3dColorList							= {};
						::llc::array_pod<::llc::SCoord3<float>>				TransformedNormals							= {};

						::llc::array_pod<::llc::STriangle3D<float>>			Triangle3dToDraw							= {};
						::llc::array_pod<::llc::STriangle2D<int32_t>>		Triangle2dToDraw							= {};
						::llc::array_pod<int16_t>							Triangle3dIndices							= {};
						::llc::array_pod<int16_t>							Triangle2dIndices							= {};
						::llc::array_pod<int16_t>							Triangle2d23dIndices						= {};

						int32_t												PixelsDrawn									= 0;
						int32_t												PixelsSkipped								= 0;
};

struct SApplication {
						::llc::SFramework									Framework									;

						::llc::STexture<::llc::SColorBGRA>					TextureFont									= {};
						::llc::STextureMonochrome<uint32_t>					TextureFontMonochrome						= {};
						::llc::SGUI											GUI											= {};

						::llc::SModelGeometry	<float>						Box											= {};
						::llc::SModelPivot		<float>						BoxPivot									= {};

						::llc::SMatrix4<float>								XViewport									= {};
						::llc::SMatrix4<float>								XViewportInverse							= {};
						::llc::SMatrix4<float>								XProjection									= {};
						::llc::SMatrix4<float>								XView										= {};
						::llc::SCoord3<float>								LightPosition								= {10, 5, 0};

						::llc::SSceneCamera									Camera										= 
							{ ::llc::SCameraPoints{{20, 2.5, 0}, {}}	
							, ::llc::SCameraRange
								{ 0.001
								, 30.0
								, .25
								}
							, ::llc::SCameraVectors	
								{ {1, 0, 0}
								, {0, 1, 0}
								, {0, 0, 1}
								}
							};
						::SRenderCache										RenderCache									= {};

																			SApplication								(::llc::SRuntimeValues& runtimeValues)			noexcept	: Framework(runtimeValues) {}
};

#endif // APPLICATION_H_098273498237423

