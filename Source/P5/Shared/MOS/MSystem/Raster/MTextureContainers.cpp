
#include "PCH.h"
#include "../Misc/MRTC_Task.h"
#include "MRender.h"
#include "MTextureContainers.h"
#include "../Misc/MRegistry.h"
#include "../MSystem.h"

#define CTC_CONVERT_SELECTNORMALMAPFORMAT	-1

#define CTC_CONVERT_DEFAULT		0
#define CTC_CONVERT_AFROMRGB	1
#define CTC_CONVERT_BUMP		2
#define CTC_CONVERT_BUMPENV		3
#define CTC_CONVERT_ALPHA		4
#define CTC_CONVERT_NONE		5
#define	CTC_CONVERT_GB2IA		6
#define	CTC_CONVERT_ANISOTROPICDIRECTION	7
#define CTC_CONVERT_CUBEFROMCYLINDER		8
#define	CTC_CONVERT_GB2GA		9
#define CTC_CONVERT_CUBEFROMSPHERE			10

#ifdef CopyFile
#undef CopyFile
#endif

const char* ms_ImageTransform[] =
{
	"flipv",
	"fliph",
	"rotatecw",
	"rotateccw",
	0
};

//Used for Cylinder-box-transformation
class CTEX_CubeFromCylinderSettings
{
public:
	bool	m_bIsEnvMap;
	uint8	m_iSide;
	fp32		m_ViewHeight;
	uint16	m_Width,m_Height;
	TArray<CPixel32> m_lLow[4],m_lHigh[4];
};

// -------------------------------------------------------------------
//  CTexture
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CTexture, CReferenceCount);

IMPLEMENT_OPERATOR_NEW(CTexture);


int CTexture::GetMipMapLevels(int _w, int _h, int _MinSize)
{
	MAUTOSTRIP(CTexture_GetMipMapLevels, 0);

//	if (!IsPow2(_w)) Error_static("CTextureContext::GetMipMapLevels", "Not 2^x width.");
//	if (!IsPow2(_h)) Error_static("CTextureContext::GetMipMapLevels", "Not 2^x height.");
	int l2w = Log2(_w / _MinSize);
	int l2h = Log2(_h / _MinSize);
	return Max(l2w, l2h) + 1;
};

CTexture::CTexture()
{
	MAUTOSTRIP(CTexture_ctor, MAUTOSTRIP_VOID);

	m_TextureID = -1;
	m_iPalette = -1;
	m_PaletteFilePos = 0;
	m_nMipmaps = 0;
	m_IsLoaded = 0;
#ifdef USE_HASHED_TEXTURENAME
	m_NameID = 0;
#else
	m_Name[0] = 0;
#endif	

	for (int t=0; t<10; t++)
	{
		m_lspMaps[t] = NULL;
		m_lMapFilePos[t] = 0;
	};
}

spCTexture CTexture::Duplicate()
{
	MAUTOSTRIP(CTexture_Duplicate, NULL);

	spCTexture spTxt = MNew(CTexture);
	if (spTxt == NULL) MemError("Duplicate");

	for(int iMaps = 0; iMaps < m_nMipmaps; iMaps++)
	{
		if (iMaps == 0)
		{
			m_LargestMap.Duplicate(&spTxt->m_LargestMap);
//			spTxt->m_LargestMap.Create(m_LargestMap);
//			spTxt->m_LargestMap.Blt(spTxt->m_LargestMap.GetClipRect(), m_LargestMap, 0, CPnt(0, 0));
		} else {
			if (m_lspMaps[iMaps] != NULL)
				spTxt->m_lspMaps[iMaps] = m_lspMaps[iMaps]->Duplicate();
		}
	}

	// Set same palette on all mipmaps.
	if (IsPalettized())
	{
		CImagePalette* pPal = spTxt->m_LargestMap.GetPalette();
		for(int iMaps = 1; iMaps < m_nMipmaps; iMaps++)
			spTxt->m_lspMaps[iMaps]->SetPalette(pPal);
	}

	spTxt->m_nMipmaps = m_nMipmaps;
#ifdef USE_HASHED_TEXTURENAME
	spTxt->m_NameID = m_NameID;
#else
	if (CStr(m_Name).Len() > 31) Error("Duplicate", "Texturename is too long. How the fuck could this happen?  / rune");
	strcpy(spTxt->m_Name, m_Name);
#endif	
	for (int n=0; n<spTxt->m_nMipmaps; n++)
		spTxt->m_IsLoaded |= (1 << n);
//	spTxt->m_bIsLoaded = true;

	spTxt->m_Properties = m_Properties;

	return spTxt;

	// Notes:	The new texture will have an independent palette and thus iPalette = -1
	//			The new texture has no TextureID.
}



void CTexture::SetPalette(spCImagePalette _spPal, int _iPal)
{
	MAUTOSTRIP(CTexture_SetPalette, MAUTOSTRIP_VOID);

	if (!IsPalettized()) return;

	m_LargestMap.SetPalette(_spPal);
	if (m_nMipmaps > 0)
		for(int iMaps = 1; iMaps < m_nMipmaps; iMaps++) m_lspMaps[iMaps]->SetPalette(_spPal);

	m_iPalette = _iPal;
}

bool CTexture::IsPalettized() const
{
	MAUTOSTRIP(CTexture_IsPalettized, false);

	return (m_LargestMap.GetFormat() & IMAGE_FORMAT_PALETTE) != 0 ||
			(m_LargestMap.GetMemModel() & IMAGE_MEM_COMPRESSTYPE_VQ) &&
			m_LargestMap.GetPalette() != 0;
}

#ifndef PLATFORM_CONSOLE
class CFilterWeight
{
public:
	int m_dX;
	int m_dY;
	fp32 m_Weight;
};

const fp32 Scale = 1.0f / 10.0f;

static CFilterWeight Sharpen[9] =
{
	-1, -1, -1.0f * Scale,	0,  -1, -2.0f * Scale,	1,  -1, -1.0f * Scale,
	-1,  0, -2.0f * Scale,	0,   0, 22.0f * Scale,	1,   0, -2.0f * Scale,
	-1,  1, -1.0f * Scale,	0,   1, -2.0f * Scale,	1,   1, -1.0f * Scale,
};

const fp32 Scale2 = 1.0f / 20.0f;

static CFilterWeight Sharpen2[9] =
{
	-1, -1, -1.0f * Scale2,	0,  -1, -2.0f * Scale2,	1,  -1, -1.0f * Scale2,
	-1,  0, -2.0f * Scale2,	0,   0, 32.0f * Scale2,	1,   0, -2.0f * Scale2,
	-1,  1, -1.0f * Scale2,	0,   1, -2.0f * Scale2,	1,   1, -1.0f * Scale2,
};

// Move to CImage, name Filter()
static spCImage Filter(CImage* _pImg, CFilterWeight* _pWeights, int _nWeights, int _ClampFlags)
{
	MAUTOSTRIP(Filter, NULL);

	if (_pImg->GetFormat() != IMAGE_FORMAT_BGRA8 &&
		_pImg->GetFormat() != IMAGE_FORMAT_RGB32_F)
		Error_static("Sharpen", "Unsupported format.");

	int w = _pImg->GetWidth();
	int h = _pImg->GetHeight();

	spCImage spDst = MNew(CImage);
	spDst->Create(w, h, _pImg->GetFormat(), IMAGE_MEM_IMAGE);

	CClipRect cr = spDst->GetClipRect();

	int Fmt = _pImg->GetFormat();

	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			fp32 r,g,b,a;
			r = g = b = a = 0;
			if(((_ClampFlags & CTC_TEXTUREFLAGS_CLAMP_V) && ((y == 0) || (y == (h - 1)))) || ((_ClampFlags & CTC_TEXTUREFLAGS_CLAMP_U) && ((x == 0) || (x == (w - 1)))))
			{
				if (Fmt == IMAGE_FORMAT_RGB32_F)
				{
					CVec3Dfp32 Pixel = _pImg->GetPixel3f(cr, CPnt(x,y));
					r = fp32(Pixel[0]);
					g = fp32(Pixel[1]);
					b = fp32(Pixel[2]);
				}
				else
				{
					CPixel32 Pixel = _pImg->GetPixel(cr, CPnt(x,y));
					r = fp32(Pixel.GetR());
					g = fp32(Pixel.GetG());
					b = fp32(Pixel.GetB());
					a = fp32(Pixel.GetA());
				}
			}
			else
			{
				for(int iW = 0; iW < _nWeights; iW++)
				{
					int xr = (x + _pWeights[iW].m_dX + w) % w;
					int yr = (y + _pWeights[iW].m_dY + h) % h;

					if (Fmt == IMAGE_FORMAT_RGB32_F)
					{
						CVec3Dfp32 Pixel = _pImg->GetPixel3f(cr, CPnt(xr,yr));
						fp32 w = _pWeights[iW].m_Weight;
						r += fp32(Pixel[0]) * w;
						g += fp32(Pixel[1]) * w;
						b += fp32(Pixel[2]) * w;
					}
					else
					{
						CPixel32 Pixel = _pImg->GetPixel(cr, CPnt(xr,yr));
						fp32 w = _pWeights[iW].m_Weight;
						r += fp32(Pixel.GetR()) * w;
						g += fp32(Pixel.GetG()) * w;
						b += fp32(Pixel.GetB()) * w;
						a += fp32(Pixel.GetA()) * w;
					}
				}
			}

			if (Fmt == IMAGE_FORMAT_RGB32_F)
			{
				spDst->SetPixel3f(cr, CPnt(x, y), CVec3Dfp32(Max(0.0f, r),Max(0.0f, g),Max(0.0f,b)) );
			}
			else
			{
				CPixel32 Res(RoundToInt(Max(0.0f, r)), RoundToInt(Max(0.0f, g)), RoundToInt(Max(0.0f, b)), RoundToInt(Max(0.0f, a)));
				spDst->SetPixel(cr, CPnt(x, y), Res);
			}
		}
	}

	return spDst;
}

static spCImage NormalizeTexture(CImage* _pImg, CFilterWeight* _pWeights, int _nWeights)
{
	MAUTOSTRIP(NormalizeTexture, NULL);

	if (_pImg->GetFormat() != IMAGE_FORMAT_BGRA8 &&
		_pImg->GetFormat() != IMAGE_FORMAT_RGB32_F)
		Error_static("Sharpen", "Unsupported format.");

	int w = _pImg->GetWidth();
	int h = _pImg->GetHeight();

	spCImage spDst = MNew(CImage);
	spDst->Create(w, h, _pImg->GetFormat(), IMAGE_MEM_IMAGE);

	CClipRect cr = spDst->GetClipRect();

	int Fmt = _pImg->GetFormat();

	for(int y = 0; y < h; y++)
		for(int x = 0; x < w; x++)
		{
			if (Fmt == IMAGE_FORMAT_RGB32_F)
			{
				CVec3Dfp32 Pixel = _pImg->GetPixel3f(cr, CPnt(x,y));
				Pixel.Normalize();
				spDst->SetPixel3f(cr, CPnt(x, y), Pixel );
			}
			else
			{
				CPixel32 Pixel = _pImg->GetPixel(cr, CPnt(x,y));
				if (Pixel.GetR() == 0 && Pixel.GetG() == 0 && Pixel.GetB() == 0)
				{
					Pixel = CPixel32(255,127,127, Pixel.GetA());
				}
				CVec3Dfp32 Vec(Pixel.GetR()-127, Pixel.GetG()-127, Pixel.GetB()-127);
				Vec.Normalize();
				Vec += 1.0f;
				Vec *= 127.0f;
				spDst->SetPixel(cr, CPnt(x, y), CPixel32((int)Vec[0], (int)Vec[1], (int)Vec[2], Pixel.GetA()));
			}
		}

	return spDst;
}

void CTexture::Create(spCImage _spTxt, int _nMipmaps, int _DestFormat, int _ImageConvertType)
{
	MAUTOSTRIP(CTexture_Create, MAUTOSTRIP_VOID);

	m_nMipmaps = 0;
	spCImage spT1 = MNew(CImage);
	spCImage spT2 = MNew(CImage);
	if (spT1 == NULL) MemError("-");
	if (spT2 == NULL) MemError("-");
	int Fmt = (_DestFormat == -1) ? _spTxt->GetFormat() : _DestFormat;
//Fmt = IMAGE_FORMAT_BGRA8;
//spT1->Create(_spTxt->GetWidth(), _spTxt->GetHeight(), Fmt, IMAGE_MEM_IMAGE, (Fmt & IMAGE_FORMAT_PALETTE) ? _spTxt->GetPalette() : NULL);
	spT1->Create(_spTxt->GetWidth(), _spTxt->GetHeight(), IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE, NULL);
	CImage::Convert(_spTxt, spT1, (_spTxt->GetFormat() & (IMAGE_FORMAT_ALPHA | IMAGE_FORMAT_CLUT8)) ? IMAGE_CONVERT_RGBA : IMAGE_CONVERT_RGB);

	spCImage spFiltered = spT1;
	if (m_Properties.m_Flags & CTC_TEXTUREFLAGS_NORMALMAP)
		spFiltered = NormalizeTexture(spT1, Sharpen, 9);

	m_LargestMap.Create(*spFiltered);
	m_LargestMap.Blt(m_LargestMap.GetClipRect(), *spFiltered, 0, CPnt(0, 0));
	if (_spTxt->GetFormat() & IMAGE_FORMAT_PALETTE)
		m_LargestMap.SetPalette(_spTxt->GetPalette());

	int nMip = Min(_nMipmaps, CTexture::GetMipMapLevels(_spTxt->GetWidth(), _spTxt->GetHeight()));
	if (nMip > CTC_MAXTEXTURESIZESHIFT)
		Error("Create", CStrF("Too large texture (%dx%d)", _spTxt->GetWidth(), _spTxt->GetHeight()));

	if (nMip > 0)
		for (int n = 1; n < nMip; n++)
		{
			int w = Max(_spTxt->GetWidth() >> n, 1);
			int h = Max(_spTxt->GetHeight() >> n, 1);
			spT2->Create(w, h, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);

			if (spT1->GetWidth() == 1)
				CImage::StretchHalfY(spT1, spT2);
			else if (spT1->GetHeight() == 1)
				CImage::StretchHalfX(spT1, spT2);
			else
				CImage::StretchHalf(spT1, spT2);

			CFilterWeight W = { 0, 0, 1 };

//			spCImage spFiltered = Filter(spT2, &W, 1);

			spCImage spFiltered;
			if (m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOSHARPEN)
				spFiltered = spT2->Duplicate();
			else
			{
				if (m_Properties.m_Flags & CTC_TEXTUREFLAGS_NORMALMAP)
				{
					spFiltered = NormalizeTexture(spT2, Sharpen, 9);
				}
				else
					spFiltered = Filter(spT2, Sharpen2, 9, m_Properties.m_Flags & (CTC_TEXTUREFLAGS_CLAMP_U | CTC_TEXTUREFLAGS_CLAMP_V));
			}

/*			spCImage spT3 = DNew(CImage) CImage;
			spT3->Create(w, h, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
			CImage::Convert(spT2, spT3);*/
			m_lspMaps[n] = spFiltered;
//			m_lspMaps[n] = spT2->Duplicate();

			Swap(spT1, spT2);
		}

	if (_DestFormat == IMAGE_FORMAT_CLUT8)
	{
		CDefaultQuantize Quant;
		Quant.Begin();
		Quant.Include(&m_LargestMap);
		for(int i = 1; i < nMip; i++)
			Quant.Include(m_lspMaps[i]);
		Quant.End();

		spCImage spLargest = Quant.Quantize(&m_LargestMap);
		spLargest->Duplicate(&m_LargestMap);
		
		for(int i = 1; i < nMip; i++)
			m_lspMaps[i] = Quant.Quantize(m_lspMaps[i]);
//		Fmt = IMAGE_FORMAT_BGRA8;
	}
	else
	{
//		int Fmt = (_DestFormat == -1) ? _spTxt->GetFormat() : _DestFormat;
		int ImageConvertType = _ImageConvertType;
	
		if (ImageConvertType == -1)
			ImageConvertType = (Fmt & IMAGE_FORMAT_ALPHA) ? IMAGE_CONVERT_RGBA : IMAGE_CONVERT_RGB;
		
		for (int n = 0; n < nMip; n++)
		{
			int w = Max(_spTxt->GetWidth() >> n, 1);
			int h = Max(_spTxt->GetHeight() >> n, 1);
			spCImage spT = MNew(CImage);
			spCImagePalette spPal = NULL;
			if(Fmt & IMAGE_FORMAT_PALETTE) spPal = _spTxt->GetPalette();
			spT->Create(w, h, Fmt, IMAGE_MEM_IMAGE,  spPal );
			if (n == 0)
			{
				CImage::Convert(&m_LargestMap, spT, ImageConvertType);
				m_LargestMap.Create(*spT);
				m_LargestMap.Blt(m_LargestMap.GetClipRect(), *spT, 0, CPnt(0, 0));
				if (spT->GetFormat() & IMAGE_FORMAT_PALETTE)
					m_LargestMap.SetPalette(spT->GetPalette());
			}
			else
			{
				CImage::Convert(m_lspMaps[n], spT, ImageConvertType);
				m_lspMaps[n] = spT;
			}
		}
	}

	m_nMipmaps = nMip;
	for (int n = 0; n < nMip; n++)
		m_IsLoaded |= (1<<n);
	//	m_bIsLoaded = true;
}
#endif

void CTexture::Read(CDataFile* _pDFile, TArray<spCImagePalette>* _plspPalettes, int _iPalBase)
{
	MAUTOSTRIP(CTexture_Read, MAUTOSTRIP_VOID);

	M_TRY
	{
	/* Reads:
	[NAME]
	[PALETTE]
	[PALETTEINDEX]
	MIPMAP * nMipMaps
	:
	:
		*/
		int w, h, d;
		spCImagePalette spPal;
		CStr NodeName;
		while((NodeName = _pDFile->GetNext()) != (const char *)"")
		{
			CCFile* pFile = _pDFile->GetFile();
			uint32 NodeHash = StringToHash(NodeName.GetStr());
			switch(NodeHash)
			{
			case MHASH1('NAME'):
				{
					if (_pDFile->GetUserData())
					{
						if (_pDFile->GetUserData() > 31) Error("Read", "Too long texture-name.");
#ifdef USE_HASHED_TEXTURENAME
						char Tmp[32];
						pFile->Read(&Tmp, _pDFile->GetUserData()-1);
						Tmp[_pDFile->GetUserData()-1] = 0;
						m_NameID = StringToHash(Tmp);
#else					
						pFile->Read(&m_Name, _pDFile->GetUserData()-1);
						m_Name[_pDFile->GetUserData()-1] = 0;
#endif					
					}
					else
					{
						CFStr TmpStr;
						TmpStr.Read(pFile);
						if (TmpStr.Len() > 31) Error("Read", "Too long texture-name.");
#ifdef USE_HASHED_TEXTURENAME
						m_NameID = StringToHash(TmpStr);
#else					
						strcpy(m_Name, (char*)TmpStr);
#endif					
					}
				}
				break;

			case MHASH3('PROP', 'ERTI', 'ES'):
				{
					m_Properties.Read(pFile);
				}
				break;

			case MHASH4('TXTP', 'ROP_', 'FLAG', 'S'):
				{
					m_Properties.m_Flags = _pDFile->GetUserData();
				}
				break;

			case MHASH4('TXTP', 'ROP_', 'FILT', 'ER'):
				{
					m_Properties.m_MagFilter = _pDFile->GetUserData() & 0xff;
					m_Properties.m_MinFilter = (_pDFile->GetUserData() >> 8) & 0xff;
					m_Properties.m_MIPFilter = (_pDFile->GetUserData() >> 16) & 0xff;
				#ifndef USE_PACKED_TEXTUREPROPERTIES
					m_Properties.m_MIPMapLODBias = (_pDFile->GetUserData() >> 24) & 0xff;
				#endif
				}
				break;

			case MHASH3('PALE', 'TTEI', 'NDEX'):
				{
					m_iPalette = _pDFile->GetUserData();
					if (m_iPalette != -1) m_iPalette += _iPalBase;
				}
				break;

			case MHASH2('PALE', 'TTE'):
				{
					if (spPal == NULL) spPal = MNew(CImagePalette);
					if (spPal == NULL) MemError("Read");
					if (_pDFile->GetUserData() != 256) Error("Read", "Supports only 256 color palettes.");

					uint8 Tmp[256*4];
					pFile->Read(&Tmp, sizeof(Tmp));

					CPixel32 Pal[256];
					for (int i=0; i<256; i++)
						Pal[i] = CPixel32(Tmp[i*4+2], Tmp[i*4+1], Tmp[i*4+0], Tmp[i*4+3]);

					spPal->SetPalette(Pal, 0, 256);
				}
				break;

			case MHASH2('MIPM', 'AP'):
				{
					if (m_nMipmaps == 0)
					{
						if ((m_iPalette != -1) && (_plspPalettes)) spPal = (*_plspPalettes)[m_iPalette];
						m_LargestMapFilePos = pFile->Pos();
						m_LargestMap.Read(pFile, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, spPal);
						if (spPal != NULL)
							m_LargestMap.SetPalette(spPal);				
						w = m_LargestMap.GetWidth() >> 1;
						h = m_LargestMap.GetHeight() >> 1;
						d = m_LargestMap.GetPixelSize();
					}
					else
					{
						// Just skip to the next mipmap
						m_lMapFilePos[m_nMipmaps] = pFile->Pos();
						/*				int DataSize = 0x10 + (w*h*d);
						w >>= 1; h >>= 1;
						pFile->Seek(pFile->Pos() + DataSize);*/
						
						spCImage spImg = MNew(CImage);
						if (spImg == NULL) MemError("Read");
						
						if ((m_iPalette != -1) && (_plspPalettes)) spPal = (*_plspPalettes)[m_iPalette];
						spImg->Read(pFile, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, spPal);
						if (spPal != NULL)
							spImg->SetPalette(spPal);				
						if (m_nMipmaps > 1)
							if ((spImg->GetWidth() != Max(1, m_lspMaps[m_nMipmaps-1]->GetWidth() >> 1)) ||
								(spImg->GetHeight() != Max(1, m_lspMaps[m_nMipmaps-1]->GetHeight() >> 1)))
								Error("Read", CStrF("Invalid mipmap size: %dx%d", spImg->GetWidth(), spImg->GetHeight()));
							
							m_lspMaps[m_nMipmaps] = spImg;
							
					}
					m_nMipmaps++;
				}
				break;
			}
		}
		
		//	m_lspMaps.OptimizeMemory();
		//	m_bIsLoaded = true;
		m_IsLoaded |= (1 << m_nMipmaps);
		
	}
	M_CATCH(
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		throw;
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch (CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		throw;
	}
	)
#endif
}

#ifndef PLATFORM_CONSOLE
void CTexture::Write(CDataFile* _pDFile)
{
#ifdef USE_HASHED_TEXTURENAME	
	Error("CTexture::Write", "Cannot write texture with texture names disabled!");
#else	
	MAUTOSTRIP(CTexture_Write, MAUTOSTRIP_VOID);

/* Writes:
		[NAME]
		[PALETTE]
		[PALETTEINDEX]
		MIPMAP * nMipMaps
		:
		:
*/
	CCFile* pFile = _pDFile->GetFile();

	// Write NAME
	CStr TmpStr(m_Name);
	if (TmpStr != (const char *)"")
	{
		if (TmpStr.Len() > 23)
			Error("Write", CStrF("Texture names can't have more than 23 characters. (%s)", TmpStr.Str()));

		_pDFile->BeginEntry("NAME");
		TmpStr.Write(pFile);
/*			char Name[16];
			FillChar(&Name, sizeof(Name), 0);
			strcpy((char*) &Name, m_Name);
			pFile->Write(&Name, sizeof(Name));*/
		_pDFile->EndEntry(0);
		_pDFile->BeginEntry(TmpStr);
		_pDFile->EndEntry(-1);
	}

	// Write PROPERTIES
	{
		_pDFile->BeginEntry("PROPERTIES");
		m_Properties.Write(pFile);
		_pDFile->EndEntry(CTC_PROPERTIES_VERSION, sizeof(CTC_TextureProperties));
	}

	// Write TXTPROP_FLAGS ?
/*	CTC_TextureProperties Default;
	if (m_Properties.m_Flags != Default.m_Flags)
	{
		_pDFile->BeginEntry("TXTPROP_FLAGS");
		_pDFile->EndEntry(m_Properties.m_Flags);
	}

	// Write TXTPROP_FILTER ?
	if (m_Properties.m_MagFilter != Default.m_MagFilter ||
		m_Properties.m_MinFilter != Default.m_MinFilter ||
		m_Properties.m_MIPFilter != Default.m_MIPFilter ||
		m_Properties.m_MIPMapLODBias != Default.m_MIPMapLODBias)
	{
		int Data = 
			m_Properties.m_MagFilter + 
			(m_Properties.m_MinFilter << 8) + 
			(m_Properties.m_MIPFilter << 16) + 
			(m_Properties.m_MIPMapLODBias << 24);

		_pDFile->BeginEntry("TXTPROP_FILTER");
		_pDFile->EndEntry(Data);
	}
*/
	bool bIsMemoryFile = (pFile->GetFileName() == "") != 0;

	// Write PALETTEINDEX or PALETTE
	if (m_iPalette != -1)
	{
		_pDFile->BeginEntry("PALETTEINDEX");
		_pDFile->EndEntry(m_iPalette);
	}
	else
		if (IsPalettized())
		{
			_pDFile->BeginEntry("PALETTE");
			if(bIsMemoryFile == false)
				m_PaletteFilePos = pFile->Pos();
			pFile->Write(m_LargestMap.GetPalette()->GetPalettePtr(), 4*256);
			_pDFile->EndEntry(256);
		}

	// Write MIPMAPs
//	m_lMapFilePos.SetLen(m_nMipmaps);
/*
	_pDFile->BeginEntry("MIPMAP");
	m_LargestMapFilePos = pFile->Pos();
	m_LargestMap.Write(pFile);
	_pDFile->EndEntry(m_LargestMap.GetWidth() + (m_LargestMap.GetHeight() << 16), m_LargestMap.GetFormat());
*/

//	m_LargestMap.Write(CRct(0,0,-1,-1), CStr("S:\\TCLightMap.tga"));


	for(int iMaps = 0; iMaps < m_nMipmaps; iMaps++)
	{
		_pDFile->BeginEntry("MIPMAP");
		if (bIsMemoryFile)
		{
			if (!iMaps)
				m_LargestMapFilePos = 0;
			else
				m_lMapFilePos[iMaps] = 0;
		}
		else
		{
			if (!iMaps)
				m_LargestMapFilePos = pFile->Pos();
			else
				m_lMapFilePos[iMaps] = pFile->Pos();
		}
		CImage* pMap;
		if (iMaps == 0)
			pMap = &m_LargestMap;
		else
			pMap = m_lspMaps[iMaps];
		pMap->Write(pFile);
		_pDFile->EndEntry(pMap->GetWidth() + (pMap->GetHeight() << 16), pMap->GetFormat());
	}
#endif	
}

void CTexture::Write2(CDataFile* _pDFile, int32 *_PicMip)
{
	MAUTOSTRIP(CTexture_Write2, MAUTOSTRIP_VOID);

	CCFile* pFile = _pDFile->GetFile();

/*	pFile->WriteLE(m_Properties.m_Flags);
		
	int32 Data = 
			m_Properties.m_MagFilter + 
			(m_Properties.m_MinFilter << 8) + 
			(m_Properties.m_MIPFilter << 16) + 
			(m_Properties.m_MIPMapLODBias << 24);

	pFile->WriteLE(Data);

	pFile->WriteLE(m_iPalette);
*/
	int8 HasPallette = IsPalettized();
//	pFile->WriteLE(HasPallette);

	if (HasPallette)
	{
		m_PaletteFilePos = pFile->Pos();
		pFile->Write(m_LargestMap.GetPalette()->GetPalettePtr(), 4*256);
	}

	bool bIsMemoryFile = (pFile->GetFileName() == "") != 0;

	int PicMip = MaxMT(0, _PicMip[m_Properties.m_iPicMipGroup] + m_Properties.GetPicMipOffset());

	if (m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP)
		PicMip = 0;

	if (PicMip > m_nMipmaps - 1)
		PicMip = m_nMipmaps - 1;

	m_MipMapFilePosPos = pFile->Pos();
#ifndef PLATFORM_CONSOLE
	pFile->DisableWrite();
#endif
	int iMaps;
	for(iMaps = PicMip; iMaps < m_nMipmaps; iMaps++)
	{
		int32 Temp = 0;
		pFile->WriteLE(Temp);
	}

	// Only pretend
	for(iMaps = PicMip; iMaps < m_nMipmaps; iMaps++)
	{
		if (bIsMemoryFile)
		{
			if (!iMaps)
				m_LargestMapFilePos = 0;
			else
				m_lMapFilePos[iMaps] = 0;
		}
		else
		{
			if (!iMaps)
				m_LargestMapFilePos = pFile->Pos();
			else
				m_lMapFilePos[iMaps] = pFile->Pos();
		}
		CImage* pMap;
		if (iMaps == 0)
			pMap = &m_LargestMap;
		else
			pMap = m_lspMaps[iMaps];
		pMap->Write(pFile);
	}
#ifndef PLATFORM_CONSOLE
	pFile->EnableWrite();
#endif

	int EndPos = pFile->Pos();

	pFile->Seek(m_MipMapFilePosPos);

	for(iMaps = PicMip; iMaps < m_nMipmaps; iMaps++)
	{
		int32 Pos;
		if (!iMaps)
			Pos = m_LargestMapFilePos;
		else
			Pos = m_lMapFilePos[iMaps];
		pFile->WriteLE(Pos);
	}

	for(iMaps = PicMip; iMaps < m_nMipmaps; iMaps++)
	{
		CImage* pMap;
		if (iMaps == 0)
			pMap = &m_LargestMap;
		else
			pMap = m_lspMaps[iMaps];
		pMap->Write(pFile);
	}

	M_ASSERT(EndPos == pFile->Pos(), "Error");

	//	pFile->Seek(EndPos);
}
#endif


void CTexture::ReadIndexData(CCFile* _pFile, TArray<spCImagePalette>* _plspPalettes, int _iPalBase)
{
	MAUTOSTRIP(CTexture_ReadIndexData, MAUTOSTRIP_VOID);

	_pFile->ReadLE(m_iPalette);
	if (m_iPalette >= 0) m_iPalette += _iPalBase;
	_pFile->ReadLE(m_PaletteFilePos);
	{
		CFStr TmpStr;
		TmpStr.Read(_pFile);
		if (TmpStr.Len() > 31) Error("Read", "Too long texture-name.");
#ifdef USE_HASHED_TEXTURENAME
		m_NameID = StringToHash(TmpStr);
#else		
		strcpy(m_Name, (char*)TmpStr);
#endif		
	}
	_pFile->ReadLE(m_nMipmaps);
//	m_lspMaps.SetLen(m_nMipmaps);
/*
	int i;
	for(i = 0; i < m_nMipmaps; i++)
	{
		if (i == 0)
		{
			m_LargestMap.ReadHeader(_pFile);
		} else {
			m_lspMaps[i] = DNew(CImage) CImage;
			if (!m_lspMaps[i]) MemError("ReadIndexData");
			m_lspMaps[i]->ReadHeader(_pFile);
		}
	}
*/
	// Read the largest map and skip the rest
	m_LargestMap.ReadHeader(_pFile);

//	if (m_nMipmaps > 0) _pFile->Seek(_pFile->Pos() + ((m_nMipmaps-1)*0x10));		// sizeof(CImage_FileHeader) == 0x10
//		_pFile->Seek(_pFile->Pos() + ((m_nMipmaps-1)*sizeof(CImage_FileHeader)));

//	m_lMapFilePos.SetLen(m_nMipmaps);
	for(int i = 0; i < m_nMipmaps; i++)
	{
		if (i == 0)
			_pFile->ReadLE(m_LargestMapFilePos);
		else
			_pFile->ReadLE(m_lMapFilePos[i]);
	}

	if (m_iPalette >= 0)
		m_spPal = (*_plspPalettes)[m_iPalette];

	m_Properties.Read(_pFile);
}

