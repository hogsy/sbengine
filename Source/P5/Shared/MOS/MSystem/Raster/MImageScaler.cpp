
#include "PCH.h"
#include "MImage.h"

#ifndef PLATFORM_CONSOLE

template <class tCompType, class tWeightType>
static M_INLINE tCompType CImage_Lerp(tCompType _a, tCompType _b, tWeightType _Weight)
{
	const int ShiftCount = sizeof(tWeightType) * 8;
	const int MaxVal = (1 << ShiftCount) - 1;
	return (uint8)((_a * (MaxVal - _Weight) + _b * _Weight) >> ShiftCount);
}

template <class tCompType, int nCompNum, int nPixelStride, int nCompStride, class tWeightType>
class CImage_Stretch_Sample;

template <class tCompType, int nPixelStride, int nCompStride, class tWeightType>
class CImage_Stretch_Sample<tCompType, 4, nPixelStride, nCompStride, tWeightType>
{
public:
	static M_INLINE void Filter(const uint8* _pSrc, uint8* _pDest, int _nRowStride, tWeightType _fColWeight, tWeightType _fRowWeight)
	{
		const tCompType *pSample00 = (const tCompType*)(_pSrc);
		const tCompType *pSample01 = (const tCompType*)(_pSrc + nPixelStride);
		const tCompType *pSample10 = (const tCompType*)(_pSrc + _nRowStride);
		const tCompType *pSample11 = (const tCompType*)(_pSrc + _nRowStride + nPixelStride);

		*(tCompType*)(_pDest + nCompStride * 0)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[0], pSample01[0], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[0], pSample11[0], _fColWeight), _fRowWeight);
		*(tCompType*)(_pDest + nCompStride * 1)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[1], pSample01[1], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[1], pSample11[1], _fColWeight), _fRowWeight);
		*(tCompType*)(_pDest + nCompStride * 2)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[2], pSample01[2], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[2], pSample11[2], _fColWeight), _fRowWeight);
		*(tCompType*)(_pDest + nCompStride * 3)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[3], pSample01[3], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[3], pSample11[3], _fColWeight), _fRowWeight);
	}
};

template <class tCompType, int nPixelStride, int nCompStride, class tWeightType>
class CImage_Stretch_Sample<tCompType, 3, nPixelStride, nCompStride, tWeightType>
{
public:
	static M_INLINE void Filter(const uint8* _pSrc, uint8* _pDest, int _nRowStride, tWeightType _fColWeight, tWeightType _fRowWeight)
	{
		const tCompType *pSample00 = (const tCompType*)(_pSrc);
		const tCompType *pSample01 = (const tCompType*)(_pSrc + nPixelStride);
		const tCompType *pSample10 = (const tCompType*)(_pSrc + _nRowStride);
		const tCompType *pSample11 = (const tCompType*)(_pSrc + _nRowStride + nPixelStride);

		*(tCompType*)(_pDest + nCompStride * 0)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[0], pSample01[0], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[0], pSample11[0], _fColWeight), _fRowWeight);
		*(tCompType*)(_pDest + nCompStride * 1)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[1], pSample01[1], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[1], pSample11[1], _fColWeight), _fRowWeight);
		*(tCompType*)(_pDest + nCompStride * 2)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[2], pSample01[2], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[2], pSample11[2], _fColWeight), _fRowWeight);
	}
};

template <class tCompType, int nPixelStride, int nCompStride, class tWeightType>
class CImage_Stretch_Sample<tCompType, 2, nPixelStride, nCompStride, tWeightType>
{
public:
	static M_INLINE void Filter(const uint8* _pSrc, uint8* _pDest, int _nRowStride, tWeightType _fColWeight, tWeightType _fRowWeight)
	{
		const tCompType *pSample00 = (const tCompType*)(_pSrc);
		const tCompType *pSample01 = (const tCompType*)(_pSrc + nPixelStride);
		const tCompType *pSample10 = (const tCompType*)(_pSrc + _nRowStride);
		const tCompType *pSample11 = (const tCompType*)(_pSrc + _nRowStride + nPixelStride);

		*(tCompType*)(_pDest + nCompStride * 0)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[0], pSample01[0], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[0], pSample11[0], _fColWeight), _fRowWeight);
		*(tCompType*)(_pDest + nCompStride * 1)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[1], pSample01[1], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[1], pSample11[1], _fColWeight), _fRowWeight);
	}
};

