#include "PCH.h"

#include "MFloat.h"
#include "WBSP3Model.h"
#include "WBSP3Def.h"

// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------

#define MODEL_BSP_LIGHTMAPFORMAT IMAGE_FORMAT_BGR8

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

static fp64 g_Time_BuildInto_Convert = 0;

// -------------------------------------------------------------------
TList_Vector<uint8> CTextureContainer_LMC::ms_lMixBuffer;
TList_Vector<uint8> CTextureContainer_LMC::ms_lMixBuffer2;

TList_Vector<spCImage> CTextureContainer_LMC::ms_lspPhongMaps;

// -------------------------------------------------------------------
CTextureContainer_LMC::CTextureContainer_LMC()
{
	MAUTOSTRIP(CTextureContainer_LMC_ctor, MAUTOSTRIP_VOID);
	m_pModel = NULL;
	m_LastInvalidatedLMC = 0;
}

CTextureContainer_LMC::~CTextureContainer_LMC()
{
	MAUTOSTRIP(CTextureContainer_LMC_dtor, MAUTOSTRIP_VOID);
	if (m_pModel)
	{
		int nLMC = m_pModel->m_lspLMC.Len();
		for(int i = 0; i < nLMC; i++)
			if (m_pModel->m_lspLMC[i]->m_TextureID)
				m_pTC->FreeID(m_pModel->m_lspLMC[i]->m_TextureID);
	}
}

void CTextureContainer_LMC::Create(CXR_Model_BSP3* _pModel)
{
	MAUTOSTRIP(CTextureContainer_LMC_Create, MAUTOSTRIP_VOID);
	m_pModel = _pModel;

//	m_lLightID.SetLen(256);
	// Build face-affect list.
#ifdef NEVER
	{
		int8 IDUsed[256];
		FillChar(&IDUsed, sizeof(IDUsed), 0);
		int nFaces = m_pModel->m_lFaces.Len();
		int nLM = m_pModel->m_lLightMapInfo.Len();
		CBSP3_Face* pFaces = &m_pModel->m_lFaces[0];
		CBSP3_LightMapInfo* pLM = &m_pModel->m_lLightMapInfo[0];
		for(int l = 0; l < nLM; l++) IDUsed[pLM[l].m_LightID] = TRUE;

		m_liFaceAffect.SetGrow(512);
		for(int ID = 0; ID < m_lLightID.Len(); ID++)
			if (IDUsed[ID])
			{
				m_lLightID[ID].m_iiFaceAffect = m_liFaceAffect.Len();
				for(int f = 0; f < nFaces; f++)
				{
					int iLM = pFaces[f].m_iLightInfo;
					int n = pFaces[f].m_nLightInfo;
					int iMax = m_pModel->m_lLightMapInfo.Len();

					// FIXME: Why is this needed?
					n = Min(n, iMax - iLM);
//					if (iLM+n-1 >= m_pModel->m_lLightMapInfo.Len())
//						Error("-", CStrF("Invalid lightmap references (%d..%d / %d) on face %d", iLM, iLM+n-1, m_pModel->m_lLightMapInfo.Len(), f));

					for(int l = 0; l < n; l++)
						if(pLM[iLM+l].m_LightID == ID)
						{
							m_liFaceAffect.Add(f);
							m_lLightID[ID].m_nFaceAffect++;
						}
				}
			}
	}
#endif
/*	{
		CBSP3_LightMapInfo* pLM = &m_pModel->m_lLightMapInfo[0];
		int nLMC = m_pModel->m_lspLMC.Len();
		for(int iLMC = 0; iLMC < nLMC; iLMC++)
		{
			CXW_LMCluster_Ext* pLMC = m_pModel->m_lspLMC[iLMC];
			for(int iE = 0; iE < pLMC->m_lElements.Len(); iE++)
			{
				int iLMI = GetLMI(iLMC, iE);
				int nLMI = GetNumLMI(iLMC, iE);
				for(int i = 0; i < nLMI; i++)
					pLMC->AddLightID(m_pModel->m_lLightMapInfo[iLMI + i].m_LightID);
			}

			pLMC->m_lLightIDs.OptimizeMemory();
//			LogFile(CStrF("(CTextureContainer_LMC::Create) Cluster %d, Num Elements %d, Num affecting light IDs %d", iLMC, pLMC->m_lElements.Len(), pLMC->m_lLightIDs.Len()));
		}

	}*/

	int nLMC = m_pModel->m_lspLMC.Len();
	for(int i = 0; i < nLMC; i++)
		m_pModel->m_lspLMC[i]->m_TextureID = m_pTC->AllocID(m_iTextureClass, i, (const char*)NULL);


//	InitBuffers();
//	InitPhongMap();
}

