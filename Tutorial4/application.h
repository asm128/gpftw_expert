#include "llc_scene.h"	
#include "llc_camera.h"
#include "llc_array_static.h"

#include "llc_framework.h"
#include "llc_gui.h"
 
#ifndef APPLICATION_H_098273498237423
#define APPLICATION_H_098273498237423

template <typename _tHeight>
struct STileHeights {
						::llc::array_static<_tHeight, 4>					Heights;
};

struct SRenderCache {
						::llc::array_pod<::llc::SCoord2<int32_t>>			TrianglePixelCoords							= {};
						::llc::array_pod<::llc::STriangleWeights<double>>	TrianglePixelWeights						= {};
						::llc::array_pod<::llc::SCoord2<int32_t>>			WireframePixelCoords						= {};

						::llc::array_pod<::llc::SColorBGRA>					Triangle3dColorList							= {};
						::llc::array_pod<::llc::SCoord3<float>>				TransformedNormalsTriangle					= {};
						::llc::array_pod<::llc::STriangle3D<float>>			TransformedNormalsVertex					= {};

						::llc::array_pod<int32_t>							Triangle3dIndices							= {};
						::llc::array_pod<::llc::STriangle3D<float>>			Triangle3dToDraw							= {};
						::llc::array_pod<::llc::STriangle3D<float>>			Triangle3dWorld								= {};

						int32_t												TrianglesDrawn								= 0;
						int32_t												PixelsDrawn									= 0;
						int32_t												PixelsSkipped								= 0;
};

struct SApplicationThreadsState { 
						bool												Running										: 1;
						bool												Closed										: 1;
						bool												RequestedClose								: 1;
};

struct SApplicationThreadsCall {
						void												* Call										= 0;
						void												* Arguments									= 0;
};

struct SApplicationThreads {
						uintptr_t											Handles	[4]						;
						SApplicationThreadsState							States	[4]						;
						::llc::array_pod<SApplicationThreadsCall>			Arguments						;
};

struct SThreadArgs {
						::SApplicationThreads								* ApplicationThreads;
						int32_t												ThreadId;
};

struct SApplication {
						::llc::SFramework									Framework									;

						::llc::STexture<::llc::SColorBGRA>					TextureGrid									= {};
						::llc::STexture<::llc::SColorBGRA>					TextureFont									= {};
						::llc::STextureMonochrome<uint32_t>					TextureFontMonochrome						= {};
						::llc::SGUI											GUI											= {};

						::llc::SModelGeometry	<float>						Grid										= {};
						::llc::SModelPivot		<float>						GridPivot									= {};

						::llc::STexture<::STileHeights<float>>				TileHeights									= {};

						::llc::SScene										Scene;
						::llc::SCoord3<float>								LightDirection								= {10, 5, 0};
						// cabildo 2954
						::SRenderCache										RenderCache									= {};
						::SApplicationThreads								Threads										= {};
						::SThreadArgs										ThreadArgs[4]								= {};

																			SApplication								(::llc::SRuntimeValues& runtimeValues)			noexcept	: Framework(runtimeValues) {}
};

#endif // APPLICATION_H_098273498237423

