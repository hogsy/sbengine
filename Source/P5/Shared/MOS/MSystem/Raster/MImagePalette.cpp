#include "PCH.h"

#include "MImage.h"
#include "MImageIO.h"

// -------------------------------------------------------------------
//  CImagePalette
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CImagePalette);


CImagePalette::CImagePalette(int _Size)
{
	MAUTOSTRIP(CImagePalette_ctor, MAUTOSTRIP_VOID);

	mpPal = NULL;
#ifndef PLATFORM_CONSOLE
	mpQuant = NULL;
	mQuantValid = FALSE;

	mpP8_P8_Shade = NULL;
	mpP8_565_Shade = NULL;
	mpP8_32_Shade = NULL;
	mpP8_P8_Mix5050 = NULL;
#endif
	mnColors = _Size;

	mpPal = DNew(CPixel32) CPixel32[_Size];
	if (mpPal == NULL) MemError("-");
	for (int i = 0; i < 256; i++) mpPal[i] = CPixel32(i, i, i);
};

CImagePalette::~CImagePalette()
{
	MAUTOSTRIP(CImagePalette_dtor, MAUTOSTRIP_VOID);

#ifndef PLATFORM_CONSOLE
	if (mpQuant != NULL)
	{
		delete mpQuant;
		mpQuant = NULL;
	};
#endif

	if (mpPal != NULL) 
	{
		delete[] mpPal;
		mpPal = NULL;
	};

#ifndef PLATFORM_CONSOLE
	if (mpP8_P8_Shade) delete[] mpP8_P8_Shade;
	if (mpP8_565_Shade) delete[] mpP8_565_Shade;
	if (mpP8_32_Shade) delete[] mpP8_32_Shade;
	if (mpP8_P8_Mix5050) delete[] mpP8_P8_Mix5050;
#endif

	mnColors = 0;
};

spCImagePalette CImagePalette::Duplicate()
{
	MAUTOSTRIP(CImagePalette_Duplicate, NULL);

	spCImagePalette spPal = MNew(CImagePalette);
	if (spPal == NULL) MemError("Duplicate");
	
	spPal->SetPalette(this);
	return spPal;
}


void CImagePalette::SetPalette(CImagePalette* _pPal)
{
	MAUTOSTRIP(CImagePalette_SetPalette, MAUTOSTRIP_VOID);

#ifndef PLATFORM_CONSOLE
	mQuantValid = FALSE;
#endif
	SetPalette(_pPal->GetPalettePtr(), 0, Min(_pPal->mnColors, mnColors));
};

void CImagePalette::SetPalette(CPixel32* Pal, int Start, int nColors)
{
	MAUTOSTRIP(CImagePalette_SetPalette, MAUTOSTRIP_VOID);

#ifndef PLATFORM_CONSOLE
	mQuantValid = FALSE;
#endif

	{ for (int i = Start; i < Start + nColors; i++) mpPal[i] = Pal[i]; };
/*	PALETTEENTRY CurPal[256];
	{
		for (int i = Start; i < Start + nColors; i++) 
		{
			CurPal[i] = pPal[i]; 
			CurPal[i].peFlags = PC_NOCOLLAPSE | D3DPAL_READONLY;
		};
	};

	if (spDDP == NULL)
	{
		spDDP = CDirectDraw::CreateDirectDrawPalette(gpCDD->spDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256, (PALETTEENTRY*) &CurPal);
	}
	else
	{
		spDDP = CDirectDraw::CreateDirectDrawPalette(gpCDD->spDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256, (PALETTEENTRY*) &CurPal);
	};*/
};

void CImagePalette::GetPalette(CPixel32* DestPal, int Start, int nColors)
{
	MAUTOSTRIP(CImagePalette_GetPalette, MAUTOSTRIP_VOID);

	int end = Min(Start + nColors, mnColors);
	for (int i = Start; i < end; i++) DestPal[i] = mpPal[i];
};

void CImagePalette::Read(const CStr& filename)
{
	MAUTOSTRIP(CImagePalette_Read, MAUTOSTRIP_VOID);

	TPtr<CImageIO> spIO = CImageIO::GetImageIO(filename);
	spIO->ReadPalette(filename, this);
#ifndef PLATFORM_CONSOLE
	mQuantValid = FALSE;
#endif
};

#ifndef PLATFORM_CONSOLE

int CImagePalette::GetIndex(CPixel32 _Color)
{
	MAUTOSTRIP(CImagePalette_GetIndex, 0);

	if ((!mQuantValid) || (mpQuant == NULL))
	{
		if (mpQuant == NULL)
		{
			mpQuant = MNew(CDefaultQuantize);
			if (mpQuant == NULL) MemError("GetIndex");
		};
		mpQuant->BuildFromPalette(this);
		mQuantValid = TRUE;
	};

	return mpQuant->GetIndex(_Color);
};

void CImagePalette::GetIndices(CPixel32* _pSrc, uint8* _pDest, int _nPixels)
{
	MAUTOSTRIP(CImagePalette_GetIndices, MAUTOSTRIP_VOID);

	if ((!mQuantValid) || (mpQuant == NULL))
	{
		if (mpQuant == NULL)
		{
			mpQuant = MNew(CDefaultQuantize);
			if (mpQuant == NULL) MemError("GetIndex");
		};
		mpQuant->BuildFromPalette(this);
		mQuantValid = TRUE;
	};

	mpQuant->GetIndices(_pSrc, _pDest, _nPixels);
}

