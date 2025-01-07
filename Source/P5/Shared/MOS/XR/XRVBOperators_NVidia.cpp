
#include "PCH.h"

#include "XRVBOperators_NVidia.h"
#include "../MSystem/Raster/MRender_NVidia.h"
#include "MFloat.h"


MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_NV20_GenEnv, CXR_VBOperator_Util);

#define VP_PIPELINE

bool CXR_VBOperator_NV20_GenEnv::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_NV20_GenEnv_OnOperate, false);
	// Assign the required geometry.
	// Obsolete since we always copy all texcoord arrays to the final VB.
	if (!_pVB->IsVBIDChain())
	{
		CXR_VBChain *pChain = _pVB->GetVBChain();
		CXR_VBChain *pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
		if (!pChain->m_pTV[1]) pChain->m_pTV[1] = pChainSrc->m_pTV[2];	// Just to fill out the texcoord set so the RC doesn't cut texcoord 2 & 3.
/*		_pVB->m_pTV[2] = _pSrcVB->m_pTV[2];
		_pVB->m_pTV[3] = _pSrcVB->m_pTV[3];
		_pVB->m_nTVComp[2] = _pSrcVB->m_nTVComp[2];
		_pVB->m_nTVComp[3] = _pSrcVB->m_nTVComp[3];*/
	}

	if(!_Context.m_pEngine)
		return false;

	if (!_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No attributes.");
		return false;
	}

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{

//		_pVB->m_pAttrib->Attrib_TextureID(1, _pVB->m_pAttrib->m_TextureID[1]);
//		_pVB->m_pAttrib->Attrib_TextureID(1, _pVB->m_pAttrib->m_TextureID[0]);
//		_pVB->m_pAttrib->Attrib_TextureID(2, _pVB->m_pAttrib->m_TextureID[0]);
//		_pVB->m_pAttrib->m_iTexCoordSet[2] = 1;
//		_pVB->m_pAttrib->m_iTexCoordSet[3] = 2;

		CRC_ExtAttributes_FragmentProgram20* pAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(8+1);
		if (!pParams) 
			return false;

		pAttr->m_pProgramName = "VBOp_GenEnv";
		pParams[8] = 1.0f / 255.0f;
		pAttr->SetParameters(&pParams[8], 1);

		// Set program
		CMat4Dfp32 WMat, VMat, VMatInv;
		WMat.Unit();
		VMat.Unit();
		if (_Context.m_pModel2World) 
			WMat = *_Context.m_pModel2World;
		else
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2W matrix.");
		VMatInv = _Context.m_pEngine->GetVC()->m_CameraWMat;

		WMat.Transpose();

		fp32 BumpScale = GetParam(_Oper, 0, 0.1f);

		CVec3Dfp32(1, BumpScale, BumpScale).SetRow(WMat, 3);

		memcpy(pParams[0].k, &WMat, sizeof(WMat));
		memcpy(pParams[4].k, &VMatInv, sizeof(VMatInv));

		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_BUMPCUBEENV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

		_pVB->m_pAttrib->m_pExtAttrib = pAttr;

		if(_pVB->m_pAttrib->m_SourceDestBlend == MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTALPHA, CRC_BLEND_INVDESTALPHA))
		{
			_pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_INVDESTALPHA);
			_pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_DESTALPHA);
			_pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND);
		}

		M_VSt_V4f32_Pixel32(M_VMul(M_VLd_Pixel32_f32(&_pVB->m_Color), M_VScalar(0.5f)), &_pVB->m_Color);
//		_pVB->m_Color *= 0.5f;

		_Context.m_iTexChannelNext = 2;
		return true;
	}
	else if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM14)
	{

		_pVB->m_pAttrib->Attrib_TextureID(3, _pVB->m_pAttrib->m_TextureID[1]);
		_pVB->m_pAttrib->Attrib_TextureID(1, _pVB->m_pAttrib->m_TextureID[0]);
		_pVB->m_pAttrib->Attrib_TextureID(2, _pVB->m_pAttrib->m_TextureID[0]);
//		_pVB->m_pAttrib->m_iTexCoordSet[2] = 1;
//		_pVB->m_pAttrib->m_iTexCoordSet[3] = 2;

		CRC_ExtAttributes_FragmentProgram14* pAttr = (CRC_ExtAttributes_FragmentProgram14*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram14));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(8+1);
		if (!pParams) 
			return false;

		pAttr->m_pProgramName = "VBOp_GenEnv";
		pParams[8] = 1.0f / 255.0f;
		pAttr->SetParameters(&pParams[8], 1);

		// Set program
		CMat4Dfp32 WMat, VMat, VMatInv;
		WMat.Unit();
		VMat.Unit();
		if (_Context.m_pModel2World) 
			WMat = *_Context.m_pModel2World;
		else
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2W matrix.");
		VMatInv = _Context.m_pEngine->GetVC()->m_CameraWMat;

		WMat.Transpose();

		fp32 BumpScale = GetParam(_Oper, 0, 0.1f);

		CVec3Dfp32(1, BumpScale, BumpScale).SetRow(WMat, 3);

		memcpy(pParams[0].k, &WMat, sizeof(WMat));
		memcpy(pParams[4].k, &VMatInv, sizeof(VMatInv));

		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_BUMPCUBEENV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(4, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

		_pVB->m_pAttrib->m_pExtAttrib = pAttr;

		if(_pVB->m_pAttrib->m_SourceDestBlend == MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTALPHA, CRC_BLEND_INVDESTALPHA))
		{
			_pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_INVDESTALPHA);
			_pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_DESTALPHA);
			_pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND);
		}
		M_VSt_V4f32_Pixel32(M_VMul(M_VLd_Pixel32_f32(&_pVB->m_Color), M_VScalar(0.5f)), &_pVB->m_Color);

		_Context.m_iTexChannelNext = 4;
		return true;
	}
	else if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{

		_pVB->m_pAttrib->Attrib_TextureID(3, _pVB->m_pAttrib->m_TextureID[1]);
		_pVB->m_pAttrib->Attrib_TextureID(1, _pVB->m_pAttrib->m_TextureID[0]);
		_pVB->m_pAttrib->Attrib_TextureID(2, _pVB->m_pAttrib->m_TextureID[0]);

		CRC_ExtAttributes_FragmentProgram11* pAttr = (CRC_ExtAttributes_FragmentProgram11*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(8);
		if (!pParams) 
			return false;

		pAttr->m_pProgramName = "VBOp_GenEnv";
		pAttr->m_Colors[0] = 0xffffffff;

		// Set program
		CMat4Dfp32 WMat, VMat, VMatInv;
		WMat.Unit();
		VMat.Unit();
		if (_Context.m_pModel2World) 
			WMat = *_Context.m_pModel2World;
		else
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2W matrix.");
		VMatInv = _Context.m_pEngine->GetVC()->m_CameraWMat;

		WMat.Transpose();

		fp32 BumpScale = GetParam(_Oper, 0, 0.1f);

		CVec3Dfp32(1, BumpScale, BumpScale).SetRow(WMat, 3);

		memcpy(pParams[0].k, &WMat, sizeof(WMat));
		memcpy(pParams[4].k, &VMatInv, sizeof(VMatInv));

		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_BUMPCUBEENV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

		if(_pVB->m_pAttrib->m_SourceDestBlend == MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTALPHA, CRC_BLEND_INVDESTALPHA))
		{
			_pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_INVDESTALPHA);
			_pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_DESTALPHA);
			_pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND);
		}
		M_VSt_V4f32_Pixel32(M_VMul(M_VLd_Pixel32_f32(&_pVB->m_Color), M_VScalar(0.5f)), &_pVB->m_Color);

		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		_Context.m_iTexChannelNext = 4;
		return true;
	}
