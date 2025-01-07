#include "PCH.h"

#include "MTextureContainers.h"
#include "MImageCubemapSampler.h"
#include "MImage.h"

// -------------------------------------------------------------------
//  CImageCubemapSampler
// -------------------------------------------------------------------

// -------------------------------------------------------------------
//  CImageCubemapSampler
// -------------------------------------------------------------------
IMPLEMENT_OPERATOR_NEW(CImageCubemapSampler);

CImageCubemapSampler::CImageCubemapSampler() : m_bSingleImage(false), m_bHardwareAccurateEdgeSampling(false)
{
}

CImageCubemapSampler::~CImageCubemapSampler()
{
}

void CImageCubemapSampler::Create()
{
}

void CImageCubemapSampler::Clear()
{
	for(int i = 0; i < 6; i++)
		m_lpSides[i]	= 0;
	m_bSingleImage	= false;
	m_bHardwareAccurateEdgeSampling	= false;
}

void CImageCubemapSampler::SetSingleImage(CImage* _pImg)
{
	M_ASSERT(_pImg->GetFlags() & IMAGE_MEM_LOCKABLE, "Cubemap sample has to be able to lock image");
	m_bSingleImage	= true;
	m_lpSides[0]	= _pImg;
}

void CImageCubemapSampler::SetSideImage(int _iSide, CImage* _pImg)
{
	M_ASSERT((_iSide >= 0) && (_iSide < 6), "Cubemap side index is out of range");
	m_bSingleImage	= false;
	m_lpSides[_iSide]	= _pImg;
}

void CImageCubemapSampler::SetHardwareAccurateEdgeSampling(bool _bMoomin)
{
	m_bHardwareAccurateEdgeSampling	= _bMoomin;
}

int CImageCubemapSampler::GetSideTransform(int _iSide)
{
	M_ASSERT((_iSide >= 0) && (_iSide < 6), "Cubemap side index is out of range");
	switch(_iSide)
	{
	case IMAGE_CUBEMAPSIDE_POSX: return IMAGE_TRANSFORM_ROTATECW | IMAGE_TRANSFORM_FLIPH;
	case IMAGE_CUBEMAPSIDE_NEGX: return IMAGE_TRANSFORM_ROTATECW | IMAGE_TRANSFORM_FLIPV;
	case IMAGE_CUBEMAPSIDE_POSY: return IMAGE_TRANSFORM_FLIPV;
	case IMAGE_CUBEMAPSIDE_NEGY: return IMAGE_TRANSFORM_FLIPH;
	case IMAGE_CUBEMAPSIDE_POSZ: return IMAGE_TRANSFORM_ROTATECW | IMAGE_TRANSFORM_FLIPH;
	case IMAGE_CUBEMAPSIDE_NEGZ: return IMAGE_TRANSFORM_ROTATECW | IMAGE_TRANSFORM_FLIPH;
	}

	return 0;
}

