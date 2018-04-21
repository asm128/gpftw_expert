#include "terrain.h"

				::llc::error_t								gndFileLoad										(SGNDFileContents& loaded, const ::llc::array_view<ubyte_t>	& input)			{
	uint32_t														nMagicHeader									= *(uint32_t*)input.begin();
#if defined (LLC_ANDROID) || defined(LLC_LINUX)
	ree_if(nMagicHeader != 0x4E475247UL , "Invalid GND file header.");
#elif defined(LLC_WINDOWS)
	ree_if(nMagicHeader != 'NGRG', "Invalid GND file header.");
#endif
	uint8_t															nVersionMajor									= input[4];
	uint8_t															nVersionMinor									= input[5];
	ree_if((nVersionMajor < 1) || (nVersionMajor == 1 && nVersionMinor < 5), "Invalid GND file version. Major version: %u, Minor version: %u", (int)nVersionMajor, (int)nVersionMinor);

	int32_t															byteOffset										= 6;
	uint32_t														nTextureCount									= 0;
	uint32_t														nTextureStringSize								= 0;
	int32_t 
		byteCount = (int32_t)sizeof(loaded.Metrics	);								memcpy(&loaded.Metrics		, &input[byteOffset], byteCount);					byteOffset += byteCount;
		byteCount = (int32_t)sizeof(uint32_t		);								memcpy(&nTextureCount		, &input[byteOffset], byteCount);					byteOffset += byteCount;
		byteCount = (int32_t)sizeof(uint32_t		);								memcpy(&nTextureStringSize	, &input[byteOffset], byteCount);					byteOffset += byteCount;
	loaded.TextureNames.resize(nTextureCount);
	for(uint32_t iTexture = 0; iTexture < nTextureCount; ++iTexture) 
		loaded.TextureNames[iTexture].resize(nTextureStringSize);

	byteCount													= nTextureStringSize;
	for(uint32_t iTexture = 0; iTexture < nTextureCount; ++iTexture) {
		memcpy(&loaded.TextureNames[iTexture][0], &input[byteOffset], byteCount);
		byteOffset													+= byteCount;
	}
	uint32_t														tileCountBrightness								= 0; 
	uint32_t														tileLightmapTiles								= 0;		// ?? 
	uint32_t														tileLightmapWidth								= 0;	
	uint32_t														tileLightmapDepth								= 0;
		byteCount = (int32_t)sizeof(uint32_t);										memcpy(&tileCountBrightness	, &input[byteOffset], byteCount);					byteOffset += byteCount;
		byteCount = (int32_t)sizeof(uint32_t);										memcpy(&tileLightmapTiles	, &input[byteOffset], byteCount);					byteOffset += byteCount;
		byteCount = (int32_t)sizeof(uint32_t);										memcpy(&tileLightmapWidth	, &input[byteOffset], byteCount);					byteOffset += byteCount;
		byteCount = (int32_t)sizeof(uint32_t);										memcpy(&tileLightmapDepth	, &input[byteOffset], byteCount);					byteOffset += byteCount;
	loaded.lstTileBrightnessData.resize(tileCountBrightness);	byteCount = (int32_t)(sizeof(STileBrightnessGND) * tileCountBrightness);	memcpy(loaded.lstTileBrightnessData	.begin(), &input[byteOffset], byteCount);	byteOffset += byteCount; 

	uint32_t														tileCountSkin									= *(uint32_t*)&input[byteOffset]; byteOffset													+= sizeof(uint32_t); 
	loaded.lstTileTextureData.resize(tileCountSkin);			byteCount = (int32_t)(sizeof(STileSkinGND) * tileCountSkin);				memcpy(loaded.lstTileTextureData	.begin(), &input[byteOffset], byteCount);	byteOffset += byteCount; 

	uint32_t														tileCountGeometry								= loaded.Metrics.Size.x * loaded.Metrics.Size.x;//*(uint32_t*)&input[byteOffset]; byteOffset													+= sizeof(uint32_t); 
	loaded.lstTileGeometryData.resize(tileCountGeometry); 
	if( nVersionMajor > 1 || ( nVersionMajor == 1 && nVersionMinor >= 7 ) ) {
		byteCount													= (int32_t)(sizeof(STileGeometryGND	) * tileCountGeometry);		
		memcpy(loaded.lstTileGeometryData	.begin(), &input[byteOffset], byteCount);	
		byteOffset													+= byteCount; 
	}
	else if( nVersionMajor < 1 || ( nVersionMajor == 1 && nVersionMinor <= 5 ) ) {// it seems old 1.5 format used 16 bit integers
		for(uint32_t iTile = 0; iTile < tileCountGeometry; ++iTile) {
			int16_t															top												= -1;
			int16_t															right											= -1;
			int16_t															front											= -1;
			int16_t															flags											= -1;
			::STileGeometryGND												& tileGeometry									= loaded.lstTileGeometryData[iTile];
			byteCount													= (int32_t)(sizeof(float) * 4);		
			memcpy(tileGeometry.fHeight, &input[byteOffset], byteCount);	
			byteOffset													+= byteCount; 
			byteCount													= 2;
			memcpy( &top	, &input[byteOffset], byteCount);	byteOffset += byteCount; 
			memcpy( &right	, &input[byteOffset], byteCount);	byteOffset += byteCount; 
			memcpy( &front	, &input[byteOffset], byteCount);	byteOffset += byteCount; 
			memcpy( &flags	, &input[byteOffset], byteCount);	byteOffset += byteCount; 
			tileGeometry.SkinMapping.SkinIndexTop						= top		;
			tileGeometry.SkinMapping.SkinIndexRight						= right		;
			tileGeometry.SkinMapping.SkinIndexFront						= front		;
			tileGeometry.Flags											= flags		;
		}
	}
	return byteCount;
}

			::llc::error_t								gndFileLoad											(SGNDFileContents& loaded, FILE								* input)			{ 
	loaded, input;
	return 0; 
}

			::llc::error_t								gndFileLoad											(SGNDFileContents& loaded, const ::llc::view_const_string	& input)			{ 
	FILE														* fp												= 0;
	ree_if(0 != fopen_s(&fp, input.begin(), "rb"), "Cannot open .gnd file: %s.", input.begin());

	fseek(fp, 0, SEEK_END);
	int32_t														fileSize											= (int32_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);
	::llc::array_pod<ubyte_t>									fileInMemory										= 0;
	fileInMemory.resize(fileSize);
	if(fileSize != fread(fileInMemory.begin(), sizeof(ubyte_t), fileSize, fp)) {
		fclose(fp);
		error_printf("fread() failed! file: '%s'.", input.begin());
		return -1;
	}
	fclose(fp);

	return gndFileLoad(loaded, fileInMemory);
}