#ifdef SUPPORT_REGISTERCOMBINERS
	else
#endif
#endif
#ifdef SUPPORT_REGISTERCOMBINERS
	if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
	{

	//	_pVB->m_pAttrib->Attrib_TextureID(0, _Context.m_pEngine->m_pRender->Texture_GetTC()->GetTextureID("SPECIAL_TESTBUMP"));
		_pVB->m_pAttrib->Attrib_TextureID(3, _pVB->m_pAttrib->m_TextureID[1]);
		_pVB->m_pAttrib->Attrib_TextureID(1, _pVB->m_pAttrib->m_TextureID[0]);
		_pVB->m_pAttrib->Attrib_TextureID(2, _pVB->m_pAttrib->m_TextureID[0]);
	//	_pVB->m_pAttrib->Attrib_TextureID(3, _Context.m_pEngine->m_pRender->Texture_GetTC()->GetTextureID("CUBE_02_0"));
	//	_pVB->m_pAttrib->Attrib_TextureID(3, _Context.m_pEngine->m_pRender->Texture_GetTC()->GetTextureID("SPECIAL_NORMALIZE0"));

	//ConOut(CStrF("TexIDs %d, %d, %d, %d", 
	//	   _pVB->m_pAttrib->m_TextureID[0], _pVB->m_pAttrib->m_TextureID[1],
	//	   _pVB->m_pAttrib->m_TextureID[2], _pVB->m_pAttrib->m_TextureID[3]));

		CRC_ExtAttributes_NV20* pAttr = (CRC_ExtAttributes_NV20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV20));
		if (!pAttr) return false;
		pAttr->Clear();
		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(8);
		if (!pParams) return false;
	#ifndef VP_PIPELINE
		pAttr->m_pProgramParams = pParams;
	#endif

		pAttr->Clear();
		pAttr->Clear(0);
		pAttr->SetFinal(NV_INPUT_TEXTURE3, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_PRIMARY|NV_COMP_ALPHA);
		pAttr->SetConst0(0xffffffff);
		pAttr->SetNumCombiners(1);

		// Set texture shader
		pAttr->SetTexureOps(NV_TS_TEXTURE_2D, NV_TS_DOT_PRODUCT_NV, NV_TS_DOT_PRODUCT_NV, NV_TS_DOT_PRODUCT_REFLECT_CUBE_MAP_NV);
		pAttr->Fixup();

		// Set program
	#ifndef VP_PIPELINE
		pAttr->m_pProgramName = "NV20_GenEnv";
	#endif
		CMat4Dfp32 WMat, VMat, VMatInv;
		WMat.Unit();
		VMat.Unit();
		if (_Context.m_pModel2World) 
			WMat = *_Context.m_pModel2World;
		else
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2W matrix.");
		VMatInv = _Context.m_pEngine->GetVC()->m_CameraWMat;

		WMat.Transpose();
	//	ConOut(CStrF("WMat = %s", (char*)WMat.GetString() ));
	//	ConOut(CStrF("WCam = %s", (char*)CVec3Dfp32::GetRow(VMatInv, 3).GetString() ));

		fp32 BumpScale = GetParam(_Oper, 0, 0.1f);

	//	fp32 BumpScale = M_Sin(0.5 * GetCPUClock()/GetCPUFrequency()) + 1.0f;
		CVec3Dfp32(1, BumpScale, BumpScale).SetRow(WMat, 3);

		memcpy(pParams[0].k, &WMat, sizeof(WMat));
		memcpy(pParams[4].k, &VMatInv, sizeof(VMatInv));

		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_BUMPCUBEENV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

		if(_pVB->m_pAttrib->m_SourceDestBlend == MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTALPHA, CRC_BLEND_INVDESTALPHA))
		{
			_pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_INVDESTALPHA);
			_pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_DESTALPHA);
			_pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND);
		}
		M_VSt_V4f32_Pixel32(M_VMul(M_VLd_Pixel32_f32(&_pVB->m_Color), M_VScalar(0.5f)), &_pVB->m_Color);

	//	ConOut(CStrF("(NV20_GenEnv) %.8x, Prior %f", _pVB->m_pV, _pVB->m_Priority));

	#ifndef VP_PIPELINE
		pAttr->m_nProgramParams = 8;
		pAttr->m_iProgramParams = 12;
	#endif

		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		_Context.m_iTexChannelNext = 4;
		return true;
	}
#endif
	else if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_TEXENVMODE_COMBINE)
	{
		_pVB->m_pAttrib->Attrib_TextureID(0, _pVB->m_pAttrib->m_TextureID[1]);
		_pVB->m_pAttrib->Attrib_TextureID(1, 0);
		_pVB->m_pAttrib->Attrib_TextureID(2, 0);
		_pVB->m_pAttrib->Attrib_TextureID(3, 0);

		_pVB->m_pAttrib->Attrib_TexGen(0, CRC_TEXGENMODE_REFLECTION, CRC_TEXGENCOMP_ALL);
		if(_pVB->m_pAttrib->m_SourceDestBlend == MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTALPHA, CRC_BLEND_INVDESTALPHA))
		{
			_pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_INVDESTALPHA);
			_pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_DESTALPHA);
			_pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND);
		}
		M_VSt_V4f32_Pixel32(M_VMul(M_VLd_Pixel32_f32(&_pVB->m_Color), M_VScalar(0.5f)), &_pVB->m_Color);

		// If worls-space transform is known we transform the generated reflection vectors to world-space.
		if (_Context.m_pWorld2View)
		{
			CMat4Dfp32* pW2VInv = _Context.m_pVBM->Alloc_M4();
			if (!pW2VInv)
				return false;
			_Context.m_pWorld2View->InverseOrthogonal(*pW2VInv);
			CVec3Dfp32::GetRow(*pW2VInv, 3) = 0;

			if (!_pVB->AllocTextureMatrix(_Context.m_pVBM))
				return false;

			_pVB->TextureMatrix_Set(0, pW2VInv);
		}
		else
			ConOutD("(CXR_VBOperator_GenReflection::OnOperate) WARNING: No world-2-view transform available.");

		_Context.m_iTexChannelNext = 4;
		return true;
	}
	return false;
}