CPixel32 CImageCubemapSampler::SampleVector(const CVec3Dfp32& _Vector) const
{
	int iSide = -1;
	fp32 sc, tc, ma;
	if(M_Fabs(_Vector.k[0]) > M_Fabs(_Vector.k[1]))
	{
		if(M_Fabs(_Vector.k[0]) > M_Fabs(_Vector.k[2]))
		{
			if(_Vector.k[0] < 0.0f)
				iSide	= IMAGE_CUBEMAPSIDE_NEGX;
			else
				iSide	= IMAGE_CUBEMAPSIDE_POSX;
		}
		else
		{
			if(_Vector.k[2] < 0.0f)
				iSide	= IMAGE_CUBEMAPSIDE_NEGZ;
			else
				iSide	= IMAGE_CUBEMAPSIDE_POSZ;
		}
	}
	else if(M_Fabs(_Vector.k[1]) > M_Fabs(_Vector.k[2]))
	{
		if(_Vector.k[1] < 0.0f)
			iSide	= IMAGE_CUBEMAPSIDE_NEGY;
		else
			iSide	= IMAGE_CUBEMAPSIDE_POSY;
	}
	else
	{
		if(_Vector.k[2] < 0.0f)
			iSide	= IMAGE_CUBEMAPSIDE_NEGZ;
		else
			iSide	= IMAGE_CUBEMAPSIDE_POSZ;
	}

	switch(iSide)
	{
	case IMAGE_CUBEMAPSIDE_POSX:
	case IMAGE_CUBEMAPSIDE_NEGX:
		{
			if(iSide == IMAGE_CUBEMAPSIDE_POSX)
				sc	= -_Vector.k[2];
			else
				sc	= _Vector.k[2];
			tc	= -_Vector.k[1];
			ma	= _Vector.k[0];
			break;
		}

	case IMAGE_CUBEMAPSIDE_POSY:
	case IMAGE_CUBEMAPSIDE_NEGY:
		{
			if(iSide == IMAGE_CUBEMAPSIDE_POSY)
				tc	= _Vector.k[2];
			else
				tc	= -_Vector.k[2];
			sc	= _Vector.k[0];
			ma	= _Vector.k[1];
			break;
		}

	case IMAGE_CUBEMAPSIDE_POSZ:
	case IMAGE_CUBEMAPSIDE_NEGZ:
		{
			if(iSide == IMAGE_CUBEMAPSIDE_POSZ)
				sc	= _Vector.k[0];
			else
				sc	= -_Vector.k[0];
			tc	= -_Vector.k[1];
			ma	= _Vector.k[2];
			break;
		}
	}

	fp32 s = (sc / M_Fabs(ma)) * 0.5f + 0.5f;
	fp32 t = (tc / M_Fabs(ma)) * 0.5f + 0.5f;

	CPixel32 Color = m_lpSides[iSide]->GetPixelUV_Bilinear(CVec2Dfp32(s, t), IMAGE_GETPIXEL_EDGECLAMP);

	return Color;
}

bool CImageCubemapSampler::Validate()
{
	for(int i = 0; i < 6; i++)
		if(m_lpSides[i] == 0 )
			return false;

	for(int i = 1; i < 6; i++)
	{
		if(m_lpSides[i]->GetFormat() != m_lpSides[0]->GetFormat())
			return false;
		if(m_lpSides[i]->GetWidth() != m_lpSides[0]->GetWidth())
			return false;
		if(m_lpSides[i]->GetHeight() != m_lpSides[0]->GetHeight())
			return false;
	}

	return true;
}

void CImageCubemapSampler::SmoothPixel(CImage* _pImg, const CPnt& _Pos0, const CPnt& _Pos1)
{
	const CClipRect& ClipRect = _pImg->GetClipRect();
	CPixel32 Pixel0 = _pImg->GetPixel(ClipRect, _Pos0);
	CPixel32 Pixel1 = _pImg->GetPixel(ClipRect, _Pos1);
	int r = (Pixel0.GetR() + Pixel1.GetR() + 1) / 2;
	int g = (Pixel0.GetG() + Pixel1.GetG() + 1) / 2;
	int b = (Pixel0.GetB() + Pixel1.GetB() + 1) / 2;
	int a = (Pixel0.GetA() + Pixel1.GetA() + 1) / 2;
	CPixel32 FinalPixel(r, g, b, a);
	_pImg->SetPixel(ClipRect, _Pos0, FinalPixel);
	_pImg->SetPixel(ClipRect, _Pos1, FinalPixel);
}