#ifndef PLATFORM_CONSOLE

void CTexture::WriteIndexData(CCFile* _pFile)
{
#ifdef USE_HASHED_TEXTURENAME
	Error("CTexture::WriteIndexData", "Cannot write texture when texture names are disabled!");
#else	
	MAUTOSTRIP(CTexture_WriteIndexData, MAUTOSTRIP_VOID);

	_pFile->WriteLE(m_iPalette);
	_pFile->WriteLE(m_PaletteFilePos);
	{
		CStr TmpStr(m_Name);
		TmpStr.Write(_pFile);
	}
	_pFile->WriteLE(m_nMipmaps);
	m_LargestMap.WriteHeader(_pFile);
/*	int i;
	for(i = 0; i < m_nMipmaps; i++)
	{
		if (i == 0)
		else
			m_lspMaps[i]->WriteHeader(_pFile);
	}*/
//	m_lMapFilePos.SetLen(m_lspMaps.Len());
	for(int i = 0; i < m_nMipmaps; i++)
	{
		if (i == 0)
			_pFile->WriteLE(m_LargestMapFilePos);
		else
			_pFile->WriteLE(m_lMapFilePos[i]);
	}
	m_Properties.Write(_pFile);
#endif
}

void CTexture::WriteIndexData2(CCFile* _pFile, int32 *_PicMip)
{
#ifdef USE_HASHED_TEXTURENAME
	Error("CTexture::WriteIndexData2", "Cannot write texture when texture names are disabled!");
#else
	MAUTOSTRIP(CTexture_WriteIndexData2, MAUTOSTRIP_VOID);

	_pFile->WriteLE(m_iPalette);
	_pFile->WriteLE(m_PaletteFilePos);
	{
		CStr TmpStr(m_Name);
		TmpStr.Write(_pFile);
	}

	int32 PicMip = MaxMT(0, _PicMip[m_Properties.m_iPicMipGroup] + m_Properties.GetPicMipOffset());

	if (m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP)
		PicMip = 0;

	if (PicMip > m_nMipmaps - 1)
		PicMip = m_nMipmaps - 1;

	_pFile->WriteLE(PicMip);
	_pFile->WriteLE(m_nMipmaps);
	
	m_LargestMap.WriteHeader(_pFile);
	_pFile->WriteLE(m_MipMapFilePosPos);
	m_Properties.Write(_pFile);
#endif	
}
#endif


bool CTexture::IsCompressed()
{
	MAUTOSTRIP(CTexture_IsCompressed, false);

	return m_LargestMap.IsCompressed() != 0;
}

#ifndef PLATFORM_CONSOLE
void CTexture::Compress(int _Compression, fp32 _Quality)
{
#ifdef USE_HASHED_TEXTURENAME
	Error("CTexture::Compress", "Can't compress texture without name!");
#else
	MAUTOSTRIP(CTexture_Compress, MAUTOSTRIP_VOID);

//	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	int Format = m_LargestMap.GetFormat();
	int Platform = D_MPLATFORM;
	if ( ((_Compression & IMAGE_MEM_COMPRESSTYPE_ALWAYS) || !(m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS)) && 
		!m_LargestMap.IsCompressed() && 
		m_LargestMap.CanCompress(_Compression))
	{
		if( _Compression & IMAGE_MEM_COMPRESSTYPE_QUANT )
		{
			if( Format != IMAGE_FORMAT_CLUT8 )
			{
//				LogFile(CStrF("Compressing %s", m_Name));
	#if 0 // OctTree
				COctreeQuantize Quant;
				Quant.Begin();
				Quant.Include(&m_LargestMap);
				for(int i = 1; i < m_nMipmaps; i++)
					Quant.Include(m_lspMaps[i], );
				Quant.End();
	#else // NeuQuant
				CDefaultQuantize Quant;
				Quant.Begin();
				Quant.Include(&m_LargestMap);
				for(int i = 1; i < m_nMipmaps; i++)
		//			Quant.Include(m_lspMaps[i], (1 << i) * (1 << i));
					Quant.Include(m_lspMaps[i]);
				Quant.End();
	#endif

				spCImage spLargest = Quant.Quantize(&m_LargestMap);
				spLargest->Duplicate(&m_LargestMap);
				
				for(int i = 1; i < m_nMipmaps; i++)
					m_lspMaps[i] = Quant.Quantize(m_lspMaps[i]);
			}
		}
		else
		{
//			if (m_LargestMap.GetWidth() * m_LargestMap.GetHeight() <= 256) return;

//			LogFile(CStrF("Compressing %s", m_Name));

			CImage Compressed;
			m_LargestMap.Compress(_Compression, _Quality, &Compressed, this);
			m_LargestMap.Destroy();
			Compressed.Duplicate(&m_LargestMap);

			for(int i = 1; i < m_nMipmaps; i++)
			{
	//			if (m_lspMaps[i]->GetWidth() * m_lspMaps[i]->GetHeight() <= 256) continue;
//				if (!m_lspMaps[i]->CanCompress(_Compression)) continue;

				spCImage spCompressed = MNew(CImage);
				if (!spCompressed) MemError("Compress");
				m_lspMaps[i]->Compress(_Compression, _Quality, spCompressed, this);
				m_lspMaps[i] = spCompressed;
			}
		}
	}
#endif	
}

void CTexture::Decompress(bool _DecompMipmap)
{
	MAUTOSTRIP(CTexture_Decompress, MAUTOSTRIP_VOID);

	//Error("Decompress", "Not implemented.");
	
	if(m_LargestMap.IsCompressed() != 0)
	{
		CImage Decompressed;
		m_LargestMap.Decompress(&Decompressed);
		Decompressed.Duplicate(&m_LargestMap);
	}
	
	if(_DecompMipmap)
	{
		for(int i = 1; i < m_nMipmaps; i++)
		{
			if(m_lspMaps[i]->IsCompressed() != 0)
			{
				spCImage spDecompressed = MNew(CImage);
				if (!spDecompressed) MemError("Decompress");
				m_lspMaps[i]->Decompress(spDecompressed);
				m_lspMaps[i] = spDecompressed;
			}
		}
	}
}
#endif

// Read the mipmap-headers
void CTexture::Virtual_Read(CDataFile* _pDFile, TArray<spCImagePalette>* _plspPalettes, int _iPalBase)
{
	MAUTOSTRIP(CTexture_VirtualRead, MAUTOSTRIP_VOID);

	M_TRY
	{
		
		spCImagePalette spPal;
		CStr NodeName;
		while((NodeName = _pDFile->GetNext()) != (const char *)"")
		{
			CCFile* pFile = _pDFile->GetFile();
			if (NodeName == (const char *)"NAME")
			{
			/*			char Name[16];
			pFile->Read(&Name, 16);
			Name[15] = 0;
				m_Name = (char*) &Name;*/
				
				//			m_Name = _pDFile->GetNext();
				{
					CStr TmpStr;
					TmpStr = _pDFile->GetNext();
					if (TmpStr.Len() > 31) Error("Read", "Too long texture-name.");
#ifdef USE_HASHED_TEXTURENAME
					m_NameID = StringToHash(TmpStr);
#else					
					strcpy(m_Name, (char*)TmpStr);
#endif					
				}
#ifndef USE_HASHED_TEXTURENAME
				if (m_Name == (const char *)"") Error("Virtual_Read", "No entry after 'NAME'.");
#endif				
				if (_pDFile->GetUserData() != -1) Error("Virtual_Read", "Entry after 'NAME' was not a name-entry.");
				
				//			_pDFile->BeginEntry(m_Name);
				//			_pDFile->EndEntry(-1);
			}
			else if (NodeName == (const char *)"PROPERTIES")
			{
				m_Properties.Read(pFile);
			}
			else if (NodeName == (const char *)"TXTPROP_FLAGS")
			{
				m_Properties.m_Flags = _pDFile->GetUserData();
			}
			else if (NodeName == (const char *)"TXTPROP_FILTER")
			{
				m_Properties.m_MagFilter = _pDFile->GetUserData() & 0xff;
				m_Properties.m_MinFilter = (_pDFile->GetUserData() >> 8) & 0xff;
				m_Properties.m_MIPFilter = (_pDFile->GetUserData() >> 16) & 0xff;
			#ifndef USE_PACKED_TEXTUREPROPERTIES
				m_Properties.m_MIPMapLODBias = (_pDFile->GetUserData() >> 24) & 0xff;
			#endif
			}
			else if (NodeName == (const char *)"PALETTEINDEX")
			{
				m_iPalette = _pDFile->GetUserData();
				if (m_iPalette != -1) m_iPalette += _iPalBase;
				if ((m_iPalette != -1) && (_plspPalettes)) 
					m_spPal = (*_plspPalettes)[m_iPalette];
			}
			else if (NodeName == (const char *)"PALETTE")
			{
				if (_pDFile->GetUserData() != 256) Error("Read", "Supports only 256 color palettes.");
				m_PaletteFilePos = pFile->Pos();
				/*			if (spPal == NULL) spPal = DNew(CImagePalette) CImagePalette;
				if (spPal == NULL) MemError("Read");
				CPixel32 Pal[256];
				if (_pDFile->GetUserData() != 256) Error("Read", "Supports only 256 color palettes.");
				
				  pFile->Read(&Pal, sizeof(Pal));
				  spPal->SetPalette((CPixel32*) &Pal, 0, 256);
				m_spPal = spPal;*/
			}
			else if (NodeName == (const char *)"MIPMAP")
			{
				if (m_nMipmaps == 0)
					m_LargestMapFilePos = pFile->Pos();
				else
					m_lMapFilePos[m_nMipmaps] = pFile->Pos();
				
				if ((m_iPalette != -1) && (_plspPalettes)) spPal = (*_plspPalettes)[m_iPalette];
				
				if (m_nMipmaps == 0)
				{
					// Check if the image-description is coded into the userdata
					if (_pDFile->GetUserData() && _pDFile->GetUserData2())
					{
						int w = _pDFile->GetUserData() & 0xffff;
						int h = _pDFile->GetUserData() >> 16;
						int fmt = _pDFile->GetUserData2();
						m_LargestMap.CreateVirtual(w, h, fmt, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM | IMAGE_MEM_VIRTUAL, spPal);
					}
					else
						m_LargestMap.Read(pFile, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM | IMAGE_MEM_VIRTUAL, spPal);
						}/* else {
						 spCImage spImg = DNew(CImage) CImage;
						 if (spImg == NULL) MemError("Read");
						 
						   // Check if the image-description is coded into the userdata
						   if (_pDFile->GetUserData() && _pDFile->GetUserData2())
						   {
						   int w = _pDFile->GetUserData() & 0xffff;
						   int h = _pDFile->GetUserData() >> 16;
						   int fmt = _pDFile->GetUserData2();
						   spImg->CreateVirtual(w, h, fmt, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM | IMAGE_MEM_VIRTUAL, spPal);
						   }
						   else
						   spImg->Read(pFile, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM | IMAGE_MEM_VIRTUAL, spPal);
						   
							 int nMip = m_nMipmaps;
							 if (nMip > 1)
							 {
							 if ((spImg->GetWidth() != Max(1, m_lspMaps[nMip-1]->GetWidth() >> 1)) ||
							 (spImg->GetHeight() != Max(1, m_lspMaps[nMip-1]->GetHeight() >> 1)))
							 Error("Read", CStrF("Invalid mipmap size: %dx%d", spImg->GetWidth(), spImg->GetHeight()));
							 }
							 m_lspMaps[m_nMipmaps] = spImg;
			}*/
				m_nMipmaps++;
			}
			else
				LogFile("(CTexture::Virtual_Read) Entry not parsed " + NodeName);
	}
	}
	M_CATCH(
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		throw;
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch (CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		throw;
	}
	)
#endif

//	m_lspMaps.OptimizeMemory();
//	m_lMapFilePos.OptimizeMemory();
//	m_nMipmaps = m_lMapFilePos.Len();
}

// Load the actual texture
void CTexture::Virtual_Load(CCFile* _pFile)
{
	MAUTOSTRIP(CTexture_Virtual_Load_CCFile, MAUTOSTRIP_VOID);

	Virtual_Load(_pFile, 0);
}

void CTexture::Virtual_Load(CCFile* _pFile, int _iMipmap)
{
	MAUTOSTRIP(CTexture_Virtual_Load_CCFile_int, MAUTOSTRIP_VOID);

	M_TRY
	{
		if ((m_IsLoaded>>_iMipmap)&1) return;
		
		if (m_PaletteFilePos && (m_spPal == NULL))
		{
			spCImagePalette spPal;
			_pFile->Seek(m_PaletteFilePos);
			if (spPal == NULL) spPal = MNew(CImagePalette);
			if (spPal == NULL) MemError("Read");
			//		if (_pDFile->GetUserData() != 256) Error("Read", "Supports only 256 color palettes.");

			uint8 Tmp[256*4];
			_pFile->Read(&Tmp, 256*4);

			CPixel32 Pal[256];
			for (int i=0; i<256; i++)
				Pal[i] = CPixel32(Tmp[i*4+2], Tmp[i*4+1], Tmp[i*4+0], Tmp[i*4+3]);

			spPal->SetPalette(Pal, 0, 256);
			m_spPal = spPal;
		}
		
		CImage* pImg;
		// Load selected mipmap
		if (_iMipmap > 0)
		{
			if (m_lspMaps[_iMipmap] == NULL)
				m_lspMaps[_iMipmap] = MNew(CImage);
			
			if (!m_lspMaps[_iMipmap]->GetMemModel() || m_lspMaps[_iMipmap]->GetMemModel() & IMAGE_MEM_VIRTUAL)
			{
				_pFile->Seek(m_lMapFilePos[_iMipmap]);
				m_lspMaps[_iMipmap]->Read(_pFile, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, m_spPal);
			}
			pImg = m_lspMaps[_iMipmap];
		}
		else
		{
			if (!m_LargestMap.GetMemModel() || (m_LargestMap.GetMemModel() & IMAGE_MEM_VIRTUAL))
			{
				_pFile->Seek(m_LargestMapFilePos);
				m_LargestMap.Read(_pFile, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, m_spPal);
				
				/*			if (m_LargestMap.GetFormat() == IMAGE_FORMAT_BGR8 &&
				m_LargestMap.GetWidth() == 512 &&
				m_LargestMap.GetHeight() == 64)
				m_LargestMap.Write(CRct(0,0,-1,-1), CStr("s:\\TCEngineLightMap.tga"));*/
				
			}
			pImg = &m_LargestMap;
		}
		
		//	LogFile("(CTexture::Virtual_Load) " + _pFile->GetFileName() + ", "+ m_Name + ", " + pImg->IDString());
		m_IsLoaded |= (1 << _iMipmap);
	}
	M_CATCH(
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		throw;
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch (CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		throw;
	}
	)
#endif
	
}

void CTexture::Virtual_Unload()
{
	MAUTOSTRIP(CTexture_Virtual_Unload, MAUTOSTRIP_VOID);

	m_IsLoaded = 0;
	for(int i = 0; i < m_nMipmaps; i++)
	{
		if (i == 0)
		{
			m_LargestMap.CreateVirtual(m_LargestMap.GetWidth(), m_LargestMap.GetHeight(), m_LargestMap.GetFormat(), m_LargestMap.GetMemModel(), m_LargestMap.GetPalette());
		} else {
//			m_lspMaps[i] = NULL;
			if (m_lspMaps[i] != NULL) m_lspMaps[i]->CreateVirtual(m_lspMaps[i]->GetWidth(), m_lspMaps[i]->GetHeight(), m_lspMaps[i]->GetFormat(), m_lspMaps[i]->GetMemModel(), m_lspMaps[i]->GetPalette());
		}
	}
}

void CTexture::Virtual_Unload(int _iMipMap)
{
	MAUTOSTRIP(CTexture_Virtual_Unload_int, MAUTOSTRIP_VOID);

	m_IsLoaded &= ~(1 << _iMipMap);
	if (_iMipMap == 0)
	{
		m_LargestMap.CreateVirtual(m_LargestMap.GetWidth(), m_LargestMap.GetHeight(), m_LargestMap.GetFormat(), m_LargestMap.GetMemModel(), m_LargestMap.GetPalette());
	} else {
//		m_lspMaps[i] = NULL;
		if (m_lspMaps[_iMipMap] != NULL) m_lspMaps[_iMipMap]->CreateVirtual(m_lspMaps[_iMipMap]->GetWidth(), m_lspMaps[_iMipMap]->GetHeight(), m_lspMaps[_iMipMap]->GetFormat(), m_lspMaps[_iMipMap]->GetMemModel(), m_lspMaps[_iMipMap]->GetPalette());
	}
}

#ifndef PLATFORM_CONSOLE
void CTexture::SerializeWrite(CDataFile* _pDFile)
{
	_pDFile->BeginEntry("TEXTURE");
	_pDFile->EndEntry(0);
	_pDFile->BeginSubDir();
	{
		if(m_iPalette != -1)
			Error_static("CTexture::SerializeWrite", "Serialize does not support external palette");
		if(m_spPal)
		{
			_pDFile->BeginEntry("PALETTE");
			uint32 nColors = m_spPal->GetNumColors();
			_pDFile->GetFile()->WriteLE(nColors);
			_pDFile->GetFile()->Write(m_spPal->GetPalettePtr(), sizeof(CPixel32) * m_spPal->GetNumColors());
			_pDFile->EndEntry(0);
		}
		_pDFile->BeginEntry("NAME");
		_pDFile->GetFile()->Write(m_Name, 32);
		_pDFile->EndEntry(0);
		{
			_pDFile->BeginEntry("MIPMAPS");
			_pDFile->EndEntry(m_nMipmaps);
			_pDFile->BeginSubDir();
			{
				_pDFile->BeginEntry("MIPMAP");
				m_LargestMap.Write(_pDFile->GetFile());
				_pDFile->EndEntry(0);
			}
			for(int iMip = 1; iMip < m_nMipmaps; iMip++)
			{
				_pDFile->BeginEntry("MIPMAP");
				m_lspMaps[iMip]->Write(_pDFile->GetFile());
				_pDFile->EndEntry(0);
			}
			_pDFile->EndSubDir();
		}

		_pDFile->BeginEntry("PROPERTIES");
		m_Properties.Write(_pDFile->GetFile());
		_pDFile->EndEntry(0);
	}
	_pDFile->EndSubDir();
}

void CTexture::SerializeWrite(CDataFile* _pDFile, int32 _PicMip )
{
	if( m_nMipmaps < 2 || _PicMip < 1 )
		return SerializeWrite( _pDFile );

	_pDFile->BeginEntry("TEXTURE");
	_pDFile->EndEntry(0);
	_pDFile->BeginSubDir();
	{
		if(m_iPalette != -1)
			Error_static("CTexture::SerializeWrite", "Serialize does not support external palette");
		if(m_spPal)
		{
			_pDFile->BeginEntry("PALETTE");
			uint32 nColors = m_spPal->GetNumColors();
			_pDFile->GetFile()->WriteLE(nColors);
			_pDFile->GetFile()->Write(m_spPal->GetPalettePtr(), sizeof(CPixel32) * m_spPal->GetNumColors());
			_pDFile->EndEntry(0);
		}
		_pDFile->BeginEntry("NAME");
		_pDFile->GetFile()->Write(m_Name, 32);
		_pDFile->EndEntry(0);
		{
			int32 nSkip = Min(_PicMip, m_nMipmaps-1 );
			int32 nMipmaps = m_nMipmaps - nSkip;
			_pDFile->BeginEntry("MIPMAPS");
			_pDFile->EndEntry(nMipmaps);
			_pDFile->BeginSubDir();
			{
				_pDFile->BeginEntry("MIPMAP");
				if( nSkip )
					m_lspMaps[nSkip]->Write(_pDFile->GetFile());
				else
					m_LargestMap.Write(_pDFile->GetFile());
				_pDFile->EndEntry(0);
			}
			for(int iMip = 1; iMip < nMipmaps; iMip++)
			{
				_pDFile->BeginEntry("MIPMAP");
				m_lspMaps[iMip + nSkip ]->Write(_pDFile->GetFile());
				_pDFile->EndEntry(0);
			}
			_pDFile->EndSubDir();
		}

		_pDFile->BeginEntry("PROPERTIES");
		m_Properties.Write(_pDFile->GetFile());
		_pDFile->EndEntry(0);
	}
	_pDFile->EndSubDir();
}


void CTexture::SerializeRead(CDataFile* _pDFile)
{
	if(!_pDFile->GetNext("TEXTURE"))
		Error_static("CTexture::SerializeRead", "Could not find texture tag");
	_pDFile->GetSubDir();
	{
		_pDFile->PushPosition();
		spCImagePalette spPal;
		if(_pDFile->GetNext("PALETTE"))
		{
			TArray<CPixel32> lColors;
			uint32 nColors;
			_pDFile->GetFile()->ReadLE(nColors);
			lColors.SetLen(nColors);
			_pDFile->GetFile()->Read(lColors.GetBasePtr(), nColors * sizeof(CPixel32));
			spPal = MNew1(CImagePalette, nColors);
			spPal->SetPalette(lColors.GetBasePtr(), 0, nColors);
			m_spPal	= spPal;
		}
		_pDFile->PopPosition();
		if(!_pDFile->GetNext("NAME"))
			Error_static("CTexture::SerializeRead", "Could not find name");
		_pDFile->GetFile()->Read(m_Name, 32);
		if(!_pDFile->GetNext("MIPMAPS"))
			Error_static("CTexture::SerializeRead", "Could not find name");
		{
			m_nMipmaps = _pDFile->GetUserData();

			_pDFile->GetSubDir();
			if(!_pDFile->GetNext("MIPMAP"))
				Error_static("CTexture::SerializeRead", "Missing texture image");
			m_LargestMap.Read(_pDFile->GetFile(), IMAGE_MEM_IMAGE, spPal);
			for(int iMip = 1; iMip < m_nMipmaps; iMip++)
			{
				if(!_pDFile->GetNext("MIPMAP"))
					Error_static("CTexture::SerializeRead", "Missing texture mipmap");
				m_lspMaps[iMip]	= MNew(CImage);
				m_lspMaps[iMip]->Read(_pDFile->GetFile(), IMAGE_MEM_IMAGE, spPal);

			}
			_pDFile->GetParent();
		}

		if(!_pDFile->GetNext("PROPERTIES"))
			Error_static("CTexture::SerializeRead", "Missing properties");

		m_Properties.Read(_pDFile->GetFile());

	}
	_pDFile->GetParent();
}
#endif
#if 0
CTextureCache_Entry::CTextureCache_Entry()
{
	MAUTOSTRIP(CTextureCache_Entry_ctor, MAUTOSTRIP_VOID);

	m_Texture = -1;
	m_MipMap = -1;
};

CTextureCache_Entry::CTextureCache_Entry(int _iTexture, int _iMipMap)
{
	MAUTOSTRIP(CTextureCache_Entry_ctor_int_int, MAUTOSTRIP_VOID);

	m_Texture = _iTexture;
	m_MipMap = _iMipMap;
};

bool CTextureCache_Entry::operator==(CTextureCache_Entry _e)
{
	MAUTOSTRIP(CTextureCache_Entry_operator_eq, false);

	if ((m_Texture == _e.GetTexture()) && (m_MipMap == _e.GetMipMap())) return true;
	return false;
};

CTextureCache_Entry CTextureCache_Entry::operator=(CTextureCache_Entry _e)
{
	MAUTOSTRIP(CTextureCache_Entry_operator_assign, *this);

	m_Texture = _e.GetTexture();
	m_MipMap = _e.GetMipMap();
	return *this;
};
#endif

// ----------------------------------------------------------------
//  CTextureContainer_Plain
// ----------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Plain, CReferenceCount);

IMPLEMENT_OPERATOR_NEW(CTextureContainer_Plain);

CTextureContainer_Plain::CTextureContainer_Plain()
{
	MAUTOSTRIP(CTextureContainer_Plain_ctor, MAUTOSTRIP_VOID);

//	m_Hash.Create(0, false);

}

CTextureContainer_Plain::~CTextureContainer_Plain()
{
	MAUTOSTRIP(CTextureContainer_Plain_dtor, MAUTOSTRIP_VOID);

//	if ((g_pOS) && (g_pOS->m_spTC))
	Clear();
}

int CTextureContainer_Plain::AddTexture(spCTexture _spTxt)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddTexture_2, 0);

//	DestroyHash();

	int iLocal = m_lspTextures.Add(_spTxt);
	int TxtID = -1;
	if( iLocal > 0 )
	{
#ifdef USE_HASHED_TEXTURENAME
		if( m_lspTextures[iLocal-1]->m_NameID == m_lspTextures[iLocal]->m_NameID )
			TxtID	= m_lspTextures[iLocal-1]->m_TextureID;
#else
		if( !CStrBase::stricmp( m_lspTextures[iLocal-1]->m_Name, m_lspTextures[iLocal]->m_Name ) )
			TxtID	= m_lspTextures[iLocal-1]->m_TextureID;
#endif
	}
	if(TxtID == -1)
	{
#ifdef USE_HASHED_TEXTURENAME
		TxtID = m_pTC->AllocID(m_iTextureClass, iLocal, m_lspTextures[iLocal]->m_NameID);
#else
		TxtID = m_pTC->AllocID(m_iTextureClass, iLocal, &m_lspTextures[iLocal]->m_Name[0]);
#endif	
	}
	m_lspTextures[iLocal]->m_TextureID = TxtID;

//	InsertHash(iLocal);

	return iLocal;
}

#ifndef PLATFORM_CONSOLE

int CTextureContainer_Plain::AddTexture(spCImage _spImg, const CTC_TextureProperties& _Properties, int _nMipmaps, CStr _Name)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddTexture_1, 0);

//	DestroyHash();

	TProfileDef(T); 
	TStartProfile(T);
	spCTexture spTxt = MNew(CTexture);
	if (spTxt == NULL) MemError("AddTexture");
	spTxt->Create(_spImg, (_nMipmaps > 0) ? _nMipmaps : CTexture::GetMipMapLevels(_spImg->GetWidth(), _spImg->GetHeight()) );
	spTxt->m_Properties = _Properties;
	int iLocal = m_lspTextures.Add(spTxt);
//	if (_Name.Len() > 15) Error("AddTexture", "Only 15 characters allowed in texture names.");
//	m_lspTextures[iLocal]->m_Name = _Name;
	int TxtID = -1;
#ifdef USE_HASHED_TEXTURENAME
	m_lspTextures[iLocal]->m_NameID = StringToHash(_Name);
	if(iLocal > 0)
	{
		if(m_lspTextures[iLocal-1]->m_NameID == m_lspTextures[iLocal]->m_NameID)
			TxtID = m_lspTextures[iLocal-1]->m_TextureID;
	}
	if(TxtID == -1 )
		TxtID = m_pTC->AllocID(m_iTextureClass, iLocal, m_lspTextures[iLocal]->m_NameID);
#else
	if (_Name != "")
	{
		if (_Name.Len() > 31) Error("Read", "Too long texture-name.");
		strcpy(m_lspTextures[iLocal]->m_Name, (char*)_Name);
	}
	if(iLocal > 0)
	{
		if( !CStrBase::stricmp(m_lspTextures[iLocal - 1]->m_Name, m_lspTextures[iLocal]->m_Name) )
			TxtID = m_lspTextures[iLocal-1]->m_TextureID;
	}
	if(TxtID == -1 )
		TxtID = m_pTC->AllocID(m_iTextureClass, iLocal, &m_lspTextures[iLocal]->m_Name[0]);
#endif
	m_lspTextures[iLocal]->m_TextureID = TxtID;

//	InsertHash(iLocal);

	TStopProfile(T); //LogFile(T_String("CTC_TC_Plain::AddTexture ", T) + "  " + _spImg->IDString());
	return iLocal;
};


int CTextureContainer_Plain::AddTexture(CStr _FileName, const CTC_TextureProperties& _Properties, CStr _Name)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddTexture_3, 0);

	spCImage spImg = MNew(CImage);
	if (spImg == NULL) MemError("AddTexture");
	spImg->Read(_FileName, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM);
	return AddTexture(spImg, _Properties, 1, _Name);
}

void CTextureContainer_Plain::SetTexture(int _iLocal, spCImage _spImg, const CTC_TextureProperties& _Properties, int _ConvertType, void* _pConvertParam)
{
	if (!m_lspTextures.ValidPos(_iLocal))
		Error("SetTexture", "Invalid position");

	CTexture* pPrev = m_lspTextures[_iLocal];
	if (!pPrev)
		Error("SetTexture", "Missing texture");

	spCTexture spTxt = MNew(CTexture);
	if (!spTxt)
		MemError("AddTexture");

	spTxt->m_Properties = _Properties;
	CreateTextureImage(spTxt, _spImg, _ConvertType, _pConvertParam);
	spTxt->m_TextureID = pPrev->m_TextureID;
#ifdef USE_HASHED_TEXTURENAME
	spTxt->m_NameID = pPrev->m_NameID;
#else
	strcpy(spTxt->m_Name, pPrev->m_Name);
#endif	
	m_lspTextures[_iLocal] = spTxt;
	m_pTC->MakeDirty(spTxt->m_TextureID);
}

#endif

#ifndef PLATFORM_CONSOLE

// -------------------------------------------------------------------
static fp32 GetHeight(CImage* _pImg, int _x, int _y)
{
	MAUTOSTRIP(GetHeight_CImage_int_int, 0.0f);

	_x &= _pImg->GetWidth() - 1;
	_y &= _pImg->GetHeight() - 1;

	CPixel32 Pixel = _pImg->GetPixel(_pImg->GetClipRect(), CPnt(_x, _y));
	return Pixel.GetGray();
}

static CVec3Dfp32 GetNormal(CImage* _pImg, int _x, int _y, fp32 _Scale)
{
	MAUTOSTRIP(GetNormal_CImage_int_int_fp32, CVec3Dfp32());

	fp32 H00 = GetHeight(_pImg, _x + 0, _y + 0);
	fp32 H10 = GetHeight(_pImg, _x + 1, _y + 0);
	fp32 H01 = GetHeight(_pImg, _x + 0, _y + 1);
	fp32 H11 = GetHeight(_pImg, _x + 1, _y + 1);

	fp32 HeightScale = 1.0f / 255.0f * _Scale;
//	HeightScale = 0;

	CVec3Dfp32 TangX(1, 0, ((H10 - H00) + (H11 - H01))*0.5f*HeightScale);
	CVec3Dfp32 TangY(0, 1, ((H01 - H00) + (H11 - H10))*0.5f*HeightScale);

	CVec3Dfp32 Normal = TangX / TangY;
	Normal[2] *= 1;
	Normal.Normalize();

	Normal += 1.0f;
	Normal *= 127.0f;

	return Normal;
}


static void CreateNormalMap(CImage* _pSrc, CImage* _pDst, fp32 _Scale, bool _bConstR)
{
	MAUTOSTRIP(CreateNormalMap, MAUTOSTRIP_VOID);

	_pDst->Create(_pSrc->GetWidth(), _pSrc->GetHeight(), IMAGE_FORMAT_BGR8, IMAGE_MEM_IMAGE);

	for(int x = 0; x < _pSrc->GetWidth(); x++)
		for(int y = 0; y < _pSrc->GetHeight(); y++)
		{
			CVec3Dfp32 N = GetNormal(_pSrc, x, y, _Scale);
			if (_bConstR)
			{
				N -= 128.0f;
				N[0] /= (N[2]/255.0f);
				N[1] /= (N[2]/255.0f);
				if (N[0] < -128.0f) N[0] = -128.0f; else if (N[0] > 127.0f) N[0] = 127.0f;
				if (N[1] < -128.0f) N[1] = -128.0f; else if (N[1] > 127.0f) N[1] = 127.0f;
				N += 128.0f;
				N[2] = 255.0f;
			}
			CPixel32 Pixel(RoundToInt(N[2]), RoundToInt(N[1]), RoundToInt(N[0]), 255);
			_pDst->SetPixel(_pDst->GetClipRect(), CPnt(x,y), Pixel);
		}
}

