#include "PCH.h"

#include "MImage.h"
#include "MImageIO.h"
#include "MTexture.h"

//#define DEBUG_DISABLE_INTELP5

// Disables CPU_INTELP5
#ifdef DEBUG_DISABLE_INTELP5
	#ifdef CPU_INTELP5
		#undef CPU_INTELP5
		#define KEEP_CPU_INTELP5
	#endif
#endif


// -------------------------------------------------------------------
//  CImage
// -------------------------------------------------------------------

// -------------------------------------------------------------------
//  (Un)lock functions
// -------------------------------------------------------------------
void* CImage::__Lock(int _ExtLockMode, CRct *_pRect) 
{
	MAUTOSTRIP(CImage___Lock, NULL);

	if (_ExtLockMode & IMAGE_LOCK_COMPRESSED)
	{
		if (!(m_Memmodel & IMAGE_MEM_COMPRESSED)) return NULL;
		return m_pBitmap;
	}
	else
	{
		if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return NULL;
		if (m_Memmodel & IMAGE_MEM_COMPRESSED) return NULL;

		if (m_Memmodel & IMAGE_MEM_ALIGN32)
			return (void*)((mint(m_pBitmap) + 31) & 0xffffffe0);
		else if (m_Memmodel & IMAGE_MEM_ALIGN16)
			return (void*)((mint(m_pBitmap) + 15) & 0xfffffff0);
		else
			return m_pBitmap;
	}
};

void CImage::__Unlock() 
{
	MAUTOSTRIP(CImage___Unlock, MAUTOSTRIP_VOID);
};

// -------------------------------------------------------------------
//  Construction & Destruction
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CImage);


CImage::CImage()
{
	MAUTOSTRIP(CImage_ctor, MAUTOSTRIP_VOID);

	m_Width = 0;
	m_Height = 0;
	m_Modulo = 0;
	m_Format = IMAGE_FORMAT_VOID;
	m_Flags = 0;
	m_Memmodel = IMAGE_MEM_VOID;
	m_pBitmap = NULL;
	m_AllocSize = 0;
	m_Pixelsize = 0;
//	UpdateFormat();
};

CImage::CImage(int _w, int _h, int _format, int _memmodel, int _Flags)
{
	MAUTOSTRIP(CImage_ctor_w_h_format_memmodel, MAUTOSTRIP_VOID);

	m_Width = 0;
	m_Height = 0;
	m_Modulo = 0;
	m_Format = IMAGE_FORMAT_VOID;
	m_Flags = 0;
	m_Memmodel = IMAGE_MEM_VOID;
	UpdateFormat();
	m_pBitmap = NULL;
	m_AllocSize = 0;

	Create(_w, _h, _format, _memmodel, 0, _Flags);
};

CImage::~CImage()
{
	MAUTOSTRIP(CImage_dtor, MAUTOSTRIP_VOID);

	Destroy();
};

void CImage::Destroy()
{
	MAUTOSTRIP(CImage_Destroy, MAUTOSTRIP_VOID);

	if (m_Memmodel & IMAGE_MEM_REFERENCE) m_pBitmap = NULL;
	if (m_pBitmap) { delete[] (uint8*)m_pBitmap; m_pBitmap = NULL; };
	m_AllocSize = 0;

	m_spPalette = NULL;
	m_Format = IMAGE_FORMAT_VOID;
	m_Flags = 0;
	m_Memmodel = IMAGE_MEM_VOID;
	m_Width = 0;
	m_Height = 0;
	m_Modulo = 0;
	m_Pixelsize = 0;
//	UpdateFormat();
};
/*
#ifdef COMPILER_NEEDOPERATORDELETE
	void operator delete(void*, void*);
#endif
*/

void CImage::operator= (const CImage& _Image)
{
	MAUTOSTRIP(CImage_operator_eq, MAUTOSTRIP_VOID);

	(const_cast<CImage*>(&_Image))->Duplicate(this);
}

// -------------------------------------------------------------------
void CImage::UpdateFormat()
{
	MAUTOSTRIP(CImage_UpdateFormat, MAUTOSTRIP_VOID);

	m_Pixelsize = Format2PixelSize(m_Format);
};

// -------------------------------------------------------------------
//  Convert format to pixelsize.
// -------------------------------------------------------------------
int CImage::Format2PixelSize(int _format)
{
	MAUTOSTRIP(CImage_Format2PixelSize, 0);

	switch(_format) {
		case IMAGE_FORMAT_VOID : return 0;
		case IMAGE_FORMAT_BGR10A2 : return 4;
		case IMAGE_FORMAT_CLUT8 : return 1;
		case IMAGE_FORMAT_RGB10A2_F : return 4;
		case IMAGE_FORMAT_BGR5 : return 2;
		case IMAGE_FORMAT_B5G6R5 : return 2;
		case IMAGE_FORMAT_BGR8 : return 3;
		case IMAGE_FORMAT_BGRX8 : return 4;
		case IMAGE_FORMAT_RGB16 : return 6;
		case IMAGE_FORMAT_BGR5A1 : return 2;
		case IMAGE_FORMAT_BGRA4 : return 2;
//		case IMAGE_FORMAT_RGBA3328 : return 2;
		case IMAGE_FORMAT_BGRA8 : return 4;
		case IMAGE_FORMAT_BGRA16 : return 8;
		case IMAGE_FORMAT_I8 : return 1;
		case IMAGE_FORMAT_RGBA8 : return 4;
		case IMAGE_FORMAT_I16 : return 2;
		case IMAGE_FORMAT_RGB8 : return 3;
		case IMAGE_FORMAT_I8A8 : return 2;
		case IMAGE_FORMAT_A8 : return 1;
		case IMAGE_FORMAT_RGB32_F : return 12;
		case IMAGE_FORMAT_RGBA32_F : return 16;
		case IMAGE_FORMAT_RGBA16 : return 8;
		case IMAGE_FORMAT_RGBA16_F : return 8;
//		case IMAGE_FORMAT_ZBUFFER24 : return 3;
		case IMAGE_FORMAT_Z24S8 : return 4;
/*		case IMAGE_FORMAT_V8U8 : return 2;
		case IMAGE_FORMAT_GREY_F16 : return 2;
		case IMAGE_FORMAT_UYVY : return 4;
		case IMAGE_FORMAT_YUY2 : return 4;
		case IMAGE_FORMAT_V16U16 : return 4;
		case IMAGE_FORMAT_G16R16 : return 4;
		case IMAGE_FORMAT_A2W10V10U10 : return 4;
		case IMAGE_FORMAT_R11G11B10 : return 4;
		case IMAGE_FORMAT_W11V11U10 : return 4;
		case IMAGE_FORMAT_G16R16F : return 4;
		case IMAGE_FORMAT_R32F : return 4;
		case IMAGE_FORMAT_G32R32F : return 8;
		case IMAGE_FORMAT_A32B32G32R32F : return 16;*/
		default :
			Error_static("CImage::Format2PixelSize", CStrF("Invalid format: %d", _format));
			return IMAGE_FORMAT_VOID;
	};
};

void CImage::Create(int _w, int _h, int _format, int _memmodel, spCImagePalette _spPalette, int _Flags)
{
	MAUTOSTRIP(CImage_Create_w_h_format_memmodel_palette, MAUTOSTRIP_VOID);

	// Check if we can keep the current image.
	if (_w == m_Width &&
		_h == m_Height &&
		_format == m_Format &&
		_Flags == m_Flags &&
		_memmodel == m_Memmodel &&
		_spPalette == m_spPalette)
	{
		return;
	}

	_memmodel &= (~IMAGE_MEM_VIRTUAL);
	_memmodel |= IMAGE_MEM_LOCKABLE;

	if (_memmodel & IMAGE_MEM_COMPRESSED)
		Error("Create", "Cannot create compressed image.");

	if ((_memmodel & IMAGE_MEM_VIDEO) ||
	//	((_memmodel & IMAGE_MEM_TYPEMASK) == IMAGE_MEM_TEXTURE) ||
		((_memmodel & IMAGE_MEM_TYPEMASK) == IMAGE_MEM_PRIMARY) ||
		((_memmodel & IMAGE_MEM_TYPEMASK) == IMAGE_MEM_PRIMARYDOUBLE))
	{
		Error("Create", "Unsupported image model.");
	}
	else
	{
		Destroy();
		int psize = Format2PixelSize(_format);
		int modulo = 0;
		if (IsPow2(_w) || (_w < 4))
			modulo = _w*psize;
		else
			modulo = ((_w*psize)+3) & 0xfffffffc;

		int AlignExtra = 0;
		if (_memmodel & IMAGE_MEM_ALIGN16) AlignExtra = 15;
		if (_memmodel & IMAGE_MEM_ALIGN32) AlignExtra = 31;

		int AllocSize = modulo*_h + AlignExtra;

		void* pMap = DNew(uint8) uint8[AllocSize];
		if (pMap == NULL) MemError("Create");

		m_pBitmap = pMap;
		m_AllocSize = AllocSize;
		m_Modulo = modulo;
		m_Width = _w;
		m_Height = _h;
		m_Format = _format;
		m_Flags = _Flags;
		m_Memmodel = _memmodel;
		m_spPalette = _spPalette;
		UpdateFormat();

		// Palette?
		if (m_Format & IMAGE_FORMAT_PALETTE)
		{
			if (_spPalette != NULL)
				m_spPalette = _spPalette;
			else
			{
				m_spPalette = MNew(CImagePalette);
				if (m_spPalette == NULL) MemError("Create");
			};
		};
	};
};

void CImage::CreateVirtual(int _w, int _h, int _format, int _memmodel, spCImagePalette _spPalette, int _Flags)
{
	MAUTOSTRIP(CImage_Create_w_h_format_memmodel_palette, MAUTOSTRIP_VOID);

	Destroy();

	m_Width = _w;
	m_Height = _h;
	m_Format = _format;
	m_Flags = _Flags;
	m_Memmodel = _memmodel | IMAGE_MEM_VIRTUAL;
	m_spPalette = _spPalette;
	UpdateFormat();
};

void CImage::CreateVirtual_NoDelete(int _w, int _h, int _Format, int _MemModel, int _Flags)
{
	m_Width = _w;
	m_Height = _h;
	m_Format = _Format;
	m_Flags = _Flags;
	m_Memmodel = _MemModel | IMAGE_MEM_VIRTUAL;
	m_Pixelsize = Format2PixelSize(_Format);
	m_Modulo = 0;
	m_pBitmap = NULL;
	m_spPalette = NULL;
}

void CImage::CreateReference(int _w, int _h, int _format, int _memmodel, int _modulo, void* _pBitmap, spCImagePalette _spPalette, int _Flags)
{
	MAUTOSTRIP(CImage_CreateReference, MAUTOSTRIP_VOID);

	Destroy();
	m_Width = _w;
	m_Height = _h;
	m_Format = _format;
	m_Flags = _Flags;
	m_Memmodel = _memmodel | IMAGE_MEM_REFERENCE | IMAGE_MEM_LOCKABLE;
	m_Modulo = _modulo;
	m_pBitmap = _pBitmap;
	m_AllocSize = _h*_modulo;
	m_spPalette = _spPalette;
	UpdateFormat();
};

void CImage::Create(CImage& imgdesc)
{
	MAUTOSTRIP(CImage_Create_imgdesc, MAUTOSTRIP_VOID);

	Create(imgdesc.GetWidth(), imgdesc.GetHeight(), imgdesc.GetFormat(), imgdesc.GetMemModel(), 0, imgdesc.GetFlags());
};

spCImage CImage::Duplicate()
{
	MAUTOSTRIP(CImage_Duplicate, NULL);

	spCImage spImg = MNew(CImage);
	if (spImg == NULL) MemError("Duplicate");

	Duplicate(spImg);
	return spImg;
}

void CImage::Duplicate(CImage* _pTarget)
{
	MAUTOSTRIP(CImage_Duplicate_pTarget, MAUTOSTRIP_VOID);

	if (GetMemModel() & IMAGE_MEM_VIRTUAL)
	{
		_pTarget->CreateVirtual(GetWidth(), GetHeight(), GetFormat(), GetMemModel(), GetPalette(), GetFlags());
	}
	else if (GetMemModel() & IMAGE_MEM_COMPRESSED)
	{
		_pTarget->m_pBitmap = DNew(uint8) uint8[m_AllocSize];		
		if (!_pTarget->m_pBitmap) MemError("Duplicate");
		memcpy(_pTarget->m_pBitmap, m_pBitmap, m_AllocSize);

		_pTarget->m_AllocSize = m_AllocSize;
		_pTarget->m_Memmodel = m_Memmodel;
		_pTarget->m_Format = m_Format;
		_pTarget->m_Flags = m_Flags;
		_pTarget->m_Width = m_Width;
		_pTarget->m_Height = m_Height;
		if(m_spPalette)
		{
			_pTarget->m_spPalette = MNew(CImagePalette);
			_pTarget->m_spPalette->SetPalette(m_spPalette);
		}												   
		_pTarget->UpdateFormat();
	}
	else
	{
		_pTarget->Create(*this);
		_pTarget->m_Memmodel	&= ~IMAGE_MEM_REFERENCE;
		_pTarget->Blt(_pTarget->GetClipRect(), *this, 0, CPnt(0, 0));
		if (GetFormat() & IMAGE_FORMAT_PALETTE)
			_pTarget->GetPalette()->SetPalette(GetPalette());
	}
}

int CImage::GetCompressFormats(int _Compression)
{
	MAUTOSTRIP(CImage_GetCompressFormats, 0);

	if (_Compression & IMAGE_MEM_COMPRESSTYPE_JPG)
		return IMAGE_FORMAT_BGR8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_I8;
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_S3TC)
		return IMAGE_FORMAT_BGR8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRX8;
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_VQ)
		return IMAGE_FORMAT_BGR8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRX8;
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_SOUND)
		return IMAGE_FORMAT_I8 | IMAGE_FORMAT_I16 | IMAGE_FORMAT_BGR8 | IMAGE_FORMAT_BGRA8;
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_QUANT)
		return IMAGE_FORMAT_BGR8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRX8;
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_3DC)
		return IMAGE_FORMAT_I8A8 | IMAGE_FORMAT_BGR8;// | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRX8;	// Should really only be G8A8
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_CTX)
		return IMAGE_FORMAT_I8A8 | IMAGE_FORMAT_BGR8;// | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRX8;	// Should really only be G8A8
	else
		return 0;
}

bool CImage::CanCompress(int _Compression)
{
	MAUTOSTRIP(CImage_CanCompress, false);

	if (!(GetCompressFormats(_Compression) & GetFormat()))
		return false;

//	if ((_Compression & IMAGE_MEM_COMPRESSTYPE_S3TC) &&
//		(GetWidth() < 4) || (GetHeight() < 4)) return false;

	return true;
}

void CImage::CompressSound(const char * _pFormat, CImage* _pDestImg, float _Priority, float _Quality, uint32 _Flags)
{
	MAUTOSTRIP(CImage_CompressSound, MAUTOSTRIP_VOID);

	Compress_Sound(_pFormat, _pDestImg, _Priority, _Quality, _Flags);
}

void CImage::Compress(int _Compression, fp32 _Quality, CImage* _pDestImg, CTexture *_pTexture)
{
	MAUTOSTRIP(CImage_Compress, MAUTOSTRIP_VOID);

#ifndef	PLATFORM_CONSOLE
	if (_Compression & IMAGE_MEM_COMPRESSTYPE_JPG)
		Compress_JPG(_Quality, _pDestImg);
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_S3TC)
		Compress_S3TC(_Quality, _pDestImg);
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_3DC)
		Compress_3DC(_Quality, _pDestImg);
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_CTX)
		Compress_CTX(_Quality, _pDestImg);
	else if (_Compression & IMAGE_MEM_COMPRESSTYPE_VQ)
		Compress_VQ(_Quality, _pDestImg, _pTexture);
	else
		Error("Compress", CStrF("Invalid compression-type: %.8x", _Compression));
#else
	Error("Compress", CStr("No form of imagecompression is supported on consoles."));
#endif

//	LogFile(CStrF("Compressed %s from %d bytes to %d bytes", (char*)IDString(), GetSize(), _pDestImg->m_AllocSize));
}

void CImage::Decompress(CImage* _pDestImg)
{
	MAUTOSTRIP(CImage_Decompress, MAUTOSTRIP_VOID);

	if (!(GetMemModel() & IMAGE_MEM_COMPRESSED))
		Error("Compress", "Image is not compressed.");

	if (GetMemModel() & IMAGE_MEM_VIRTUAL)
		Error("Compress", "Image is virtual.");

	if (GetMemModel() & IMAGE_MEM_COMPRESSTYPE_JPG)
		Decompress_JPG(_pDestImg);
	else if (GetMemModel() & IMAGE_MEM_COMPRESSTYPE_S3TC)
		Decompress_S3TC(_pDestImg);
	else if (GetMemModel() & IMAGE_MEM_COMPRESSTYPE_3DC)
		Decompress_3DC(_pDestImg);
	else if (GetMemModel() & IMAGE_MEM_COMPRESSTYPE_CTX)
		Decompress_CTX(_pDestImg);
	else if (GetMemModel() & IMAGE_MEM_COMPRESSTYPE_SOUND)
		Decompress_Sound(_pDestImg);
	else
		Error("Decompress", "Unsupported compress-type.");
}

void CImage::Wait()
{
	MAUTOSTRIP(CImage_Wait, MAUTOSTRIP_VOID);
};

spCImage CImage::Swizzle(  )
{
	MAUTOSTRIP(CImage_Swizzle, NULL);

	spCImage spImg = MNew(CImage);
	if (spImg == NULL) MemError("Swizzle");

	spImg->Create( *this );

	Swizzle(spImg);
	return spImg;
}



/*
	fixEndian().
	Reverse the byte order within a block of bytes.
*/
#ifndef	PLATFORM_CONSOLE
static void	fixEndian( uint8 *pSrc, uint32 numBytes )
{
	MAUTOSTRIP(CImage_fixEndian, MAUTOSTRIP_VOID);

	uint8	tmp[8];	//	Large enough to hold a double.
	uint32	max, i;

	if( numBytes > 8 )
	{
		while(1);
	}

	if( (numBytes == 0) || (numBytes == 1) )	return;

	max = numBytes - 1;

	for( i=0; i<numBytes; i++ )
		tmp[(max - i)] = pSrc[i];

	for( i=0; i<numBytes; i++ )
		pSrc[i] = tmp[i];
}
#endif

/*
	fixCmpWord().
	Switch tuple and byte order within 16-bit words of an s3-packed tile to match GC hw.
	1)	switch 2-bit tuple order within bytes from ( 0,1,2,3 ) to ( 3,2,1,0 ).
	2)	leave byte order within the word as is.
*/
#ifndef	PLATFORM_CONSOLE
static void	fixCmpWord( uint16 *pData )
{
	MAUTOSTRIP(fixCmpWord, MAUTOSTRIP_VOID);

	uint16 tmp = *pData;

	//	Reverse tuple order within bytes.
	*pData =	( (tmp & 0x3 )   << 6 ) |	( (tmp & 0xC )   << 2 ) |	( (tmp & 0x30)   >> 2 ) |	( (tmp & 0xC0)   >> 6 ) |
          	( (tmp & 0x300 ) << 6 ) |	( (tmp & 0xC00 ) << 2 ) |	( (tmp & 0x3000) >> 2 ) |	( (tmp & 0xC000) >> 6 ) ;
}
#endif


#ifndef	PLATFORM_CONSOLE
void* g_start;
void* g_end;

void	CheckPtr( void *ptr )
{
	MAUTOSTRIP(CheckPtr, MAUTOSTRIP_VOID);

	if( (char*)ptr < (char*)g_start )
	{
		Error_static( "CheckPtr", CStrF( "Memory underrun by %d bytes", ((char*)g_start) - ((char*)ptr) ) );
	}

	if( (char*)ptr >= (char*)g_end)
	{
		Error_static( "CheckPtr", CStrF( "Memory overrun by %d bytes", ((char*)ptr) - ((char*)g_end) ) );
	}
}
#endif

/*
*/
#ifndef	PLATFORM_CONSOLE
static void	packTile_S3( uint8 *pData, uint32 width, uint32 height, uint32 tileX, uint32 tileY, uint16 *pDestPtr )
{
	MAUTOSTRIP(packTile_S3, MAUTOSTRIP_VOID);

	uint32	x, y;
	uint16	*pSrcPtr;
	uint16	tmp;
	uint32	srcTileOffset;
	uint32	subTileRows, subRowShorts;    // number of s3 4x4 tiles
	uint32	srcPadWidth, srcPadHeight;
	uint16	*pBuffPtr;

	//	Set the padded size of the s3 source image out to a 4-texel boundary.
	srcPadWidth		=	(width + 3)		>> 2;
	srcPadHeight	=	(height + 3)	>> 2;

	//	Number of bytes in a single row of 4x4 texel source tiles.
	srcTileOffset = srcPadWidth * 8;

	//	Number of 4x4 (source) tile rows to copy. (will be 1 or 2)
	subTileRows = 2;
	if( (srcPadHeight - tileY) < 2 )	subTileRows = 1;

	//	Number of 4x4 tile cols to copy translated into number of short values. (will be 4 or 8)
	subRowShorts = 8;
	if( (srcPadWidth - tileX) < 2 )	subRowShorts = 4;

	for( y=0; y<subTileRows; y++ )
	{
		pSrcPtr = (uint16 *)( (uint8 *)(pData) + ((tileY + y) * srcTileOffset) + (tileX*8) );
		pBuffPtr = (pDestPtr + (y*8) );		//	16 bytes per subRow = 8 shorts.

		//	Process one or both 4x4 row tiles at once- 4 shorts each.
		for( x=0; x<subRowShorts; x++ )
		{
			switch( x )
			{
				//	Color table entries - switch bytes within a 16-bit word only.
				case 0:
				case 1:
				case 4:
				case 5:
								tmp = *pSrcPtr++;
								fixEndian( (uint8 *)(&tmp), 2 );
								//CheckPtr(pBuffPtr);
								*pBuffPtr++ = tmp;
								break;

				//	2-bit color tuples:
				//	reverse tuple order within bytes of word.
				case 2:
				case 3:
				case 6:
				case 7:
								tmp = *pSrcPtr++;
								fixCmpWord( &tmp );
								//CheckPtr(pBuffPtr);
								*pBuffPtr++ = tmp;
								break;
			}
		}
	}
}
#endif

/*
*/
#ifndef	PLATFORM_CONSOLE
static	void	SwizzleS3Data( uint8 *pData, uint32 width, uint32 height, uint8 *pDest )
{
	MAUTOSTRIP(SwizzleS3Data, MAUTOSTRIP_VOID);

	uint32	tileRow, tileCol;

	uint32	srcTileRows = (height	+ 3) >> 2;	
	uint32	srcTileCols	=	(width	+ 3) >> 2;

	uint16	*pDestPtr = (uint16 *)pDest;

	for( tileRow=0; tileRow<srcTileRows; tileRow += 2 )
		for( tileCol=0; tileCol<srcTileCols; tileCol += 2 )
		{
			packTile_S3( pData, width, height, tileCol, tileRow, pDestPtr );
			pDestPtr += 16;	//	32B per dest tile, short ptr...

			CheckPtr(pDestPtr);
		}
}
#endif

/*
	RepackCompressed_Dolphin().
	This method is used when a texture is compressed with something other than S3TC (like jpg..) 
	The texture is uncompressed and stored in a GameCube-friendly format..
*/
#ifndef	PLATFORM_CONSOLE
void CImage::RepackCompressed_Dolphin(CImage *pImage)
{
	MAUTOSTRIP(CImage_RepackCompressed_Dolphin, MAUTOSTRIP_VOID);

	CImage temp;
	Decompress(&temp);

	if (GetFormat() == IMAGE_FORMAT_I8)
	{
		temp.Duplicate(this);
		AssureImageSize(16, 16);
	}
	else if (GetFormat() & IMAGE_FORMAT_ALPHA)
	{
		temp.Convert(this, IMAGE_FORMAT_BGRA4, IMAGE_CONVERT_RGBA);
		AssureImageSize(4, 4);
	}
	else
	{
		temp.Convert(this, IMAGE_FORMAT_B5G6R5, IMAGE_CONVERT_RGBA);
		AssureImageSize(4, 4);
	}

	Swizzle_Dolphin();
}
#endif

/*
	swizzleS3Dxt1().

*/
#ifndef	PLATFORM_CONSOLE
void	CImage::swizzleS3Dxt1()
{
	MAUTOSTRIP( CImage_swizzleS3Dxt1, MAUTOSTRIP_VOID );

	/*	Image is now DXT1 compressed, ready for swizzling.	*/	
	CImage	tmp;

	tmp.m_pBitmap = DNew(uint8) uint8[ m_AllocSize ];
	if( !tmp.m_pBitmap) MemError("CImage_swizzleS3Dxt1(): Memory error");

	memcpy( tmp.m_pBitmap, m_pBitmap, m_AllocSize );
	tmp.m_AllocSize	= m_AllocSize;
	tmp.m_Memmodel	= m_Memmodel;
	tmp.m_Format		= m_Format;
	tmp.m_Flags			= m_Flags;
	tmp.m_Width			= m_Width;
	tmp.m_Height		= m_Height;

	tmp.UpdateFormat();

	uint8	*pSource	= (uint8	*)tmp.LockCompressed() + sizeof(CImage_CompressHeader_S3TC);
	uint8	*pDest		= (uint8	*)this->LockCompressed() + sizeof(CImage_CompressHeader_S3TC);

	g_start = pDest;
	g_end = (char*)g_start + m_AllocSize;

	/*	Swizzle S3TC data the way the gpu wants it.	*/	
	SwizzleS3Data( pSource, this->GetWidth(), this->GetHeight(), pDest );

	this->Unlock();
	tmp.Unlock();
}
#endif

/*
	mergeDXT3_Dolphin().

*/
#ifndef	PLATFORM_CONSOLE
void	CImage::mergeDXT3_Dolphin( CImage *pAlphaImage )
{
	MAUTOSTRIP(CImage_mergeDXT3_Dolphin, MAUTOSTRIP_VOID);

	/*	Get ptr to colordata.	*/	
	uint8	*pColorData = (uint8 *)this->LockCompressed();
	const CImage_CompressHeader_S3TC	&colorHeader = *(CImage_CompressHeader_S3TC *)pColorData;

	/*	Store this image, since it will be lost in the reallocation.	*/	
	uint8	*pThisTempData = DNew(uint8) uint8[ this->m_AllocSize ];
	memcpy( pThisTempData, pColorData, this->m_AllocSize );
	uint32	tempCopySize = colorHeader.getSizeData() + colorHeader.getOffsetData();

	/*	Get ptr to alphadata.	*/	
	uint8	*pAlphaData = (uint8 *)pAlphaImage->LockCompressed();
	const CImage_CompressHeader_S3TC	&alphaHeader = *(CImage_CompressHeader_S3TC *)pAlphaData;

	/*	Logic check.	*/	
	M_ASSERT( colorHeader.getSizeData() == alphaHeader.getSizeData(), "Alpha & Color needs to be equal size" );

	/*	Build buffer containing both images.	*/	
	uint32	memSize = colorHeader.getOffsetData() + colorHeader.getSizeData() + alphaHeader.getSizeData();

///	LogFile( CStrF( "getSizeData() = %d", colorHeader.getSizeData() ) );

	/*	Reallocate mem.	*/	
	delete [] this->m_pBitmap;
	this->m_AllocSize	= memSize;

	M_ASSERT( this->m_pBitmap, "WTF? No imagedata..." );
	this->m_pBitmap = DNew(uint8) uint8[ memSize ];

	/*	Copy colordata.	*/	
	memcpy( this->m_pBitmap, pThisTempData, tempCopySize );

	uint8 *pDest = (uint8 *)this->m_pBitmap + tempCopySize;

	/*	Copy/append alphadata.	*/	
	memcpy( pDest, pAlphaData + alphaHeader.getOffsetData(), alphaHeader.getSizeData() );

	this->Unlock();
	pAlphaImage->Unlock();

	/*	Kill some mem.	*/	
	delete [] pThisTempData;
}
#endif

/*
	ConvertAlphaToColor().
	Set each pixel color equal to alpha.
*/
#ifndef	PLATFORM_CONSOLE
void	CImage::ConvertAlphaToColor()
{
	MAUTOSTRIP(CImage_ConvertAlphaToColor, MAUTOSTRIP_VOID);

	/*	This involves alpha, dude!	*/	
	if( !(this->GetFormat() & IMAGE_FORMAT_ALPHA) )
		return;

	/*	Only RGBA32, my man!	*/	
	if( this->GetFormat() != IMAGE_FORMAT_BGRA8 )
		return;

	uint8	*pPtr;

	MLock( this, pPtr );

	int mod = this->GetModulo();

	for( int y=0; y<this->m_Height; y++ )
	{
		uint8 *pSrc = (uint8 *)&pPtr[y*mod];

		for( int t=0; t<this->m_Width; t++ )
		{
			pSrc[0] = pSrc[3];
			pSrc[1] = pSrc[3];
			pSrc[2] = pSrc[3];
			pSrc += 4;
		}
	}

	MUnlock( this );
}
#endif


/*
	RepackS3TC_Dolphin().
	If texture is compressed anything other than DXT1, unpack image, store alpha, repack image as DXT1, and append alpha map.
*/
#ifndef	PLATFORM_CONSOLE
bool	CImage::RepackS3TC_Dolphin( CImage *pImage )
{
	MAUTOSTRIP(CImage_RepackS3TC_Dolphin, false);

	uint8	*pTextureData = (uint8 *)pImage->LockCompressed();

	//	Get S3TC header.
	const	CImage_CompressHeader_S3TC &Header = *(CImage_CompressHeader_S3TC *)pTextureData;

	bool	rePack;
	CStr	cmprType;

	switch( Header.getCompressType() )
	{
		case	IMAGE_COMPRESSTYPE_S3TC_DXT1:	cmprType = "DXT1";	rePack = false;	break;
		case	IMAGE_COMPRESSTYPE_S3TC_DXT3:	cmprType = "DXT3";	rePack = true;	break;
		default:
						Error( "RepackS3TC_Dolphin", "Unknown S3TC compression type" );
						return( false );
						break;
	}

	//	Output info.
///	LogFile( CStrF( "%d, ", Header.getSizeData() ) + cmprType );

	if( rePack )
	{
		CImage	temp, temp2;
		CImage	alpha8Image, alphaRgbA, alphaRgb, compressedAlphaImage;

		/*	Unpack image.	*/	
///		LogFile( "Uncompressing image..." );
		Decompress( &temp );

		/*	Store alpha.	*/	
///		LogFile( "Storing alpha..." );
		temp.Convert( &alpha8Image, IMAGE_FORMAT_A8, IMAGE_CONVERT_RGBA );
		alpha8Image.Convert( &alphaRgbA, IMAGE_FORMAT_BGRA8 );

		alphaRgbA.ConvertAlphaToColor();

		alphaRgbA.Convert( &alphaRgb, IMAGE_FORMAT_BGR8 );

		/*	Compress alphaImage.	*/	
///		LogFile( "Compressing alpha image to DXT1..." );
		alphaRgb.Compress( IMAGE_MEM_COMPRESSTYPE_S3TC, 0.8f, &compressedAlphaImage );
		compressedAlphaImage.swizzleS3Dxt1();

		/*	Convert image to RGB24.	*/	
///		LogFile( "Converting to RGB24..." );
		temp.Convert( &temp2, IMAGE_FORMAT_BGR8 );

		/*	Compress image.	*/	
///		LogFile( "Compressing image to DXT1..." );
		temp2.Compress( IMAGE_MEM_COMPRESSTYPE_S3TC, 0.8f, this );
		this->swizzleS3Dxt1();

		/*	Append the alphaImage.	*/	
///		LogFile( "Merging Color & Alpha Textures (GC Pseudo-DXT3)" );
		this->mergeDXT3_Dolphin( &compressedAlphaImage );

		/*	Make sure this image is DXT3, not DXT1. (make it appear that way.. we all know it isn´t..)*/	
		uint8	*pS3tcData = (uint8 *)this->LockCompressed();
		CImage_CompressHeader_S3TC	*pS3tcHeader = (CImage_CompressHeader_S3TC *)pS3tcData;
		pS3tcHeader->setCompressType( IMAGE_COMPRESSTYPE_S3TC_DXT3 );

		return( true );
	}

	this->swizzleS3Dxt1();

	return( true );
}
#endif


/*
	Swizzle_Dolphin().
	Swizzle to gc compatible format.
*/
#ifndef	PLATFORM_CONSOLE

#define	packedRGB565(r,g,b)   ((uint16)((((r)&0xf8)<<8)|(((g)&0xfc)<<3)|(((b)&0xf8)>>3)))
#define	packedRGBA4(r,g,b,a)  ((uint16)((((r)&0xf0)<<8)|(((g)&0xf0)<<4)|(((b)&0xf0))|(((a)&0xf0)>>4)))
#define	packedRGB5A3(r,g,b,a) ((uint16)((a)>=224?((((r)&0xf8)<<7)|(((g)&0xf8)<<2)|(((b)&0xf8)>>3)|(1<<15)):((((r)&0xf0)<<4)|(((g)&0xf0))|(((b)&0xf0)>>4)|(((a)&0xe0)<<7))))

void CImage::Swizzle_Dolphin()
{
	MAUTOSTRIP(CImage_Swizzle_Dolphin, MAUTOSTRIP_VOID);

	CImage tmp;

	tmp.m_pBitmap = DNew(uint8) uint8[ m_AllocSize ];
	if( !tmp.m_pBitmap)
		MemError("swizzle_Dolphin(): Memory error");

	memcpy( tmp.m_pBitmap, m_pBitmap, m_AllocSize );

	tmp.m_AllocSize = m_AllocSize;
	tmp.m_Memmodel  = m_Memmodel;
	tmp.m_Format    = m_Format;
	tmp.m_Flags		= m_Flags;
	tmp.m_Width     = m_Width;
	tmp.m_Height    = m_Height;

	tmp.UpdateFormat();

	if (m_Format == IMAGE_FORMAT_A8 || 
	    m_Format == IMAGE_FORMAT_CLUT8 ||
	    m_Format == IMAGE_FORMAT_I8)		// TODO: all 8bpp formats should use this code..
	{	
		uint8* pSource = (uint8*)tmp.Lock();
		uint8* pDest   = (uint8*)Lock();
		int    xbits   = Log2(m_Width);

		if (!IsPow2(m_Height) || xbits<3)
			Error("CImage::swizzle_Dolphin", CStrF("Image must be at least 8 pixels wide! (%d x %d)", m_Width, m_Height));

		for (int y=0; y<m_Height; y++)
		{
			for (int x=0; x<m_Width; x++)
			{
				// 8bpp -> 4x8 texels / cache line   (..vvvvvv ..uuuuu vv uuu)
				int nOffset = (x&7) + ((y&3)<<3) + ((x>>3)<<5) + ((y>>2) << (5+xbits-3));
				M_ASSERT(nOffset*sizeof(uint8) < m_AllocSize, "bad offset");
				pDest[nOffset] = pSource[x];
			}
			(int &)pSource += m_Modulo;
		}

		Unlock();
		tmp.Unlock();
		return;
	}

	if (m_Format == IMAGE_FORMAT_BGRA8 && m_AllocSize < 30000) // only for small textures..
	{
		uint8* pSource = (uint8*)tmp.Lock();
		uint8* pDest   = (uint8*)Lock();
		int    xbits   = Log2(m_Width);

		if (!IsPow2(m_Width) || xbits<2)
			Error("CImage::swizzle_Dolphin", CStrF("Image must be at least 4 pixels wide! (%d x %d)", m_Width, m_Height));

		for (int y=0; y<m_Height; y++)
		{
			for (int x=0; x<m_Width; x++)
			{
				// 32bpp -> 4x4 texels / 2 cache lines
				int nCacheLine = (x>>2) + ((y>>2) << (xbits-2));
				int nIndex = (x&3) + ((y&3)<<2);
				M_ASSERT(32*(nCacheLine*2+1)+nIndex*2+1 < m_AllocSize, "bad offset");
				pDest[32*(nCacheLine*2+0)+nIndex*2+0] = pSource[x*4+3];
				pDest[32*(nCacheLine*2+0)+nIndex*2+1] = pSource[x*4+2];
				pDest[32*(nCacheLine*2+1)+nIndex*2+0] = pSource[x*4+1];
				pDest[32*(nCacheLine*2+1)+nIndex*2+1] = pSource[x*4+0];
			}
			(int &)pSource += m_Modulo;
		}

		Unlock();
		tmp.Unlock();
		return;
	}

	// TODO: replace the following code with general swizzling for 16bpp and 32bpp, 
	//       and move the conversion code somewhere else..

	uint16 *pSource = (uint16 *)tmp.Lock();
	uint8  *pDest   = (uint8 *)this->Lock();

	//y*m_Modulo + x*m_Pixelsize

	g_start = pDest;
	g_end = (char *)g_start + m_AllocSize;

	uint32 xbits = Log2( m_Modulo / sizeof(short) );
	uint16 *pDst = (uint16 *)pDest;

	/*	Swizzle data the way the gc gpu wants it.	*/	
	for( int32 y=0; y<m_Height; y++ )
		for( int32 x=0; x<m_Width; x++ )
		{
			/*	Swizzled offset.	*/	
			uint32	offset = (x&3) + ((y&3)<<2) + ((x>>2)<<4) + ((y>>2) << (4+xbits-2));
			uint16	c16, color;

			//c16 = *(pSource++);
			c16 = (uint16)this->Internal_GetPixel( x, y, (uint8 *)pSource );

			if( m_Format & IMAGE_FORMAT_ALPHA )
			{
				/*	Image is in RGB4444 format.	*/	
				CPixel32	col = ConvFromRGBA4444( c16 );
				color = packedRGB5A3( col.GetR(), col.GetG(), col.GetB(), col.GetA() );
				fixEndian( (uint8 *)&color, 2 );
			}
			else
			{
				/*	Image is in RGB565 format.	*/	
				CPixel32	col = ConvFromRGB565( c16 );
				color = packedRGB565( col.GetR(), col.GetG(), col.GetB() );
				fixEndian( (uint8 *)&color, 2 );
			}

			//CheckPtr( (void *)(pDst + offset) );

			*(pDst + offset) = color;
		}

	this->Unlock();
	tmp.Unlock();
}
#endif

/*
*/
#ifndef	PLATFORM_CONSOLE
void CImage::DoubleWidth()
{
	MAUTOSTRIP(CImage_DoubleWidth, MAUTOSTRIP_VOID);

	if( !( m_Memmodel & IMAGE_MEM_LOCKABLE ) )
		return;

	CImage Img;
	Img.Create( GetWidth()*2, GetHeight(), GetFormat(), GetFormat(), 0, GetFlags() );

	int nPixelSize = Format2PixelSize(GetFormat());

	uint8 *pSrc;
	uint8 *pDst;
	MLock(this, pSrc);
	MLock((&Img), pDst);

	for (int y=0; y<GetHeight(); y++)
	{
		for (int x=0; x<GetWidth(); x+=nPixelSize)
		{
			for (int i=0; i<nPixelSize; i++)
			{
				pDst[2*x+i           ] = pSrc[x+i];
				pDst[2*x+i+nPixelSize] = pSrc[x+i];
			}
		}
		(int&)pSrc += GetModulo();
		(int&)pDst += Img.GetModulo();
	}

	MUnlock(this);
	MUnlock((&Img));

	Img.Duplicate(this);
}
#endif