void CImageCubemapSampler::SmoothPixel(CImage* _pImg, const CPnt& _Pos0, const CPnt& _Pos1, const CPnt& _Pos2, const CPnt& _Pos3)
{
	const CClipRect& ClipRect = _pImg->GetClipRect();
	CPixel32 Pixel0 = _pImg->GetPixel(ClipRect, _Pos0);
	CPixel32 Pixel1 = _pImg->GetPixel(ClipRect, _Pos1);
	CPixel32 Pixel2 = _pImg->GetPixel(ClipRect, _Pos2);
	CPixel32 Pixel3 = _pImg->GetPixel(ClipRect, _Pos3);
	int r = (Pixel0.GetR() + Pixel1.GetR() + Pixel2.GetR() + Pixel3.GetR() + 2) / 4;
	int g = (Pixel0.GetG() + Pixel1.GetG() + Pixel2.GetG() + Pixel3.GetG() + 2) / 4;
	int b = (Pixel0.GetB() + Pixel1.GetB() + Pixel2.GetB() + Pixel3.GetB() + 2) / 4;
	int a = (Pixel0.GetA() + Pixel1.GetA() + Pixel2.GetA() + Pixel3.GetA() + 2) / 4;
	CPixel32 FinalPixel(r, g, b, a);
	_pImg->SetPixel(ClipRect, _Pos0, FinalPixel);
	_pImg->SetPixel(ClipRect, _Pos1, FinalPixel);
	_pImg->SetPixel(ClipRect, _Pos2, FinalPixel);
	_pImg->SetPixel(ClipRect, _Pos3, FinalPixel);
}

void CImageCubemapSampler::SmoothPixel(int _iSide0, int _iSide1, const CPnt& _Pos0, const CPnt& _Pos1)
{
	const CClipRect& ClipRect = m_lpSides[_iSide0]->GetClipRect();
	CPixel32 Pixel0 = m_lpSides[_iSide0]->GetPixel(ClipRect, _Pos0);
	CPixel32 Pixel1 = m_lpSides[_iSide1]->GetPixel(ClipRect, _Pos1);
	int r = (Pixel0.GetR() + Pixel1.GetR() + 1) / 2;
	int g = (Pixel0.GetG() + Pixel1.GetG() + 1) / 2;
	int b = (Pixel0.GetB() + Pixel1.GetB() + 1) / 2;
	int a = (Pixel0.GetA() + Pixel1.GetA() + 1) / 2;
	CPixel32 FinalPixel(r, g, b, a);
	m_lpSides[_iSide0]->SetPixel(ClipRect, _Pos0, FinalPixel);
	m_lpSides[_iSide1]->SetPixel(ClipRect, _Pos1, FinalPixel);
}

void CImageCubemapSampler::SmoothPixel(int _iSide0, int _iSide1, int _iSide2, const CPnt& _Pos0, const CPnt& _Pos1, const CPnt& _Pos2)
{
	const CClipRect& ClipRect = m_lpSides[_iSide0]->GetClipRect();
	CPixel32 Pixel0 = m_lpSides[_iSide0]->GetPixel(ClipRect, _Pos0);
	CPixel32 Pixel1 = m_lpSides[_iSide1]->GetPixel(ClipRect, _Pos1);
	CPixel32 Pixel2 = m_lpSides[_iSide2]->GetPixel(ClipRect, _Pos2);
	int r = (Pixel0.GetR() + Pixel1.GetR() + Pixel2.GetR() + 1) / 3;
	int g = (Pixel0.GetG() + Pixel1.GetG() + Pixel2.GetG() + 1) / 3;
	int b = (Pixel0.GetB() + Pixel1.GetB() + Pixel2.GetB() + 1) / 3;
	int a = (Pixel0.GetA() + Pixel1.GetA() + Pixel2.GetA() + 1) / 3;
	CPixel32 FinalPixel(r, g, b, a);
	m_lpSides[_iSide0]->SetPixel(ClipRect, _Pos0, FinalPixel);
	m_lpSides[_iSide1]->SetPixel(ClipRect, _Pos1, FinalPixel);
	m_lpSides[_iSide2]->SetPixel(ClipRect, _Pos2, FinalPixel);
}