static void CreateTSVectorMap(CImage* _pSrc, CImage* _pDst)
{
	MAUTOSTRIP(CreateNormalMap, MAUTOSTRIP_VOID);

	spCImage spSrc = _pSrc->Convert(IMAGE_FORMAT_I8);

	int bAlpha = (_pSrc->GetFormat() & IMAGE_FORMAT_ALPHA);
	int Format = (bAlpha) ? IMAGE_FORMAT_BGRA8 : IMAGE_FORMAT_BGR8;

	_pDst->Create(_pSrc->GetWidth(), _pSrc->GetHeight(), Format, IMAGE_MEM_IMAGE);

	const uint8* pSrc = (const uint8*) spSrc->Lock();
	int SrcModulo = spSrc->GetModulo();

	for(int x = 0; x < _pSrc->GetWidth(); x++)
		for(int y = 0; y < _pSrc->GetHeight(); y++)
		{
			fp32 Angle = fp32(pSrc[x + y*SrcModulo]) / 256.0f;
			fp32 TangU = M_Cos(Angle*2.0f*_PI);
			fp32 TangV = M_Sin(Angle*2.0f*_PI);

			int Alpha = (bAlpha) ? _pSrc->GetPixel(_pSrc->GetClipRect(), CPnt(x,y)).GetA() : 255;

			CPixel32 Pixel(128, RoundToInt(TangU*127.5f+127.5f), RoundToInt(TangV*127.5f+127.5f), Alpha);
			_pDst->SetPixel(_pDst->GetClipRect(), CPnt(x,y), Pixel);
		}

	spSrc->Unlock();
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Get pixel by Linear Interpolation	

Parameters:		
_pImg:		Source image
_TexX:		X-value, in pixels
_TexY:		Y-value, in pixels

Returns:	Interpolated pixel

Comments:	Function used for cylinder map faking
\*____________________________________________________________________*/ 
CPixel32 GetPixel(CImage * _pImg,fp32 _TexX,fp32 _TexY)
{
	_TexY = Clamp(_TexY,0.0f,(fp32)(_pImg->GetHeight()-1));

	fp32	IntX,IntY,FracX,FracY;
	IntX = Floor(_TexX);
	IntY = Floor(_TexY);
	FracX = _TexX - IntX;
	FracY = _TexY - IntY;

	CVec4Dfp32 Pixel[4];
	Pixel[0] = _pImg->GetPixel3f(_pImg->GetClipRect(),CPnt(IntX,IntY));
	Pixel[1] = _pImg->GetPixel3f(_pImg->GetClipRect(),CPnt(((int)IntX + 1)%_pImg->GetWidth(),IntY));
	Pixel[2] = _pImg->GetPixel3f(_pImg->GetClipRect(),CPnt(IntX,(int)IntY + 1));
	Pixel[3] = _pImg->GetPixel3f(_pImg->GetClipRect(),CPnt(((int)IntX + 1)%_pImg->GetWidth(),(int)IntY + 1));

	CVec4Dfp32 FinalPixel = ((Pixel[1]*FracX + Pixel[0]*(1.0f-FracX)) * (1.0f - FracY) + 
						    (Pixel[3]*FracX + Pixel[2]*(1.0f-FracX)) * FracY) * 255.0f;
	return CPixel32(FinalPixel);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Get UV coordinates by sphere map

Parameters:		
	_Pos:	Point to sample

Returns:	UV coordinates of sphere map
\*____________________________________________________________________*/ 
CVec2Dfp32 SphereMap(const CVec3Dfp32 &_Pos)
{
	const fp32 PI = 3.14159265f;

	fp32 Radius = _Pos.Length();
	CVec2Dfp32 Ret;

	fp32 Rto = _Pos.k[2] / Radius;
	Ret.k[1] = AlmostEqual(Rto,0.0f,0.001f) ? ((_Pos.k[2] < 0) ? -0.5f : 0.5f) :
		M_ACos(Rto) / PI;
	
	Rto = Radius * M_Sin(PI*Ret.k[1]);
	fp32 Ang = AlmostEqual(Rto,0.0f,0.001f) ? PI :
		M_ACos(_Pos.k[0] / Rto);

	if( _Pos.k[1] >= 0 )
		Ret.k[0] = Ang / (2 * PI);
	else
	{
		Ret.k[0] = (PI * 2 - Ang) / (2 * PI);
	}

	return Ret;
}

// -------------------------------------------------------------------
void CTextureContainer_Plain::CreateTextureImage(spCTexture _spTxt, spCImage _spImg, int _ConvertType, void * _pConvertParam)
{
	MAUTOSTRIP(CTextureContainer_Plain_CreateTexture, MAUTOSTRIP_VOID);

	int DestFmt = -1;
	int ImageConvertType = -1;

	switch(_ConvertType)
	{
	case CTC_CONVERT_SELECTNORMALMAPFORMAT:
		{
			if (_spTxt->m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS)
			{
				if (_spTxt->m_Properties.m_Flags & CTC_TEXTUREFLAGS_PALETTE)
				{
					DestFmt = IMAGE_FORMAT_CLUT8;
				}
				else
				{
					if (_spTxt->m_Properties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY)
						DestFmt = IMAGE_FORMAT_BGRA8;
					else
						DestFmt = IMAGE_FORMAT_BGRA4;
				}
			}
		}
		break;

	case CTC_CONVERT_AFROMRGB :
		{
			spCImage spImg2 = MNew(CImage);
			if (spImg2 == NULL) MemError("AddTexture");
			spImg2->Create(_spImg->GetWidth(), _spImg->GetHeight(), IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
			CImage::Convert(_spImg, spImg2, IMAGE_CONVERT_RGBA_AFROMRGB);
			
			_spImg = spImg2;
			spImg2 = NULL;
		}
		break;
		
	case CTC_CONVERT_BUMP :
	case CTC_CONVERT_BUMPENV :
		{
			spCImage spImg2 = MNew(CImage);
			if (spImg2 == NULL) MemError("AddTexture");
			_spImg = _spImg->Convert(IMAGE_FORMAT_I8);
			fp32 Scale = (_pConvertParam) ? *((fp32*)_pConvertParam) : 1.0f;
			CreateNormalMap(_spImg, spImg2, Scale, _ConvertType == CTC_CONVERT_BUMPENV);
			_spImg = spImg2;
		}
		break;
		
	case CTC_CONVERT_ALPHA :
		{
			spCImage spImg2 = MNew(CImage);
			if (spImg2 == NULL) MemError("AddTexture");
			spImg2->Create(_spImg->GetWidth(), _spImg->GetHeight(), IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
			CImage::Convert(_spImg, spImg2, IMAGE_CONVERT_RGBA_AFROMRGB);
			spCImage spImg3 = spImg2->Convert(IMAGE_FORMAT_A8, IMAGE_CONVERT_RGBA);
			_spImg = spImg3;
		}
		break;

	//When doing a cylinder image, also do the default processing
	case CTC_CONVERT_CUBEFROMCYLINDER:
		{
			CTEX_CubeFromCylinderSettings *pSettings = (CTEX_CubeFromCylinderSettings*)_pConvertParam;
			if( pSettings->m_iSide < 4 )
			{
				//Determine width and height of new image- so as not to loose any pixel information
				spCImage spNewImg = MNew(CImage);
				fp32 Diameter = ((fp32)_spImg->GetWidth()+4.0f) / 3.1415926535f;
				fp32 Radius = Diameter / 2.0f;
				uint16 Width = (uint16)Ceil( Diameter );
				
				fp32 StartX = (fp32)((pSettings->m_iSide * _spImg->GetWidth()) / 4);

				fp32 DepthInc = Radius * (M_Sqrt(2.0f)-1.0f);
				uint16 Height = _spImg->GetHeight();
				uint16 EHeight = (uint16)(((fp32)Height / Radius) * DepthInc);
				Height += EHeight;
				EHeight /= 2;

				//Make sure wanted aspect ratio and current match
				bool bResize = true;
				fp32 Ratio = (fp32)Width / (fp32)Height;
				if( (pSettings->m_Width == Width) && (pSettings->m_Height == Height) )
					bResize = false;
				else if( (pSettings->m_Width != 0) && (pSettings->m_Height != 0) )
				{
					fp32 NewRatio = (fp32)pSettings->m_Width / (fp32)pSettings->m_Height;
					if( NewRatio < Ratio )
					{
						Height = (fp32)Height * (Ratio / NewRatio);
						EHeight = (Height - _spImg->GetHeight()) / 2;
					}
				}
				else if(pSettings->m_Width != 0)
					pSettings->m_Height = (fp32)pSettings->m_Width / Ratio;
				else if(pSettings->m_Height != 0)
					pSettings->m_Width = (fp32)pSettings->m_Height * Ratio;
				else
				{
					pSettings->m_Height = Height;
					pSettings->m_Width = Width;
					bResize = false;
				}

				spNewImg->Create(Width,Height,IMAGE_FORMAT_BGRA8,IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM);

				CVec3Dfp32 Origin(0.0f,pSettings->m_ViewHeight * (fp32)_spImg->GetHeight(),0.0f);

				TArray<CPixel32> & lLow = pSettings->m_lLow[pSettings->m_iSide];
				TArray<CPixel32> & lHigh = pSettings->m_lHigh[pSettings->m_iSide];
				lLow.SetLen(pSettings->m_Width);
				lHigh.SetLen(pSettings->m_Width);

				//Copy needed pixels
				int i;
				for(i = 0;i < Width;i++)
				{
					CVec2Dfp32 Vec2D(-(fp32)Width/2.0f + (fp32)i,Radius);
					CVec2Dfp32 Vec2Dunit = Vec2D;
					Vec2Dunit.Normalize();
					fp32 Angle = M_ACos(Vec2Dunit * CVec2Dfp32(-1.0f/M_Sqrt(2.0f),1.0f/M_Sqrt(2.0f))) / (3.1415926535f * 0.5f);
					fp32 PosX = StartX + Angle*_spImg->GetWidth()/4;
					
					for(int j = 0;j < Height;j++)
					{
						CVec3Dfp32 Target(Vec2D.k[0],(fp32)(j-EHeight),Radius);

						CVec3Dfp32 Vec,Pos;
						Vec = Target - Origin;
						Vec = Vec.Normalize();
						Vec *= (Vec2Dunit.k[1] * Radius / Vec.k[2]);
						Pos = Vec + Origin;

						CPixel32 Pixel = GetPixel(_spImg,PosX,Pos.k[1]);
						spNewImg->SetPixel(spNewImg->GetClipRect(),CPnt(i,j),Pixel);
					}
				}

				//Resize image if needed
				if( bResize )
				{
					_spImg = MNew(CImage);
					spNewImg->Stretch(spNewImg,_spImg,pSettings->m_Width,pSettings->m_Height);
				}
				else
				{
					_spImg = spNewImg;
				}

				//Used for top/bottom generation
				for(i = 0;i < pSettings->m_Width;i++)
				{
					lLow[i] = _spImg->GetPixel(_spImg->GetClipRect(),CPnt(i,0));
					lHigh[i] = _spImg->GetPixel(_spImg->GetClipRect(),CPnt(i,pSettings->m_Height-1));
				}
			}
			else
			{
				// This is an *ugly* way of auto-generating top/bottom images
				// Should be reworked if used heavily.
				_spImg = MNew(CImage);
				_spImg->Create(pSettings->m_Width,pSettings->m_Width,IMAGE_FORMAT_BGRA8,IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM);
				uint16 Width = pSettings->m_Width;
				uint16 nRows = ((Width-1) / 2);
				int i;
				
				// Set borders to correct pixels
				TArray<CPixel32> *lPix = (pSettings->m_iSide == 5) ? pSettings->m_lHigh : pSettings->m_lLow;
				if( pSettings->m_iSide == 4 )
				{
					for(i = 0;i < Width;i++)
					{
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(Width-1-i,0),lPix[2][i]); // 0
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(0,i),lPix[3][i]); // 1
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(i,Width-1),lPix[0][i]); // 3
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(Width-1,Width-1-i),lPix[1][i]); // 2
					}
				}
				else
				{
					for(i = 0;i < Width;i++)
					{
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(i,0),lPix[0][i]); // 0
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(0,Width-1 - i),lPix[3][i]); // 1
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(Width-1 - i,Width-1),lPix[2][i]); // 3
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(Width-1,i),lPix[1][i]); // 2
					}
				}

				//Interpolate all other pixels
				for(i = 0;i < nRows;i++)
				{
					Width-=2;
					for(int j = 0;j < Width;j++)
					{
						fp32 Frac = (fp32)i / (2.0f * (fp32)(nRows-Abs(nRows - j - ((pSettings->m_Width-Width)/2))));
						CPixel32 Color = _spImg->GetPixel(_spImg->GetClipRect(),CPnt(i+j+1,0)) * (1.0f-Frac) +
							_spImg->GetPixel(_spImg->GetClipRect(),CPnt((i+j+1>pSettings->m_Width/2) ? pSettings->m_Width-1 : 0,
							nRows - Abs(nRows-(i+j+1)))) * Frac;
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(i+j+1,i+1),Color);
						Color = _spImg->GetPixel(_spImg->GetClipRect(),CPnt(i+j+1,pSettings->m_Width-1)) * (1.0f-Frac) +
							_spImg->GetPixel(_spImg->GetClipRect(),CPnt((i+j+1>pSettings->m_Width/2) ? pSettings->m_Width-1 : 0,
							pSettings->m_Width - (1+nRows - Abs(nRows-(i+j+1))))) * Frac;
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(i+j+1,pSettings->m_Width-2-i),Color);
						Color = _spImg->GetPixel(_spImg->GetClipRect(),CPnt(0,i+j+1)) * (1.0f-Frac) +
							_spImg->GetPixel(_spImg->GetClipRect(),CPnt(nRows - Abs(nRows-(i+j+1)),
							(i+j+1>pSettings->m_Width/2) ? pSettings->m_Width-1 : 0)) * Frac;
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(i+1,i+j+1),Color);
						Color = _spImg->GetPixel(_spImg->GetClipRect(),CPnt(pSettings->m_Width-1,i+j+1)) * (1.0f-Frac) +
							_spImg->GetPixel(_spImg->GetClipRect(),CPnt(pSettings->m_Width - (1+nRows - Abs(nRows-(i+j+1))),
							(i+j+1>pSettings->m_Width/2) ? pSettings->m_Width-1 : 0)) * Frac;
						_spImg->SetPixel(_spImg->GetClipRect(),CPnt(pSettings->m_Width-i-2,i+j+1),Color);
					}
				}
			}
		}

	//Also do default... but skip this in CubeFromCylinder
	case CTC_CONVERT_CUBEFROMSPHERE :
		{
			CTEX_CubeFromCylinderSettings *pSettings = (CTEX_CubeFromCylinderSettings*)_pConvertParam;

			if(_ConvertType == CTC_CONVERT_CUBEFROMSPHERE)
			{
				spCImage spNewImg = MNew(CImage);

				//Default to height of old image
				int Width = _spImg->GetHeight();
				CVec2Dfp32 Dim(fp32(_spImg->GetWidth()),fp32(_spImg->GetHeight()));
				if( pSettings->m_Height )
				{
					Width = pSettings->m_Height;
				}
				else if( pSettings->m_Width )
				{
					Width = pSettings->m_Width;
				}

				spNewImg->Create(Width,Width,IMAGE_FORMAT_BGRA8,IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM);

				fp32 HalfSide = fp32(Width) / 2.0f;
				CVec2Dfp32 UV(-HalfSide-0.5f,-HalfSide-0.5f);
				fp32 Inc = (fp32(Width) + 1.0f) / fp32(Width);

				//Do image...
				for(int i = 0;i < Width;i++)
				{
					CVec2Dfp32 Tmp = UV;

					for(int j = 0;j < Width;j++)
					{
						CVec2Dfp32 TexCoord;
						
						//Depending on side...
						if( pSettings->m_iSide == 0 ) TexCoord = SphereMap(CVec3Dfp32(Tmp.k[0],HalfSide,-Tmp.k[1]));
						if( pSettings->m_iSide == 1 ) TexCoord = SphereMap(CVec3Dfp32(HalfSide,-Tmp.k[0],-Tmp.k[1]));
						if( pSettings->m_iSide == 2 ) TexCoord = SphereMap(CVec3Dfp32(-Tmp.k[0],-HalfSide,-Tmp.k[1]));
						if( pSettings->m_iSide == 3 ) TexCoord = SphereMap(CVec3Dfp32(-HalfSide,Tmp.k[0],-Tmp.k[1]));
						if( pSettings->m_iSide == 4 ) TexCoord = SphereMap(CVec3Dfp32(Tmp.k[0],Tmp.k[1],HalfSide));
						if( pSettings->m_iSide == 5 ) TexCoord = SphereMap(CVec3Dfp32(Tmp.k[0],-Tmp.k[1],-HalfSide));

						//Mirror since we're looking at the inside
						TexCoord.k[0] = -TexCoord.k[0];

						//Finalize
						CPixel32 Color = _spImg->GetPixelUV_Bilinear(TexCoord);
						spNewImg->SetPixel(spNewImg->GetClipRect(),CPnt(i,j),Color);
						Tmp.k[1] += Inc;
					}
					UV.k[0] += Inc;
				}

				//Done!
				_spImg = spNewImg;
			}

			// Handle transformation of cubemaps - both cylinders and spheres!
			if( pSettings->m_bIsEnvMap )
			{
				int lImgTransform[6] =
				{
					IMAGE_TRANSFORM_ROTATECW | IMAGE_TRANSFORM_FLIPH,
						IMAGE_TRANSFORM_FLIPH,
						IMAGE_TRANSFORM_ROTATECW | IMAGE_TRANSFORM_FLIPV,
						IMAGE_TRANSFORM_FLIPV,
						IMAGE_TRANSFORM_ROTATECW | IMAGE_TRANSFORM_FLIPH,
						IMAGE_TRANSFORM_ROTATECW | IMAGE_TRANSFORM_FLIPH
				};
				_spImg = _spImg->Transform(lImgTransform[pSettings->m_iSide]);
			}
		}

	case CTC_CONVERT_DEFAULT :
		{
			if (_spTxt->m_Properties.m_Flags & CTC_TEXTUREFLAGS_PALETTE)
			{
				DestFmt = IMAGE_FORMAT_CLUT8;
				_spTxt->m_Properties.m_Flags |= CTC_TEXTUREFLAGS_NOCOMPRESS;
			}
			else if (_spTxt->m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS)
			{
				if (!(_spTxt->m_Properties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY))
				{
					if (_spImg->GetFormat() & IMAGE_FORMAT_ALPHA)
					{
						DestFmt = IMAGE_FORMAT_BGRA4;
					}
					else
					{
						DestFmt = IMAGE_FORMAT_BGR5;
					}
				}
			}
		}
		break;

	case CTC_CONVERT_GB2IA:
		{
			DestFmt = IMAGE_FORMAT_I8A8;
			ImageConvertType = IMAGE_CONVERT_RGB_GB2IA;
			break;
		}
	case CTC_CONVERT_ANISOTROPICDIRECTION :
		{
			spCImage spImg2 = MNew(CImage);
			if (spImg2 == NULL) MemError("AddTexture");
			CreateTSVectorMap(_spImg, spImg2);
			_spImg = spImg2;
			break;
		}
	case CTC_CONVERT_GB2GA:
		{
			DestFmt = IMAGE_FORMAT_BGRA8;
			ImageConvertType = IMAGE_CONVERT_RGB_GB2GA;
			break;
		}
	default :;
	}
	
	_spTxt->Create(_spImg, CTexture::GetMipMapLevels(_spImg->GetWidth(), _spImg->GetHeight()), DestFmt, ImageConvertType);
}

int CTextureContainer_Plain::AddTexture(spCImage _spImg, const CTC_TextureProperties& _Properties, CStr _Name, int _ConvertType, void* _pConvertParam)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddTexture_4, 0);

	spCTexture spTxt = MNew(CTexture);
	if (!spTxt)
		MemError("AddTexture");

	spTxt->m_Properties = _Properties;
	CreateTextureImage(spTxt, _spImg, _ConvertType, _pConvertParam);
#ifdef USE_HASHED_TEXTURENAME
	spTxt->m_NameID = StringToHash(_Name);
#else
	if (_Name != "")
	{
		if (_Name.Len() > 31) Error("Read", "Too long texture-name.");
		strcpy(spTxt->m_Name, (char*)_Name);
	}
#endif	
	return AddTexture(spTxt);
}

static spCImage ReadImage(CStr _FileName)
{
	if(!CDiskUtil::FileExists(_FileName))
		FileError_static("ReadImage", _FileName, 2);	// File not found error

	spCImage spImg = MNew(CImage);
	if (spImg == NULL) Error_static("::ReadImage", "Out of memory.");
	spImg->Read(_FileName, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM);
	return spImg;
}

int CTextureContainer_Plain::AddTexture(CStr _FileName, const CTC_TextureProperties& _Properties, CStr _Name, int _ConvertType, void* _pConvertParam)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddTexture_5, 0);

	spCImage spImg = ReadImage(_FileName);
	return AddTexture(spImg, _Properties, _Name, _ConvertType, _pConvertParam);
}

void CTextureContainer_Plain::Add(CTextureContainer_Plain* _pSrcXTC)
{
	int nTex = _pSrcXTC->m_lspTextures.Len();
	m_lspTextures.SetGrow(nTex + m_lspTextures.Len());
	for (int i = 0; i < nTex; ++i)
	{
		spCTexture spTexture = _pSrcXTC->GetTextureMap(i, CTC_TEXTUREVERSION_ANY, true);
		m_lspTextures.Add(spTexture->Duplicate());
	}
}

void CTextureContainer_Plain::AddFiltered(CTextureContainer_Plain *_pSrcXTC, TArray<CStr> *_pAllowed)
{	
	CStringHashConst Hash;
	Hash.Create(_pAllowed->Len());

	for (int i = 0; i < _pAllowed->Len(); ++i)
	{
		Hash.Insert(i, (*_pAllowed)[i]);
	}

	TArray<spCTexture> lspTextures;
	lspTextures.SetLen(_pSrcXTC->m_lspTextures.Len());
	int iCurrent = 0;

	for (int i = 0; i < lspTextures.Len(); ++i)
	{
		int iHash = Hash.GetIndex(_pSrcXTC->m_lspTextures[i]->m_Name);

		if (iHash >= 0)
		{
			lspTextures[iCurrent] = _pSrcXTC->m_lspTextures[i];
//			lspTextures[iCurrent]->m_TextureID = m_pTC->AllocID(m_iTextureClass, iCurrent, _pSrcXTC->m_lspTextures[i]->m_Name);
			++iCurrent;
		}
	}
	lspTextures.SetLen(iCurrent);
	m_lspTextures = lspTextures;
    
}

#endif

void CTextureContainer_Plain::AddFromXTC(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddFromXTC, MAUTOSTRIP_VOID);
	
	int iFirstID = m_lspTextures.Len();

	m_ContainerName = CStr(_pName).GetFilename();

	CDataFile DFile;
	DFile.Open(_pName);
	
	{
		DFile.PushPosition();
		if(DFile.GetNext("SOURCE"))
			m_Source.Read(DFile.GetFile());
		DFile.PopPosition();
	}

	if (!DFile.GetNext("IMAGELIST")) Error("AddFromXTC", "Invalid XTC. (No IMAGELIST)");
	if (!DFile.GetSubDir()) Error("AddFromXTC", "Invalid XTC. (IMAGELIST was not a directory.)");
	AddFromImageList(&DFile);
	DFile.GetParent();
	if(DFile.GetNext("COMPILECHECKSUMS"))
	{
		uint OrgLen = m_lTextureCompileChecksums.Len();
		uint OrgSize = m_lTextureCompileChecksums.ListSize();
		m_lTextureCompileChecksums.SetLen(OrgLen + DFile.GetUserData());
		if((m_lTextureCompileChecksums.ListSize() - OrgSize) != DFile.GetUserData2())
			Error("AddFromXTC", "Invalid XTC. (Size of TextureCompileChecksums is invalid)");

		DFile.GetSubDir();
		DFile.GetFile()->Read(m_lTextureCompileChecksums.GetBasePtr() + OrgLen, DFile.GetUserData2());
		DFile.GetParent();
	}
	DFile.Close();
	
/*	{
		CCFile File;
		File.Open("C:\\textures.txt", CFILE_WRITE | CFILE_APPEND);

		for(int i = iFirstID; i < m_lspTextures.Len(); i++)
		{
			if(m_lspTextures[i] && m_lspTextures[i]->m_Properties.m_iPicMip == 0)
				File.Writeln(CStrF("%-20s %s", m_lspTextures[i]->m_Name, _pName));
		}
		File.Close();
	}*/
}

void CTextureContainer_Plain::AddFromXTC(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddFromXTC, MAUTOSTRIP_VOID);
	
	m_ContainerName = _pDFile->GetFile()->GetFileName().GetFilename();
	int iFirstID = m_lspTextures.Len();

	{
		_pDFile->PushPosition();
		if(_pDFile->GetNext("SOURCE"))
			m_Source.Read(_pDFile->GetFile());
		_pDFile->PopPosition();
	}

	if (!_pDFile->GetNext("IMAGELIST")) Error("AddFromXTC", "Invalid XTC. (No IMAGELIST)");
	if (!_pDFile->GetSubDir()) Error("AddFromXTC", "Invalid XTC. (IMAGELIST was not a directory.)");
	AddFromImageList(_pDFile);
	{
		_pDFile->PushPosition();
		if(_pDFile->GetNext("COMPILECHECKSUMS"))
		{
			m_lTextureCompileChecksums.SetLen(_pDFile->GetUserData());
			if(m_lTextureCompileChecksums.ListSize() != _pDFile->GetUserData2())
				Error("AddFromXTC", "Invalid XTC. (Size of TextureCompileChecksums is invalid)");

			_pDFile->GetSubDir();
			_pDFile->GetFile()->Read(m_lTextureCompileChecksums.GetBasePtr(), _pDFile->GetUserData2());
			_pDFile->GetParent();
		}
		_pDFile->PopPosition();
	}
	
/*	{
		CCFile File;
		File.Open("C:\\textures.txt", CFILE_WRITE | CFILE_APPEND);

		for(int i = iFirstID; i < m_lspTextures.Len(); i++)
		{
			if(m_lspTextures[i] && m_lspTextures[i]->m_Properties.m_iPicMip == 0)
				File.Writeln(CStrF("%-20s %s", m_lspTextures[i]->m_Name, _pName));
		}
		File.Close();
	}*/
}

#ifndef PLATFORM_CONSOLE

int CTextureContainer_Plain::AddFromScriptLine(CStr s, CStr _ScriptPath)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddFromScriptLine, 0);

	CStr Name = s.GetStrSep(",");
	Name = Name.GetBounded(CStr('"', 1)).UpperCase();
	CStr TxtName = s.GetStrSep(",");
	TxtName = TxtName.GetBounded(CStr('"', 1)).UpperCase();

	int Flags = 0;
	if (s != (const char *)"")
	{
		Flags = s.GetStrSep(",").Val_int();
	}

/*	if (s != "")
	{
		int x = s.GetStrSep(",").Val_int();
		int y = s.GetStrSep(",").Val_int();
		Pos = CPnt(x, y);

		int w = s.GetStrSep(",").Val_int();
		int h = s.GetStrSep(",").Val_int();
		Size = CPnt(w, h);
	}*/

	if (TxtName.GetDevice() == (const char *)"") TxtName = _ScriptPath + TxtName;
	LogFile(Name + ", " + TxtName);

	spCImage spImg = MNew(CImage);
	if (spImg == NULL) MemError("AddTexture");
	spImg->Read(TxtName, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM);

	if (Flags & 1)
	{
		spCImage spImg2 = MNew(CImage);
		if (spImg2 == NULL) MemError("AddTexture");
		spImg2->Create(spImg->GetWidth(), spImg->GetHeight(), IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
		CImage::Convert(spImg, spImg2, IMAGE_CONVERT_RGBA_AFROMRGB);

		spImg = spImg2;
		spImg2 = NULL;
	}

	spCTexture spTxt = MNew(CTexture);
	spTxt->Create(spImg, CTexture::GetMipMapLevels(spImg->GetWidth(), spImg->GetHeight())/*, IMAGE_FORMAT_CLUT8*/);
//	spTxt->m_Name = Name;
#ifdef USE_HASHED_TEXTURENAME
	spTxt->m_NameID = StringToHash(Name);
#else
	if (Name != "")
	{
		if (Name.Len() > 31) Error("Read", "Too long texture-name.");
		strcpy(spTxt->m_Name, (char*)Name);
	}
#endif	
	return AddTexture(spTxt);			
}