CDefaultQuantize* CImagePalette::GetQuantizer()
{
	MAUTOSTRIP(CImagePalette_GetQuantizer, NULL);

	if ((!mQuantValid) || (mpQuant == NULL))
	{
		if (mpQuant == NULL)
		{
			mpQuant = MNew(CDefaultQuantize);
			if (mpQuant == NULL) MemError("GetIndex");
		};
		mpQuant->BuildFromPalette(this);
		mQuantValid = TRUE;
	};

	return mpQuant;
}
#endif

#ifndef PLATFORM_CONSOLE
void* CImagePalette::GetAcceleratorTable(int _SrcFmt, int _DestFmt, int _TableFmt, CImagePalette* _pDestPal)
{
	MAUTOSTRIP(CImagePalette_GetAcceleratorTable, NULL);

	switch(_SrcFmt)
	{
	case IMAGE_FORMAT_CLUT8 :
		{
			switch(_DestFmt)
			{
			case IMAGE_FORMAT_CLUT8 :
				{
					switch(_TableFmt)
					{
					case PAL_ACCEL_SHADE :
						{
							if (mpP8_P8_Shade != NULL) return mpP8_P8_Shade;
							mpP8_P8_Shade = DNew(uint8) uint8[256*PAL_ACCEL_NSTEPS];
							if (_pDestPal == NULL) Error("GetAcceleratorTable", "NULL destination palette.");

							for(int sh = 0; sh < PAL_ACCEL_NSTEPS; sh++)
							{
			/*					fp4 s = (fp4) sh / (fp4) PAL_ACCEL_NSTEPS;
								for(int x = 0; x < 256; x++)
									mpP8_P8_Shade[sh*256 + x] = 
										_pDestPal->GetIndex(mpPal[x] * s);*/

								int h = PAL_ACCEL_NSTEPS >> 1;
								if (sh <= h)
								{
									fp4 s = (fp4)sh / (fp4)h;
									for (int x = 0; x < mnColors; x++)
										mpP8_P8_Shade[sh*256 + x] = 
											_pDestPal->GetIndex(mpPal[x] * s);
								}
								else
								{
									int add = 255.0f*fp4(sh - h) / (fp4)h / 2.0f;
									for (int x = 0; x < mnColors; x++)
										mpP8_P8_Shade[sh*256 + x] = 
											_pDestPal->GetIndex(CPixel32(mpPal[x].GetR()+add, mpPal[x].GetG()+add, mpPal[x].GetB()+add));
								};
							};

							return mpP8_P8_Shade;
							break;
						};

					case PAL_ACCEL_MIX50 :
						{
							if (mpP8_P8_Mix5050 != NULL) return mpP8_P8_Mix5050;
							mpP8_P8_Mix5050 = DNew(uint8) uint8[256*256];
							if (_pDestPal == NULL) Error("GetAcceleratorTable", "NULL destination palette.");

							for(int i = 0; i < 256; i++)
								for (int j = 0; j < 256; j++)
								{
									mpP8_P8_Mix5050[i*256 + j] = 
										_pDestPal->GetIndex((mpPal[i]*0.5f) + (mpPal[j]*0.5f));
								}

							return mpP8_P8_Mix5050;
							break;
						};
					//------------------------------
					default :
						Error("GetAcceleratorTable", "Unsupported table type.");
					};
					break;
				};
			//--------------------------------------
			case IMAGE_FORMAT_B5G6R5 :
				{
					switch(_TableFmt)
					{
					case PAL_ACCEL_SHADE :
						{
							if (mpP8_565_Shade != NULL) return mpP8_565_Shade;
							mpP8_565_Shade = DNew(uint16) uint16[256*PAL_ACCEL_NSTEPS];

							for(int sh = 0; sh < PAL_ACCEL_NSTEPS; sh++)
							{
								int h = PAL_ACCEL_NSTEPS >> 1;
								if (sh <= h)
								{
									fp4 s = (fp4)sh / (fp4)h;
									for (int x = 0; x < mnColors; x++)
										mpP8_565_Shade[sh*256 + x] = (mpPal[x] * s).operator CPixel565();
								}
								else
								{
									int add = 255.0f*fp4(sh - h) / (fp4)h / 2.0f;
									for (int x = 0; x < mnColors; x++)
										mpP8_565_Shade[sh*256 + x] = 
											CPixel32(mpPal[x].GetR()+add, mpPal[x].GetG()+add, mpPal[x].GetB()+add).operator CPixel565();
								};
							};

							return mpP8_565_Shade;
							break;
						};
					//------------------------------
					default :
						Error("GetAcceleratorTable", "Unsupported table type.");
					};
					break;
				};
			//--------------------------------------
			default :
				Error("GetAcceleratorTable", "Unsupported destination format.");
			};
			break;
		};
	//----------------------------------------------
	default :
		Error("GetAcceleratorTable", "Unsupported source format.");
	};

	Error("GetAcceleratorTable", "Internal error.");
	return NULL;
};

#endif