void CImageCubemapSampler::SmoothCubemapEdges()
{
	int w = m_lpSides[0]->GetWidth();
	int h = m_lpSides[0]->GetHeight();
	
	if( ( w > 2 ) && ( h > 2 ) )
	{
		//
		//   2
		//  140
		//   3
		//   5
		//
		//
		
		for( int nC = 1; nC < ( w - 1 ); nC++ )
		{
			SmoothPixel(1, 2, CPnt(nC, 0), CPnt(0, nC));

			SmoothPixel(1, 3, CPnt(nC, h - 1), CPnt(0, h - 1 - nC));

			SmoothPixel(0, 2, CPnt(nC, 0), CPnt(w - 1, h - 1 - nC));

			SmoothPixel(0, 3, CPnt(nC, h - 1), CPnt(w - 1, nC));

			SmoothPixel(4, 0, CPnt(w - 1, nC), CPnt(0, nC));

			SmoothPixel(4, 1, CPnt(0, nC), CPnt(w - 1, nC));

			SmoothPixel(4, 2, CPnt(nC, 0), CPnt(nC, h - 1));

			SmoothPixel(4, 3, CPnt(nC, h - 1), CPnt(nC, 0));

			SmoothPixel(0, 5, CPnt(w - 1, nC), CPnt(0, nC));

			SmoothPixel(1, 5, CPnt(0, nC), CPnt(w - 1, nC));

			SmoothPixel(2, 5, CPnt(w - 1 - nC, 0), CPnt(nC, 0));

			SmoothPixel(3, 5, CPnt(nC, h - 1), CPnt(w - 1 - nC, h - 1));
		}
		
		{
			SmoothPixel(4, 1, 2, CPnt(0, 0), CPnt(w - 1, 0), CPnt(0, h - 1));

			SmoothPixel(4, 0, 2, CPnt(w - 1, 0), CPnt(0, 0), CPnt(w - 1, h - 1));

			SmoothPixel(4, 1, 3, CPnt(0, h - 1), CPnt(w - 1, h - 1), CPnt(0, 0));

			SmoothPixel(4, 0, 3, CPnt(w - 1, h - 1), CPnt(0, h - 1), CPnt(w - 1, 0));

			SmoothPixel(5, 2, 0, CPnt(0, 0), CPnt(w - 1, 0), CPnt(w - 1, 0));

			SmoothPixel(5, 2, 1, CPnt(w - 1, 0), CPnt(0, 0), CPnt(0, 0));

			SmoothPixel(5, 3, 0, CPnt(0, h - 1), CPnt(w - 1, h - 1), CPnt(w - 1, h - 1));

			SmoothPixel(5, 3, 1, CPnt(w - 1, h - 1), CPnt(0, h - 1), CPnt(0, h - 1));
		}
	}
}

void CImageCubemapSampler::SmoothCubemapEdges_SingleImage(CImage* _pImg)
{
	int w = _pImg->GetWidth();
	int h = _pImg->GetHeight();

	if( ( w > 2 ) && ( h > 2 ) )
	{
		int nC;
		
		for( nC = 1; nC < ( h - 1 ); nC++ )
		{
			SmoothPixel(_pImg, CPnt(0, nC), CPnt(w - 1, nC));
			SmoothPixel(_pImg, CPnt(nC, 0), CPnt(nC, h - 1));
		}
		
		// Finally handle the corners (special case)
		SmoothPixel(_pImg, CPnt(0, 0), CPnt(w - 1, 0), CPnt(0, h - 1), CPnt(w - 1, h - 1));
	}
}