int CTextureContainer_Plain::AddFromKeys(CKeyContainer* _pKeys, CStr _BasePath)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddFromKeys, 0);

	CKeyContainer* pKC = _pKeys;

	int iTxt = -1;

	int nLOD = 0;

	int ConvertType = CTC_CONVERT_NONE;
	fp32 BumpScale = 1.0f;
	bool bAutoCrop = false;

	CStr TxtName;
	CStr TxtPath;
	CTC_TextureProperties Properties;
	for(int i = 0; i < pKC->GetnKeys(); i++)
	{
		CStr Key = pKC->GetKeyName(i);
		CStr Value = pKC->GetKeyValue(i);
		if (Key == (const char *)"NAME")
			TxtName = Value.UpperCase();
		else if (Key == (const char *)"PATH")
		{
			if (Value.GetDevice() == (const char *)"")
				TxtPath = _BasePath + Value;
			else
				TxtPath = Value;
		}
		else if (Key == (const char*)"FLAGS")
		{
			Properties.m_Flags = Value.TranslateFlags(CTC_TextureProperties::ms_TxtPropFlagsTranslate);
		}
		else if (Key == (const char*)"MAGFILTER")
		{
			Properties.m_MagFilter = Value.TranslateInt(CTC_TextureProperties::ms_TxtPropFilterTranslate);
		}
		else if (Key == (const char*)"MINFILTER")
		{
			Properties.m_MinFilter = Value.TranslateInt(CTC_TextureProperties::ms_TxtPropFilterTranslate);
		}
		else if (Key == (const char*)"MIPFILTER")
		{
			Properties.m_MIPFilter = Value.TranslateInt(CTC_TextureProperties::ms_TxtPropMIPTranslate);
		}
	#ifndef USE_PACKED_TEXTUREPROPERTIES
		else if (Key == (const char*)"MIPMAPLODBIAS")
		{
			Properties.m_MIPMapLODBias = Value.Val_int();
		}
		else if (Key == (const char*)"ANISOTROPY")
		{
			Properties.m_Anisotropy = Value.Val_int();
		}
	#endif
		else if (Key == (const char*)"PICMIP")
		{
			Properties.m_iPicMipGroup = Max(0, Min(Value.Val_int(), 16));
		}
		else if (Key == (const char*)"PICMIPOFFSET" || Key == (const char*)"PICMIPOFS")
		{
			Properties.m_PicMipOffset = Value.Val_int();
		}
		else if (Key == "GENLOD")
		{
			nLOD = Value.Val_int();
		}
		else if (Key == "CONVERT")
		{
			ConvertType = Value.TranslateInt(CTC_TextureProperties::ms_TxtConvertTranslate);
		}
		else if (Key == "BUMPSCALE")
		{
			BumpScale = Value.Val_fp64();
		}
		else if (Key == "AUTOCROP") //AR-ADD
		{
			bAutoCrop = (Value.CompareNoCase("true") == 0);
		}
	}

	if (ConvertType == CTC_CONVERT_BUMP || ConvertType == CTC_CONVERT_BUMPENV)
		Properties.m_Flags |= CTC_TEXTUREFLAGS_NORMALMAP;

	if ((TxtName == (const char *)"") || (TxtPath == (const char *)""))
	{
		LogFile(CStrF("WARNING: Ignoring texture: NAME '%s', PATH: '%s'", 
			(char*) TxtName,
			(char*) TxtPath));
	}
	else
	{
		if (CDiskUtil::FileExists(TxtPath))
		{
			LogFile(CStrF("Adding texture %s (%s)", TxtName.Str(), TxtPath.Str()));
			try
			{
				if (nLOD)
				{
					iTxt = AddTexture(TxtPath, Properties, TxtName + "L0", ConvertType, &BumpScale);
					if (nLOD > GetTextureMap(iTxt, CTC_TEXTUREVERSION_ANY)->m_nMipmaps)
						Error("AddFromKeys", "Invalid GENLOD count.");
					for(int i = 1; i < nLOD; i++)
					{
						int iLodTxt = AddTexture(GetTextureMap(iTxt, CTC_TEXTUREVERSION_ANY)->m_lspMaps[i], Properties, -1, TxtName + CStrF("L%d", i) );
					}
				}
				else
				{
					iTxt = AddTexture(TxtPath, Properties, TxtName, ConvertType, &BumpScale);

					if (bAutoCrop)
					{
						//AR-NOTE: This is a late Enclave/GC hack to prevent texture memory waste for GUI icons.
						//         It will only work for rgba32 textures with no mipmaps.
						//         The code will always crop around center of the image. 
						//         This is to easily adjust for the cropping in game code.
						CImage* pImg = GetTexture(iTxt, 0, CTC_TEXTUREVERSION_RAW);
						if (pImg && (pImg->GetFormat() == IMAGE_FORMAT_BGRA8))
						{
							// calculate size needed.
							CClipRect cr_0 = pImg->GetClipRect();
							int w = pImg->GetWidth();
							int h = pImg->GetHeight();
							int crop_w = 0, crop_h = 0;
							for (int y=0; y<h; y++)
							{
								for (int x=0; x<w; x++)
								{
									CPixel32 c = pImg->GetPixel(cr_0, CPnt(x,y));
									if (c.A() != 0)
									{
										crop_w = Max(crop_w, 2*Abs(x-w/2));
										crop_h = Max(crop_h, 2*Abs(y-h/2));
									}
								}
							}

							// create cropped image.
							if (crop_w > 0 && crop_h > 0)
							{
								crop_w = 1 << Log2(crop_w*2-1);
								crop_h = 1 << Log2(crop_h*2-1);
								M_ASSERT(crop_w <= w && crop_h <= h, "!");

								if (crop_w < w || crop_h < h)
								{
									LogFile(CStrF("  Auto-cropping from size (%d x %d) to (%d x %d)", w, h, crop_w, crop_h));
									spCImage spCopy = MNew(CImage);
									pImg->Duplicate(spCopy);
									pImg->Create(crop_w, crop_h, pImg->GetFormat(), pImg->GetMemModel());
									//pImg->Blt(CClipRect(CRct(0, 0, crop_w, crop_h), CPnt((w-crop_w)/2, (h-crop_h)/2)), *spCopy, 0, CPnt(0,0));
									// never got Blt() to work, so here come putpixel-loop :(
									cr_0 = spCopy->GetClipRect();
									CClipRect cr_1 = pImg->GetClipRect();
									for (int y=0; y<crop_h; y++)
										for (int x=0; x<crop_w; x++)
											pImg->SetPixel(cr_1, CPnt(x,y), spCopy->GetPixel(cr_0, CPnt(x+(w-crop_w)/2, y+(h-crop_h)/2)));
								}
							}
							else
								LogFile("WARNING: No visible pixels in image!");
						}
					}
				}
			}
			catch(CCException)
			{
				LogFile(CStrF("WARNING: Failed to add texture: NAME '%s', PATH: '%s'", 
					(char*) TxtName,
					(char*) TxtPath));
	//				while(CCException::ErrorAvail()) LogFile(CCException::ErrorMsg());
			}
		}
		else
			LogFile(CStrF("WARNING: Texture not found: NAME '%s', PATH: '%s'", 
					(char*) TxtName,
					(char*) TxtPath));
	}

	return iTxt;
}

int CTextureContainer_Plain::AddFromScript(CStr _FileName)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddFromScript, 0);

	CKeyContainerNode Root;
	Root.ReadFromScript(_FileName);

	int iFirstTxt = m_lspTextures.Len();

	CStr Path = _FileName.GetPath();

	{
		CKeyContainer* pKC = Root.GetKeys();
		for(int i = 0; i < pKC->GetnKeys(); i++)
		{
			CStr Key = pKC->GetKeyName(i);
			CStr Value = pKC->GetKeyValue(i);
			if (Key == (const char *)"BASEDIR")
				if (Value.GetDevice() != (const char *)"")
					Path = Value;
				else
					Path = Path + Value;
		}
	}

	for(int iChild = 0; iChild < Root.GetNumChildren(); iChild++)
	{
		CKeyContainerNode* pChild = Root.GetChild(iChild);
		AddFromKeys(pChild->GetKeys(), Path);
	}

	return iFirstTxt;
}

void CTextureContainer_Plain::WriteImageList(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTextureContainer_Plain_WriteImageList, MAUTOSTRIP_VOID);

	CCFile* pFile = _pDFile->GetFile();

	// Write palettes
	if (m_lspPalettes.Len() > 0)
	{
		_pDFile->BeginEntry("PALETTES");
		_pDFile->EndEntry(m_lspPalettes.Len());
		_pDFile->BeginSubDir();

		for(int iPal = 0; iPal < m_lspPalettes.Len(); iPal++)
		{
			_pDFile->BeginEntry("PALETTE");
			pFile->Write(m_lspPalettes[iPal]->GetPalettePtr(), 4*256);
			_pDFile->EndEntry(256);
		}
		_pDFile->EndSubDir();
	}

	for(int iTxt = 0; iTxt < m_lspTextures.Len(); iTxt++)
	{
		_pDFile->BeginEntry("IMAGE");
		_pDFile->EndEntry(0);

		_pDFile->BeginSubDir();
		m_lspTextures[iTxt]->Write(_pDFile);
		_pDFile->EndSubDir();

//CImage* pImg = m_lspTextures[iTxt]->m_lspMaps[0];
//pImg->Write(pImg->GetClipRect().clip, CStrF("E:\\TEST\\TXT%d.TGA", iTxt));
	}
}

void CTextureContainer_Plain::WriteImageList2(CDataFile* _pDFile, int32 *_PicMip)
{
	MAUTOSTRIP(CTextureContainer_Plain_WriteImageList2, MAUTOSTRIP_VOID);

	CCFile* pFile = _pDFile->GetFile();

	// Write palettes
	if (m_lspPalettes.Len() > 0)
	{
		_pDFile->BeginEntry("PALETTES");

		for(int iPal = 0; iPal < m_lspPalettes.Len(); iPal++)
		{
			pFile->Write(m_lspPalettes[iPal]->GetPalettePtr(), 4*256);
		}

		_pDFile->EndEntry(m_lspPalettes.Len());
	}

	_pDFile->BeginEntry("TEXTURES");
	for(int iTxt = 0; iTxt < m_lspTextures.Len(); iTxt++)
	{
		m_lspTextures[iTxt]->Write2(_pDFile, _PicMip);
	}
	_pDFile->EndEntry(m_lspTextures.Len());
}
static fint g_FilePos = 0;

void CTextureContainer_Plain::WriteImageDirectory(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTextureContainer_Plain_WriteImageDirectory, MAUTOSTRIP_VOID);

	if (g_FilePos == 0) 
	{
		g_FilePos = _pDFile->GetFile()->Pos();
		_pDFile->BeginEntry("IMAGEDIRECTORY4");
		for(int iTxt = 0; iTxt < m_lspTextures.Len(); iTxt++)
			m_lspTextures[iTxt]->WriteIndexData(_pDFile->GetFile());
		_pDFile->EndEntry(m_lspTextures.Len());
	}
	else
	{
		int Pos = _pDFile->GetFile()->Pos();
		_pDFile->GetFile()->Seek(g_FilePos);
		for(int iTxt = 0; iTxt < m_lspTextures.Len(); iTxt++)
			m_lspTextures[iTxt]->WriteIndexData(_pDFile->GetFile());
		_pDFile->GetFile()->Seek(Pos);
	}
}

void CTextureContainer_Plain::WriteImageDirectory2(CDataFile* _pDFile, int32 *_PicMip)
{
	MAUTOSTRIP(CTextureContainer_Plain_WriteImageDirectory2, MAUTOSTRIP_VOID);

//	g_FilePos = _pDFile->GetFile()->Pos();
	_pDFile->BeginEntry("IMAGEDIRECTORY5");
	for(int iTxt = 0; iTxt < m_lspTextures.Len(); iTxt++)
		m_lspTextures[iTxt]->WriteIndexData2(_pDFile->GetFile(), _PicMip);
	_pDFile->EndEntry(m_lspTextures.Len());
}

void CTextureContainer_Plain::WriteXTC(CDataFile* _pDFile, bool _bWriteList)
{
	MAUTOSTRIP(CTextureContainer_Plain_WriteXTC_pDFile, MAUTOSTRIP_VOID);

	if(m_Source != "")
	{
		_pDFile->BeginEntry("SOURCE");
		m_Source.Write(_pDFile->GetFile());
		_pDFile->EndEntry(0);
	}
	_pDFile->BeginEntry("IMAGELIST");
	_pDFile->EndEntry(0);
	_pDFile->BeginSubDir();

	g_FilePos = 0;
	WriteImageDirectory(_pDFile);

	if (_bWriteList)
	{
		WriteImageList(_pDFile);
		WriteImageDirectory(_pDFile);
	}

	_pDFile->EndSubDir();
	if(m_lTextureCompileChecksums.Len() > 0)
	{
		_pDFile->BeginEntry("COMPILECHECKSUMS");
		_pDFile->GetFile()->Write(m_lTextureCompileChecksums.GetBasePtr(), m_lTextureCompileChecksums.ListSize());
		_pDFile->EndEntry(m_lTextureCompileChecksums.Len(), m_lTextureCompileChecksums.ListSize());
	}
}

void CTextureContainer_Plain::WriteXTC(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_Plain_WriteXTC_pName, MAUTOSTRIP_VOID);

	CDataFile DFile;
	DFile.Create(_pName, 2);
	WriteXTC(&DFile);
	DFile.Close();
}

void CTextureContainer_Plain::WriteXTC2(CDataFile* _pDFile, int32 *_PicMip)
{
	MAUTOSTRIP(CTextureContainer_Plain_WriteXTC2, MAUTOSTRIP_VOID);

	g_FilePos = 0;
	WriteImageList2(_pDFile, _PicMip);
	WriteImageDirectory2(_pDFile, _PicMip);
}

void CTextureContainer_Plain::WriteXTC2(const char* _pName, int32 *_PicMip)
{
	MAUTOSTRIP(CTextureContainer_Plain_WriteXTC2_pName, MAUTOSTRIP_VOID);

	CDataFile DFile;
	DFile.Create(_pName, 2);
	WriteXTC2(&DFile, _PicMip);
	DFile.Close();
}

class CWriteTexture
{
public:
	CTexture *m_pTexture;
	uint32 m_iIndex;
	uint32 m_iFilePos;
};
void CTextureContainer_Plain::WriteXTC2_XT0(const char* _pName, int32 *_PicMip)
{
    CCFile File;
	File.Open(_pName, CFILE_WRITE|CFILE_BINARY);

	uint32 nTextures = m_lspTextures.Len();

	TThinArray<CWriteTexture> lTextureWrite;
	lTextureWrite.SetLen(nTextures);
	uint32 nWrite = 0;

	for (int i = 0; i < nTextures; ++i)
	{
		CTexture *pTexture = m_lspTextures[i];

		int iPicMipGroup = pTexture->m_Properties.m_iPicMipGroup;
		if(iPicMipGroup & ~0x1f)
		{
			LogFile(CStrF("Picmip group for texture '%s' is invalid (%d), forcing to 0", pTexture->m_Name, iPicMipGroup));
			iPicMipGroup = 0;
		}

		int PicMip = (pTexture->m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP) ? 0 : MaxMT(0, _PicMip[iPicMipGroup] + pTexture->m_Properties.GetPicMipOffset());
		if (!(pTexture->m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP))
		{
			lTextureWrite[nWrite].m_pTexture = pTexture;
			lTextureWrite[nWrite].m_iIndex = i;
			++nWrite;
		}
	}

	File.WriteLE(nWrite);
	fint SavePos = File.Pos();
	File.RelSeek(sizeof(uint32) * nWrite * 2);

	for (int i = 0; i < nWrite; ++i)
	{
		CTexture *pTexture = m_lspTextures[lTextureWrite[i].m_iIndex];

		int iPicMipGroup = pTexture->m_Properties.m_iPicMipGroup;
		if(iPicMipGroup & ~0x1f)
		{
			LogFile(CStrF("Picmip group for texture '%s' is invalid (%d), forcing to 0", pTexture->m_Name, iPicMipGroup));
			iPicMipGroup = 0;
		}

		int PicMip = MinMT((pTexture->m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP) ? 0 : MaxMT(0, _PicMip[iPicMipGroup] + pTexture->m_Properties.GetPicMipOffset()), pTexture->m_nMipmaps - 1);
		lTextureWrite[i].m_iFilePos = File.Pos();
		if (!PicMip)
		{
			pTexture->m_LargestMap.Write(&File);
		}
		else
		{
			
			if (pTexture->m_lspMaps[PicMip])
				pTexture->m_lspMaps[PicMip]->Write(&File);
			else
				Error_static(M_FUNCTION, "Internal ellor");
		}
	}

	File.Seek(SavePos);
	for (int i = 0; i < nWrite; ++i)
	{
		File.WriteLE(lTextureWrite[i].m_iIndex);
		File.WriteLE(lTextureWrite[i].m_iFilePos);
	}

}

static int NextPow2(int _Value)
{
	int i = 1;
	while(i < _Value)
		i	<<= 1;

	return i;
}

class MRTC_TaskTexturePow2 : public MRTC_RemoteTaskBase
{
	MRTC_DECLARE;
public:
	class CArg : public MRTC_TaskBaseArg
	{
		MRTC_DECLARE;
	public:
		CArg(CTextureContainer_Plain* _pTC)
		{
			m_pContainer = _pTC;
		}
		CTextureContainer_Plain	*m_pContainer;
	};

	class CProcess : public MRTC_TaskBase
	{
		MRTC_DECLARE;
	public:

		virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
		{
			MRTC_TaskArgRemoteTask *pArg = TDynamicCast<MRTC_TaskArgRemoteTask>(_pArg);
			if(pArg == 0)
				return TASK_RETURN_ERROR;

			spCCFile spFile = CDiskUtil::CreateCCFile(pArg->m_lParamData, CFILE_READ);

			CImage InImage;
			uint32 iTex;
			uint32 iMip;
			spFile->ReadLE(iTex);
			spFile->ReadLE(iMip);
			InImage.Read(spFile, IMAGE_MEM_IMAGE, NULL);
			
			int Width = InImage.GetWidth();
			int Height = InImage.GetHeight();
			int PowWidth = NextPow2(Width);
			int PowHeight = NextPow2(Height);
			int CompressFormat = (InImage.GetMemModel() & IMAGE_MEM_COMPRESS_ALLFLAGS);
			CImage Temp;
			InImage.Decompress(&Temp);
			CImage Temp2;
			CImage::Stretch(&Temp, &Temp2, PowWidth, PowHeight);
			Temp2.Compress(CompressFormat, 1.0, &Temp);

			spCCFile spOutput = CDiskUtil::CreateCCFile(pArg->m_lOutput, CFILE_WRITE);
			spOutput->WriteLE(iTex);
			spOutput->WriteLE(iMip);
			Temp.Write(spOutput);


			return TASK_RETURN_FINISHED;


		}
	};

	virtual bool ProcessResult(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg, TArray<uint8> _lResult)
	{
		CArg* pArg = TDynamicCast<CArg>(_pArg);
		if(!pArg) return false;

		spCCFile spFile = CDiskUtil::CreateCCFile(_lResult, CFILE_READ);

		MRTC_IncProgress(NULL);

		uint32 iTex;
		uint32 iMip;
		spFile->ReadLE(iTex);
		spFile->ReadLE(iMip);
		CImage *pImg = &(pArg->m_pContainer->GetRealLocal(iTex)->m_LargestMap);
		if (iMip > 0)
			pImg = pArg->m_pContainer->GetRealLocal(iTex)->m_lspMaps[iMip];
		pImg->Read(spFile, IMAGE_MEM_IMAGE, NULL);
		return true;
	}

	virtual int Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
	{
		CArg* pArg = TDynamicCast<CArg>(_pArg);
		if(!_pArg) return TASK_RETURN_ERROR;

		int nTasks = 0;
		int nTextures = pArg->m_pContainer->GetNumRealLocal();
		for(int32 i = 0; i < nTextures; i++)
		{
			CTexture *pTex = pArg->m_pContainer->GetRealLocal(i);
			int nMaps = pTex->m_nMipmaps;
			for (int j = 0; j < nMaps; ++j)
			{
				CImage *pImage;

				if (j == 0)
					pImage = &(pTex->m_LargestMap);
				else
					pImage = pTex->m_lspMaps[j];

				if (pImage && pImage->GetWidth() && pImage->GetWidth())
				{
					int Width = pImage->GetWidth();
					int Height = pImage->GetWidth();
					int PowWidth = NextPow2(Width);
					int PowHeight = NextPow2(Height);
					if (Width != PowWidth || Height != PowHeight)
					{
						CImage Temp;
						if (pImage->IsCompressed())
						{
							TArray<uint8> lParamData;
							{
								spCCFile spParam = CDiskUtil::CreateCCFile(lParamData, CFILE_WRITE);
								uint32 iTex = i;
								uint32 iMip = j;
								spParam->WriteLE(iTex);
								spParam->WriteLE(iMip);
								pImage->Write(spParam);
							}
							++nTasks;

							TArray<uint8> Data;
							if (_pTask->m_pManager->Distribute("MRTC_TaskTexturePow2_Process", lParamData, Data, _pTask, _pArg, this) > 20)
								_pTask->m_pManager->Host_BlockOnNumTasks(10);

						}
						else
						{
							CImage Temp2;
							try 
							{
								CImage::Stretch(pImage, &Temp2, PowWidth, PowHeight);
							}
							catch (CCException)
							{
								CImage Conv;
								pImage->Convert(&Conv, IMAGE_FORMAT_RGBA8, IMAGE_CONVERT_RGBA);
								CImage::Stretch(&Conv, &Temp2, PowWidth, PowHeight);
							}
							*pImage = Temp2;
						}
					}
				}
			}
		}
		
		MRTC_InitProgressCount(nTasks, "Distributing compressed textures");

		return TASK_RETURN_INPROGRESS;
	}

};