/*
*/
#ifndef	PLATFORM_CONSOLE
void CImage::DoubleHeight()
{
	MAUTOSTRIP(CImage_DoubleHeight, MAUTOSTRIP_VOID);

	if( !( m_Memmodel & IMAGE_MEM_LOCKABLE ) )
		return;

	CImage Img;
	Img.Create( GetWidth(), GetHeight()*2, GetFormat(), GetFormat(), 0, GetFlags() );

	int nPixelSize = Format2PixelSize(GetFormat());

	uint8 *pSrc;
	uint8 *pDst;
	MLock(this, pSrc);
	MLock((&Img), pDst);

	int nModulo = Img.GetModulo();

	for (int y=0; y<GetHeight(); y++)
	{
		for (int x=0; x<nModulo; x+=nPixelSize)
		{
			for (int i=0; i<nPixelSize; i++)
			{
				pDst[x+i]         = pSrc[x+i];
				pDst[x+i+nModulo] = pSrc[x+i];
			}
		}
		(int&)pSrc += GetModulo();
		(int&)pDst += nModulo*2;
	}

	MUnlock(this);
	MUnlock((&Img));

	Img.Duplicate(this);

}
#endif

/*
*/
#ifndef	PLATFORM_CONSOLE
void CImage::AssureImageSize(int _MinWidth, int _MinHeight)
{
	MAUTOSTRIP(CImage_AssureImageSize, MAUTOSTRIP_VOID);

	M_ASSERT(_MinWidth > 0 && _MinHeight > 0, "bad input!");

	// Anything to do?
	if( GetWidth() >= _MinWidth && GetHeight() >= _MinHeight )
		return;

///	LogFile("Assuring image size");

	while (GetWidth() < _MinWidth)
		DoubleWidth();

	while (GetHeight() < _MinHeight)
		DoubleHeight();
}
#endif

/*
	clearLodForDetailTexture().

*/
#ifndef	PLATFORM_CONSOLE
void	CImage::clearLodForDetailTexture( void )
{
	MAUTOSTRIP(CImage_clearLodForDetailTexture, MAUTOSTRIP_VOID);

	uint8	*pTextureData = (uint8 *)this->LockCompressed();

	//	Get S3TC header.
	const	CImage_CompressHeader_S3TC &Header = *(CImage_CompressHeader_S3TC *)pTextureData;

	if( Header.getCompressType() != IMAGE_COMPRESSTYPE_S3TC_DXT1 )
		Error( "clearLodForDetailTexture", "Detail texture can only be DXT1" );

	/*	Unpack.	*/	
	CImage	temp, temp2;

///	LogFile( "Uncompressing image..." );
	Decompress( &temp );

	/*	Clear with basecolor.	*/	
	temp.Fill( this->GetRect(), CPixel32( 128, 128, 128, 128 ) );

	/*	Compress image.	*/	
///	LogFile( "Compressing image to DXT1..." );
	temp.Compress( IMAGE_MEM_COMPRESSTYPE_S3TC, 0.8f, this );
}
#endif


/*
	swizzleGC().

*/
#ifndef	PLATFORM_CONSOLE
void CImage::SwizzleGC( bool bCompressed, bool bClearLodForDetailTexture )
{
	MAUTOSTRIP(CImage_SwizzleGC, MAUTOSTRIP_VOID);

	if( !IsPow2( GetWidth() ) || !IsPow2( GetHeight() ) )
		Error( "SwizzleGC", "Textures dimensions must be power of 2 to allow for swizzling." );

	if( bCompressed )
	{
		/*	S3TC Compressed. */	
		if( this->GetMemModel() & IMAGE_MEM_COMPRESSTYPE_S3TC )
		{
			if( this->GetWidth()>4 && this->GetHeight()>4 )
			{
				if( bClearLodForDetailTexture )
					this->clearLodForDetailTexture();

				if( RepackS3TC_Dolphin( this ) == false )
					RepackCompressed_Dolphin( this );
				}
			}
		else
		{
			if( this->GetWidth()>2 && this->GetHeight()>2 )
				RepackCompressed_Dolphin( this );
		}
	}
	else
	{
		bool bNeedConversion = true;
		int  fmt = this->GetFormat();

		if (fmt == IMAGE_FORMAT_A8 || fmt == IMAGE_FORMAT_CLUT8 || fmt == IMAGE_FORMAT_I8)
		{
			bNeedConversion = false;
			AssureImageSize(16, 16);
		}
		else if (fmt == IMAGE_FORMAT_BGRA8 && m_AllocSize < 30000)
		{
			bNeedConversion = false;
			AssureImageSize(4, 4);
		}

		if (bNeedConversion)
		{
			spCImage spImg = MNew(CImage);
			if (spImg == NULL) MemError("Unswizzle");

			Duplicate(spImg);

			if (fmt == IMAGE_FORMAT_BGRX8 && m_AllocSize < 30000)
			{
				spImg->Convert(this, IMAGE_FORMAT_BGRA8, IMAGE_CONVERT_RGBA);
			}
			if (fmt & IMAGE_FORMAT_ALPHA)
			{
///				LogFile( "Converting image with alpha" );
				spImg->Convert(this, IMAGE_FORMAT_BGRA4, IMAGE_CONVERT_RGBA);
			}
			else
			{
///				LogFile( "Converting image without alpha" );
				spImg->Convert( this, IMAGE_FORMAT_B5G6R5 );
			}

			AssureImageSize(4, 4);
		}

		// Swizzle.
		Swizzle_Dolphin();
	}
}

static void ReadPSMCT32(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void *data, unsigned int *pGSMem);
static void WritePSMT8(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void *data, unsigned int *pGSMem);

void CImage::SwizzlePS2( void *pGSMem )
{
	MAUTOSTRIP(CImage_SwizzlePS2, MAUTOSTRIP_VOID);

	if( !IsPow2( GetWidth() ) || !IsPow2( GetHeight() ) )
		Error( "SwizzlePS2", "Textures dimensions must be power of 2 to allow for swizzling." );

	// Mask unswizzeble textures
	if(GetWidth() < 16 || GetHeight() < 8)		// No smaller than one block (block in 8bpp is 16x16. block in 32bpp is 8x8)
		return;
	if(GetWidth() <= 64 && GetHeight() > 64)	// If width less than one page-width in 8bpp, the height must not be higher than a page height (page 8bpp: 128x64, 32bpp: 64x64)
		return;
	// else, width is a multiple of the page width in 8bpp, so no restriction on the height

	if(GetWidth() > 512 || GetHeight() > 512)	// PB--PS2 don't support larger than this
		return;

	void *pData;
	int  fmt = this->GetFormat();
	if(this->GetMemModel() & IMAGE_MEM_COMPRESSED)
		return;
/*	{
		fmt = IMAGE_FORMAT_CLUT8;
		pData = LockCompressed();
	}
	else*/
		pData = Lock();

	if (fmt == IMAGE_FORMAT_CLUT8)
	{
		if(pData)
		{
			int BW = (((GetWidth() + 127) & ~127) >> 6);
			WritePSMT8(0, BW, 0, 0, GetWidth(), GetHeight(), pData, (unsigned int *)pGSMem);			
			BW = ((((GetWidth()>>1) + 63) & ~63) >> 6);
			ReadPSMCT32(0, BW, 0, 0, GetWidth() >> 1, GetHeight() >> 1, pData, (unsigned int *)pGSMem);
			m_Flags |= IMAGE_FLAGS_SWIZZLED;
		}
	}
	if(pData)
		Unlock();
}

void ReadPSMCT32(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void *data, unsigned int *pGSMem)
{
	int block32[32] =
	{
		0,  1,  4,  5, 16, 17, 20, 21,
			2,  3,  6,  7, 18, 19, 22, 23,
			8,  9, 12, 13, 24, 25, 28, 29,
			10, 11, 14, 15, 26, 27, 30, 31
	};


	int columnWord32[16] =
	{
		0,  1,  4,  5,  8,  9, 12, 13,
			2,  3,  6,  7, 10, 11, 14, 15
	};

	unsigned int *src = (unsigned int *)data;
	int startBlockPos = dbp * 64;

	for(int y = dsay; y < dsay + rrh; y++)
	{
		for(int x = dsax; x < dsax + rrw; x++)
		{
			int pageX = x / 64;
			int pageY = y / 32;
			int page  = pageX + pageY * dbw;

			int px = x - (pageX * 64);
			int py = y - (pageY * 32);

			int blockX = px / 8;
			int blockY = py / 8;
			int block  = block32[blockX + blockY * 8];

			int bx = px - blockX * 8;
			int by = py - blockY * 8;

			int column = by / 2;

			int cx = bx;
			int cy = by - column * 2;
			int cw = columnWord32[cx + cy * 8];

			*src = pGSMem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
			src++;
		}
	}
}

void WritePSMT8(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void *data, unsigned int *pGSMem)
{
	int block8[32] =
	{
		0,  1,  4,  5, 16, 17, 20, 21,
			2,  3,  6,  7, 18, 19, 22, 23,
			8,  9, 12, 13, 24, 25, 28, 29,
			10, 11, 14, 15, 26, 27, 30, 31
	};

	int columnWord8[2][64] = 
	{
		{
			0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
				2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,

				8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,
				10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7
		},
		{
			8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,
				10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,

				0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
				2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
		}
	};

	int columnByte8[64] = 
	{
		0, 0, 0, 0, 0, 0, 0, 0,  2, 2, 2, 2, 2, 2, 2, 2,
			0, 0, 0, 0, 0, 0, 0, 0,  2, 2, 2, 2, 2, 2, 2, 2,

			1, 1, 1, 1, 1, 1, 1, 1,  3, 3, 3, 3, 3, 3, 3, 3,
			1, 1, 1, 1, 1, 1, 1, 1,  3, 3, 3, 3, 3, 3, 3, 3
	};

	dbw >>= 1;
	unsigned char *src = (unsigned char *)data;
	int startBlockPos = dbp * 64;

	for(int y = dsay; y < dsay + rrh; y++)
	{
		for(int x = dsax; x < dsax + rrw; x++)
		{
			int pageX = x / 128;
			int pageY = y / 64;
			int page  = pageX + pageY * dbw;

			int px = x - (pageX * 128);
			int py = y - (pageY * 64);

			int blockX = px / 16;
			int blockY = py / 16;
			int block  = block8[blockX + blockY * 8];

			int bx = px - (blockX * 16);
			int by = py - (blockY * 16);

			int column = by / 4;

			int cx = bx;
			int cy = by - column * 4;
			int cw = columnWord8[column & 1][cx + cy * 16];
			int cb = columnByte8[cx + cy * 16];

			unsigned char *dst = (unsigned char *)&pGSMem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
			dst[cb] = *src++;
		}
	}
}

#endif

/*
	Swizzle().

*/
void CImage::Swizzle(CImage* _pDestImg)
{
	MAUTOSTRIP(CImage_Swizzle_pDestImg, MAUTOSTRIP_VOID);

	int x, y;

	if (GetMemModel() & IMAGE_MEM_COMPRESSED)
		Error( "Swizzle", "Cannot swizzle compressed textures." );

	if ( ( GetWidth() != _pDestImg->GetWidth() ) || ( GetHeight() != _pDestImg->GetHeight() ) ) 
		Error( "Swizzle", "Destination image is not of same size." );

	if( !IsPow2( GetWidth() ) || !IsPow2( GetHeight() ) )
		Error( "Swizzle", "Textures dimensions must be power of 2 to allow for swizzling." );

	M_ASSERT( CTC_MAXTEXTURESIZE <= 2048, "Max texturesize for swizzling is locked at 2048." );

	int height = GetHeight(), width = GetWidth();
	int chunks = height / width;
	if ( chunks == 0 )
		chunks	= 1;
	else
		height	= GetWidth();

	int shifty = Log2( width );
	int yoffset = 0;
	CPnt TopLeft( 0, 0 );
	CPnt BottomRight( GetWidth(), GetHeight() );
	CClipRect sClipRect( CRct( TopLeft, BottomRight ), CPnt( 0, 0 ) );

	while ( chunks > 0 )
	{
		CPnt	sPos;
		for ( y = 0; y < height; y++ )
		{
			for ( x = 0; x < width; x++ )
			{
#ifdef	CPU_X86
				int nDest;

				__asm
				{
					xor eax, eax
					mov ecx, [x]
					mov edx, [y]
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1

					shr eax, 32-22
					mov [nDest], eax
				}
#else

				int nValue, nDest = 0;
				nValue	= x & 2048;	nDest	+= nValue * nValue;
				nValue	= x & 1024;	nDest	+= nValue * nValue;
				nValue	= x & 512;	nDest	+= nValue * nValue;
				nValue	= x & 256;	nDest	+= nValue * nValue;
				nValue	= x & 128;	nDest	+= nValue * nValue;
				nValue	= x & 64;	nDest	+= nValue * nValue;
				nValue	= x & 32;	nDest	+= nValue * nValue;
				nValue	= x & 16;	nDest	+= nValue * nValue;
				nValue	= x & 8;	nDest	+= nValue * nValue;
				nValue	= x & 4;	nDest	+= nValue * nValue;
				nValue	= x & 2;	nDest	+= nValue * nValue;
				nValue	= x & 1;	nDest	+= nValue;

				nValue	= y & 2048;	nDest	+= nValue * nValue * 2;
				nValue	= y & 1024;	nDest	+= nValue * nValue * 2;
				nValue	= y & 512;	nDest	+= nValue * nValue * 2;
				nValue	= y & 256;	nDest	+= nValue * nValue * 2;
				nValue	= y & 128;	nDest	+= nValue * nValue * 2;
				nValue	= y & 64;	nDest	+= nValue * nValue * 2;
				nValue	= y & 32;	nDest	+= nValue * nValue * 2;
				nValue	= y & 16;	nDest	+= nValue * nValue * 2;
				nValue	= y & 8;	nDest	+= nValue * nValue * 2;
				nValue	= y & 4;	nDest	+= nValue * nValue * 2;
				nValue	= y & 2;	nDest	+= nValue * nValue * 2;
				nValue	= y & 1;	nDest	+= nValue * 2;
#endif
				// Translate pixelnumber into pixelcoordinates
				sPos.x	= nDest & ( width - 1 );
				sPos.y	= yoffset + ( nDest >> shifty );

				_pDestImg->SetPixel( sClipRect, sPos, GetPixel( sClipRect, CPnt( x, y + yoffset ) ) );
			}
		}

		yoffset	+= height;
		chunks--;
	}

	_pDestImg->m_Flags	|= IMAGE_FLAGS_SWIZZLED;
	_pDestImg->UpdateFormat();
}

/*
*/
void CImage::Swizzle(void *_pDest, int _Pitch, void *_pSrc, int _Width, int _Height, int _PixelSize)
{
	MAUTOSTRIP(CImage_Swizzle_pDest_Pitch_pSrc_Width_Height_PixelSize, MAUTOSTRIP_VOID);

	int x, y;

	if( !IsPow2( _Width ) || !IsPow2( _Height ) )
		Error_static( "Swizzle", "Textures dimensions must be power of 2 to allow for swizzling." );

	M_ASSERT( CTC_MAXTEXTURESIZE <= 2048, "Max texturesize for swizzling is locked at 2048." );

	int height = _Height, width = _Width;
	int chunks = height / width;
	if ( chunks == 0 )
		chunks	= 1;
	else
		height	= _Width;

	int shifty = Log2( width );
	int yoffset = 0;
//	CClipRect sClipRect( CRct( CPnt( 0, 0 ), CPnt( _Width, _Height ) ), CPnt( 0, 0 ) );

	while ( chunks > 0 )
	{
		CPnt	sPos;
		for ( y = 0; y < height; y++ )
		{
			for ( x = 0; x < width; x++ )
			{
#ifdef	CPU_X86
				int nDest;

				__asm
				{
					xor eax, eax
					mov ecx, [x]
					mov edx, [y]
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1

					shr eax, 32-22
					mov [nDest], eax
				}
#else

				int nValue, nDest = 0;
				nValue	= x & 2048;	nDest	+= nValue * nValue;
				nValue	= x & 1024;	nDest	+= nValue * nValue;
				nValue	= x & 512;	nDest	+= nValue * nValue;
				nValue	= x & 256;	nDest	+= nValue * nValue;
				nValue	= x & 128;	nDest	+= nValue * nValue;
				nValue	= x & 64;	nDest	+= nValue * nValue;
				nValue	= x & 32;	nDest	+= nValue * nValue;
				nValue	= x & 16;	nDest	+= nValue * nValue;
				nValue	= x & 8;	nDest	+= nValue * nValue;
				nValue	= x & 4;	nDest	+= nValue * nValue;
				nValue	= x & 2;	nDest	+= nValue * nValue;
				nValue	= x & 1;	nDest	+= nValue;

				nValue	= y & 2048;	nDest	+= nValue * nValue * 2;
				nValue	= y & 1024;	nDest	+= nValue * nValue * 2;
				nValue	= y & 512;	nDest	+= nValue * nValue * 2;
				nValue	= y & 256;	nDest	+= nValue * nValue * 2;
				nValue	= y & 128;	nDest	+= nValue * nValue * 2;
				nValue	= y & 64;	nDest	+= nValue * nValue * 2;
				nValue	= y & 32;	nDest	+= nValue * nValue * 2;
				nValue	= y & 16;	nDest	+= nValue * nValue * 2;
				nValue	= y & 8;	nDest	+= nValue * nValue * 2;
				nValue	= y & 4;	nDest	+= nValue * nValue * 2;
				nValue	= y & 2;	nDest	+= nValue * nValue * 2;
				nValue	= y & 1;	nDest	+= nValue * 2;
#endif
				// Translate pixelnumber into pixelcoordinates
				sPos.x	= nDest & ( width - 1 );
				sPos.y	= yoffset + ( nDest >> shifty );

				memcpy((uint8 *)_pDest + sPos.x * _PixelSize + sPos.y * _PixelSize * _Pitch, (uint8 *)_pSrc + x*_PixelSize + (y + yoffset) * _PixelSize * _Width, _PixelSize);

			}
		}

		yoffset	+= height;
		chunks--;
	}
}

spCImage CImage::Unswizzle()
{
	MAUTOSTRIP(CImage_Unswizzle, NULL);

	spCImage spImg = MNew(CImage);
	if (spImg == NULL) MemError("Unswizzle");

	spImg->Create( *this );

	Unswizzle(spImg);
	return spImg;
}

void CImage::Unswizzle(CImage * _pDestImg)
{
	MAUTOSTRIP(CImage_Unswizzle_pDestImg, MAUTOSTRIP_VOID);

	int x, y;

	if (!(GetFlags() & IMAGE_FLAGS_SWIZZLED))
		Error( "Swizzle", "Cannot unswizzle non-swizzled textures." );

	if ( ( GetWidth() != _pDestImg->GetWidth() ) || ( GetHeight() != _pDestImg->GetHeight() ) ) 
		Error( "Swizzle", "Destination image is not of same size." );

	// This test is obsolete since swizzled textures always are a power of 2
	if( !IsPow2( GetWidth() ) || !IsPow2( GetHeight() ) )
		Error( "Swizzle", "Textures dimensions must be power of 2 to allow for swizzling." );

	M_ASSERT( CTC_MAXTEXTURESIZE <= 2048, "Max texturesize for swizzling is locked at 2048." );

	int height = GetHeight(), width = GetWidth();
	int chunks = height / width;
	if ( chunks == 0 )
		chunks	= 1;
	else
		height	= GetWidth();

	int shifty = Log2( width );
	int yoffset = 0;
	CPnt TopLeft( 0, 0 );
	CPnt BottomRight( GetWidth(), GetHeight() );
	CClipRect sClipRect( CRct( TopLeft, BottomRight ), CPnt( 0, 0 ) );
//	CClipRect sClipRect( CRct( CPnt( 0, 0 ), CPnt( GetWidth(), GetHeight() ) ), CPnt( 0, 0 ) );

	while ( chunks > 0 )
	{
		CPnt	sPos;
		for ( y = 0; y < height; y++ )
		{
			for ( x = 0; x < width; x++ )
			{
#ifdef	CPU_X86
				int nDest;

				__asm
				{
					xor eax, eax
					mov ecx, [x]
					mov edx, [y]
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1
					shr ecx, 1
					rcr eax, 1
					shr edx, 1
					rcr eax, 1

					shr eax, 32-22
					mov [nDest], eax
				}
#else

				int nValue, nDest = 0;
				nValue	= x & 2048;	nDest	+= nValue * nValue;
				nValue	= x & 1024;	nDest	+= nValue * nValue;
				nValue	= x & 512;	nDest	+= nValue * nValue;
				nValue	= x & 256;	nDest	+= nValue * nValue;
				nValue	= x & 128;	nDest	+= nValue * nValue;
				nValue	= x & 64;	nDest	+= nValue * nValue;
				nValue	= x & 32;	nDest	+= nValue * nValue;
				nValue	= x & 16;	nDest	+= nValue * nValue;
				nValue	= x & 8;	nDest	+= nValue * nValue;
				nValue	= x & 4;	nDest	+= nValue * nValue;
				nValue	= x & 2;	nDest	+= nValue * nValue;
				nValue	= x & 1;	nDest	+= nValue;

				nValue	= y & 2048;	nDest	+= nValue * nValue * 2;
				nValue	= y & 1024;	nDest	+= nValue * nValue * 2;
				nValue	= y & 512;	nDest	+= nValue * nValue * 2;
				nValue	= y & 256;	nDest	+= nValue * nValue * 2;
				nValue	= y & 128;	nDest	+= nValue * nValue * 2;
				nValue	= y & 64;	nDest	+= nValue * nValue * 2;
				nValue	= y & 32;	nDest	+= nValue * nValue * 2;
				nValue	= y & 16;	nDest	+= nValue * nValue * 2;
				nValue	= y & 8;	nDest	+= nValue * nValue * 2;
				nValue	= y & 4;	nDest	+= nValue * nValue * 2;
				nValue	= y & 2;	nDest	+= nValue * nValue * 2;
				nValue	= y & 1;	nDest	+= nValue * 2;
#endif
				// Translate pixelnumber into pixelcoordinates
				sPos.x	= nDest & ( width - 1 );
				sPos.y	= yoffset + ( nDest >> shifty );

				_pDestImg->SetPixel( sClipRect, CPnt( x, y + yoffset ), GetPixel( sClipRect, sPos ) );
			}
		}

		yoffset	+= height;
		chunks--;
	}

	_pDestImg->m_Flags	&= ~IMAGE_FLAGS_SWIZZLED;
	_pDestImg->UpdateFormat();
}

// -------------------------------------------------------------------

void CImage::SetPalette(spCImagePalette _spPal)
{
	MAUTOSTRIP(CImage_SetPalette, MAUTOSTRIP_VOID);

	if (_spPal == NULL) Error("SetPalette", "NULL palette.");
	m_spPalette = _spPal;
	if (GetFormat() & IMAGE_FORMAT_PALETTE)
	{
	};
};

int CImage::Format2BPP(int _format) 
{
	MAUTOSTRIP(CImage_Format2BPP, 0);

	switch (_format) {
		case IMAGE_FORMAT_BGR10A2 : return 32;
		case IMAGE_FORMAT_CLUT8 : return 8;
		case IMAGE_FORMAT_RGB10A2_F : return 32;
		case IMAGE_FORMAT_BGR5 : return 16;
		case IMAGE_FORMAT_B5G6R5 : return 16;
		case IMAGE_FORMAT_BGR8 : return 24;
		case IMAGE_FORMAT_BGRX8 : return 32;
		case IMAGE_FORMAT_RGB16 : return 48;
		case IMAGE_FORMAT_BGR5A1 : return 16;
		case IMAGE_FORMAT_BGRA4 : return 16;
//		case IMAGE_FORMAT_RGBA3328 : return 16;
		case IMAGE_FORMAT_BGRA8 : return 32;
		case IMAGE_FORMAT_BGRA16 : return 64;
		case IMAGE_FORMAT_I8 : return 8;
		case IMAGE_FORMAT_RGBA8 : return 32;
		case IMAGE_FORMAT_I16 : return 16;
		case IMAGE_FORMAT_RGB8 : return 24;
		case IMAGE_FORMAT_I8A8 : return 16;
		case IMAGE_FORMAT_A8 : return 8;
		case IMAGE_FORMAT_RGB32_F : return 12*8;
		case IMAGE_FORMAT_RGBA32_F : return 16*8;
		case IMAGE_FORMAT_RGBA16 : return 64;
		case IMAGE_FORMAT_RGBA16_F : return 64;
//		case IMAGE_FORMAT_ZBUFFER24 : return 24;
		case IMAGE_FORMAT_Z24S8 : return 32;
/*		case IMAGE_FORMAT_V8U8 : return 16;
		case IMAGE_FORMAT_GREY_F16 : return 16;
		case IMAGE_FORMAT_UYVY : return 32;
		case IMAGE_FORMAT_YUY2 : return 32;
		case IMAGE_FORMAT_V16U16 : return 32;
		case IMAGE_FORMAT_G16R16 : return 32;
		case IMAGE_FORMAT_A2W10V10U10 : return 32;
		case IMAGE_FORMAT_R11G11B10 : return 32;
		case IMAGE_FORMAT_W11V11U10 : return 32;
		case IMAGE_FORMAT_G16R16F : return 32;
		case IMAGE_FORMAT_R32F : return 32;
		case IMAGE_FORMAT_G32R32F : return 64;
		case IMAGE_FORMAT_A32B32G32R32F : return 128;*/
		default : return 0;
	};
};

int CImage::BPP2Format(int bpp) 
{
	MAUTOSTRIP(CImage_BPP2Format, 0);

	// A not too rigorous converter, but Bill thinks that bpp is enough.
	switch (bpp) {
		case 15 : return IMAGE_FORMAT_BGR5;
		case 16 : return IMAGE_FORMAT_B5G6R5;
		case 24 : return IMAGE_FORMAT_BGR8;
		case 32 : return IMAGE_FORMAT_BGRX8 | IMAGE_FORMAT_RGBA8 | IMAGE_FORMAT_BGRA8;
		case 48 : return IMAGE_FORMAT_RGB16;
		case 64 : return IMAGE_FORMAT_RGBA16 | IMAGE_FORMAT_RGBA16_F;
		default : return 0;
	};
};

int CImage::ZBufferBits2Format(int _zbits)
{
	MAUTOSTRIP(CImage_ZBufferBits2Format, 0);

	switch (_zbits) {
/*		case 8 : return IMAGE_FORMAT_ZBUFFER8;
		case 16 : return IMAGE_FORMAT_ZBUFFER16;
		case 24 : return IMAGE_FORMAT_ZBUFFER24;*/
		case 32 : return IMAGE_FORMAT_Z24S8;
		default : return 0;
	};
};

int CImage::GetMemSize() const
{
	MAUTOSTRIP(CImage_GetMemSize, 0);

	if(m_AllocSize)
		return m_AllocSize;

	fp32 PixelSize = 0.5f;
	if(m_Format == IMAGE_FORMAT_ALPHA)
		PixelSize = 1;

	return RoundToInt(GetWidth() * GetHeight() * PixelSize);
}

const char* CImage::GetFormatName(int _format)
{
	MAUTOSTRIP(CImage_GetFormatName, NULL);

	switch (_format) {
		case IMAGE_FORMAT_BGR10A2 : return ("A2R10G10B10");
		case IMAGE_FORMAT_CLUT8 : return ("8-Bit Palette");
		case IMAGE_FORMAT_RGB10A2_F : return ("A2B10G10R10F");
		case IMAGE_FORMAT_BGR5 : return ("RGB-555");
		case IMAGE_FORMAT_B5G6R5 : return ("RGB-565");
		case IMAGE_FORMAT_BGR8 : return ("RGB-24");
		case IMAGE_FORMAT_BGRX8 : return ("RGB-32");
		case IMAGE_FORMAT_RGB16 : return ("RGB-48");
		case IMAGE_FORMAT_BGR5A1 : return ("RGBA-5551");
		case IMAGE_FORMAT_BGRA4 : return ("RGBA-4444");
//		case IMAGE_FORMAT_RGBA3328 : return ("RGBA-3328");
		case IMAGE_FORMAT_BGRA8 : return ("RGBA-32");
		case IMAGE_FORMAT_BGRA16 : return ("RGBA-64");
		case IMAGE_FORMAT_I8 : return ("GRAY8");
		case IMAGE_FORMAT_RGBA8 : return ("ABGR-32");
		case IMAGE_FORMAT_I16 : return ("GRAY16");
		case IMAGE_FORMAT_RGB8 : return ("BGR-24");
		case IMAGE_FORMAT_I8A8 : return ("GRAY8-ALPHA8");
		case IMAGE_FORMAT_A8 : return ("ALPHA8");
		case IMAGE_FORMAT_RGB32_F : return ("RGB-FLOAT");
		case IMAGE_FORMAT_RGBA32_F : return ("RGBA-FLOAT");
		case IMAGE_FORMAT_RGBA16 : return ("A16B16G16R16");
		case IMAGE_FORMAT_RGBA16_F : return ("A16B16G16R16F");
		case IMAGE_FORMAT_Z24S8 : return ("Z24STENCIL8");
/*		case IMAGE_FORMAT_V8U8 : return ("V8U8");
		case IMAGE_FORMAT_GREY_F16 : return ("GREY_F16");
		case IMAGE_FORMAT_UYVY : return ("UYVY");
		case IMAGE_FORMAT_YUY2 : return ("YUY2");
		case IMAGE_FORMAT_V16U16 : return ("V16U16");
		case IMAGE_FORMAT_G16R16 : return ("G16R16");
		case IMAGE_FORMAT_A2W10V10U10 : return ("A2W10V10U10");
		case IMAGE_FORMAT_R11G11B10 : return ("R11G11B10");
		case IMAGE_FORMAT_W11V11U10 : return ("W11V11U10");
		case IMAGE_FORMAT_G16R16F : return ("G16R16F");
		case IMAGE_FORMAT_R32F : return ("R32F");
		case IMAGE_FORMAT_G32R32F : return ("G32R32F");
		case IMAGE_FORMAT_A32B32G32R32F : return ("A32B32G32R32F");*/

		default : return ("VOID");
	};
};

const char* CImage::GetMemModelName(int _memmodel)
{
	MAUTOSTRIP(CImage_GetMemModelName, NULL);

	switch (_memmodel & IMAGE_MEM_TYPEMASK) {
		case IMAGE_MEM_VOID : return("Void");
		case IMAGE_MEM_OFFSCREEN : return("OffScr");
		case IMAGE_MEM_PRIMARY : return("Primary");
		case IMAGE_MEM_PRIMARYDOUBLE : return("PrimaryDB");
		case IMAGE_MEM_TEXTURE : return("Texture");
		case IMAGE_MEM_ZBUFFER : return ("ZBuffer");
		default : return("Unknown");
	};

//	if ((_memmodel & IMAGE_MEM_WINDOWED) != 0) ms += " (W)";
//	if ((_memmodel & IMAGE_MEM_BACKBUFFER) != 0) ms += " (B)";
//	return ms;
};

const char* CImage::GetMemTypeName(int _memmodel)
{
	MAUTOSTRIP(CImage_GetMemTypeName, NULL);

	if ((_memmodel & IMAGE_MEM_SYSTEM) != 0) return("SystemMem");
	if ((_memmodel & IMAGE_MEM_VIDEO) != 0) return("VideoMem");
	return("Void");
};

CStr CImage::IDString() const
{
	MAUTOSTRIP(CImage_IDString, CStr());

	CStr s;
	if ((GetMemModel() & IMAGE_MEM_WINDOWED) != 0)
		s = CStrF("Windowed %s, %s, %s", GetFormatName(GetFormat()), GetMemModelName(GetMemModel()), GetMemTypeName(GetMemModel()));
	else
		s = CStrF("%dx%d %s, %s, %s", GetWidth(), GetHeight(), GetFormatName(GetFormat()), GetMemModelName(GetMemModel()), GetMemTypeName(GetMemModel()));
	return s;
};

// -------------------------------------------------------------------
//  General pixel format converter
// -------------------------------------------------------------------

int CImage::Convert2Format(int _indexed, int bitspp, int rdepth, int gdepth, int bdepth, int adepth)
{
	MAUTOSTRIP(CImage_Convert2Format, 0);

	if (_indexed > 0) {
		switch (bitspp) {
//			case 4 : return(IMAGE_FORMAT_CLUT4);
			case 8 : return(IMAGE_FORMAT_CLUT8);
			default : return(IMAGE_FORMAT_VOID);
		};
	} else {

//		if ((bitspp == 8) && (rdepth == 3) && (gdepth == 3) && (bdepth == 2) && (adepth == 0)) return(IMAGE_FORMAT_RGB332);
		if ((bitspp == 16) && (rdepth == 5) && (gdepth == 5) && (bdepth == 5) && (adepth == 0)) return(IMAGE_FORMAT_BGR5);
		if ((bitspp == 16) && (rdepth == 5) && (gdepth == 6) && (bdepth == 5) && (adepth == 0)) return(IMAGE_FORMAT_B5G6R5);
		if ((bitspp == 24) && (rdepth == 8) && (gdepth == 8) && (bdepth == 8) && (adepth == 0)) return(IMAGE_FORMAT_BGR8);
		if ((bitspp == 48) && (rdepth == 16) && (gdepth == 16) && (bdepth == 16) && (adepth == 0)) return(IMAGE_FORMAT_RGB16);
		if ((bitspp == 32) && (rdepth == 8) && (gdepth == 8) && (bdepth == 8) && (adepth == 0)) return(IMAGE_FORMAT_BGRX8);

		if ((bitspp == 16) && (rdepth == 5) && (gdepth == 6) && (bdepth == 5) && (adepth == 1)) return(IMAGE_FORMAT_BGR5A1);
		if ((bitspp == 16) && (rdepth == 4) && (gdepth == 4) && (bdepth == 4) && (adepth == 4)) return(IMAGE_FORMAT_BGRA4);
//		if ((bitspp == 16) && (rdepth == 3) && (gdepth == 3) && (bdepth == 2) && (adepth == 8)) return(IMAGE_FORMAT_RGBA3328);
		if ((bitspp == 32) && (rdepth == 8) && (gdepth == 8) && (bdepth == 8) && (adepth == 8)) return(IMAGE_FORMAT_BGRA8);
		if ((bitspp == 64) && (rdepth == 16) && (gdepth == 16) && (bdepth == 16) && (adepth == 8)) return(IMAGE_FORMAT_BGRA16);

		return(IMAGE_FORMAT_VOID);
	};
};

// -------------------------------------------------------------------
//  Image loaders
// -------------------------------------------------------------------
spCImage CImage::ReadInfo(const CStr& filename)
{
	MAUTOSTRIP(CImage_ReadInfo, NULL);

	TPtr<CImageIO> spIO = CImageIO::GetImageIO(filename);

	spCImage spImgInfo;
	spImgInfo = spIO->ReadInfo(filename);
	return spImgInfo;
};

void CImage::Read(const CStr& filename, int _MemModel)
{
	MAUTOSTRIP(CImage_Read_srcrect_filename_MemModel, MAUTOSTRIP_VOID);

	TPtr<CImageIO> spIO = CImageIO::GetImageIO(filename);

	Destroy();
	spIO->Read(filename, this, _MemModel);
};

void CImage::Write(const CStr& filename)
{
	MAUTOSTRIP(CImage_Write, MAUTOSTRIP_VOID);

	TPtr<CImageIO> spIO = CImageIO::GetImageIO(filename);
	spIO->Write(filename, this);
};


// -------------------------------------------------------------------
void CImage::ReadRAW(CCFile* _pFile, int _Width, int _Height, int _Format, int _MemModel, int _Flags, spCImagePalette _spPal)
{
	MAUTOSTRIP(CImage_ReadRAW, MAUTOSTRIP_VOID);

	Create(_Width, _Height, _Format, _MemModel, _spPal, _Flags);

	int wsize = GetWidth() * GetPixelSize();
	int h = GetHeight();

	uint8* pSurfMem = NULL;
	MLock(this, pSurfMem);
	for(int y = 0; y < h; y++)
		_pFile->Read(pSurfMem + m_Modulo * y, wsize);
	MUnlock(this);


/* AR-NOTE: This code shouldn't be needed (the code above works fine)

	uint8* pScanLine = DNew(uint8) uint8[wsize+16];
	if (!pScanLine) MemError("Read");
	try
	{
		for(int y = 0; y < h; y++)
		{
			_pFile->Read(pScanLine, wsize);
			SetRAWData(CPnt(0, y), wsize, pScanLine);
		}
	}
	catch(CCException)
	{
		if (pScanLine) delete[] pScanLine;
		pScanLine = NULL;
		throw;
	}
	if (pScanLine) delete[] pScanLine;
	pScanLine = NULL;
*/	
}

void CImage::WriteRAW(CCFile* _pFile)
{
	MAUTOSTRIP(CImage_WriteRAW, MAUTOSTRIP_VOID);

	int wsize = GetWidth() * GetPixelSize();
	int h = GetHeight();
	if ((wsize < 0) || (h < 0))
		Error("Write", "Invalid image.");

	uint8* pScanLine = DNew(uint8) uint8[wsize+16];
	if (!pScanLine) MemError("Write");
	M_TRY
	{
		for(int y = 0; y < h; y++)
		{
			GetRAWData(CPnt(0, y), wsize, pScanLine);
			_pFile->Write(pScanLine, wsize);
		}
	}
	M_CATCH(
	catch(CCException)
	{
		if (pScanLine) delete[] pScanLine;
		pScanLine = NULL;
		throw;
	}
	)
	if (pScanLine) delete[] pScanLine;
	pScanLine = NULL;
}

// -------------------------------------------------------------------
#define IMAGE_FILEHEADER_VERSION	0x0300

class CImage_FileHeader
{
public:
	int32 m_Compression;
	int32 m_AllocSize;
	int32 m_Width;
	int32 m_Height;
	int32 m_Format;
	int32 m_Flags;

	CImage_FileHeader()
	{
		MAUTOSTRIP(CImage_FileHeader_ctor, MAUTOSTRIP_VOID);

		m_Compression = 0;
		m_AllocSize = 0;
		m_Width = 0;
		m_Height = 0;
		m_Format = 0;
		m_Flags = 0;
	}


	void Write(CCFile* _pFile)
	{
		MAUTOSTRIP(CImage_FileHeader_Write, MAUTOSTRIP_VOID);

		uint32 Ver(IMAGE_FILEHEADER_VERSION);
		_pFile->WriteLE(Ver);
		_pFile->WriteLE(m_Compression);
		_pFile->WriteLE(m_AllocSize);
		_pFile->WriteLE(m_Width);
		_pFile->WriteLE(m_Height);
		_pFile->WriteLE(m_Format);
		_pFile->WriteLE(m_Flags);
	}

	void Read(CCFile* _pFile)
	{
		MAUTOSTRIP(CImage_FileHeader_Read, MAUTOSTRIP_VOID);
		uint32 Ver;
		_pFile->ReadLE(Ver);

		switch(Ver)
		{
		case 0x0200 :
			_pFile->ReadLE(&m_Compression, 5); // read all at once
			break;
		case 0x0300 :
			// wow, this is nice. make sure to only use int32 in this struct or this won't work on big endian
			_pFile->ReadLE(&m_Compression, 6); // read all at once
			break;

		case 0 :
			{
				_pFile->ReadLE(m_Width);
				_pFile->ReadLE(m_Height);
				_pFile->ReadLE(m_Format);
			}
			break;
		default :
			Error_static("CImage_FileHeader::Write", CStrF("Unsupported version %.4x", Ver));
		}
	}

	void UnmakeLE()
	{
	}
};


void CImage::ReadHeader(CCFile* _pFile)
{
	MAUTOSTRIP(CImage_ReadHeader, MAUTOSTRIP_VOID);

	CImage_FileHeader H;
	H.Read(_pFile);
	CreateVirtual(H.m_Width, H.m_Height, H.m_Format, IMAGE_MEM_VIRTUAL | H.m_Compression, 0, H.m_Flags);
}