/*
	SmoothCubemaps().

*/
#ifndef PLATFORM_CONSOLE
void CTextureContainer_Plain::SmoothCubemapEdges(int _iTexture)
{
	uint8 aVersions[CTC_TEXTUREVERSION_MAX];
	int nVersions = EnumTextureVersions(_iTexture, aVersions, CTC_TEXTUREVERSION_MAX);
	int nMipmaps;
	int Flags;
	{
		spCTexture spT = GetTextureMap(_iTexture, CTC_TEXTUREVERSION_ANY);
		nMipmaps = spT->m_nMipmaps;
		Flags = spT->m_Properties.m_Flags;
	}

	if(Flags & CTC_TEXTUREFLAGS_CUBEMAP)
	{
		// Single image cubemap
		for(int iVersion = 0; iVersion < nVersions; iVersion++)
		{
			int iTxt = _iTexture + iVersion;
			for( int nMip = 0; nMip < nMipmaps; nMip++ )
			{
				CImage* pImg = GetTexture(iTxt, nMip, CTC_TEXTUREVERSION_ANY);
				CImageCubemapSampler::SmoothCubemapEdges_SingleImage(pImg);
			}
		}
	}
	else if(Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN)
	{
		// This texture is a cubemap (and the following 5)

		for(int iVersion = 0; iVersion < nVersions; iVersion++)
		{
			int iTxt = _iTexture + iVersion;

			for(int iMip = 0; iMip < nMipmaps; iMip++)
			{
				CImageCubemapSampler CubeSampler;
				CubeSampler.SetSideImage(0, GetTexture(iTxt + nVersions * 0, iMip, CTC_TEXTUREVERSION_ANY));
				CubeSampler.SetSideImage(1, GetTexture(iTxt + nVersions * 1, iMip, CTC_TEXTUREVERSION_ANY));
				CubeSampler.SetSideImage(2, GetTexture(iTxt + nVersions * 2, iMip, CTC_TEXTUREVERSION_ANY));
				CubeSampler.SetSideImage(3, GetTexture(iTxt + nVersions * 3, iMip, CTC_TEXTUREVERSION_ANY));
				CubeSampler.SetSideImage(4, GetTexture(iTxt + nVersions * 4, iMip, CTC_TEXTUREVERSION_ANY));
				CubeSampler.SetSideImage(5, GetTexture(iTxt + nVersions * 5, iMip, CTC_TEXTUREVERSION_ANY));

				if(CubeSampler.Validate())
					CubeSampler.SmoothCubemapEdges();
			}
		}
	}
}

