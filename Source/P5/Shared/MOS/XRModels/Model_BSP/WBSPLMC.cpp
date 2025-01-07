#include "PCH.h"

#include "MFloat.h"
#include "WBSPModel.h"
#include "WBSPDef.h"

// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------

#define MODEL_BSP_LIGHTMAPFORMAT IMAGE_FORMAT_BGR8

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

static fp64 g_Time_BuildInto_Convert = 0;

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

void CTextureContainer_LMC::Create(CXR_Model_BSP* _pModel)
{
	MAUTOSTRIP(CTextureContainer_LMC_Create, MAUTOSTRIP_VOID);
	m_pModel = _pModel;

	int nLMC = m_pModel->m_lspLMC.Len();
	for(int i = 0; i < nLMC; i++)
		m_pModel->m_lspLMC[i]->m_TextureID = m_pTC->AllocID(m_iTextureClass, i, (const char*)NULL);

}


void CTextureContainer_LMC::PrepareFrame(spCXR_WorldLightState _spCurrentWLS)
{
	MAUTOSTRIP(CTextureContainer_LMC_PrepareFrame, MAUTOSTRIP_VOID);
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
	Properties.m_iPicMipGroup = 9;
	_Properties = Properties;
}

void CTextureContainer_LMC::BuildLMI(int _iLMI, int _nLMI, CImage* _pImg, CClipRect& _Clip)
{
	MAUTOSTRIP(CTextureContainer_LMC_BuildLMI, MAUTOSTRIP_VOID);
	CXR_LightID* pLightIDs = (m_spCurrentWLS != NULL) ? m_spCurrentWLS->m_lLightIDs.GetBasePtr() : NULL;

	const CBSP_LightMapInfo* pLMI = &m_pModel->m_lLightMapInfo[_iLMI];
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
		const CBSP_LightMapInfo* pLMI = &m_pModel->m_lLightMapInfo[iLM];

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