void CImage::WriteHeader(CCFile* _pFile) const
{
	MAUTOSTRIP(CImage_WriteHeader, MAUTOSTRIP_VOID);

	CImage_FileHeader H;
	H.m_Compression = GetMemModel() & IMAGE_MEM_COMPRESS_ALLFLAGS;
	H.m_AllocSize = m_AllocSize;
	H.m_Width = GetWidth();
	H.m_Height = GetHeight();
	H.m_Format = GetFormat();
	H.m_Flags = GetFlags();
	H.Write(_pFile);
}

void CImage::Read(CCFile* _pFile, int _MemModel, spCImagePalette _spPal)
{
	MAUTOSTRIP(CImage_Read_pFile_MemModel_spPal, MAUTOSTRIP_VOID);

	CImage_FileHeader H;
	H.Read(_pFile);

	if ((H.m_Width < 0) || (H.m_Height < 0))
		Error("Read", "Invalid image.");

	if (H.m_Compression & IMAGE_MEM_COMPRESSED)
	{
		Destroy();
		if(!(_MemModel & IMAGE_MEM_VIRTUAL))
		{
			m_pBitmap = DNew(uint8) uint8[H.m_AllocSize];
			if (!m_pBitmap) MemError("Read");
		}
		m_AllocSize = H.m_AllocSize;
		m_Width = H.m_Width;
		m_Height = H.m_Height;
		m_Format = H.m_Format;
		m_Flags = H.m_Flags;
		m_Memmodel = H.m_Compression | _MemModel;
		UpdateFormat();

		if(!(_MemModel & IMAGE_MEM_VIRTUAL))
			_pFile->Read(m_pBitmap, m_AllocSize);
	}
	else
	{
		if(!(_MemModel & IMAGE_MEM_VIRTUAL))
			ReadRAW(_pFile, H.m_Width, H.m_Height, H.m_Format, _MemModel, H.m_Flags, _spPal);
		else
			CreateVirtual(H.m_Width, H.m_Height, H.m_Format, _MemModel, _spPal, H.m_Flags);
	}
}

void CImage::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CImage_Write_pFile, MAUTOSTRIP_VOID);

	CImage_FileHeader H;
	H.m_Compression = GetMemModel() & IMAGE_MEM_COMPRESS_ALLFLAGS;
	H.m_Width = GetWidth();
	H.m_Height = GetHeight();
	H.m_Format = GetFormat();
	H.m_Flags = GetFlags();
	H.m_AllocSize = m_AllocSize;
	H.Write(_pFile);

	if (m_Memmodel & IMAGE_MEM_COMPRESSED)
	{
		_pFile->Write(m_pBitmap, m_AllocSize);
	}
	else
		WriteRAW(_pFile);
}


// -------------------------------------------------------------------
//  Geometric operations
// -------------------------------------------------------------------
bool CImage::ClipLine(const CRct& cr, int& _x0, int& _y0, int& _x1, int& _y1)
{
	MAUTOSTRIP(CImage_ClipLine_cr_ix0_iy0_ix1_iy1, false);

	int x0(_x0);
	int y0(_y0);
	int x1(_x1);
	int y1(_y1);

	// Clip X
	if (x0 > x1)
	{
		Swap(x0,x1);
		Swap(y0,y1);
	};
	if (x0 >= cr.p1.x) return false;
	if (x1 < cr.p0.x) return false;

	if (x0 < cr.p0.x) 
	{
		y0 = y0 + ((cr.p0.x - x0)*(y1-y0))/(x1-x0);
		x0 = cr.p0.x;
	};
	if (x1 >= cr.p1.x) 
	{
		y1 = y1 + ((cr.p1.x - x1)*(y1-y0))/(x1-x0);
		x1 = cr.p1.x;
	};
	
	// Clip Y
	if (y0 > y1)
	{
		Swap(x0,x1);
		Swap(y0,y1);
	};
	if (y0 >= cr.p1.y) return false;
	if (y1 < cr.p0.y) return false;

	if (y0 < cr.p0.y) 
	{
		x0 = x0 + ((cr.p0.y - y0)*(x1-x0))/(y1-y0);
		y0 = cr.p0.y;
	};

	if (y1 >= cr.p1.y) 
	{
		x1 = x1 + ((cr.p1.y - y1)*(x1-x0))/(y1-y0);
		y1 = cr.p1.y;
	};

	// Dubbel koll
	if (x0 > x1) 
	{
		Swap(x0,x1);
		Swap(y0,y1);
	};
	if (x0 >= cr.p1.x) return false;
	if (x1 < cr.p0.x) return false;

	_x0 = x0;
	_y0 = y0;
	_x1 = x1;
	_y1 = y1;
//	return ((abs(x1-x0) > 1) || (abs(y1-y0) > 1));
	return true;
};

// -------------------------------------------------------------------
bool CImage::ClipLine(const CRct& cr, fp32& x0, fp32& y0, fp32& x1, fp32& y1)
{
	MAUTOSTRIP(CImage_ClipLine_cr_fx0_fy0_fx1_fy1, false);

	fp32 crx0 = cr.p0.x;
	fp32 cry0 = cr.p0.y;
	fp32 crx1 = cr.p1.x;
	fp32 cry1 = cr.p1.y;

	// Clip X
	if (x0 > x1) 
	{
		Swap(x0,x1);
		Swap(y0,y1);
	};
	if (x0 >= crx1) return false;
	if (x1 < crx0) return false;

	if (x0 < crx0) 
	{
		y0 = y0 + ((crx0 - x0)*(y1-y0))/(x1-x0);
		x0 = crx0;
	};
	if (x1 >= crx1) 
	{
		y1 = y1 + ((crx1 - x1)*(y1-y0))/(x1-x0);
		x1 = crx1;
	};
	
	// Clip Y
	if (y0 > y1)
	{
		Swap(x0,x1);
		Swap(y0,y1);
	};
	if (y0 >= cry1) return false;
	if (y1 < cry0) return false;

	if (y0 < cry0) 
	{
		x0 = x0 + ((cry0 - y0)*(x1-x0))/(y1-y0);
		y0 = cry0;
	};

	if (y1 >= cry1) 
	{
		x1 = x1 + ((cry1 - y1)*(x1-x0))/(y1-y0);
		y1 = cry1;
	};

	// Dubbel koll
	if (x0 > x1) 
	{
		Swap(x0,x1);
		Swap(y0,y1);
	};
	if (x0 >= crx1) return false;
	if (x1 < crx0) return false;

//	return ((abs(x1-x0) > 1) || (abs(y1-y0) > 1));
	return true;
};

// -------------------------------------------------------------------
//  Image drawing operations
// -------------------------------------------------------------------

/*	inline void SetPixel(const CClipRect& cr, CPnt p, int32 color);
	void Fill(const CClipRect& cr, int32 color);
	void Rect(const CClipRect& cr, CRct rect, int32 color);
	void Line(const CClipRect& cr, CPnt p0, CPnt p1, int32 color);
	void Frame(const CClipRect& cr, CRct rect, int32 color);
	void Blt(const CClipRect& cr, CImage& src, int32 _flags, CPnt destp);
*/

int32 CImage::ConvertColor(CPixel32 _Color, int _format, CImagePalette* _pPal)
{
	MAUTOSTRIP(CImage_ConvertColor, 0);

	switch (_format) 
	{
		case IMAGE_FORMAT_BGR5 : return ConvToRGBA5551(_Color);
		case IMAGE_FORMAT_B5G6R5 : return ConvToRGB565(_Color);
		case IMAGE_FORMAT_RGB8 : return _Color.GetR() | (_Color.GetG() << 8) | (_Color.GetB() << 16);
		case IMAGE_FORMAT_RGBA8 : return _Color.GetR() | (_Color.GetG() << 8) | (_Color.GetB() << 16) | (_Color.GetA() << 24);
		case IMAGE_FORMAT_BGR8 : return _Color; 
		case IMAGE_FORMAT_BGRX8 : return _Color;
		case IMAGE_FORMAT_BGRA4 : return ConvToRGBA4444(_Color);
		case IMAGE_FORMAT_BGR5A1 : return ConvToRGBA5551(_Color);
		case IMAGE_FORMAT_BGRA8 : return _Color;
/*		case IMAGE_FORMAT_ZBUFFER8 : return _Color;
		case IMAGE_FORMAT_ZBUFFER16 : return _Color;
		case IMAGE_FORMAT_ZBUFFER24 : return _Color;*/
		case IMAGE_FORMAT_Z24S8 : return _Color;
		case IMAGE_FORMAT_CLUT8 : 
			{
#ifndef PLATFORM_CONSOLE
				if (_pPal == NULL) return 0;
				return _pPal->GetIndex(_Color);
#else
				return 0;
#endif
			};
		case IMAGE_FORMAT_I8 :
			{
				return (_Color.GetR() + 2*_Color.GetG() + _Color.GetB()) >> 2;
			};
		case IMAGE_FORMAT_A8 :
			return _Color.GetA();

		default : return 0xffffffff;
	};
}

CPixel32 CImage::ConvertRAWColor(uint32 _Col, int _format, CImagePalette* _pPal)
{
	MAUTOSTRIP(CImage_ConvertRAWColor, 0);

	switch (_format) 
	{
		case IMAGE_FORMAT_BGR5 : return ConvFromRGBA5551(_Col);
		case IMAGE_FORMAT_B5G6R5 : return ConvFromRGB565(_Col);
		case IMAGE_FORMAT_RGB8 : return CPixel32(_Col & 0xff, (_Col >> 8) & 0xff, (_Col >> 16) & 0xff, 0xff);
		case IMAGE_FORMAT_RGBA8 : return CPixel32(_Col & 0xff, (_Col >> 8) & 0xff, (_Col >> 16) & 0xff, (_Col >> 24) & 0xff);
		case IMAGE_FORMAT_BGR8 : return (_Col & 0xffffff) + 0xff000000; 
		case IMAGE_FORMAT_BGRX8 : return (_Col & 0xffffff) + 0xff000000;
		case IMAGE_FORMAT_BGRA4 : return ConvFromRGBA4444(_Col);
		case IMAGE_FORMAT_BGR5A1 : return ConvFromRGBA5551(_Col);
		case IMAGE_FORMAT_BGRA8 : return _Col;
/*		case IMAGE_FORMAT_ZBUFFER8 : return _Col;
		case IMAGE_FORMAT_ZBUFFER16 : return _Col;
		case IMAGE_FORMAT_ZBUFFER24 : return _Col;*/
		case IMAGE_FORMAT_Z24S8 : return _Col;
		case IMAGE_FORMAT_CLUT8 : 
			{
#ifndef PLATFORM_CONSOLE
				if (_pPal == NULL) return 0;
				return _pPal->GetIndex(_Col);
#else
				return 0;
#endif
			};
		case IMAGE_FORMAT_I8 :
			{
				return CPixel32(_Col, _Col, _Col, 255);
			};

		case IMAGE_FORMAT_A8 :
			return CPixel32(255, 255, 255, _Col);

		default : return 0xffffffff;
	};
}

void CImage::SetRAWData(CPnt pos, int bytecount, uint8* data)
{
	MAUTOSTRIP(CImage_SetRAWData, MAUTOSTRIP_VOID);

	int datapos = 0;
	int pixelcount = bytecount/m_Pixelsize;

	// Clip obvious outsides.
	if (pos.x >= m_Width) return;
	if ((pos.x + pixelcount) < 0) return;
	if ((pos.y < 0) || (pos.y >= m_Height)) return;

	// Clip left 
	if (pos.x < 0) {
		datapos += -pos.x;
		pixelcount += pos.x;
		pos.x = 0;
	};

	// Clip right
	if ((pos.x + pixelcount) > m_Width) pixelcount = m_Width-pos.x;

	// Do the funky stuff...
	uint8* pSurfMem = NULL;
	MLock(this, pSurfMem);
	if (pSurfMem)
	{
		M_TRY 
		{
//			memmove(&(pSurfMem[(pos.x*pixelsize) + (pos.y*modulo)]), &(data[datapos*pixelsize]), pixelcount*pixelsize);
			_Move(&(data[datapos*m_Pixelsize]), &(pSurfMem[(pos.x*m_Pixelsize) + (pos.y*m_Modulo)]), pixelcount*m_Pixelsize);
		}
		M_CATCH(
		catch(...) 
		{
			Error("SetRAWData", "Possible access violation.");
		};
		)
	}
	MUnlock(this);
};

void CImage::GetRAWData(CPnt pos, int bytecount, uint8* data)
{
	MAUTOSTRIP(CImage_GetRAWData, MAUTOSTRIP_VOID);

	int datapos = 0;
	int pixelcount = bytecount/m_Pixelsize;

	// Clip obvious outsides.
	if (pos.x >= m_Width) return;
	if ((pos.x + pixelcount) < 0) return;
	if ((pos.y < 0) || (pos.y >= m_Height)) return;

	// Clip left 
	if (pos.x < 0) {
		datapos += -pos.x;
		pixelcount += pos.x;
		pos.x = 0;
	};

	// Clip right
	if ((pos.x + pixelcount) > m_Width) pixelcount = m_Width-pos.x;

	// Do the funky stuff...
	uint8* pSurfMem = NULL;
	MLock(this, pSurfMem);
	if (pSurfMem)
	{
		M_TRY
		{
//			memmove(&(pSurfMem[(pos.x*pixelsize) + (pos.y*modulo)]), &(data[datapos*pixelsize]), pixelcount*pixelsize);
			_Move(&(pSurfMem[(pos.x*m_Pixelsize) + (pos.y*m_Modulo)]), &(data[datapos*m_Pixelsize]), pixelcount*m_Pixelsize);
		}
		M_CATCH(
		catch(...) 
		{
			Error("GetRAWData", "Possible access violation.");
		};
		)
	}
	MUnlock(this);
};

// -------------------------------------------------------------------
static void CImage_Convert_Alpha8_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_Alpha8_RGBA32, MAUTOSTRIP_VOID);

	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 c;
	uint8 *pDest = (uint8 *)_pDest;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		c = *pSrc++;
		*pDest = 0xff; pDest++;
		*pDest = 0xff; pDest++;
		*pDest = 0xff; pDest++;
		*pDest = c; pDest++;
	}
}

static void CImage_Convert_CLUT8_RGBA5551(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_RGBA5551, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
	
		__asm
		{
			mov esi, _pSrc
			mov edi, _pDest
			mov edx, _pSrcPal
			mov ecx, _nPixels
		Lp8_555:
				xor eax, eax
				mov al, [esi]
				mov eax, [eax*4 + edx]
				mov ebx, 1
				shl eax, 8				// rrrrrrrrggggggggbbbbbbbb00000000
				shld ebx, eax, 5		// 000000000000000000000000001rrrrr
				shl eax, 3+5			// ggggggggbbbbbbbb0000000000000000
				shld ebx, eax, 5		// 0000000000000000000001rrrrrggggg
				shl eax, 3+5			// bbbbbbbb000000000000000000000000
				shld ebx, eax, 5		// 00000000000000001rrrrrgggggbbbbb
				mov [edi], bx
				inc esi
				dec ecx
				lea edi, [edi + 2]
			jnz Lp8_555
		};
	#else
		uint8 *pSrc = (uint8 *)_pSrc;
		uint16 *pDest = (uint16 *)_pDest;
		uint8 r, g, b, a;
		uint32 Col;
		int t;
		for( t = 0 ; t < _nPixels ; t++ )
		{
			Col = _pSrcPal[ *pSrc ];
			r = (Col>>19) & 0x1f;
			g = (Col>>11) & 0x1f;
			b = (Col>>3) & 0x1f;
			a = 0x1;

			*pDest = ((a<<15)|(r<<10)|(g<<5)|(b));
			pSrc++;
			pDest++;
		}

	#endif
}

static void CImage_Convert_CLUT8_RGB565(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_RGB565, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, _pSrc
			mov edi, _pDest
			mov edx, _pSrcPal
			mov ecx, _nPixels
		Lp8_565:
				xor eax, eax
				mov al, [esi]
				mov eax, [eax*4 + edx]
				xor ebx, ebx
				shl eax, 8				// rrrrrrrrggggggggbbbbbbbb00000000
				shld ebx, eax, 5		// 000000000000000000000000000rrrrr
				shl eax, 8				// ggggggggbbbbbbbb0000000000000000
				shld ebx, eax, 6		// 000000000000000000000rrrrrgggggg
				shl eax, 8				// bbbbbbbb000000000000000000000000
				shld ebx, eax, 5		// 0000000000000000rrrrrggggggbbbbb

				mov [edi], bx
				inc esi
				dec ecx
				lea edi, [edi + 2]
			jnz Lp8_565
		}

	#else
		uint8 *pSrc = (uint8 *)_pSrc;
		uint16 *pDest = (uint16 *)_pDest;
		uint8 r, g, b;
		uint32 Col;
		int t;
		for( t = 0 ; t < _nPixels ; t++ )
		{
			Col = _pSrcPal[ *pSrc ];
			r = (Col>>19) & 0x1f;
			g = (Col>>10) & 0x3f;
			b = (Col>>3 ) & 0x1f;

			*pDest = ((r<<11)|(g<<5)|(b));
			pSrc++;
			pDest++;
		}

	#endif
}

static void CImage_Convert_CLUT8_RGB24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_RGB24, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, _pSrc
			mov edi, _pDest
			mov edx, _pSrcPal
			mov ecx, _nPixels
		Lp8_24:
				xor eax, eax
				mov al, [esi]
				mov eax, [eax*4 + edx]
				mov [edi], ax
				shr eax, 16
				mov [edi+2], al
				inc esi
				dec ecx
				lea edi, [edi + 3]
			jnz Lp8_24
		}

	#else
		uint8 *pSrc = (uint8 *)_pSrc;
		uint8 *pDest = (uint8 *)_pDest;
		uint8 r, g, b;
		int t; int32 Col;
		for( t = 0 ; t < _nPixels ; t++ )
		{
			Col = _pSrcPal[ *pSrc ];
			r = (Col>>16);
			g = (Col>>8);
			b = (Col);

			*pDest = r; pDest++;
			*pDest = g; pDest++;
			*pDest = b; pDest++;
			pSrc++;
		}

	#endif
}

static void CImage_Convert_CLUT8_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_RGBA32, MAUTOSTRIP_VOID);

/*	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, _pSrc
			mov edi, _pDest
			mov edx, _pSrcPal
			mov ecx, _nPixels
		Lp8_32:
				xor eax, eax
				mov al, [esi]
				mov eax, [eax*4 + edx]
				or eax, 0xff000000
				mov [edi], eax
				inc esi
				dec ecx
				lea edi, [edi + 4]
				jnz Lp8_32
		}

	#else*/
		uint32 *pDest = (uint32 *)_pDest;
		uint8 *pSrc = (uint8 *)_pSrc;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			*pDest = _pSrcPal[ *pSrc ];
//			*pDest = 0xffffffff;
			pDest++;
			pSrc++;
		}
//	#endif
}

static void CImage_Convert_CLUT8_RGBA4444(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_RGBA4444, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, _pSrc;
			mov edi, _pDest;
			mov edx, _pSrcPal;
			mov ecx, _nPixels
		Lp8_4444:
				xor ebx, ebx
				mov bl, [esi]
				mov eax, [ebx*4 + edx]
				mov ebx, 0x0f
				shl eax, 8				// rrrrrrrrggggggggbbbbbbbb00000000
				shld ebx, eax, 4		// 0000000000000000000000001111rrrr
				shl eax, 8				// ggggggggbbbbbbbb0000000000000000
				shld ebx, eax, 4		// 000000000000000000001111rrrrgggg
				shl eax, 8				// bbbbbbbb000000000000000000000000
				shld ebx, eax, 4		// 00000000000000001111rrrrggggbbbb
				mov [edi], bx
				inc esi
				dec ecx
				lea edi, [edi + 2]
			jnz Lp8_4444
		}

	#else
		uint8 *pSrc = (uint8 *)_pSrc;
		uint16 *pDest = (uint16 *)_pDest;
		int32 Col;
		uint8 r, g, b, a;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			Col = _pSrcPal[ *pSrc ];
			r = (Col>>20) & 0xf;
			g = (Col>>12) & 0xf;
			b = (Col>>4 ) & 0xf;
			a = 0xf;

/*			r = 0xf;
			g = 0xf;
			b = 0xf;
			a = 0xf;*/

			*pDest = ((a<<12)|(r<<8)|(g<<4)|(b));
			pSrc++;
			pDest++;
		}

	#endif
}

static void CImage_Convert_CLUT8_ABGR32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_ABGR32, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, _pSrc
			mov edi, _pDest
			mov edx, _pSrcPal
			mov ecx, _nPixels
		Lp8_ABGR32:
				xor eax, eax
				mov al, [esi]
				mov eax, [eax*4 + edx]
				bswap eax
				ror eax, 8
				or eax, 0xff000000
				mov [edi], eax
				inc esi
				dec ecx
				lea edi, [edi + 4]
			jnz Lp8_ABGR32
		};
	#else
		uint8 *pSrc = (uint8 *)_pSrc;
		uint32 *pDest = (uint32 *)_pDest;
		uint32 Col;
		uint8 r, g, b, a;
//		uint8 Alpha;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			Col = _pSrcPal[ *pSrc ];
			r = (Col>>16);
			g = (Col>>8);
			b = (Col);
			a = 0xff;

			*pDest = ((a)|(b<<24)|(g<<16)|(r<<8));

			pSrc++;
			pDest++;
		}

	#endif
}

static void CImage_Convert_CLUT8_BGR24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_BGR24, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, _pSrc
			mov edi, _pDest
			mov edx, _pSrcPal
			mov ecx, _nPixels
		Lp8_24:
				xor eax, eax
				mov al, [esi]
				mov eax, [eax*4 + edx]
				mov [edi+2], al
				bswap eax
				shr eax, 8
				mov [edi], ax
				inc esi
				dec ecx
				lea edi, [edi + 3]
			jnz Lp8_24
		}
	#else
		uint8 *pSrc = (uint8 *)_pSrc;
		uint8 *pDest = (uint8 *)_pDest;
		uint8 r, g, b;
		uint32 Col;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			Col = _pSrcPal[ *pSrc ];
			r = (Col>>16);
			g = (Col>>8);
			b = (Col);

//			r=0xff; g=0xff; b=0xff;// a = 0xff;

			*pDest = b; pDest++;
			*pDest = g; pDest++;
			*pDest = r; pDest++;
			pSrc++;
		}
	#endif
}

// -------------------------------------------------------------------
static void CImage_Convert_RGB565_RGBA5551(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB565_RGBA5551, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, [_pSrc]
			mov edi, [_pDest]
			mov ecx, _nPixels
		Lp565_5551:
				mov ax, [esi]

				mov ebx, 1
				shl eax, 16				// rrrrrggggggbbbbb0000000000000000
				shld ebx, eax, 5		// 000000000000000000000000001rrrrr
				shl eax, 5				// ggggggbbbbb000000000000000000000
				shld ebx, eax, 5		// 0000000000000000000001rrrrrggggg
				shl eax, 6				// bbbbb000000000000000000000000000
				shld ebx, eax, 5		// 00000000000000001rrrrrgggggbbbbb

				lea esi, [esi+2]
				mov [edi], bx
				dec ecx
				lea edi, [edi+2]
			jnz Lp565_5551
		}
	#else
		uint16 *pSrc = (uint16 *)_pSrc;
		uint16 *pDst = (uint16 *)_pDest;
		uint16 Col;
		uint16 NewCol;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			Col = *pSrc;
			NewCol = 0x8000;
			NewCol |= (Col & 0x1f);
			NewCol |= ((Col >> 6) & 0x1f) << 5;
			NewCol |= ((Col >> 11) & 0x1f) << 10;
			*pDst = NewCol;

			pDst++;
			pSrc++;
		}
	#endif
}

static void CImage_Convert_RGB565_RGB24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB565_RGB24, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, [_pSrc]
			mov edi, [_pDest]
			mov ecx, _nPixels
		Lp565_24:
				mov ax, [esi]

				xor ebx, ebx
				shl eax, 16				// rrrrrggggggbbbbb0000000000000000
				shld ebx, eax, 8		// 000000000000000000000000rrrrrxxx
				shl eax, 5				// ggggggbbbbb000000000000000000000
				shld ebx, eax, 8		// 0000000000000000rrrrrxxxggggggxx
				shl eax, 6				// bbbbb000000000000000000000000000
				shld ebx, eax, 8		// 00000000rrrrrxxxggggggxxbbbbbxxx
				and ebx, 0x00f8fcf8		// 00000000rrrrr000gggggg00bbbbb000

				lea esi, [esi+2]
				mov [edi], bx
				shr ebx, 16
				mov [edi+2], bl
				dec ecx
				lea edi, [edi+3]
			jnz Lp565_24
		};
	#else
		uint16 *pSrc = (uint16 *)_pSrc;
		uint8 *pDest = (uint8 *)_pDest;
		uint8 r, g, b;
		uint16 Col;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			Col = *pSrc; pSrc++;

			r = (Col>>11) & 0x1f;
			g = (Col>>5) & 0x3f;
			b = (Col) & 0x1f;
			r = (r<<3)|(r&3);
			g = (g<<2)|(g&2);
			b = (b<<3)|(b&3);

			*pDest = r; pDest++;
			*pDest = g; pDest++;
			*pDest = b; pDest++;
		}
	#endif
}

static void CImage_Convert_RGB565_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB565_RGBA32, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, [_pSrc]
			mov edi, [_pDest]
			mov ecx, _nPixels
		Lp565_32:
				mov ax, [esi]

				mov ebx, 0xff
				shl eax, 16				// rrrrrggggggbbbbb0000000000000000
				shld ebx, eax, 8		// 000000000000000011111111rrrrrxxx
				shl eax, 5				// ggggggbbbbb000000000000000000000
				shld ebx, eax, 8		// 0000000011111111rrrrrxxxggggggxx
				shl eax, 6				// bbbbb000000000000000000000000000
				shld ebx, eax, 8		// 11111111rrrrrxxxggggggxxbbbbbxxx
				and ebx, 0xfff8fcf8		// 11111111rrrrr000gggggg00bbbbb000

				lea esi, [esi+2]
				mov [edi], ebx
				dec ecx
				lea edi, [edi+4]
			jnz Lp565_32
		}
	#else
		uint16 *pSrc = (uint16 *)_pSrc;
		uint16 Col;
		uint8 *pDest = (uint8 *)_pDest;
		uint8 r, g, b, a;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			Col = *pSrc; pSrc++;
			::SwapLE(Col);

			r = (Col>>11) & 0x1f;
			g = (Col>>5) & 0x3f;
			b = (Col) & 0x1f;
			r = (r<<3)|(r&3);
			g = (g<<2)|(g&2);
			b = (b<<3)|(b&3);
			a = 0xff;

			*pDest = r;	pDest++;
			*pDest = g;	pDest++;
			*pDest = b;	pDest++;
			*pDest = a;	pDest++;
		}
	#endif
}

static void CImage_Convert_RGBA5551_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGBA5551_RGBA32, MAUTOSTRIP_VOID);

	uint16 *pSrc = (uint16 *)_pSrc;
	uint16 Col;
//	uint8 *pDest = (uint8 *)_pDest;
//	uint8 r, g, b, a;
	uint32* pDest =(uint32*)_pDest;
	uint32 FCol, r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		Col = *pSrc; pSrc++;
		::SwapLE(Col);

		r = (Col>>10) & 0x1f;
		g = (Col>>5) & 0x1f;
		b = (Col) & 0x1f;
		a = ((Col>>15) & 0x1)*255;
		r = (r<<3)|(r&3);
		g = (g<<3)|(g&3);
		b = (b<<3)|(b&3);

		FCol = b | (g << 8) | (r << 16) | (a << 24);
		::SwapLE(FCol);
		*pDest++ = FCol;

//		*pDest = b;	pDest++;
//		*pDest = g;	pDest++;
//		*pDest = r;	pDest++;
//		*pDest = a;	pDest++;
	}
}

// -------------------------------------------------------------------
static void CImage_Convert_RGB24_RGBA5551(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_RGBA5551, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, [_pSrc]
			mov edi, [_pDest]
			mov ecx, _nPixels
		Lp24_555:
				mov al, [esi+2]
				xor ah, ah
				shl eax, 16
				mov ax, [esi]

				mov ebx, 1
				shl eax, 8				// rrrrrrrrggggggggbbbbbbbb00000000
				shld ebx, eax, 5		// 000000000000000000000000001rrrrr
				shl eax, 3+5			// ggggggggbbbbbbbb0000000000000000
				shld ebx, eax, 5		// 0000000000000000000001rrrrrggggg
				shl eax, 3+5			// bbbbbbbb000000000000000000000000
				shld ebx, eax, 5		// 00000000000000001rrrrrgggggbbbbb

				lea esi, [esi+3]
				mov [edi], bx
				dec ecx
				lea edi, [edi+2]
			jnz Lp24_555
		}
	#else
		uint16 *pDest = (uint16 *)_pDest;
		uint8 *pSrc = (uint8 *)_pSrc;
		uint8 r, g, b, a;
		for( int t = 0 ; t < _nPixels; t++ )
		{
			b = (*pSrc>>3)&0x1f; pSrc++;
			g = (*pSrc>>3)&0x1f; pSrc++;
			r = (*pSrc>>3)&0x1f; pSrc++;
			a = 0x1;

			*pDest = ((a<<15)|(r<<10)|(g<<5)|(b)); pDest++;
		}
	#endif
}

static void CImage_Convert_RGB24_RGB565(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_RGB565, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, [_pSrc]
			mov edi, [_pDest]
			mov ecx, _nPixels
		Lp24_565:
				mov al, [esi+2]
				xor ah, ah
				shl eax, 16
				mov ax, [esi]

				shl eax, 8			// rrrrrrrrggggggggbbbbbbbb00000000
				shld ebx, eax, 5		// 000000000000000000000000000rrrrr
				shl eax, 8			// ggggggggbbbbbbbb0000000000000000
				shld ebx, eax, 6		// 000000000000000000000rrrrrgggggg
				shl eax, 8			// bbbbbbbb000000000000000000000000
				shld ebx, eax, 5		// 0000000000000000rrrrrggggggbbbbb
			
				lea esi, [esi+3]
				mov [edi], bx
				dec ecx
				lea edi, [edi+2]
			jnz Lp24_565
		}
	#else
		uint8 *pSrc = (uint8 *)_pSrc;
		uint8 r, g, b;
		uint16 *pDest = (uint16 *)_pDest;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			b = (*pSrc>>3)&0x1f; pSrc++;
			g = (*pSrc>>2)&0x3f; pSrc++;
			r = (*pSrc>>3)&0x1f; pSrc++;

			*pDest = ((r<<11) | (g<<5) | (b)); pDest++;
		}
	#endif
}

static void CImage_Convert_RGB24_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_RGBA32, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, [_pSrc]
			mov edi, [_pDest]
			mov ecx, _nPixels
		Lp24_32:
				mov al, [esi+2]
				mov ah, 0xff
				shl eax, 16
				mov ax, [esi]
				lea esi, [esi+3]
				mov [edi], eax
				dec ecx
				lea edi, [edi+4]
			jnz Lp24_32
		}
	#else
		uint8 *pSrc = (uint8 *)_pSrc;
		uint8 *pDest = (uint8 *)_pDest;

		for( int t = 0 ; t < _nPixels ;t++ )
		{
/*
			Col = *pSrc;
			NewCol = Col;
			pSrc++;

			Col = *pSrc;
			NewCol |= Col << 8;
			pSrc++;
			
			Col = *pSrc;
			NewCol |= Col << 16;
			pSrc++;
			
			*pDest = NewCol | 0xff000000;
			pDest++;*/

			*pDest++ = *pSrc++;
			*pDest++ = *pSrc++;
			*pDest++ = *pSrc++;
			*pDest++ = 0xff;
		}
	#endif
}

static void CImage_Convert_RGB24_ABGR32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_ABGR32, MAUTOSTRIP_VOID);

	#ifdef CPU_INTELP5
		__asm
		{
			mov esi, [_pSrc]
			mov edi, [_pDest]
			mov ecx, _nPixels
		Lp24_ABGR32:
				mov al, [esi+2]
				shl eax, 16
				mov ax, [esi]
				bswap eax
				shr eax, 8
				or eax, 0xff000000
				lea esi, [esi+3]
				mov [edi], eax
				dec ecx
				lea edi, [edi+4]
			jnz Lp24_ABGR32
		}
	#else
		uint8 *pDst = (uint8 *)_pDest;
		uint8 *pSrc = (uint8 *)_pSrc;
		uint8 r, g, b;
		for( int t = 0 ; t < _nPixels ; t++ )
		{
			r = *pSrc++;
			g = *pSrc++;
			b = *pSrc++;
			*pDst++ = 0xff;
			*pDst++ = b;
			*pDst++ = g;
			*pDst++ = r;
		}
	#endif
}

static void CImage_Convert_RGB24_BGR24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_BGR24, MAUTOSTRIP_VOID);

	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		r = *pSrc++;
		g = *pSrc++;
		b = *pSrc++;
		*pDst++ = b;
		*pDst++ = g;
		*pDst++ = r;
	}
}

static void CImage_Convert_RGB24_GREY8(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_GREY8, MAUTOSTRIP_VOID);

	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		int r = *pSrc++;
		int g = *pSrc++;
		int b = *pSrc++;
		*pDst++ = (r + g*2 + b) >> 2;
	}
}


// -------------------------------------------------------------------
static void CImage_Convert_RGB32_RGBA5551(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGBA5551, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		mov ecx, _nPixels
	Lp32_555:
			mov eax, [esi]
			
			mov ebx, 1
			shl eax, 8				// rrrrrrrrggggggggbbbbbbbb00000000
			shld ebx, eax, 5		// 000000000000000000000000001rrrrr
			shl eax, 3+5			// ggggggggbbbbbbbb0000000000000000
			shld ebx, eax, 5		// 0000000000000000000001rrrrrggggg
			shl eax, 3+5			// bbbbbbbb000000000000000000000000
			shld ebx, eax, 5		// 00000000000000001rrrrrgggggbbbbb
	
			lea esi, [esi+4]
			mov [edi], bx
			dec ecx
			lea edi, [edi+2]
		jnz Lp32_555
	}
#else
	uint8 *pSrc = (uint8 *)_pSrc;
	uint16 *pDest = (uint16 *)_pDest;
	uint8 r, g, b;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		r = (*pSrc>>3)&0x1f; pSrc++;
		g = (*pSrc>>3)&0x1f; pSrc++;
		b = (*pSrc>>3)&0x1f; pSrc++;
		*pDest = (0x8000|(r<<10)|(g<<5)|(b)); pDest++;
	}
#endif
}

static void CImage_Convert_RGB32_RGB565(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGB565, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		mov ecx, _nPixels
	Lp32_565:
			mov eax, [esi]

			shl eax, 8				// rrrrrrrrggggggggbbbbbbbb00000000
			shld ebx, eax, 5		// 000000000000000000000000000rrrrr
			shl eax, 8				// ggggggggbbbbbbbb0000000000000000
			shld ebx, eax, 6		// 000000000000000000000rrrrrgggggg
			shl eax, 8				// bbbbbbbb000000000000000000000000
			shld ebx, eax, 5		// 0000000000000000rrrrrggggggbbbbb
			
			lea esi, [esi+4]
			mov [edi], bx
			dec ecx
			lea edi, [edi+2]
		jnz Lp32_565
	}
#else
	uint16 *pDest = (uint16 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		pSrc++;
		r = (*pSrc>>3)&0x1f; pSrc++;
		g = (*pSrc>>2)&0x3f; pSrc++;
		b = (*pSrc>>3)&0x1f; pSrc++;

		*pDest = ((r<<11)|(g<<5)|(b)); pDest++;
	}
#endif
}

static void CImage_Convert_RGB32_RGB24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGB24, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		mov ecx, _nPixels
	Lp32_24:
			mov eax, [esi]
			lea esi, [esi+4]
			mov [edi], ax
			shr eax, 16
			mov [edi+2], al
			dec ecx
			lea edi, [edi+3]
		jnz Lp32_24
	};
#else
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 *pDst = (uint8 *)_pDest;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		pSrc++;
		*pDst++ = *pSrc++;
		*pDst++ = *pSrc++;
		*pDst++ = *pSrc++;
	}
#endif
}

static void CImage_Convert_RGB32_BGR24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_BGR24, MAUTOSTRIP_VOID);

	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 *pDst = (uint8 *)_pDest;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		*pDst++ = pSrc[2];
		*pDst++ = pSrc[1];
		*pDst++ = pSrc[0];
		pSrc += 4;
	}
}

static void CImage_Convert_RGB32_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGBA32, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		xor ebx, ebx
		mov ecx, _nPixels
	Lp32_RGBA32:
			mov eax, [esi+ebx]
			or eax, 0xff000000
			mov [edi+ebx], eax
			dec ecx
			lea ebx, [ebx+4]
		jnz Lp32_RGBA32
	}
#else
	uint32 *pSrc = (uint32 *)_pSrc;
	uint32 *pDest = (uint32 *)_pDest;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		*pDest = *pSrc | 0xff000000;
		pDest++;
		pSrc++;
	}
#endif
}

static void CImage_Convert_RGB32_RGBA4444(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGBA4444, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, _pSrc;
		mov edi, _pDest;
		mov ecx, _nPixels
	Lp32_4444:
			mov ebx, 0xf
			mov eax, [esi]

			shl eax, 8			// rrrrrrrrggggggggbbbbbbbb00000000
			shld ebx, eax, 4		// 0000000000000000000000001111rrrr
			shl eax, 8			// ggggggggbbbbbbbb0000000000000000
			shld ebx, eax, 4		// 000000000000000000001111rrrrgggg
			shl eax, 8			// bbbbbbbb000000000000000000000000
			shld ebx, eax, 4		// 00000000000000001111rrrrggggbbbb
			mov [edi], bx

			lea esi, [esi + 4]
			dec ecx
			lea edi, [edi + 2]
		jnz Lp32_4444
	}
#else
	uint16 *pDest = (uint16 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		pSrc++;
		r = (*pSrc>>4)&0x0f; pSrc++;
		g = (*pSrc>>4)&0x0f; pSrc++;
		b = (*pSrc>>4)&0x0f; pSrc++;
		a = 0xf;
		*pDest = ((a<<12)|(r<<8)|(g<<4)|(b));

		pDest++;
	}
#endif
}


static void CImage_Convert_RGB32_ABGR32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_ABGR32, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		xor ebx, ebx
		mov ecx, _nPixels
	Lp32_ABGR32:
			mov eax, [esi+ebx]
			bswap eax
			shr eax, 8
			or eax, 0xff000000
			mov [edi+ebx], eax
			dec ecx
			lea ebx, [ebx+4]
		jnz Lp32_ABGR32
	}
#else
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 *pDst = (uint8 *)_pDest;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		a = 0xff; pSrc++;
		r = *pSrc; pSrc++;
		g = *pSrc; pSrc++;
		b = *pSrc; pSrc++;
		*pDst = a; pDst++;
		*pDst = b; pDst++;
		*pDst = g; pDst++;
		*pDst = r; pDst++;
	}
#endif
}

static void CImage_Convert_RGB32_GREY8(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_GREY8, MAUTOSTRIP_VOID);

	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		int b = *pSrc++;
		int g = *pSrc++;
		int r = *pSrc++;
		pSrc++;
		*pDst++ = (r + g*2 + b) >> 2;
	}
}

