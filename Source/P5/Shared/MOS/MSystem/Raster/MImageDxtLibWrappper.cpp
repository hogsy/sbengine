#include "PCH.h"
#include "MImageDxtLibWrapper.h"

#ifdef S3TC_USE_DXTLIB
#define EXCLUDE_LIBS
#include "../../../../SDK/NVidia dxtlib/inc/dxtlib/dxtlib.h"


typedef HRESULT (*FPnvDXTcompressBGRA)(unsigned char * image_data, 
                      unsigned long image_width,
                      unsigned long image_height, 
                      DWORD byte_pitch,
                      CompressionOptions * options,
                      DWORD planes,
                      MIPcallback MIPMapRoutine,   
                      FileWriteCallback fileWriteRoutine,
                      RECT * rect);


class CLoadNVidiaWrapper
{
public:
	CLoadNVidiaWrapper()
	{
		m_hNvidiaDll = NULL;
		nvDXTcompressBGRAPtr = NULL; 
	}

	~CLoadNVidiaWrapper()
	{
		if (m_hNvidiaDll)
		{
			FreeLibrary((HMODULE)m_hNvidiaDll);
		}
	}

	void *m_hNvidiaDll;
	void *nvDXTcompressBGRAPtr;

	void Load()
	{
		if (!m_hNvidiaDll)
		{
			m_hNvidiaDll = LoadLibrary("NVDxtWrapper.dll");
			nvDXTcompressBGRAPtr = NULL;
			if (m_hNvidiaDll)
			{
				nvDXTcompressBGRAPtr = GetProcAddress((HMODULE)m_hNvidiaDll, "nvDXTcompressBGRA");
				if (!nvDXTcompressBGRAPtr)
				{
					Error_static(M_FUNCTION, "Cannot find nvDXTcompressBGRA in NVDxtWrapper.dll");
				}
			}
			else
			{
				Error_static(M_FUNCTION, "Cannot load NVDxtWrapper.dll");
			}
		}
	}
};

CLoadNVidiaWrapper g_NVidiaWrapperLoader;


CDXTLibCompressor::CDXTLibCompressor()
{
	g_NVidiaWrapperLoader.Load();
}

CDXTLibCompressor::~CDXTLibCompressor()
{
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:	Compresses an image with S3TC using DXTlib

	Parameters:		
		_pSrcImg:		Source stream (CPixel32 BGRA values)
		_pDst:			Destination stream
		_Width:			Image width
		_Height:		Image height
		_bAlpha:		Image alpha
		_pRGBWeights:	RGB weighting when compressing

	Returns:	alpha drop status
\*____________________________________________________________________*/ 
	
	
bool CDXTLibCompressor::Encode(uint8* _pSrcImg,uint8 * _pDst,uint16 _Width,uint16 _Height,
							   bool _bAlpha,fp32 *_pRGBWeights)
{
	if (!g_NVidiaWrapperLoader.m_hNvidiaDll || !g_NVidiaWrapperLoader.nvDXTcompressBGRAPtr)
		return false;
	uint32 i;
	uint8 Bpp = (_bAlpha) ? 4 : 3;
	bool bOriginalAlpha = _bAlpha;

	//Make sure we have a valid image
	TThinArray<uint8>	lRealImage;
	if( (_Width & 0x03) || (_Height & 0x03) )
	{
		uint16 NewWidth = (_Width+3) & 0xFFFC;
		uint16 NewHeight = (_Height+3) & 0xFFFC;
		lRealImage.SetLen(NewWidth * NewHeight * Bpp);

		uint8 * pDst = lRealImage.GetBasePtr();
		for(i = 0;i < NewHeight;i++)
		{
			uint32 iStart = ((i < _Height) ? i : _Height-1) * _Width;
			for(int j = 0;j < NewWidth;j++)
			{
				uint8 * pSrc = _pSrcImg + 4*(iStart + ((j < _Width) ? j : _Width-1));
				memcpy(pDst,pSrc,Bpp);
				pDst += Bpp;
			}
		}

		_Width = NewWidth;
		_Height = NewHeight;
		_pSrcImg = lRealImage.GetBasePtr();
	}
	else if(!_bAlpha)
	{
		lRealImage.SetLen(_Width * _Height * 3);
		for(i = 0;i < _Width * _Height;i++)
		{
			memcpy(lRealImage.GetBasePtr()+i*3,_pSrcImg,3);
			_pSrcImg += 4;
		}
		_pSrcImg = lRealImage.GetBasePtr();
	}


	CompressionOptions Options;
	Options.mipMapGeneration = kNoMipMaps;
	Options.textureFormat = (_bAlpha) ? kDXT5 : kDXT1;

	//These values could be parameterized
	/*
	if( _bAlpha )
	{
		uint32 nAlphaBinary = NonBinaryAlpha(_pSrcImg,_Width * _Height,2);
		if( nAlphaBinary == 0 )
		{
			_bAlpha = false;
			Options.bBinaryAlpha = true;
			Options.alphaThreshold = 128;
			Options.textureFormat = kDXT1a;
		}
	}
	*/

	Options.user_data = this;
	if( _pRGBWeights )
	{
		memcpy(Options.weight,_pRGBWeights,3*sizeof(fp32));
	}
	// Options.bDitherColor = true;

	m_nBytesWritten = 0;
	m_pTarget = _pDst;
	MRTC_GOM()->ForgiveDebugNew(1);
	long Res = ((FPnvDXTcompressBGRA)g_NVidiaWrapperLoader.nvDXTcompressBGRAPtr)((uint8*)_pSrcImg,_Width,_Height,Bpp*_Width,&Options,Bpp,0,Callback, NULL);
	MRTC_GOM()->ForgiveDebugNew(-1);

	return (_bAlpha != bOriginalAlpha);
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Callback function for libDXT

Parameters:
	_nBytes:	Bytes to be written
	_pBuf:		pointer to data
	_pUserData:	the CDXTLibCompressor object

Returns:	0
\*____________________________________________________________________*/ 
long CDXTLibCompressor::Callback(size_t _nBytes,void * _pBuf,void * _pUserData)
{
	CDXTLibCompressor * pComp = (CDXTLibCompressor*)_pUserData;
	
	if( pComp->m_nBytesWritten >= 128 )
	{
		memcpy( pComp->m_pTarget + pComp->m_nBytesWritten - 128,_pBuf,_nBytes );
	}
	else if( pComp->m_nBytesWritten + _nBytes > 128 )
	{
		size_t nRealBytes = (pComp->m_nBytesWritten + _nBytes)-128;
		memcpy( pComp->m_pTarget,(uint8*)_pBuf + (_nBytes - nRealBytes),nRealBytes );
	}
	
	pComp->m_nBytesWritten += _nBytes;

	return 0;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	NonBinaryAlpha

Parameters:
	_pSrcImage:	Source data
	_nPixels:	Number of pixels
	_Threshold: darkness threshold

Returns:	Number of pixels where 
		    ((alpha <= _Threshold) || (alpha >= 255-_Threshold))
\*____________________________________________________________________*/
uint32 CDXTLibCompressor::NonBinaryAlpha(const uint8 * _pSrcImage,uint32 _nPixels,uint8 _Threshold)
{
	uint32 nPxl = 0;

	_pSrcImage += 3;
	for(int i = 0;i < _nPixels;i++)
	{
		if( (*_pSrcImage > _Threshold) && (*_pSrcImage < (255-_Threshold)) )
			nPxl++;

		_pSrcImage	+= 4;
	}

	return nPxl;
}

#endif