bool CXR_VBOperator_NV20_GenEnv::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_GenEnv_OnTestHWAccelerated, false);
	return true;
}

void CXR_VBOperator_NV20_GenEnv::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_GenEnv_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDTANGENTS;
//	_pSurf->m_Requirements |= XW_SURFREQ_NV20;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_RMBM2D
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_NV20_RMBM2D, CXR_VBOperator);

/*static bool SetTexGenUVW(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, int _iTxtChannel, const CPlane3Dfp32& _U, const CPlane3Dfp32& _V, const CPlane3Dfp32& _W)
{
	MAUTOSTRIP(SetTexGenUVW, false);
	fp32* pTexGenU = _pVBM->Alloc_fp32(4);
	fp32* pTexGenV = _pVBM->Alloc_fp32(4);
	fp32* pTexGenW = _pVBM->Alloc_fp32(4);
	if (!pTexGenU || !pTexGenV || !pTexGenW) return false;

	pTexGenU[0] = _U.n[0];		pTexGenU[1] = _U.n[1];		pTexGenU[2] = _U.n[2];		pTexGenU[3] = _U.d;
	pTexGenV[0] = _V.n[0];		pTexGenV[1] = _V.n[1];		pTexGenV[2] = _V.n[2];		pTexGenV[3] = _V.d;
	pTexGenW[0] = _W.n[0];		pTexGenW[1] = _W.n[1];		pTexGenW[2] = _W.n[2];		pTexGenW[3] = _W.d;

	_pVB->m_pAttrib->Attrib_TexGenU(_iTxtChannel, CRC_TEXGENMODE_LINEAR, pTexGenU);
	_pVB->m_pAttrib->Attrib_TexGenV(_iTxtChannel, CRC_TEXGENMODE_LINEAR, pTexGenV);
	_pVB->m_pAttrib->Attrib_TexGenW(_iTxtChannel, CRC_TEXGENMODE_LINEAR, pTexGenW);

	return true;
}
*/

bool CXR_VBOperator_NV20_RMBM2D::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_NV20_RMBM2D_OnOperate, false);

	if (!_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_NV20_RMBM2D::OnOperate) No attributes.");
		return false;
	}

	_pVB->m_pAttrib->Attrib_TextureID(2, _pVB->m_pAttrib->m_TextureID[1]);

	fp32 BumpScale = GetParam(_Oper, 0, 0.1f);

	CMat4Dfp32* pMatTexGen = (CMat4Dfp32*)_Context.m_pVBM->Alloc(sizeof(CMat4Dfp32) * 2);
	if (!pMatTexGen) return false;

	pMatTexGen[0].Unit();
	pMatTexGen[0].k[3][1] = BumpScale;
	pMatTexGen[0].k[1][1] = 0;
	pMatTexGen[0].k[2][2] = 0;

	pMatTexGen[1].Unit();
	pMatTexGen[1].k[3][2] = BumpScale;
	pMatTexGen[1].k[0][0] = 0;
	pMatTexGen[1].k[1][0] = 1;
	pMatTexGen[1].k[1][1] = 0;
	pMatTexGen[1].k[2][2] = 0;

	_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pMatTexGen);
	_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
	_pVB->m_pAttrib->Attrib_TexCoordSet(2, 1);
	_pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		CRC_ExtAttributes_FragmentProgram20* pAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		pAttr->m_pProgramName = "VBOp_RMBM2D";


		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		return true;
	}
	else if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{
		CRC_ExtAttributes_FragmentProgram11* pAttr = (CRC_ExtAttributes_FragmentProgram11*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		pAttr->m_pProgramName = "VBOp_RMBM2D";


		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		return true;
	}
#ifdef SUPPORT_REGISTERCOMBINERS
	else
#endif
#endif
#ifdef SUPPORT_REGISTERCOMBINERS
	{
		CRC_ExtAttributes_NV20* pAttr = (CRC_ExtAttributes_NV20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV20));
		if (!pAttr) return false;
		pAttr->Clear();

		// Set combiners
		pAttr->Clear(0);
		pAttr->SetFinal(NV_INPUT_TEXTURE2, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_PRIMARY|NV_COMP_ALPHA);
		pAttr->SetConst0(0xffffffff);
		pAttr->SetNumCombiners(0);

		// Set texture shader
		pAttr->SetTexureOps(NV_TS_TEXTURE_2D, NV_TS_DOT_PRODUCT_NV, NV_TS_DOT_PRODUCT_TEXTURE_2D_NV, NV_TS_PASS_THROUGH_NV);
		pAttr->Fixup();

		// Set program
		fp32 BumpScale = GetParam(_Oper, 0, 0.1f);

#ifdef VP_PIPELINE

#else
		pAttr->m_pProgramName = "NV20_RMBM";
		pAttr->m_pProgramParams = _Context.m_pVBM->Alloc_V4(2);
		if (!pAttr->m_pProgramParams) return false;

		//	fp32 BumpScale = 0.1f*(M_Sin(0.5 * GetCPUClock()/GetCPUFrequency()) + 1.0f);
		//	fp32 BumpScale = 0.05f;

		pAttr->m_pProgramParams[0].k[1] = BumpScale;
		pAttr->m_pProgramParams[0].k[2] = 0.0f;
		pAttr->m_pProgramParams[1].k[1] = 0.0f;
		pAttr->m_pProgramParams[1].k[2] = BumpScale;

		pAttr->m_nProgramParams = 2;
		pAttr->m_iProgramParams = 12;
#endif
		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		return true;
	}
#endif
	return true;
}

bool CXR_VBOperator_NV20_RMBM2D::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_RMBM2D_OnTestHWAccelerated, false);
	return true;
}

void CXR_VBOperator_NV20_RMBM2D::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_RMBM2D_OnInitSurface, MAUTOSTRIP_VOID);
//	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
//	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDTANGENTS;
	_pSurf->m_Requirements |= XW_SURFREQ_NV20;
}

#if 0
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_Transform
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_VBOperator_NV20_Transform::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_NV20_Transform__OnOperate, false);
	// Only the head of a chain has attributes
	if (_Context.m_VBChainIndex) return true;

	if (!_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No attributes.");
		return false;
	}

	CRC_ExtAttributes_NV20* pAttr = (CRC_ExtAttributes_NV20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV20));
	if (!pAttr) return false;

	pAttr->Clear();
	pAttr->m_bRegCombinersEnable = false;
	pAttr->m_bTexShaderEnable = false;

	// Set program
	pAttr->m_pProgramName = "NV20_Transform";
	_pVB->m_pAttrib->m_pExtAttrib = pAttr;
	return true;
}