#ifdef M_RTM
#	define MRTC_IMPLEMENT_DYNAMIC_NAMED(Name, BaseClass, ClassName)	\
	MRTC_CRuntimeClass ClassName::m_RuntimeClass = {#Name, &CreateCObj< ClassName >, &BaseClass::m_RuntimeClass};	\
		MRTC_CClassInit g_ClassReg##Name(&ClassName::m_RuntimeClass);				\
		MRTC_CRuntimeClass* ClassName::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#else
#	define MRTC_IMPLEMENT_DYNAMIC_NAMED(Name, BaseClass, ClassName)	\
	MRTC_CRuntimeClass ClassName::m_RuntimeClass = {#Name, &CreateCObj< ClassName >, &BaseClass::m_RuntimeClass, 0};	\
		MRTC_CClassInit g_ClassReg##Name(&ClassName::m_RuntimeClass);				\
		MRTC_CRuntimeClass* ClassName::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };

#endif

#ifdef M_RTM
#	define MRTC_IMPLEMENT_NAMED(Name, BaseClass, ClassName)																					\
		MRTC_CRuntimeClass ClassName::m_RuntimeClass = {#Name, NULL, &BaseClass::m_RuntimeClass};	\
		MRTC_CClassInit g_ClassReg##Name(&ClassName::m_RuntimeClass);				\
		MRTC_CRuntimeClass* ClassName::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#else
#	define MRTC_IMPLEMENT_NAMED(Name, BaseClass, ClassName)																					\
		MRTC_CRuntimeClass ClassName::m_RuntimeClass = {#Name, NULL, &BaseClass::m_RuntimeClass, 0};	\
		MRTC_CClassInit g_ClassReg##Name(&ClassName::m_RuntimeClass);				\
		MRTC_CRuntimeClass* ClassName::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#endif


MRTC_IMPLEMENT_DYNAMIC_NAMED(MRTC_TaskTexturePow2, MRTC_RemoteTaskBase, MRTC_TaskTexturePow2);
MRTC_IMPLEMENT_DYNAMIC_NAMED(MRTC_TaskTexturePow2_Process, MRTC_TaskBase, MRTC_TaskTexturePow2::CProcess);
MRTC_IMPLEMENT_NAMED(MRTC_TaskTexturePow2_Arg, MRTC_TaskBaseArg, MRTC_TaskTexturePow2::CArg);


static void BilinearRescale(CImage &_Dest, CImage &_Image, int _Width, int _Height)
{

	int w = _Width;
	int h = _Height;

	_Dest.Create(w, h, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);

	uint32 *pDest = (uint32*)_Dest.Lock();		// Since we know the format and size of spMap we lock the texture and write to it directly

	fp32 invU = 1.0f / fp32(w);
	fp32 invV = 1.0f / fp32(h);
	CVec2Dfp32 UV;
	UV.k[1]	= 0.5f * invV;
	for(int y = 0; y < h; y++)
	{
		UV.k[0] = 0.5f * invU;
		for(int x = 0; x < w; x++)
		{
			CPixel32 Col = _Image.GetPixelUV_Bilinear(UV);
			*pDest++	= Col;
			UV.k[0] += invU;
		}
		UV.k[1]	+= invV;
	}

	_Dest.Unlock();
}


void CTextureContainer_Plain::RemoveTextureVersion(CStr _Name, uint8 _Version)
{
	int nTex = GetNumLocal();
	for(int i = 0; i < nTex; i++)
	{
		CTexture* pTex = m_lspTextures[i];
		if(CStr(pTex->m_Name) == _Name && pTex->m_Properties.m_TextureVersion == _Version)
		{
			m_lspTextures.Del(i);
			return;
		}
	}
}

void CTextureContainer_Plain::StripVersions(const uint8* _pKeepVersions, int _nKeepVersions)
{
	for(int i = 0; i < m_lspTextures.Len(); i++)
	{
		uint8 aVersions[CTC_TEXTUREVERSION_MAX];
		int nVersions = EnumTextureVersions(i, aVersions, CTC_TEXTUREVERSION_MAX);

		bool bDone = false;
		for(int iKeep = 0; iKeep < _nKeepVersions && !bDone; iKeep++)
		{
			for(int iVer = 0; iVer < nVersions && !bDone; iVer++)
			{
				if(_pKeepVersions[iKeep] == aVersions[iVer])
				{
					// We found a version we want to keep, nuke all other versions of this format
					CStr TextureName = m_lspTextures[i]->m_Name;

					for(int iRemove = 0; iRemove < nVersions; iRemove++)
					{
						if(iRemove != iVer)
							RemoveTextureVersion(TextureName, aVersions[iRemove]);
					}

					bDone = true;
				}
			}
		}

		if(bDone == false)
		{
			LogFile(CStrF("WARNING: Was unable to find a requested version for texture '%s'", m_lspTextures[i]->m_Name));
		}
	}
}

void CTextureContainer_Plain::ScaleToPow2()
{
/*	MRTC_TaskManager* pTaskMan = TDynamicCast<MRTC_TaskManager>((CReferenceCount*)MRTC_GOM()->GetRegisteredObject("SYSTEM.TASKMANAGER"));
	M_ASSERT(pTaskMan != 0, "No taskmanager available");

	spMRTC_TaskInstance spTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskTexturePow2", MNew1(MRTC_TaskTexturePow2::CArg, this));
	pTaskMan->AddTask(spTask);
	pTaskMan->Host_BlockUntilDone();*/

	for (int i = 0; i < m_lspTextures.Len(); ++i)
	{
		CTexture *pTex = m_lspTextures[i];
		int Width = pTex->m_LargestMap.GetWidth();
		int Height = pTex->m_LargestMap.GetHeight();
		int PowWidth = NextPow2(Width);
		int PowHeight = NextPow2(Height);

		if (!(pTex->m_Properties.m_Flags & CTC_TEXTUREFLAGS_CLAMP_U) || !(pTex->m_Properties.m_Flags & CTC_TEXTUREFLAGS_CLAMP_V) || !(pTex->m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP))
		{
			int nMaps = pTex->m_nMipmaps;
			for (int j = 0; j < nMaps; ++j)
			{
				CImage *pImage;
				
				if (j == 0)
					pImage = &(pTex->m_LargestMap);
				else
					pImage = pTex->m_lspMaps[j];

				if (pImage)
				{
					if (pImage->GetMemModel() & IMAGE_MEM_VIRTUAL)
					{
						pImage->CreateVirtual(PowWidth, PowHeight, pImage->GetFormat(), pImage->GetMemModel(), pImage->GetPalette());					
					}
					else
					{
						int Width = pImage->GetWidth();
						int Height = pImage->GetHeight();
						if (Width != PowWidth || Height != PowHeight)
						{
							CImage Temp;
							if (pImage->IsCompressed())
							{			
								int CompressFormat = (pImage->GetMemModel() & IMAGE_MEM_COMPRESS_ALLFLAGS);

								pImage->Decompress(&Temp);
								int Format = Temp.GetFormat();
								CImage Temp2;
								Temp.Convert(&Temp2, IMAGE_FORMAT_BGRA8, IMAGE_CONVERT_RGBA);
	//							Temp2.Write("c:\\Decompress.tga");
	//							Temp.Write("c:\\Decompress.tga");

								BilinearRescale(Temp, Temp2, PowWidth, PowHeight);
	//							Temp.Write("c:\\Resacled.tga");

								Temp.Convert(&Temp2, Format, IMAGE_CONVERT_RGBA);

	//							if (CompressFormat & IMAGE_MEM_COMPRESSTYPE_3DC)
	//								M_BREAKPOINT;
								Temp2.Compress(CompressFormat, 1.0, pImage);
							}
							else
							{
								CImage Temp2;
								BilinearRescale(Temp2, *pImage, PowWidth, PowHeight);
								Temp2.Convert(pImage, pImage->GetFormat(), IMAGE_CONVERT_RGBA);
							}
						}
					}
				}			
				PowWidth = Max(PowWidth/2,1);
				PowHeight = Max(PowHeight/2,1);
			}

			if (nMaps > 1)
			{
				while (1)
				{
					CImage *pImage;
					
					int iImage = Max(0, nMaps-1);
					if (iImage == 0)
						pImage = &(pTex->m_LargestMap);
					else
						pImage = pTex->m_lspMaps[iImage];

					if (pImage->GetWidth() != 1 || pImage->GetHeight() != 1)
					{
						pTex->m_nMipmaps += 1;
						++nMaps;
						CImage Temp;
						if (pImage->IsCompressed())
						{			
							int CompressFormat = (pImage->GetMemModel() & IMAGE_MEM_COMPRESS_ALLFLAGS);
							pImage->Decompress(&Temp);
							int Format = Temp.GetFormat();
							CImage Temp2;
							Temp.Convert(&Temp2, IMAGE_FORMAT_BGRA8, IMAGE_CONVERT_RGBA);
							BilinearRescale(Temp, Temp2, PowWidth, PowHeight);
							Temp.Convert(&Temp2, Format, IMAGE_CONVERT_RGBA);
							Temp2.Compress(CompressFormat, 1.0, &Temp);
							pTex->m_lspMaps[iImage+1] = Temp.Duplicate();
						}
						else
						{
							CImage Temp2;
							BilinearRescale(Temp2, *pImage, PowWidth, PowHeight);
							Temp2.Convert(&Temp, pImage->GetFormat(), IMAGE_CONVERT_RGBA);
							pTex->m_lspMaps[iImage+1] = Temp.Duplicate();
						}

					}
					else
						break;
				}
			}
		}
	}
}

void CTextureContainer_Plain::FilterTextures(TArray<CStr> *_pValidTextures)
{
	if (!_pValidTextures)
		return;

	CStringHash Hash;
	Hash.Create(_pValidTextures->Len());

	for (int i = 0; i < _pValidTextures->Len(); ++i)
		Hash.Insert(i, (*_pValidTextures)[i].Ansi().LowerCase());

	TArray<spCTexture> lspTextures;
	lspTextures.SetLen(m_lspTextures.Len());
	int iCurrent = 0;

	for (int i = 0; i < lspTextures.Len(); ++i)
	{
		CStr id = CStr(m_lspTextures[i]->m_Name).Ansi().LowerCase();
		int iHash = Hash.GetIndex(id);

		if (m_lspTextures[i]->m_TextureID > 0)
		{
			m_pTC->FreeID(m_lspTextures[i]->m_TextureID);

			for (int k = i+1; k < lspTextures.Len(); ++k)
			{
				if(m_lspTextures[i]->m_TextureID == m_lspTextures[k]->m_TextureID)
					m_lspTextures[k]->m_TextureID = 0;
			}
		}

		if (iHash >= 0)
		{
			lspTextures[iCurrent] = m_lspTextures[i];
			lspTextures[iCurrent]->m_TextureID = m_pTC->AllocID(m_iTextureClass, iCurrent, m_lspTextures[i]->m_Name);
			++iCurrent;
		}
	}
	lspTextures.SetLen(iCurrent);
	m_lspTextures = lspTextures;
}
#endif

int CTextureContainer_Plain::AddFromImageList(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddFromImageList, 0);

	CCFile* pFile = _pDFile->GetFile();
	int iPalBase = m_lspPalettes.Len();
	int iFirstTxt = m_lspTextures.Len();
	int iCurrentTxt = iFirstTxt;

	// Check for Image-directory.
	bool bScan = true;
	_pDFile->PushPosition();
	if (_pDFile->GetNext("IMAGEDIRECTORY4"))
	{
		int nTxt = _pDFile->GetUserData();
		m_lspTextures.SetLen(iFirstTxt + nTxt);
		int iLastNewAdd = -1;
		for(int iTxt = 0; iTxt < nTxt; iTxt++)
		{
			m_lspTextures[iFirstTxt + iTxt] = MNew(CTexture);
			m_lspTextures[iFirstTxt + iTxt]->ReadIndexData(_pDFile->GetFile(), &m_lspPalettes, iPalBase);
			int TxtID = -1;
#ifdef USE_HASHED_TEXTURENAME
			if(iLastNewAdd != -1)
			{
				if(m_lspTextures[iLastNewAdd]->m_NameID == m_lspTextures[iFirstTxt + iTxt]->m_NameID)
					TxtID = m_lspTextures[iLastNewAdd]->m_TextureID;
			}
			if(TxtID == -1)
			{
				iLastNewAdd	= iFirstTxt + iTxt;
				TxtID = m_pTC->AllocID(m_iTextureClass, iFirstTxt + iTxt, m_lspTextures[iFirstTxt + iTxt]->m_NameID);
			}
#else
			if(iLastNewAdd != -1)
			{
				if( !CStrBase::stricmp(m_lspTextures[iLastNewAdd]->m_Name, m_lspTextures[iFirstTxt + iTxt]->m_Name) )
					TxtID = m_lspTextures[iLastNewAdd]->m_TextureID;
			}
			if(TxtID == -1)
			{
				iLastNewAdd	= iFirstTxt + iTxt;
				TxtID = m_pTC->AllocID(m_iTextureClass, iFirstTxt + iTxt, &m_lspTextures[iFirstTxt + iTxt]->m_Name[0]);
			}
#endif
			m_lspTextures[iFirstTxt + iTxt]->m_TextureID = TxtID;
			m_lspTextures[iFirstTxt + iTxt]->m_nMipmaps = 0;
		}

		bScan = false;
	}
	_pDFile->PopPosition();


	CStr NodeName;
	while((NodeName = _pDFile->GetNext()) != (const char *)"")
	{
		uint32 NodeHash = StringToHash(NodeName.GetStr());
		switch(NodeHash)
		{
		case MHASH2('PALE', 'TTES'):
			{
				if (_pDFile->GetSubDir())
				{
					while(_pDFile->GetNext("PALETTE"))
					{
						spCImagePalette spPal;
						if (spPal == NULL) spPal = MNew(CImagePalette);
						if (spPal == NULL) MemError("Read");
						CPixel32 Pal[256];
						if (_pDFile->GetUserData() != 256) Error("Read", "Supports only 256 color palettes.");
						pFile->Read(&Pal, sizeof(Pal));

						spPal->SetPalette((CPixel32*) &Pal, 0, 256);
						m_lspPalettes.Add(spPal);
					}
					_pDFile->GetParent();
				}
			}
			break;

		case MHASH2('IMAG', 'E'):
			{
				if (_pDFile->GetSubDir())
				{
					if (!bScan)
					{
						m_lspTextures[iCurrentTxt]->Read(_pDFile, &m_lspPalettes, iPalBase);
					}
					else
					{
						spCTexture spTxt = MNew(CTexture);
						if (spTxt == NULL) MemError("Read");
						spTxt->Read(_pDFile, &m_lspPalettes, iPalBase);
						AddTexture(spTxt);
					}
					iCurrentTxt++;

//CImage* pImg = spTxt->m_lspMaps[0];
//pImg->Write(pImg->GetClipRect().clip, CStrF("E:\\TEST\\TXT_IN%.2X.TGA", iTxt));
					_pDFile->GetParent();
				}
			}
			break;
		}
	}
/*
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
	{
		CRegistry* pEnv = pSys->GetEnvironment();
		
		if (pEnv && pEnv->GetValuei("RESOURCES_LOG", 0) != 0)
		{
			MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
			if (pTC)
			{
				CCFile TulFile;
				TulFile.Open(pSys->m_ExePath + "\\Textures.tul", CFILE_READ);
				int32 Len;
				TulFile.ReadLE(Len);
				for (int i = 0; i < Len; ++i)
				{
					CStr Temp;
					
					Temp.Read(&TulFile);

					int TexID = pTC->GetTextureID(Temp);
					if (TexID >= 0)
						pTC->SetTextureParam(TexID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_USED);
				}
			}
		}
	}*/

	return iFirstTxt;
}

void CTextureContainer_Plain::SetSource(CStr _Source)
{
	MAUTOSTRIP(CTextureContainer_Plain_SetSource, MAUTOSTRIP_VOID);

	m_Source = _Source;
}

CStr CTextureContainer_Plain::GetSource()
{
	MAUTOSTRIP(CTextureContainer_Plain_GetSource, CStr());

	return m_Source;
}

#ifndef PLATFORM_CONSOLE

#ifdef MERGE_NORMAL_SPECULAR

spCImage CTextureContainer_Plain::MergeNormalSpecMaps(CImage* _pNMap, CImage* _pSMap)
{
	spCImage spMap = MNew(CImage);
	if (!spMap)
		Error_static("::MergeNormalSpecMaps", "Out of memory.");

	int w = _pNMap->GetWidth();
	int h = _pNMap->GetHeight();

	spMap->Create(w, h, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);

	uint32 *pDest = (uint32*)spMap->Lock();		// Since we know the format and size of spMap we lock the texture and write to it directly

	fp32 invU = 1.0f / fp32(w);
	fp32 invV = 1.0f / fp32(h);
	CVec2Dfp32 UV;
	UV.k[1]	= 0.5f * invV;
	for(int y = 0; y < h; y++)
	{
		UV.k[0] = 0.5f * invU;
		for(int x = 0; x < w; x++)
		{
//			CPixel32 NCol = _pNMap->GetPixelUV_Bilinear(UV);
			CPixel32 NCol = _pNMap->GetPixel(_pNMap->GetClipRect(), CPnt(x,y));		// No point in doing a Bilinear filter when the ratio is 1:1 for normalmap
			CPixel32 SCol = _pSMap->GetPixelUV_Bilinear(UV);

			CPixel32 NSCol(NCol.GetR(), NCol.GetG(), NCol.GetB(), SCol.GetGray());
//			spMap->SetPixel(spMap->GetClipRect(), CPnt(x, y), NSCol);
			*pDest++	= NSCol;
			UV.k[0] += invU;
		}
		UV.k[1]	+= invV;
	}

	spMap->Unlock();
	return spMap;
}

class CRNMS_Params
{
public:
	CVec3Dfp32 m_LightVec;
	CVec3Dfp32 m_DiffuseScale;
	CVec3Dfp32 m_SpecularScale;
	fp32 m_SpecularPower;

	CRNMS_Params()
	{
		m_LightVec = CVec3Dfp32(1.0f, -0.5f, -0.0f);
		m_DiffuseScale = 1.0f;
		m_SpecularScale = 1.0f;
		m_SpecularPower = 16.0f;
	}
};

static fp32 pow16(fp32 x)
{
	fp32 x2 = x*x;
	fp32 x4 = x2*x2;
	fp32 x8 = x4*x4;
	return x8*x8;
}

static spCImage RenderNormalMapSurface(CImage* _pDiffuse, CImage* _pSpecular, CImage* _pNormal, CRNMS_Params& _Params)
{
#ifdef	PLATFORM_CONSOLE
	spCImage spImg;
	return spImg;
#else
	spCImage spMap = MNew(CImage);
	if (!spMap)
		Error_static("::RenderNormalMapSurface", "Out of memory.");

	M_ASSERT(_pDiffuse, "!");
	M_ASSERT(_pSpecular, "!");
	M_ASSERT(_pNormal, "!");


	int w = _pNormal->GetWidth() * 2;
	int h = _pNormal->GetHeight() * 2;

	int Format = (_pDiffuse->GetFormat() & IMAGE_FORMAT_ALPHA) ? IMAGE_FORMAT_BGRA8 : IMAGE_FORMAT_BGRX8;

	spMap->Create(w, h, Format, IMAGE_MEM_IMAGE);

	CVec3Dfp32 EyeVec(-1.0f, 0, 0);
	CVec3Dfp32 LightVec = _Params.m_LightVec;
	LightVec.Normalize();

	bool bDefaultPower = M_Fabs(_Params.m_SpecularPower - 16.0f) < 0.00001f;

	for(int y = 0; y < h; y++)
		for(int x = 0; x < w; x++)
		{
			CVec2Dfp32 UV(fp32(x + 0.5f) / fp32(w), fp32(y + 0.5f) / fp32(h));
			CPixel32 DCol = _pDiffuse->GetPixelUV_Bilinear(UV);
			CPixel32 NCol = _pNormal->GetPixelUV_Bilinear(UV);
			CPixel32 SCol = _pSpecular->GetPixelUV_Bilinear(UV);

			CVec3Dfp32 Normal(NCol.GetR() - 127, NCol.GetG() - 127, NCol.GetB() - 127);
			Normal.Normalize();

			CVec3Dfp32 Ref = EyeVec - Normal * (2.0f * (Normal * EyeVec));

			CVec3Dfp32 Diffuse = _Params.m_DiffuseScale * ((1.0f + LightVec * Normal)*0.5f);
			fp32 SpecScale = Clamp01(LightVec * Ref);
			fp32 SpecPower = (bDefaultPower) ? 
				pow16(SpecScale) :
				M_Pow(Clamp01(LightVec * Ref), _Params.m_SpecularPower);

			CVec3Dfp32 Specular = _Params.m_SpecularScale * (M_Pow(Clamp01(LightVec * Ref), _Params.m_SpecularPower));

			CVec3Dfp32 DColf(DCol.GetR(), DCol.GetG(), DCol.GetB());
			fp32 DColMag = DColf.Length();
			if (DColMag > 0)
			{
				CVec3Dfp32 DColf2;
				DColf.CompMul(DColf, DColf2);
				DColf2.Scale(DColMag / DColf2.Length(), DColf2);
				
				DColf2.Lerp(DColf, 0.5f, DColf);
			}

			CVec3Dfp32 SColfNew = DColf;
			CVec3Dfp32 SColf(SCol.GetR(), SCol.GetG(), SCol.GetB());
			SColfNew[1] *= 0.5f;
			SColf[1] *= 0.5f;
			if (DColMag > 0)
			{
				SColfNew *= 1.0f / DColMag;
				SColfNew *= SColf.Length();
			}
			SColfNew[1] *= 2.0f;

			Diffuse[0] *= DColf[0];
			Diffuse[1] *= DColf[1];
			Diffuse[2] *= DColf[2];
			Specular[0] *= SColfNew[0];
			Specular[1] *= SColfNew[1];
			Specular[2] *= SColfNew[2];
/*			Specular[0] *= SCol.GetR();
			Specular[1] *= SCol.GetG();
			Specular[2] *= SCol.GetB();*/

			CVec3Dfp32 Shading = Diffuse + Specular;

/*			fp32 DiffScalar = (1.0f + LightVec * Normal)*0.5f;
			DiffScalar = DiffScalar * DiffScalar * _Params.m_DiffuseScale;
			fp32 SpecScalar = Clamp01(LightVec * Ref);
			SpecScalar = pow(SpecScalar, _Params.m_SpecularPower) * _Params.m_SpecularScale;*/

/*			CPixel32 Spec = SCol;
			Spec *= SpecScalar;
			CPixel32 Col = DCol;
			Col *= DiffScalar;
			Col += Spec;*/

			CPixel32 Col;
			Col.R() = RoundToInt(ClampRange(Shading[0], 255.0f));
			Col.G() = RoundToInt(ClampRange(Shading[1], 255.0f));
			Col.B() = RoundToInt(ClampRange(Shading[2], 255.0f));
			Col.A() = DCol.GetA();

			spMap->SetPixel(spMap->GetClipRect(), CPnt(x, y), Col);
		}

		spCImage spMapResampled = MNew(CImage);
		if (!spMapResampled)
			Error_static("::RenderNormalMapSurface", "Out of memory.");
		spMapResampled->Create(w >> 1, h >> 1, Format, IMAGE_MEM_IMAGE);
		CImage::StretchHalf(spMap, spMapResampled);
		//	spMap->Create(w >> 2, h >> 2, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
		//	CImage::StretchHalf(spMapResampled, spMap);

		return spMapResampled;
#endif
}


#endif

class CCMap
{
public:
	spCImage m_spMap;
	CTC_TextureProperties m_Properties;
	CStr m_Name;
	int m_iCMap;
};

static bool IsTextureAvailable(CTextureContainer_Plain* _pContainer, CStr _Name)
{
	int nLocal = _pContainer->GetNumLocal();
	for(int i = 0; i < nLocal; i++)
		if(_Name == _pContainer->GetName(i))
			return true;

	return false;
}

static void AddTexturesFromContainer(CTextureContainer_Plain* _pDest, CTextureContainer_Plain* _pSource, CStr _Name)
{
	int nLocal = _pSource->GetNumLocal();
	int iLocal;
	for(iLocal = 0; iLocal < nLocal; iLocal++)
	{
		if(_pSource->GetName(iLocal) == _Name)
			break;
	}

	while((iLocal < nLocal) && (_pSource->GetName(iLocal) == _Name))
	{
		_pDest->AddTexture(_pSource->GetTextureMap(iLocal, CTC_TEXTUREVERSION_ANY)->Duplicate());
		_pDest->AddCompileChecksum(_pSource->GetCompileChecksum(iLocal));
		iLocal++;
	}
}

spCTextureContainer_Plain GenerateThumbnails(CTextureContainer_Plain* _pTextures)
{
	spCTextureContainer_Plain spThumb = MNew(CTextureContainer_Plain);
	CStr LastAdd;
	int nLocal = _pTextures->GetNumLocal();
	for(int i = 0; i < nLocal; i++)
	{
		if(_pTextures->GetName(i) != LastAdd)
		{
			LastAdd	= _pTextures->GetName(i);

			spCTexture spTxt = _pTextures->GetTextureMap(i, CTC_TEXTUREVERSION_ANY);
			CImage* pImg = _pTextures->GetTexture(i, 0, CTC_TEXTUREVERSION_ANY);

			int nMip = 0;
			CImage Desc;
			_pTextures->GetTextureDesc(i, &Desc, nMip);

			int j = 1;
			while((j < nMip) && (Max(pImg->GetWidth(), pImg->GetHeight()) > 64))
				pImg = _pTextures->GetTexture(i, j++, CTC_TEXTUREVERSION_ANY);

			CImage TempImage;
			if(pImg->GetMemModel() & IMAGE_MEM_COMPRESSED)
			{
				pImg->Decompress(&TempImage);
				pImg = &TempImage;
			}

			CTC_TextureProperties ThumbProperties;
			ThumbProperties	= spTxt->m_Properties;
			ThumbProperties.m_Flags	|= CTC_TEXTUREFLAGS_NOPICMIP | CTC_TEXTUREFLAGS_NOMIPMAP;
			ThumbProperties.m_TextureVersion = CTC_TEXTUREVERSION_RAW;
			int iLocal = spThumb->AddTexture(pImg->Duplicate(), ThumbProperties, 1, 
#ifdef USE_HASHED_TEXTURENAME
					CStrF("#%08X", spTxt->m_NameID));
#else
					CStrF("#%s", (char*)&spTxt->m_Name[1]));
#endif
		}
	}

	return spThumb;
}

int GetTextureIndexByName(CTextureContainer_Plain* _pContainer, CStr _Name, int _Version)
{
	int nLocal = _pContainer->GetNumLocal();
	for(int i = 0; i < nLocal; i++)
	{
		CTexture* pTex = _pContainer->GetTextureMap(i, CTC_TEXTUREVERSION_ANY);
		if(_Name == _pContainer->GetName(i))
		{
			return i;
		}
	}

	return -1;
}

static CStr GetCompleteString_r(const CRegistry* _pParent)
{
	CStr KeyString = _pParent->GetThisName() + _pParent->GetThisValue();
	for(int i = 0; i < _pParent->GetNumChildren(); i++)
		KeyString	+= GetCompleteString_r(_pParent->GetChild(i));

	return KeyString;
}

static uint32 GenerateChecksum(const CRegistry* _pParent)
{
	return StringToHash(GetCompleteString_r(_pParent));
}

void CTextureContainer_Plain::XTXUpdate(int _Flags, const CRegistry& _Reg, CStr _ContainerName, CStr _SourcePath, CXTXCompileResult& _Result)
{
	if(!CDiskUtil::FileExists(_ContainerName))
	{
		XTXCompile(_Flags, _Reg, _SourcePath, _Result);
		if (_Flags & XTX_COMPILE_THUMBNAILS)
			_Result.m_spTCThumb	= GenerateThumbnails(this);
		return;
	}

	if(_SourcePath != "" && _SourcePath[_SourcePath.Len() - 1] != '\\')
		_SourcePath += "\\";

	CTextureContainer_Plain OldContainer;
	OldContainer.AddFromXTC(_ContainerName);

	CFileInfo MasterContainerInfo = CDiskUtil::FileTimeGet(_ContainerName);

	CRegistry_Dynamic NewRegistry;
	TThinArray<uint8>	lRequiresAdd;
	TThinArray<uint8>	lProcessed;
	lProcessed.SetLen(_Reg.GetNumChildren());
	lRequiresAdd.SetLen(_Reg.GetNumChildren());
	uint8* pRequiresAdd = lRequiresAdd.GetBasePtr();
	uint8* pProcessed = lProcessed.GetBasePtr();
	memset(pRequiresAdd, 0, _Reg.GetNumChildren());
	memset(pProcessed, 0, _Reg.GetNumChildren());

	int aDependsOn[256];
	int nDependencies;

	LogFile("XTX: Scanning container to find changed textures (using inherent dependencies)");
	bool bOnlyNormal = true;
	for(int iPass = 0; iPass < 2; iPass++)
	{
		for(int i = 0; i < _Reg.GetNumChildren(); i++)
		{
			const CRegistry* pChild = _Reg.GetChild(i);
			if(pChild->GetThisName().CompareNoCase("TEXTURE") != 0)
				continue;

			if(pProcessed[i] || pRequiresAdd[i])	// This one is already processed
				continue;

			nDependencies	= 0;

			CStr Path = pChild->GetValue("PATH");
			CStr Name = pChild->GetValue("NAME");
			if(Name == "")
				Name	= Path.GetFilenameNoExt();
			CTC_TextureProperties Properties;
			{
				for(int i = 0; i < pChild->GetNumChildren(); i++)
					Properties.Parse_XRG(*pChild->GetChild(i));
			}
			{
				
	#ifdef MERGE_NORMAL_SPECULAR
				if (Properties.m_Flags & CTC_TEXTUREFLAGS_NORMALMAP)
				{
					CStr NMapName = Path.GetFilenameNoExt();
					int UnderScore = NMapName.FindReverse("_");

					if (UnderScore >= 0)
					{
						CStr NMapNameCut = NMapName.Copy(0, UnderScore);
						CStr SMapPath;
						int iSMap = -1;

						CStr SMapName = NMapNameCut + "_S";
	//							LogFile(CStrF("    Looking for %s", SMapName.Str()));
						CTC_TextureProperties SProperties;

						for(int j = 0; j < _Reg.GetNumChildren(); j++)
						{
							const CRegistry *pSChild = _Reg.GetChild(j);
							if(pSChild->GetThisName().CompareNoCase("TEXTURE") == 0)
							{
								CStr SPath = pSChild->GetValue("PATH");
								if ((SPath.GetFilenameNoExt()).CompareNoCase(SMapName) == 0)
								{
									{
										for(int i = 0; i < pSChild->GetNumChildren(); i++)
											SProperties.Parse_XRG(*pSChild->GetChild(i));
									}

									SMapPath = _SourcePath + SPath;
									iSMap = j;
									break;
								}
							}
						}

						if (SMapPath != "" && iSMap >= 0)
						{
							TArray<CCMap> lCMaps;

							if ( (Properties.m_Flags & CTC_TEXTUREFLAGS_PALETTE) && (Properties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY) )
							{
								bool bQuit = false;
								// Find all color maps for this normal map and put the specular in the color alpha
								int iCurrentCheck = 0;
								while (iCurrentCheck <= 9)
								{
									CCMap CurrentCMap;
									
									CStr CMapName;

									int iFindNumber = 1;
									while (1)
									{
										if (iCurrentCheck == 0)
											CMapName = NMapNameCut + "_C";
										else if (iCurrentCheck == 1)
											CMapName = NMapNameCut + CStrF("_C%02d", iFindNumber);
										else if (iCurrentCheck == 2)
											CMapName = NMapNameCut + CStrF("_C%d", iFindNumber);
										else if (iCurrentCheck == 3)
											CMapName = NMapNameCut + CStrF("_%02d_C", iFindNumber);
										else if (iCurrentCheck == 4)
											CMapName = NMapNameCut + CStrF("_%d_C", iFindNumber);
										else if (iCurrentCheck == 5)
											CMapName = NMapNameCut + "_D";
										else if (iCurrentCheck == 6)
											CMapName = NMapNameCut + CStrF("_D%02d", iFindNumber);
										else if (iCurrentCheck == 7)
											CMapName = NMapNameCut + CStrF("_D%d", iFindNumber);
										else if (iCurrentCheck == 8)
											CMapName = NMapNameCut + CStrF("_%02d_D", iFindNumber);
										else if (iCurrentCheck == 9)
											CMapName = NMapNameCut + CStrF("_%d_D", iFindNumber);

	//											LogFile(CStrF("    Looking for %s", CMapName.Str()));

										CStr CMapPath;

										int d = 0;

										for(; d < _Reg.GetNumChildren(); d++)
										{
											const CRegistry *pDChild = _Reg.GetChild(d);
											if(pDChild->GetThisName().CompareNoCase("TEXTURE") == 0)
											{													
												CStr CPath = pDChild->GetValue("PATH");
												if ((CPath.GetFilenameNoExt()).CompareNoCase(CMapName) == 0)
												{
													{
														for(int i = 0; i < pDChild->GetNumChildren(); i++)
															CurrentCMap.m_Properties.Parse_XRG(*pDChild->GetChild(i));
													}

													CMapPath = _SourcePath + CPath;
													CurrentCMap.m_Name = pDChild->GetValue("NAME");
													if (CurrentCMap.m_Name == "")
														CurrentCMap.m_Name = CMapPath.GetFilenameNoExt();

													break;
												}
											}
										}
										CurrentCMap.m_iCMap = d;

										if (CMapPath != "")
										{
											// Merge specular map with color map

            								CurrentCMap.m_spMap = ReadImage(CMapPath);

											if (CurrentCMap.m_spMap->GetFormat() & IMAGE_FORMAT_ALPHA)
											{
												bQuit = true;

												lCMaps.Clear();
												break;
											}

											lCMaps.Add(CurrentCMap);
										}
										else if (iFindNumber > 4)
											break;

										++iFindNumber;

										if (iCurrentCheck == 0 || iCurrentCheck == 5)
											break;
									}

									if (bQuit)
										break;
									++iCurrentCheck;
								}

							}

							aDependsOn[nDependencies++]	= iSMap;
							pProcessed[iSMap]	= 1;

							if (lCMaps.Len())
							{
								for (int i = 0; i < lCMaps.Len(); ++i)
								{
									aDependsOn[nDependencies++]	= lCMaps[i].m_iCMap;
									pProcessed[lCMaps[i].m_iCMap]	= 1;
								}
							}
						}
					}
				}
				else if(bOnlyNormal)
					continue;
	#endif // MERGE_NORMAL_SPECULAR

				bool bIsCylCubeTex = (pChild->GetValue("CONVERT").CompareNoCase(CTC_TextureProperties::ms_TxtConvertTranslate[CTC_CONVERT_CUBEFROMCYLINDER]) == 0 ||
									pChild->GetValue("CONVERT").CompareNoCase(CTC_TextureProperties::ms_TxtConvertTranslate[CTC_CONVERT_CUBEFROMSPHERE]) == 0 );

				if( (Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN) && (!bIsCylCubeTex) )
				{
					for(int j = 1; j < 6; j++)
					{
						if((i + j) >= _Reg.GetNumChildren() )
							LogFile(CStrF("XTX: Broken cubemap chain for image %s", pChild->GetValue("PATH").Str()));
						else
						{
							aDependsOn[nDependencies++]	= i + j;
							pProcessed[i+j]	= 1;
						}
					}
				}

				pProcessed[i]	= 1;
				aDependsOn[nDependencies++]	= i;

				bool bAddAll = false;
				for(int d = 0; d < nDependencies; d++)
				{
					CStr FileName = _SourcePath + _Reg.GetChild(aDependsOn[d])->GetValue("PATH");

					if(!CDiskUtil::FileExists(FileName) || (MasterContainerInfo.m_TimeWrite < CDiskUtil::FileTimeGet(FileName).m_TimeWrite))
					{
						bAddAll	= true;
						break;
					}

					CTC_TextureProperties OldProperties;
					CTC_TextureProperties NewProperties;

					//Acquire old version... if this was a converted cylinder map, just get the front one
					int iOld;
					if( bIsCylCubeTex )
					{
						iOld = GetTextureIndexByName(&OldContainer, Name + "_00", CTC_TEXTUREVERSION_ANY);
					}
					else iOld = GetTextureIndexByName(&OldContainer, Name, CTC_TEXTUREVERSION_ANY);
					
					if((iOld >= 0) && (iOld < OldContainer.m_lTextureCompileChecksums.Len()))
					{
						uint32 NewChecksum = GenerateChecksum(pChild);
						uint32 OldChecksum = OldContainer.m_lTextureCompileChecksums[iOld];

						if(NewChecksum != OldChecksum)
						{
							bAddAll	= true;
							break;
						}
					}
					else
					{
						// Couldn't match properties, recompile all 
						bAddAll	= true;
						break;
					}

				}

				if(bAddAll)
				{
					for(int d = 0; d < nDependencies; d++)
					{
						pRequiresAdd[aDependsOn[d]]	= 1;
						pProcessed[aDependsOn[d]]	= 1;
					}
				}

			}
		}
		bOnlyNormal	= false;
	}

	for(int i = 0; i < _Reg.GetNumChildren(); i++)
	{
		const CRegistry* pChild = _Reg.GetChild(i);
		if(pChild->GetThisName().CompareNoCase("TEXTURE") != 0)
			continue;

		if(!pRequiresAdd[i])
			continue;

		NewRegistry.AddReg(pChild->Duplicate());
	}

	if(NewRegistry.GetNumChildren() > 0 )
	{
		LogFile(CStrF("XTX: %d textures requires update", NewRegistry.GetNumChildren()));
		CXTXCompileResult NewResult;
		CTextureContainer_Plain NewContainer;
		NewContainer.XTXCompile(_Flags, NewRegistry, _SourcePath, NewResult);

		uint CubeChain = 0;

		for(int i = 0; i < _Reg.GetNumChildren(); i++)
		{
			const CRegistry* pChild = _Reg.GetChild(i);
			if(pChild->GetThisName().CompareNoCase("TEXTURE") != 0)
				continue;

			CStr Name = pChild->GetValue("NAME");
			if (Name == "")
				Name = pChild->GetValue("PATH").GetFilenameNoExt();

			{
				// Patch to find cubemapchain children
				if(CubeChain > 0)
				{
					if(Name.Len() > 3)
						memcpy(Name.GetStr(), "$$$", 3);
					else
						Name = "$$$";
					CubeChain--;
				}

				uint32 Flags = CStr::TranslateFlags(pChild->GetValue("FLAGS"), CTC_TextureProperties::ms_TxtPropFlagsTranslate);

				if(Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN)
				{
					M_ASSERT(CubeChain == 0, "New cubemap chain before finishing with old one");
					// This is a chain, the 5 following will have broken names
					CubeChain = 5;
				}
			}

			if(pRequiresAdd[i] && IsTextureAvailable(&NewContainer, Name))
				AddTexturesFromContainer(this, &NewContainer, Name);
			else if(!pRequiresAdd[i] && IsTextureAvailable(&OldContainer, Name))
				AddTexturesFromContainer(this, &OldContainer, Name);

			else if( (pChild->GetValue("CONVERT").CompareNoCase(CTC_TextureProperties::ms_TxtConvertTranslate[CTC_CONVERT_CUBEFROMCYLINDER])==0 ||
					pChild->GetValue("CONVERT").CompareNoCase(CTC_TextureProperties::ms_TxtConvertTranslate[CTC_CONVERT_CUBEFROMSPHERE])==0) && 
					!pRequiresAdd[i] && IsTextureAvailable(&OldContainer, Name+CStrF("_00")))
			{
				AddTexturesFromContainer(this, &OldContainer, Name + CStrF("_00"));	
				AddTexturesFromContainer(this, &OldContainer, Name + CStrF("_01"));
				AddTexturesFromContainer(this, &OldContainer, Name + CStrF("_02"));
				AddTexturesFromContainer(this, &OldContainer, Name + CStrF("_03"));
				AddTexturesFromContainer(this, &OldContainer, Name + CStrF("_04"));
				AddTexturesFromContainer(this, &OldContainer, Name + CStrF("_05"));
			}
			else if( (pChild->GetValue("CONVERT").CompareNoCase(CTC_TextureProperties::ms_TxtConvertTranslate[CTC_CONVERT_CUBEFROMCYLINDER])==0 ||
					pChild->GetValue("CONVERT").CompareNoCase(CTC_TextureProperties::ms_TxtConvertTranslate[CTC_CONVERT_CUBEFROMSPHERE])==0) && 
					pRequiresAdd[i] && IsTextureAvailable(&NewContainer, Name+CStrF("_00")))
			{
				AddTexturesFromContainer(this, &NewContainer, Name + CStrF("_00"));	
				AddTexturesFromContainer(this, &NewContainer, Name + CStrF("_01"));
				AddTexturesFromContainer(this, &NewContainer, Name + CStrF("_02"));
				AddTexturesFromContainer(this, &NewContainer, Name + CStrF("_03"));
				AddTexturesFromContainer(this, &NewContainer, Name + CStrF("_04"));
				AddTexturesFromContainer(this, &NewContainer, Name + CStrF("_05"));	
			}

			else
				LogFile(CStrF("XTX: Could not find texture '%s' (has it been merged with another texture?)", Name.Str()));
		}

		if (_Flags & XTX_COMPILE_THUMBNAILS)
			_Result.m_spTCThumb	= GenerateThumbnails(this);
	}
	else
	{
		LogFile("XTX: No textures changed");
	}
}

class MRTC_TaskArgCompressTexture : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgCompressTexture(CTexture* _pTexture, fp32 _Quality)
	{
		m_pTexture	= _pTexture;
		m_Quality	= _Quality;
	}
	CTexture*	m_pTexture;
	fp32	m_Quality;
};

class MRTC_TaskArgConvertTexture : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgConvertTexture(CTextureContainer_Plain* _pTC, int _iTexture, int _SupportedFormats)
	{
		m_pContainer	= _pTC;
		m_iTexture		= _iTexture;
		m_SupportedFormats	= _SupportedFormats;
	}
	CTextureContainer_Plain*	m_pContainer;
	int	m_iTexture;
	int	m_SupportedFormats;
};

class MRTC_TaskCompressTexture : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg);
};

class MRTC_TaskConvertTexture : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg);
};

class MRTC_TaskArgCompressTextureContainer : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgCompressTextureContainer(CTextureContainer_Plain* _pTC, fp32 _Quality)
	{
		m_pContainer	= _pTC;
		m_Quality	= _Quality;
	}
	CTextureContainer_Plain*	m_pContainer;
	fp32	m_Quality;
};

class MRTC_TaskCompressTextureContainer : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg);
};

class MRTC_TaskArgConvertTextureContainer : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgConvertTextureContainer(CTextureContainer_Plain* _pTC, int _Formats)
	{
		m_pContainer	= _pTC;
		m_SupportedFormat	= _Formats;
	}
	CTextureContainer_Plain*	m_pContainer;
	int	m_SupportedFormat;
};

class MRTC_TaskConvertTextureContainer : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg);
};

class MRTC_TaskArgGaussFilterCubemap : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgGaussFilterCubemap(CTextureContainer_Plain* _pTC, int _iTexture, CTextureContainer_Plain::CTC_CubeFilterParams& _Params)
	{
		m_pContainer	= _pTC;
		m_iTexture		= _iTexture;
		m_Params		= _Params;
	}
	CTextureContainer_Plain* m_pContainer;
	int	m_iTexture;
	CTextureContainer_Plain::CTC_CubeFilterParams m_Params;
};

class MRTC_TaskGaussFilterCubemap : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg);
};

class MRTC_TaskArgSmoothCubemapEdges : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgSmoothCubemapEdges(CTextureContainer_Plain* _pTC, int _iTexture)
	{
		m_pContainer	= _pTC;
		m_iTexture	= _iTexture;
	}
	CTextureContainer_Plain* m_pContainer;
	int m_iTexture;
};

class MRTC_TaskArgTrashCubemapNames : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgTrashCubemapNames(CTextureContainer_Plain* _pTC, int _iTexture)
	{
		m_pContainer	= _pTC;
		m_iTexture	= _iTexture;
	}
	CTextureContainer_Plain* m_pContainer;
	int m_iTexture;
};

class MRTC_TaskSmoothCubemapEdges : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg);
};

class MRTC_TaskTrashCubemapNames : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg);
};

class MRTC_TaskArgTexturePostFilter : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgTexturePostFilter(CTextureContainer_Plain* _pTC, int _iTexture, CTextureContainer_Plain::CTC_PostFilterParams& _Params)
	{
		m_pContainer	= _pTC;
		m_iTexture		= _iTexture;
		m_Params		= _Params;
	}
	CTextureContainer_Plain* m_pContainer;
	int	m_iTexture;
	CTextureContainer_Plain::CTC_PostFilterParams	m_Params;
};