// -------------------------------------------------------------------
static void CImage_Convert_RGBA32_RGBA5551(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGBA32_RGBA5551, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, _pSrc;
		mov edi, _pDest;
		mov ecx, _nPixels
	Lp32_5551:
			xor ebx, ebx
			mov eax, [esi]

			shld ebx, eax, 1		// 0000000000000000000000000000000a
			shl eax, 8			// rrrrrrrrggggggggbbbbbbbb00000000
			shld ebx, eax, 5		// 00000000000000000000000000arrrrr
			shl eax, 8			// ggggggggbbbbbbbb0000000000000000
			shld ebx, eax, 5		// 000000000000000000000arrrrrggggg
			shl eax, 8			// bbbbbbbb000000000000000000000000
			shld ebx, eax, 5		// 0000000000000000arrrrrgggggbbbbb
			mov [edi], bx

			lea esi, [esi + 4]
			dec ecx
			lea edi, [edi + 2]
		jnz Lp32_5551
	}
#else
	uint16 *pDst = (uint16 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		a = (*pSrc>>7)&0x01; pSrc++;
		r = (*pSrc>>3)&0x1f; pSrc++;
		g = (*pSrc>>3)&0x1f; pSrc++;
		b = (*pSrc>>3)&0x1f; pSrc++;
		*pDst = ((a<<15)|(r<<10)|(g<<5)|(b)); pDst++;
	}
#endif
}

static void CImage_Convert_RGBA32_RGBA4444(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGBA32_RGBA4444, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, _pSrc;
		mov edi, _pDest;
		mov ecx, _nPixels
	Lp32_4444:
			xor ebx, ebx
			mov eax, [esi]

			shld ebx, eax, 4		// 0000000000000000000000000000aaaa
			shl eax, 8				// rrrrrrrrggggggggbbbbbbbb00000000
			shld ebx, eax, 4		// 000000000000000000000000aaaarrrr
			shl eax, 8				// ggggggggbbbbbbbb0000000000000000
			shld ebx, eax, 4		// 00000000000000000000aaaarrrrgggg
			shl eax, 8				// bbbbbbbb000000000000000000000000
			shld ebx, eax, 4		// 0000000000000000aaaarrrrggggbbbb
			mov [edi], bx

			lea esi, [esi + 4]
			dec ecx
			lea edi, [edi + 2]
		jnz Lp32_4444
	}
#else
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	uint16 *pDest = (uint16 *)_pDest;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		b = (*pSrc>>4)&0x0f; pSrc++;
		g = (*pSrc>>4)&0x0f; pSrc++;
		r = (*pSrc>>4)&0x0f; pSrc++;
		a = (*pSrc>>4)&0x0f; pSrc++;
		*pDest = ((a<<12)|(r<<8)|(g<<4)|(b)); pDest++;
	}
#endif
}

static void CImage_Convert_RGBA32_ABGR32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGBA32_ABGR32, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		xor ebx, ebx
		mov ecx, _nPixels
	Lp32_ABGR32:
			mov eax, [esi+ebx]
			mov edx, eax
			bswap eax
			shr eax, 8
			and edx, 0ff000000h
			or eax, edx
//			bswap eax
			mov [edi+ebx], eax
			dec ecx
			lea ebx, [ebx+4]
		jnz Lp32_ABGR32
	}
#else
	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		a = *pSrc++;
		r = *pSrc++;
		g = *pSrc++;
		b = *pSrc++;
		*pDst = a; pDst++;
		*pDst = b; pDst++;
		*pDst = g; pDst++;
		*pDst = r; pDst++;
	}
#endif
}

static void CImage_Convert_RGBA32_Alpha8(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGBA32_Alpha8, MAUTOSTRIP_VOID);

	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		b = *pSrc++;
		g = *pSrc++;
		r = *pSrc++;
		a = *pSrc++;
		*pDst = a; pDst++;
	}
}

static void CImage_Convert_BGR10A2_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	uint32 *pSrc = (uint32 *)_pSrc;
	uint32 Col;
	uint8 *pDest = (uint8 *)_pDest;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		Col = *pSrc; pSrc++;
		SwapLE(Col);

		a = ((Col>>30) & 0x3);
		a = a + (a << 2) + (a << 4) + (a << 6);
		r = ((Col>>20) & 0x3ff) >> 2;
		g = ((Col>>10) & 0x3ff) >> 2;
		b = ((Col>>0) & 0x3ff) >> 2;

		*pDest = b;	pDest++;
		*pDest = g;	pDest++;
		*pDest = r;	pDest++;
		*pDest = a;	pDest++;
	}
}

static void CImage_Convert_BGR10A2_RGB24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	uint32 *pSrc = (uint32 *)_pSrc;
	uint32 Col;
	uint8 *pDest = (uint8 *)_pDest;
	uint8 r, g, b;
	r = 0;
	g = 0;
	b = 0;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		Col = *pSrc; pSrc++;
		SwapLE(Col);

		r = ((Col>>20) & 0x3ff) >> 2;
		g = ((Col>>10) & 0x3ff) >> 2;
		b = ((Col>>0) & 0x3ff) >> 2;

		*pDest = b;	pDest++;
		*pDest = g;	pDest++;
		*pDest = r;	pDest++;
	}
}

static void CImage_Convert_RGBA4444_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	uint16 *pSrc = (uint16 *)_pSrc;
	uint16 Col;
	uint8 *pDest = (uint8 *)_pDest;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		Col = *pSrc; pSrc++;

		a = (Col>>12) & 0xf;
		r = (Col>>8) & 0xf;
		g = (Col>>4) & 0xf;
		b = (Col) & 0xf;

		r = (r << 4) | (r & 0xf);
		g = (g << 4) | (g & 0xf);
		b = (b << 4) | (b & 0xf);
		a = (a << 4) | (a & 0xf);

		*pDest = b;	pDest++;
		*pDest = g;	pDest++;
		*pDest = r;	pDest++;
		*pDest = a;	pDest++;
	}
}

// -------------------------------------------------------------------
static void CImage_Convert_GREY8_RGB565(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_GREY8_RGB565, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, _pSrc
		mov edi, _pDest
		mov ecx, _nPixels
	Lp:
			xor eax, eax
			mov al, [esi]
			mov ebx, eax
			mov ah, al
			shr al, 3
			shr ebx, 2
			and ah, 0f8h
			shl ebx, 5
			or eax, ebx

			inc esi
			mov [edi], ax
			dec ecx
			lea edi, [edi + 2]
		jnz Lp
	}
#else
	uint16 *pDest = (uint16 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		r=g=b = *pSrc; pSrc++;
		r >>= 3;
		g >>= 2;
		b >>= 3;

		*pDest = ((r<<11)|(g<<5)|(b)); pDest++;
	}
#endif
}

static void CImage_Convert_GREY8_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_GREY8_RGBA32, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, _pSrc
		mov edi, _pDest
		mov ecx, _nPixels
	Lp:
			xor eax, eax
			mov al, [esi]
			mov ah, al
			mov ebx, eax
			rol eax, 16
			or eax, ebx
			or eax, 0xff000000

			inc esi
			mov [edi], eax
			dec ecx
			lea edi, [edi + 4]
		jnz Lp
	}
#else
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 c;
	uint8 *pDest = (uint8 *)_pDest;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		c = *pSrc++;
		*pDest = 0xff; pDest++;
		*pDest = c; pDest++;
		*pDest = c; pDest++;
		*pDest = c; pDest++;
	}
#endif
}

static void CImage_Convert_GREY8_RGB24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_GREY8_RGB24, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, _pSrc
		mov edi, _pDest
		mov ecx, _nPixels
	Lp:
			xor eax, eax
			mov al, [esi]
			mov [edi], al
			mov [edi+1], al
			inc esi
			mov [edi+2], al
			dec ecx
			lea edi, [edi + 3]
		jnz Lp
	}
#else
	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 c;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		c = *pSrc++;
		*pDst = c; pDst++;
		*pDst = c; pDst++;
		*pDst = c; pDst++;
	}
#endif
}

// -------------------------------------------------------------------
static void CImage_Convert_RGB32_RGBA4444_InverseAlpha(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGBA4444_InverseAlpha, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, _pSrc;
		mov edi, _pDest;
		mov ecx, _nPixels
	Lp32_4444:
			xor ebx, ebx
			mov eax, [esi]

			add ebx, eax
			shl eax, 8
			and eax, 0x00ffffff
			add ebx, eax
			add ebx, eax
			shl eax, 8
			and eax, 0x00ffffff
			add ebx, eax
			shr ebx, 16+4+2 - 12
			and ebx, 0xf000
			xor ebx, 0xf000

			lea esi, [esi + 4]
			mov [edi], bx
			dec ecx
			lea edi, [edi + 2]
		jnz Lp32_4444
	}
#else
	uint16 *pDst = (uint16 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		pSrc++;
		r = *pSrc>>4; pSrc++;
		g = *pSrc>>4; pSrc++;
		b = *pSrc>>4; pSrc++;
		a = Min(255, ((r+2*g+b) >> 2));
		a ^= 0xf;
		*pDst = ((a<<12)|(r<<8)|(g<<4)|(b)); pDst++;
	}
#endif
}

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable: 4731)
#endif

static void CImage_Convert_CLUT8_ABGR32_InverseAlpha(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_ABGR32_InverseAlpha, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov ebx, [_pSrcPal]
		mov ecx, _nPixels
		push ebp
		mov ebp, [_pDest]
	Lp32_RGBA32:
			xor eax, eax
			mov al, [esi]
			inc esi
			mov eax, [ebx + eax*4]
			mov edx, eax
			mov edi, eax
			and edx, 0x0000ff
			and edi, 0x00ff00
			and eax, 0xff0000
			shr edi, 8
			shr eax, 16
			add edx, edi
			add edx, edi
			add edx, eax
			shr edx, 2
			shl edx, 24
			xor edx, 0xff000000
			dec ecx
			mov [ebp], edx
			lea ebp, [ebp+4]
		jnz Lp32_RGBA32
		pop ebp
	}
#else
	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint32 Col;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		Col = _pSrcPal[ *pSrc ] & 0xffffff; pSrc++;
		r = (Col>>16);
		g = (Col>>8);
		b = (Col);
		a = Min(255, ((r+2*g+b) >> 2));
		a ^= 0xff;
		*pDst = a; pDst++;
		*pDst = b; pDst++;
		*pDst = g; pDst++;
		*pDst = r; pDst++;
	}
#endif
}

static void CImage_Convert_RGB32_ABGR32_InverseAlpha(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_ABGR32_InverseAlpha, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		xor ebx, ebx
		add ebx, edi
		sub esi, edi
		mov ecx, _nPixels
	Lp32_RGBA32:
			mov eax, [esi+ebx]
			mov edx, eax
			mov edi, eax
			and edx, 0x0000ff
			and edi, 0x00ff00
			and eax, 0xff0000
			shr edi, 8
			shr eax, 16
			add edx, edi
			add edx, edi
			add edx, eax
			shr edx, 2
			shl edx, 24
			xor edx, 0xff000000
			dec ecx
			mov [ebx], edx
			lea ebx, [ebx+4]
		jnz Lp32_RGBA32
	}
#else
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 *pDst = (uint8 *)_pDest;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		pSrc++;
		r = *pSrc++;
		g = *pSrc++;
		b = *pSrc++;
		a = Min(255, ((r+2*g+b) >> 2));
		a ^= 0xff;
		*pDst = a; pDst++;
		*pDst = b; pDst++;
		*pDst = g; pDst++;
		*pDst = r; pDst++;
	}
#endif
}

// -------------------------------------------------------------------
static void CImage_Convert_CLUT8_RGBA32_AFromRGB(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_CLUT8_RGBA32_AFromRGB, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		mov ebx, [_pSrcPal]
		push ebp
		mov ebp, _nPixels
	Lp:
		    xor eax, eax
		    xor ecx, ecx
			mov al, [esi]
			mov eax, [eax*4 + ebx]

			mov cl, ah
			mov edx, eax
			xor ah,ah
			add cx, ax
			add cx, ax
			shr eax, 16
			and eax, 0xff;
			and edx, 0x00ffffff
			add ecx, eax
			shr ecx, 2
			shl ecx, 24
			or edx, ecx
			inc esi
			mov [edi], edx
			dec ebp
			lea edi, [edi+4]
		jnz Lp
		pop ebp
	}
#else
	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	uint32 Col;
	for( int t = 0 ; t  < _nPixels ; t++ )
	{
		Col = _pSrcPal[ *pSrc ]; pSrc++;
		r = (Col>>16);
		g = (Col>>8);
		b = (Col);
		a = Min(255, ((r+2*g+b) >> 2));
		*pDst = ((a<<24)|(r<<16)|(g<<8)|(b)); pDst++;
	}
#endif
}

static void CImage_Convert_RGB24_RGBA32_AFromRGB(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_RGBA32_AFromRGB, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		xor ebx, ebx
		push ebp
		mov ebp, _nPixels
	Lp24_RGBA32:
		    xor ecx, ecx
			mov eax, [esi]
			mov cl, ah
			mov edx, eax
			xor ah,ah
			add cx, ax
			add cx, ax
			shr eax, 16
			and eax, 0xff;
			and edx, 0x00ffffff
			add ecx, eax
			shr ecx, 2
			shl ecx, 24
			or edx, ecx
			mov [edi+ebx], edx
			add esi, 3
			dec ebp
			lea ebx, [ebx+4]
		jnz Lp24_RGBA32
		pop ebp
	}
#else
	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		r = *pSrc++;
		g = *pSrc++;
		b = *pSrc++;
		a = Min(255, ((r+2*g+b) >> 2));

		*pDst = a; pDst++;
		*pDst = r; pDst++;
		*pDst = g; pDst++;
		*pDst = b; pDst++;
	}
#endif
}

static void CImage_Convert_GREY8_RGBA32_AFromRGB(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 i;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		i = *pSrc++;
		*pDst = i; pDst++;
		*pDst = i; pDst++;
		*pDst = i; pDst++;
		*pDst = i; pDst++;
	}
}

static void CImage_Convert_RGB32_RGBA32_AFromRGB(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGBA32_AFromRGB, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		xor ebx, ebx
		push ebp
		mov ebp, _nPixels
	Lp32_RGBA32:
		    xor ecx, ecx
			mov eax, [esi+ebx]
			mov cl, ah
			mov edx, eax
			xor ah,ah
			add cx, ax
			add cx, ax
			shr eax, 16
			and eax, 0xff;
			and edx, 0x00ffffff
			add ecx, eax
			shr ecx, 2
			shl ecx, 24
			or edx, ecx
			mov [edi+ebx], edx
			dec ebp
			lea ebx, [ebx+4]
		jnz Lp32_RGBA32
		pop ebp
	}
#else
	uint8 *pDst = (uint8 *)_pDest;
	uint8 *pSrc = (uint8 *)_pSrc;
	uint8 r, g, b, a;
	for( int t = 0 ; t < _nPixels ; t++ )
	{
		pSrc++;
		r = *pSrc++;
		g = *pSrc++;
		b = *pSrc++;
		a = Min(255, ((r+2*g+b) >> 2));

		*pDst = a; pDst++;
		*pDst = r; pDst++;
		*pDst = g; pDst++;
		*pDst = b; pDst++;
	}
#endif
}

static void CImage_Convert_RGBFLOAT_RGBA32(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGBFLOAT_RGBA32, MAUTOSTRIP_VOID);

	CVec3Dfp32* pSrc = (CVec3Dfp32*)_pSrc;
	uint32 *pDest = (uint32 *)_pDest;
	for(int t = 0 ; t < _nPixels ; t++ )
	{
		*pDest = CPixel32(
			RoundToInt(Min(pSrc->k[0] * 255.0f, 255.0f)),
			RoundToInt(Min(pSrc->k[1] * 255.0f, 255.0f)),
			RoundToInt(Min(pSrc->k[2] * 255.0f, 255.0f)));
		pSrc++;
		pDest++;
	}
}

static void CImage_Convert_RGB32_RGBFLOAT(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGBFLOAT, MAUTOSTRIP_VOID);

	uint32 *pSrc = (uint32 *)_pSrc;
	CVec3Dfp32* pDest = (CVec3Dfp32*)_pDest;
	for(int t = 0 ; t < _nPixels ; t++ )
	{
//		CPixel32 Pix = *pSrc;
//		*pDest = CVec3Dfp32(Pix.GetR(), Pix.GetG(), Pix.GetB());
		M_VSt_V3_Slow(M_VMul(M_VLd_Pixel32_f32(pSrc), M_VScalar(1.0f/255.0f)), pDest);
		pSrc++;
		pDest++;
	}
}

static void CImage_Convert_RGB24_RGBFLOAT(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_RGBFLOAT, MAUTOSTRIP_VOID);

	uint8 *pSrc = (uint8*)_pSrc;
	CVec3Dfp32* pDest = (CVec3Dfp32*)_pDest;
	for(int t = 0 ; t < _nPixels ; t++ )
	{
		CVec3Dfp32 Pix;
		Pix[2] = fp32(*(pSrc++)) / 255.0f;
		Pix[1] = fp32(*(pSrc++)) / 255.0f;
		Pix[0] = fp32(*(pSrc++)) / 255.0f;
		*pDest = Pix;
		pDest++;
	}
}

static void CImage_Convert_RGBFLOAT_RGB24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGBFLOAT_RGB24, MAUTOSTRIP_VOID);

	CVec3Dfp32* pSrc = (CVec3Dfp32*)_pSrc;
	uint8 *pDest = (uint8*)_pDest;
	for(int t = 0 ; t < _nPixels ; t++ )
	{
		*(pDest++) = RoundToInt(Min(pSrc->k[2] * 255.0f, 255.0f));
		*(pDest++) = RoundToInt(Min(pSrc->k[1] * 255.0f, 255.0f));
		*(pDest++) = RoundToInt(Min(pSrc->k[0] * 255.0f, 255.0f));
		pSrc++;
	}
}

static void CImage_Convert_RGB32_I8A8_GB2IA(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_I8A8_GB2IA, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDest = (uint8*)_pDest;
	for(int i = 0; i < _nPixels; i++)
	{
		*pDest++	= *pSrc++;	// blue
		*pDest++	= *pSrc++;	// green

		pSrc++;
		pSrc++;
	}
}

static void CImage_Convert_RGB24_I8A8_GB2IA(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_I8A8_GB2IA, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDest = (uint8*)_pDest;
	for(int i = 0; i < _nPixels; i++)
	{
		*pDest++	= *pSrc++;
		*pDest++	= *pSrc++;
		pSrc++;
	}
}

static void CImage_Convert_RGB32_RGBA32_GB2GA(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB32_RGBA32_GB2GA, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDest = (uint8*)_pDest;
	for(int i = 0; i < _nPixels; i++)
	{
		uint8 Blue = *pSrc++;
		uint8 Green = *pSrc++;
		pSrc++;
		uint8 Alpha = *pSrc++;

		*pDest++	= Alpha;
		*pDest++	= Blue;
		*pDest++	= 0;
		*pDest++	= Green;
	}
}

static void CImage_Convert_RGB24_RGBA32_GB2GA(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_RGB24_RGBA32_GB2GA, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDest = (uint8*)_pDest;
	for(int i = 0; i < _nPixels; i++)
	{
		uint8 Blue = *pSrc++;
		uint8 Green = *pSrc++;
		pSrc++;

		*pDest++	= 255; // alpha
		*pDest++	= Green;
		*pDest++	= 0;
		*pDest++	= Blue;
	}
}

static void CImage_Convert_I8A8_BGR8_IA2Normal(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_I8A8_BGR8_IA2Normal, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDest = (uint8*)_pDest;
	for(int i = 0; i < _nPixels; i++)
	{
		uint8 B = *pSrc++;
		uint8 G = *pSrc++;

		float fx = B - 127.5f;
		float fy = G - 127.5f;
		float fz = 16256.25f - Sqr(fx) - Sqr(fy);
		if(fz < 0) fz = 0;

		uint8 R = uint8(M_Sqrt(fz) + 127.5f);

		*pDest++	= B;
		*pDest++	= G;
		*pDest++	= R;
	}
}

static void CImage_Convert_I8A8_BGRA8_IA2Normal(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_I8A8_BGRA8_IA2Normal, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDest = (uint8*)_pDest;
	for(int i = 0; i < _nPixels; i++)
	{
		uint8 B = *pSrc++;
		uint8 G = *pSrc++;

		float fx = B - 127.5f;
		float fy = G - 127.5f;
		float fz = 16256.25f - Sqr(fx) - Sqr(fy);
		if(fz < 0) fz = 0;

		uint8 R = (uint8)(M_Sqrt(fz) + 127.5f);

		*pDest++	= B;
		*pDest++	= G;
		*pDest++	= R;
		*pDest++	= 255;
	}
}

static void CImage_Convert_A8_I8A8(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_A8_I8A8, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDst = (uint8*)_pDest;

	for(int x = 0; x < _nPixels; x++)
	{
		*pDst++	= 255;
		*pDst++	= *pSrc++;
	}
}

static void CImage_Convert_I8A8_RGBA8(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_I8A8_RGBA8, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDest = (uint8*)_pDest;

	for(int x = 0; x < _nPixels; x++)
	{
		int i = *pSrc++;
		int a = *pSrc++;
		*pDest++	= i;		// blue
		*pDest++	= i;		// green
		*pDest++	= i;		// red
		*pDest++	= a;		// alpha
	}
}

static void CImage_Convert_BGRA8_I8A8(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels)
{
	MAUTOSTRIP(CImage_Convert_BGRA8_I8A8, MAUTOSTRIP_VOID);

	uint8* pSrc = (uint8*)_pSrc;
	uint8* pDest = (uint8*)_pDest;

	for(int x = 0; x < _nPixels; x++)
	{
		int b = *pSrc++;
		int g = *pSrc++;
		int r = *pSrc++;
		int a = *pSrc++;
		*pDest++	= (b+g*2+r) >> 2;		// intensity
		*pDest++	= a;					// alpha
	}
}