void CTextureContainer_LMC::InitBuffers()
{
	MAUTOSTRIP(CTextureContainer_LMC_InitBuffers, MAUTOSTRIP_VOID);
	int maxw = 0;
	int maxh = 0;
	for(int i = 0; i < m_pModel->m_lspLMC.Len(); i++)
	{
		maxw = MaxMT(m_pModel->m_lspLMC[i]->m_Width, maxw);
		maxh = MaxMT(m_pModel->m_lspLMC[i]->m_Height, maxh);

	}

	int maxsize = MaxMT(maxw*maxh*4, ms_lMixBuffer.Len());

	ms_lMixBuffer.SetLen(maxsize);
//	ms_lMixBuffer2.SetLen(maxsize);
}

void CTextureContainer_LMC::InitPhongMap()
{
	MAUTOSTRIP(CTextureContainer_LMC_InitPhongMap, MAUTOSTRIP_VOID);
	// Check if phongmaps has been created already
	if (ms_lspPhongMaps.Len() != 0)
		if (ms_lspPhongMaps[0]->GetFormat() != 0) return;

	const CVec3Dfp32 BumpNormals[5] =
	{
		CVec3Dfp32(0,0,1),
		CVec3Dfp32(1,0,0),
		CVec3Dfp32(0,1,0),
		CVec3Dfp32(-1,0,0),
		CVec3Dfp32(0,-1,0)
	};

	const fp32 LDist = 0.15f;
	CVec3Dfp32 L(0, 0, LDist);

	const fp32 Attn = Length2(1.0f, LDist);

	ms_lspPhongMaps.SetLen(5);
	for(int iBump = 0; iBump < 5; iBump++)
	{
		ms_lspPhongMaps[iBump] = DNew(CImage) CImage;
		if (ms_lspPhongMaps[iBump] == NULL) MemError("InitPhongMap");

		int Size = 64;
		ms_lspPhongMaps[iBump]->Create(Size, Size, IMAGE_FORMAT_BGRX8, IMAGE_MEM_IMAGE);

		CClipRect Clip = ms_lspPhongMaps[iBump]->GetClipRect();
		for (int x = 0; x < Size; x++)
			for (int y = 0; y < Size; y++)
			{
				CVec3Dfp32 P(x - (Size >> 1), y - (Size >> 1), 0);
				P *= -2.0f / Size;

				CVec3Dfp32 v;
				L.Sub(P, v);
				fp32 vlen = v.Length();
				v *= 1.0f/vlen;
				fp32 i = (v*BumpNormals[iBump]);
				if (i < 0.0f) i = 0.0f;
				//* (Max(Attn - vlen, 0.0f) / Attn);
//				i = Min(1.0f, i);


				i *= M_Cos(Min(1.0f, vlen / Attn) * _PI*0.5f);

/*				fp32 r = M_Sqrt(Sqr(x - (Size >> 1)) + Sqr(z - (Size >> 1))) / (fp32(Size >> 1));
				r = Min(1.0f, r);
	#ifdef MODEL_BSP_LIGHTATTENUATIONCOS
				fp32 i = cos(r * _PI * 0.5f);
	#else
				fp32 i = (cos(r * _PI) + 1) * 0.5;
	#endif*/
				CPixel32 Color(i*255.0f, i*255.0f, i*255.0f);
				ms_lspPhongMaps[iBump]->SetPixel(Clip, CPnt(x, y), Color);
			};

//		ms_lspPhongMaps[iBump]->Write(ms_lspPhongMaps[iBump]->GetRect(), CStrF("E:\\TEST\\PHONG%d.TGA", iBump) );

//		ms_lspPhongMaps[iBump]->Write("
	}
}

