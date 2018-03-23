#include "llc_framework.h"
#include "llc_gui.h"
#include "llc_geometry.h"

#ifndef APPLICATION_H_098273498237423
#define APPLICATION_H_098273498237423

//struct SCameraVectors {
//						::llc::SCoord3<float>							Right, Up, Front;					
//};

struct SApplication {
						::llc::SFramework								Framework									;

						::llc::STexture<::llc::SColorBGRA>				TextureFont									= {};
						::llc::STextureMonochrome<uint32_t>				TextureFontMonochrome						= {};
						::llc::SGUI										GUI											= {};

						::llc::SModelGeometry							Box											= {};
						::llc::SModelPivot								BoxTransform								= {};

						double											CameraAngle									= .75;//.25;

																		SApplication								(::llc::SRuntimeValues& runtimeValues)			noexcept	: Framework(runtimeValues) {}
};

#endif // APPLICATION_H_098273498237423