// -------------------------------------------------------------------
void CImage::ConvertPixelArray(void* pSrc, void* pDest, int SrcFmt, int DestFmt, int nPixels, CPixel32* pSrcPal, int _ConvertType)
{
	MAUTOSTRIP(CImage_ConvertPixelArray, MAUTOSTRIP_VOID);

	switch(_ConvertType)
	{
	// ---------------------------------
	//	RGB
	// ---------------------------------
	case IMAGE_CONVERT_RGB :
		{
			if (SrcFmt == DestFmt)
			{
				_Move(pSrc, pDest, nPixels * Format2PixelSize(SrcFmt));
				return;
			};

			switch(SrcFmt)
			{
			// ---------------------------------
			//	ALPHA8
			// ---------------------------------
			case IMAGE_FORMAT_A8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_Alpha8_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGB)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
				}

			// ---------------------------------
			//	CLUT8
			// ---------------------------------
			case IMAGE_FORMAT_CLUT8 :
				{
					if (pSrcPal == NULL) return;

					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5 :
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_CLUT8_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_B5G6R5 :
						{
							CImage_Convert_CLUT8_RGB565(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGR8 :
						{
							CImage_Convert_CLUT8_RGB24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA8 :
					case IMAGE_FORMAT_BGRX8 :
						{
							CImage_Convert_CLUT8_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA4 :
						{
							CImage_Convert_CLUT8_RGBA4444(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_CLUT8_ABGR32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGB8 :
						{
							CImage_Convert_CLUT8_BGR24(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	RGB565
			// ---------------------------------
			case IMAGE_FORMAT_B5G6R5 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5 :
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_RGB565_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGR8 :
						{
							CImage_Convert_RGB565_RGB24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGB565_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			case IMAGE_FORMAT_BGR5 :
			case IMAGE_FORMAT_BGR5A1 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGBA5551_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	RGB24
			// ---------------------------------
			case IMAGE_FORMAT_BGR8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5 :
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_RGB24_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_B5G6R5 :
						{
							CImage_Convert_RGB24_RGB565(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGB24_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_RGB24_ABGR32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGB8 :
						{
							CImage_Convert_RGB24_BGR24(pSrc, pDest, pSrcPal, nPixels);
						}
					case IMAGE_FORMAT_I8 :
						{
							CImage_Convert_RGB24_GREY8(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
					case IMAGE_FORMAT_RGB32_F :
						{
							CImage_Convert_RGB24_RGBFLOAT(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	RGB32
			// ---------------------------------
			case IMAGE_FORMAT_BGRX8 :
			case IMAGE_FORMAT_BGRA8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5 :
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_RGB32_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_B5G6R5 :
						{
							CImage_Convert_RGB32_RGB565(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGR8 :
						{
							CImage_Convert_RGB32_RGB24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGB8:
						{
							CImage_Convert_RGB32_BGR24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
							CImage_Convert_RGB32_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;

					case IMAGE_FORMAT_BGRA4 :
							CImage_Convert_RGB32_RGBA4444(pSrc, pDest, pSrcPal, nPixels);
							return;

					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_RGB32_ABGR32(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
					case IMAGE_FORMAT_I8 :
						{
							CImage_Convert_RGB32_GREY8(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
					case IMAGE_FORMAT_RGB32_F :
						{
							CImage_Convert_RGB32_RGBFLOAT(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	GREY8
			// ---------------------------------
			case IMAGE_FORMAT_I8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_B5G6R5 :
						{
							CImage_Convert_GREY8_RGB565(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGR8 :
						{
							CImage_Convert_GREY8_RGB24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_GREY8_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			case IMAGE_FORMAT_RGB32_F :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGBFLOAT_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGR8 :
						{
							CImage_Convert_RGBFLOAT_RGB24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
					break;
				}

			case IMAGE_FORMAT_BGR10A2 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_BGR10A2_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGR8 :
						{
							CImage_Convert_BGR10A2_RGB24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
					break;
				}
			default :
				Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
			} // IMAGE_CONVERT_RGB
		}
	// ---------------------------------
	//	RGBA
	// ---------------------------------
	case IMAGE_CONVERT_RGBA :
		{
			if (SrcFmt == DestFmt)
			{
				_Move(pSrc, pDest, nPixels * Format2PixelSize(SrcFmt));
				return;
			};

			switch(SrcFmt)
			{
			// ---------------------------------
			//	IMAGE_FORMAT_I8A8
			// ---------------------------------
			case IMAGE_FORMAT_I8A8:
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRA8:
						{
							CImage_Convert_I8A8_RGBA8(pSrc, pDest, pSrcPal, nPixels);
							return;
						}

					default:
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
					break;
				}

			// ---------------------------------
			//	RGFB_FLOAT
			// ---------------------------------
			case IMAGE_FORMAT_RGB32_F:
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGBFLOAT_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGR8 :
						{
							CImage_Convert_RGBFLOAT_RGB24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
					break;
				}

			// ---------------------------------
			//	ALPHA8
			// ---------------------------------
			case IMAGE_FORMAT_A8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_Alpha8_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
		        }
      
			// ---------------------------------
			//	CLUT8
			// ---------------------------------
			case IMAGE_FORMAT_CLUT8 :
				{
					if (pSrcPal == NULL) return;

					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_CLUT8_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_CLUT8_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA4 :
						{
							CImage_Convert_CLUT8_RGBA4444(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_CLUT8_ABGR32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	RGB565
			// ---------------------------------
			case IMAGE_FORMAT_B5G6R5 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_RGB565_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGB565_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			case IMAGE_FORMAT_BGR5 :
			case IMAGE_FORMAT_BGR5A1 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGBA5551_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	RGB24
			// ---------------------------------
			case IMAGE_FORMAT_BGR8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_RGB24_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGB24_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_RGB24_ABGR32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	RGB32
			// ---------------------------------
			case IMAGE_FORMAT_BGRX8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_RGB32_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGB32_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA4 :
						{
							CImage_Convert_RGB32_RGBA4444(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_RGB32_ABGR32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	RGBA32
			// ---------------------------------
			case IMAGE_FORMAT_BGRA8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGR5A1 :
						{
							CImage_Convert_RGBA32_RGBA5551(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGRA4 :
						{
							CImage_Convert_RGBA32_RGBA4444(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_RGBA32_ABGR32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_A8 :
						{
							CImage_Convert_RGBA32_Alpha8(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_I8A8 :
						{
							CImage_Convert_BGRA8_I8A8(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	GREY8
			// ---------------------------------
			case IMAGE_FORMAT_I8 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRA8 :
					case IMAGE_FORMAT_RGBA8 :
						{
							CImage_Convert_GREY8_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};

			// ---------------------------------
			//	RGBA4444
			// ---------------------------------
			case IMAGE_FORMAT_BGRA4 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_RGBA4444_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					};
					break;
				};
			case IMAGE_FORMAT_BGR10A2 :
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRX8 :
					case IMAGE_FORMAT_BGRA8 :
						{
							CImage_Convert_BGR10A2_RGBA32(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					case IMAGE_FORMAT_BGR8 :
						{
							CImage_Convert_BGR10A2_RGB24(pSrc, pDest, pSrcPal, nPixels);
							return;
						};
					default :
						Error_static("ConvertScanline", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
					break;
				}
			default :
				Error_static("ConvertScanline (RGBA)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
			} // IMAGE_CONVERT_RGBA
		}

	// ---------------------------------
	//	INVERSEALPHA
	// ---------------------------------
	case IMAGE_CONVERT_INVERSEALPHA :
		switch(SrcFmt)
		{
		// ---------------------------------
		//	CLUT8
		// ---------------------------------
		case IMAGE_FORMAT_CLUT8 :
			{
				switch(DestFmt)
				{
				case IMAGE_FORMAT_BGRA8 :
				case IMAGE_FORMAT_RGBA8 :
					{
						CImage_Convert_CLUT8_ABGR32_InverseAlpha(pSrc, pDest, pSrcPal, nPixels);
						return;
					};
				default :
					Error_static("ConvertScanline (InvAlpha)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
				};
				break;
			};

		// ---------------------------------
		//	RGB32
		// ---------------------------------
		case IMAGE_FORMAT_BGRX8 :
			{
				switch(DestFmt)
				{
				case IMAGE_FORMAT_BGRA4 :
					{
						CImage_Convert_RGB32_RGBA4444_InverseAlpha(pSrc, pDest, pSrcPal, nPixels);
						return;
					};
				case IMAGE_FORMAT_BGRA8 :
				case IMAGE_FORMAT_RGBA8 :
					{
						CImage_Convert_RGB32_ABGR32_InverseAlpha(pSrc, pDest, pSrcPal, nPixels);
						return;
					}
				default :
					Error_static("ConvertScanline (InvAlpha)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
				};
				break;
			};
		default :
			Error_static("ConvertScanline (InvAlpha)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
		} // IMAGE_CONVERT_INVERSEALPHA

	// ---------------------------------
	//	RGBA_AFROMRGB
	// ---------------------------------
	case IMAGE_CONVERT_RGBA_AFROMRGB :
		switch(SrcFmt)
		{
		// ---------------------------------
		//	CLUT8
		// ---------------------------------
		case IMAGE_FORMAT_CLUT8 :
			{
				switch(DestFmt)
				{
				case IMAGE_FORMAT_BGRA8 :
					{
						CImage_Convert_CLUT8_RGBA32_AFromRGB(pSrc, pDest, pSrcPal, nPixels);
						return;
					};
				default :
					Error_static("ConvertScanline (AFromRGB)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
				};
				break;
			};

		// ---------------------------------
		//	GREY8
		// ---------------------------------
		case IMAGE_FORMAT_I8 :
			{
				switch(DestFmt)
				{
				case IMAGE_FORMAT_BGRA8 :
					{
						CImage_Convert_GREY8_RGBA32_AFromRGB(pSrc, pDest, pSrcPal, nPixels);
						return;
					};
				default :
					Error_static("ConvertScanline (AFromRGB)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
				};
				break;
			};

		// ---------------------------------
		//	RGB24
		// ---------------------------------
		case IMAGE_FORMAT_BGR8:
			{
				switch(DestFmt)
				{
				case IMAGE_FORMAT_BGRA8 :
					{
						CImage_Convert_RGB24_RGBA32_AFromRGB(pSrc, pDest, pSrcPal, nPixels);
						return;
					};
				default :
					Error_static("ConvertScanline (AFromRGB)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
				};
				break;
			};

		// ---------------------------------
		//	RGB32, RGBA32
		// ---------------------------------
		case IMAGE_FORMAT_BGRX8 :
		case IMAGE_FORMAT_BGRA8 :
			{
				switch(DestFmt)
				{
				case IMAGE_FORMAT_BGRA8 :
					{
						CImage_Convert_RGB32_RGBA32_AFromRGB(pSrc, pDest, pSrcPal, nPixels);
						return;
					};
				default :
					Error_static("ConvertScanline (AFromRGB)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
				};
				break;
			};
		default :
			Error_static("ConvertScanline (AFromRGB)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
		} // IMAGE_CONVERT_INVERSEALPHA

	// ---------------------------------
	//	RGB_GB2IA
	// ---------------------------------
	case IMAGE_CONVERT_RGB_GB2IA:
		{
			switch(SrcFmt)
			{
			case IMAGE_FORMAT_BGRX8:
			case IMAGE_FORMAT_BGRA8:
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_I8A8:
						{
							CImage_Convert_RGB32_I8A8_GB2IA(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
					default:
						Error_static("ConvertScaneline (GB2IA)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
				}
				break;

			case IMAGE_FORMAT_BGR8:
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_I8A8:
						{
							CImage_Convert_RGB24_I8A8_GB2IA(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
					default:
						Error_static("ConvertScanline (GB2IA)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
				}
				break;

			default:
				Error_static("ConvertScanline (GB2IA)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
			}
		}

	case IMAGE_CONVERT_RGB_GB2GA:
		{
			switch(SrcFmt)
			{
			case IMAGE_FORMAT_BGRA8:
			case IMAGE_FORMAT_BGRX8:
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRA8:
						{
							CImage_Convert_RGB32_RGBA32_GB2GA(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
						break;

					default:
						Error_static("ConvertScanLine (GB2GA)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
				}
				break;

			case IMAGE_FORMAT_BGR8:
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRA8:
						{
							CImage_Convert_RGB24_RGBA32_GB2GA(pSrc, pDest, pSrcPal, nPixels);
							return;
						}
						break;

					default:
						Error_static("ConvertScanLine (GB2GA)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
				}
				break;

			default:
				Error_static("ConvertScanLine (GB2GA)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
			}
		}

	// ---------------------------------
	//	IA2NORMAL
	// ---------------------------------
	case IMAGE_CONVERT_IA2NORMAL :
		{
			switch(SrcFmt)
			{
			case IMAGE_FORMAT_I8A8:
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_BGRX8:
					case IMAGE_FORMAT_BGRA8:
						{
							CImage_Convert_I8A8_BGRA8_IA2Normal(pSrc, pDest, pSrcPal, nPixels);
							return;
						}

					case IMAGE_FORMAT_BGR8:
						{
							CImage_Convert_I8A8_BGR8_IA2Normal(pSrc, pDest, pSrcPal, nPixels);
							return;
						}

					default:
						Error_static("ConvertScanline (IA2NORMAL)", CStrF("Convertion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
					break;
				}

			default:
				Error_static("ConvertScanline (IA2NORMAL)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
			}
		}

	// ---------------------------------
	//	IA2NORMAL
	// ---------------------------------
	case IMAGE_CONVERT_ALPHA_TO_RGB :
		{
			switch(SrcFmt)
			{
			case IMAGE_FORMAT_A8:
				{
					switch(DestFmt)
					{
					case IMAGE_FORMAT_I8A8:
						{
							CImage_Convert_A8_I8A8(pSrc, pDest, pSrcPal, nPixels);
							return;
						};

					default:
						Error_static("ConvertScanline (ALPHA_TO_RGB)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
					}
				};

			default:
				Error_static("ConvertScanline (ALPHA_TO_RGB)", CStrF("Conversion not supported. (%s -> %s)", CImage::GetFormatName(SrcFmt), CImage::GetFormatName(DestFmt)));
			}
		}


	default :
		Error_static("ConvertScanline", CStrF("Convertion type not supported. (%d)", _ConvertType));
	}
};

void CImage::Convert(CImage* pSrc, CImage* pDest, int _ConvertType)
{
	MAUTOSTRIP(CImage_Convert_pSrc_pDest_ConvertType, MAUTOSTRIP_VOID);

	if (pSrc == NULL) Error_static("CImage::Convert", "Source is NULL.");
	if (pDest == NULL) Error_static("CImage::Convert", "Destination is NULL.");

	int srcfmt = pSrc->GetFormat();
	int destfmt = pDest->GetFormat();
	if ((srcfmt == destfmt) && (_ConvertType <= IMAGE_CONVERT_RGBA))
	{
		pDest->Blt(pDest->GetClipRect(), *pSrc, 0, CPnt(0,0));
		return;
	};

	if (!(pSrc->m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!(pDest->m_Memmodel & IMAGE_MEM_LOCKABLE)) return;

	int h = Min(pDest->GetHeight(), pSrc->GetHeight());
	int w = Min(pDest->GetWidth(), pSrc->GetWidth());

	if (destfmt == IMAGE_FORMAT_CLUT8)
	{
#ifndef PLATFORM_CONSOLE
		if (_ConvertType != IMAGE_CONVERT_RGB && _ConvertType != IMAGE_CONVERT_RGBA)
			Error_static("CImage::Convert", "Only RGB convertion supported on palettized images.");

		CImagePalette* pPal = pDest->GetPalette();
		if (!pPal) Error_static("CImage::Convert", "No destination palette for palette-convertion.");
		CDefaultQuantize* pQ = pPal->GetQuantizer();
/*		if ((pSrc->GetFormat() != IMAGE_FORMAT_CLUT8) && (pSrc->GetFormat() != IMAGE_FORMAT_CLUT8))
		{
			spCImage spTmp = DNew(CImage) CImage;
			if (spTmp == NULL) Error_static("Convert", "Out of memory.");
			spTmp->Create(pSrc->GetWidth(), pSrc->GetHeight(), IMAGE_FORMAT_BGR8, IMAGE_MEM_IMAGE);
			spTmp->Blt(spTmp->GetClipRect(), *pSrc, 0, CPnt(0,0));
			spCImage spTmp2 = pQ->Quantize(spTmp);
			pDest->Blt(pDest->GetClipRect(), *spTmp2, 0, CPnt(0, 0));
		}
		else
		{*/
			pQ->Begin();
			pQ->Include(pSrc);
			pQ->End();
			spCImage spImg = pQ->Quantize(pSrc);
			pDest->Blt(pDest->GetClipRect(), *spImg, 0, CPnt(0, 0));
			pDest->SetPalette(spImg->GetPalette());
//		}
		return;
#else
		Error_static("CImage::Convert", "Cannot convert to a clut texture on console.");
#endif
	}

	CPixel32 Pal[256];
	if (srcfmt == IMAGE_FORMAT_CLUT8) pSrc->GetPalette()->GetPalette((CPixel32*) &Pal, 0, 256);

	uint8* pSrcMem;
	uint8* pDestMem;
	MLock(pSrc, pSrcMem);
	MLock(pDest, pDestMem);

		int srcmod = pSrc->GetModulo();
		int destmod = pDest->GetModulo();

		for(int y = 0; y < h; y++)
			ConvertPixelArray(&pSrcMem[y*srcmod], &pDestMem[y*destmod], srcfmt, destfmt, w, (CPixel32*) &Pal, _ConvertType);

	MUnlock(pDest);
	MUnlock(pSrc);
};

int CImage::ConvertDestinationFormats(int _SrcFmt, int _ConvertType)
{
	MAUTOSTRIP(CImage_ConvertDestinationFormats, 0);

	switch(_ConvertType)
	{
	// Note: Any new alpha channel is set to 1.
	case IMAGE_CONVERT_RGB :
		switch(_SrcFmt)
		{
		case IMAGE_FORMAT_A8 : return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_CLUT8 : return IMAGE_FORMAT_BGR5 | IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_B5G6R5 | IMAGE_FORMAT_BGR8 | IMAGE_FORMAT_BGRX8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRA4 | IMAGE_FORMAT_RGBA8;
		case IMAGE_FORMAT_B5G6R5 : return IMAGE_FORMAT_BGR5 | IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_BGR8 | IMAGE_FORMAT_BGRX8 | IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_BGR8 : return IMAGE_FORMAT_BGR5 | IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_B5G6R5 | IMAGE_FORMAT_BGRX8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_RGBA8 | IMAGE_FORMAT_I8 | IMAGE_FORMAT_RGB32_F;
		case IMAGE_FORMAT_BGRA8 : ;
		case IMAGE_FORMAT_BGRX8 : return IMAGE_FORMAT_BGR5 | IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_B5G6R5 | IMAGE_FORMAT_BGR8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRA4 | IMAGE_FORMAT_RGBA8 | IMAGE_FORMAT_I8;
		case IMAGE_FORMAT_I8 : return IMAGE_FORMAT_B5G6R5 | IMAGE_FORMAT_BGRX8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_RGBA8;
		case IMAGE_FORMAT_RGB32_F : return IMAGE_FORMAT_BGRX8 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGR8;
		default : return 0;
		};

	// Note: Any /NEW/ alpha channel is set to 1, only RGBA-Formats can be a destination format.
	case IMAGE_CONVERT_RGBA :
		switch(_SrcFmt)
		{
		case IMAGE_FORMAT_A8 : return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_CLUT8 : return IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_BGRA4 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_RGBA8;
		case IMAGE_FORMAT_B5G6R5 : return IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_BGR8 : return IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_RGBA8;
		case IMAGE_FORMAT_BGRX8 : return IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRA4 | IMAGE_FORMAT_RGBA8;
		case IMAGE_FORMAT_BGRA8 : return IMAGE_FORMAT_BGR5A1 | IMAGE_FORMAT_BGRA4 | IMAGE_FORMAT_RGBA8 | IMAGE_FORMAT_A8 | IMAGE_FORMAT_I8A8;
		case IMAGE_FORMAT_I8A8 : return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_I8 : return IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_RGBA8;
		case IMAGE_FORMAT_BGRA4 : return IMAGE_FORMAT_BGRA8;

		default : return 0;
		};

	case IMAGE_CONVERT_INVERSEALPHA :
		switch(_SrcFmt)
		{
		case IMAGE_FORMAT_BGRX8 : return IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_BGRA4 | IMAGE_FORMAT_RGBA8;
		default : return 0;
		};

	case IMAGE_CONVERT_RGBA_AFROMRGB :
		switch(_SrcFmt)
		{
		case IMAGE_FORMAT_CLUT8: return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_I8 : return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_BGR8 : return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_BGRX8 : return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_BGRA8 : return IMAGE_FORMAT_BGRA8;
		default : return 0;
		};

	case IMAGE_CONVERT_RGB_GB2IA :
		switch(_SrcFmt)
		{
		case IMAGE_FORMAT_BGR8 : 
		case IMAGE_FORMAT_BGRA8 : 
		case IMAGE_FORMAT_BGRX8 : return IMAGE_FORMAT_I8A8;
		default : return 0;
		};

	case IMAGE_CONVERT_RGB_GB2GA:
		switch(_SrcFmt)
		{
		case IMAGE_FORMAT_BGR8: return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_BGRA8: return IMAGE_FORMAT_BGRA8;
		case IMAGE_FORMAT_BGRX8: return IMAGE_FORMAT_BGRA8;
		default : return 0;
		}

	case IMAGE_CONVERT_IA2NORMAL :
		switch(_SrcFmt)
		{
		case IMAGE_FORMAT_I8A8 : return IMAGE_FORMAT_BGRA8;
		default : return 0;
		};

	default : return 0;
	}
	return 0;
};

void CImage::Convert(CImage* _pTarget, int _TargetFmt, int _ConvertType)
{
	MAUTOSTRIP(CImage_Convert_pTarget_TargetFmt_ConvertType, MAUTOSTRIP_VOID);

	if (!_pTarget) Error("Convert", "_pTarget == NULL");
	if ((_pTarget->GetWidth() != GetWidth()) ||
		(_pTarget->GetHeight() != GetHeight()) ||
		(_pTarget->GetFormat() != _TargetFmt))
		_pTarget->Create(GetWidth(), GetHeight(), _TargetFmt, IMAGE_MEM_IMAGE, GetPalette(), GetFlags());

	CImage::Convert(this, _pTarget, _ConvertType);
}

spCImage CImage::Convert(int _TargetFmt, int _ConvertType)
{
	MAUTOSTRIP(CImage_Convert_TargetFmt_ConvertType, NULL);

	spCImage spImg = MNew(CImage);
	if (!spImg) MemError("Convert");

	spImg->Create(GetWidth(), GetHeight(), _TargetFmt, IMAGE_MEM_IMAGE, GetPalette(), GetFlags());
	CImage::Convert(this, spImg, _ConvertType);
	return spImg;
}

// -------------------------------------------------------------------
//#define CIMAGE_ADDSATURATE_OLDMIX

void SYSTEMDLLEXPORT PPA_Mul_RGB32(int _EffectValue1, const void* _pSrc, void* _pDest, int _nPixels);
void SYSTEMDLLEXPORT PPA_Mul_RGB32(int _EffectValue1, const void* _pSrc, void* _pDest, int _nPixels)
{
	MAUTOSTRIP(PPA_Mul_RGB32, MAUTOSTRIP_VOID);

	// Operation:
	// D.col = S.col * Eff.col
	// D.a = 1

	// OBS!  0xff != 1.0

	CPixel32  effCol = CPixel32( _EffectValue1 );
	effCol.A() = 0xff;

#ifdef CPU_INTELP5

	int RVal = effCol.GetR();
	int GVal = effCol.GetG();
	int BVal = effCol.GetB();

	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		mov ecx, _nPixels
Lp:
		push ecx

			mov ecx, [esi]
			xor ebx, ebx
			movzx eax, cl
			mul [word ptr BVal]
			mov bl, ah

			movzx eax, ch
			rol ecx, 16
			mul [word ptr GVal]
			mov bh, ah

			movzx eax, cl
			rol ebx, 16
			mul [word ptr RVal]
			mov bl, ah
			mov bh, 0xff
			rol ebx, 16

		mov [edi], ebx
		pop ecx
		lea esi, [esi+4]
		dec ecx
		lea edi, [edi+4]

		jnz Lp
	}
#else

	CPixel32 *pSrc = (CPixel32 *)_pSrc;
	CPixel32 *pDest = (CPixel32 *)_pDest;

	while( _nPixels )
	{
	  *pDest = *pSrc * effCol;
		pDest++; pSrc++;
		_nPixels--;
	}
#endif
}

void SYSTEMDLLEXPORT PPA_Mul_RGBA32(int _EffectValue1, const void* _pSrc, void* _pDest, int _nPixels);
void SYSTEMDLLEXPORT PPA_Mul_RGBA32(int _EffectValue1, const void* _pSrc, void* _pDest, int _nPixels)
{
	MAUTOSTRIP(PPA_Mul_RGBA32, MAUTOSTRIP_VOID);

	// Operation:
	// D.col = S.col * Eff.col
	// D.a = S.a * Eff.a

	// 0xff == 1.0

	CPixel32  effCol = CPixel32( _EffectValue1 );

#ifdef CPU_INTELP5

	int RVal = CPixel32(effCol).GetR();
	int GVal = CPixel32(effCol).GetG();
	int BVal = CPixel32(effCol).GetB();
	int AVal = CPixel32(effCol).GetA();

	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		mov ecx, _nPixels
Lp:
		push ecx

			mov ecx, [esi]
			xor ebx, ebx
			movzx eax, cl
			mul [word ptr BVal]
			add eax, 128
			add al, ah
			adc ah, 0
			mov bl, ah

			movzx eax, ch
			rol ecx, 16
			mul [word ptr GVal]
			add eax, 128
			add al, ah
			adc ah, 0
			mov bh, ah

			movzx eax, cl
			rol ebx, 16
			mul [word ptr RVal]
			add eax, 128
			add al, ah
			adc ah, 0
			mov bl, ah

			movzx eax, ch
			mul [word ptr AVal]
			add eax, 128
			add al, ah
			adc ah, 0
			mov bh, ah

			rol ebx, 16

		mov [edi], ebx
		pop ecx
		lea esi, [esi+4]
		dec ecx
		lea edi, [edi+4]

		jnz Lp
	}
#else

/*
	movzx eax, cl			; col = srccol & 0x000000ff
	mul [word ptr BVal]		; col *= BVal
	add eax, 128			; col += 128
	add al, ah				; c = ((((col&0xff00)>>8)+(col&0xff)) < 255) ? 1 : 0;
	adc ah, 0				; col += c;
	mov bl, ah				; stuva in i minnet
*/

	CPixel32 *pSrc = (CPixel32 *)_pSrc;
	CPixel32 *pDest = (CPixel32 *)_pDest;

	while( _nPixels )
	{
	  *pDest = *pSrc * effCol;
		pDest++; pSrc++;
		_nPixels--;
	}
#endif
}

void SYSTEMDLLEXPORT PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* _pSrc, void* _pDest, int _nPixels);
void SYSTEMDLLEXPORT PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* _pSrc, void* _pDest, int _nPixels)
{
	MAUTOSTRIP(PPA_MulAddSaturate_RGB32, MAUTOSTRIP_VOID);

	// Operation:
	// D.col = Min(255, D.col + S.col*Eff.col)
	// D.a = 1


	CPixel32  effCol = CPixel32( _EffectValue1 );
	effCol.A() = 0xff;

#ifdef CPU_INTELP5

	int RVal = CPixel32(effCol).GetR();
	int GVal = CPixel32(effCol).GetG();
	int BVal = CPixel32(effCol).GetB();

	__asm
	{
		mov esi, [_pSrc]
		mov edi, [_pDest]
		mov ecx, _nPixels
Lp:
		push ecx

			mov ecx, [esi]
			xor ebx, ebx
			movzx eax, cl
			mul [word ptr BVal]
			mov bl, ah

			movzx eax, ch
			rol ecx, 16
			mul [word ptr GVal]
			mov bh, ah

			movzx eax, cl
			rol ebx, 16
			mul [word ptr RVal]
			mov bl, ah
			mov eax, [edi]
			rol ebx, 16

			and ebx, 0xfefefe
			and eax, 0xfefefe
			mov edx, 0x1010100
			add ebx, eax
			mov eax, ebx
			shr ebx, 7
			and ebx,0x020202
			sub edx, ebx
			or eax, edx
			or eax, 0xff000000		// Alpha = 1

		mov [edi], eax
		pop ecx
		lea esi, [esi+4]
		dec ecx
		lea edi, [edi+4]

		jnz Lp
	}
#else
	// D.col = Min(255, D.col + S.col*Eff.col)
	// D.a = 1

	CPixel32 *pSrc = (CPixel32 *)_pSrc;
	CPixel32 *pDest = (CPixel32 *)_pDest;

	while( _nPixels )
	{
	  *pDest = *pDest + ( *pSrc * effCol );
		pDest++; pSrc++;
		_nPixels--;
	}
#endif
}

void CImage::ProcessPixelArray(int _Operation, int _EffectValue1, int _EffectValue2, 
	void* pSrc, void* pDest, int SrcFmt, int DestFmt, int nPixels, CPixel32* pSrcPal)
{
	MAUTOSTRIP(CImage_ProcessPixelArray, MAUTOSTRIP_VOID);

#ifdef  PLATFORM_DOLPHIN
	Error_static( "ProcessPixelArray", "FIX COLORS FOR DOLPHIN!!!!!" );
#endif
	if (DestFmt != SrcFmt) Error_static("ProcessPixelArray", "Does not support color convertion.");

	switch(_Operation)
	{
	case IMAGE_OPER_ADDWRAP:
		{
			if(SrcFmt == IMAGE_FORMAT_RGB32_F)
			{
				CVec3Dfp32* npSrc = (CVec3Dfp32*)pSrc;
				CVec3Dfp32* npDst = (CVec3Dfp32*)pDest;
				while(nPixels)
				{
					*npDst = *npSrc + *npDst;
					npDst++;
					npSrc++;
					nPixels--;
				}

				return;
			}
			else
				Error_static("ProcessPixelArray (ADDWRAP)", CStr("Unsupported format: ")+ GetFormatName(SrcFmt));
			break;
		};
	case IMAGE_OPER_ADDSATURATE :
		{

			if (SrcFmt == IMAGE_FORMAT_BGR8)
			{
				uint8 *npSrc = (uint8 *)pSrc;
				uint8 *npDst = (uint8 *)pDest;

				nPixels *= Format2PixelSize(SrcFmt);
				while( nPixels )		// mov ecx, nPixels
				{
					*npDst = Min( 255, *npDst + *npSrc );
					npDst++; npSrc++;
					nPixels--;
				}
				return;
			}

			if ((SrcFmt != IMAGE_FORMAT_BGRX8) && 
				(SrcFmt != IMAGE_FORMAT_BGRA8)) Error_static("ProcessPixelArray (ADDSATURATE)", CStr("Unsupported format: ")+ GetFormatName(SrcFmt));

#ifdef CPU_INTELP5
			__asm
			{
				mov esi, [pSrc]
				mov edi, [pDest]
				mov ecx, nPixels
Lp_RGB32_ASAT:
#ifdef CIMAGE_ADDSATURATE_OLDMIX
					mov ebx, [esi]
					mov edx, [edi]
					add dl, bl
					jnc RGB32_ASAT_NoRClip
						mov dl, 255
RGB32_ASAT_NoRClip:
					add dh, bh
					jnc RGB32_ASAT_NoGClip
						mov dh, 255
RGB32_ASAT_NoGClip:
					rol ebx, 16
					rol edx, 16
					add dl, bl
					jnc RGB32_ASAT_NoBClip
						mov dl, 255
RGB32_ASAT_NoBClip:
					xor dh, dh
					rol edx, 16

				mov [edi], edx
#else
					mov ebx, [esi]
					mov edx, [edi]
					shr ebx, 1
					shr edx, 1
					and ebx, 0x7f7f7f7f
					and edx, 0x7f7f7f7f
					add ebx, edx
					mov eax, ebx
					shr eax, 7

					mov edx, 0x80808080
					and eax,0x01010101
					sub edx, eax
					or ebx, edx

					and ebx, 0x7f7f7f7f
					add ebx, ebx
				mov [edi], ebx
#endif
//				mov [edi], ecx
//				pop ecx
				lea esi, [esi+4]
				dec ecx
				lea edi, [edi+4]

				jnz Lp_RGB32_ASAT

			}
#else
//			uint32 *npSrc = (uint32 *)pSrc;		// mov esi, [pSrc]
//			uint32 *npDest = (uint32 *)pDest;	// mov edi, [pDest]
//			uint32 a, b, c;
			uint8 *npSrc = (uint8 *)pSrc;
			uint8 *npDst = (uint8 *)pDest;

			nPixels<<=2;
			while( nPixels )		// mov ecx, nPixels
			{
				*npDst = Min( 255, *npDst + *npSrc );
				npDst++; npSrc++;
				nPixels--;
/*				a = *npSrc;			// mov ebx, [esi]
				b = *npDest;		// mov edx, [edi]
				a >>= 1;			// shr ebx, 1
				b >>= 1;			// shr edx, 1
				a &= 0x7f7f7f7f;	// and ebx, 0x7f7f7f7f
				b &= 0x7f7f7f7f;	// and edx, 0x7f7f7f7f
				a += b;				// add ebx, edx
				c = a;				// mov eax, ebx
				c >>= 7;			// shr eax, 7
				b = 0x80808080;		// mov edx, 0x80808080
				c &= 0x01010101;	// and eax,0x01010101
				b -= c;				// sub edx, eax
				b |= a;				// or ebx, edx
				a &= 0x7f7f7f7f;	// and ebx, 0x7f7f7f7f
				a += a;				// add ebx, ebx
				*npDest = a;		// mov [edi], ebx
				npDest++;			// lea esi, [esi+4]
				nPixels--;			// dec ecx
				npSrc++;			// lea edi, [edi+4]*/
			}
#endif
			return;
		}

	case IMAGE_OPER_MULADDSATURATE :
		{
			if ((SrcFmt != IMAGE_FORMAT_BGRX8) && 
				(SrcFmt != IMAGE_FORMAT_BGRA8)) Error_static("ProcessPixelArray (MULADDSATURATE)", CStr("Unsupported format: ")+ GetFormatName(SrcFmt));

			int RVal = CPixel32(_EffectValue1).GetR();
			int GVal = CPixel32(_EffectValue1).GetG();
			int BVal = CPixel32(_EffectValue1).GetB();

#ifdef CPU_INTELP5
			__asm
			{
				mov esi, [pSrc]
				mov edi, [pDest]
				mov ecx, nPixels
Lp_RGB32_MASAT:
				push ecx
					xor eax, eax
					mov ebx, [esi]
					mov al, bl
					mul [word ptr BVal]
					mov ecx, [edi]
					add cl, ah
					jnc RGB32_MASAT_NoRClip
						mov cl, 255
RGB32_MASAT_NoRClip:
					xor eax, eax
					mov al, bh
					mul [word ptr GVal]
					add ch, ah
					jnc RGB32_MASAT_NoGClip
						mov ch, 255
RGB32_MASAT_NoGClip:
					xor eax, eax
					rol ecx, 16
					rol ebx, 16
					mov al, bl
					mul [word ptr RVal]
					add cl, ah
					jnc RGB32_MASAT_NoBClip
						mov cl, 255
RGB32_MASAT_NoBClip:
					xor ch, ch
					rol ecx, 16

				mov [edi], ecx
				pop ecx
				lea esi, [esi+4]
				dec ecx
				lea edi, [edi+4]

				jnz Lp_RGB32_MASAT

			}
#else
//			uint32 *npSrc = (uint32 *)pSrc;			//	mov esi, [pSrc]
//			uint32 *npDest = (uint32 *)pDest;		//	mov edi, [pDest]
			uint8 *npSrc = (uint8 *)pSrc;
			uint8 *npDst = (uint8 *)pDest;
//			uint32 a, c;
//			uint32 r, g, b;

			while( nPixels )
			{
#ifdef  PLATFORM_DOLPHIN
				*npDst = Min( 255, *npDst + ((*npSrc * BVal)>>8));
				npDst++; npSrc++;

				*npDst = Min( 255, *npDst + ((*npSrc * GVal)>>8));
				npDst++; npSrc++;

				*npDst = Min( 255, *npDst + ((*npSrc * RVal)>>8));
				npDst++; npSrc++;

				*npDst = 0xff;
				npDst++; npSrc++;
#else
				*npDst = Min( 255, *npDst + ((*npSrc * RVal)>>8));
				npDst++; npSrc++;

				*npDst = Min( 255, *npDst + ((*npSrc * GVal)>>8));
				npDst++; npSrc++;

				*npDst = Min( 255, *npDst + ((*npSrc * BVal)>>8));
				npDst++; npSrc++;

				*npDst = 0xff;
				npDst++; npSrc++;
#endif				

				nPixels--;
/*				a = 0;
				c = *npDest;

				r = (c & 0xff) * RVal;
				r += *npSrc;
				if( r < 0xff ) r = 0xff;

				g = ((c >> 8) & 0xff) * GVal;
				g += *npSrc;
				if( g < 0xff ) g = 0xff;

				b = ((c >> 16) & 0xff) * BVal;
				b += *npSrc;
				if( b < 0xff ) b = 0xff;

				c = r + (g << 8) + (b << 16);
				*npDest = c;

				npSrc++;
				npDest++;
				nPixels--;*/
			}
/*			while( nPixels )						// mov ecx, nPixels
			{										// push ecx
				a = 0;								// xor eax, eax
				b = *npSrc;							// mov ebx, [esi]
				a = b & 0xff;						// mov al, bl
				a *= BVal;							// mul [word ptr BVal]
				c = *npDest;						// mov ecx, [edi]
				d = (c & 0xff) + ((a >> 8) 0xff );	// add cl, ah
				if( d < 0xff )						// jnc RGB32_MASAT_NoRClip
					c |= 0x000000ff;				// mov cl, 255
				else								//
				{									//
					c &= 0xffffff00;				//
					c |= d;							//
				}									//
													// RGB32_MASAT_NoRClip:
				a = 0;								// xor eax, eax
				a = (b >> 8) & 0xff;				// mov al, bh
				a *= GVal;							// mul [word ptr GVal]
				d = ((c>>8)&0xff) + ((a>>8)&0xff);	// add ch, ah
				if( d < 0xff )						// jnc RGB32_MASAT_NoGClip
					c |= 0x0000ff00;				// mov ch, 255
				else								//
				{									//
					c &= 0xffff00ff;				//
					c |= d << 8;					//
				}									//
													// RGB32_MASAT_NoGClip:
				a = 0;								// xor eax, eax
													// rol ecx, 16
													// rol ebx, 16
				a = (( b >> 16 ) & 0xff );			// mov al, bl
				a *= RVal;							// mul [word ptr RVal]
				d = ((c>>16)&0xff) + ((a>>8)&0xff);	// add cl, ah
				if( d < 0xff )						// jnc RGB32_MASAT_NoBClip
					c |= 0x00ff0000;				// mov cl, 255
				else								//
				{									//
					c &= 0xff00ffff;				//
					c |= d << 16;					//
				}									//
													// RGB32_MASAT_NoBClip:
				c &= 0x00ffffff;					// xor ch, ch
													// rol ecx, 16
				*npDest = c;						// mov [edi], ecx
													// pop ecx
				npSrc++;							// lea esi, [esi+4]
				nPixels--;							// dec ecx
				npDest++;							// lea edi, [edi+4]
			}										// jnz Lp_RGB32_MASAT
*/
#endif
			return;
		}
	}
}

// -------------------------------------------------------------------
void CImage::ProcessPixelArray_Stretch(int _Operation, int _EffectValue1, int _EffectValue2, 
	void* pSrc, void* pDest, int SrcFmt, int DestFmt, int nPixels, CPixel32* pSrcPal,
	int _nSrcPixels, fp32 _SourcePos, fp32 _SourceStep)
{
	MAUTOSTRIP(CImage_ProcessPixelArray_Stretch, MAUTOSTRIP_VOID);

	switch(_Operation)
	{
	case IMAGE_OPER_MULADDSATURATE :
		{
			if (DestFmt != SrcFmt) Error_static("ProcessPixelArray_Stretch", "Does not support color convertion.");
			if ((SrcFmt != IMAGE_FORMAT_BGRX8) && 
				(SrcFmt != IMAGE_FORMAT_BGRA8)) Error_static("ProcessPixelArray_Stretch (MULADDSATURATE)", CStr("Unsupported format: ")+ GetFormatName(SrcFmt));

			int RVal = CPixel32(_EffectValue1).GetR();
			int GVal = CPixel32(_EffectValue1).GetG();
			int BVal = CPixel32(_EffectValue1).GetB();
//			_SourceStep = 32.0f;
//			_SourcePos = 128.0f;


/*int32* pS = (int32*) pSrc;
for(int i = 0; i < _nSrcPixels; i++)
	if (pS[i] != 0x00ff00)
		LogFile(CStrF("%.8X,  %d, %.8X", pS, i, pS[i]));
*/


			int SrcStep = (int)(_SourceStep*65536.0f);
			int SrcPos = (int)(_SourcePos*65536.0f);
			int nSrcPixels = _nSrcPixels;
//LogFile(CStrF("%.8X, %.8X, %d", SrcStep, SrcPos, nSrcPixels));
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, [pSrc]
				mov edi, [pDest]
				mov ecx, nPixels
				mov edx, [SrcPos]
Lp_RGB32_MASAT:
				push ecx
					xor eax, eax
					xor ecx, ecx
					push edx
					sar edx, 16
					cmp edx, [nSrcPixels]
					jae RGB32_MASAT_Skip

					mov ebx, [esi + edx*4]
//				mov ebx, 0x00ff00
					mov al, bl
					mul [word ptr BVal]
					mov ecx, [edi]
					add cl, ah
					jnc RGB32_MASAT_NoRClip
						mov cl, 255
RGB32_MASAT_NoRClip:
					xor eax, eax
					mov al, bh
					mul [word ptr GVal]
					add ch, ah
					jnc RGB32_MASAT_NoGClip
						mov ch, 255
RGB32_MASAT_NoGClip:
					xor eax, eax
					rol ecx, 16
					rol ebx, 16
					mov al, bl
					mul [word ptr RVal]
					add cl, ah
					jnc RGB32_MASAT_NoBClip
						mov cl, 255
RGB32_MASAT_NoBClip:
					xor ch, ch
					rol ecx, 16
//		mov ecx,0x00ff00;
					mov [edi], ecx
RGB32_MASAT_Skip:
					pop edx

				pop ecx
				add edx, [SrcStep]
				dec ecx
				lea edi, [edi+4]

				jnz Lp_RGB32_MASAT

			}
#else
			/*     Aaaaaassssssssssss  */

/*
			uint32 *npSrc = (uint32 *)pSrc;		// mov esi, [pSrc]
			uint32 *npDest = (uint32 *)pDest;	// mov edi, [pDest]
			uint32 c, d, e;
			uint32 r, g, b;

			while( nPixels )					// mov ecx, nPixels
			{
												// mov edx, [SrcPos]
												// Lp_RGB32_MASAT:
												// push ecx
												// xor eax, eax
												// xor ecx, ecx
												// push edx
												// sar edx, 16
												// cmp edx, [nSrcPixels]
				c = npSrc[ SrcPos >> 16 ];		// mov ebx, [esi + edx*4]
				d = *npDest;					//
				if( SrcPos < nSrcPixels )		// jae RGB32_MASAT_Skip
				{								//
					e = c & 0xff;				// mov al, bl
					e *= RVal;					// mul [word ptr RVal]
					e += d & 0xff;				// mov ecx, [edi]
												// add cl, ah
					if( e < 0xff ) e = 0xff;	// jnc RGB32_MASAT_NoRClip
												// mov cl, 255
					r = e;						//
												// RGB32_MASAT_NoRClip:
												// xor eax, eax
					e = (c >> 8) & 0xff;		// mov al, bh
					e *= GVal;					// mul [word ptr GVal]
					e += (d >> 8) & 0xff;		// add ch, ah
					if( e < 0xff ) e = 0xff;	// jnc RGB32_MASAT_NoGClip
												// mov ch, 255
					g = e;						//
												// RGB32_MASAT_NoGClip:
												// xor eax, eax
					e = (c >> 16) & 0xff;		// rol ecx, 16
												// rol ebx, 16
												// mov al, bl
					e *= BVal;					// mul [word ptr BVal]
					e += (d >> 16) & 0xff;		// add cl, ah
					if( e < 0xff ) e = 0xff;	// jnc RGB32_MASAT_NoBClip
												// mov cl, 255
					b = e;						//
												// RGB32_MASAT_NoBClip:
					d = r + (g<<8) + (b<<16);	// xor ch, ch
												// rol ecx, 16
					*npDest = d;				// mov [edi], ecx
				}								//
												// RGB32_MASAT_Skip:
												// pop edx
												// pop ecx
				SrcPos += SrcStep;				// add edx, [SrcStep]
				nPixels--;						// dec ecx
				npDest++;						// lea edi, [edi+4]
			}									// jnz Lp_RGB32_MASAT
*/
		// D.col = Min(255, D.col + S.col*Eff.col)
		// D.a = 1
		uint8 *npDest = (uint8 *)pDest;
		uint8 *npSrcBase = (uint8 *)pSrc;
//		int a, c;
		uint32 t = 0;
		uint8 *npSrc = (uint8 *)npSrcBase;
		int max = nPixels;
		if( nSrcPixels < max ) max = nSrcPixels;

		while( t < nPixels )
		{
			if((SrcPos>>16) < nSrcPixels )
			{
				int SrcOfs = (SrcPos>>16)*4;

#ifdef  PLATFORM_DOLPHIN
				*npDest = Min( 255, *npDest + ((npSrc[SrcOfs+0] * BVal) >> 8));
				npDest++;

				*npDest = Min( 255, *npDest + ((npSrc[SrcOfs+1] * GVal) >> 8));
				npDest++;

				*npDest = Min( 255, *npDest + ((npSrc[SrcOfs+2] * RVal) >> 8));
				npDest++;

				*npDest = 0xff;
				npDest++;
#else
				*npDest = Min( 255, *npDest + ((npSrc[SrcOfs+0] * RVal) >> 8));
				npDest++;

				*npDest = Min( 255, *npDest + ((npSrc[SrcOfs+1] * GVal) >> 8));
				npDest++;

				*npDest = Min( 255, *npDest + ((npSrc[SrcOfs+2] * BVal) >> 8));
				npDest++;

				*npDest = 0xff;
				npDest++;
#endif
			}

//			npSrc += SrcStep;
			SrcPos += SrcStep;

			t++;
		}

#endif
		}
	}
}

// -------------------------------------------------------------------
#ifndef PLATFORM_CONSOLE
void CImage::StretchHalfNonPow2(CImage* pSrc, CImage* pDest)
{
	int w = pDest->GetWidth();
	int h = pDest->GetHeight();

	if((2 * w) != (pSrc->GetWidth() & ~1)) Error_static("CImage::StretchHalfNonPow2", CStrF("Illegal size parameters (%dx%d) -> (%dx%d)", pSrc->GetWidth(), pSrc->GetHeight(), w, h));
	if((2 * h) != (pSrc->GetHeight() & ~1)) Error_static("CImage::StretchHalfNonPow2", CStrF("Illegal size parameters (%dx%d) -> (%dx%d)", pSrc->GetWidth(), pSrc->GetHeight(), w, h));

	uint8* pS;
	uint8* pD;
	MLock(pSrc, pS);
	MLock(pDest, pD);
	int SrcFmt = pSrc->GetFormat();
	int SrcMod = pSrc->GetModulo();
	int DestMod = pDest->GetModulo();
	switch(SrcFmt)
	{
	case IMAGE_FORMAT_I8A8 :
		{
			uint8 *npSrc0 = pS;
			uint8 *npSrc1 = (pS + SrcMod);
			uint8 *npDst = pD;

			for(int y=0 ; y < h; y++ )
			{
				for(int x=0 ; x < w; x++ )
				{
					int SrcX = x << 1;

					uint32 i = (npSrc0[SrcX * 2 + 0] + npSrc0[(SrcX + 1) * 2 + 0] + npSrc1[SrcX * 2 + 0] + npSrc1[(SrcX + 1) * 2 + 0])>>2;
					uint32 a = (npSrc0[SrcX * 2 + 1] + npSrc0[(SrcX + 1) * 2 + 1] + npSrc1[SrcX * 2 + 1] + npSrc1[(SrcX + 1) * 2 + 1])>>2;
					npDst[x * 2 + 0] = i;
					npDst[x * 2 + 1] = a;
				}

				npSrc0 += SrcMod * 2;
				npSrc1 += SrcMod * 2;
				npDst += DestMod;
			}
			break;
		}

	case IMAGE_FORMAT_RGB8:
	case IMAGE_FORMAT_BGR8:
		{
			uint8 *npSrc0 = pS;
			uint8 *npSrc1 = (pS + SrcMod);
			uint8 *npDst = pD;

			for(int y=0 ; y < h; y++ )
			{
				for(int x=0 ; x < w; x++ )
				{
					int SrcX = x << 1;

					uint32 r = (npSrc0[SrcX * 3 + 0] + npSrc0[(SrcX + 1) * 3 + 0] + npSrc1[SrcX * 3 + 0] + npSrc1[(SrcX + 1) * 3 + 0])>>2;
					uint32 g = (npSrc0[SrcX * 3 + 1] + npSrc0[(SrcX + 1) * 3 + 1] + npSrc1[SrcX * 3 + 1] + npSrc1[(SrcX + 1) * 3 + 1])>>2;
					uint32 b = (npSrc0[SrcX * 3 + 2] + npSrc0[(SrcX + 1) * 3 + 2] + npSrc1[SrcX * 3 + 2] + npSrc1[(SrcX + 1) * 3 + 2])>>2;
					npDst[x * 3 + 0] = r;
					npDst[x * 3 + 1] = g;
					npDst[x * 3 + 2] = b;
				}

				npSrc0 += SrcMod * 2;
				npSrc1 += SrcMod * 2;
				npDst += DestMod;
			}
			break;
		}

	case IMAGE_FORMAT_RGBA8:
	case IMAGE_FORMAT_BGRA8:
	case IMAGE_FORMAT_BGRX8:
		{
			uint32 *npSrc0 = (uint32 *)pS;
			uint32 *npSrc1 = (uint32 *)(pS + SrcMod);
			uint32 *npDst = (uint32 *)pD;

			for(int y=0 ; y < h; y++ )
			{
				for(int x=0 ; x < w; x++ )
				{
					uint32 a, b, c, d;

					a = (npSrc0[ (x<<1)   ] >>1) & 0x7f7f7f7f;
					b = (npSrc0[ (x<<1)+1 ] >>1) & 0x7f7f7f7f;
					c = (npSrc1[ (x<<1)   ] >>1) & 0x7f7f7f7f;
					d = (npSrc1[ (x<<1)+1 ] >>1) & 0x7f7f7f7f;
					a += b;
					c += d;
					a >>= 1;
					c >>= 1;
					a &= 0x7f7f7f7f;
					c &= 0x7f7f7f7f;
					npDst[ x ] = a + c;
				}

				npSrc0 += (SrcMod>>1);
				npSrc1 += (SrcMod>>1);
				npDst += (DestMod>>2);
			}
			break;
		}
	default:
		Error_static("CImage::StretchHalfNonPow2", CStrF("Unsupported format: %s", GetFormatName(SrcFmt)));
	};
	MUnlock(pDest);
	MUnlock(pSrc);
}

void CImage::StretchHalfXNonPow2(CImage* pSrc, CImage* pDest)
{
	int w = pDest->GetWidth();
	int h = pDest->GetHeight();

	if((2 * w) != (pSrc->GetWidth() & ~1)) Error_static("CImage::StretchHalfXNonPow2", CStrF("Illegal size parameters (%dx%d) -> (%dx%d)", pSrc->GetWidth(), pSrc->GetHeight(), w, h));

	uint8* pS;
	uint8* pD;
	MLock(pSrc, pS);
	MLock(pDest, pD);
	int SrcFmt = pSrc->GetFormat();
	int SrcMod = pSrc->GetModulo();
	int DestMod = pDest->GetModulo();
	switch(SrcFmt)
	{
	case IMAGE_FORMAT_I8A8:
		{
			uint8 *npSrc0 = pS;
			uint8 *npDst = pD;

			for(int y = 0; y < h; y++)
			{
				for(int x=0 ; x < w; x++ )
				{
					int SrcX = x << 1;
					uint32 i = (npSrc0[SrcX * 2 + 0] + npSrc0[(SrcX + 1) * 2 + 0])>>1;
					uint32 a = (npSrc0[SrcX * 2 + 1] + npSrc0[(SrcX + 1) * 2 + 1])>>1;
					npDst[x * 2 + 0]	= i;
					npDst[x * 2 + 1]	= a;
				}
				npSrc0	+= SrcMod;
				npDst	+= DestMod;
			}
			break;
		}

	case IMAGE_FORMAT_RGB8:
	case IMAGE_FORMAT_BGR8:
		{
			uint8 *npSrc0 = pS;
			uint8 *npDst = pD;

			for(int y = 0;y < h; y++)
			{
				for(int x=0 ; x < w; x++ )
				{
					int SrcX = x << 1;

					uint32 r = (npSrc0[SrcX * 3 + 0] + npSrc0[(SrcX + 1) * 3 + 0])>>1;
					uint32 g = (npSrc0[SrcX * 3 + 1] + npSrc0[(SrcX + 1) * 3 + 1])>>1;
					uint32 b = (npSrc0[SrcX * 3 + 2] + npSrc0[(SrcX + 1) * 3 + 2])>>1;
					npDst[x * 3 + 0] = r;
					npDst[x * 3 + 1] = g;
					npDst[x * 3 + 2] = b;
				}
				npSrc0	+= SrcMod;
				npDst	+= DestMod;
			}
			break;
		}

	case IMAGE_FORMAT_RGBA8:
	case IMAGE_FORMAT_BGRA8:
	case IMAGE_FORMAT_BGRX8:
		{
			uint32 *npSrc0 = (uint32 *)pS;
			uint32 *npDst = (uint32 *)pD;

			for(int y = 0; y < h; y++)
			{
				for(int x=0 ; x < w; x++ )
				{
					uint32 a, b;

					a = (npSrc0[ (x<<1)   ] >>1) & 0x7f7f7f7f;
					b = (npSrc0[ (x<<1)+1 ] >>1) & 0x7f7f7f7f;
					npDst[ x ] = a + b;
				}
				npSrc0	+= SrcMod;
				npDst	+= DestMod;
			}
			break;
		}
	default:
		Error_static("CImage::StretchHalfXNonPow2", CStrF("Unsupported format: %s", GetFormatName(SrcFmt)));
	};
	MUnlock(pDest);
	MUnlock(pSrc);
}

void CImage::StretchHalfYNonPow2(CImage* pSrc, CImage* pDest)
{
	int w = pDest->GetWidth();
	int h = pDest->GetHeight();

	if((2 * h) != (pSrc->GetHeight() & ~1)) Error_static("CImage::StretchHalfYNonPow2", CStrF("Illegal size parameters (%dx%d) -> (%dx%d)", pSrc->GetWidth(), pSrc->GetHeight(), w, h));

	uint8* pS;
	uint8* pD;
	MLock(pSrc, pS);
	MLock(pDest, pD);
	int SrcFmt = pSrc->GetFormat();
	int SrcMod = pSrc->GetModulo();
	int DestMod = pDest->GetModulo();
	switch(SrcFmt)
	{
	case IMAGE_FORMAT_I8A8:
		{
			uint8 *npSrc0 = pS;
			uint8 *npSrc1 = (pS + SrcMod);
			uint8 *npDst = pD;

			for(int y=0 ; y < h; y++ )
			{
				for(int x = 0; x < w; x++)
				{
					uint32 i = (npSrc0[x * 2 + 0] + npSrc1[x * 2 + 0])>>1;
					uint32 a = (npSrc0[x * 2 + 1] + npSrc1[x * 2 + 1])>>1;
					npDst[x * 2 + 0] = i;
					npDst[x * 2 + 1] = a;
				}
				npSrc0 += SrcMod * 2;
				npSrc1 += SrcMod * 2;
				npDst += DestMod;
			}
			break;
		}

	case IMAGE_FORMAT_RGB8:
	case IMAGE_FORMAT_BGR8:
		{
			uint8 *npSrc0 = pS;
			uint8 *npSrc1 = (pS + SrcMod);
			uint8 *npDst = pD;

			for(int y=0 ; y < h; y++ )
			{
				for(int x = 0; x < w; x++)
				{
					uint32 r = (npSrc0[x * 3 + 0] + npSrc1[x * 3 + 0])>>1;
					uint32 g = (npSrc0[x * 3 + 1] + npSrc1[x * 3 + 1])>>1;
					uint32 b = (npSrc0[x * 3 + 2] + npSrc1[x * 3 + 2])>>1;
					npDst[x * 3 + 0] = r;
					npDst[x * 3 + 1] = g;
					npDst[x * 3 + 2] = b;
				}

				npSrc0 += SrcMod * 2;
				npSrc1 += SrcMod * 2;
				npDst += DestMod;
			}
			break;
		}

	case IMAGE_FORMAT_RGBA8:
	case IMAGE_FORMAT_BGRA8:
	case IMAGE_FORMAT_BGRX8:
		{
			uint32 *npSrc0 = (uint32 *)pS;
			uint32 *npSrc1 = (uint32 *)(pS + SrcMod);
			uint32 *npDst = (uint32 *)pD;

			for(int y=0 ; y < h; y++ )
			{
				for(int x = 0; x < w; x++)
				{
					uint32 a, c;

					a = (npSrc0[x] >>1) & 0x7f7f7f7f;
					c = (npSrc1[x] >>1) & 0x7f7f7f7f;
					npDst[x] = a + c;
				}

				npSrc0 += (SrcMod>>1);
				npSrc1 += (SrcMod>>1);
				npDst += (DestMod>>2);
			}
			break;
		}
	default:
		Error_static("CImage::StretchHalfYNonPow2", CStrF("Unsupported format: %s", GetFormatName(SrcFmt)));
	};
	MUnlock(pDest);
	MUnlock(pSrc);
}

void CImage::StretchHalf(CImage* pSrc, CImage* pDest)
{
	MAUTOSTRIP(CImage_StretchHalf, MAUTOSTRIP_VOID);

	if (!(pSrc->m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!(pDest->m_Memmodel & IMAGE_MEM_LOCKABLE)) return;

	int SrcFmt = pSrc->GetFormat();
	int DestFmt = pDest->GetFormat();
	if (SrcFmt != DestFmt) Error_static("CImage::StretchHalf", "Does not support color convertions.");

	int w = pSrc->GetWidth();
	int h = pSrc->GetHeight();
	if ((pDest->GetWidth()*2 != w) || (pDest->GetHeight()*2 != h))
	{
		StretchHalfNonPow2(pSrc, pDest);
		return;
	}

	uint8* pS;
	uint8* pD;
	MLock(pSrc, pS);
	MLock(pDest, pD);
	int SrcMod = pSrc->GetModulo();
	int DestMod = pDest->GetModulo();
	switch(SrcFmt)
	{
	case IMAGE_FORMAT_CLUT8 :
		{
			uint8* pMixTab = (uint8*) pSrc->GetPalette()->GetAcceleratorTable(SrcFmt, DestFmt, PAL_ACCEL_MIX50, pDest->GetPalette());
#ifdef CPU_INTELP5
			__asm
			{
				mov edx, pMixTab
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_8:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
				xor ebx, ebx
				xor eax, eax
				push ebp
				mov ebp, SrcMod
LpX_8:
					mov bx, [esi]
					mov al, [ebx + edx]
					mov bx, [esi + ebp]
					mov ah, [ebx + edx]
					mov al, [eax + edx]

					add esi, 2
					mov [edi], al
					dec ecx
					lea edi, [edi+1]
					jnz LpX_8

				pop ebp
				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_8
			}
#else
			for( uint32 y = 0 ; y < h>>1 ; y++ )
			{
				uint8 *npSrc = (uint8 *)(pS + y*SrcMod*2);
				uint8 *npDest = (uint8 *)(pD + y*DestMod);
				for( uint32 x = 0 ; x < w>>1 ; x++ )
				{
					uint8 c;
					uint8 b = npSrc[ 0 ];
					uint16 a = pMixTab[ b ];
					b = npSrc[ 1 ];
					a |= (pMixTab[ b ] << 8);
					c = pMixTab[ a ];
					npSrc += 2;
					*npDest = c;
					npDest++;
				}
//				npSrc += SrcMod<<1;
//				npDest += DestMod;
			}
/*
			int x, y;		//
			int c;	//
			int a, b;		//
							// mov edx, pMixTab
							// mov esi, pS
							// mov edi, pD
			uint8 *npSrc = (uint8 *)pS;		//
			uint8 *npDest = (uint8 *)pD;	//
			for( y = 0 ; y < h>>1 ; y++ )	// mov ecx, h
			{								// shr ecx, 1
											// LpY_8:
											// push ecx
				for( x = 0 ; x < w>>1 ; x++ )	// mov ecx, w
				{								// shr ecx, 1
												// push esi
												// push edi
					b = 0;						// xor ebx, ebx
					a = 0;						// xor eax, eax
												// push ebp
												// mov ebp, SrcMod
												// LpX_8:
					b |= *npSrc;				// mov bx, [esi]
					a |= pMixTab[ b ];			// mov al, [ebx + edx]
					b = *npSrc;					// mov bx, [esi + ebp]
					a |= (pMixTab[ b ] << 8);	// mov ah, [ebx + edx]
					c = pMixTab[ a ];			// mov al, [eax + edx]
												//
					npSrc += 2;					// add esi, 2
					*npDest = c & 0xff;			// mov [edi], al
												// dec ecx
					npDest++;					// lea edi, [edi+1]
				}								// jnz LpX_8
											//
											// pop ebp
											// pop edi
											// pop esi
											// pop ecx
				npSrc += SrcMod;			// add esi, SrcMod
				npSrc += SrcMod;			// add esi, SrcMod
				npDest += DestMod;			// add edi, DestMod
											// dec ecx
			}								// jnz LpY_8
*/
#endif
			break;
		}

	case IMAGE_FORMAT_B5G6R5 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_565:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
				mov edx, SrcMod
LpX_565:
					xor eax, eax
					xor ebx, ebx
					push ecx
					mov ax, [esi]
					mov bx, [esi+2]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7bcf
					and ebx, 0x7bcf
					add eax, ebx
//0x7bef
					xor ebx, ebx
					xor ecx, ecx
					mov bx, [esi + edx]
					mov cx, [esi+2 + edx]
					shr ebx, 1
					shr ecx, 1
					and ebx, 0x7bcf
					and ecx, 0x7bcf
					add ebx, ecx
					
					shr eax, 1
					shr ebx, 1
					and eax, 0x7bcf
					and ebx, 0x7bcf
					add eax, ebx

					mov [edi], ax
					pop ecx
					add esi, 4
					add edi, 2
					dec ecx
					jnz LpX_565

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_565
			};
#else
/*/
			uint32 x, y, a, b, c;
			uint16 Col;
			uint16 *npSrc = (uint16 *)pS;	// mov esi, pS
			uint16 *npDest = (uint16 *)pD;	// mov edi, pD

			for(y=0;y<(h>>1);y++)	// mov ecx, h
									// shr ecx, 1
			{						// LpY_565:
									// push ecx
				uint16 *npTSrc = npSrc;		// push esi
				uint16 *npTDest = npDest;	// push edi
				for(x=0;x<(w>>1);x++)	// mov ecx, w
										// shr ecx, 1
										// mov edx, SrcMod
				{						// LpX_565:
					a = 0;					// xor eax, eax
					b = 0;					// xor ebx, ebx
											// push ecx
					a = *npTSrc;			// mov ax, [esi]
					b = npTSrc[1];			// mov bx, [esi+2]
					a >>= 1;				// shr eax, 1
					b >>= 1;				// shr ebx, 1
					a &= 0x7bcf;			// and eax, 0x7bcf
					b &= 0x7bcf;			// and ebx, 0x7bcf
					a += b;					// add eax, ebx
											//
					b = 0;					// xor ebx, ebx
					c = 0;					// xor ecx, ecx
					b = npTSrc[SrcMod];		// mov bx, [esi + edx]
					c = npTSrc[SrcMod+1];	// mov cx, [esi+2 + edx]
					b >>= 1;				// shr ebx, 1
					c >>= 1;				// shr ecx, 1
					b &= 0x7bcf;			// and ebx, 0x7bcf
					c &= 0x7bcf;			// and ecx, 0x7bcf
					b += c;					// add ebx, ecx
											//
					a >>= 1;				// shr eax, 1
					b >>= 1;				// shr ebx, 1
					a &= 0x7bcf;			// and eax, 0x7bcf
					b &= 0x7bcf;			// and ebx, 0x7bcf
					a += b;					// add eax, ebx
											//
					*npTDest = a & 0xffff;	// mov [edi], ax
//					*npTDest = 0xffff;	// mov [edi], ax
											// pop ecx
					npTSrc += 2;				// add esi, 4
					npTDest++;				// add edi, 2
											// dec ecx
				}						// jnz LpX_565
										//
									// pop edi
									// pop esi
									// pop ecx
				npSrc+=SrcMod;		// add esi, SrcMod
				npDest+=DestMod>>1;	// add edi, DestMod
									// dec ecx
			}					//jnz LpY_565
/*/
			uint16 *npSrc = (uint16 *)pS;
			uint16 *npDst = (uint16 *)pD;
/*			uint8 r[2], g[2], b[2], rr[2], gg[2], bb[2];
			uint8 fr, fg, fb;*/
			uint16 c[2];
			uint16 cc[2];
			for( uint32 y=0 ; y<(h>>1) ; y++ )
			{
				for( uint32 x=0 ; x<(w>>1) ; x++ )
				{
					uint32 SrcOfs = ((y<<1)*SrcMod) + (x<<1);
					for( uint8 t0=0 ; t0<2 ; t0++ )
					{
						for( uint8 t1=0 ; t1<2 ; t1++ )
						{
/*							r[t1] = (npSrc[ (((y<<1)+t0)*SrcMod) + ((x<<1)+t1) ] >> 11) & 0x1f;
							g[t1] = (npSrc[ (((y<<1)+t0)*SrcMod) + ((x<<1)+t1) ] >> 5) & 0x3f;
							b[t1] = (npSrc[ (((y<<1)+t0)*SrcMod) + ((x<<1)+t1) ]) & 0x1f;*/
//							c[t1] = (npSrc[ (((y<<1)+t0)*SrcMod) + ((x<<1)+t1) ] >> 1) & 0x7bef;
							c[t1] = (npSrc[ SrcOfs + ((t0) ? SrcMod : 0) + t1 ]>>1) & 0x7bef;
						}
						cc[t0] = ((c[0] + c[1]) >> 1) & 0x7bef;
/*						rr[t0] = (r[0] + r[1])>>1;
						gg[t0] = (g[0] + g[1])>>1;
						bb[t0] = (b[0] + b[1])>>1;*/
					}
					npDst[ x ] = cc[0] + cc[1];
				}

				npSrc += SrcMod;
				npSrc += SrcMod;
				npDst += DestMod;
			}
/**/
#endif
			break;
		};
	case IMAGE_FORMAT_BGR8 :
	case IMAGE_FORMAT_RGB8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_24:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
				mov edx, SrcMod
LpX_24:
					push ecx
					mov eax, [esi]
					mov ebx, [esi+3]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7f7f7f7f
					and ebx, 0x7f7f7f7f
					add eax, ebx

					mov ebx, [esi + edx]
					mov ecx, [esi+3 + edx]
					shr ebx, 1
					shr ecx, 1
					and ebx, 0x7f7f7f7f
					and ecx, 0x7f7f7f7f
					add ebx, ecx
					
					shr eax, 1
					shr ebx, 1
					and eax, 0x7f7f7f7f
					and ebx, 0x7f7f7f7f
					add eax, ebx

					mov [edi], eax
					pop ecx
					add esi, 6
					add edi, 3
					dec ecx
					jnz LpX_24

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_24
			};
#else
//*/
			uint8 r[2][2], g[2][2], b[2][2], rr[2], gg[2], bb[2];
			uint8 *npSrc = (uint8 *)pS;
			uint8 *npDst = (uint8 *)pD;
			for( uint32 y=0 ; y<h ; y+=2 ){
				for( uint32 x=0 ; x<w ; x+=2 ){
					for( int t0=0 ; t0<2 ; t0++ ){
						uint32 SrcOfs = t0*SrcMod;
						for( int t1=0 ; t1<2 ; t1++ ){
							r[t0][t1] = npSrc[ SrcOfs + (x+x+x + t1+t1+t1) + 0] >> 1;
							g[t0][t1] = npSrc[ SrcOfs + (x+x+x + t1+t1+t1) + 1] >> 1;
							b[t0][t1] = npSrc[ SrcOfs + (x+x+x + t1+t1+t1) + 2] >> 1;
						}
						rr[t0] = (r[t0][0] + r[t0][1]) >> 1;
						gg[t0] = (g[t0][0] + g[t0][1]) >> 1;
						bb[t0] = (b[t0][0] + b[t0][1]) >> 1;
					}

					npDst[((x+x+x)<<1) + 0] = rr[0] + rr[1];
					npDst[((x+x+x)<<1) + 1] = gg[0] + gg[1];
					npDst[((x+x+x)<<1) + 2] = bb[0] + bb[1];
				}
				npSrc += SrcMod<<1;
				npDst += DestMod;
			}
/*/			uint32 x, y;
			uint32 a, b, c;
			uint8 *npSrc = (uint8 *)pS;		// mov esi, pS
			uint8 *npDest = (uint8 *)pD;	// mov edi, pD
			for( y=0 ; y<(h>>1) ; y++ )		// mov ecx, h
											// shr ecx, 1
			{								// LpY_24:
												// push ecx
												// push esi
												// push edi
				for( x=0 ; x<(w>>1) ; x++ )		// mov ecx, w
												// shr ecx, 1
												// mov edx, SrcMod
				{								// LpX_24:
																// push ecx
					a = *((uint32*)(&npSrc[x+x+x]));			// mov eax, [esi]
					b = *((uint32*)(&npSrc[x+x+x+3]));			// mov ebx, [esi+3]
					a >>= 1;									// shr eax, 1
					b >>= 1;									// shr ebx, 1
					a &= 0x7f7f7f7f;							// and eax, 0x7f7f7f7f
					b &= 0x7f7f7f7f;							// and ebx, 0x7f7f7f7f
					a += b;										// add eax, ebx
																//
					b = *((uint32*)(&npSrc[(x+SrcMod)*3]));		// mov ebx, [esi + edx]
					c = *((uint32*)(&npSrc[(x+SrcMod)*3+3]));	// mov ecx, [esi+3 + edx]
					b >>= 1;									// shr ebx, 1
					c >>= 1;									// shr ecx, 1
					b &= 0x7f7f7f7f;							// and ebx, 0x7f7f7f7f
					c &= 0x7f7f7f7f;							// and ecx, 0x7f7f7f7f
					b += c;										// add ebx, ecx
																//
					a >>= 1;									// shr eax, 1
					b >>= 1;									// shr ebx, 1
					a &= 0x7f7f7f7f;							// and eax, 0x7f7f7f7f
					b &= 0x7f7f7f7f;							// and ebx, 0x7f7f7f7f
					a += b;										// add eax, ebx
																//
					*npDest = a;								// mov [edi], eax
																// pop ecx
																// add esi, 6
																// add edi, 3
																// dec ecx
				}								// jnz LpX_24
												//
												// pop edi
												// pop esi
												// pop ecx
				npSrc += SrcMod;				// add esi, SrcMod
				npDest += DestMod >> 1;			// add edi, DestMod
												// dec ecx
			}								// jnz LpY_24
*/
#endif
			break;
		};
	case IMAGE_FORMAT_BGRX8 :
	case IMAGE_FORMAT_BGRA8 :
	case IMAGE_FORMAT_RGBA8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_32:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
				mov edx, SrcMod
LpX_32:
					push ecx
					mov eax, [esi]
					mov ebx, [esi+4]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7f7f7f7f
					and ebx, 0x7f7f7f7f
					add eax, ebx

					mov ebx, [esi + edx]
					mov ecx, [esi+4 + edx]
					shr ebx, 1
					shr ecx, 1
					and ebx, 0x7f7f7f7f
					and ecx, 0x7f7f7f7f
					add ebx, ecx
					
					shr eax, 1
					shr ebx, 1
					and eax, 0x7f7f7f7f
					and ebx, 0x7f7f7f7f
					add eax, ebx

					mov [edi], eax
					pop ecx
					add esi, 8
					add edi, 4
					dec ecx
					jnz LpX_32

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_32
			};
#else
			uint32 *npSrc0 = (uint32 *)pS;
			uint32 *npSrc1 = (uint32 *)(pS + SrcMod);
			uint32 *npDst = (uint32 *)pD;

			for( uint32 y=0 ; y<(h>>1) ; y++ )
			{
				for( uint32 x=0 ; x<(w>>1) ; x++ )
				{
					uint32 a, b, c, d;

					a = (npSrc0[ (x<<1)   ] >>1) & 0x7f7f7f7f;
					b = (npSrc0[ (x<<1)+1 ] >>1) & 0x7f7f7f7f;
					c = (npSrc1[ (x<<1)   ] >>1) & 0x7f7f7f7f;
					d = (npSrc1[ (x<<1)+1 ] >>1) & 0x7f7f7f7f;
					a += b;
					c += d;
					a >>= 1;
					c >>= 1;
					a &= 0x7f7f7f7f;
					c &= 0x7f7f7f7f;
					npDst[ x ] = a + c;
				}

				npSrc0 += (SrcMod>>1);
				npSrc1 += (SrcMod>>1);
				npDst += (DestMod>>2);
			}
/*
			uint8 r[2][2], g[2][2], b[2][2], a[2][2], rr[2], gg[2], bb[2], aa[2];
			uint8 *npSrc = (uint8 *)pS;
			uint8 *npDst = (uint8 *)pD;
			for( uint32 y=0 ; y<h ; y+=2 ){
				for( uint32 x=0 ; x<w ; x+=2 ){
					for( int t0=0 ; t0<2 ; t0++ ){
						uint32 SrcOfs = t0*SrcMod;
						for( int t1=0 ; t1<2 ; t1++ ){
							r[t0][t1] = npSrc[ SrcOfs + ((x+t1)<<2) + 0] >> 1;
							g[t0][t1] = npSrc[ SrcOfs + ((x+t1)<<2) + 1] >> 1;
							b[t0][t1] = npSrc[ SrcOfs + ((x+t1)<<2) + 2] >> 1;
							a[t0][t1] = npSrc[ SrcOfs + ((x+t1)<<2) + 3] >> 1;
						}
						rr[t0] = (r[t0][0] + r[t0][1]) >> 1;
						gg[t0] = (g[t0][0] + g[t0][1]) >> 1;
						bb[t0] = (b[t0][0] + b[t0][1]) >> 1;
						aa[t0] = (a[t0][0] + a[t0][1]) >> 1;
					}
					npDst[(x<<1) + 0] = rr[0] + rr[1];
					npDst[(x<<1) + 1] = gg[0] + gg[1];
					npDst[(x<<1) + 2] = bb[0] + bb[1];
					npDst[(x<<1) + 3] = aa[0] + aa[1];
				}
				npSrc += SrcMod<<1;
				npDst += DestMod;
			}
*/

/*			uint32 x, y, a, b, c;
			uint32 *npSrc = (uint32 *)pS;	// mov esi, pS
			uint32 *npDest = (uint32 *)pD;	// mov edi, pD

			for( y=0 ; y<(h>>1) ; y++ )		// mov ecx, h
											// shr ecx, 1
			{								// LpY_32:
												// push ecx
				uint32 *npTSrc = npSrc;			// push esi
				uint32 *npTDest = npDest;		// push edi
												//
				for( x=0 ; x<(w>>1) ; x++)		// mov ecx, w
												// shr ecx, 1
												// mov edx, SrcMod
				{								// LpX_32:
													// push ecx
					a = *npTSrc;					// mov eax, [esi]
					b = *(npTSrc+1);				// mov ebx, [esi+4]
					a >>= 1;						// shr eax, 1
					b >>= 1;						// shr ebx, 1
					a &= 0x7f7f7f7f;				// and eax, 0x7f7f7f7f
					b &= 0x7f7f7f7f;				// and ebx, 0x7f7f7f7f
					a += b;							// add eax, ebx
													//
					b = *(npTSrc+(SrcMod>>1));		// mov ebx, [esi + edx]
					c = *(npTSrc+(SrcMod>>1)+1);	// mov ecx, [esi+4 + edx]
					b >>= 1;						// shr ebx, 1
					c >>= 1;						// shr ecx, 1
					b &= 0x7f7f7f7f;				// and ebx, 0x7f7f7f7f
					b &= 0x7f7f7f7f;				// and ecx, 0x7f7f7f7f
					b += c;							// add ebx, ecx
													//
					a >>= 1;						// shr eax, 1
					b >>= 1;						// shr ebx, 1
					a &= 0x7f7f7f7f;				// and eax, 0x7f7f7f7f
					b &= 0x7f7f7f7f;				// and ebx, 0x7f7f7f7f
					a += b;							// add eax, ebx
													//
					*npTDest = a;					// mov [edi], eax
//					*npTDest = 0xffffffff;					// mov [edi], eax
													// pop ecx
					npTSrc += 2;					// add esi, 8
					npTDest++;						// add edi, 4
													// dec ecx
				}								// jnz LpX_32
												//
												// pop edi
												// pop esi
												// pop ecx
				npSrc += SrcMod;				// add esi, SrcMod
												// add esi, SrcMod
				npDest += DestMod>>2;			// add edi, DestMod
												// dec ecx
			}								// jnz LpY_32
*/
#endif
			break;
		};
  
  
	case IMAGE_FORMAT_A8: // Alpha8 == grey8 ... /Jim
	case IMAGE_FORMAT_I8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_Gray:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
				mov edx, SrcMod
LpX_Gray:
					push ecx

						xor eax, eax
						movzx eax, word ptr [esi]
						movzx ecx, al
						shr eax, 8
						add ecx, eax

						movzx eax, word ptr [esi + edx]
						movzx ebx, al
						shr eax, 8
						add ecx, ebx
						add ecx, eax
						shr ecx, 2

					mov [edi], cl
					pop ecx
					add esi, 2
					add edi, 1
					dec ecx
					jnz LpX_Gray

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_Gray
			};
#else
//*/
			uint8 *npSrc0 = (uint8 *)pS;
			uint8 *npSrc1 = (uint8 *)(pS + SrcMod);
			uint8 *npDst = (uint8 *)pD;
			for( uint32 y=0 ; y<(h>>1) ; y++ )
			{
				for( uint32 x=0 ; x<(w>>1) ; x++ )
				{
					uint8 u = ((npSrc0[(x<<1)] + npSrc0[(x<<1)+1])>>2);
					uint8 l = ((npSrc1[(x<<1)] + npSrc1[(x<<1)+1])>>2);
					npDst[x] = u+l;
				}

				npSrc0 += SrcMod;
				npSrc0 += SrcMod;
				npSrc1 += SrcMod;
				npSrc1 += SrcMod;
				npDst += DestMod;
			}
/*/
			uint32 x, y;
			uint32 a, b, c;
			uint16 *npSrc = (uint16 *)pS;	// mov esi, pS
			uint8 *npDest = (uint8 *)pD;	// mov edi, pD
			for( y=0 ; y<(h>>1) ; y++ )		// mov ecx, h
											// shr ecx, 1
			{								// LpY_Gray:
												// push ecx
				uint16 *npTSrc = npSrc;			// push esi
				uint8 *npTDest = npDest;		// push edi
				for( x=0 ; x<(w>>1) ; x++ )		// mov ecx, w
												// shr ecx, 1
												// mov edx, SrcMod
				{								// LpX_Gray:
													// push ecx
													// xor eax, eax
						a = *npTSrc;				// mov ax, [esi]
						c = a & 0xff;				// movzx ecx, al
						a >>= 8;					// shr eax, 8
						c += a;						// add ecx, eax
													//
						a = *(npTSrc+SrcMod);		// mov eax, [esi + edx]
						b = a & 0xff;				// movzx ebx, al
						a >>= 8;					// shr eax, 8
						c += b;						// add ecx, ebx
						c += a;						// add ecx, eax
						c >>= 2;					// shr ecx, 2
													//
						*npTDest = c & 0xff;		// mov [edi], cl
													// pop ecx
						npTSrc++;					// add esi, 2
						npTDest++;					// add edi, 1
													// dec ecx
				}								// jnz LpX_Gray
												//
												// pop edi
												// pop esi
												// pop ecx
				npSrc += (SrcMod >> 1);			// add esi, SrcMod
												// add esi, SrcMod
				npDest += DestMod;				// add edi, DestMod
												// dec ecx
			}								// jnz LpY_Gray
*/
#endif
			break;
		};

	case IMAGE_FORMAT_I8A8 :
		{
			uint8 *npSrc0 = (uint8 *)pS;
			uint8 *npSrc1 = (uint8 *)(pS + SrcMod);
			uint8 *npDst = (uint8 *)pD;

			for( uint32 y=0 ; y < (h >> 1) ; y++ )
			{
				for( uint32 x=0 ; x < (w >> 1) ; x++ )
				{
					int xs = x << 2;
					int i = (npSrc0[xs] + npSrc0[xs+2] + npSrc1[xs] + npSrc1[xs+2]) >> 2;
					int a = (npSrc0[xs+1] + npSrc0[xs+3] + npSrc1[xs+1] + npSrc1[xs+3]) >> 2;
					npDst[x*2] = i;
					npDst[x*2+1] = a;
				}

				npSrc0 += SrcMod;
				npSrc0 += SrcMod;
				npSrc1 += SrcMod;
				npSrc1 += SrcMod;
				npDst += DestMod;
			}
			break;
		};

	case IMAGE_FORMAT_RGB32_F :
		{
			for(int y = 0; y < (h>>1); y++)
				for(int x = 0; x < (w>>1); x++)
				{
					int sx = x+x;
					int sy = y+y;
					CVec3Dfp32 v = *(CVec3Dfp32*)&pS[sx*12 + sy*SrcMod];
					v += *(CVec3Dfp32*)&pS[(sx+1)*12 + sy*SrcMod];
					v += *(CVec3Dfp32*)&pS[sx*12 + (sy+1)*SrcMod];
					v += *(CVec3Dfp32*)&pS[(sx+1)*12 + (sy+1)*SrcMod];
					v.Scale(0.25f, *(CVec3Dfp32*)&pD[x*12 + y*DestMod]);
				}
			break;
		}
	default :
		Error_static("CImage::StretchHalf", CStrF("Unsupported format: %s", GetFormatName(SrcFmt)));
	};
	MUnlock(pDest);
	MUnlock(pSrc);
};

// -------------------------------------------------------------------
void CImage::StretchHalfX(CImage* pSrc, CImage* pDest)
{
	MAUTOSTRIP(CImage_StretchHalfX, MAUTOSTRIP_VOID);

	if (!(pSrc->m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!(pDest->m_Memmodel & IMAGE_MEM_LOCKABLE)) return;

	int SrcFmt = pSrc->GetFormat();
	int DestFmt = pDest->GetFormat();
	if (SrcFmt != DestFmt) Error_static("CImage::StretchHalfX", "Does not support color convertions.");

	int w = pSrc->GetWidth();
	int h = pSrc->GetHeight();
	if( (pDest->GetWidth()*2 != w) || (pDest->GetHeight() != h))
	{
		StretchHalfXNonPow2(pSrc, pDest);
		return;
	}

	uint8* pS;
	uint8* pD;
	MLock(pSrc, pS);
	MLock(pDest, pD);
	int SrcMod = pSrc->GetModulo();
	int DestMod = pDest->GetModulo();
	switch(SrcFmt)
	{
	case IMAGE_FORMAT_CLUT8 :
		{
			uint8* pMixTab = (uint8*) pSrc->GetPalette()->GetAcceleratorTable(SrcFmt, DestFmt, PAL_ACCEL_MIX50, pDest->GetPalette());
#ifdef CPU_INTELP5
			__asm
			{
				mov edx, pMixTab
				mov esi, pS
				mov edi, pD
				mov ecx, h
LpY_8:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
				xor ebx, ebx
				xor eax, eax
LpX_8:
					mov bx, [esi]
					add esi, 2
					mov al, [ebx + edx]
					dec ecx
					mov [edi], al
					lea edi, [edi+1]
					jnz LpX_8

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_8
			}
#else
			//uint16 Col;
			uint16 *npSrc = (uint16 *)pS;
			uint8 *npDest = (uint8 *)pD;
			uint32 x, y;
			for( y = 0 ; y<h ; y++ )
			{
				uint16 *npTSrc = npSrc;
				uint8 *npTDest = npDest;
				for( x = 0 ; x < w>>1 ; x++ )
				{
					*npTDest = pMixTab[ *npTSrc ];
					npTDest++;
					npTSrc++;
				}
				npSrc += SrcMod >> 1;
				npDest += DestMod;
			}

#endif
			break;
		}

	case IMAGE_FORMAT_B5G6R5 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
LpY_565:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
LpX_565:
					xor eax, eax
					xor ebx, ebx
					push ecx
					mov ax, [esi]
					mov bx, [esi+2]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7bcf
					and ebx, 0x7bcf
					add eax, ebx

					mov [edi], ax
					pop ecx
					add esi, 4
					add edi, 2
					dec ecx
					jnz LpX_565

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_565
			};
#else
			uint16 *npSrc = (uint16 *)pS;
			uint16 *npDest = (uint16 *)pD;
			uint16 c1, c2;
			uint32 x, y;
			for( y = 0 ; y < h ; y++ )
			{
				uint16 *npTSrc = npSrc;
				uint16 *npTDest = npDest;
				for( x = 0 ; x < w>>1 ; x++ )
				{
					c1 = *npTSrc;
					c2 = *(npTSrc+1);
					c1 >>= 1;
					c2 >>= 1;
					c1 &= 0x7bcf;
					c2 &= 0x7bcf;
					*npTDest = (c1 + c2);

					npTSrc += 2;
					npTDest++;
				}
				npSrc += SrcMod>>1;
				npDest += DestMod>>1;
			}
#endif
			break;
		};
	case IMAGE_FORMAT_BGR8 :
	case IMAGE_FORMAT_RGB8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
LpY_24:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
LpX_24:
					push ecx
					mov eax, [esi]
					mov ebx, [esi+3]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7f7f7f7f
					and ebx, 0x7f7f7f7f
					add eax, ebx

					mov [edi], eax
					pop ecx
					add esi, 6
					add edi, 3
					dec ecx
					jnz LpX_24

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_24
			};
#else
			uint8 *npSrc = (uint8 *)pS;
			uint8 *npDest = (uint8 *)pD;
			uint32 x, y, c1, c2;
			for( y = 0 ; y < h ; y++ )
			{
				uint8 *npTSrc = npSrc;
				uint8 *npTDest = npDest;
				for( x = 0 ; x < w>>1 ; x++ )
				{
					c1 = *((uint32 *)(&npTSrc));
					c2 = *((uint32 *)(&npTSrc[3]));
					c1 >>= 1;
					c2 >>= 1;
					c1 &= 0x7f7f7f7f;
					c2 &= 0x7f7f7f7f;
					*((uint32 *)(&npTDest)) = (c1 + c2);

					npTSrc += 6;
					npTDest += 3;
				}
				npSrc += SrcMod;
				npDest += DestMod;
			}
#endif
			break;
		};
	case IMAGE_FORMAT_BGRX8 :
	case IMAGE_FORMAT_BGRA8 :
	case IMAGE_FORMAT_RGBA8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
LpY_32:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
LpX_32:
					push ecx
					mov eax, [esi]
					mov ebx, [esi+4]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7f7f7f7f
					and ebx, 0x7f7f7f7f
					add eax, ebx

					mov [edi], eax
					pop ecx
					add esi, 8
					add edi, 4
					dec ecx
					jnz LpX_32

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_32
			};
#else
			uint32 c1, c2, *npSrc = (uint32 *)pS;
			uint32 *npDest = (uint32 *)pD;
			uint32 x, y;
			for( y = 0 ; y < h ; y++ )
			{
				uint32 *npTSrc = npSrc;
				uint32 *npTDest = npDest;
				for( x = 0 ; x < w>>1 ; x++ )
				{
					c1 = *npTSrc;
					c2 = *(npTSrc+1);
					c1 >>= 1;
					c2 >>= 1;
					c1 &= 0x7f7f7f7f;
					c2 &= 0x7f7f7f7f;
					*npTDest = (c1 + c2);

					npTDest++;
					npTSrc += 2;
				}
				npSrc += SrcMod>>2;
				npDest += DestMod>>2;
			}
#endif
			break;
		};

	case IMAGE_FORMAT_A8 :
	case IMAGE_FORMAT_I8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
LpY_Gray:
				push ecx
				mov ecx, w
				shr ecx, 1
				push esi
				push edi
LpX_Gray:
					push ecx

						xor eax, eax
						movzx eax, word ptr [esi]
						movzx ecx, al
						shr eax, 8
						add ecx, eax
						shr ecx, 1

					mov [edi], cl
					pop ecx
					add esi, 2
					add edi, 1
					dec ecx
					jnz LpX_Gray

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_Gray
			};
#else
			uint8 *npSrc = (uint8 *)pS;
			uint8 *npDest = (uint8 *)pD;
			uint16 c1, c2;
			uint32 x, y;
			for( y = 0 ; y < h ; y++ )
			{
				uint16 *npTSrc = (uint16 *)npSrc;
				uint8 *npTDest = (uint8 *)npDest;
				for( x = 0 ; x < w>>1 ; x++ )
				{
					c1 = *npTSrc;
					c2 = (c1 & 0xff) + (c1 >> 8);
					c2 >>= 1;
					*npTDest = c2 & 0xff;

					npTSrc++;
					npTDest++;
				}
				npSrc += SrcMod;
				npDest += DestMod;
			}
#endif
			break;
		};
	case IMAGE_FORMAT_I8A8 :
		{
			uint8 *npSrc0 = (uint8 *)pS;
			uint8 *npDst = (uint8 *)pD;

			for( uint32 y=0 ; y < h; y++ )
			{
				for( uint32 x=0 ; x < (w >> 1) ; x++ )
				{
					int xs = x << 2;
					int i = (npSrc0[xs] + npSrc0[xs+2]) >> 1;
					int a = (npSrc0[xs+1] + npSrc0[xs+3]) >> 1;
					npDst[x*2] = i;
					npDst[x*2+1] = a;
				}

				npSrc0 += SrcMod;
				npDst += DestMod;
			}
			break;
		};

	default :
		Error_static("CImage::StretchHalfX", CStrF("Unsupported format: %s", GetFormatName(SrcFmt)));
	};
	MUnlock(pDest);
	MUnlock(pSrc);
};

// -------------------------------------------------------------------
void CImage::StretchHalfY(CImage* pSrc, CImage* pDest)
{
	MAUTOSTRIP(CImage_StretchHalfY, MAUTOSTRIP_VOID);

	if (!(pSrc->m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!(pDest->m_Memmodel & IMAGE_MEM_LOCKABLE)) return;

	int SrcFmt = pSrc->GetFormat();
	int DestFmt = pDest->GetFormat();
	if (SrcFmt != DestFmt) Error_static("CImage::StretchHalf", "Does not support color convertions.");

	int w = pSrc->GetWidth();
	int h = pSrc->GetHeight();
	if ((pDest->GetWidth() != w) || (pDest->GetHeight()*2 != h))
	{
		StretchHalfYNonPow2(pSrc, pDest);
		return;
	}

	uint8* pS;
	uint8* pD;
	MLock(pSrc, pS);
MLock(pDest, pD);
	int SrcMod = pSrc->GetModulo();
	int DestMod = pDest->GetModulo();
	switch(SrcFmt)
	{
	case IMAGE_FORMAT_CLUT8 :
		{
			uint8* pMixTab = (uint8*) pSrc->GetPalette()->GetAcceleratorTable(SrcFmt, DestFmt, PAL_ACCEL_MIX50, pDest->GetPalette());
#ifdef CPU_INTELP5
			__asm
			{
				mov edx, pMixTab
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_8:
				push ecx
				mov ecx, w
				push esi
				push edi
				xor ebx, ebx
				xor eax, eax
				push ebp
				mov ebp, SrcMod
LpX_8:
					mov bl, [esi]
					mov bh, [esi + ebp]
					mov al, [ebx + edx]

					add esi, 1
					mov [edi], al
					dec ecx
					lea edi, [edi+1]
					jnz LpX_8

				pop ebp
				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_8
			}
#else
			uint8 *npSrc = (uint8 *)pS;
			uint8 *npDest = (uint8 *)pD;
			uint8 *npTSrc1, *npTSrc2, *npTDest;
			uint16 c;
			uint32 x, y;
			for( y = 0 ; y < h>>1 ; y++ )
			{
				npTSrc1 = npSrc;
				npTSrc2 = npSrc + SrcMod;
				npTDest = npDest;
				for( x = 0 ; x < w ; x++ )
				{
					c = (*npTSrc1) + ((*npTSrc2) << 8);
					*npTDest = *(pMixTab + c);

					npTSrc1++;
					npTSrc2++;
					npTDest++;
				}
				npSrc += SrcMod<<1;
				npDest += DestMod;
			}
#endif
			break;
		}

	case IMAGE_FORMAT_B5G6R5 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_565:
				push ecx
				mov ecx, w
				push esi
				push edi
				mov edx, SrcMod
LpX_565:
					xor eax, eax
					xor ebx, ebx
					push ecx
					mov ax, [esi]
					mov bx, [esi+edx]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7bcf
					and ebx, 0x7bcf
					add eax, ebx

					mov [edi], ax
					pop ecx
					add esi, 2
					add edi, 2
					dec ecx
					jnz LpX_565

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_565
			};
#else
			uint8 *npSrc = pS;
			uint8 *npDest = pD;
			uint16 *npTSrc1, *npTSrc2, *npTDest;
			uint32 c1, c2;
			uint32 x, y;
			for( y = 0 ; y < h>>1 ; y++ )
			{
				npTSrc1 = (uint16 *)npSrc;
				npTSrc2 = (uint16 *)(npSrc + SrcMod);
				npTDest = (uint16 *)npDest;
				for( x = 0 ; x < w ; x++ )
				{
					c1 = *npTSrc1;
					c2 = *npTSrc2;
					c1 >>= 1;
					c2 >>= 1;
					c1 &= 0x7bcf;
					c2 &= 0x7bcf;
					*npTDest = (c1 + c2);

					npTSrc1++;
					npTSrc2++;
					npTDest++;
				}
				npSrc += SrcMod<<1;
				npDest += DestMod;
			}
#endif
			break;
		};
	case IMAGE_FORMAT_BGR8 :
	case IMAGE_FORMAT_RGB8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_24:
				push ecx
				mov ecx, w
				push esi
				push edi
				mov edx, SrcMod
LpX_24:
					push ecx
					mov eax, [esi]
					mov ebx, [esi+edx]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7f7f7f7f
					and ebx, 0x7f7f7f7f
					add eax, ebx

					mov [edi], eax
					pop ecx
					add esi, 3
					add edi, 3
					dec ecx
					jnz LpX_24

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_24
			};
#else
			uint8 *npSrc = pS;
			uint8 *npDest = pD;
			uint8 *npTSrc1, *npTSrc2, *npTDest;
			uint32 x, y;
			uint32 c1, c2;
			for( y = 0 ; y < h>>1 ; y++ )
			{
				npTSrc1 = npSrc;
				npTSrc2 = npSrc + SrcMod;
				npTDest = npDest;
				for( x = 0 ; x < w ; x++ )
				{
					c1 = *((uint32 *)(&npTSrc1));
					c2 = *((uint32 *)(&npTSrc2));
					c1 >>= 1;
					c2 >>= 1;
					c1 &= 0x7f7f7f7f;
					c2 &= 0x7f7f7f7f;
					*npTDest = (c1 + c2);

					npTDest += 3;
					npTSrc1 += 3;
					npTSrc2 += 3;
				}
				npSrc += SrcMod<<1;
				npDest += DestMod;
			}
#endif
			break;
		};
	case IMAGE_FORMAT_BGRX8 :
	case IMAGE_FORMAT_BGRA8 :
	case IMAGE_FORMAT_RGBA8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_32:
				push ecx
				mov ecx, w
				push esi
				push edi
				mov edx, SrcMod
LpX_32:
					push ecx
					mov eax, [esi]
					mov ebx, [esi+edx]
					shr eax, 1
					shr ebx, 1
					and eax, 0x7f7f7f7f
					and ebx, 0x7f7f7f7f
					add eax, ebx

					mov [edi], eax
					pop ecx
					add esi, 4
					add edi, 4
					dec ecx
					jnz LpX_32

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_32
			};
#else
			uint8 *npSrc = pS;
			uint8 *npDest = pD;
			uint32 *npTSrc1, *npTSrc2, *npTDest;
			uint32 x, y, c1, c2;
			for( y = 0 ; y < h>>1 ; y++ )
			{
				npTSrc1 = (uint32 *)npSrc;
				npTSrc2 = (uint32 *)(npSrc + SrcMod);
				npTDest = (uint32 *)npDest;
				for( x = 0 ; x < w ; x++ )
				{
					c1 = *npTSrc1;
					c2 = *npTSrc2;
					c1 >>= 1;
					c2 >>= 1;
					c1 &= 0x7f7f7f7f;
					c2 &= 0x7f7f7f7f;
					*npTDest = (c1 + c2);

					npTDest++;
					npTSrc1++;
					npTSrc2++;
				}
				npSrc += SrcMod<<1;
				npDest += DestMod;
			}
#endif
			break;
		};

	case IMAGE_FORMAT_A8 :
	case IMAGE_FORMAT_I8 :
		{
#ifdef CPU_INTELP5
			__asm
			{
				mov esi, pS
				mov edi, pD
				mov ecx, h
				shr ecx, 1
LpY_Gray:
				push ecx
				mov ecx, w
				push esi
				push edi
				mov edx, SrcMod
LpX_Gray:
					push ecx

						xor eax, eax
						movzx eax, byte ptr [esi]
						movzx ecx, byte ptr [esi + edx]
						add ecx, eax
						shr ecx, 1

					mov [edi], cl
					pop ecx
					add esi, 1
					add edi, 1
					dec ecx
					jnz LpX_Gray

				pop edi
				pop esi
				pop ecx
				add esi, SrcMod
				add esi, SrcMod
				add edi, DestMod
				dec ecx
				jnz LpY_Gray
			};
#else
			uint8 *npSrc = pS;
			uint8 *npDest = pD;
			uint8 *npTSrc1, *npTSrc2, *npTDest;
			uint32 x, y, c;
			for( y = 0 ; y < h>>1 ; y++ )
			{
				npTSrc1 = npSrc;
				npTSrc2 = npSrc + SrcMod;
				npTDest = npDest;

				for( x = 0 ; x < w ; x++ )
				{
					c = ((*npTSrc1)>>1) + ((*npTSrc2)>>1);
					*npTDest = c;

					npTDest++;
					npTSrc1++;
					npTSrc2++;
				}
				npSrc += SrcMod<<1;
				npDest += DestMod;
			}
#endif
			break;
		};

	case IMAGE_FORMAT_I8A8 :
		{
			uint8 *npSrc0 = (uint8 *)pS;
			uint8 *npSrc1 = (uint8 *)(pS + SrcMod);
			uint8 *npDst = (uint8 *)pD;

			for( uint32 y=0 ; y < (h >> 1) ; y++ )
			{
				for( uint32 x=0 ; x < w ; x++ )
				{
					int xs = x << 1;
					int i = (npSrc0[xs] + npSrc1[xs]) >> 1;
					int a = (npSrc0[xs+1] + npSrc1[xs+1]) >> 1;
					npDst[x*2] = i;
					npDst[x*2+1] = a;
				}

				npSrc0 += SrcMod;
				npSrc0 += SrcMod;
				npSrc1 += SrcMod;
				npSrc1 += SrcMod;
				npDst += DestMod;
			}
			break;
		};

	default :
		Error_static("CImage::StretchHalfY", CStrF("Unsupported format: %s", GetFormatName(SrcFmt)));
	};
	MUnlock(pDest);
	MUnlock(pSrc);
};

#endif

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif

// -------------------------------------------------------------------
/*
void CImage::ShrinkPow2HQ(CImage* pSrc, CImage* pDest, int _Pow)
{
	int w = pSrc->GetWidth();
	int h = pSrc->GetHeight();
	if (pDest->GetWidth()*2 != w) Error_static("CImage::ShrinkPow2HQ", "Invalid target width.");
	if (pDest->GetHeight()*2 != h) Error_static("CImage::ShrinkPow2HQ", "Invalid target height.");

	int SrcFmt = pSrc->GetFormat();
	int DestFmt = pDest->GetFormat();

	uint8* pS;
	uint8* pD;
	MLock(pSrc, pS);
	MLock(pDest, pD);
	int SrcMod = pSrc->GetModulo();
	int DestMod = pDest->GetModulo();
	switch(SrcFmt)
	{
	case IMAGE_FORMAT_CLUT8 :

	};
	MUnlock(pDest);
	MUnlock(pSrc);
}
*/
// -------------------------------------------------------------------
inline void CImage::Internal_SetPixel(int x, int y, int color, uint8* pSurfMem)
{
	MAUTOSTRIP(CImage_Internal_SetPixel, MAUTOSTRIP_VOID);

	switch(m_Pixelsize)
	{
	case 1 : pSurfMem[y*m_Modulo + x*m_Pixelsize] = color; return;
	case 2 : *((uint16*) &pSurfMem[y*m_Modulo + x*2]) = color; return;
	case 3 : 
			{
				pSurfMem[y*m_Modulo + x*3 + 0] = color;
				pSurfMem[y*m_Modulo + x*3 + 1] = color >> 8;
				pSurfMem[y*m_Modulo + x*3 + 2] = color >> 16;
/*				CPixel32* pPixel = ((CPixel32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]);
				pPixel->p16.w[0] = color;
				pPixel->p16.p8.b[2] = color >> 16;*/
				return;
			};
	case 4 : *((uint32*) &pSurfMem[y*m_Modulo + x*4]) = color; return;
	}

/*	switch (m_Format) {
		
		case IMAGE_FORMAT_BGR5 :
			{
				*((uint16*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]) = color; 
				break;
			};
		case IMAGE_FORMAT_B5G6R5 :
			{
				*((uint16*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]) = color; 
				break;
			};
		case IMAGE_FORMAT_BGR8 : 
			{
				CPixel32* pPixel = ((CPixel32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]);
				pPixel->p16.w[0] = color;
				pPixel->p16.p8.b[2] = color >> 16;
				break;
			};

		case IMAGE_FORMAT_BGRX8 : *((int32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]) = color; break;
		case IMAGE_FORMAT_BGRA8 : *((int32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]) = color; break;

		case IMAGE_FORMAT_ZBUFFER8 : *((uint8*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]) = color; break;
		case IMAGE_FORMAT_ZBUFFER16 : *((uint16*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]) = color; break; 
		case IMAGE_FORMAT_ZBUFFER32 : *((uint32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]) = color; break;

		case IMAGE_FORMAT_CLUT8 : pSurfMem[y*m_Modulo + x*m_Pixelsize] = color; break;
		case IMAGE_FORMAT_I8 : pSurfMem[y*m_Modulo + x*m_Pixelsize] = color; break;

		default : pSurfMem[y*m_Modulo + x*m_Pixelsize] = 0x0ff;
	};*/
};

// -------------------------------------------------------------------
inline int CImage::Internal_GetPixel(int x, int y, uint8* pSurfMem)
{
	MAUTOSTRIP(CImage_Internal_GetPixel, 0);

	switch(m_Pixelsize)
	{
	case 1 : return *((uint8*) &pSurfMem[y*m_Modulo + x*1]);
	case 2 : return *((uint16*) &pSurfMem[y*m_Modulo + x*2]);
	case 3 : 
			{
				return
					pSurfMem[y*m_Modulo + x*3 + 0] + 
					(pSurfMem[y*m_Modulo + x*3 + 1] << 8) +
					(pSurfMem[y*m_Modulo + x*3 + 2] << 16);
			}
//	case 3 : return *((uint32*) &pSurfMem[y*m_Modulo + x*3]) & 0xffffff;
	case 4 : return *((uint32*) &pSurfMem[y*m_Modulo + x*4]);
	};
	return 0;

/*	switch (m_Format) {
		
		case IMAGE_FORMAT_BGR5 : return *((uint16*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]); 
		case IMAGE_FORMAT_B5G6R5 : return *((uint16*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]); 
		case IMAGE_FORMAT_BGR8 :  return *((int32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]);
		case IMAGE_FORMAT_BGRX8 : return *((int32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]);
		case IMAGE_FORMAT_BGRA8 : return *((int32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]);

		case IMAGE_FORMAT_ZBUFFER8 : return *((uint8*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]);
		case IMAGE_FORMAT_ZBUFFER16 : return *((uint16*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]); 
		case IMAGE_FORMAT_ZBUFFER32 : return *((uint32*) &pSurfMem[y*m_Modulo + x*m_Pixelsize]);

		case IMAGE_FORMAT_CLUT8 : return pSurfMem[y*m_Modulo + x*m_Pixelsize];
		case IMAGE_FORMAT_I8 : return pSurfMem[y*m_Modulo + x*m_Pixelsize];

		default : return pSurfMem[y*m_Modulo + x*m_Pixelsize];
	};*/
};

// -------------------------------------------------------------------
void CImage::SetPixel(const CClipRect& cr, CPnt p, CPixel32 _Color)
{
	MAUTOSTRIP(CImage_SetPixel, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!cr.Visible(p)) return;
	p += cr;

	int col = ConvertColor(_Color, m_Format, m_spPalette);
	uint8* pSurfMem;
	MLock(this, pSurfMem);
	Internal_SetPixel(p.x, p.y, col, pSurfMem);
	MUnlock(this);
};

void CImage::SetPixel3f(const CClipRect& cr, CPnt p, const CVec3Dfp32& _Color)
{
	MAUTOSTRIP(CImage_SetPixel3f, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!cr.Visible(p)) return;
	p += cr;

	if (m_Format != IMAGE_FORMAT_BGRA16 &&
		m_Format != IMAGE_FORMAT_RGBA32_F &&
		m_Format != IMAGE_FORMAT_RGB32_F) return;

	uint8* pSurfMem;
	MLock(this, pSurfMem);
	if (m_Format & IMAGE_FORMAT_RGB32_F)
	{
		CVec3Dfp32* pImg = (CVec3Dfp32*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		pImg->k[0] = _Color[0];
		pImg->k[1] = _Color[1];
		pImg->k[2] = _Color[2];
	}
	else if (m_Format & IMAGE_FORMAT_RGBA32_F)
	{
		CVec4Dfp32* pImg = (CVec4Dfp32*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		pImg->k[0] = _Color[0];
		pImg->k[1] = _Color[1];
		pImg->k[2] = _Color[2];
		pImg->k[3] = 1.0f;
	}
	else if (m_Format & IMAGE_FORMAT_BGRA16)
	{
		uint16* pImg = (uint16*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		pImg[0] = RoundToInt(Clamp01(_Color[2]) * 65535.0f);
		pImg[1] = RoundToInt(Clamp01(_Color[1]) * 65535.0f);
		pImg[2] = RoundToInt(Clamp01(_Color[0]) * 65535.0f);
		pImg[3] = 65535;
	}
	MUnlock(this);
};

void CImage::SetPixel4f(const CClipRect& cr, CPnt p, const CVec4Dfp32& _Color)
{
	MAUTOSTRIP(CImage_SetPixel4f, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!cr.Visible(p)) return;
	p += cr;

	if (m_Format != IMAGE_FORMAT_BGRA16 &&
		m_Format != IMAGE_FORMAT_RGBA32_F &&
		m_Format != IMAGE_FORMAT_RGB32_F) return;

	uint8* pSurfMem;
	MLock(this, pSurfMem);
//	CVec4Dfp32* pImg = (CVec4Dfp32*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
//	*pImg = _Color;
	if (m_Format & IMAGE_FORMAT_RGB32_F)
	{
		CVec3Dfp32* pImg = (CVec3Dfp32*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		pImg->k[0] = _Color[0];
		pImg->k[1] = _Color[1];
		pImg->k[2] = _Color[2];
	}
	else if (m_Format & IMAGE_FORMAT_RGBA32_F)
	{
		CVec4Dfp32* pImg = (CVec4Dfp32*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		pImg->k[0] = _Color[0];
		pImg->k[1] = _Color[1];
		pImg->k[2] = _Color[2];
		pImg->k[3] = _Color[3];
	}
	else if (m_Format & IMAGE_FORMAT_BGRA16)
	{
		uint16* pImg = (uint16*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		pImg[0] = RoundToInt(Clamp01(_Color[2]) * 65535.0f);
		pImg[1] = RoundToInt(Clamp01(_Color[1]) * 65535.0f);
		pImg[2] = RoundToInt(Clamp01(_Color[0]) * 65535.0f);
		pImg[3] = RoundToInt(Clamp01(_Color[3]) * 65535.0f);
	}
	MUnlock(this);
};


#define FRAC(a) ((a) - M_Floor(a))

CPixel32 CImage::GetPixelUV_Nearest(const CVec2Dfp32& _UV)
{
	MAUTOSTRIP(CImage_GetPixel, 0);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return 0;

	fp32 u = FRAC(_UV[0]);
	fp32 v = FRAC(_UV[1]);
	fp32 w = GetWidth();
	fp32 h = GetHeight();
	fp32 x = u * w;
	fp32 y = v * h;
	int x0 = TruncToInt(x);
	int y0 = TruncToInt(y);
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x0 >= GetWidth())
		x0 = GetWidth()-1;
	if (y0 >= GetHeight())
		y0 = GetHeight()-1;

	int col;
	uint8* pSurfMem;
	MLock(this, pSurfMem);

	col = Internal_GetPixel(x0, y0, pSurfMem);

	MUnlock(this);
	return ConvertRAWColor(col, m_Format, m_spPalette);
}

CPixel32 CImage::GetPixelUV_Bilinear(const CVec2Dfp32& _UV, int _EdgeClamp)
{
	MAUTOSTRIP(CImage_GetPixel, 0);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return 0;

	fp32 w = GetWidth();
	fp32 h = GetHeight();
	fp32 winv = 1.0f / w;
	fp32 hinv = 1.0f / h;
	fp32 u = FRAC(_UV[0] - 0.5f*winv);
	fp32 v = FRAC(_UV[1] - 0.5f*hinv);
	fp32 x = u * w;
	fp32 y = v * h;
	int x0 = TruncToInt(x);
	int y0 = TruncToInt(y);
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x0 >= GetWidth())
		x0 = GetWidth()-1;
	if (y0 >= GetHeight())
		y0 = GetHeight()-1;

	int x1 = x0 + 1;
	int y1 = y0 + 1;
	if (x1 >= GetWidth())
	{
		if(_EdgeClamp & IMAGE_GETPIXEL_EDGECLAMP)
			x1 = GetWidth() - 1;
		else
			x1	= 0;
	}
	if (y1 >= GetHeight())
	{
		if(_EdgeClamp & IMAGE_GETPIXEL_EDGECLAMP)
			y1 = GetHeight() - 1;
		else
			y1 = 0;
	}
	fp32 xf = FRAC(x);
	fp32 yf = FRAC(y);

	CPixel32 col;
	uint8* pSurfMem;
	MLock(this, pSurfMem);

	uint32 rcol_00 = Internal_GetPixel(x0, y0, pSurfMem);
	uint32 rcol_10 = Internal_GetPixel(x1, y0, pSurfMem);
	uint32 rcol_01 = Internal_GetPixel(x0, y1, pSurfMem);
	uint32 rcol_11 = Internal_GetPixel(x1, y1, pSurfMem);
	CPixel32 col_00 = ConvertRAWColor(rcol_00, m_Format, m_spPalette);
	CPixel32 col_10 = ConvertRAWColor(rcol_10, m_Format, m_spPalette);
	CPixel32 col_01 = ConvertRAWColor(rcol_01, m_Format, m_spPalette);
	CPixel32 col_11 = ConvertRAWColor(rcol_11, m_Format, m_spPalette);

	col = CPixel32::BilinearRGBA(
		col_00, col_10, col_01, col_11, 
		RoundToInt(xf * 256.0f), RoundToInt(yf * 256.0f));

	MUnlock(this);

	return col;
}

CPixel32 CImage::GetPixel(const CClipRect& cr, const CPnt& _p)
{
	MAUTOSTRIP(CImage_GetPixel, 0);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return 0;
	if (!cr.Visible(_p)) return 0;
	CPnt p = _p;
	p += cr;

	int col;
	uint8* pSurfMem;
	MLock(this, pSurfMem);
	col = Internal_GetPixel(p.x, p.y, pSurfMem);
	MUnlock(this);
	return ConvertRAWColor(col, m_Format, m_spPalette);
}

CVec3Dfp32 CImage::GetPixel3f(const CClipRect& cr, CPnt p)
{
	MAUTOSTRIP(CImage_GetPixel3f, 0.0f);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return 0;
	if (!cr.Visible(p)) return 0;
	p += cr;

	CVec3Dfp32 Ret;
	uint8* pSurfMem;
	MLock(this, pSurfMem);
	if (m_Format & IMAGE_FORMAT_RGB32_F)
	{
		CVec3Dfp32* pImg = (CVec3Dfp32*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		Ret.k[0] = pImg->k[0];
		Ret.k[1] = pImg->k[1];
		Ret.k[2] = pImg->k[2];
	}
	else if (m_Format & IMAGE_FORMAT_RGBA32_F)
	{
		CVec4Dfp32* pImg = (CVec4Dfp32*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		Ret.k[0] = pImg->k[0];
		Ret.k[1] = pImg->k[1];
		Ret.k[2] = pImg->k[2];
	}
	else if (m_Format & IMAGE_FORMAT_BGRA16)
	{
		uint16* pImg = (uint16*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		Ret.k[0] = fp32(pImg[2]) / 65535.0f;
		Ret.k[1] = fp32(pImg[1]) / 65535.0f;
		Ret.k[2] = fp32(pImg[0]) / 65535.0f;
	}
	else
	{
		CPixel32 Color = ConvertRAWColor(Internal_GetPixel(p.x, p.y, pSurfMem), m_Format, m_spPalette);
		Ret.k[0] = Color.R() * (1.0f / 255.0f);
		Ret.k[1] = Color.G() * (1.0f / 255.0f);
		Ret.k[2] = Color.B() * (1.0f / 255.0f);
	}
	MUnlock(this);
	return Ret;
}

CVec4Dfp32 CImage::GetPixel4f(const CClipRect& cr, CPnt p)
{
	MAUTOSTRIP(CImage_GetPixel4f, 0.0f);

	CVec4Dfp32 Ret;
	Ret = 0;
	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return Ret;
	if (!cr.Visible(p)) return Ret;
	p += cr;

	uint8* pSurfMem;
	MLock(this, pSurfMem);
	if (m_Format & IMAGE_FORMAT_RGBA32_F)
	{
		Ret = *(CVec4Dfp32*)&pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
	}
	else if (m_Format & IMAGE_FORMAT_RGB32_F)
	{
		CVec3Dfp32* pImg = (CVec3Dfp32*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		Ret.k[0] = pImg->k[0];
		Ret.k[1] = pImg->k[1];
		Ret.k[2] = pImg->k[2];
		Ret.k[3] = 1.0f;
	}
	else if (m_Format & IMAGE_FORMAT_BGRA16)
	{
		uint16* pImg = (uint16*) &pSurfMem[m_Modulo*p.y + m_Pixelsize*p.x];
		Ret.k[0] = fp32(pImg[2]) / 65535.0f;
		Ret.k[1] = fp32(pImg[1]) / 65535.0f;
		Ret.k[2] = fp32(pImg[0]) / 65535.0f;
		Ret.k[3] = fp32(pImg[3]) / 65535.0f;
	}
	else
	{
		CPixel32 Color = ConvertRAWColor(Internal_GetPixel(p.x, p.y, pSurfMem), m_Format, m_spPalette);
		Ret.k[0] = Color.R() * (1.0f / 255.0f);
		Ret.k[1] = Color.G() * (1.0f / 255.0f);
		Ret.k[2] = Color.B() * (1.0f / 255.0f);
		Ret.k[3] = Color.A() * (1.0f / 255.0f);
	}
	MUnlock(this);
	return Ret;
}

// -------------------------------------------------------------------
void CImage::SetRAWPixel(const CClipRect& cr, CPnt p, uint32 _Color)
{
	MAUTOSTRIP(CImage_SetRAWPixel, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!cr.Visible(p)) return;
	p += cr;

	uint8* pSurfMem;
	MLock(this, pSurfMem);
	Internal_SetPixel(p.x, p.y, _Color, pSurfMem);
	MUnlock(this);
};

uint32 CImage::GetRAWPixel(const CClipRect& cr, CPnt p)
{
	MAUTOSTRIP(CImage_GetRAWPixel, 0);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return 0;
	if (!cr.Visible(p)) return 0;
	p += cr;

	uint32 col;
	uint8* pSurfMem;
	MLock(this, pSurfMem);
	col = Internal_GetPixel(p.x, p.y, pSurfMem);
	MUnlock(this);
	return col;
}

void CImage::FillZStencil(const CClipRect& cr, int32 color)
{
	MAUTOSTRIP(CImage_FillZStencil, MAUTOSTRIP_VOID);

	// Only works on hardware
}

// -------------------------------------------------------------------
void CImage::Fill(const CClipRect& cr, int32 color)
{
	MAUTOSTRIP(CImage_Fill, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!cr.clip.Valid()) return;
	int col = ConvertColor(color, m_Format, m_spPalette);

	int w = cr.clip.p1.x - cr.clip.p0.x;

	CPixel32 Color(color);
	uint8* pSurfMem;
	MLock(this, pSurfMem);
		int mod = GetModulo();
		int xofs = cr.clip.p0.x*m_Pixelsize;
		for (int y = cr.clip.p0.y; y < cr.clip.p1.y; y++)
		switch(m_Pixelsize)
		{
			case 1 : FillChar(&pSurfMem[mod*y + xofs], w, col); break;
			case 2 : FillW(&pSurfMem[mod*y + xofs], w, col); break;
			case 3 :
				{
					uint8* pScanLine = &pSurfMem[mod*y + xofs];
					uint8 b = color;
					uint8 g = color >> 8;
					uint8 r = color >> 16;
					for(int i = 0; i < w; i++)
					{
						*(pScanLine++) = r;
						*(pScanLine++) = g;
						*(pScanLine++) = b;
					}
					break;
				}
			case 4 : FillD(&pSurfMem[mod*y + xofs], w, col); break;
			case 12 :
				{
					// Assume IMAGE_FORMAT_RGB32_F
					fp32* pSurf = (fp32*)&pSurfMem[mod*y + xofs];
					fp32 r = Color.GetR();
					fp32 g = Color.GetG();
					fp32 b = Color.GetB();
					for( int i = 0; i < w; i++ )
					{
						*(pSurf++)	= r;
						*(pSurf++)	= g;
						*(pSurf++)	= b;
					}
					break;
				}
			case 16 :
				{
					// Assume IMAGE_FORMAT_RGBA32_F
					fp32* pSurf = (fp32*)&pSurfMem[mod*y + xofs];
					fp32 r = Color.GetR();
					fp32 g = Color.GetG();
					fp32 b = Color.GetB();
					fp32 a = Color.GetA();
					for( int i = 0; i < w; i++ )
					{
						*(pSurf++)	= r;
						*(pSurf++)	= g;
						*(pSurf++)	= b;
						*(pSurf++)	= a;
					}
					break;
				}
			default : FillChar(&pSurfMem[mod*y + xofs], w*m_Pixelsize, 0xff);
		};
	MUnlock(this);
};

// -------------------------------------------------------------------
void CImage::Rect(const CClipRect& cr,  CRct rect, int32 color)
{
	MAUTOSTRIP(CImage_Rect, MAUTOSTRIP_VOID);

//	rect += cr;
//	if (!rect.Valid()) return;
	CClipRect r = cr;
	rect += cr.ofs;
	r.clip.Bound(rect);
	Fill(r, color);
};

// -------------------------------------------------------------------
/*void CImage::Line(const CClipRect& cr, CPnt p0, CPnt p1, int32 color)
{
	// Om man känner att man har för mycket tid över, då kan man roa sig med att skapa DCs.
	
	HDC DC;
	HRESULT ddrval = mspDDS->GetObj()->GetDC(&DC);
	DDErr(ddrval, "CImage::Line");

	p0 += cr;
	p1 += cr;
	try {
		MoveToEx(DC, p0.x, p0.y, NULL);
		LineTo(DC, p1.x, p1.y);
	}
	catch(...) {
		ddrval = mspDDS->GetObj()->ReleaseDC(DC);
		DDErr(ddrval, "CImage::Line");
		throw;
	};
	
	ddrval = mspDDS->GetObj()->ReleaseDC(DC);
	DDErr(ddrval, "CImage::Line");
};*/

void CImage::Internal_Line(CPnt p0, CPnt p1, CPixel32 col, uint8* pSurfMem)
{
	MAUTOSTRIP(CImage_Internal_Line, MAUTOSTRIP_VOID);

	// See Bjarne sid 195 för alt. algoritm.

	int dx = abs(p1.x - p0.x);
	int dy = abs(p1.y - p0.y);
	int ydir,y;
	int xdir,x;
	int dec;
	int c = ConvertColor(col, m_Format, m_spPalette);

	if (dx > dy) 
	{
		if (p1.x < p0.x) 
		{
			Swap(p0.x, p1.x);
			Swap(p0.y, p1.y);
		};
		ydir = (p0.y < p1.y) ? 1: -1;
		y = p0.y;
		dec = 0;
		for (int i=p0.x; i < p1.x; i++) 
		{
//			SetPixel(NULL, i,y,color);
			if (!((i<0) | (i>=m_Width) | (y<0) | (y>=m_Height)))
				Internal_SetPixel(i, y, c, pSurfMem);
			dec += dy;
			if (dec >= dx) { dec -= dx; y += ydir; }
		};
	}
	else 
	{
		if (p1.y < p0.y) 
		{
			Swap(p0.x, p1.x);
			Swap(p0.y, p1.y);
		};
		xdir = (p0.x < p1.x) ? 1: -1;
		x = p0.x;
		dec = 0;
		for (int i=p0.y; i < p1.y; i++)
		{
//			SetPixel(NULL, x,i,color);
			if (!((x<0) | (x>=m_Width) | (i<0) | (i>=m_Height)))
				Internal_SetPixel(x, i, c, pSurfMem);
			dec += dx;
			if (dec >= dy) { dec -= dy; x += xdir; }
		};
	};
};

void CImage::Line(const CClipRect& cr, CPnt p0, CPnt p1, int32 color)
{
	MAUTOSTRIP(CImage_Line, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	
	p0 += cr;
	p1 += cr;
	if (!ClipLine(cr.clip, p0.x, p0.y, p1.x, p1.y)) return;

	uint8* pSurfMem;
	MLock(this, pSurfMem)
	Internal_Line(p0, p1, color, pSurfMem);
	MUnlock(this);
};



/*
void CImage::Line(const CClipRect& cr, CI_LineContainer& LC)
{
	// Totalt värdelös lösning...
	HDC DC;
	HRESULT ddrval = mspDDS->GetObj()->GetDC(&DC);
	try
	{
		HANDLE HPEN = CreatePen(PS_SOLID, 1, 0xffff00);
		SelectObject(DC, HPEN);

		int nl = LC.LineList.Len();
		for (int l = 0; l < nl; l++)
		{
			CPnt p0(LC.LineList[l].p0);
			CPnt p1(LC.LineList[l].p1);
			p0 += cr;
			p1 += cr;
			if (ClipLine(cr.clip, p0.x, p0.y, p1.x, p1.y))
			{
				MoveToEx(DC, p0.x, p0.y, NULL);
				LineTo(DC, p1.x, p1.y);
			};
		};
		DeleteObject(HPEN);
	}
	catch(...)
	{
		ddrval = mspDDS->GetObj()->ReleaseDC(DC);
		DDErr(ddrval, "Line");
		throw;
	};
	ddrval = mspDDS->GetObj()->ReleaseDC(DC);
	DDErr(ddrval, "Line");
};*/

// -------------------------------------------------------------------
void CImage::Frame(const CClipRect& cr, CRct rect, int32 color)
{
	MAUTOSTRIP(CImage_Frame, MAUTOSTRIP_VOID);

	Line(cr, rect.p0, CPnt(rect.p1.x, rect.p0.y), color);
	Line(cr, CPnt(rect.p0.x, rect.p1.y-1), CPnt(rect.p1.x, rect.p1.y-1), color);
	Line(cr, rect.p0, CPnt(rect.p0.x, rect.p1.y), color);
	Line(cr, CPnt(rect.p1.x-1, rect.p0.y), CPnt(rect.p1.x-1, rect.p1.y), color);
}

// -------------------------------------------------------------------
void CImage::Blt(const CClipRect& cr, CImage& src, int _flags, CPnt destp, int _EffectValue)
{
	MAUTOSTRIP(CImage_Blt, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE))
		Error("Blt", "Destination image cannot be locked.");

	CRct srcrect(0, 0, src.GetWidth(), src.GetHeight());
	CRct destrect = srcrect;
	destrect += destp;
	if (!cr.Visible(destrect)) return;
	destrect += cr.ofs;
	CPnt dp0 = destrect.p0;
	destrect.Bound(cr.clip);
	srcrect.p0 = srcrect.p0 + (destrect.p0 - dp0);
	srcrect.p1 = srcrect.p0 + (destrect.p1 - destrect.p0);
	if (!destrect.Valid()) return;

	int h = srcrect.p1.y - srcrect.p0.y;
	int w = srcrect.p1.x - srcrect.p0.x;

	if (!(src.GetMemModel() & IMAGE_MEM_LOCKABLE))
	{
		if (m_Format != src.GetFormat())
			Error("Blt", "Format convertion for unlockable-BLT not supported.");
		uint8* pDestMem;
		MLock(this, pDestMem);
		int DestMod = GetModulo();

		for(int y = 0; y < h; y++)
		{
			src.GetRAWData(CPnt(srcrect.p0.x, (y + srcrect.p0.y)), w*m_Pixelsize, 
				&pDestMem[(y + destrect.p0.y)*DestMod + destrect.p0.x*m_Pixelsize]);
		}

		MUnlock(this);
		return;
	}

	uint8* pSrcMem;
	uint8* pDestMem;
	MLock(this, pDestMem);
	MLock((&src), pSrcMem);

		int SrcMod = src.GetModulo();
		int DestMod = GetModulo();

		if (GetFormat() != src.GetFormat())
		{
		//	CPixel32* pPal;
		//	if (src.m_Format == IMAGE_FORMAT_CLUT8) src.GetPalette()->GetPalettePtr();
			CPixel32 Pal[256];
			if (src.m_Format == IMAGE_FORMAT_CLUT8) src.GetPalette()->GetPalette((CPixel32*) &Pal, 0, 256);

			int ConvertType = (GetFormat() & IMAGE_FORMAT_ALPHA) ? IMAGE_CONVERT_RGBA : IMAGE_CONVERT_RGB;
			for (int y = 0; y < h; y++)
				CImage::ConvertPixelArray(
					&pSrcMem[(y + srcrect.p0.y)*SrcMod + srcrect.p0.x*src.m_Pixelsize],
					&pDestMem[(y + destrect.p0.y)*DestMod + destrect.p0.x*m_Pixelsize],
					src.m_Format, m_Format, w, (CPixel32*) &Pal, ConvertType);
		}
		else
		{
			switch(_flags)
			{
			case 0 :
				{
					for (int y = 0; y < h; y++)
						_Move(&pSrcMem[(y + srcrect.p0.y)*SrcMod + srcrect.p0.x*m_Pixelsize],
							 &pDestMem[(y + destrect.p0.y)*DestMod + destrect.p0.x*m_Pixelsize], w*m_Pixelsize);
					break;
				}
			default :
				{
					CPixel32 Pal[256];
					if (src.m_Format == IMAGE_FORMAT_CLUT8) src.GetPalette()->GetPalette((CPixel32*) &Pal, 0, 256);

					for (int y = 0; y < h; y++)
						CImage::ProcessPixelArray(_flags, _EffectValue, 0, 
							&pSrcMem[(y + srcrect.p0.y)*SrcMod + srcrect.p0.x*src.m_Pixelsize],
							&pDestMem[(y + destrect.p0.y)*DestMod + destrect.p0.x*m_Pixelsize],
							src.m_Format, m_Format, w, (CPixel32*) &Pal);
					break;
				}
			}
		};
	MUnlock((&src));
	MUnlock(this);
}

void CImage::BltStretch(const CClipRect& cr, CImage& _Src, int _Flags, CVec2Dfp32 _Dest, CVec2Dfp32 _Scale, int _EffectValue, CRct* _pRetDestRect)
{
	MAUTOSTRIP(CImage_BltStretch, MAUTOSTRIP_VOID);

	// _Scale == The size of a source pixel in screen pixels.

	if (_pRetDestRect) *_pRetDestRect = CRct(0,0,0,0);
	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (!(_Src.m_Memmodel & IMAGE_MEM_LOCKABLE)) return;

	CVec2Dfp32 SrcPos(0);
	_Dest.k[0] += cr.ofs.x;
	_Dest.k[1] += cr.ofs.y;
	fp32 xFrac = _Dest.k[0] - M_Floor(_Dest.k[0]);
	fp32 yFrac = _Dest.k[1] - M_Floor(_Dest.k[1]);
	_Dest.k[0] = M_Floor(_Dest.k[0]);
	_Dest.k[1] = M_Floor(_Dest.k[1]);

	if ((_Scale.k[0] == 0) || (_Scale.k[1] == 0)) return;
	CVec2Dfp32 ScaleInv(1.0f/_Scale.k[0], 1.0f/_Scale.k[1]);

	SrcPos.k[0] -= xFrac * ScaleInv.k[0];
	SrcPos.k[1] -= yFrac * ScaleInv.k[1];

	if (_Dest.k[0] < cr.clip.p0.x)
	{
		SrcPos.k[0] += ((fp32)cr.clip.p0.x - _Dest.k[0]) * ScaleInv.k[0];
		_Dest.k[0] = cr.clip.p0.x;
	}

	if (_Dest.k[1] < cr.clip.p0.y)
	{
		SrcPos.k[1] += ((fp32)cr.clip.p0.y - _Dest.k[1]) * ScaleInv.k[1];
		_Dest.k[1] = cr.clip.p0.y;
	}

	int clipw = cr.clip.p1.x - cr.clip.p0.x;
	int cliph = cr.clip.p1.y - cr.clip.p0.y;
	int w = Min((int)(((fp32)_Src.GetWidth() - SrcPos.k[0]) * ScaleInv.k[0]) + 1, clipw);
	int h = Min((int)(((fp32)_Src.GetHeight() - SrcPos.k[1]) * ScaleInv.k[1]) + 1, cliph);
	w = Min(GetWidth() - int(_Dest.k[0]), w);
	h = Min(GetHeight() - int(_Dest.k[1]), h);
	if ((w <= 0) || (h <= 0)) return;
	if (_pRetDestRect) *_pRetDestRect = CRct((int)_Dest.k[0], (int)_Dest.k[1], (int)(_Dest.k[0] + w), (int)(_Dest.k[1] + h));

	uint8* pSrcMem;
	uint8* pDestMem;
	MLock(this, pDestMem);
	MLock((&_Src), pSrcMem);

		int SrcMod = _Src.GetModulo();
		int DestMod = GetModulo();

		switch(_Flags)
		{
		case 0 :
			{
				Error("BltStretch", "Copy blt not supported.");
/*				for (int y = 0; y < h; y++)
					_Move(&pSrcMem[(y + srcrect.p0.y)*SrcMod + srcrect.p0.x*m_Pixelsize],
						 &pDestMem[(y + destrect.p0.y)*DestMod + destrect.p0.x*m_Pixelsize], w*m_Pixelsize);
				break;*/
			}
		case IMAGE_OPER_MULADDSATURATE :
			{
				CPixel32 Pal[256];
				if (_Src.m_Format == IMAGE_FORMAT_CLUT8) _Src.GetPalette()->GetPalette((CPixel32*) &Pal, 0, 256);

				int SrcW = _Src.GetWidth();
				int SrcH = _Src.GetHeight();

//				LogFile(CStrF("BltStretch %.2f, %.2f, %d, %d, %.2f, %.2f, SInv %.2f", 
//					_Dest.k[0], _Dest.k[1], w, h, SrcPos.k[0], SrcPos.k[1], ScaleInv.k[0]));

				fp32 SrcY = SrcPos.k[1];
				for (int y = 0; y < h; y++)
				{
					int sy = (int)SrcY;
					if ((sy >= 0) && (sy < SrcH))
						CImage::ProcessPixelArray_Stretch(IMAGE_OPER_MULADDSATURATE, _EffectValue, 0, 
							&pSrcMem[sy * SrcMod],
							&pDestMem[(y + (int)_Dest.k[1])*DestMod + (int)_Dest.k[0]*m_Pixelsize],
							_Src.m_Format, m_Format, w, (CPixel32*) &Pal,
							SrcW, SrcPos.k[0], ScaleInv.k[0]);

					SrcY += ScaleInv.k[1];
				}
				break;
			}
		default :
			Error("BltStretch", "Invalid blt mode.");
		}

	MUnlock((&_Src));
	MUnlock(this);
}

bool CImage::CompareColor(const CClipRect& cr, CRct rect, int32 color)
{
	MAUTOSTRIP(CImage_CompareColor, false);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return TRUE;
	CRct destrect = rect;
	if (!cr.Visible(destrect)) return TRUE;
	destrect += cr.ofs;
	CPnt dp0 = destrect.p0;
	destrect.Bound(cr.clip);
	if (!destrect.Valid()) return TRUE;

	uint8* pDestMem;
	MLock(this, pDestMem);

//		int DestMod = GetModulo();
		{
//			for (int y = 0; y < h; y++)
//				CImage::ConvertPixelArray(
//					&pSrcMem[(y + srcrect.p0.y)*SrcMod + srcrect.p0.x*src.m_Pixelsize],
//					&pDestMem[(y + destrect.p0.y)*DestMod + destrect.p0.x*m_Pixelsize],
//					src.m_Format, m_Format, w, (CPixel32*) &Pal);
		}
	MUnlock(this);
	return TRUE;
}

//
// Flip Functions
//
void CImage::FlipV()
{
	MAUTOSTRIP(CImage_FlipV, MAUTOSTRIP_VOID);

	int8 *MoveBuffer = DNew(int8) int8[m_Pixelsize*m_Modulo];
	int8 *Data = (int8 *)__Lock(0);

	for(int32 y = m_Height/2; y >= 0; y--)
	{
		int8 *Pos1 = Data+y*m_Modulo;
		int8 *Pos2 = Data+(m_Height-y-1)*m_Modulo;
		memcpy(MoveBuffer, Pos1, m_Modulo);
		memcpy(Pos1, Pos2, m_Modulo);
		memcpy(Pos2, MoveBuffer, m_Modulo);
	}

	__Unlock();

	delete [] MoveBuffer;
}

void CImage::FlipH()
{
	MAUTOSTRIP(CImage_FlipH, MAUTOSTRIP_VOID);

	// FIXME:Flippaa
}

// -------------------------------------------------------------------
void CImage::Invert()
{
	MAUTOSTRIP(CImage_Invert, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;

	uint8* pTarget;
	MLock(this, pTarget);
	int Modulo = m_Modulo;
	switch(m_Format)
	{
	case IMAGE_FORMAT_CLUT8 :
	case IMAGE_FORMAT_I8 :
	case IMAGE_FORMAT_A8 :
	case IMAGE_FORMAT_BGR8 :
	case IMAGE_FORMAT_BGRX8 :
	case IMAGE_FORMAT_BGRA8 :
	case IMAGE_FORMAT_RGBA8 :
	case IMAGE_FORMAT_RGB8 :
		{
			int w = m_Width * m_Pixelsize;
			for(int y = 0; y < m_Height; y++)
				for(int x = 0; x < w; x++)
					pTarget[y * Modulo + x] = -pTarget[y * Modulo + x];
		}
		break;
	default:
		Error("Invert", CStrF("Unsupported format: %s", GetFormatName(m_Format)));
	}

	MUnlock(this);
}

// -------------------------------------------------------------------
void CImage::Internal_TextureSpan_Clut8_Copy(int SpanLen, fp32 zinv0, fp32 zinv1,
	const CVec2Dfp32& _tc0,  const CVec2Dfp32& _tc1,
	uint8* pSurfMem, uint8* pTextureMem, int Txt_Modulo, int Txt_xAnd, int Txt_yAnd)
{
	MAUTOSTRIP(CImage_Internal_TextureSpan_Clut8_Copy, MAUTOSTRIP_VOID);

	if (SpanLen <= 0) return;
	int tc0x, tc0y;
	tc0x = (int)(_tc0.k[0] * 65536.0f*512.0f);
	tc0y = (int)(_tc0.k[1] * 65536.0f*512.0f);
//	fp32 leninv = 65536.0*512.0/SpanLen;

#ifdef CPU_INTELP5
//	fp32 leninv = GetMathAccel()->FastDivFp32Int(65536.0f*512.0f, SpanLen);
	fp32 leninv = (65536.0f * 512.0f) / SpanLen;
	int iptcx = (int)((_tc1.k[0] - _tc0.k[0]) * leninv);
	int iptcy = (int)((_tc1.k[1] - _tc0.k[1]) * leninv);
	__asm
	{
		mov edi, [pSurfMem]
		mov esi, [pTextureMem]
		mov ebx, tc0x
		mov ecx, tc0y

		mov edx, SpanLen
		and edx, 3
		jz NoBytes

/*		test edx, 1

			shld edi, ecx, 7
			add ecx, [iptcy]
			shld edi, ebx, 7
			and edi, 16384-1
			add ebx, [iptcx]
			mov al, [edi+esi]*/
		

		// Do the dword stuff.
NoBytes:
		push edx
		push edi
		mov edx, SpanLen
		shr edx, 2
		jnz Lp
		jmp NoLoop

		ALIGN 16
Lp:
		// 5.12ms/16000iter, 12.5Mb/s
			shld edi, ecx, 7
			add ecx, [iptcy]
			shld edi, ebx, 7
			shld edx, ecx, 7
			and edi, 16384-1
			add ebx, [iptcx]
			mov al, [edi+esi]

			shld edx, ebx, 7
			add ecx, [iptcy]
			and edx, 16384-1
			add ebx, [iptcx]
			shld edi, ecx, 7
			mov ah, [edx+esi]

			shld edi, ebx, 7
			add ecx, [iptcy]
			rol eax,16
			and edi, 16384-1
			shld edx, ecx, 7
			add ebx, [iptcx]
			mov al, [edi+esi]

			shld edx, ebx, 7
			add ecx, [iptcy]
			and edx, 16384-1
			add ebx, [iptcx]
		pop edi
			mov ah, [edx+esi]

		add edi,4
		pop edx
		rol eax,16
		dec edx
		mov [edi-4], eax
		push edx
		push edi
		jnz Lp 

NoLoop:
		pop edi
		pop edx
	};
#else
	// BLAH! Fanken är det en vanlig memcpy eller?  *testar*
	memcpy( pSurfMem, pTextureMem, SpanLen );
#endif
};
/*
			shld edi, ebx, 7
			shld edi, ecx, 7
			add ebx, edx
			add ecx, ebp
			and edi, 16384-1
			mov al, [edi+esi]

*/
/* 4.10 OBS ingen stackframe tillgänglig.

		mov al,bh
		mov ah,ch
		add ebx,256
		rol eax,16
		add ecx,256
		mov al,bh
		mov ah,ch
		add ebx,256
		add ebp,4
		rol eax,16
		add ecx,256
		dec edx
		mov [ebp-4], eax
		jnz Lp 
// 5.16
			shld edi, ecx, 7
		push edx
			add ecx, 65536*512
			shld edi, ebx, 7
			shld edx, ecx, 7
			and edi, 16384-1
			add ebx, 65536*512
			mov al, [edi+esi]

			shld edx, ebx, 7
			add ecx, 65536*512
			and edx, 16384-1
			add ebx, 65536*512
			shld edi, ecx, 7
			mov ah, [edx+esi]

			shld edi, ebx, 7
			add ecx, 65536*512
			rol eax,16
			and edi, 16384-1
			shld edx, ecx, 7
			add ebx, 65536*512
			mov al, [edi+esi]

			shld edx, ebx, 7
			add ecx, 65536*512
			and edx, 16384-1
			add ebx, 65536*512
			mov ah, [edx+esi]

		add ebp,4
		rol eax,16
		pop edx
		mov [ebp-4], eax
		dec edx

// 6.87 OBS ingen stackframe tillgänglig.

			shld edi, ecx, 7
		push edx
			add ecx, 65536*512
			shld edi, ebx, 7
			shld edx, ecx, 7
			and edi, 8192-1
			add ebx, 65536*512
			mov ax, [edi*2+esi]

			shld edx, ebx, 7
			add ecx, 65536*512
		rol eax,16
			and edx, 8192-1
			add ebx, 65536*512
			shld edi, ecx, 7
			mov ax, [edx*2+esi]

			shld edi, ebx, 7
			add ecx, 65536*512
		rol eax,16
			and edi, 8192-1
		mov [ebp], eax
			shld edx, ecx, 7
			add ebx, 65536*512
			mov ax, [edi*2+esi]

			shld edx, ebx, 7
			add ecx, 65536*512
		rol eax,16
			and edx, 8192-1
			add ebx, 65536*512
			mov ax, [edx*2+esi]

		add ebp,8
		rol eax,16
		pop edx
		mov [ebp-4], eax
		dec edx

 */
/*	7.8		add ecx, [iptcy]
			add ebx, [iptcx]
			mov edx, ecx
			mov edi, ebx
			sar edx, 16
			sar edi, 16
			and edx, [Txt_yAnd]
			imul edx, [Txt_Modulo]
			and edi, [Txt_xAnd]
			add edx, edi
			mov al, [edx+esi]

			add ecx, [iptcy]
			add ebx, [iptcx]
			mov edx, ecx
			mov edi, ebx
			sar edx, 16
			sar edi, 16
			and edx, [Txt_yAnd]
			imul edx, [Txt_Modulo]
			and edi, [Txt_xAnd]
			add edx, edi
			mov ah, [edx+esi]

			rol eax,16

			add ecx, [iptcy]
			add ebx, [iptcx]
			mov edx, ecx
			mov edi, ebx
			sar edx, 16
			sar edi, 16
			and edx, [Txt_yAnd]
			imul edx, [Txt_Modulo]
			and edi, [Txt_xAnd]
			add edx, edi
			mov al, [edx+esi]

			add ecx, [iptcy]
			add ebx, [iptcx]
			mov edx, ecx
			mov edi, ebx
			sar edx, 16
			sar edi, 16
			and edx, [Txt_yAnd]
			imul edx, [Txt_Modulo]
			and edi, [Txt_xAnd]
			add edx, edi
			mov ah, [edx+esi]
*/

/*
void CImage::TextureTest(CImage* Texture)
{
	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	if (Texture == NULL) return;
	uint8* pSurfMem;
	uint8* pTextureMem;
	MLock(this, pSurfMem);
	MLock(Texture, pTextureMem);

	int Mod = GetModulo();
	int Txt_Mod = Texture->GetModulo();
	int Txt_xAnd = Texture->GetWidth()-1;
	int Txt_yAnd = Texture->GetHeight()-1;

	int ydest = Min(GetHeight(), 100);
	int spanlen = Min(GetWidth(), 160);
	for (int y = 0; y < ydest; y++)
	{
		Internal_TextureSpan_Clut8_Copy(spanlen, 1, 1, CVec2Dfp32(0, y), CVec2Dfp32(128, y/2),
			&pSurfMem[y*Mod], pTextureMem,
			Txt_Mod, Txt_xAnd, Txt_yAnd);
	};

	MUnlock(Texture);
	MUnlock(this);
};
*/
// -------------------------------------------------------------------
void CImage::Triangle(const CClipRect& cr, const CVec3Dfp32& _p0,  const CVec3Dfp32& _p1, const CVec3Dfp32& _p2, int32 color)
{
	MAUTOSTRIP(CImage_Triangle, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	// Single color triangle.
	if ((((_p1.k[0]-_p0.k[0]) * (_p1.k[1]-_p2.k[1])) - ((_p1.k[1]-_p0.k[1]) * (_p1.k[0]-_p2.k[0]))) > 0) return;

	int col = ConvertColor(color, m_Format, m_spPalette);
	int psize = GetPixelSize();

	CVec3Dfp32 p[3];
	fp32 ymin = _FP32_MAX;
	int yl0 = 0;
	if (_p0.k[1] < ymin) ymin = _p0.k[1];
	if (_p1.k[1] < ymin) { ymin = _p1.k[1]; yl0 = 1; };
	if (_p2.k[1] < ymin) { ymin = _p2.k[1]; yl0 = 2; };
	int yl1 = (yl0-1+3) % 3;
	int yr0 = yl0;
	int yr1 = (yr0+1) % 3;

	p[0] = _p0;
	p[1] = _p1;
	p[2] = _p2;

	bool IPl = FALSE;
	bool IPr = FALSE;
	fp32 ipxl;
	fp32 ipxr;
	fp32 xl;
	fp32 xr;

	uint8* pSurfMem;
	MLock(this, pSurfMem);

	int mod = m_Modulo;
	bool Finished = FALSE;
	while (!Finished)
	{
		fp32 dyl = p[yl1].k[1] - p[yl0].k[1];
		fp32 dyr = p[yr1].k[1] - p[yr0].k[1];

		if ((dyl > 0) && (dyr > 0) && 
			(p[yl1].k[1] > cr.clip.p0.y) && (p[yl0].k[1] < cr.clip.p1.y) &&
			(p[yr1].k[1] > cr.clip.p0.y) && (p[yr0].k[1] < cr.clip.p1.y))
		{
			int y = Max((int) M_Floor((fp32)Max((int)p[yl0].k[1], (int)p[yr0].k[1]) + 1), cr.clip.p0.y);
			IPl = FALSE;
			IPr = FALSE;
			if (!IPl)
			{
				ipxl = (p[yl1].k[0] - p[yl0].k[0]) / dyl;
				xl = p[yl0].k[0] + ipxl * (y - p[yl0].k[1]);
				IPl = TRUE;
			};
			if (!IPr)
			{
				ipxr = (p[yr1].k[0] - p[yr0].k[0]) / dyr;
				xr = p[yr0].k[0] + ipxr * (y - p[yr0].k[1]);
				IPr = TRUE;
			};
			int ystop = Min((int)M_Floor(Min(p[yl1].k[1], p[yr1].k[1]) ), cr.clip.p1.y-1);
			for (; y <= ystop; y++)
			{
				int ixl = (int)M_Floor(xl+1);
				int ixr = (int)M_Floor(xr);
				if ((ixr >= ixl) && (ixl < cr.clip.p1.x) && (ixr >= cr.clip.p0.x))
				{
					if (ixl < cr.clip.p0.x) ixl = cr.clip.p0.x;
					if (ixr >= cr.clip.p1.x) ixr = cr.clip.p1.x-1;
					int len = ixr - ixl + 1;
					switch(psize)
					{
					case 1 : FillChar(&pSurfMem[mod*y + ixl*psize], len, col); break;
					case 2 : FillW(&pSurfMem[mod*y + ixl*psize], len, col); break;
					case 4 : FillD(&pSurfMem[mod*y + ixl*psize], len, col); break;
					default : FillChar(&pSurfMem[mod*y + ixl*psize], len*psize, 0xff);
					};
//					Line(cr, CPnt(xl, y), CPnt(xr, y), color);
				};
				xl += ipxl;
				xr += ipxr;
			};
		};

		color = 0xffff00;
		// Step segment
		if (p[yl1].k[1] < p[yr1].k[1]) 
			{ yl0 = (yl0 - 1 + 3) % 3; yl1 = (yl1 - 1 + 3) % 3; IPl = FALSE; }
		else
			{ yr0 = (yr0 + 1) % 3; yr1 = (yr1 + 1) % 3; IPr = FALSE; };
		Finished = (yl0 == yr0);
	};
	
	MUnlock(this);
};

// -------------------------------------------------------------------
#define CONST_CIMAGE_PERSPECTIVESTEP 16

void CImage::Triangle(const CClipRect& cr, const CVec3Dfp32& _p0,  const CVec3Dfp32& _p1, const CVec3Dfp32& _p2,
					  CPixel32 _col0, CPixel32 _col1, CPixel32 _col2, 
					  const CVec2Dfp32 _tc0, const CVec2Dfp32 _tc1, const CVec2Dfp32 _tc2, CImage& Texture)
{
	MAUTOSTRIP(CImage_Triangle_2, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	// Single color triangle.
	if ((((_p1.k[0]-_p0.k[0]) * (_p1.k[1]-_p2.k[1])) - ((_p1.k[1]-_p0.k[1]) * (_p1.k[0]-_p2.k[0]))) > 0) return;

//	int psize = GetPixelSize();

	CVec3Dfp32 p[3];
	CVec3Dfp32 ck[3];
	CVec2Dfp32 tck[3];

	fp32 ymin = _FP32_MAX;
	int yl0 = 0;
	if (_p0.k[1] < ymin) ymin = _p0.k[1];
	if (_p1.k[1] < ymin) { ymin = _p1.k[1]; yl0 = 1; };
	if (_p2.k[1] < ymin) { ymin = _p2.k[1]; yl0 = 2; };
	int yl1 = (yl0-1+3) % 3;
	int yr0 = yl0;
	int yr1 = (yr0+1) % 3;

	p[0] = _p0;
	p[1] = _p1;
	p[2] = _p2;
	ck[0] = CVec3Dfp32(_col0.GetR(), _col0.GetG(), _col0.GetB()) * p[0].k[2];
	ck[1] = CVec3Dfp32(_col1.GetR(), _col1.GetG(), _col1.GetB()) * p[1].k[2];
	ck[2] = CVec3Dfp32(_col2.GetR(), _col2.GetG(), _col2.GetB()) * p[2].k[2];
	tck[0] = _tc0 * p[0].k[2];
	tck[1] = _tc1 * p[1].k[2];
	tck[2] = _tc2 * p[2].k[2];

	bool IPl = FALSE;
	bool IPr = FALSE;
	fp32 ipxl, ipxr;
	fp32 ipzinvl, ipzinvr;
	fp32 xl, xr, zinvl, zinvr;
	CVec2Dfp32 iptckl, iptckr, tckl, tckr;
	CVec3Dfp32 ipckl, ipckr, ckl, ckr;

	uint8* pSurfMem;
	MLock(this, pSurfMem);

//	int mod = m_Modulo;
	bool Finished = FALSE;
	while (!Finished)
	{
		fp32 dyl = p[yl1].k[1] - p[yl0].k[1];
		fp32 dyr = p[yr1].k[1] - p[yr0].k[1];

		if ((dyl > 0) && (dyr > 0) && 
			(p[yl1].k[1] > cr.clip.p0.y) && (p[yl0].k[1] < cr.clip.p1.y) &&
			(p[yr1].k[1] > cr.clip.p0.y) && (p[yr0].k[1] < cr.clip.p1.y))
		{
			int y = Max((int)M_Floor((fp32)Max((int)p[yl0].k[1], (int)p[yr0].k[1]) + 1), cr.clip.p0.y);
			IPl = FALSE;
			IPr = FALSE;
			if (!IPl)
			{
				fp32 clipdy = (y - p[yl0].k[1]);
				fp32 dyinv = 1/dyl;
				ipxl = (p[yl1].k[0] - p[yl0].k[0]) * dyinv;
				xl = p[yl0].k[0] + ipxl * clipdy;
				ipzinvl = (p[yl1].k[2] - p[yl0].k[2]) * dyinv;
				zinvl = p[yl0].k[2] + ipzinvl * clipdy;
				iptckl = (tck[yl1] - tck[yl0]) * dyinv;
				tckl = tck[yl0] + (iptckl * clipdy);
				ipckl = (ck[yl1] - ck[yl0]) * dyinv;
				ckl = ck[yl0] + (ipckl * clipdy);
				IPl = TRUE;
			};
			if (!IPr)
			{
				fp32 clipdy = (y - p[yr0].k[1]);
				fp32 dyinv = 1/dyr;
				ipxr = (p[yr1].k[0] - p[yr0].k[0]) * dyinv;
				xr = p[yr0].k[0] + ipxr * clipdy;
				ipzinvr = (p[yr1].k[2] - p[yr0].k[2]) * dyinv;
				zinvr = p[yr0].k[2] + ipzinvr * clipdy;
				iptckr = (tck[yr1] - tck[yr0]) * dyinv;
				tckr = tck[yr0] + (iptckr * clipdy);
				ipckr = (ck[yr1] - ck[yr0]) * dyinv;
				ckr = ck[yr0] + (ipckr * clipdy);
				IPr = TRUE;
			};
			int ystop = Min((int)M_Floor(Min(p[yl1].k[1], p[yr1].k[1]) ), cr.clip.p1.y-1);
			for (; y <= ystop; y++)
			{
				int ixl = (int)M_Floor(xl+1);
				int ixr = (int)M_Floor(xr);
				if ((ixr >= ixl) && (ixl < cr.clip.p1.x) && (ixr >= cr.clip.p0.x))
				{
					if (ixl < cr.clip.p0.x) ixl = cr.clip.p0.x;
					if (ixr >= cr.clip.p1.x) ixr = cr.clip.p1.x-1;
//					int len = ixr - ixl + 1;
					fp32 dx = xr-xl;
					fp32 xclip = (fp32)ixl - xl;
					if (M_Fabs(dx) <= _FP32_MIN) dx = 0.00000001f;
					fp32 dxinv = 1/dx;

					CVec3Dfp32 x_ipck = (ckr-ckl) * dxinv;
					CVec2Dfp32 x_iptck = (tckr - tckl) * dxinv;
//					fp32 x_ipzinv = (zinvr - zinvl) * dxinv;

					CVec3Dfp32 x_ck0 = ckl + (x_ipck * xclip);
					CVec2Dfp32 x_tck0 = tckl + (x_iptck * xclip);
//					fp32 x_zinv0 = zinvl + (x_ipzinv * xclip);

					int x0 = ixl;
					while (x0 < ixr)
					{
						int x1;
						fp32 dxx_inv;
						if (x0 + CONST_CIMAGE_PERSPECTIVESTEP <= ixr)
						{
							x1 = x0 + CONST_CIMAGE_PERSPECTIVESTEP;
							dxx_inv = 1.0f/(fp32)CONST_CIMAGE_PERSPECTIVESTEP;
						}
						else
						{
							dxx_inv = 1.0f/(fp32)(ixr - x0);
							x1 = ixr;
						};

//						x_
						




						x0 = x1;
					};

/*					switch(psize)
					{
					case 1 : FillChar(&pSurfMem[mod*y + ixl*psize], len, col); break;
					case 2 : FillW(&pSurfMem[mod*y + ixl*psize], len, col); break;
					case 4 : FillD(&pSurfMem[mod*y + ixl*psize], len, col); break;
					default : FillChar(&pSurfMem[mod*y + ixl*psize], len*psize, 0xff);
					};*/

//					Line(cr, CPnt(xl, y), CPnt(xr, y), color);
				};
				xl += ipxl;
				xr += ipxr;
				tckl += iptckl;
				tckr += iptckr;
				ckl += ipckl;
				ckr += ipckr;
				zinvl += ipzinvl;
				zinvr += ipzinvr;
			};
		};

		// Step segment
		if (p[yl1].k[1] < p[yr1].k[1]) 
			{ yl0 = (yl0 - 1 + 3) % 3; yl1 = (yl1 - 1 + 3) % 3; IPl = FALSE; }
		else
			{ yr0 = (yr0 + 1) % 3; yr1 = (yr1 + 1) % 3; IPr = FALSE; };
		Finished = (yl0 == yr0);
	};
	
	MUnlock(this);
};

// -------------------------------------------------------------------
//  Debug text routine
// -------------------------------------------------------------------

// 128 tecken, 8*8, 1 bpp
uint8 CImage::ms_DebugFont[128][8] = 
{
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},

    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x80,0x80,0x80,0x80,0x00,0x80,0x00,0x00},
    {0xA0,0xA0,0x00,0x00,0x00,0x00,0x00,0x00},{0x50,0xF8,0x50,0x50,0xF8,0x50,0x00,0x00},
    {0x78,0xA0,0x70,0x28,0x28,0xF0,0x00,0x00},{0x00,0x90,0x20,0x40,0x90,0x00,0x00,0x00},
    {0x60,0x50,0x20,0x54,0x88,0x74,0x00,0x00},{0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x40,0x80,0x80,0x80,0x80,0x40,0x00,0x00},{0x80,0x40,0x40,0x40,0x40,0x80,0x00,0x00},
    {0x00,0x00,0xA0,0x40,0xA0,0x00,0x00,0x00},{0x00,0x00,0x40,0xE0,0x40,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x40,0x80,0x00},{0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00},{0x04,0x08,0x10,0x20,0x40,0x80,0x00,0x00},
    {0x60,0x90,0x90,0x90,0x90,0x60,0x00,0x00},{0x40,0xC0,0x40,0x40,0x40,0xE0,0x00,0x00},
    {0x60,0x90,0x20,0x40,0x80,0xF0,0x00,0x00},{0xE0,0x10,0x70,0x10,0x10,0xE0,0x00,0x00},
    {0x90,0x90,0xF0,0x10,0x10,0x10,0x00,0x00},{0xF0,0x80,0xE0,0x10,0x10,0xE0,0x00,0x00},
    {0x70,0x80,0xE0,0x90,0x90,0x60,0x00,0x00},{0xF0,0x10,0x60,0x20,0x40,0x40,0x00,0x00},
    {0x60,0x90,0x60,0x90,0x90,0x60,0x00,0x00},{0x70,0x90,0x70,0x10,0x10,0xE0,0x00,0x00},
    {0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x00},{0x00,0x00,0x80,0x00,0x80,0x80,0x00,0x00},
    {0x00,0x20,0x40,0x80,0x40,0x20,0x00,0x00},{0x00,0x00,0xF0,0x00,0xF0,0x00,0x00,0x00},
    {0x00,0x80,0x40,0x20,0x40,0x80,0x00,0x00},{0xE0,0x10,0x20,0x40,0x00,0x40,0x00,0x00},
    {0x60,0x90,0xB0,0xB0,0x80,0x70,0x00,0x00},{0x60,0x90,0xF0,0x90,0x90,0x90,0x00,0x00},
    {0xE0,0x90,0xE0,0x90,0x90,0xE0,0x00,0x00},{0x70,0x80,0x80,0x80,0x80,0x70,0x00,0x00},
    {0xE0,0x90,0x90,0x90,0x90,0xE0,0x00,0x00},{0xF0,0x80,0xE0,0x80,0x80,0xF0,0x00,0x00},
    {0xF0,0x80,0xE0,0x80,0x80,0x80,0x00,0x00},{0x70,0x80,0xB0,0x90,0x90,0x60,0x00,0x00},
    {0x90,0x90,0xF0,0x90,0x90,0x90,0x00,0x00},{0xE0,0x40,0x40,0x40,0x40,0xE0,0x00,0x00},
    {0x70,0x10,0x10,0x10,0x10,0xE0,0x00,0x00},{0x90,0x90,0xE0,0x90,0x90,0x90,0x00,0x00},
    {0x80,0x80,0x80,0x80,0x80,0xF0,0x00,0x00},{0x88,0xD8,0xA8,0x88,0x88,0x88,0x00,0x00},
    {0x90,0xD0,0xB0,0x90,0x90,0x90,0x00,0x00},{0x60,0x90,0x90,0x90,0x90,0x60,0x00,0x00},
    {0xE0,0x90,0x90,0xE0,0x80,0x80,0x00,0x00},{0x60,0x90,0x90,0x90,0x80,0x70,0x00,0x00},
    {0xE0,0x90,0x90,0xE0,0x90,0x90,0x00,0x00},{0x70,0x80,0x60,0x10,0x10,0xE0,0x00,0x00},
    {0xF8,0x20,0x20,0x20,0x20,0x20,0x00,0x00},{0x90,0x90,0x90,0x90,0x90,0x60,0x00,0x00},
    {0x90,0x90,0x90,0x90,0x50,0x20,0x00,0x00},{0x88,0x88,0x88,0xA8,0xD8,0x88,0x00,0x00},
    {0x88,0x50,0x20,0x50,0x88,0x88,0x00,0x00},{0x90,0x90,0x70,0x10,0x10,0xE0,0x00,0x00},
    {0xF0,0x10,0x60,0x80,0x80,0xF0,0x00,0x00},{0xC0,0x80,0x80,0x80,0x80,0xC0,0x00,0x00},
    {0x80,0xC0,0x60,0x30,0x18,0x08,0x00,0x00},{0xC0,0x40,0x40,0x40,0x40,0xC0,0x00,0x00},
    {0x20,0x50,0x88,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00},
    {0x80,0x40,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x60,0x90,0x90,0x70,0x00,0x00},
    {0x80,0x80,0xE0,0x90,0x90,0xE0,0x00,0x00},{0x00,0x00,0x70,0x80,0x80,0x70,0x00,0x00},
    {0x10,0x10,0x70,0x90,0x90,0x70,0x00,0x00},{0x00,0x00,0x60,0xF0,0x80,0x70,0x00,0x00},
    {0x20,0x40,0xE0,0x40,0x40,0x40,0x00,0x00},{0x00,0x00,0x70,0x90,0x90,0x70,0x10,0xE0},
    {0x80,0x80,0xE0,0x90,0x90,0x90,0x00,0x00},{0x80,0x00,0x80,0x80,0x80,0x80,0x00,0x00},
    {0x40,0x00,0x40,0x40,0x40,0x40,0x40,0x80},{0x80,0x80,0xA0,0xC0,0xA0,0x90,0x00,0x00},
    {0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00},{0x00,0x00,0xD0,0xA8,0xA8,0xA8,0x00,0x00},
    {0x00,0x00,0xE0,0x90,0x90,0x90,0x00,0x00},{0x00,0x00,0x60,0x90,0x90,0x60,0x00,0x00},
    {0x00,0x00,0xE0,0x90,0x90,0xE0,0x80,0x80},{0x00,0x00,0x70,0x90,0x90,0x70,0x10,0x10},
    {0x00,0x00,0xB0,0xC0,0x80,0x80,0x00,0x00},{0x00,0x00,0x70,0xE0,0x10,0xF0,0x00,0x00},
    {0x00,0x40,0xE0,0x40,0x40,0x20,0x00,0x00},{0x00,0x00,0x90,0x90,0x90,0x70,0x00,0x00},
    {0x00,0x00,0x90,0x90,0x50,0x20,0x00,0x00},{0x00,0x00,0xA8,0xA8,0xA8,0x58,0x00,0x00},
    {0x00,0x00,0x90,0x60,0x90,0x90,0x00,0x00},{0x00,0x00,0x90,0x90,0x90,0x70,0x10,0xE0},
    {0x00,0x00,0xF0,0x20,0x40,0xF0,0x00,0x00},{0x60,0x40,0xC0,0x40,0x40,0x60,0x00,0x00},
    {0x00,0x80,0x80,0x00,0x80,0x80,0x00,0x00},{0xC0,0x40,0x60,0x40,0x40,0xC0,0x00,0x00},
    {0x50,0xA0,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x60,0x90,0x90,0xF0,0x00,0x00}
};

uint8 CImage::ms_DebugFontWidth[128] = 
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    0x03,0x02,0x04,0x06,0x06,0x05,0x07,0x02,0x03,0x03,0x04,0x04,0x03,0x04,0x02,0x07,
    0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x02,0x02,0x04,0x05,0x04,0x05,
    0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x04,0x05,0x05,0x05,0x06,0x05,0x05,
    0x05,0x05,0x05,0x05,0x06,0x05,0x05,0x06,0x06,0x05,0x05,0x03,0x06,0x03,0x06,0x05,
    0x03,0x05,0x05,0x05,0x05,0x05,0x04,0x05,0x05,0x02,0x03,0x05,0x02,0x06,0x05,0x05,
    0x05,0x05,0x05,0x05,0x04,0x05,0x05,0x06,0x05,0x05,0x05,0x04,0x02,0x04,0x05,0x05,
};

int CImage::DebugTextLen(const char* _pStr)
{
	MAUTOSTRIP(CImage_DebugTextLen, 0);

	int strlen = CStr::StrLen(_pStr);
	int l = 0;
	for (int i = 0; i < strlen; i++)
	{
		int c = (_pStr[i] & 127);
		l += ms_DebugFontWidth[c];
	};
	return l;
};

int CImage::DebugTextHeight()
{
	MAUTOSTRIP(CImage_DebugTextHeight, 8);

	return 8;
};

void CImage::Internal_FillMask(int mask, int color, int nPixels, uint8* pSurfMem, int psize)
{
	MAUTOSTRIP(CImage_Internal_FillMask, MAUTOSTRIP_VOID);

#ifdef CPU_INTELP5
	// Supports 8, 16, 24 & 32 bit modes.
	// Fills a scanline of maximum 32 pixels with color, bit 8000000 is the leftmost pixel.
	__asm
	{
		mov ecx, nPixels
		or ecx, ecx
		jz NoPixels
		mov edi, pSurfMem

		mov eax, color
		mov ebx, mask

		mov edx, psize
		cmp edx, 2
		je WordLp
		cmp edx, 4
		je DWordLp
		cmp edx, 3
		je TriByte
		cmp edx, 1
		jne Done

//-----------------------------------------
ByteLp:
			test ebx, 0x80000000
			jz NoByte
			mov [edi], al
NoByte:
			shl ebx, 1
			inc edi
			dec ecx
		jnz ByteLp
		jmp Done

//-----------------------------------------
WordLp:
			test ebx, 0x80000000
			jz NoWord
			mov [edi], ax
NoWord:
			shl ebx, 1
			add edi, 2
			dec ecx
		jnz WordLp
		jmp Done

//-----------------------------------------
TriByte:
			mov edx, eax
			shr edx, 16
TriByteLp:
			test ebx, 0x80000000
			jz NoTriByte
			mov [edi], ax
			mov [edi+2], dl
NoTriByte:
			shl ebx, 1
			add edi, 3
			dec ecx
		jnz TriByteLp
		jmp Done

//-----------------------------------------
DWordLp:
			test ebx, 0x80000000
			jz NoDWord
			mov [edi], eax
NoDWord:
			shl ebx, 1
			add edi, 4
			dec ecx
		jnz DWordLp
		jmp Done

//-----------------------------------------
Done:
NoPixels:
	};
#else
	for (int p = 0; p < nPixels; p++)
	{
		if ((mask & 0x80000000) != 0)
			switch (psize)
			{
			case 1 : pSurfMem[p] = color; break;
			case 2 : ((int16*)pSurfMem)[p] = color; break;
			case 3 : 
				{
					pSurfMem[p*3] = color;
					pSurfMem[p*3+1] = color >> 8;
					pSurfMem[p*3+2] = color >> 16;
					break;
				};
			case 4 : ((int32*)pSurfMem)[p] = color; break;
			default:;
			};
		mask >>= 1;
	};
#endif
};

void CImage::DebugText(const CClipRect cr, CPnt pos, const char* _pStr, int32 color)
{
	MAUTOSTRIP(CImage_DebugText, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	int l = DebugTextLen(_pStr);
	CRct r(pos.x, pos.y, pos.x + l, pos.y + 8);
	if (!cr.Visible(r)) return;
	pos += cr;

//	CPixel32 col(color);
	int col = ConvertColor(color, m_Format, m_spPalette);
	uint8* pSurfMem;
	MLock(this, pSurfMem);
	int mod = GetModulo();
	int psize = GetPixelSize();
	int strl = CStr::StrLen(_pStr);
	int xpos = pos.x;
	int fnth = DebugTextHeight();
	for (int cpos = 0; cpos < strl; cpos++)
	{
		int c = _pStr[cpos] & 127;
		int cwidth = ms_DebugFontWidth[c];

		if ((xpos+cwidth >= cr.clip.p0.x) && (pos.y+8 >= cr.clip.p0.y) &&
			(xpos < cr.clip.p1.x) && (pos.y < cr.clip.p1.y))
		{
			int32 xbitstart = Max(0, cr.clip.p0.x-xpos);
			int32 xbitstop = Min(cwidth, cr.clip.p1.x-xpos);

			for (int y = 0; y < fnth; y++) {
				int yp = pos.y + y;
				if ((yp >= cr.clip.p0.y) && (yp < cr.clip.p1.y))
				{
					int32 scanline = ms_DebugFont[c][y];
/*					if (scanline != 0)
					{
						int32 bit = 0x80 >> xbitstart;
						for (int32 x = xbitstart; x<xbitstop; x++)
						{
							if ((scanline & bit) != 0)
								Internal_SetPixel(x+xpos, yp, col, pSurfMem);
							bit >>= 1;
						};
					};*/
					Internal_FillMask(scanline<<(24-xbitstart), col, xbitstop-xbitstart, &pSurfMem[yp*mod + (xbitstart + xpos)*psize], psize);
				};
			};
		};
		xpos += cwidth;
	};

	MUnlock(this);
};

void CImage::DrawPalette(const CClipRect cr, CPnt pos)
{
	MAUTOSTRIP(CImage_DrawPalette, MAUTOSTRIP_VOID);

	if (!(m_Memmodel & IMAGE_MEM_LOCKABLE)) return;
	CRct r(pos, CPnt(pos.x + 256, pos.y + 8));
	r += cr;
	
	if (GetFormat() != IMAGE_FORMAT_CLUT8) return;
	uint8* pSurfMem;
	MLock(this, pSurfMem);
	{
		int mod = GetModulo();
		int psize = GetPixelSize();
		for (int x = r.p0.x; x < r.p1.x; x++)
			for (int y = r.p0.y; y < r.p1.y; y++)
				pSurfMem[x*psize + y*mod] = x;
	};

	MUnlock(this);
};


/*
void CImage::Text(const CClipRect& cr, CPnt pos, const CStr& str, CFont* font)
{
	int32 w = font->TextWidth(str);
	int32 h = font->TextHeight();
	CRct r(pos.x, pos.y, w+pos.x, h+pos.y);
	if (!cr.Visible(r)) return;

};
*/

// -------------------------------------------------------------------
//  Colorconvertion
// -------------------------------------------------------------------
CPixel32 SYSTEMDLLEXPORT CImage::ConvFromRGBA4444(uint16 _c)
{
	MAUTOSTRIP(CImage_ConvFromRGBA4444, 0);

	int r = (_c & 0xf00) >> 8;
	int g = (_c & 0xf0) >> 4;
	int b = (_c & 0xf) >> 0;
	int a = (_c & 0xf000) >> 12;

	return (
		(((r << 4) + r) << 16) +
		(((g << 4) + g) << 8) +
		(((b << 4) + b) << 0) +
		(((a << 4) + a) << 24)
	);
}

CPixel32 SYSTEMDLLEXPORT CImage::ConvFromRGB565(uint16 _c)
{
	MAUTOSTRIP(CImage_ConvFromRGB565, 0);

	int r = (_c >> 11) & 31;
	r = (r << 3) + (r >> 2);
	int g = (_c >> 5) & 63;
	g = (g << 2) + (g >> 4);
	int b = (_c & 31);
	b = (b << 3) + (b >> 2);
	int a = 255;

	return (a << 24) + (r << 16) + (g << 8) + (b << 0);
}

CPixel32 SYSTEMDLLEXPORT CImage::ConvFromRGBA5551(uint16 _c)
{
	MAUTOSTRIP(CImage_ConvFromRGBA5551, 0);

	int a = (_c >> 15) & 1;
	a = (a << 8) - a;
	int r = (_c >> 10) & 31;
	r = (r << 3) + (r >> 2);
	int g = (_c >> 5) & 31;
	g = (g << 3) + (g >> 2);
	int b = (_c & 31);
	b = (b << 3) + (b >> 2);
	return (a << 24) + (r << 16) + (g << 8) + (b << 0);
}

// -------------------------------------------------------------------
uint16 SYSTEMDLLEXPORT CImage::ConvToRGBA4444(CPixel32 _c)
{
	MAUTOSTRIP(CImage_ConvToRGBA4444, 0);

	int r = (_c >> 20) & 0x0f;
	int g = (_c >> 12) & 0x0f;
	int b = (_c >> 4) & 0x0f;
	int a = (_c >> 28) & 0x0f;

	return (a << 12) + (r << 8) + (g << 4) + b;
}

uint16 SYSTEMDLLEXPORT CImage::ConvToRGB565(CPixel32 _c)
{
	MAUTOSTRIP(CImage_ConvToRGB565, 0);

	int r = (_c >> (16+3)) & 0x1f;
	int g = (_c >> (8+2)) & 0x3f;
	int b = (_c >> 3) & 0x1f;
	return (r << 11) + (g << 5) + b;
}

uint16 SYSTEMDLLEXPORT CImage::ConvToRGBA5551(CPixel32 _c)
{
	MAUTOSTRIP(CImage_ConvToRGBA5551, 0);

	int a = (_c >> (24+7)) & 1;
	int r = (_c >> (16+3)) & 0x1f;
	int g = (_c >> (8+3)) & 0x1f;
	int b = (_c >> 3) & 0x1f;
	return (a << 15) + (r << 10) + (g << 5) + b;
}

CVec2Dfp32 SYSTEMDLLEXPORT CImage::TransformSampleVector(int _iTransform, const CVec2Dfp32& _InVector, fp32 _PixelWidth, fp32 _PixelHeight)
{
	CVec2Dfp32 OutVector = _InVector;
	if(_iTransform & IMAGE_TRANSFORM_ROTATECW)
	{
		CVec2Dfp32 TempVector = OutVector;
		OutVector.k[0]	= 1.0f - TempVector.k[1] - _PixelWidth;
		OutVector.k[1]	= TempVector.k[0];
	}

	if(_iTransform & IMAGE_TRANSFORM_ROTATECCW)
	{
		CVec2Dfp32 TempVector = OutVector;
		OutVector.k[0]	= TempVector.k[1];
		OutVector.k[1]	= 1.0f - TempVector.k[0] - _PixelHeight;
	}

	if(_iTransform & IMAGE_TRANSFORM_FLIPH)
		OutVector.k[0]	= 1.0f - OutVector.k[0] - _PixelWidth;
	if(_iTransform & IMAGE_TRANSFORM_FLIPV)
		OutVector.k[1]	= 1.0f - OutVector.k[1] - _PixelHeight;

	return OutVector;
}

spCImage CImage::Transform(int _iTransform)
{
	spCImage spImg = MNew(CImage);
	Duplicate(spImg);
	spImg->Fill(spImg->GetClipRect(), 0xff80ff00);

	int Width = GetWidth();
	int Height = GetHeight();
	fp32 invWidth = 1.0f / Width;
	fp32 invHeight = 1.0f / Height;

	for(int y = 0; y < Height; y++)
	{
		for(int x = 0; x < Width; x++)
		{
			CVec2Dfp32 SampleTexcoord(x * invWidth, y * invHeight);
			CVec2Dfp32 OutputTexcoord = TransformSampleVector(_iTransform, SampleTexcoord, invWidth, invHeight);

			CPixel32 Pixel = GetPixel(GetClipRect(), CPnt((int)(SampleTexcoord.k[0] * Width), (int)(SampleTexcoord.k[1] * Height)));
			spImg->SetPixel(spImg->GetClipRect(), CPnt((int)(OutputTexcoord.k[0] * Width), (int)(OutputTexcoord.k[1] * Height)), Pixel);
		}
	}

	return spImg;
}