template <class tCompType, int nPixelStride, int nCompStride, class tWeightType>
class CImage_Stretch_Sample<tCompType, 1, nPixelStride, nCompStride, tWeightType>
{
public:
	static M_INLINE void Filter(const uint8* _pSrc, uint8* _pDest, int _nRowStride, tWeightType _fColWeight, tWeightType _fRowWeight)
	{
		const tCompType *pSample00 = (const tCompType*)(_pSrc);
		const tCompType *pSample01 = (const tCompType*)(_pSrc + nPixelStride);
		const tCompType *pSample10 = (const tCompType*)(_pSrc + _nRowStride);
		const tCompType *pSample11 = (const tCompType*)(_pSrc + _nRowStride + nPixelStride);

		*(tCompType*)(_pDest + nCompStride * 0)	= CImage_Lerp<tCompType,tWeightType>(CImage_Lerp<tCompType,tWeightType>(pSample00[0], pSample01[0], _fColWeight), CImage_Lerp<tCompType,tWeightType>(pSample10[0], pSample11[0], _fColWeight), _fRowWeight);
	}
};


template <class tCompType, int nCompNum>
static void CImage_Stretch(CImage* _pSrc, CImage* _pDest)
{
	const int PixelSize = sizeof(tCompType) * nCompNum;
	int SourceWidth = _pSrc->GetWidth();
	int SourceModulo = _pSrc->GetModulo();
	int SourceHeight = _pSrc->GetHeight();
	int TargetWidth = _pDest->GetWidth();
	int TargetHeight = _pDest->GetHeight();
	int TargetModulo = _pDest->GetModulo();
	fp32 DeltaX = (fp32)(SourceWidth) / (fp32)TargetWidth;
	fp32 DeltaY = (fp32)(SourceHeight) / (fp32)TargetHeight;

	TThinArray<int> lColOffsets;
	TThinArray<tCompType> lColWeights;
	TThinArray<int> lRowOffsets;
	TThinArray<tCompType> lRowWeights;
	lColOffsets.SetLen(TargetWidth);
	lColWeights.SetLen(TargetWidth);
	lRowOffsets.SetLen(TargetHeight);
	lRowWeights.SetLen(TargetHeight);
	int* pColOffsets = lColOffsets.GetBasePtr();
	tCompType* pColWeights = lColWeights.GetBasePtr();
	int* pRowOffsets = lRowOffsets.GetBasePtr();
	tCompType* pRowWeights = lRowWeights.GetBasePtr();

	if(TargetWidth >= SourceWidth)
	{
		fp32 Scale = TargetWidth / (fp32)SourceWidth;
		int BorderWidth = RoundToInt(Scale * 0.5f);

		for(int i = 0; i < BorderWidth; i++)
		{
			pColOffsets[i]	= 0;
			pColWeights[i]	= 0.0f;
			pColOffsets[TargetWidth - BorderWidth + i]	= (SourceWidth-2) * PixelSize;
			pColWeights[TargetWidth - BorderWidth + i]	= 1.0f;
		}

		fp32 FloatColWeight = 0.0f;
		int ColPos = 0;
		for(int i = BorderWidth; i < (TargetWidth - BorderWidth); i++)
		{
			pColOffsets[i]	= ColPos;
			pColWeights[i]	= ((1 << sizeof(tCompType) * 8) - 1) * FloatColWeight;
			FloatColWeight	+= DeltaX;
			if(FloatColWeight > 1.0f)
			{
				FloatColWeight	-= 1.0f;
				ColPos	+= PixelSize;
			}
		}
	}
	else
	{
		// This becomes pointsampling.. should do it some other way
		fp32 FloatColWeight = 0.0f;
		int ColPos = 0;
		for(int i = 0; i < TargetWidth; i++)
		{
			pColOffsets[i]	= ColPos;
			pColWeights[i]	= ((1 << sizeof(tCompType) * 8) - 1) * FloatColWeight;
			FloatColWeight	+= DeltaX;
			if(FloatColWeight > 1.0f)
			{
				int Count = (int)FloatColWeight;
				FloatColWeight	-= (fp32)Count;
				ColPos	+= PixelSize * Count;
			}
		}
	}

	if(TargetHeight >= SourceHeight)
	{
		fp32 Scale = TargetHeight / (fp32)SourceHeight;
		int BorderHeight = RoundToInt(Scale * 0.5f);

		for(int i = 0; i < BorderHeight; i++)
		{
			pRowOffsets[i]	= 0;
			pRowWeights[i]	= 0.0f;
			pRowOffsets[TargetHeight - BorderHeight + i]	= (SourceHeight-2) * SourceModulo;
			pRowWeights[TargetHeight - BorderHeight + i]	= 1.0f;
		}

		fp32 FloatRowWeight = 0.0f;
		int RowPos = 0;
		for(int i = BorderHeight; i < (TargetHeight - BorderHeight); i++)
		{
			pRowOffsets[i]	= RowPos;
			pRowWeights[i]	= ((1 << sizeof(tCompType) * 8) - 1) * FloatRowWeight;
			FloatRowWeight	+= DeltaY;
			if(FloatRowWeight > 1.0f)
			{
				FloatRowWeight	-= 1.0f;
				RowPos	+=  SourceModulo;
			}
		}
	}
	else
	{
		fp32 FloatRowWeight = 0.0f;
		int RowPos = 0;
		for(int i = 0; i < TargetHeight; i++)
		{
			pRowOffsets[i]	= RowPos;
			pRowWeights[i]	= ((1 << sizeof(tCompType) * 8) - 1) * FloatRowWeight;
			FloatRowWeight	+= DeltaY;
			if(FloatRowWeight > 1.0f)
			{
				int Count = (int)FloatRowWeight;
				FloatRowWeight	-= (fp32)Count;
				RowPos	+= SourceModulo * Count;
			}
		}
	}


	{
		const uint8* pSrcData = (const uint8*)_pSrc->Lock();
		uint8* pDestData = (uint8*)_pDest->Lock();

		const int nRowStride = _pSrc->GetModulo();

		for(int iRow = 0; iRow < TargetHeight-1; iRow++)
		{
			const uint8* pSrcRow = pSrcData + pRowOffsets[iRow];
			for(int iCol = 0; iCol < TargetWidth; iCol++)
			{
				CImage_Stretch_Sample<tCompType, nCompNum, PixelSize, sizeof(tCompType), tCompType>::Filter(pSrcRow + pColOffsets[iCol], pDestData, nRowStride, pColWeights[iCol], pRowWeights[iRow]);
				pDestData	+= PixelSize;
			}
		}
		const uint8* pSrcRow = pSrcData + pRowOffsets[TargetHeight-1];
		for(int iCol = 0; iCol < TargetWidth; iCol++)
		{
			CImage_Stretch_Sample<tCompType, nCompNum, PixelSize, sizeof(tCompType), tCompType>::Filter(pSrcRow + pColOffsets[iCol], pDestData, 0, pColWeights[iCol], pRowWeights[TargetHeight-1]);
			pDestData	+= PixelSize;
		}

		_pDest->Unlock();
		_pSrc->Unlock();
	}
}