class MRTC_TaskTexturePostFilter : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg);
};

int MRTC_TaskConvertTexture::Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
{
	MRTC_TaskArgConvertTexture* pArg = TDynamicCast<MRTC_TaskArgConvertTexture>(_pArg);
	if(!pArg) return TASK_RETURN_ERROR;

	do
	{
		CImage TexDesc;
		int nMipmaps;

		pArg->m_pContainer->GetTextureDesc(pArg->m_iTexture, &TexDesc, nMipmaps);

		if( TexDesc.GetMemModel() & IMAGE_MEM_COMPRESSED )
		{
			break;
		}

		if(TexDesc.GetFormat() & pArg->m_SupportedFormats)
		{
			break;
		}

		if( TexDesc.GetFormat() & IMAGE_FORMAT_BGR8 )
		{
			CTexture *pTexture = pArg->m_pContainer->GetTextureMap(pArg->m_iTexture, CTC_TEXTUREVERSION_ANY);

			spCImage spImg;

			if (pTexture->m_LargestMap.IsCompressed())
			{
				spImg = MNew(CImage);
				pTexture->m_LargestMap.Decompress(spImg);				
			}
			else
			{
				spImg = pTexture->m_LargestMap.Duplicate();
			}

#ifndef USE_HASHED_TEXTURENAME
			LogFile(CStrF("XTX: Converting %s from 24 bit texture to 32 bit ", pTexture->m_Name));
#endif			

			pTexture->m_LargestMap.Create(spImg->GetWidth(), spImg->GetHeight(), IMAGE_FORMAT_BGRX8, spImg->GetMemModel());

			CImage::Convert(spImg, &pTexture->m_LargestMap);			

			for( int nMipmap = 1; nMipmap < pTexture->m_nMipmaps; nMipmap++ )
			{
				if (pTexture->m_lspMaps[nMipmap]->IsCompressed())
				{
					spImg = MNew(CImage);
					pTexture->m_lspMaps[nMipmap]->Decompress(spImg);				
					pTexture->m_lspMaps[nMipmap] = spImg->Convert(IMAGE_FORMAT_BGRX8);
				}
				else
				{
					pTexture->m_lspMaps[nMipmap] = pTexture->m_lspMaps[nMipmap]->Convert(IMAGE_FORMAT_BGRX8);
				}
			}
		}
	} while(0);

	return TASK_RETURN_FINISHED;
}

class MRTC_TaskArgHostCompressTexture : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgHostCompressTexture(CTexture* _pTex, fp32 _Quality) : m_pTexture(_pTex), m_Quality(_Quality) {}
	CTexture*	m_pTexture;
	fp32	m_Quality;
};

// This task is spawned on host once for each texture
class MRTC_TaskHostCompressTexture : public MRTC_RemoteTaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
	{
		MRTC_TaskArgHostCompressTexture* pArg = TDynamicCast<MRTC_TaskArgHostCompressTexture>(_pArg);
		if(!_pArg) return TASK_RETURN_ERROR;
		CTexture* pTex = pArg->m_pTexture;

		TArray<uint8> lParamChunk;
/*		{
			CDataFile DataFile;
			DataFile.Create(lParamChunk);
			DataFile.BeginEntry("QUALITY");
			DataFile.GetFile()->WriteLE(pArg->m_Quality);
			DataFile.EndEntry(0);
			pTex->SerializeWrite(&DataFile);
			DataFile.Close();
		}
*/		TArray<TArray<uint8> > lDataChunks;

		_pTask->m_pManager->Distribute("MRTC_TaskCompressTexture", lParamChunk, lDataChunks, _pTask, _pArg, this, MRTC_TaskHostCompressTexture::OnDistribute);

		return TASK_RETURN_INPROGRESS;
	}

	static void OnDistribute(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg, TArray<uint8>& _lParamChunk, TArray<TArray<uint8> >& _lDataChunks)
	{
		MRTC_TaskArgHostCompressTexture* pArg = TDynamicCast<MRTC_TaskArgHostCompressTexture>(_pArg);
		if(!_pArg) return;
		CTexture* pTex = pArg->m_pTexture;

		{
			CDataFile DataFile;
			DataFile.Create(_lParamChunk);
			DataFile.BeginEntry("QUALITY");
			DataFile.GetFile()->WriteLE(pArg->m_Quality);
			DataFile.EndEntry(0);
			pTex->SerializeWrite(&DataFile);
			DataFile.Close();
		}
	}
	virtual bool ProcessResult(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg, TArray<uint8> _lResult)
	{
		MRTC_TaskArgHostCompressTexture* pArg = TDynamicCast<MRTC_TaskArgHostCompressTexture>(_pArg);
		if(!_pArg) return false;
		CDataFile DFile;
		DFile.Open(_lResult);
		pArg->m_pTexture->SerializeRead(&DFile);
		return true;
	}
};

int MRTC_TaskCompressTexture::Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
{
	MRTC_TaskArgRemoteTask *pArg = TDynamicCast<MRTC_TaskArgRemoteTask>(_pArg);
	if(pArg == 0)
		return TASK_RETURN_ERROR;

	fp32 Quality = 1.0f;
	CTexture Texture;
	{
		CDataFile ParamDataFile;
		ParamDataFile.Open(pArg->m_lParamData);
		if(!ParamDataFile.GetNext("QUALITY"))
			return TASK_RETURN_ERROR;
		ParamDataFile.GetFile()->ReadLE(Quality);
		Texture.SerializeRead(&ParamDataFile);
	}

	switch(Texture.m_Properties.m_TextureVersion)
	{
	case CTC_TEXTUREVERSION_S3TC:
		{
			LogFile(CStrF("XTX: Compressing %s using S3TC", Texture.m_Name));
			Texture.Compress(IMAGE_MEM_COMPRESSTYPE_S3TC, Quality);
			break;
		}
	case CTC_TEXTUREVERSION_3DC:
		{
			LogFile(CStrF("XTX: Compressing %s using 3DC", Texture.m_Name));
			Texture.Compress(IMAGE_MEM_COMPRESSTYPE_3DC, Quality);
			break;
		}
	case CTC_TEXTUREVERSION_CTX:
		{
			LogFile(CStrF("XTX: Compressing %s using CTX", Texture.m_Name));
			Texture.Compress(IMAGE_MEM_COMPRESSTYPE_CTX, Quality);
			break;
		}
	}

	{
		CDataFile OutputDataFile;
		OutputDataFile.Create(pArg->m_lOutput);
		Texture.SerializeWrite(&OutputDataFile);
		OutputDataFile.Close();
	}

	return TASK_RETURN_FINISHED;
}

int MRTC_TaskCompressTextureContainer::Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
{
	MRTC_TaskArgCompressTextureContainer* pArg = TDynamicCast<MRTC_TaskArgCompressTextureContainer>(_pArg);
	if(!pArg) return TASK_RETURN_ERROR;
	for(int i = 0; i < pArg->m_pContainer->GetNumLocal(); i++)
	{
		CTexture* pTex = pArg->m_pContainer->GetTextureMap(i, CTC_TEXTUREVERSION_ANY);
		if( !(pTex->m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS))
		{
			spMRTC_TaskInstance spTask;
//			if(_pTask->m_pManager->NetworkEnabled())
			{
				// If we support network then spawn a MRTC_TaskHostCompressTexture otherwise spawn a MRTC_TaskCompressTexture
				spTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskHostCompressTexture", MNew2(MRTC_TaskArgHostCompressTexture, pTex, pArg->m_Quality));
			}
//			else
//			{
//				spTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskCompressTexture", MNew2(MRTC_TaskArgCompressTexture, pTex, pArg->m_Quality));
//			}
			_pTask->AddTask(spTask);
		}
	}
	return TASK_RETURN_INPROGRESS;
}

int MRTC_TaskConvertTextureContainer::Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
{
	MRTC_TaskArgConvertTextureContainer* pArg = TDynamicCast<MRTC_TaskArgConvertTextureContainer>(_pArg);
	if(!pArg) return TASK_RETURN_ERROR;
	for(int i = 0; i < pArg->m_pContainer->GetNumLocal(); i++)
	{
		spMRTC_TaskInstance spTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskConvertTexture", MNew3(MRTC_TaskArgConvertTexture, pArg->m_pContainer, i, pArg->m_SupportedFormat));
		_pTask->AddTask(spTask);
	}
	return TASK_RETURN_INPROGRESS;
}

int MRTC_TaskGaussFilterCubemap::Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
{
	MRTC_TaskArgGaussFilterCubemap* pArg = TDynamicCast<MRTC_TaskArgGaussFilterCubemap>(_pArg);
	if(!pArg) return TASK_RETURN_ERROR;
	try
	{
		pArg->m_pContainer->FilterCubemap(pArg->m_iTexture, pArg->m_Params);
	}
	catch(...)
	{
		return TASK_RETURN_ERROR;
	}

	return TASK_RETURN_FINISHED;
}

int MRTC_TaskSmoothCubemapEdges::Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
{
	MRTC_TaskArgSmoothCubemapEdges* pArg = TDynamicCast<MRTC_TaskArgSmoothCubemapEdges>(_pArg);
	if(!pArg) return TASK_RETURN_ERROR;

	try
	{
		pArg->m_pContainer->SmoothCubemapEdges(pArg->m_iTexture);
	}
	catch(...)
	{
		return TASK_RETURN_ERROR;
	}

	return TASK_RETURN_FINISHED;
}

int MRTC_TaskTrashCubemapNames::Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
{
	MRTC_TaskArgTrashCubemapNames* pArg = TDynamicCast<MRTC_TaskArgTrashCubemapNames>(_pArg);
	if(!pArg) return TASK_RETURN_ERROR;

	try
	{
		int nVersions = pArg->m_pContainer->EnumTextureVersions(pArg->m_iTexture, 0, CTC_TEXTUREVERSION_ANY);
		for(int iVersion = 0; iVersion < nVersions; iVersion++)
		{
			CTexture* pTex0 = pArg->m_pContainer->GetTextureMap(pArg->m_iTexture + 0 * nVersions, CTC_TEXTUREVERSION_ANY);
			CTexture* pTex1 = pArg->m_pContainer->GetTextureMap(pArg->m_iTexture + 1 * nVersions, CTC_TEXTUREVERSION_ANY);
			CTexture* pTex2 = pArg->m_pContainer->GetTextureMap(pArg->m_iTexture + 2 * nVersions, CTC_TEXTUREVERSION_ANY);
			CTexture* pTex3 = pArg->m_pContainer->GetTextureMap(pArg->m_iTexture + 3 * nVersions, CTC_TEXTUREVERSION_ANY);
			CTexture* pTex4 = pArg->m_pContainer->GetTextureMap(pArg->m_iTexture + 4 * nVersions, CTC_TEXTUREVERSION_ANY);
			CTexture* pTex5 = pArg->m_pContainer->GetTextureMap(pArg->m_iTexture + 5 * nVersions, CTC_TEXTUREVERSION_ANY);

			memcpy(pTex1->m_Name, "$$$", 3);
			memcpy(pTex2->m_Name, "$$$", 3);
			memcpy(pTex3->m_Name, "$$$", 3);
			memcpy(pTex4->m_Name, "$$$", 3);
			memcpy(pTex5->m_Name, "$$$", 3);
		}
	}
	catch(...)
	{
		return TASK_RETURN_ERROR;
	}

	return TASK_RETURN_FINISHED;
}

int MRTC_TaskTexturePostFilter::Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
{
	MRTC_TaskArgTexturePostFilter* pArg = TDynamicCast<MRTC_TaskArgTexturePostFilter>(_pArg);
	if(!pArg) return TASK_RETURN_ERROR;
	try
	{
		pArg->m_pContainer->FilterTexture(pArg->m_iTexture, pArg->m_Params);
	}
	catch(...)
	{
		return TASK_RETURN_ERROR;
	}

	return TASK_RETURN_FINISHED;
}

MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskCompressTexture, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskConvertTexture, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskCompressTextureContainer, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskConvertTextureContainer, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskGaussFilterCubemap, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskSmoothCubemapEdges, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskTrashCubemapNames, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskTexturePostFilter, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskHostCompressTexture, MRTC_RemoteTaskBase);

MRTC_IMPLEMENT(MRTC_TaskArgCompressTexture, MRTC_TaskBaseArg);
MRTC_IMPLEMENT(MRTC_TaskArgConvertTexture, MRTC_TaskBaseArg);
MRTC_IMPLEMENT(MRTC_TaskArgCompressTextureContainer, MRTC_TaskBaseArg);
MRTC_IMPLEMENT(MRTC_TaskArgConvertTextureContainer, MRTC_TaskBaseArg);
MRTC_IMPLEMENT(MRTC_TaskArgGaussFilterCubemap, MRTC_TaskBaseArg);
MRTC_IMPLEMENT(MRTC_TaskArgTexturePostFilter, MRTC_TaskBaseArg);
MRTC_IMPLEMENT(MRTC_TaskArgSmoothCubemapEdges, MRTC_TaskBaseArg);
MRTC_IMPLEMENT(MRTC_TaskArgTrashCubemapNames, MRTC_TaskBaseArg);
MRTC_IMPLEMENT(MRTC_TaskArgHostCompressTexture, MRTC_TaskBaseArg);

void CTextureContainer_Plain::XTXCompile(int _Flags, const CRegistry &_Reg, CStr _SourcePath, CXTXCompileResult& _Result)
{
	MAUTOSTRIP(CTextureContainer_Plain_XTXCompile, MAUTOSTRIP_VOID);
	MACRO_GetSystem;

	int platform = D_MPLATFORM;

	int QualityOverride = _Flags & XTX_COMPILE_QUALITY_MASK;
	CStr Opt = pSys->GetEnvironment()->GetValue("XWC_TCOPT", "HQNP").UpperCase();
	if (strstr(Opt.Str(), "LQ"))
		QualityOverride = 1;
	else if (strstr(Opt.Str(), "HQNP"))
		QualityOverride = 3;
	else if (strstr(Opt.Str(), "HQ"))
		QualityOverride = 2;

	if(_SourcePath != "" && _SourcePath[_SourcePath.Len() - 1] != '\\')
		_SourcePath += "\\";

	int Compression = _Reg.GetValuei("COMPRESSION", 80);
	int iPicmip = _Reg.GetValuei("PICMIP");

	TThinArray<uint8> lTexturesDone;
	lTexturesDone.SetLen(_Reg.GetNumChildren());
	FillChar(lTexturesDone.GetBasePtr(), lTexturesDone.ListSize(), 0);

	int RegNumChildren = _Reg.GetNumChildren();

	// Fallback if no taskmanager exists
	MRTC_TaskManager DummyManager;

	MRTC_TaskManager* pTaskManager = TDynamicCast<MRTC_TaskManager>((CReferenceCount*)MRTC_GOM()->GetRegisteredObject("SYSTEM.TASKMANAGER"));
	if(!pTaskManager)
		pTaskManager	= &DummyManager;

	spMRTC_TaskInstance spCubemapGaussGroupTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskGroup", 0);
	spMRTC_TaskInstance spPostprocessGroupTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskGroup", 0);
	spMRTC_TaskInstance spCubemapEdgeGroupTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskGroup", 0);
	spMRTC_TaskInstance spCubemapTrashNamesGroupTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskGroup", 0);
	bool bOnlyNormal = true;
	for (int j = 0; j < 2; ++j)
	{
		MRTC_InitProgressCount(RegNumChildren, "Compiling " + _SourcePath + "...");
		for(int i = 0; i < RegNumChildren; i++)
		{
			MRTC_IncProgress(NULL);

			if (lTexturesDone[i])
				continue;

			const CRegistry *pChild = _Reg.GetChild(i);
			if(pChild->GetThisName().CompareNoCase("TEXTURE") == 0)
			{
				int ConvertType = (_Flags & 8) ? CTC_CONVERT_NONE : CTC_CONVERT_DEFAULT;

				if (pChild->GetValue("CONVERT") != "")
				{
					ConvertType = pChild->GetValue("CONVERT").TranslateInt(CTC_TextureProperties::ms_TxtConvertTranslate);
				}

				CStr Path = pChild->GetValue("PATH");

				CStr Name = pChild->GetValue("NAME");
				if (Name == "")
					Name = Path.GetFilenameNoExt();

				if(Name != "")
				{
					CTC_TextureProperties Properties;
					CTC_TextureProperties CompileProperties;
					{
						for(int i = 0; i < pChild->GetNumChildren(); i++)
							Properties.Parse_XRG(*pChild->GetChild(i));
					}
					CompileProperties	= Properties;
					uint32 CompileChecksum = GenerateChecksum(pChild);
					
					int iTxt[5] = {-1, -1, -1, -1, -1};
					int iCurrentTxt = 0;
	#ifdef MERGE_NORMAL_SPECULAR
					if (Properties.m_Flags & CTC_TEXTUREFLAGS_NORMALMAP)
					{
						LogFile(CStrF("XTX: Adding texture %s (%s)", Name.Str(), Path.Str()));

						CStr NMapName = Path.GetFilenameNoExt();
						int UnderScore = NMapName.FindReverse("_");

						if (UnderScore >= 0)
						{
							CStr NMapNameCut = NMapName.Copy(0, UnderScore);
							CStr SMapPath;
							int iSMap = -1;

							CStr SMapName = NMapNameCut + "_S";
							CTC_TextureProperties SProperties;

							for(int j = 0; j < RegNumChildren; j++)
							{
								const CRegistry *pSChild = _Reg.GetChild(j);
								if(pSChild->GetThisName().CompareNoCase("TEXTURE") == 0)
								{
									CStr SPath = pSChild->GetValue("PATH");
									if ((SPath.GetFilenameNoExt()).CompareNoCase(SMapName) == 0)
									{
										{
											for(int i = 0; i < pSChild->GetNumChildren(); i++)
												SProperties.Parse_XRG(*pSChild->GetChild(i));
										}

										SMapPath = _SourcePath + SPath;
										iSMap = j;
										break;
									}
								}
							}

							if (SMapPath != "" && iSMap >= 0)
							{
								TArray<CCMap> lCMaps;
								TArray<CTC_TextureProperties> lCMapProperties;
								TArray<uint32> lCMapChecksums;

								if (QualityOverride == 1)
								{
									Properties.m_Flags &= (~CTC_TEXTUREFLAGS_HIGHQUALITY);
									Properties.m_Flags |= CTC_TEXTUREFLAGS_PALETTE;
								}
								else if (QualityOverride == 2)
								{
									Properties.m_Flags |= CTC_TEXTUREFLAGS_HIGHQUALITY;
								}
								else if (QualityOverride == 3)
								{
									Properties.m_Flags |= CTC_TEXTUREFLAGS_HIGHQUALITY;
									Properties.m_Flags &= (~CTC_TEXTUREFLAGS_PALETTE);
								}

								if ( (platform != 2) && (Properties.m_Flags & CTC_TEXTUREFLAGS_PALETTE) && (Properties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY) )
								{
									bool bQuit = false;
									// Find all color maps for this normal map and put the specular in the color alpha
									int iCurrentCheck = 0;
									while (iCurrentCheck <= 9)
									{
										CCMap CurrentCMap;
										uint32 Checksum = 0;
										
										CStr CMapName;

										int iFindNumber = 1;
										while (1)
										{
											if (iCurrentCheck == 0)
												CMapName = NMapNameCut + "_C";
											else if (iCurrentCheck == 1)
												CMapName = NMapNameCut + CStrF("_C%02d", iFindNumber);
											else if (iCurrentCheck == 2)
												CMapName = NMapNameCut + CStrF("_C%d", iFindNumber);
											else if (iCurrentCheck == 3)
												CMapName = NMapNameCut + CStrF("_%02d_C", iFindNumber);
											else if (iCurrentCheck == 4)
												CMapName = NMapNameCut + CStrF("_%d_C", iFindNumber);
											else if (iCurrentCheck == 5)
												CMapName = NMapNameCut + "_D";
											else if (iCurrentCheck == 6)
												CMapName = NMapNameCut + CStrF("_D%02d", iFindNumber);
											else if (iCurrentCheck == 7)
												CMapName = NMapNameCut + CStrF("_D%d", iFindNumber);
											else if (iCurrentCheck == 8)
												CMapName = NMapNameCut + CStrF("_%02d_D", iFindNumber);
											else if (iCurrentCheck == 9)
												CMapName = NMapNameCut + CStrF("_%d_D", iFindNumber);

//											LogFile(CStrF("    Looking for %s", CMapName.Str()));

											CStr CMapPath;

											int d = 0;

											for(; d < RegNumChildren; d++)
											{
												const CRegistry *pDChild = _Reg.GetChild(d);
												if(pDChild->GetThisName().CompareNoCase("TEXTURE") == 0)
												{													
													CStr CPath = pDChild->GetValue("PATH");
													if ((CPath.GetFilenameNoExt()).CompareNoCase(CMapName) == 0)
													{
														{
															for(int i = 0; i < pDChild->GetNumChildren(); i++)
																CurrentCMap.m_Properties.Parse_XRG(*pDChild->GetChild(i));
														}
														Checksum	= GenerateChecksum(pDChild);

														CMapPath = _SourcePath + CPath;
														CurrentCMap.m_Name = pDChild->GetValue("NAME");
														if (CurrentCMap.m_Name == "")
															CurrentCMap.m_Name = CMapPath.GetFilenameNoExt();

														break;
													}
												}
											}
											CurrentCMap.m_iCMap = d;

											if (CMapPath != "")
											{
												// Merge specular map with color map

            									CurrentCMap.m_spMap = ReadImage(CMapPath);

												if (CurrentCMap.m_spMap->GetFormat() & IMAGE_FORMAT_ALPHA)
												{
													bQuit = true;
													LogFile("XTX: WARNING: A diffuse map already have a alpha channel, reverting to non HQ.");
													Properties.m_Flags &= ~CTC_TEXTUREFLAGS_HIGHQUALITY;

													lCMaps.Clear();
													lCMapProperties.Clear();
													lCMapChecksums.Clear();
													break;
												}

												lCMaps.Add(CurrentCMap);
												lCMapProperties.Add(CurrentCMap.m_Properties);
												lCMapChecksums.Add(Checksum);
											}
											else if (iFindNumber > 4)
												break;

											++iFindNumber;

											if (iCurrentCheck == 0 || iCurrentCheck == 5)
												break;
										}

										if (bQuit)
											break;
										++iCurrentCheck;
									}

									if (!lCMaps.Len() && !bQuit)
									{
										LogFile("XTX: ERROR: Found no diffuse map to put specular map in, reverting to non HQ");
										Properties.m_Flags &= ~CTC_TEXTUREFLAGS_HIGHQUALITY;
									}

								}

								spCImage spNMap = ReadImage(_SourcePath + Path);
								spCImage spSMap = ReadImage(SMapPath);
								if (platform == 2)
								{
									lTexturesDone[i] = 1;
									lTexturesDone[iSMap] = 1;
									iTxt[iCurrentTxt++] = -2;
								}
								else
								{
									//if (!(_Flags & 8))
									// Never allow compressed normal maps

									Properties.m_Flags |= CTC_TEXTUREFLAGS_NOCOMPRESS;

									bool bMergeMaps = true;
									if (Properties.m_Flags & CTC_TEXTUREFLAGS_PALETTE)
									{
										if (Properties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY)
											bMergeMaps = false;

									}

									if (bMergeMaps)
									{
										LogFile(CStrF("XTX:        Merging %s with %s", Name.Str(), SMapName.Str()));

										spCImage spMerged = MergeNormalSpecMaps(spNMap, spSMap);
										if ((Properties.m_Flags & CTC_TEXTUREFLAGS_PALETTE) && !(SProperties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY) && (!(_Flags & 8)))
										{
											// Reduce specular depth to 16 shades

											M_ASSERT(spMerged->GetFormat() & IMAGE_FORMAT_BGRA8, "Must be RGBA32");

											CPixel32 *pPixel = (CPixel32 *)spMerged->Lock();
											int Size = spMerged->GetWidth() * spMerged->GetHeight();

											for (int i = 0; i < Size; ++i)
											{
												pPixel[i].A() = (pPixel[i].A() / 16) * 17;
											}

											spMerged->Unlock();
										}

										Properties.m_iPicMipGroup += 8;
										Properties.m_TextureVersion	= CTC_TEXTUREVERSION_RAW;
										if (!(_Flags & 8))
											iTxt[iCurrentTxt++] = AddTexture(spMerged, Properties, Name, CTC_CONVERT_SELECTNORMALMAPFORMAT);
										else
											iTxt[iCurrentTxt++] = AddTexture(spMerged, Properties, Name, CTC_CONVERT_NONE);
										m_lTextureCompileChecksums.Add(CompileChecksum);
										lTexturesDone[iSMap] = 1;
									}
									else
									{
										Properties.m_iPicMipGroup += 8;
										Properties.m_TextureVersion	= CTC_TEXTUREVERSION_RAW;
										if (!(_Flags & 8))
											iTxt[iCurrentTxt++] = AddTexture(spNMap, Properties, Name, CTC_CONVERT_SELECTNORMALMAPFORMAT);
										else
											iTxt[iCurrentTxt++] = AddTexture(spNMap, Properties, Name, CTC_CONVERT_NONE);
										m_lTextureCompileChecksums.Add(CompileChecksum);
										lTexturesDone[iSMap] = 1;
									}

								}

								if (lCMaps.Len())
								{
									for (int i = 0; i < lCMaps.Len(); ++i)
									{
										if (!lTexturesDone[lCMaps[i].m_iCMap])
										{
											spCImage spMerged = MergeNormalSpecMaps(lCMaps[i].m_spMap, spSMap);
											LogFile(CStrF("XTX:    Merging specular (%s) with color map (%s)", SMapName.Str(), lCMaps[i].m_Name.Str()));


											lCMaps[i].m_Properties.m_TextureVersion	= CTC_TEXTUREVERSION_RAW;
											if (!(_Flags & 8))
												AddTexture(spMerged, lCMaps[i].m_Properties, lCMaps[i].m_Name, CTC_CONVERT_DEFAULT);
											else
												AddTexture(spMerged, lCMaps[i].m_Properties, lCMaps[i].m_Name, CTC_CONVERT_NONE);
											m_lTextureCompileChecksums.Add(lCMapChecksums[i]);

											lTexturesDone[lCMaps[i].m_iCMap] = 1;
										}
									}
								}
							}
							else
							{
								spCImage spNMap = ReadImage(_SourcePath + Path);
								Properties.m_iPicMipGroup += 8;
								Properties.m_TextureVersion	= CTC_TEXTUREVERSION_RAW;
								if (!(_Flags & 8))
									iTxt[iCurrentTxt++] = AddTexture(spNMap, Properties, Name, CTC_CONVERT_SELECTNORMALMAPFORMAT);
								else
									iTxt[iCurrentTxt++] = AddTexture(spNMap, Properties, Name, CTC_CONVERT_NONE);
								m_lTextureCompileChecksums.Add(CompileChecksum);
								lTexturesDone[iTxt[iCurrentTxt-1]] = 1;
							}
						}
					}
					else if (bOnlyNormal)
						continue;
	#endif
					if(Properties.m_Flags & (CTC_TEXTUREFLAGS_CUBEMAP | CTC_TEXTUREFLAGS_CUBEMAPCHAIN))
					{
						spMRTC_TaskInstance spCubemapEdgeTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskSmoothCubemapEdges", MNew2(MRTC_TaskArgSmoothCubemapEdges, this, m_lspTextures.Len()));
						spCubemapEdgeGroupTask->AddTask(spCubemapEdgeTask);
					}
					if(Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN)
					{
						spMRTC_TaskInstance spCubemapNameTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskTrashCubemapNames", MNew2(MRTC_TaskArgTrashCubemapNames, this, m_lspTextures.Len()));
						spCubemapTrashNamesGroupTask->AddTask(spCubemapNameTask);
					}
					LogFile(CStrF("XTX: Adding texture %s (%s)", Name.Str(), Path.Str()));
					if (iCurrentTxt == 0)
					{
						spCImage spImage;
						try
						{
							spImage = ReadImage(_SourcePath + Path);
						}
						catch(CCExceptionFile)
						{
							spImage = NULL;
						}

						if (spImage != NULL)
						{
							int iTransform = CStr::TranslateFlags(pChild->GetValue("TRANSFORM"), ms_ImageTransform);
							if(iTransform != 0)
							{
								// Transform image (most likely a cubemap)
								LogFile(CStrF("XTX: Applying transform '%s' to image %s (%s)", pChild->GetValue("TRANSFORM").Str(), Name.Str(), Path.Str()));
								spImage = spImage->Transform(iTransform);
							}

							CStr CubeFilter = pChild->GetValue("CUBEFILTER");
							if(CubeFilter != "")
							{
								if(Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN)
								{
									CTC_CubeFilterParams FilterParams;
									FilterParams.m_Type	= 0;
									FilterParams.m_aParams[0]	= CubeFilter.Val_fp64();

									spMRTC_TaskInstance spGaussFilterTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskGaussFilterCubemap", MNew3(MRTC_TaskArgGaussFilterCubemap, this, m_lspTextures.Len(), FilterParams));
									spCubemapGaussGroupTask->AddTask(spGaussFilterTask);
								}
								else
									LogFile("XTX: CUBEFILTER option is only allowed on CUBEMAPCHAIN texture");
							}

							const CRegistry* pPostFilter = pChild->FindChild("POSTFILTER");
							if(pPostFilter)
							{
								CTC_PostFilterParams FilterParams;
								FilterParams.m_spFilters	= pPostFilter->Duplicate();

								spMRTC_TaskInstance spPostFilterTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskTexturePostFilter", MNew3(MRTC_TaskArgTexturePostFilter, this, m_lspTextures.Len(), FilterParams));
								spPostprocessGroupTask->AddTask(spPostFilterTask);
							}

							if(Properties.m_Flags & CTC_TEXTUREFLAGS_NORMALMAP)
							{
								CTC_TextureProperties Properties1, Properties2, Properties3, Properties4;
								
								if(Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS)
								{
									Properties1 = Properties;
									Properties1.m_Flags	|= CTC_TEXTUREFLAGS_NOCOMPRESS;
									Properties1.m_TextureVersion	= CTC_TEXTUREVERSION_RAW;
									AddTexture(spImage, Properties1, Name, CTC_CONVERT_GB2IA);
									m_lTextureCompileChecksums.Add(CompileChecksum);
								}
								else
								{
									if(!(Properties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY))
									{
										if( spImage->CanCompress(IMAGE_MEM_COMPRESSTYPE_CTX ) )
										{
											Properties4 = Properties;
											Properties4.m_Flags	&= ~CTC_TEXTUREFLAGS_NOCOMPRESS;
											Properties4.m_TextureVersion	= CTC_TEXTUREVERSION_CTX;
											AddTexture(spImage, Properties4, Name, CTC_CONVERT_GB2IA);
											m_lTextureCompileChecksums.Add(CompileChecksum);
										}
									}
									if( spImage->CanCompress(IMAGE_MEM_COMPRESSTYPE_3DC) )
									{
										Properties3 = Properties;
										Properties3.m_Flags	&= ~CTC_TEXTUREFLAGS_NOCOMPRESS;
										Properties3.m_TextureVersion	= CTC_TEXTUREVERSION_3DC;
										AddTexture(spImage, Properties3, Name, CTC_CONVERT_GB2IA);
										m_lTextureCompileChecksums.Add(CompileChecksum);
									}
									if(spImage->CanCompress(IMAGE_MEM_COMPRESSTYPE_S3TC))
									{
										Properties2 = Properties;
										Properties2.m_Flags &= ~CTC_TEXTUREFLAGS_NOCOMPRESS;
										Properties2.m_TextureVersion	= CTC_TEXTUREVERSION_S3TC;
										AddTexture(spImage, Properties2, Name, CTC_CONVERT_GB2GA);
										m_lTextureCompileChecksums.Add(CompileChecksum);
									}
								}
							}
							else
							{
								CTC_TextureProperties Properties1, Properties2;
								Properties1 = Properties;
								Properties1.m_Flags	|= CTC_TEXTUREFLAGS_NOCOMPRESS;
								Properties1.m_TextureVersion	= CTC_TEXTUREVERSION_RAW;
								Properties2 = Properties;
								Properties2.m_Flags	&= ~CTC_TEXTUREFLAGS_NOCOMPRESS;
								Properties2.m_TextureVersion	= CTC_TEXTUREVERSION_S3TC;

								//"CubeFromCylinder" - type adds 4/6 new textures
								if( (ConvertType == CTC_CONVERT_CUBEFROMCYLINDER) || (ConvertType == CTC_CONVERT_CUBEFROMSPHERE) )
								{
									CTEX_CubeFromCylinderSettings Settings;
									uint8 liImageOrder[6] =	{ 0,2,3,1,4,5 };
									Settings.m_bIsEnvMap = (Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN) == CTC_TEXTUREFLAGS_CUBEMAPCHAIN;

									Settings.m_Height = pChild->GetValuei("HEIGHT",0);
									Settings.m_Width = pChild->GetValuei("WIDTH",0);
									Settings.m_ViewHeight = 1.0f - pChild->GetValuef("VIEW",0.5f);

									int nSides = ((pChild->GetValuei("CUSTOMTOPBTM",0) != 0) && (ConvertType != CTC_CONVERT_CUBEFROMSPHERE)) ? 4 : 6;

									for(int k = 0;k < nSides;k++)
									{
										Settings.m_iSide = liImageOrder[k];
										if(!(Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS) && spImage->CanCompress(IMAGE_MEM_COMPRESSTYPE_S3TC))
										{
											iTxt[iCurrentTxt++] = AddTexture(spImage, Properties2, Name + CStrF("_0%d",k), ConvertType, &Settings);
											m_lTextureCompileChecksums.Add(CompileChecksum);
										}
										else
										{
											iTxt[iCurrentTxt++] = AddTexture(spImage, Properties1, Name + CStrF("_0%d",k), ConvertType, &Settings);
											m_lTextureCompileChecksums.Add(CompileChecksum);
										}

										//Clear flag- we only need this once
										if( Properties1.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN )
										{
											Properties1.m_Flags &= ~CTC_TEXTUREFLAGS_CUBEMAPCHAIN;
											Properties2.m_Flags &= ~CTC_TEXTUREFLAGS_CUBEMAPCHAIN;
										}
									}
								}
								else
								{
									fp32 BumpScale = pChild->GetValuef("BUMPSCALE", 1.0f);

									if(!(Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS) && spImage->CanCompress(IMAGE_MEM_COMPRESSTYPE_S3TC))
									{
										iTxt[iCurrentTxt++] = AddTexture(spImage, Properties2, Name, ConvertType, &BumpScale);
										m_lTextureCompileChecksums.Add(CompileChecksum);
									}
									else
									{
										iTxt[iCurrentTxt++] = AddTexture(spImage, Properties1, Name, ConvertType, &BumpScale);
										m_lTextureCompileChecksums.Add(CompileChecksum);
									}
								}
							}
						}
					}

					lTexturesDone[i] = 1;
				}
			}
		}
		bOnlyNormal = false;
	}

	// ----------------------------------------------------------------
	// Create original texture containers?

	if (!(_Flags & 8))
	{
		// Create conversion task (converts textures to supported formats)
		spMRTC_TaskInstance spConvertTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskConvertTextureContainer", MNew2(MRTC_TaskArgConvertTextureContainer, this , 0));

		if(Compression > 0)
		{
			// Create compression task
			spMRTC_TaskInstance spCompressTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_TaskCompressTextureContainer", MNew2(MRTC_TaskArgCompressTextureContainer, this, (fp32)Compression * 0.01f));

			spCompressTask->DependOn(spPostprocessGroupTask);
			spConvertTask->DependOn(spCompressTask);
			pTaskManager->AddTask(spCompressTask);
		}

		// Must add this task after the convert task so it won't get started before the dependency data has been generated
		pTaskManager->AddTask(spConvertTask);
	}

	// Post-process must be run after cubemap gauss filtering
	spPostprocessGroupTask->DependOn(spCubemapGaussGroupTask);
	spCubemapEdgeGroupTask->DependOn(spPostprocessGroupTask);
	spCubemapTrashNamesGroupTask->DependOn(spCubemapEdgeGroupTask);
	pTaskManager->AddTask(spCubemapGaussGroupTask);
	pTaskManager->AddTask(spPostprocessGroupTask);
	pTaskManager->AddTask(spCubemapEdgeGroupTask);
	pTaskManager->AddTask(spCubemapTrashNamesGroupTask);

	pTaskManager->Host_BlockUntilDone();
}