void CTextureContainer_LMC::PrepareFrame(spCXR_WorldLightState _spCurrentWLS)
{
	MAUTOSTRIP(CTextureContainer_LMC_PrepareFrame, MAUTOSTRIP_VOID);
	m_Time_BuildInto = 0;
	m_Time_BuildInto_Mixing = 0;
	g_Time_BuildInto_Convert = 0;
	m_Time_BuildInto_Trace = 0;
	m_Time_BuildInto_TraceAdm = 0;
	m_Time_BuildInto_TraceAdm2 = 0;
	m_Time_BuildInto_TraceAdm3 = 0;
	m_BuildInto_NumTrace = 0;
	m_BuildInto_NumSurfaces = 0;

	m_spCurrentWLS = _spCurrentWLS;


/*	CXR_LightID* pLS = &m_spCurrentWLS->m_lLightIDs[0];
	int nLightIDs = m_spCurrentWLS->m_lLightIDs.Len();

	int nLMC = m_pModel->m_lspLMC.Len();
	if (nLMC)
	{
		int iLMC = m_LastInvalidatedLMC;
		bool bBreak = false;
		do
		{
			iLMC++;
			if (iLMC >= nLMC) iLMC = 0;
			CXW_LMCluster_Ext* pLMC = m_pModel->m_lspLMC[iLMC];
			CXR_LMC_LightID* pIDs = pLMC->m_lLightIDs.GetBasePtr();
			for(int i = 0; i < pLMC->m_lLightIDs.Len(); i++)
			{
				int ID = pIDs[i].m_LightID;
				if (pIDs[i].m_LastIntensity != pLS[ID].m_IntensityInt32)
				{
ConOutL(CStrF("Invalidated LMC %d, ID %d, Lights %.8x -> %.8x", iLMC, pLMC->m_TextureID, pIDs[i].m_LastIntensity, pLS[ID].m_IntensityInt32));
					pIDs[i].m_LastIntensity = pLS[ID].m_IntensityInt32;
					m_LastInvalidatedLMC = iLMC;
					m_pTC->MakeDirty(pLMC->m_TextureID);
					bBreak = true;
				}
			}
			if (bBreak) break;
		}
		while(iLMC != m_LastInvalidatedLMC);
	}
*/

/*	for(int ID = 1; ID < nLightIDs; ID++)
	{
		CXR_LMLightIDInfo* pLID = &m_lLightID[ID];
		if (pLS[ID].m_IntensityInt32 != pLID->m_LastIntensity)
		{
			pLID->m_LastIntensity = pLS[ID].m_IntensityInt32;

			int nAffect = pLID->m_nFaceAffect;
			if (nAffect)
			{
				int iiAffect = pLID->m_iiFaceAffect;
				uint32* piFaces = &m_liFaceAffect[0];
				fp64 Time;
				T_Start(Time);
				for (int f = 0; f < nAffect; f++)
				{
					m_pTC->MakeDirty(GetFaceLMCTextureID(f));
				}
				T_Stop(Time);
			}
		}
	}*/
}

int CTextureContainer_LMC::GetNumLocal()
{
	MAUTOSTRIP(CTextureContainer_LMC_GetNumLocal, 0);
	return m_pModel->m_lspLMC.Len();
}

int CTextureContainer_LMC::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetTextureID, 0);
	if (m_pModel->m_lspLMC.ValidPos(_iLocal))
		return m_pModel->m_lspLMC[_iLocal]->m_TextureID;
	else
		Error("GetTextureID", CStrF("Invalid index %d/%d", _iLocal, m_pModel->m_lspLMC.Len()));
		
	return 0;
};

int CTextureContainer_LMC::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetTextureDesc, 0);
	if (!m_pModel->m_lspLMC.ValidPos(_iLocal))
		Error("GetTextureDesc", CStrF("Invalid index %d/%d", _iLocal, m_pModel->m_lspLMC.Len()));

	_Ret_nMipmaps = 1;
	CXW_LMCluster* pLMC = m_pModel->m_lspLMC[_iLocal];
	_pTargetImg->CreateVirtual(pLMC->m_Width, pLMC->m_Height, MODEL_BSP_LIGHTMAPFORMAT, IMAGE_MEM_TEXTURE);
	return CTC_TEXTURE_NOWRAP;