void SYSTEMDLLEXPORT CImage::Stretch(CImage* _pSrc, CImage* _pDest, int _NewWidth, int _NewHeight)
{
	int Format = _pSrc->GetFormat();
	if((_pDest->GetFormat() != Format) || (_pDest->GetWidth() != _NewWidth) || (_pDest->GetHeight() != _NewHeight))
	{
		_pDest->Destroy();
		_pDest->Create(_NewWidth, _NewHeight, Format, IMAGE_MEM_IMAGE);
	}
	switch(Format)
	{
	case IMAGE_FORMAT_RGBA8:
	case IMAGE_FORMAT_BGRX8:
	case IMAGE_FORMAT_BGRA8:
		{
			CImage_Stretch<uint8, 4>(_pSrc, _pDest);
			break;
		}

	case IMAGE_FORMAT_RGB8:
	case IMAGE_FORMAT_BGR8:
		{
			CImage_Stretch<uint8, 3>(_pSrc, _pDest);
			break;
		}

	case IMAGE_FORMAT_I8A8:
		{
			CImage_Stretch<uint8, 2>(_pSrc, _pDest);
			break;
		}

	case IMAGE_FORMAT_I8:
		{
			CImage_Stretch<uint8, 1>(_pSrc, _pDest);
			break;
		}

	default:
		Error_static("Stretch", CStrF("Format not supported. (%s)", CImage::GetFormatName(Format)));
	}
}

#endif	// PLATFORM_CONSOLE