bool CXR_VBOperator_NV20_Transform::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_Transform_OnTestHWAccelerated, false);
	return true;
}
#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_MultiTxt
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_MultiTxt, CXR_VBOperator_Util);

bool CXR_VBOperator_MultiTxt::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_MultiTxt_OnOperate, false);

	if (!_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_MultiTxt::OnOperate) No attributes.");
		return false;
	}

	if(!_Context.m_pEngine)
		return false;

	int CapFlags = _Context.m_pEngine->m_RenderCaps_Flags;

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (CapFlags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		CRC_ExtAttributes_FragmentProgram20* pAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));

		if (!pAttr) 
			return false;

		pAttr->Clear();

		int RasterMode1 = RoundToInt(GetParam(_Oper, 0, CRC_RASTERMODE_NONE));
		switch(RasterMode1)
		{
		case CRC_RASTERMODE_NONE :
			{
				// Spare0.rgb = Texture0.rgb * Primary.rgb;
				// Spare0.a = Texture1.a * Primary.a;

				pAttr->m_pProgramName = "VBOp_MultiTxt_None";
			}
			break;

		case CRC_RASTERMODE_ALPHABLEND :
			{
				// Spare0.rgb = Texture0.rgb * Primary.rgb;
				// Spare1.rgb = Texture1.rgb * Primary.rgb;
				// Spare0.a = Texture0.a * Primary.a;
				// Spare1.a = Texture1.a * Primary.a;


				// Spare0.rgb = Spare1.rgb*(1-Spare0.a) + Spare0.rgb*Spare0.a;
				// Spare0.a = Spare1.a

				pAttr->m_pProgramName = "VBOp_MultiTxt_AlphaBlend";

			}
			break;

		case CRC_RASTERMODE_MULADD :
			{
				// Spare0.rgb = Texture0.rgb;
				// Spare1.rgb = Texture1.rgb * Primary.rgb;
				// Spare0.a = Texture0.a;
				// Spare1.a = Texture1.a * Primary.a;

				// Spare0.rgb = Spare1.rgb + Spare1.rgb*Spare0.rgb;
				// Spare0.a = Spare1.a
				pAttr->m_pProgramName = "VBOp_MultiTxt_MulAdd";

			}
			break;

		case CRC_RASTERMODE_MULTIPLY2 :
			{
				// Spare0.rgb = Texture0.rgb * Primary.rgb;
				// Spare0.a = Texture0.a * Primary.a;

				// Spare0.rgb = Spare0.rgb*Spare1.rgb*2;
				// Spare0.a = Spare1.a
				pAttr->m_pProgramName = "VBOp_MultiTxt_Multiply2";
			}
			break;

	//	case CRC_RASTERMODE_ONE_INVALPHA :
	//		break;

		default:
			ConOut(CStrF("(CXR_VBOperator_MultiTxt::OnOperate) Unsupported mode %d", RasterMode1));
		}

		// Set program
		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		return true;
	}
	else if (CapFlags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{
		CRC_ExtAttributes_FragmentProgram11* pAttr = (CRC_ExtAttributes_FragmentProgram11*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));

		if (!pAttr) 
			return false;

		pAttr->Clear();

		int RasterMode1 = RoundToInt(GetParam(_Oper, 0, CRC_RASTERMODE_NONE));
		switch(RasterMode1)
		{
		case CRC_RASTERMODE_NONE :
			{
				// Spare0.rgb = Texture0.rgb * Primary.rgb;
				// Spare0.a = Texture1.a * Primary.a;

				pAttr->m_pProgramName = "VBOp_MultiTxt_None";
			}
			break;

		case CRC_RASTERMODE_ALPHABLEND :
			{
				// Spare0.rgb = Texture0.rgb * Primary.rgb;
				// Spare1.rgb = Texture1.rgb * Primary.rgb;
				// Spare0.a = Texture0.a * Primary.a;
				// Spare1.a = Texture1.a * Primary.a;


				// Spare0.rgb = Spare1.rgb*(1-Spare0.a) + Spare0.rgb*Spare0.a;
				// Spare0.a = Spare1.a

				pAttr->m_pProgramName = "VBOp_MultiTxt_AlphaBlend";

			}
			break;

		case CRC_RASTERMODE_MULADD :
			{
				// Spare0.rgb = Texture0.rgb;
				// Spare1.rgb = Texture1.rgb * Primary.rgb;
				// Spare0.a = Texture0.a;
				// Spare1.a = Texture1.a * Primary.a;

				// Spare0.rgb = Spare1.rgb + Spare1.rgb*Spare0.rgb;
				// Spare0.a = Spare1.a
				pAttr->m_pProgramName = "VBOp_MultiTxt_MulAdd";

			}
			break;

		case CRC_RASTERMODE_MULTIPLY2 :
			{
				// Spare0.rgb = Texture0.rgb * Primary.rgb;
				// Spare0.a = Texture0.a * Primary.a;

				// Spare0.rgb = Spare0.rgb*Spare1.rgb*2;
				// Spare0.a = Spare1.a
				pAttr->m_pProgramName = "VBOp_MultiTxt_Multiply2";
			}
			break;

	//	case CRC_RASTERMODE_ONE_INVALPHA :
	//		break;

		default:
			ConOut(CStrF("(CXR_VBOperator_MultiTxt::OnOperate) Unsupported mode %d", RasterMode1));
		}

		// Set program
		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		return true;
	}
#ifdef SUPPORT_REGISTERCOMBINERS
	else