// ----------------------------------------------------------------
//  CTextureContainer_Plain WAD2-IO
// ----------------------------------------------------------------
class CWAD2_Header
{
public:
	char m_Sign[4];             // "WAD2"
	int32 m_nEntries;			// Number of entries
	int32 m_DirOffset;			// Position of WAD directory in file
};

class CWAD2_Entry
{
public:
	int32 m_FileOffset;
	int32 m_Size;
	int32 m_SizeInMemory;
	uint8 m_Type;
	uint8 m_Compression;
	uint16 __m_Padding;
	char m_Name[16];
};

class CWAD2_MipTexture
{
public:
	char m_Name[16];
	int32 m_Width;
	int32 m_Height;
	int32 m_RelOffsetMip0;	// Relative to start of this header
	int32 m_RelOffsetMip1;
	int32 m_RelOffsetMip2;
	int32 m_RelOffsetMip3;
};

void CTextureContainer_Plain::AddFromWAD(CStr _FileName)
{
	MAUTOSTRIP(CTextureContainer_Plain_AddFromWAD, MAUTOSTRIP_VOID);

	TPtr<CCFile> spFile = MNew(CCFile);
	if (spFile == NULL) MemError("AddFromWAD");

	spFile->Open(_FileName, CFILE_BINARY | CFILE_READ);

	// Read header and check if its a WAD2.
	CWAD2_Header Header;
	spFile->Read(&Header, sizeof(Header));
	if (memcmp(&Header.m_Sign, "WAD2", 4) != 0) Error("AddFromWAD", "Not a WAD2 file.");

	// Read entry directory.
	int nEntries = Header.m_nEntries;

	TArray<CWAD2_Entry> m_lEntries;
	m_lEntries.SetLen(nEntries);
	spFile->Seek(Header.m_DirOffset);
	if (nEntries) spFile->Read(&m_lEntries[0], m_lEntries.ListSize());

	// Read all entries that are textures & palettes
	int nLoaded = 0;
	int iCurPal = -1;
	for(int iEntry = 0; iEntry < nEntries; iEntry++)
	{
		CWAD2_Entry* pE = &m_lEntries[iEntry];
		CStr Name = (char*) &pE->m_Name;
//		LogFile(CStrF("Entry: %d, %.2x, %.2x, %.8x,  ", iEntry, pE->m_Type, pE->m_Compression, pE->m_FileOffset) + Name);
		switch(pE->m_Type)
		{
		case 0x44 :
			{
				if (iCurPal < 0)
				{
					CStr PalPath = _FileName.GetPath() + _FileName.GetFilenameNoExt() + ".pal";
					if(!CDiskUtil::FileExists(PalPath))
						PalPath = _FileName.GetPath() + "default.pal";
					if(CDiskUtil::FileExists(PalPath))
					{
						spCImagePalette spPal = MNew(CImagePalette);
						if (spPal == NULL) MemError("AddFromWAD");
						CCFile PalFile;
						PalFile.Open(PalPath, CFILE_BINARY | CFILE_READ);

						uint8 Pal[768];
						CPixel32 Pal32[768];
						PalFile.Read(&Pal, sizeof(Pal));

						for(int i = 0; i < 256; i++)
							Pal32[i] = CPixel32(Pal[i*3], Pal[i*3+1], Pal[i*3+2]);

						spPal->SetPalette((CPixel32*) &Pal32, 0, 256);
						iCurPal = m_lspPalettes.Add(spPal);
					}
					else
						Error("AddFromWAD", CStrF("Could not find palette from either the WAD or as separate palette (defualt.pal or %s).", PalPath));
					break;
				}
				spFile->Seek(pE->m_FileOffset);
				CWAD2_MipTexture Txt;
				spFile->Read(&Txt, sizeof(Txt));
				int w = Txt.m_Width;
				int h = Txt.m_Height;
				if ((w <= 0) || (w > 8192) || (h <= 0) || (h > 8192))
					Error("AddFromWAD", "Strange texture size.");
				spCImage spImg = MNew(CImage);
				if (spImg == NULL) MemError("AddFromWAD");
				spImg->Create(GetGEPow2(w), GetGEPow2(h), IMAGE_FORMAT_CLUT8, IMAGE_MEM_IMAGE, m_lspPalettes[iCurPal]);
				spImg->Fill(spImg->GetClipRect(), 0);

				spFile->RelSeek(Txt.m_RelOffsetMip0 - sizeof(Txt));
				uint8* pTxt = DNew(uint8) uint8[w*h];
				if (!pTxt) MemError("AddFromWAD");
				try
				{
					spFile->Read(pTxt, w*h);
					for(int y = 0; y < h; y++)
						spImg->SetRAWData(CPnt(0, y), w, &pTxt[w*y]);
				}
				catch(CCException)
				{
					if (pTxt) delete[] pTxt;
					pTxt = NULL;
					throw;
				}
				if (pTxt) delete[] pTxt;
				pTxt = NULL;

				spCTexture spTxt = MNew(CTexture);
				if (spTxt == NULL) MemError("AddFromWAD");
				
				spTxt->Create(spImg, CTexture::GetMipMapLevels(spImg->GetWidth(), spImg->GetHeight()), IMAGE_FORMAT_BGRX8);
				spTxt->m_iPalette= iCurPal;
				{
					CStr TmpStr((char*)&pE->m_Name);
					TmpStr.UpperCase();
					if (TmpStr.Len() > 31) Error("AddFromWAD", "Too long texture-name.");
#ifdef USE_HASHED_TEXTURENAME
					spTxt->m_NameID = StringToHash(TmpStr);
#else
					strcpy(spTxt->m_Name, (char*)TmpStr);
#endif
				}
//				spTxt->m_Name = CStr((char*)&pE->m_Name).UpperCase();

				AddTexture(spTxt);
				nLoaded++;
				break;
			};

		case 0x40 :
			{
				spCImagePalette spPal = MNew(CImagePalette);
				if (spPal == NULL) MemError("AddFromWAD");

				uint8 Pal[768];
				CPixel32 Pal32[768];
				spFile->Seek(pE->m_FileOffset);
				spFile->Read(&Pal, sizeof(Pal));

				for(int i = 0; i < 256; i++)
					Pal32[i] = CPixel32(Pal[i*3], Pal[i*3+1], Pal[i*3+2]);

				spPal->SetPalette((CPixel32*) &Pal32, 0, 256);
				iCurPal = m_lspPalettes.Add(spPal);
				break;
			}
		}
	}

	spFile->Close();
}

void CTextureContainer_Plain::WriteWAD(CStr _FileName)
{
#ifdef USE_HASHED_TEXTURENAME
	Error("CTextureContainer_Plain::WriteWAD", "Can't write!");
#else
	MAUTOSTRIP(CTextureContainer_Plain_WriteWAD, MAUTOSTRIP_VOID);

	if (m_lspPalettes.Len() != 1)
		Error("WriteWAD", "WADs must have exactly one palette.");

	TArray<CWAD2_Entry> lEntries;

	TPtr<CCFile> spFile = MNew(CCFile);
	if (spFile == NULL) MemError("Read");

	spFile->Open(_FileName, CFILE_BINARY | CFILE_WRITE);

	// Write header
	CWAD2_Header Header;
	memcpy(&Header.m_Sign, "WAD2", 4);
	int HeaderOffs = spFile->Pos();
	Header.m_nEntries = m_lspTextures.Len() + 1;
	Header.m_DirOffset = 0;
	spFile->Write(&Header, sizeof(Header));

	// Write palette
	{
		CWAD2_Entry E;
		E.m_FileOffset = spFile->Pos();
		E.m_Size = 768;
		E.m_SizeInMemory = 768;
		E.m_Compression = 0;
		E.m_Type = 0x40;
		E.__m_Padding = 0;
		memcpy(&E.m_Name, "PALETTE", 8);
		lEntries.Add(E);

		uint8 Pal[768];
		CPixel32 Pal32[768];
		m_lspPalettes[0]->GetPalette((CPixel32*) &Pal32, 0, 256);

		for(int i = 0; i < 256; i++)
		{
			Pal[i*3] = Pal32[i].GetR();
			Pal[i*3+1] = Pal32[i].GetG();
			Pal[i*3+2] = Pal32[i].GetB();
		}

		spFile->Write(&Pal, sizeof(Pal));
	}
	
	// Write textures
	for(int iTxt = 0; iTxt < m_lspTextures.Len(); iTxt++)
	{
		CTexture* pTxt = m_lspTextures[iTxt];
		CImage* pImg = pTxt->m_lspMaps[0];
		int Width = pImg->GetWidth();
		int Height = pImg->GetHeight();
		int Format = pImg->GetFormat();
		int TxtSize = Width * Height * pImg->GetPixelSize();
		CWAD2_Entry E;
		FillChar(&E, sizeof(E), 0);
		E.m_FileOffset = spFile->Pos();
		E.m_Size = TxtSize + (TxtSize >> 2) + (TxtSize >> 4) + (TxtSize >> 6) + sizeof(CWAD2_MipTexture);
		E.m_SizeInMemory = TxtSize + (TxtSize >> 2) + (TxtSize >> 4) + (TxtSize >> 6);
		E.m_Compression = 0;
		E.m_Type = 0x44;
		E.__m_Padding = 0;
		strcpy((char*)&E.m_Name, (char*) pTxt->m_Name);
		lEntries.Add(E);

		CWAD2_MipTexture TxtH;
//		memcpy(&TxtH.m_Name, (char*)m_lTextureNames[iTxt], m_lTextureNames[iTxt].Len()+1);
		strcpy((char*)&TxtH.m_Name, (char*) pTxt->m_Name);
		TxtH.m_Width = Width;
		TxtH.m_Height = Height;
		TxtH.m_RelOffsetMip0 = sizeof(CWAD2_MipTexture);
		TxtH.m_RelOffsetMip1 = sizeof(CWAD2_MipTexture) + TxtSize;
		TxtH.m_RelOffsetMip2 = sizeof(CWAD2_MipTexture) + TxtSize + (TxtSize >> 2);
		TxtH.m_RelOffsetMip3 = sizeof(CWAD2_MipTexture) + TxtSize + (TxtSize >> 2) + (TxtSize >> 4);
		spFile->Write(&TxtH, sizeof(TxtH));

		pImg->WriteRAW(spFile);
		spCImage spMip1 = MNew(CImage);
		if (spMip1 == NULL) MemError("Write");
		spCImage spMip2 = MNew(CImage);
		if (spMip2 == NULL) MemError("Write");

		spMip1->Create(Width >> 1, Height >> 1, Format, IMAGE_MEM_IMAGE, pImg->GetPalette());
		CImage::StretchHalf(pImg, spMip1);
		spMip1->WriteRAW(spFile);

		spMip2->Create(Width >> 2, Height >> 2, Format, IMAGE_MEM_IMAGE, pImg->GetPalette());
		CImage::StretchHalf(spMip1, spMip2);
		spMip2->WriteRAW(spFile);

		spMip1->Create(Width >> 3, Height >> 3, Format, IMAGE_MEM_IMAGE, pImg->GetPalette());
		CImage::StretchHalf(spMip2, spMip1);
		spMip1->WriteRAW(spFile);
	}

	// Write entry directory
	Header.m_DirOffset = spFile->Pos();
	spFile->Write(&lEntries[0], lEntries.ListSize());

	// Re-write header.
	{
		int pos = spFile->Pos();
		spFile->Seek(HeaderOffs);
		spFile->Write(&Header, sizeof(Header));
		spFile->Seek(pos);
	}
#endif	
}

// ----------------------------------------------------------------
//  Quantize
// ----------------------------------------------------------------
void CTextureContainer_Plain::Quantize(spCImagePalette _spPal)
{
	MAUTOSTRIP(CTextureContainer_Plain_Quantize, MAUTOSTRIP_VOID);

	int nTxt = m_lspTextures.Len();
	if (!nTxt) return;

	CDefaultQuantize Quant;

	if (_spPal == NULL)
	{
		Quant.Begin();
		for(int t = 0; t < nTxt; t++)
		{
			CTexture* pTxt = m_lspTextures[t];
			for(int iMip = 0; iMip < pTxt->m_nMipmaps; iMip++)
				Quant.Include(pTxt->m_lspMaps[iMip]);
		}
		Quant.End();
	}
	else
		Quant.BuildFromPalette(_spPal->GetPalettePtr());

	for(int t = 0; t < nTxt; t++)
	{
		CTexture* pTxt = m_lspTextures[t];
		for(int iMip = 0; iMip < pTxt->m_nMipmaps; iMip++)
		{
			spCImage spImg = Quant.Quantize(pTxt->m_lspMaps[iMip]);
			pTxt->m_lspMaps[iMip] = spImg;
			pTxt->m_iPalette = 0;
		}
	}

	m_lspPalettes.Clear();
	m_lspPalettes.Add(m_lspTextures[0]->m_lspMaps[0]->GetPalette());
}

spCTextureContainer_Plain CTextureContainer_Plain::CreateSubContainer(uint8* _pTxtAddFlags, int _FlagListLen)
{
	MAUTOSTRIP(CTextureContainer_Plain_CreateSubContainer, NULL);

	TArray<uint32> lPalUsed;
	TArray<uint32> liPalNew;
	lPalUsed.SetLen(m_lspPalettes.Len());
	liPalNew.SetLen(m_lspPalettes.Len());
	if (lPalUsed.Len()) FillChar(&lPalUsed[0], lPalUsed.ListSize(), 0);

	spCTextureContainer_Plain spTC = MNew(CTextureContainer_Plain);

	int nTxt = Min(m_lspTextures.Len(), _FlagListLen);
	for(int iTxt = 0; iTxt < nTxt; iTxt++)
		if (_pTxtAddFlags[iTxt])
		{
			CTexture* pTxt = m_lspTextures[iTxt];
			int iTxtNew = spTC->AddTexture(pTxt->Duplicate());

			// Does this texture use a shared palette?
			if (pTxt->m_iPalette != -1)
			{
				int iPal = pTxt->m_iPalette;
				if (!lPalUsed[iPal])
				{
					lPalUsed[iPal] = 1;
					liPalNew[iPal] = spTC->m_lspPalettes.Add(m_lspPalettes[iPal]->Duplicate());
				}

				// Set shared palette
				spTC->m_lspTextures[iTxtNew]->SetPalette(spTC->m_lspPalettes[liPalNew[iPal]], liPalNew[iPal]);
			}
		}

	return spTC;
}

spCTextureContainer_Plain CTextureContainer_Plain::CreateSubContainer(uint32* _piTxtAdd, int _nTextures)
{
	MAUTOSTRIP(CTextureContainer_Plain_CreateSubContainer_2, NULL);

	return NULL;
}

void CTextureContainer_Plain::Compress(int _Compression, fp32 _Quality)
{
	MAUTOSTRIP(CTextureContainer_Plain_Compress, MAUTOSTRIP_VOID);

	LogFile("XTX: Compressing...");
	MRTC_InitProgressCount( m_lspTextures.Len(), "Compressing...");
	for(int i = 0; i < m_lspTextures.Len(); i++)
	{
		MRTC_IncProgress(NULL);
		if (m_lspTextures[i] != NULL)
		{
			switch( m_lspTextures[i]->m_Properties.m_TextureVersion )
			{
			case CTC_TEXTUREVERSION_S3TC:
				{
					m_lspTextures[i]->Compress(IMAGE_MEM_COMPRESSTYPE_S3TC, _Quality);
					break;
				}
			case CTC_TEXTUREVERSION_3DC:
				{
					m_lspTextures[i]->Compress(IMAGE_MEM_COMPRESSTYPE_3DC, _Quality);
					break;
				}
			case CTC_TEXTUREVERSION_CTX:
				{
					m_lspTextures[i]->Compress(IMAGE_MEM_COMPRESSTYPE_CTX, _Quality);
					break;
				}
			}
		}
	}
}

void CTextureContainer_Plain::Decompress()
{
	MAUTOSTRIP(CTextureContainer_Plain_Decompress, MAUTOSTRIP_VOID);

	LogFile("XTX: Decompressing...");
	MRTC_InitProgressCount( m_lspTextures.Len(), "Decompressing...");
	for(int i = 0; i < m_lspTextures.Len(); i++)
	{
		MRTC_IncProgress(NULL);
		if (m_lspTextures[i] != NULL) m_lspTextures[i]->Decompress();
	}
}

void CTextureContainer_Plain::Recompress( int _FromFormat, int _ToFormat, fp32 _Quality )
{
	MAUTOSTRIP(CTextureContainer_Plain_Recompress, MAUTOSTRIP_VOID );

	LogFile("XTX: Recompressing...");
	MRTC_InitProgressCount( m_lspTextures.Len(), "Recompressing...");
	for( int i = 0; i < m_lspTextures.Len(); i++ )
	{
		MRTC_IncProgress(NULL);
		if( m_lspTextures[i] != NULL )
		{
			if( ( m_lspTextures[i]->m_LargestMap.GetMemModel() & _FromFormat ) )
			{			
				m_lspTextures[i]->Decompress( true );
				m_lspTextures[i]->Compress( _ToFormat, _Quality );
			}
		}
	}
}
#endif

int CTextureContainer_Plain::GetNumTextures()
{
	MAUTOSTRIP(CTextureContainer_Plain_GetNumTextures, 0);

	return m_lspTextures.Len();
}

int CTextureContainer_Plain::GetNumLocal()
{
	MAUTOSTRIP(CTextureContainer_Plain_GetNumLocal, 0);

	return m_lspTextures.Len();
}

void CTextureContainer_Plain::Clear(bool _FreeIDs)
{
	MAUTOSTRIP(CTextureContainer_Plain_Clear, MAUTOSTRIP_VOID);

	if (_FreeIDs)
	{
		int l = m_lspTextures.Len();
		if (l > 0)
		{
			int iLastID = -1;
			for (int i = 0; i < l; i++)
			{
				if( iLastID != m_lspTextures[i]->m_TextureID )
					m_pTC->FreeID(m_lspTextures[i]->m_TextureID);
				iLastID	= m_lspTextures[i]->m_TextureID;
			}
		}
	}

	m_lspTextures.Clear();
}

#ifndef PLATFORM_CONSOLE
void CTextureContainer_Plain::DeleteTexture(int _LocalID)
{
	MAUTOSTRIP(CTextureContainer_Plain_DeleteTexture, MAUTOSTRIP_VOID);

	if (_LocalID >= m_lspTextures.Len() || _LocalID < 0)
		return;

//	DestroyHash();
	m_pTC->FreeID(_LocalID);
	m_lspTextures.Del(_LocalID);

}
#endif

int CTextureContainer_Plain::GetLocal(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_Plain_GetLocal, 0);

//	int TextureID = m_pTC->GetTextureID(_pName);
//	m_lspTextures.Len()
	

//	M_ASSERT(0, "Obsolete");

//	CreateHash();

//	if (m_spHash != NULL)
//	{
//		return m_Hash.GetIndex(_pName);
//	}
//	else
//	{
		int nTxt = m_lspTextures.Len();
#ifdef USE_HASHED_TEXTURENAME
		uint32 NameID = StringToHash(_pName);
		for (int iTxt=0; iTxt<nTxt; iTxt++)
		{
			if (m_lspTextures[iTxt]->m_NameID == NameID)
				return iTxt;
		}
#else
		for(int iTxt = 0; iTxt < nTxt; iTxt++)
		{
			if (0 == CStrBase::CompareNoCase(&m_lspTextures[iTxt]->m_Name[0], _pName))
				return iTxt;
		}
#endif		
//	}

	return -1;
}

spCTexture CTextureContainer_Plain::GetTextureMap(int _iLocal, int _TextureVersion, bool _bForceLoaded)
{
	MAUTOSTRIP(CTextureContainer_Plain_GetTexture_int_bool, NULL);

	int iLocal = GetCorrectLocal(_iLocal, _TextureVersion);

	return m_lspTextures[iLocal];
//	return m_lspTextures[_iLocal]->Duplicate();
}

// ----------------------------------------------------------------
//  CTextureContainer standard services
// ----------------------------------------------------------------
CStr CTextureContainer_Plain::GetName(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Plain_GetName, CStr());

#ifdef USE_HASHED_TEXTURENAME
	Error("CTextureContainer_Plain::GetName", "Can't get name when using hashed texturenames!");
#else	
	return m_lspTextures[_iLocal]->m_Name;
#endif
}

int CTextureContainer_Plain::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Plain_GetTextureID, 0);

	return m_lspTextures[_iLocal]->m_TextureID;
}

void CTextureContainer_Plain::SetTextureParam(int _iLocal, int _Param, int _Value)
{
	MAUTOSTRIP(CTextureContainer_Plain_SetTextureParam, MAUTOSTRIP_VOID);

	switch(_Param)
	{
	case CTC_TEXTUREPARAM_PICMIPINDEX :
		{
			CTexture& Txt = *m_lspTextures[_iLocal];
			if (Txt.m_Properties.m_iPicMipGroup != _Value)
			{
				Txt.m_Properties.m_iPicMipGroup = _Value;
				m_pTC->MakeDirty(Txt.m_TextureID);
			}
		}
		break;

	default :
		CTextureContainer::SetTextureParam(_iLocal, _Param, _Value);
	}
}

int CTextureContainer_Plain::GetTextureParam(int _iLocal, int _Param)
{
	MAUTOSTRIP(CTextureContainer_Plain_GetTextureParam, 0);

	switch(_Param)
	{
	case CTC_TEXTUREPARAM_PICMIPINDEX :
		{
			CTexture& Txt = *m_lspTextures[_iLocal];
			return Txt.m_Properties.m_iPicMipGroup;
		}
		break;

	default :
		return CTextureContainer::GetTextureParam(_iLocal, _Param);
	}
}

int CTextureContainer_Plain::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_Plain_GetTextureDesc, 0);

	CTexture* pTxt = m_lspTextures[_iLocal];
	CImage* pT = &pTxt->m_LargestMap;

	if (_pTargetImg)
		_pTargetImg->CreateVirtual(pT->GetWidth(), pT->GetHeight(), pT->GetFormat(), pT->GetMemModel(), pT->GetPalette());
	
	_Ret_nMipmaps = m_lspTextures[_iLocal]->m_nMipmaps;
	return (pT->IsCompressed()) ? 0 : CTC_TEXTURE_ACCESS;
}

void CTextureContainer_Plain::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
{
	MAUTOSTRIP(CTextureContainer_Plain_GetTextureProperties, MAUTOSTRIP_VOID);

	_Prop = m_lspTextures[_iLocal]->m_Properties;
}

CImage* CTextureContainer_Plain::GetTexture(int _iLocal, int _iMipMap, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_Plain_GetTexture_int_int, NULL);

	int iLocal = GetCorrectLocal(_iLocal, _TextureVersion);
	CTexture* pT = m_lspTextures[iLocal];
	int nMip = pT->m_nMipmaps;
	if ((_iMipMap < 0) || (_iMipMap >= nMip)) Error("GetTexture", CStrF("Invalid MIP-map number. (%d/%d, %s)", _iMipMap, nMip, (char*) pT->m_LargestMap.IDString() ));

//	if (pT->IsCompressed()) return NULL;

	if (_iMipMap == 0)
	{
		return &pT->m_LargestMap;
	} else {
		return pT->m_lspMaps[_iMipMap];
	}

}

void CTextureContainer_Plain::ReleaseTexture(int _iLocal, int _iMipMap)
{
	MAUTOSTRIP(CTextureContainer_Plain_ReleaseTexture, MAUTOSTRIP_VOID);

	// Do nothing.
}

void CTextureContainer_Plain::ReleaseTextureAllMipmaps(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Plain_ReleaseTextureAllMipmaps, MAUTOSTRIP_VOID);

	// Do nothing.
}

void CTextureContainer_Plain::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BulidFlags)
{
	MAUTOSTRIP(CTextureContainer_Plain_BuildInto, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Plain::BuildInto, CTextureContainer_Plain);

	int iLocal = GetCorrectLocal(_iLocal, _TextureVersion);
	CTexture* pT = m_lspTextures[iLocal];
	if (!pT) Error("BuildInto", "NULL texture.");

	//??? Needs to be rewritten so _ppImg includes m_LargestMap?
	int nMip = pT->m_nMipmaps;
	if (nMip < _nMipmaps+_iStartMip) Error("BuildInto", "Invalid number of mipmaps.");

	for(int i = 0; i < _nMipmaps; i++)
	{
		int iMip = i + _iStartMip;
		CImage* pImg = (iMip == 0) ?
			&pT->m_LargestMap :
			(CImage*) pT->m_lspMaps[iMip];

		if (!pImg)
			Error("BuildInto", "Image not loaded.");

		CImage Tmp;
		if (pImg->IsCompressed())
		{
			pImg->Decompress(&Tmp);
			pImg = &Tmp;
		}

		CImage::Convert(pImg, _ppImg[i], _ConvertType);
	}
}

/*
	swizzleTexturesForGC().

*/
#ifndef	PLATFORM_CONSOLE
void CTextureContainer_Plain::swizzleTexturesForGC( void )
{
	MAUTOSTRIP(CTextureContainer_Plain_swizzleTexturesForGC, MAUTOSTRIP_VOID);
	int nTextures = GetNumTextures();

	for( int iTxt=0; iTxt<nTextures; iTxt++ )
	{
		bool	bDetailTexture = false;
		spCTexture	spT = GetTextureMap( iTxt, CTC_TEXTUREVERSION_ANY );

		try
		{
			LogFile( CStrF("-------- %s (%d) --------", spT->m_Name, iTxt ) );

			/*	Check for detail texture.	*/	
			if( strcmp( spT->m_Name, "detail5" ) == 0 ||
					strcmp( spT->m_Name, "detail10" ) == 0 ||
					strcmp( spT->m_Name, "detail10" ) == 0 ||
					strcmp( spT->m_Name, "detaildebug" ) == 0 )	bDetailTexture = true;

			for( int iMip=0; iMip<spT->m_nMipmaps; iMip++ )
			{
				CImage* pImage;
				pImage = GetTexture( iTxt, iMip, CTC_TEXTUREVERSION_ANY );

				bool	bClearLod = false;

				if( bDetailTexture && iMip > 1 )
					bClearLod = true;

				/*	Swizzle the image.	*/	
				//pImage->SwizzleGC( (pImage->GetMemModel() & IMAGE_MEM_COMPRESS_ALLFLAGS) ? true : false );

				int width		= pImage->GetWidth();
				int height	= pImage->GetHeight();

				if( pImage->GetMemModel() & IMAGE_MEM_COMPRESS_ALLFLAGS )
					pImage->SwizzleGC( true, bClearLod );		//	Compressed image to deal with.
				else
					pImage->SwizzleGC( false, bClearLod );		//	Raw image data.
			}

///			LogFile( "--------------------" );
		}
		catch( CCException )
		{
			throw;
		}
		catch( ... )
		{
			Error("swizzleTexturesForGC", CStrF("Fatal exception processing %s", spT->m_Name ));
		}
	}
}