/*	CXR_LMInfo* pLMI = &m_lLMInfo[_iLocal / 5];
	_pTargetImg->CreateVirtual(pLMI->m_TxtWidth, pLMI->m_TxtHeight, pLMI->m_Format, IMAGE_MEM_TEXTURE);
	_Ret_nMipmaps = 1;
	return CTC_TEXTURE_NOWRAP;*/
};

void CTextureContainer_LMC::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetTextureProperties, MAUTOSTRIP_VOID);
	CTC_TextureProperties Properties;
	Properties.m_Flags = CTC_TEXTUREFLAGS_NOMIPMAP | CTC_TEXTUREFLAGS_NOPICMIP | CTC_TEXTUREFLAGS_NOCOMPRESS;
	Properties.m_MagFilter = CTC_MAGFILTER_LINEAR;
	Properties.m_MinFilter = CTC_MINFILTER_LINEAR;
	Properties.m_MIPFilter = CTC_MIPFILTER_NEAREST;
	Properties.m_iPicMip = 9;
	_Properties = Properties;
}

/*
int CTextureContainer_LMC::GetLMI(int _iLMC, int _iElem)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetLMI, 0);
	const CXW_LMCElement* pElem = &m_pModel->m_lspLMC[_iLMC]->m_lElements[_iElem];
	switch(pElem->m_ElementType)
	{
	case XW_LMCLUSTER_FACE :
		{
			return m_pModel->m_lFaces[pElem->m_iElement].m_iLightInfo;
		}
		break;
	case XW_LMCLUSTER_SB :
		{
			int iSB = pElem->m_iElement & 0xffff;
			int iFace = pElem->m_iElement >> 16;

			CSplineBrush* pSB = m_pModel->m_lspSplineBrushes[iSB];
			CSplineBrush_Face* pF= &pSB->m_lFaces[iFace];
			return pF->m_iLightInfo;
		}
		break;
	default:
		Error("GetLMI", CStrF("Invalid element type. (iElem %d, Type %d, iLMC %d)", _iElem, pElem->m_ElementType, _iLMC));
	}
	return 0;
}

int CTextureContainer_LMC::GetNumLMI(int _iLMC, int _iElem)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetNumLMI, 0);
	const CXW_LMCElement* pElem = &m_pModel->m_lspLMC[_iLMC]->m_lElements[_iElem];
	switch(pElem->m_ElementType)
	{
	case XW_LMCLUSTER_FACE :
		{
			return m_pModel->m_lFaces[pElem->m_iElement].m_nLightInfo;
		}
		break;
	case XW_LMCLUSTER_SB :
		{
			int iSB = pElem->m_iElement & 0xffff;
			int iFace = pElem->m_iElement >> 16;

			CSplineBrush* pSB = m_pModel->m_lspSplineBrushes[iSB];
			CSplineBrush_Face* pF= &pSB->m_lFaces[iFace];
			return pF->m_nLightInfo;
		}
		break;
	default:
		Error("GetNumLMI", "Invalid element type.");
	}
	return 0;
}
*/

/*
int CTextureContainer_LMC::GetFaceLMC(int _iFace)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetFaceLMC, 0);
	return m_pModel->m_lLightMapInfo[m_pModel->m_lFaces[_iFace].m_iLightInfo].m_iLMC;
}

int CTextureContainer_LMC::GetFaceLMCTextureID(int _iFace)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetFaceLMCTextureID, 0);
	return m_pModel->m_lspLMC[m_pModel->m_lLightMapInfo[m_pModel->m_lFaces[_iFace].m_iLightInfo].m_iLMC]->m_TextureID;
}

int CTextureContainer_LMC::GetSBFaceLMC(const CSplineBrush_Face* pF)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetSBFaceLMC, 0);
	return m_pModel->m_lLightMapInfo[pF->m_iLightInfo].m_iLMC;
}

int CTextureContainer_LMC::GetSBFaceLMCTextureID(const CSplineBrush_Face* pF)
{
	MAUTOSTRIP(CTextureContainer_LMC_GetSBFaceLMCTextureID, 0);
	return m_pModel->m_lspLMC[m_pModel->m_lLightMapInfo[pF->m_iLightInfo].m_iLMC]->m_TextureID;
}
*/