#endif
#endif
#ifdef SUPPORT_REGISTERCOMBINERS
		if (CapFlags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
		{

			CRC_ExtAttributes_NV20* pAttr = (CRC_ExtAttributes_NV20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV20));
			if (!pAttr) return false;
			pAttr->Clear();

			int RasterMode1 = RoundToInt(GetParam(_Oper, 0, CRC_RASTERMODE_NONE));
			switch(RasterMode1)
			{
			case CRC_RASTERMODE_NONE :
				{
					// Spare0.rgb = Texture0.rgb * Primary.rgb;
					// Spare0.a = Texture1.a * Primary.a;

					pAttr->Clear(0);
					pAttr->SetInputRGB(0, NV_INPUT_TEXTURE0, NV_INPUT_PRIMARY);
					pAttr->SetInputAlpha(0, NV_INPUT_TEXTURE1|NV_COMP_ALPHA, NV_INPUT_PRIMARY|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
					pAttr->SetOutputAlpha(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
					pAttr->SetNumCombiners(1);
				}
				break;

			case CRC_RASTERMODE_ALPHABLEND :
				{
					// Spare0.rgb = Texture0.rgb * Primary.rgb;
					// Spare1.rgb = Texture1.rgb * Primary.rgb;
					// Spare0.a = Texture0.a * Primary.a;
					// Spare1.a = Texture1.a * Primary.a;

					pAttr->Clear(0);
					pAttr->SetInputRGB(0, NV_INPUT_TEXTURE0, NV_INPUT_PRIMARY, NV_INPUT_TEXTURE1, NV_INPUT_PRIMARY);
					pAttr->SetInputAlpha(0, NV_INPUT_TEXTURE0|NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_TEXTURE1|NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(0, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
					pAttr->SetOutputAlpha(0, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

					// Spare0.rgb = Spare1.rgb*(1-Spare0.a) + Spare0.rgb*Spare0.a;
					// Spare0.a = Spare1.a

					pAttr->Clear(1);
					pAttr->SetInputRGB(1, NV_INPUT_SPARE1, NV_INPUT_SPARE0|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_SPARE0, NV_INPUT_SPARE0|NV_COMP_ALPHA);
					pAttr->SetInputAlpha(1, NV_INPUT_PRIMARY|NV_COMP_ALPHA, NV_INPUT_SPARE1| NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(1, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false);
					pAttr->SetOutputAlpha(1, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
					pAttr->SetNumCombiners(2);
				}
				break;

			case CRC_RASTERMODE_MULADD :
				{
					// Spare0.rgb = Texture0.rgb;
					// Spare1.rgb = Texture1.rgb * Primary.rgb;
					// Spare0.a = Texture0.a;
					// Spare1.a = Texture1.a * Primary.a;

					pAttr->Clear(0);
					pAttr->SetInputRGB(0, NV_INPUT_TEXTURE0, NV_INPUT_ZERO|NV_MAPPING_INVERT, NV_INPUT_TEXTURE1, NV_INPUT_PRIMARY);
					pAttr->SetInputAlpha(0, NV_INPUT_TEXTURE0|NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_TEXTURE1|NV_COMP_ALPHA, NV_INPUT_PRIMARY|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(0, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
					pAttr->SetOutputAlpha(0, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

					// Spare0.rgb = Spare1.rgb + Spare1.rgb*Spare0.rgb;
					// Spare0.a = Spare1.a

					pAttr->Clear(1);
					pAttr->SetInputRGB(1, NV_INPUT_SPARE1, NV_INPUT_SPARE0, NV_INPUT_SPARE1, NV_INPUT_ZERO|NV_MAPPING_INVERT);
					pAttr->SetInputAlpha(1, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_SPARE1|NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(1, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false);
					pAttr->SetOutputAlpha(1, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
					pAttr->SetNumCombiners(2);
				}
				break;

			case CRC_RASTERMODE_MULTIPLY2 :
				{
					// Spare0.rgb = Texture0.rgb * Primary.rgb;
					// Spare0.a = Texture0.a * Primary.a;

					pAttr->Clear(0);
					pAttr->SetInputRGB(0, NV_INPUT_TEXTURE0, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO);
					pAttr->SetInputAlpha(0, NV_INPUT_TEXTURE0|NV_COMP_ALPHA, NV_INPUT_PRIMARY|NV_COMP_ALPHA, NV_INPUT_TEXTURE1|NV_COMP_ALPHA, NV_INPUT_PRIMARY|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
					pAttr->SetOutputAlpha(0, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

					// Spare0.rgb = Spare0.rgb*Spare1.rgb*2;
					// Spare0.a = Spare1.a

					pAttr->Clear(1);
					pAttr->SetInputRGB(1, NV_INPUT_TEXTURE1, NV_INPUT_SPARE0, NV_INPUT_ZERO, NV_INPUT_ZERO);
					pAttr->SetInputAlpha(1, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_SPARE1|NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_ZERO|NV_MAPPING_INVERT|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(1, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_TWO, false, false, false);
					pAttr->SetOutputAlpha(1, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

					pAttr->SetNumCombiners(2);
				}
				break;

				//	case CRC_RASTERMODE_ONE_INVALPHA :
				//		break;

			case 20 :
				{
					pAttr->Clear(0);
					pAttr->SetInputRGB(0, NV_INPUT_TEXTURE0, NV_INPUT_PRIMARY, NV_INPUT_TEXTURE1, NV_INPUT_PRIMARY|NV_MAPPING_INVERT);
					pAttr->SetOutputRGB(0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false);

					pAttr->Clear(1);
					pAttr->SetConst0(0xffffffff);
					pAttr->SetInputRGB(1, NV_INPUT_SPARE0, NV_INPUT_PRIMARY|NV_COMP_ALPHA, NV_INPUT_COLOR0, NV_INPUT_PRIMARY|NV_MAPPING_INVERT|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(1, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false);
					pAttr->SetNumCombiners(2);
				}
				break;

			case 21 :
				{
					// Spare0.rgb = Texture0.rgb * Primary.rgb;
					// Spare1.rgb = Texture1.rgb * Primary.rgb;
					// Spare0.a = Texture0.a * Primary.a;
					// Spare1.a = Texture1.a * Primary.a;

					pAttr->Clear(0);
					pAttr->SetInputRGB(0, NV_INPUT_TEXTURE0, NV_INPUT_PRIMARY, NV_INPUT_TEXTURE1, NV_INPUT_PRIMARY|NV_MAPPING_INVERT);
					pAttr->SetOutputRGB(0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false);

					// Spare0.rgb = Spare1.rgb*(1-Primary.a) + Spare0.rgb*Primary.a;
					// Spare0.a = Spare1.a

					pAttr->Clear(1);
					pAttr->SetConst0(0xff808080);
					pAttr->SetInputRGB(1, NV_INPUT_SPARE0, NV_INPUT_PRIMARY|NV_COMP_ALPHA, NV_INPUT_COLOR0, NV_INPUT_PRIMARY|NV_MAPPING_INVERT|NV_COMP_ALPHA);
					pAttr->SetOutputRGB(1, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false);
					pAttr->SetNumCombiners(2);
				}
				break;

			default:
				{
					pAttr->Clear(0);
					pAttr->SetNumCombiners(1);
					ConOut(CStrF("(CXR_VBOperator_MultiTxt::OnOperate) Unsupported mode %d", RasterMode1));
				}
			}

			pAttr->SetFinal(NV_INPUT_FOG|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_FOG, NV_INPUT_SPARE0, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0|NV_COMP_ALPHA);
//			pAttr->SetFinal(NV_INPUT_FOG|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_FOG, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0|NV_COMP_ALPHA);
			pAttr->Fixup();

			// Set program
			_pVB->m_pAttrib->m_pExtAttrib = pAttr;
			return true;
	}
	else 
#endif
		if (CapFlags & CRC_CAPS_FLAGS_TEXENVMODE2)
	{
		if (_pVB->m_pAttrib)
		{
			int RasterMode1 = RoundToInt(GetParam(_Oper, 0, CRC_RASTERMODE_NONE));
			switch(RasterMode1)
			{
			case CRC_RASTERMODE_NONE :
				{
					_pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE2_NONE);
//					_pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE2_NONE1);
					
					// Spare0.rgb = Texture0.rgb * Primary.rgb;
					// Spare0.a = Texture1.a * Primary.a;
				}
				break;
				
			case CRC_RASTERMODE_ALPHABLEND :
				{
					// Spare0.rgb = Texture0.rgb * Primary.rgb;
					// Spare1.rgb = Texture1.rgb * Primary.rgb;
					// Spare0.a = Texture0.a * Primary.a;
					// Spare1.a = Texture1.a * Primary.a;

					_pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE2_ALPHABLEND);
//					_pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE2_ALPHABLEND1);
					
					// Spare0.rgb = Spare1.rgb*(1-Spare0.a) + Spare0.rgb*Spare0.a;
					// Spare0.a = Spare1.a

//					_pVB->m_pAttrib->Attrib_TexEnvMode(2, CRC_TEXENVMODE2_ALPHABLEND2);
				}
				break;
				
			case CRC_RASTERMODE_MULADD :
				{
					// Spare0.rgb = Texture0.rgb;
					// Spare1.rgb = Texture1.rgb * Primary.rgb;
					// Spare0.a = Texture0.a;
					// Spare1.a = Texture1.a * Primary.a;
					
					_pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE2_MULADD);
//					_pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE2_MULADD1);
					
					// Spare0.rgb = Spare1.rgb + Spare1.rgb*Spare0.rgb;
					// Spare0.a = Spare1.a

//					_pVB->m_pAttrib->Attrib_TexEnvMode(2, CRC_TEXENVMODE2_MULADD2);
				}
				break;
				
			case CRC_RASTERMODE_MULTIPLY2 :
				{
					// Spare0.rgb = Texture0.rgb * Primary.rgb;
					// Spare0.a = Texture0.a * Primary.a;
					
					_pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE2_MULTIPLY2);
					
					// Spare0.rgb = Spare0.rgb*Spare1.rgb*2;
					// Spare0.a = Spare1.a
					
//					_pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE2_MULTIPLY2_1);
				}
				break;
				
				//	case CRC_RASTERMODE_ONE_INVALPHA :
				//		break;
				
			default:
				ConOut(CStrF("(CXR_VBOperator_MultiTxt::OnOperate) Unsupported mode %d", RasterMode1));
			}
			return true;
		}
	}

	return false;
}

bool CXR_VBOperator_MultiTxt::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_MultiTxt_OnTestHWAccelerated, false);
	return true;
}

void CXR_VBOperator_MultiTxt::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_MultiTxt_OnInitSurface, MAUTOSTRIP_VOID);	
	_pSurf->m_Requirements |= XW_SURFREQ_NV20;
//	_pSurf->m_Requirements |= XW_SURFREQ_TEXENVMODE2;
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_Fresnel
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_NV20_Fresnel, CXR_VBOperator_Util);

CXR_VBOperator_NV20_Fresnel::CXR_VBOperator_NV20_Fresnel()
{
	MAUTOSTRIP(CXR_VBOperator_NV20_Fresnel_ctor, MAUTOSTRIP_VOID);
	m_TextureID_Normalize = 0;
}

bool CXR_VBOperator_NV20_Fresnel::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_NV20_Fresnel_OnOperate, false);
	// Assign the required geometry.
	// Obsolete since we always copy all texcoord arrays to the final VB.
	if (!_pVB->IsVBIDChain())
	{
		CXR_VBChain *pChain = _pVB->GetVBChain();
		CXR_VBChain *pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
		if (!pChain->m_pTV[1]) 
			pChain->m_pTV[1] = pChainSrc->m_pTV[2];	// Just to fill out the texcoord set so the RC doesn't cut texcoord 2 & 3.
/*		_pVB->m_pTV[2] = _pSrcVB->m_pTV[2];
		_pVB->m_pTV[3] = _pSrcVB->m_pTV[3];
		_pVB->m_nTVComp[2] = _pSrcVB->m_nTVComp[2];
		_pVB->m_nTVComp[3] = _pSrcVB->m_nTVComp[3];*/
	}

	if(!_Context.m_pEngine)
		return false;

	if (!_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_NV20_Fresnel::OnOperate) No attributes.");
		return false;
	}

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		_pVB->m_pAttrib->Attrib_TextureID(1, m_TextureID_Normalize);

		CRC_ExtAttributes_FragmentProgram20* pAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(1+2);
		if (!pParams) 
			return false;

		CVec4Dfp32* pFPParams = pParams + 1;

		pAttr->m_pProgramName = "VBOp_Fresnel";
		
		int FresnelMul = RoundToInt(GetParam(_Oper, 0, 1.0f) * 127.0f);
		int FresnelAdd = RoundToInt(GetParam(_Oper, 1, 0.0f) * 127.0f);

		pFPParams[0] = FresnelMul / 255.0f;
		pFPParams[1] = FresnelAdd / 255.0f;
		pAttr->SetParameters(pFPParams, 2);

		CMat4Dfp32 V2M;
		if (!_Context.m_pModel2View)
		{
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2V matrix.");
			return false;
		}
		_Context.m_pModel2View->InverseOrthogonal(V2M);

		pParams[0][0] = V2M.k[3][0];
		pParams[0][1] = V2M.k[3][1];
		pParams[0][2] = V2M.k[3][2];
		pParams[0][3] = 1.0f;
		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexCoordSet(2, 1);
		_pVB->m_pAttrib->Attrib_TexCoordSet(3, 2);
		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		_Context.m_iTexChannelNext = 4;
		return true;
	}
#endif
#if 0
#ifdef SUPPORT_FRAGMENTPROGRAM
	else if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM14)
	{
		_pVB->m_pAttrib->Attrib_TextureID(1, m_TextureID_Normalize);

		CRC_ExtAttributes_FragmentProgram14* pAttr = (CRC_ExtAttributes_FragmentProgram14*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram14));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(1+2);
		if (!pParams) 
			return false;

		CVec4Dfp32* pFPParams = pParams + 1;

		pAttr->m_pProgramName = "VBOp_Fresnel";
		
		int FresnelMul = RoundToInt(GetParam(_Oper, 0, 1.0f) * 127.0f);
		int FresnelAdd = RoundToInt(GetParam(_Oper, 1, 0.0f) * 127.0f);

		pFPParams[0] = FresnelMul / 255.0f;
		pFPParams[1] = FresnelAdd / 255.0f;
		pAttr->SetParameters(pFPParams, 2);

		CMat4Dfp32 V2M;
		if (!_Context.m_pModel2View)
		{
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2V matrix.");
			return false;
		}
		_Context.m_pModel2View->InverseOrthogonal(V2M);

		pParams[0][0] = V2M.k[3][0];
		pParams[0][1] = V2M.k[3][1];
		pParams[0][2] = V2M.k[3][2];
		pParams[0][3] = 1.0f;
		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		_Context.m_iTexChannelNext = 4;
		return true;
	}
	else if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{
		_pVB->m_pAttrib->Attrib_TextureID(1, m_TextureID_Normalize);

		CRC_ExtAttributes_FragmentProgram11* pAttr = (CRC_ExtAttributes_FragmentProgram11*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(1);
		if (!pParams) 
			return false;

		pAttr->m_pProgramName = "VBOp_Fresnel";
		
		int FresnelMul = RoundToInt(GetParam(_Oper, 0, 1.0f) * 127.0f);
		int FresnelAdd = RoundToInt(GetParam(_Oper, 1, 0.0f) * 127.0f);

		pAttr->m_Colors[0] = CPixel32(FresnelMul, FresnelMul, FresnelMul, FresnelMul);
		pAttr->m_Colors[1] = CPixel32(FresnelAdd, FresnelAdd, FresnelAdd, FresnelAdd);

		CMat4Dfp32 V2M;
		if (!_Context.m_pModel2View)
		{
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2V matrix.");
			return false;
		}
		_Context.m_pModel2View->InverseOrthogonal(V2M);

		pParams[0][0] = V2M.k[3][0];
		pParams[0][1] = V2M.k[3][1];
		pParams[0][2] = V2M.k[3][2];
		pParams[0][3] = 1.0f;
		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		_Context.m_iTexChannelNext = 4;
		return true;
	}
#ifdef SUPPORT_REGISTERCOMBINERS
	else
#endif
#endif
#ifdef SUPPORT_REGISTERCOMBINERS
	if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
	{
		_pVB->m_pAttrib->Attrib_TextureID(1, m_TextureID_Normalize);

		CRC_ExtAttributes_NV10* pAttr = (CRC_ExtAttributes_NV10*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
		if (!pAttr) return false;
		pAttr->Clear();
		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(1);
		if (!pParams) return false;

		// Combiner0: Spare0 = Texture0 dot Texture1
		pAttr->Clear(0);
		pAttr->SetInputRGB(0, NV_INPUT_TEXTURE0|NV_MAPPING_EXPNORMAL, NV_INPUT_TEXTURE1|NV_MAPPING_HALFBIASNORMAL);
		pAttr->SetOutputRGB(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, true, false, false);
		pAttr->Clear(1);
		pAttr->SetInputRGB(1, NV_INPUT_SPARE0, NV_INPUT_COLOR0, NV_INPUT_COLOR1, NV_INPUT_ZERO|NV_MAPPING_INVERT);
		pAttr->SetOutputRGB(1, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_TWO, false, false, false);

		// Final: RGB = 1-Spare0, Alpha = 1-Spare0.b
		pAttr->SetFinal(NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0|NV_MAPPING_INVERT);

		int FresnelMul = RoundToInt(GetParam(_Oper, 0, 1.0f) * 127.0f);
		int FresnelAdd = RoundToInt(GetParam(_Oper, 1, 0.0f) * 127.0f);

		pAttr->SetConst0(CPixel32(FresnelMul, FresnelMul, FresnelMul, FresnelMul));
		pAttr->SetConst1(CPixel32(FresnelAdd, FresnelAdd, FresnelAdd, FresnelAdd));

		pAttr->SetNumCombiners(2);
		pAttr->Fixup();

		CMat4Dfp32 V2M;
		if (!_Context.m_pModel2View)
		{
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2V matrix.");
			return false;
		}
		_Context.m_pModel2View->InverseOrthogonal(V2M);

		pParams[0][0] = V2M.k[3][0];
		pParams[0][1] = V2M.k[3][1];
		pParams[0][2] = V2M.k[3][2];
		pParams[0][3] = 1.0f;
	//ConOut(CStrF("EV %f, %f, %f", pParams[0][0], pParams[0][1], pParams[0][2]));
		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);

		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		_Context.m_iTexChannelNext = 4;
		return true;
	}
#endif
#if defined(SUPPORT_REGISTERCOMBINERS) || defined(SUPPORT_FRAGMENTPROGRAM)
	else
#endif
#endif
	if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_TEXENVMODE_COMBINE)
	{
		_pVB->m_pAttrib->Attrib_TextureID(1, m_TextureID_Normalize);

//		int FresnelMul = RoundToInt(GetParam(_Oper, 0, 1.0f) * 127.0f);
//		int FresnelAdd = RoundToInt(GetParam(_Oper, 1, 0.0f) * 127.0f);

/*		pFPParams[0] = FresnelMul / 255.0f;
		pFPParams[1] = FresnelAdd / 255.0f;
		pAttr->SetParameters(pFPParams, 2);
*/
		CMat4Dfp32 V2M;
		if (!_Context.m_pModel2View)
		{
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2V matrix.");
			return false;
		}
		_Context.m_pModel2View->InverseOrthogonal(V2M);

		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(1+2);
		if (!pParams) 
			return false;
		pParams[0][0] = V2M.k[3][0];
		pParams[0][1] = V2M.k[3][1];
		pParams[0][2] = V2M.k[3][2];
		pParams[0][3] = 1.0f;
		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE_REPLACE);
		_pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE_COMBINE_DOT3);
		_pVB->m_pAttrib->Attrib_TexCoordSet(2, 1);
		_pVB->m_pAttrib->Attrib_TexCoordSet(3, 2);
//		_pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE_REPLACE);

//_pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_COLORWRITE);
//_pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_COLORWRITE);
		_Context.m_iTexChannelNext = 4;
		return true;
	}
	return true;
}