void CTextureContainer_Plain::FilterCubemap(int _iTexture, const CTC_CubeFilterParams& _Params)
{
	uint8 aVersions[CTC_TEXTUREVERSION_MAX];
	int nVersions = EnumTextureVersions(_iTexture, aVersions, CTC_TEXTUREVERSION_MAX);
	int nMipmaps = GetTextureMap(_iTexture, CTC_TEXTUREVERSION_ANY)->m_nMipmaps;

	for(int iVersion = 0; iVersion < nVersions; iVersion++)
	{
		int nTxt = _iTexture + iVersion;

		{

			CImage* aRefSides[6];
			aRefSides[0]	= GetTexture(nTxt + nVersions * 0, 0, CTC_TEXTUREVERSION_ANY);
			aRefSides[1]	= GetTexture(nTxt + nVersions * 1, 0, CTC_TEXTUREVERSION_ANY);
			aRefSides[2]	= GetTexture(nTxt + nVersions * 2, 0, CTC_TEXTUREVERSION_ANY);
			aRefSides[3]	= GetTexture(nTxt + nVersions * 3, 0, CTC_TEXTUREVERSION_ANY);
			aRefSides[4]	= GetTexture(nTxt + nVersions * 4, 0, CTC_TEXTUREVERSION_ANY);
			aRefSides[5]	= GetTexture(nTxt + nVersions * 5, 0, CTC_TEXTUREVERSION_ANY);

			CImageCubemapSampler CubeSampler;
			CubeSampler.SetSideImage(0, aRefSides[0]);
			CubeSampler.SetSideImage(1, aRefSides[1]);
			CubeSampler.SetSideImage(2, aRefSides[2]);
			CubeSampler.SetSideImage(3, aRefSides[3]);
			CubeSampler.SetSideImage(4, aRefSides[4]);
			CubeSampler.SetSideImage(5, aRefSides[5]);
			if(CubeSampler.Validate())
			{
				spCImage aSides[6];
				// Check postfilter list to see if this texture should be processed any further

				fp32 FilterRadius = _Params.m_aParams[0];

				int Width = aRefSides[0]->GetWidth();
				int Height = aRefSides[0]->GetHeight();
				fp32 HalfWidth = (Width - 1) * 0.5f;
				fp32 HalfHeight = (Height - 1) * 0.5f;
				CClipRect Cliprect = aRefSides[0]->GetClipRect();

				int KernelMax = FilterRadius;//M_Ceil(FilterRadius * M_Sqrt(3.0f));
				int KernelWidth = KernelMax * 2 + 1;

				fp32 XAxisScale = 1.0f / (Width);
				fp32 YAxisScale = 1.0f / (Height);

				TThinArray<fp32> lCalcWeights;
				lCalcWeights.SetLen( Sqr(KernelWidth) );
				fp32* pCalcWeights = lCalcWeights.GetBasePtr();
				fp32 Sigma = M_Sqrt( -Sqr(KernelWidth/2) / (2 * (logf(0.125f) / logf(expf(1.0f)))) );
				fp32 WeightSum = 0.0f;
				for(int ky = -KernelMax; ky <= KernelMax; ky++)
				{
					for(int kx = -KernelMax; kx <= KernelMax; kx++)
					{
						fp32 Weight	= (2 * 3.14159265f * Sqr(Sigma)) * expf(-(Sqr(kx) + Sqr(ky)) / (2 * Sqr(Sigma)));
						pCalcWeights[(KernelMax + ky) * KernelWidth + (KernelMax + kx)]	= Weight;
						WeightSum	+= Weight;
					}
				}

				for(int iSide = 0; iSide < 6; iSide++)
				{
					aSides[iSide]	= MNew(CImage);
					aRefSides[iSide]->Duplicate(aSides[iSide]);

					for(int y = 0; y < Height; y++)
					{
						for(int x = 0; x < Width; x++)
						{
							CVec3Dfp32 CenterVector = CImageCubemapSampler::CreateSampleVector(iSide, (x - HalfWidth) / HalfWidth, (HalfHeight - y) / HalfHeight);
							CenterVector.Normalize();
							CVec3Dfp32 XAxis, YAxis;

							if(CenterVector * CVec3Dfp32(0, 1, 0) > 0.99f)
							{
								CenterVector.CrossProd(CVec3Dfp32(1, 0, 0), YAxis);
								YAxis.CrossProd(CenterVector, XAxis);
							}
							else
							{
								CVec3Dfp32(0, 1, 0).CrossProd(CenterVector, XAxis);
								CenterVector.CrossProd(XAxis, YAxis);
							}

							XAxis.SetLength(XAxisScale);
							YAxis.SetLength(YAxisScale);

							CVec4Dfp32 Pixel(0);
							for(int ky = -KernelMax; ky <= KernelMax; ky++)
							{
								for(int kx = -KernelMax; kx <= KernelMax; kx++)
								{
									CVec3Dfp32 SampleVector = CenterVector + (XAxis * (fp32)kx) + (YAxis *(fp32)ky);
									Pixel	+= (CVec4Dfp32)CubeSampler.SampleVector(SampleVector) * pCalcWeights[(KernelMax + ky) * KernelWidth + (KernelMax + kx)];
								}
							}

							Pixel	= Pixel * (1.0f / WeightSum);

							CVec2Dfp32 InVector(x / (fp32)Width, y / (fp32)Height);
							CVec2Dfp32 OutVector = CImage::TransformSampleVector(CubeSampler.GetSideTransform(iSide), InVector, 1.0f / Width, 1.0f / Height);

							aSides[iSide]->SetPixel(Cliprect, CPnt(OutVector.k[0] * Width, OutVector.k[1] * Height), CPixel32(Pixel));
						}
					}
				}

				// Since we got here a filter was used, create 6 new textures and replace the old ones with these

				for(int i = 0; i < 6; i++)
				{
					CTexture* pOldTexture = m_lspTextures[nTxt + nVersions * i];
					CTexture* pNewTexture = MNew(CTexture);

					strcpy(pNewTexture->m_Name, pOldTexture->m_Name);
					pNewTexture->m_TextureID	= pOldTexture->m_TextureID;
					pNewTexture->m_Properties		= pOldTexture->m_Properties;

					pNewTexture->Create(aSides[i], nMipmaps, aSides[i]->GetFormat());


					m_lspTextures[nTxt + nVersions * i]	= pNewTexture;
				}
			}
		}
	}
}
#endif