void CTextureContainer_LMC::BuildLMI(int _iLMI, int _nLMI, CImage* _pImg, CClipRect& _Clip)
{
	MAUTOSTRIP(CTextureContainer_LMC_BuildLMI, MAUTOSTRIP_VOID);
	CXR_LightID* pLightIDs = (m_spCurrentWLS != NULL) ? m_spCurrentWLS->m_lLightIDs.GetBasePtr() : NULL;

	const CBSP3_LightMapInfo* pLMI = &m_pModel->m_lLightMapInfo[_iLMI];
	CPnt Pos(pLMI->m_LMCOffsetXHalf*2, pLMI->m_LMCOffsetYHalf*2);

	// #black
/*	{
		int w = m_pModel->m_lspLightMaps[_iLMI]->GetWidth();
		int h = m_pModel->m_lspLightMaps[_iLMI]->GetHeight();
		_pImg->Rect(_Clip, CRct(Pos.x, Pos.y, Pos.x + w, Pos.y + h), 0x000000);
	}*/

/*LogFile(CStrF("     Element %d, Clip (%d,%d,%d,%d)(%d,%d), Pos (%d,%d)", _iLMI, 
		_Clip.clip.p0.x, _Clip.clip.p0.y, _Clip.clip.p1.x, _Clip.clip.p1.y, _Clip.ofs.x, _Clip.ofs.y,
		Pos.x, Pos.y));*/

	for(int i = 0; i < _nLMI; i++)
	{
		int iLM = _iLMI + i;
		const CBSP3_LightMapInfo* pLMI = &m_pModel->m_lLightMapInfo[iLM];

		CImage LMRef;
		m_pModel->m_spLightMapContainer->GetLightMap(iLM, LMRef);

		CImage* pLM = &LMRef;

//		int LightID = pLMI->m_LightID;
//		CPixel32 Col = (pLightIDs) ? pLightIDs[LightID].m_IntensityInt32 & 0xffffff : 0xffffff;

		CPixel32 Col = 0xffffff;
		if (!i)
		{
			if (Col == 0xffffff)
				_pImg->Blt(_Clip, *pLM, 0, Pos);
			else
				_pImg->Blt(_Clip, *pLM, IMAGE_OPER_MULADDSATURATE, Pos, Col);
		}
		else
		{
//			_pImg->Blt(_Clip, *pLM, 0, Pos);
			if (Col == 0xffffff)
				_pImg->Blt(_Clip, *pLM, IMAGE_OPER_ADDSATURATE, Pos);
			else
				_pImg->Blt(_Clip, *pLM, IMAGE_OPER_MULADDSATURATE, Pos, Col);
		}
	}

}

/*
void CTextureContainer_LMC::BuildFace(int _iFace, CImage* _pImg, CClipRect& _Clip)
{
	MAUTOSTRIP(CTextureContainer_LMC_BuildFace, MAUTOSTRIP_VOID);
	CBSP3_Face* pFace = &m_pModel->m_lFaces[_iFace];
	BuildLMI(pFace->m_iLightInfo, pFace->m_nLightInfo, _pImg, _Clip);
}

void CTextureContainer_LMC::BuildSB(int _iElem, CImage* _pImg, CClipRect& _Clip)
{
	MAUTOSTRIP(CTextureContainer_LMC_BuildSB, MAUTOSTRIP_VOID);
	int iSB = _iElem & 0xffff;
	int iFace = _iElem >> 16;

	CSplineBrush* pSB = m_pModel->m_lspSplineBrushes[iSB];
	CSplineBrush_Face* pF= &pSB->m_lFaces[iFace];

	BuildLMI(pF->m_iLightInfo, pF->m_nLightInfo, _pImg, _Clip);
}
*/