bool CXR_VBOperator_NV20_Fresnel::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_Fresnel_OnTestHWAccelerated, false);
	return true;
}

void CXR_VBOperator_NV20_Fresnel::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_Fresnel_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDTANGENTS;
//	_pSurf->m_Requirements |= XW_SURFREQ_NV20;

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (pTC)
		m_TextureID_Normalize = pTC->GetTextureID("SPECIAL_CUBENORMALIZE0");
}


int CXR_VBOperator_NV20_Fresnel::OnGetTextureCount(const class CXW_Surface* _pSurf, class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper) const
{
	MAUTOSTRIP(CXR_VBOperator_NV20_Fresnel_OnGetTextureCount, 0);
	return 1;
}

int CXR_VBOperator_NV20_Fresnel::OnEnumTextureID(const class CXW_Surface* _pSurf, class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper, int _iEnum) const
{
	MAUTOSTRIP(CXR_VBOperator_NV20_Fresnel_OnEnumTextureID, 0);
	return m_TextureID_Normalize;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_YUV2RGB
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_NV20_YUV2RGB, CXR_VBOperator_Util);

bool CXR_VBOperator_NV20_YUV2RGB::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_NV20_YUV2RGB_OnOperate, false);
	// Assign the required geometry.
	// Obsolete since we always copy all texcoord arrays to the final VB.
	if (!_pVB->IsVBIDChain())
	{
		CXR_VBChain *pChain = _pVB->GetVBChain();
		CXR_VBChain *pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
		if (!pChain->m_pTV[1]) 
			pChain->m_pTV[1] = pChainSrc->m_pTV[2];	// Just to fill out the texcoord set so the RC doesn't cut texcoord 2 & 3.
/*		_pVB->m_pTV[2] = _pSrcVB->m_pTV[2];
		_pVB->m_pTV[3] = _pSrcVB->m_pTV[3];
		_pVB->m_nTVComp[2] = _pSrcVB->m_nTVComp[2];
		_pVB->m_nTVComp[3] = _pSrcVB->m_nTVComp[3];*/
	}

	if(!_Context.m_pEngine)
		return false;

	if (!_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_NV20_YUV2RGB::OnOperate) No attributes.");
		return false;
	}

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (_Context.m_pEngine->m_RenderCaps_Flags & (CRC_CAPS_FLAGS_FRAGMENTPROGRAM20 | CRC_CAPS_FLAGS_FRAGMENTPROGRAM14) )
	{
		if(_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20 )
		{
			CRC_ExtAttributes_FragmentProgram20* pShaderAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
			if (!pShaderAttr) 
				return false;
			pShaderAttr->Clear();
			pShaderAttr->SetProgram( "CMWnd_ModTexture_PaintVideo_YUV2RGB", 0 );
			_pVB->m_pAttrib->m_pExtAttrib	= pShaderAttr;
		}
		else if(_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM14 )
		{
			CRC_ExtAttributes_FragmentProgram14* pShaderAttr = (CRC_ExtAttributes_FragmentProgram14*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram14));
			if (!pShaderAttr) 
				return false;
			pShaderAttr->Clear();
			CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(3);
			pParams[0]	= CVec4Dfp32( 0.0f, 0.582f, 1.164f * 16.0f / 255.0f, 0.0f );
			pParams[1]	= CVec4Dfp32( 0.0f, 0.391f, 0.99999999999999f, 0.0f );
			pParams[2]	= CVec4Dfp32( 0.768f, 0.813f, 0.0f, 0.0f );
			pShaderAttr->SetParameters(pParams, 3);

			_pVB->m_pAttrib->Attrib_TexCoordSet( 1, 0 );
		}
		_Context.m_iTexChannelNext = 2;
		return true;
	}
