#include "llc_ro_rsw.h"
#include "llc_ro_gnd.h"
#include "llc_ro_rsm.h"

#include "llc_scene.h"	
#include "llc_camera.h"

#include "llc_framework.h"
#include "llc_gui.h"
 
#ifndef APPLICATION_H_098273498237423
#define APPLICATION_H_098273498237423

struct STiledTerrainCounters {
						uint32_t												nBaseTileCount						;	// Base tile count is equal to (tile map width*tile map depth)
						uint32_t												nTileColumnCount					;	// Stores the amount of tile columns that contain a single map chunk
						uint32_t												nTileRowCount						;	// Stores the amount of tile rows that contain a single map chunk
						uint32_t												nTopTileFaceCount					;	// Stores the number of tiles facing upwards that contain valid attributes and geometry

						uint32_t												nFrontTileFaceCount					;	// Stores the number of tiles facing front that contain valid attributes and geometry
						uint32_t												nRightTileFaceCount					;	// Stores the number of tiles facing right that contain valid attributes and geometry
						uint32_t												nBottomTileFaceCount				;	// Stores the number of tiles facing upwards that contain valid attributes and geometry
						uint32_t												nBackTileFaceCount					;	// Stores the number of tiles facing front that contain valid attributes and geometry

						uint32_t												nLeftTileFaceCount					;	// Stores the number of tiles facing right that contain valid attributes and geometry
						uint32_t												nTotalTileFaceCount					;	// Stores the total number of valid tiles, which should be equal to the addition of top, front and right tiles.
						uint32_t												nChunkColumnTileCount				;	// Stores the amount of tile columns that contain a single map chunk
						uint32_t												nChunkRowTileCount					;	// Stores the amount of tile rows that contain a single map chunk

						uint32_t												nChunkTotalTileCount				;	// Stores the total number of tiles contained in a single chunk
						uint32_t												nColumnChunkCount					;	// Stores the amount of column chunks
						uint32_t												nRowChunkCount						;	// Stores the amount of row chunks
						uint32_t												nTotalChunkCount					;	// Stores the total chunks contained in a map
};

struct SRenderCache {
						::llc::array_pod<::llc::SCoord2<int32_t>>				TrianglePixelCoords					= {};
						::llc::array_pod<::llc::STriangleWeights<double>>		TrianglePixelWeights				= {};
						::llc::array_pod<::llc::SCoord2<int32_t>>				WireframePixelCoords				= {};

						::llc::array_pod<::llc::STriangle3D<float>>				TransformedNormalsVertex			= {};

						::llc::array_pod<int32_t>								Triangle3dIndices					= {};
						::llc::array_pod<::llc::STriangle3D<float>>				Triangle3dToDraw					= {};
						::llc::array_pod<::llc::STriangle3D<float>>				Triangle3dWorld						= {};

						uint32_t												TrianglesDrawn						= 0;
						uint32_t												PixelsDrawn							= 0;
						uint32_t												PixelsSkipped						= 0;
};


struct SApplication {
						::llc::SFramework										Framework							;
						::llc::SGUI												GUI									= {};
						::llc::STexture<::llc::SColorBGRA>						TextureFont							= {};
						::llc::STextureMonochrome<uint32_t>						TextureFontMonochrome				= {};

						::llc::array_obj<::llc::STexture<::llc::SColorBGRA>>	TexturesGND							= {};
						::llc::SModelPivot	<float>								GridPivot							= {};
						::llc::SScene											Scene;
						::llc::SCoord3<float>									LightDirection						= {10, 5, 0};
						::llc::SGNDFileContents									GNDData								= {};
						::llc::SRSWFileContents									RSWData								= {};
						::llc::array_obj<::llc::SRSMFileContents>				RSMData								= {};
						::llc::SModelGND										GNDModel							= {};
						::SRenderCache											RenderCache							= {};

																				SApplication						(::llc::SRuntimeValues& runtimeValues)			noexcept	: Framework(runtimeValues) {}
};

#endif // APPLICATION_H_098273498237423