void CTextureContainer_LMC::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _ConvertType, int _iStartMip)
{
	MAUTOSTRIP(CTextureContainer_LMC_BuildInto, MAUTOSTRIP_VOID);
//	if (!m_pWLS) Error("BuildInto", "PrepareFrame() has not been called.");

	if (!m_pModel->m_lspLMC.ValidPos(_iLocal))
		Error("BuildInto", CStrF("Invalid index %d/%d", _iLocal, m_pModel->m_lspLMC.Len()));

	m_Time_BuildInto += -GetCPUClock();
	if (_nMipmaps != 1) Error("BuildInto", CStrF("nMipmaps == %d", _nMipmaps));
	if (_iStartMip != 0) Error("BuildInto", CStrF("_iStartMip == %d", _iStartMip));

//LogFile(CStrF("Building %d, %d, %d", _iLocal, _iLocal / 5, _iLocal % 5));
	int iBump = 0;

	CXW_LMCluster_Ext* pLMC = m_pModel->m_lspLMC[_iLocal];

	// Set all last light IDs to current
/*	if (m_pWLS)
	{
		CXR_LMC_LightID* pIDs = pLMC->m_lLightIDs.GetBasePtr();
		for(int i = 0; i < pLMC->m_lLightIDs.Len(); i++)
		{
			int ID = pIDs[i].m_LightID;
			pIDs[i].m_LastIntensity = m_pWLS->m_lLightIDs[ID].m_IntensityInt32;
		}
	}*/


	// Create refence image
	CImage MixBuffer;
	int Width = pLMC->m_Width;
	int Height = pLMC->m_Height;
//	MixBuffer.CreateReference(Width, Height, MODEL_BSP_LIGHTMAPFORMAT, IMAGE_MEM_IMAGE, Width * 4, ms_lMixBuffer.GetBasePtr(), NULL);

//	if (_ppImg[0]->GetPixelSize() != 4)
//		Error("BuildInto", "Target image must be 32-bit per pixel.");

	void* pBuildImg = _ppImg[0]->Lock();
	int Modulo = _ppImg[0]->GetModulo();
	MixBuffer.CreateReference(Width, Height, MODEL_BSP_LIGHTMAPFORMAT, IMAGE_MEM_IMAGE, Modulo, pBuildImg, NULL);
	_ppImg[0]->Unlock();

	// #green
	CClipRect Clip = MixBuffer.GetClipRect();
	MixBuffer.Rect(Clip, CRct(0, 0, Width, Height), 0x000000);

	for(int iElem = 0; iElem < pLMC->m_lElements.Len(); iElem++)
	{
		const CXW_LMCElement& Elem = pLMC->m_lElements[iElem];
		BuildLMI(Elem.GetLightInfoI(), Elem.GetLightInfoN(), &MixBuffer, Clip);
	}

//	MixBuffer.Write(CRct(0,0,-1,-1), CStrF("s:\\LightMap%.4x.tga", _iLocal));

/*	for(int iElem = 0; iElem < pLMC->m_lElements.Len(); iElem++)
	{
		const CXW_LMCElement* pElem = &pLMC->m_lElements[iElem];
		switch(pElem->m_ElementType)
		{
		case XW_LMCLUSTER_FACE :
			{
				BuildFace(pElem->m_iElement, &MixBuffer, Clip);
			}
			break;
		case XW_LMCLUSTER_SB :
			{
				BuildSB(pElem->m_iElement, &MixBuffer, Clip);
			}
			break;
		}
	}*/

	g_Time_BuildInto_Convert += -GetCPUClock();
	if (_ppImg[0]->GetFormat() != MODEL_BSP_LIGHTMAPFORMAT)
	{
		// NOTE: Source and destination is the same image data.
		CImage::Convert(&MixBuffer, _ppImg[0], _ConvertType);
	}

	T_Stop(g_Time_BuildInto_Convert);
	T_Stop(m_Time_BuildInto);

//	srand(_iLocal);
//	_ppImg[0]->Fill(_ppImg[0]->GetClipRect(), CPixel32(Random*128+32, Random*128+32, Random*128+32));
};

