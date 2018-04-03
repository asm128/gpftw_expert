#include "llc_framework.h"
#include "llc_gui.h"
#include "llc_geometry.h"

#ifndef APPLICATION_H_098273498237423
#define APPLICATION_H_098273498237423

//struct SCameraVectors {
//						::llc::SCoord3<float>							Right, Up, Front;					
//};

struct SRenderCache {
	::llc::array_pod<::llc::SColorBGRA>											Triangle3dColorList							= {};
	::llc::array_pod<::llc::SCoord3<float>>										TransformedNormals							= {};

	::llc::array_pod<::llc::STriangle3D<float>>									Triangle3dToDraw							= {};
	::llc::array_pod<::llc::STriangle2D<int32_t>>								Triangle2dToDraw							= {};
	::llc::array_pod<int16_t>													Triangle3dIndices							= {};
	::llc::array_pod<int16_t>													Triangle2dIndices							= {};
	::llc::array_pod<int16_t>													Triangle2d23dIndices						= {};
};

struct SApplication {
						::llc::SFramework								Framework									;

						::llc::STexture<::llc::SColorBGRA>				TextureFont									= {};
						::llc::STextureMonochrome<uint32_t>				TextureFontMonochrome						= {};
						::llc::SGUI										GUI											= {};

						::llc::SModelGeometry							Box											= {};
						::llc::SModelPivot								BoxPivot									= {};

						::llc::SMatrix4<float>							XViewport									= {};
						::llc::SMatrix4<float>							XViewportInverse							= {};
						::llc::SMatrix4<float>							XProjection									= {};
						::llc::SMatrix4<float>							XView										= {};

						double											CameraFar									= 30.0;
						double											CameraNear									= 0.001;
						double											CameraAngle									= .25;
						::llc::SCoord3<float>							CameraUp									= {};
						::llc::SCoord3<float>							CameraFront									= {};
						::llc::SCoord3<float>							CameraRight									= {};

						::SRenderCache									RenderCache									= {};

																		SApplication								(::llc::SRuntimeValues& runtimeValues)			noexcept	: Framework(runtimeValues) {}
};

#endif // APPLICATION_H_098273498237423