#ifdef SUPPORT_REGISTERCOMBINERS
	else
#endif
#endif
#ifdef SUPPORT_REGISTERCOMBINERS
	if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
	{
		CRC_ExtAttributes_NV10* pAttr = (CRC_ExtAttributes_NV10*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
		if (!pAttr) return false;
		pAttr->Clear();

		int nComb = 0;

		// Perform everything in colorspace / 2
		pAttr->Clear(nComb);
		pAttr->SetInputRGB( nComb, NV_INPUT_TEXTURE0, NV_INPUT_ZERO | NV_MAPPING_INVERT, NV_INPUT_TEXTURE1 | NV_MAPPING_HALFBIASNORMAL, NV_INPUT_ZERO | NV_MAPPING_INVERT );
		pAttr->SetOutputRGB( nComb, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false );
		pAttr->SetInputAlpha( nComb, NV_INPUT_TEXTURE1 | NV_COMP_ALPHA | NV_MAPPING_HALFBIASNORMAL, NV_INPUT_ZERO | NV_MAPPING_INVERT  );
		pAttr->SetOutputAlpha( nComb, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false );
		nComb++;
		// r0 = t0_half, r1 = t1_bias_half

		pAttr->Clear( nComb );
		pAttr->SetConst0( nComb, 0x94949494 );
		pAttr->SetConst1( nComb, 0x09090909 );
		pAttr->SetInputRGB( nComb, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_COLOR0, NV_INPUT_COLOR1 | NV_MAPPING_SIGNEDNEGATE, NV_INPUT_ZERO | NV_MAPPING_INVERT );
		pAttr->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false );
		nComb++;
		// mad r0.rgb, r0, c0, c1_neg;

		pAttr->Clear( nComb );
		pAttr->SetConst0( nComb, CPixel32( 0, 0x31, 0, 0 ) );
		pAttr->SetInputRGB( nComb, NV_INPUT_COLOR0 | NV_MAPPING_SIGNEDNEGATE, NV_INPUT_SPARE1 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_ZERO | NV_MAPPING_INVERT );
		pAttr->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false );
		nComb++;
		// mad r0.rgb c0_neg.rgb, r1.g, r0;

		pAttr->Clear( nComb );
		pAttr->SetConst0( nComb, CPixel32( 0, 0, 0xff, 0 ) );
		pAttr->SetInputRGB( nComb, NV_INPUT_COLOR0, NV_INPUT_SPARE1 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_ZERO | NV_MAPPING_INVERT );
		pAttr->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false );
		nComb++;
		// mad r0.rgb, one.rgb, r1.b, r0;

		pAttr->Clear( nComb );
		pAttr->SetConst0( nComb, CPixel32( 0xc3, 0, 0, 0 ) );
		pAttr->SetInputRGB( nComb, NV_INPUT_COLOR0, NV_INPUT_SPARE1 | NV_MAPPING_SIGNEDIDENTITY | NV_COMP_ALPHA, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_ZERO | NV_MAPPING_INVERT );
		pAttr->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false );
		nComb++;
		// mad r0.rgb, c0.rgb, r1.a, r0;

		pAttr->Clear( nComb );
		pAttr->SetConst0( nComb, CPixel32( 0, 0x67, 0, 0 ) );
		pAttr->SetInputRGB( nComb, NV_INPUT_COLOR0 | NV_MAPPING_SIGNEDNEGATE, NV_INPUT_SPARE1 | NV_MAPPING_SIGNEDIDENTITY | NV_COMP_ALPHA, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_ZERO | NV_MAPPING_INVERT );
		pAttr->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_TWO, false, false, false );
		nComb++;
		// mad r0_x2.rgb, c0_neg.rgb, r1.a, r0;

		pAttr->SetNumCombiners(nComb, true);
		pAttr->ClearFinal();
		pAttr->SetFinal( NV_INPUT_SPARE0, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO );
		// mul r0.rgb, r0, fragment.color;


		_pVB->m_pAttrib->Attrib_TexCoordSet( 1, 0 );
		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		_Context.m_iTexChannelNext = 2;
		return true;
	}
#endif

	return true;
}

bool CXR_VBOperator_NV20_YUV2RGB::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_YUV2RGB_OnTestHWAccelerated, false);
	return (_Context.m_pEngine->m_RenderCaps_Flags & (CRC_CAPS_FLAGS_FRAGMENTPROGRAM20 | CRC_CAPS_FLAGS_FRAGMENTPROGRAM14) ) != 0;
}

void CXR_VBOperator_NV20_YUV2RGB::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_NV20_YUV2RGB_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Requirements |= XW_SURFREQ_MULTITEX2;
}