/*
	swizzleTexturesForPS2().

*/
void CTextureContainer_Plain::swizzleTexturesForPS2( void )
{
	MAUTOSTRIP(CTextureContainer_Plain_swizzleTexturesForGC, MAUTOSTRIP_VOID);

	int nTextures = GetNumTextures();
	int *pGSMem = DNew(int) int[1024 * 1024];

	for( int iTxt=0; iTxt<nTextures; iTxt++ )
	{
		bool	bDetailTexture = false;
		spCTexture	spT = GetTextureMap( iTxt, CTC_TEXTUREVERSION_ANY );

		try
		{
			for( int iMip=0; iMip < spT->m_nMipmaps; iMip++ )
			{
				CImage	*pImage = GetTexture( iTxt, iMip, CTC_TEXTUREVERSION_ANY );

				/*	Swizzle the image.	*/	
				pImage->SwizzlePS2( pGSMem );
			}
		}
		catch( CCException )
		{
			LogFile( CStrF("-------- Exception when swizzling %s (%d) --------", spT->m_Name, iTxt ) );
			delete[] pGSMem;
			throw;
		}
		catch( ... )
		{
			delete[] pGSMem;
			Error("swizzleTexturesForPS2", CStrF("Fatal exception processing %s", spT->m_Name ));
		}
	}
	delete[] pGSMem;
}
#endif

/*
	swizzleTexturesForXBOX().

*/
#ifndef PLATFORM_CONSOLE
void CTextureContainer_Plain::swizzleTexturesForXBOX( void )
{
	MAUTOSTRIP(CTextureContainer_Plain_swizzleTexturesForXBOX, MAUTOSTRIP_VOID);
	Error( "swizzleTexturesForXBOX", "Implement this" );
}
#endif


/*
	CParaboloidFromCubeMap.
	Takes one/six input images and builds one/two paraboloidmaps, then stores them in the two first textures.
	Note, if there is only one side texture, the two paraboloidmaps are treated to be identical. (This needs to be fixed probably, but hopefully no one will notice...)
*/
#ifndef	PLATFORM_CONSOLE
class	CParaboloidFromCubeMap	{

#define PARA_SPLIT_Z

	//	Determines the largest of a, b, c and returns 0, 1, 2 respectively.
	int	iMax( float a, float b, float c )
	{
		if( a>b )
			return( (a > c) ? 0 : 2 );
		else
			return( (b > c) ? 1 : 2 );
	}

	CTextureContainer_Plain	*m_pTC;

	CTexture	*m_pSource[6];
	spCImage	m_pSides[6];
	int				m_nSides;
	int				m_Width;
	int				m_Height;
	int				m_iBaseTxt;
	uint32		*m_pTempData;

	//
	void	prepareData( int _side, int _mip )
	{
		m_pSides[_side] = MNew(CImage);

		CImage	*pImg = m_pTC->GetTexture( m_iBaseTxt + _side, _mip, CTC_TEXTUREVERSION_RAW );
		pImg->Duplicate( m_pSides[_side] );

		//	Store dimensions for later use.
		if( _side == 0 )
		{
			m_Width  = m_pSides[_side]->GetWidth();
			m_Height = m_pSides[_side]->GetHeight();
		}
	}

	//
	CPnt	getPixelPnt( fp32 _x, fp32 _y )
	{
		int x = (int)(m_Width * _x);
		int y = (int)(m_Height * _y);

		if(x<0)	x = 0;
		if(y<0)	y = 0;

		if(x >= m_Width )		x = m_Width - 1;
		if(y >= m_Height )	y = m_Height - 1;

		return( CPnt( x, y ) );
	}

	//	Do the actual mapping.
	void	convert( bool _bOrientation, int _iMip )
	{
		//	Allocate tempdata.
		m_pTempData = DNew(uint32) uint32[ m_Width * m_Height ];

		if( _bOrientation )
			LogFile( CStrF( "[paraboloiding front (%dx%d)]", m_Width, m_Height ) );
		else
			LogFile( CStrF( "[paraboloiding back (%dx%d)]", m_Width, m_Height ) );

		CPnt TopLeft( 0, 0 );
		CPnt BottomRight( m_Width, m_Height );
		CClipRect sClipRect( CRct( TopLeft, BottomRight ), CPnt( 0, 0 ) );

		float flip = (_bOrientation==0 ? 1.0f : -1.0f);

		int u, v;
		for( v=0; v<m_Height; v++ )
		{
			for( u=0; u<m_Width; u++ )
			{
				float fu = (u * 2.0f / m_Width - 1.0f);
				float fv = (v * 2.0f / m_Height - 1.0f);
				float nx, ny, nz, anx, any, anz, r2;
				float	cu, cv;
				int		side;

				nx =  fu * flip;
				ny = -fv;
				r2 = nx*nx + ny*ny;

				//	Need normalization?
				if( r2 > 1.0f )
				{
					fp32 l = M_InvSqrt(r2);
					nx *= l;
					ny *= l;
					r2 = 1;
				}

				anx = M_Fabs(nx);
				any = M_Fabs(ny);
				anz = M_Sqrt(1.0f - r2);
				nz  = flip * anz;

				switch( iMax( anx, any, anz ) )
				{
					case 0:	//	X is major.
							cu = -nz / nx;
							cv = -ny / anx;
							side = (nx >= 0.0f) ? 0 : 1;
							break;
					case 1: //	Y is major.
							cu = nx / any;
							cv = nz / ny;
							side = (ny >= 0.0f) ? 2 : 3;
							break;
					case 2: //	Z is major.
							cu = nx / nz;
							cv = -ny / anz;
							side = (nz >= 0.0f) ? 4 : 5;
							break;
				}

				m_pTempData[ u + (v * m_Width) ] = m_pSides[side]->GetPixel( sClipRect, getPixelPnt( 0.5f + 0.5f*cu, 0.5f + 0.5f*cv ) );
			}
		}

		//	Save the data in the first/second texture, depending on orientation.
		CImage	*pImage = m_pTC->GetTexture( m_iBaseTxt + _bOrientation, _iMip, CTC_TEXTUREVERSION_RAW );

		for( v=0; v<pImage->GetWidth(); v++ )
		{
			for( u=0; u<pImage->GetHeight(); u++ )
				pImage->SetPixel( sClipRect, CPnt(u,v), (CPixel32)m_pTempData[ u + (v * pImage->GetWidth()) ] );
		}

		//	Delete tempdata.
		delete	[]	m_pTempData;
	}

	public:
			//	Constructor.
			CParaboloidFromCubeMap( CTextureContainer_Plain *_pTC )
			{
				m_nSides = 0;
				m_pTC = _pTC;
			}

			//	Destructor.
			~CParaboloidFromCubeMap()
			{
				for( int i=0; i<6; i++ )
					this->m_pSides[i] = NULL;
			}

			//	Push a sidetexture.
			void	pushSource( spCTexture _Image )
			{
				this->m_pSource[ m_nSides++ ] = _Image;
			}

			//	Do the actual legwork.
			void	save( bool _bTrueCubeMap, int _base )
			{
				m_iBaseTxt = _base;

				//	Do all mipmaps.
				for( int iMip=0; iMip<m_pSource[0]->m_nMipmaps; iMip++ )
				{
					prepareData( 0, iMip );

					//	Need all six?
					if( _bTrueCubeMap )
					{
						prepareData( 1, iMip );
						prepareData( 2, iMip );
						prepareData( 3, iMip );
						prepareData( 4, iMip );
						prepareData( 5, iMip );
					}

					convert( false, iMip );

					if( _bTrueCubeMap )
						convert( true, iMip );
				}

				m_nSides = 0;
			}
};
#endif



/*
	createDualParaboloidMaps().
	Convert all cubemaps in a texturecontainer into dual-paraboloid maps.
*/
#ifndef	PLATFORM_CONSOLE
void	CTextureContainer_Plain::createDualParaboloidMaps( void )
{
	MAUTOSTRIP( CTextureContainer_Plain_createDualParaboloidMaps, MAUTOSTRIP_VOID );

	//	The dude.
	CParaboloidFromCubeMap	parabolid( this );

	//	The babes.
	int	iTxt, i;

	for( iTxt=0; iTxt<GetNumTextures(); iTxt++ )
	{
		spCTexture	spT = GetTextureMap( iTxt, CTC_TEXTUREVERSION_ANY );

		try
		{
			if( spT->m_Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAP )
			{
				//	Push start-texture.
				for( i=0; i<6; i++ )
					parabolid.pushSource( spT );

				//	Save directly, since this cubemap has all it´s sides made up of this one texture.
				parabolid.save( false, iTxt );
			}
			else if( spT->m_Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN )
			{
				//	Push start-texture.
				for( i=0; i<6; i++ )
					parabolid.pushSource( GetTextureMap( iTxt + i, CTC_TEXTUREVERSION_ANY ) );

				//	Save, and increment past the other five textures that makes up the cubemap.
				parabolid.save( true, iTxt );
				iTxt += 5;
			}
		}
		catch( CCException )
		{
			throw;
		}
		catch(...)
		{
			#ifdef USE_HASHED_TEXTURENAME
			Error( "createDualParaboloidMaps", "Hardware exception processing texture" );
			#else
			Error( "SmoothCubemaps", CStrF( "Hardware exception processing %s", spT->m_Name ) );
			#endif
		}
	}
}
#endif

#ifndef	PLATFORM_CONSOLE
void CTextureContainer_Plain::ConvertTextures( uint32 _SupportedFormats, uint32 _compression, float _quality)
{
	MAUTOSTRIP(CTextureContainer_Plain_ConvertTextures, MAUTOSTRIP_VOID);

	LogFile("XTX:Convering textures to supported formats...");
	int iTxt;
	for(iTxt = 0; iTxt < GetNumTextures(); iTxt++ )
	{
		CTexture *pTexture = GetTextureMap(iTxt, CTC_TEXTUREVERSION_ANY);
		for( int nMipmap = 1; nMipmap < pTexture->m_nMipmaps; nMipmap++ )
		{
			if (!pTexture->m_lspMaps[nMipmap])
			{				
#ifndef USE_HASHED_TEXTURENAME
				LogFile(CStrF("XTX: Adding mipmaps to texture %s", pTexture->m_Name));
#endif				
				spCImage spFirstMip = pTexture->m_LargestMap.Duplicate();
				pTexture->Create(spFirstMip, pTexture->m_nMipmaps, spFirstMip->GetFormat());
				break;
			}
		}
	}

	for(iTxt = 0; iTxt < GetNumTextures(); iTxt++ )
	{
		CImage TexDesc;
		int nMipmaps;

		GetTextureDesc( iTxt, &TexDesc, nMipmaps );

		if( TexDesc.GetMemModel() & _compression )
		{
			continue;
		}

		if( TexDesc.GetFormat() & _SupportedFormats )
		{
			continue;
		}

		if( TexDesc.GetFormat() & IMAGE_FORMAT_BGR8 )
		{
			CTexture *pTexture = GetTextureMap(iTxt, CTC_TEXTUREVERSION_ANY);

#ifndef USE_HASHED_TEXTURENAME
			LogFile(CStrF("XTX:Converting %s from 24 bit texture to 32 bit ", pTexture->m_Name));
#endif			

			spCImage spImg;

			if (pTexture->m_LargestMap.IsCompressed())
			{
				spImg = MNew(CImage);
				pTexture->m_LargestMap.Decompress(spImg);				
			}
			else
			{
				spImg = pTexture->m_LargestMap.Duplicate();
			}

			pTexture->m_LargestMap.Create(spImg->GetWidth(), spImg->GetHeight(), IMAGE_FORMAT_BGRX8, spImg->GetMemModel());

			CImage::Convert(spImg, &pTexture->m_LargestMap);			

			for( int nMipmap = 1; nMipmap < pTexture->m_nMipmaps; nMipmap++ )
			{
				if (pTexture->m_lspMaps[nMipmap]->IsCompressed())
				{
					spImg = MNew(CImage);
					pTexture->m_lspMaps[nMipmap]->Decompress(spImg);				
					pTexture->m_lspMaps[nMipmap] = spImg->Convert(IMAGE_FORMAT_BGRX8);
				}
				else
				{
					pTexture->m_lspMaps[nMipmap] = pTexture->m_lspMaps[nMipmap]->Convert(IMAGE_FORMAT_BGRX8);
				}
			}
		}
	}
}
#endif

int CTextureContainer_Plain::GetCorrectLocal(int _iLocal, int _TextureVersion)
{
	if(_TextureVersion == CTC_TEXTUREVERSION_ANY)
		return _iLocal;

	int iLocal = _iLocal;
	CTexture* pT = m_lspTextures[iLocal];
	int iGlobalID = pT->m_TextureID;
	int nNumLocal = m_lspTextures.Len();
	while(pT->m_Properties.m_TextureVersion != _TextureVersion)
	{
		bool bError = true;
		iLocal++;
		if(iLocal < nNumLocal)
		{
			pT = m_lspTextures[iLocal];
			if(pT->m_TextureID == iGlobalID)
				bError = false;
		}

		if(bError)
		{
			LogFile(CStrF("WARNING: Unable to find requested version for texture %s (Req. %d, Returned %d)", m_lspTextures[_iLocal]->m_LargestMap.IDString().Str(), _TextureVersion, m_lspTextures[_iLocal]->m_Properties.m_TextureVersion));
			return _iLocal;
		}
	}

	return iLocal;
}

void CTextureContainer_Plain::AddCompileChecksum(uint32 _Checksum)
{
	m_lTextureCompileChecksums.Add(_Checksum);
}

uint32 CTextureContainer_Plain::GetCompileChecksum(int _iLocal)
{
	M_ASSERT(_iLocal >= 0 && _iLocal < m_lTextureCompileChecksums.Len(), "Index out of range!");

	return m_lTextureCompileChecksums[_iLocal];
}

int CTextureContainer_Plain::EnumTextureVersions(int _iLocal, uint8* _pDestVersion, int _nMaxVersions)
{
	int nVersions = 0;
	int TexID = GetTextureID(_iLocal);
	int iLocal = _iLocal;
	int nLocal = GetNumLocal();
	while((iLocal < nLocal) && (nVersions < _nMaxVersions) && (GetTextureID(iLocal) == TexID))
	{
		if(_pDestVersion)
		{
			CTC_TextureProperties Properties;
			GetTextureProperties(iLocal, Properties);
			_pDestVersion[nVersions]	= Properties.m_TextureVersion;
		}
		nVersions++;
		iLocal++;
	}

	return nVersions;

/*	int nVersions = 0;
	int iLocal = _iLocal;

	CStr LocalName = GetName(_iLocal);

	// Since _iLocal might not be the first instance of this texture we have to go backwards a bit first

	while((iLocal > 0) && (GetName(iLocal - 1) == LocalName))
		iLocal--;

	// Now we move forward and count all instances of this texture

	int nLocal = GetNumTextures();
	while((iLocal < nLocal) && (GetName(iLocal) == LocalName) && (nVersions < _nMax))
	{
		if (nVersions >= _nMaxVersions)
			Error("EnumTextureVersions", CStrF("Too many versions. (%d/%d)", nVersions, _nMaxVersions));

		_pDestVersion[nVersions++]	= m_lspTextures[iLocal]->m_Properties.m_TextureVersion;
		iLocal++;
	}

	return nVersions;*/
}

/*spCImage CTextureContainer_Plain::Build(int _iLocal)
{
	return m_lspTextures[_iLocal]->m_lspMaps[0];
}
*/
// -------------------------------------------------------------------
//  CTextureContainer_VirtualXTC
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_VirtualXTC, CTextureContainer_Plain);

IMPLEMENT_OPERATOR_NEW(CTextureContainer_VirtualXTC);


void CTextureContainer_VirtualXTC::ScanImageList(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_ScanImageList, MAUTOSTRIP_VOID);

	CCFile* pFile = _pDFile->GetFile();
	int iPalBase = m_lspPalettes.Len();
	int iFirstTxt = m_lspTextures.Len();

	// Read palettes
	_pDFile->PushPosition();
	while(_pDFile->GetNext("PALETTES"))
	{
		if (_pDFile->GetSubDir())
		{
			while(_pDFile->GetNext("PALETTE"))
			{
				spCImagePalette spPal;
				if (spPal == NULL) spPal = MNew(CImagePalette);
				if (spPal == NULL) MemError("Read");
				CPixel32 Pal[256];
				if (_pDFile->GetUserData() != 256) Error("Read", "Supports only 256 color palettes.");
				pFile->Read(&Pal, sizeof(Pal));

				spPal->SetPalette((CPixel32*) &Pal, 0, 256);
				m_lspPalettes.Add(spPal);
			}
			_pDFile->GetParent();
		}
	}
	_pDFile->PopPosition();

	// Check for Image-directory.
	bool bScan = true;
	_pDFile->PushPosition();
	if (_pDFile->GetNext("IMAGEDIRECTORY4"))
	{
		int iLastNewImage = -1;
		int nTxt = _pDFile->GetUserData();
		m_lspTextures.SetLen(iFirstTxt + nTxt);
		for(int iTxt = 0; iTxt < m_lspTextures.Len(); iTxt++)
		{
			m_lspTextures[iFirstTxt + iTxt] = MNew(CTexture);
			m_lspTextures[iFirstTxt + iTxt]->ReadIndexData(_pDFile->GetFile(), &m_lspPalettes, iPalBase);
#ifdef USE_HASHED_TEXTURENAME
			if( iLastNewImage == -1 || (m_lspTextures[iFirstTxt + iTxt]->m_NameID != m_lspTextures[iLastNewImage]->m_NameID) )
			{
				iLastNewImage = iFirstTxt + iTxt;
				m_lspTextures[iFirstTxt + iTxt]->m_TextureID = m_pTC->AllocID(m_iTextureClass, iTxt, m_lspTextures[iFirstTxt + iTxt]->m_NameID);
			}
			else
				m_lspTextures[iFirstTxt + iTxt]->m_TextureID	= m_lspTextures[iLastNewImage]->m_TextureID;
#else			
			if( iLastNewImage == -1 || (CStrBase::stricmp(m_lspTextures[iFirstTxt + iTxt]->m_Name, m_lspTextures[iLastNewImage]->m_Name) != 0) )
			{
				iLastNewImage = iFirstTxt + iTxt;
				m_lspTextures[iFirstTxt + iTxt]->m_TextureID = m_pTC->AllocID(m_iTextureClass, iTxt, &m_lspTextures[iFirstTxt + iTxt]->m_Name[0]);
			}
			else
				m_lspTextures[iFirstTxt + iTxt]->m_TextureID	= m_lspTextures[iLastNewImage]->m_TextureID;
#endif
//			InsertHash(iTxt); // FIXME: Should we do a AddTexture ?
		}

		bScan = false;
	}
	_pDFile->PopPosition();


	if (bScan)
	{
		while(_pDFile->GetNext("IMAGE"))
		{
			if (_pDFile->GetSubDir())
			{
				spCTexture spTxt = MNew(CTexture);
				if (spTxt == NULL) MemError("Read");
				spTxt->Virtual_Read(_pDFile, &m_lspPalettes, iPalBase);
				AddTexture(spTxt);
				_pDFile->GetParent();
			}
		}
	}

/*	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
	{
		CRegistry* pEnv = pSys->GetEnvironment();
		
		if (pEnv && pEnv->GetValuei("RESOURCES_LOG", 0) != 0)
		{
			MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
			if (pTC)
			{
				CCFile TulFile;
				TulFile.Open(pSys->m_ExePath + "\\Textures.tul", CFILE_READ);
				int32 Len;
				TulFile.ReadLE(Len);
				for (int i = 0; i < Len; ++i)
				{
					CStr Temp;
					
					Temp.Read(&TulFile);

					int TexID = pTC->GetTextureID(Temp);
					if (TexID >= 0)
						pTC->SetTextureParam(TexID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_USED);
				}
			}
		}
	}*/

}

void CTextureContainer_VirtualXTC::Unload(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_Unload_int, MAUTOSTRIP_VOID);

	if (_iLocal < 0) return;

	// Find correct _iLocal
	m_lspTextures[_iLocal]->Virtual_Unload();

	/*
	int i, nc = m_liCached.Len();
	for(i = 0; i < nc; i++)
		if (m_liCached[i] == CTextureCache_Entry(_iLocal, 0))
		{
			m_liCached[i] = CTextureCache_Entry();
			return;
		}
		*/
}

void CTextureContainer_VirtualXTC::Unload(int _iLocal, int _iMipMap)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_Unload_int_int, MAUTOSTRIP_VOID);

	if (_iLocal < 0) return;
	if (_iMipMap < 0) return;
	m_lspTextures[_iLocal]->Virtual_Unload(_iMipMap);

	/*
	int i, nc = m_liCached.Len();
	for(i = 0; i < nc; i++)
		if (m_liCached[i] == CTextureCache_Entry(_iLocal, _iMipMap))
		{
			m_liCached[i] = CTextureCache_Entry();
			return;
		}
		*/

}

/*
void CTextureContainer_VirtualXTC::Load(int _iLocal)
{
//	if (!m_lspTextures[_iLocal]->m_bIsLoaded)
//	{
//		CCFile File;
//		File.Open(m_FileName, CFILE_BINARY | CFILE_READ);
//		m_lspTextures[_iLocal]->Virtual_Load(&File);
//		File.Close();
//	}

	int i, nc = m_liCached.Len();
	for(i = 0; i < nc; i++)
		if (m_liCached[i] == _iLocal) break;

	if (i == nc)
	{
		// Not loaded
		Unload(m_liCached[nc-1]);
		for(int j = nc-1; j > 0; j--)
			m_liCached[j] = m_liCached[j-1];
		m_liCached[0] = _iLocal;

		CCFile File;
		File.Open(m_FileName, CFILE_BINARY | CFILE_READ);
		m_lspTextures[_iLocal]->Virtual_Load(&File);
		File.Close();

		if (!m_lspTextures[_iLocal]->m_bIsLoaded) 
			Error("Load", "Internal error. (1)");
	}
	else
	{
		if (i > 0)
		{
			// Loaded, but not in front
			for(int j = i; j > 0; j--)
				m_liCached[j] = m_liCached[j-1];
			m_liCached[0] = _iLocal;

			if (!m_lspTextures[_iLocal]->m_bIsLoaded) 
				Error("Load", "Internal error. (2)");
		}
	}
}
*/

void CTextureContainer_VirtualXTC::Load(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_Load_int, MAUTOSTRIP_VOID);

	Load(_iLocal, 0);
}

void CTextureContainer_VirtualXTC::Load(int _iLocal, int _iMipmap, int _nMips)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_Load_int_int, MAUTOSTRIP_VOID);

	if ((m_lspTextures[_iLocal]->m_IsLoaded>>_iMipmap) & 1) return;

	CCFile *pFile;
	CCFile File;
	if (m_spPrecacheFile)
		pFile = m_spPrecacheFile;
	else
	{
		File.Open(GetFileName(), CFILE_BINARY | CFILE_READ);
		pFile = &File;
	}

	for (int i = 0; i < _nMips; ++i)
	{
//		File.Open(GetFileName(), CFILE_BINARY | CFILE_READ);
		if (! ((m_lspTextures[_iLocal]->m_IsLoaded>>(_iMipmap + i)) & 1))
		{
			m_lspTextures[_iLocal]->Virtual_Load(pFile, _iMipmap + i);
			m_lspTextures[_iLocal]->m_IsLoaded |= (1 << (_iMipmap + i));
		}
	}

#if 0
/*	if (!m_lspTextures[_iLocal]->m_bIsLoaded)
	{
		CCFile File;
		File.Open(m_FileName, CFILE_BINARY | CFILE_READ);
		m_lspTextures[_iLocal]->Virtual_Load(&File);
		File.Close();
	}*/
	CTexture* pT = m_lspTextures[_iLocal];
	if ((_iMipmap < 0) || (_iMipmap >= pT->m_nMipmaps)) Error("GetTexture", CStrF("Invalid MIP-map number. (%d/%d, %s)", _iMipmap, pT->m_nMipmaps, (char*) pT->m_LargestMap.IDString() ));

	int i, nc = m_liCached.Len();
	for(i = 0; i < nc; i++)
		if (m_liCached[i] == CTextureCache_Entry(_iLocal, _iMipmap)) break;

	if (i == nc)
	{
		// Not loaded
		Unload(m_liCached[nc-1].GetTexture(), m_liCached[nc-1].GetMipMap());
		for(int j = nc-1; j > 0; j--)
			m_liCached[j] = m_liCached[j-1];
		m_liCached[0] = CTextureCache_Entry(_iLocal, _iMipmap);

		CCFile File;
//		File.Open(GetFileName(), CFILE_BINARY | CFILE_READ);
		File.OpenExt(GetFileName(), CFILE_BINARY | CFILE_READ, NO_COMPRESSION, NORMAL_COMPRESSION, 0, 1, 65536);
		m_lspTextures[_iLocal]->Virtual_Load(&File, _iMipmap);
		File.Close();

/*
		if (!m_lspTextures[_iLocal]->m_bIsLoaded) 
			Error("Load", "Internal error. (1)");*/
	}
	else
	{
		if (i > 0)
		{
			// Loaded, but not in front
			for(int j = i; j > 0; j--)
				m_liCached[j] = m_liCached[j-1];
			m_liCached[0] = CTextureCache_Entry(_iLocal, _iMipmap);

/*
			if (!m_lspTextures[_iLocal]->m_bIsLoaded) 
				Error("Load", "Internal error. (2)");
*/
		}
	}

	m_lspTextures[_iLocal]->m_IsLoaded |= (1 << _iMipmap);
#endif
}

CStr CTextureContainer_VirtualXTC::GetFileName()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetFileName, NULL);

//	if (m_CacheFileName == "")
		return m_FileName;
/*	else
	{
		if (!m_bIsCached)
		{
			CStr CachePath = m_CacheFileName.GetPath();

			ConOutL(CStrF("(CTextureContainer_VirtualXTC) Caching %s to %s", (char*) m_FileName, (char*)m_CacheFileName));

			if (CachePath != "")
				CDiskUtil::CreatePath(CachePath);
			CDiskUtil::CpyFile(m_FileName, m_CacheFileName, 1024 * 512);
			m_bIsCached = true;
		}
		return m_CacheFileName;
	}*/
}


// -------------------------------------------------------------------
void CTextureContainer_VirtualXTC::Create(CDataFile* _pDFile, CStr _FileName, int _NumCache)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_Create_CDataFile_int, MAUTOSTRIP_VOID);

//	m_liCached.SetLen(_NumCache);
//	for(int i = 0; i < _NumCache; i++) m_liCached[i] = CTextureCache_Entry(-1, -1);

	m_FileName = _FileName.Len() ? _FileName : _pDFile->GetFile()->GetFileName();
	m_ContainerName = m_FileName.GetFilename();

	m_lspTextures.SetGrow(128);
	{
		_pDFile->PushPosition();
		if(_pDFile->GetNext("SOURCE"))
			m_Source.Read(_pDFile->GetFile());
		_pDFile->PopPosition();
	}

	if (!_pDFile->GetNext("IMAGELIST")) Error("-", "Invalid XTC.");
	if (!_pDFile->GetSubDir()) Error("-", "Invalid XTC.");
	ScanImageList(_pDFile);
	m_lspTextures.OptimizeMemory();
}

void CTextureContainer_VirtualXTC::Create(CStr _FileName, int _NumCache)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_Create_CStr_int, MAUTOSTRIP_VOID);

	spCCFile spFile = MNew(CCFile);
	spFile->OpenExt(_FileName, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 0, 1, 16384);

	CDataFile DFile;
	DFile.Open(spFile, 0);
	Create(&DFile, _FileName, _NumCache);
	DFile.Close();
}

#if 0
void CTextureContainer_VirtualXTC::SetCacheFile(CStr _CacheFileName)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_SetCacheFile, MAUTOSTRIP_VOID);

	m_CacheFileName = _CacheFileName;

	if (CDiskUtil::FileExists(_CacheFileName))
		m_bIsCached = true;
}
#endif

void CTextureContainer_VirtualXTC::AddFromXTC(const char* _pName)
{
	Create(_pName);
}

CTextureContainer_VirtualXTC::CTextureContainer_VirtualXTC()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_ctor, MAUTOSTRIP_VOID);

//	m_bIsCached = false;
}

CTextureContainer_VirtualXTC::~CTextureContainer_VirtualXTC()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_dtor, MAUTOSTRIP_VOID);
}

int CTextureContainer_VirtualXTC::GetNumTextures()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetNumTextures, 0);
	return CTextureContainer_Plain::GetNumTextures();
}

spCTexture CTextureContainer_VirtualXTC::GetTextureMap(int _iLocal, int _TextureVersion, bool _bForceLoaded)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetNumTexture_int_bool, NULL);
	if (_bForceLoaded)
	{
		int iLocal = GetCorrectLocal(_iLocal, _TextureVersion);
		Load(iLocal, 0);
	}
	return CTextureContainer_Plain::GetTextureMap(_iLocal, _TextureVersion);
}


// ----------------------------------------------------------------
void CTextureContainer_VirtualXTC::Clear()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_Clear, MAUTOSTRIP_VOID);
	CTextureContainer_Plain::Clear();

//	m_liCached.Clear();
}

int CTextureContainer_VirtualXTC::GetLocal(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetLocal, 0);
	return CTextureContainer_Plain::GetLocal(_pName);
}

int CTextureContainer_VirtualXTC::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetTextureID, 0);
	return CTextureContainer_Plain::GetTextureID(_iLocal);
}

int CTextureContainer_VirtualXTC::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetTextureDesc, 0);
//	Load(_iLocal);
	return CTextureContainer_Plain::GetTextureDesc(_iLocal, _pTargetImg, _Ret_nMipmaps);
}

void CTextureContainer_VirtualXTC::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetTextureProperties, MAUTOSTRIP_VOID);
	_Prop = m_lspTextures[_iLocal]->m_Properties;
}

CImage* CTextureContainer_VirtualXTC::GetTexture(int _iLocal, int _iMipMap, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetTexture_int_int, NULL);
	int iLocal = GetCorrectLocal(_iLocal, _TextureVersion);
	Load(iLocal, _iMipMap);
	return CTextureContainer_Plain::GetTexture(_iLocal, _iMipMap, _TextureVersion);
}

CImage* CTextureContainer_VirtualXTC::GetTextureNumMip(int _iLocal, int _iMipMap, int _nMips, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_GetTexture_int_int, NULL);
	int iLocal = GetCorrectLocal(_iLocal, _TextureVersion);
	Load(iLocal, _iMipMap, _nMips);
	return CTextureContainer_Plain::GetTexture(_iLocal, _iMipMap, _TextureVersion);
}

void CTextureContainer_VirtualXTC::ReleaseTexture(int _iLocal, int _iMipMap, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_ReleaseTexture, MAUTOSTRIP_VOID);
	int iLocal = GetCorrectLocal(_iLocal, _TextureVersion);
	Unload(iLocal, _iMipMap);
}

void CTextureContainer_VirtualXTC::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BulidFlags)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC_BuildInto, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_VirtualXTC::BuildInto, CTextureContainer_VirtualXTC);

	if (_nMipmaps > 1)
		Error("BuildInto", "Supports only a single mip-map per build.");

	int iLocal = GetCorrectLocal(_iLocal, _TextureVersion);
	Load(iLocal, _iStartMip);
	CTextureContainer_Plain::BuildInto(_iLocal, _ppImg, _nMipmaps, _TextureVersion, _ConvertType, _iStartMip);

//#ifdef _XBOX

	Unload(iLocal, _iStartMip);

/*	int i, nc = m_liCached.Len();
	for(i = 0; i < nc; i++)
		Unload(m_liCached[i].GetTexture(), m_liCached[i].GetMipMap());

	m_liCached.Clear();*/
		// Not loaded


//#endif
}

void CTextureContainer_VirtualXTC::OpenPrecache()
{
	m_spPrecacheFile = MNew(CCFile);
	m_spPrecacheFile->Open(GetFileName(), CFILE_BINARY | CFILE_READ);
}

void CTextureContainer_VirtualXTC::ClosePrecache()
{
	m_spPrecacheFile = NULL;
}

MRTC_IMPLEMENT(CTextureContainer_Video, CTextureContainer);
