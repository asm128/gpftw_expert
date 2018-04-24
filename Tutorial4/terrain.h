#include "llc_coord.h"
#include "llc_color.h"
#include "llc_array.h"
#include <string>

#ifndef TERRAIN_H_289374982374
#define TERRAIN_H_289374982374

#pragma pack( push, 1 )
	// U and V TexCoords follow the same order from GAT format heights
	struct STileSkinGND {
				float										u[4]							= { 0, 1, 0, 1 };				//< west->east, north->south ordering; 0=left 1=right
				float										v[4]							= { 0, 0, 1, 1 };				//< west->east, north->south ordering; 0=up 1=down
				int16_t										nTextureIndex					= -1			;		//< -1 for none
				int16_t										nLightmapIndex					= -1			;		//< -1 for none?
				uint32_t									dwColor							= 0xFFFFFFFFU	;			// a default color to set to the tile for other kind of representations
	};

	struct STileFaceSkinMapping {
				int32_t										SkinIndexTop					= -1;	// <= -1 for none
				int32_t										SkinIndexRight					= -1;	// <= -1 for none
				int32_t										SkinIndexFront					= -1;	// <= -1 for none
	};

	struct STileGeometryGND {
				float										fHeight[4]						= {0, 0, 0, 0};				// west->east, north->south ordering
				::STileFaceSkinMapping						SkinMapping;	// <= -1 for none
				//int16_t										Flags							= 0;					// GND v <= 1.5 // maybe a color key? a terrain property? We're going to use it to tell if the triangle is inverted.
	};

	// 0 - No walkable - No Snipable
	// 1 - Walkable
	// 2 - Snipable
	// 3 - Walkable - Snipable
	struct STileNavigabilityGND {
				float										fHeight[4]						= {}; // west->east, south->north ordering
				uint8_t										Type							= 0;
				uint8_t										FlagsTop						= 0;
				uint8_t										FlagsRight						= 0;
				uint8_t										FlagsFront						= 0;
	};

	struct STileBrightnessGND 	{
				uint8_t										Brightness	[8][8]				= {};
				::llc::SColorRGB							ColorRGB	[8][8]				= {};
	};

	struct STiledTerrainMetricsGND {
				::llc::SCoord2<uint32_t>					Size							;	// Tile count 
				float										TileScale						;	// The size to expand the tiles
	};

	struct STerrainWaterGND {
				float										fSeaLevel						;	//
				int32_t										nWaterType						;	//
				float										fWaveHeight						;	//
				float										fWaveSpeed						;	//
				float										fWavePitch						;	//
				int32_t										nAnimationSpeed					;	//
	};

	struct SGNDFileContents {
				STiledTerrainMetricsGND						Metrics;
				::llc::array_obj<::std::string>				TextureNames;
				uint32_t									LightmapTiles;		// ?? 
				::llc::SCoord2<uint32_t>					LightmapSize;	
				::llc::array_pod<STileBrightnessGND	>		lstTileBrightnessData;
				::llc::array_pod<STileSkinGND		>		lstTileTextureData;
				::llc::array_pod<STileGeometryGND	>		lstTileGeometryData;
	};

	//709655609
			::llc::error_t								gndFileLoad						(SGNDFileContents& loaded, const ::llc::array_view<ubyte_t>	& input);
			::llc::error_t								gndFileLoad						(SGNDFileContents& loaded, FILE								* input);
			::llc::error_t								gndFileLoad						(SGNDFileContents& loaded, const ::llc::view_const_string	& input);

			struct SModelNodeGND {
				::llc::array_pod<::llc::SCoord3<float>>				Normals			;
				::llc::array_pod<::llc::SCoord3<float>>				Vertices		;
				::llc::array_pod<::llc::SCoord2<float>>				UVs				;
				::llc::array_pod<::llc::STriangleWeights<uint32_t>>	VertexIndices	; 
				::llc::array_pod<int32_t>							SkinIndices		; // one per triangle (VertexIndices.size() / 3)
			};

			struct SModelGND {
				::llc::array_obj<SModelNodeGND>						Nodes;

//																	SModelGND			()							= default;
//																	SModelGND			(const SModelGND& other)	= default;
			};

			enum TILE_FACE_FACING 
				{ TILE_FACE_FACING_TOP
				, TILE_FACE_FACING_FRONT
				, TILE_FACE_FACING_RIGHT
				, TILE_FACE_FACING_BOTTOM
				, TILE_FACE_FACING_BACK
				, TILE_FACE_FACING_LEFT
				};

			::llc::error_t								gndGenerateFaceGeometry			(const SGNDFileContents& loaded, TILE_FACE_FACING facing_direction, SModelNodeGND& generated);
#pragma pack(pop)

#endif // TERRAIN_H_289374982374