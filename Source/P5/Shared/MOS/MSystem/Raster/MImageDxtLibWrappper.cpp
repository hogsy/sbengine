

#include "PCH.h"
#include "MImageDxtLibWrapper.h"

#ifdef S3TC_USE_DXTLIB

#include "../../../../SDK/NVidia dxtlib/inc/dxtlib/dxtlib.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Compresses an image with S3TC using DXTlib

Parameters:		
	_pSrcImg:	Source stream (CPixel32 BGRA values)
	_pDst:		Destination stream
	_Width:		Image width
	_Height:	Image height
	_bAlpha:	Image alpha
	_pRGBWeights:	RGB weighting when compressing
\*____________________________________________________________________*/ 
void CDXTLibCompressor::Encode(uint8* _pSrcImg,uint8 * _pDst,uint16 _Width,uint16 _Height,
							   bool _bAlpha,fp4 *_pRGBWeights)
{
	uint32 i;
	uint8 Bpp = (_bAlpha) ? 4 : 3;

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
	Options.user_data = this;
	if( _pRGBWeights )
	{
		memcpy(Options.weight,_pRGBWeights,3*sizeof(fp4));
	}
	// Options.bDitherColor = true;

	m_nBytesWritten = 0;
	m_pTarget = _pDst;
	MRTC_GOM()->ForgiveDebugNew(1);
	long Res = nvDXTcompressBGRA((uint8*)_pSrcImg,_Width,_Height,Bpp*_Width,&Options,Bpp,0,Callback);
	MRTC_GOM()->ForgiveDebugNew(-1);
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

#endif
