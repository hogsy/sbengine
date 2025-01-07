
#include "PCH.h"

#include "XRVBOperators.h"
#include "XRVBOperators_NVidia.h"
#include "MFloat.h"
#include "XRUtil.h"

#include "XREngineImp.h"

#if defined(PLATFORM_DOLPHIN)
#include	"..\\MRndrDolphin\VertexProcessing_GC.h"
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Util
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
static fp32 WrapFloat(fp32 _x, fp32 _range)
{
	MAUTOSTRIP(WrapFloat, 0.0f);
	if (_x < 0.0f)
		return _x - fp32(TruncToInt(_x / _range) - 1) * _range;
	else
		return _x - fp32(TruncToInt(_x / _range)) * _range;
}
*/

fp32 CXR_VBOperator_Util::GetParam(const class CXW_LayerOperation& _Oper, int iParam, fp32 _Default) const
{
	MAUTOSTRIP(CXR_VBOperator_Util_GetParam, 0.0f);
	const fp32* pParams = _Oper.m_lParams.GetBasePtr();
	int nParams = _Oper.m_lParams.Len();
	if (iParam < nParams)
		return pParams[iParam];
	else
		return _Default;
}


bool CXR_VBOperator_Util::UVW_AddTransform(CXR_VBOperatorContext& _Context, class CXR_VertexBuffer* _pVB, const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(CXR_VBOperator_Util_UVW_AddTransform, false);
	if (!_pVB->m_pTextureTransform)
	{
		CMat4Dfp32*M_RESTRICT pMat = (CMat4Dfp32*)_Context.m_pVBM->Alloc(sizeof(CMat4Dfp32) + sizeof(const CMat4Dfp32*) * CRC_MAXTEXCOORDS);
		if (!pMat)
			return false;
		CMat4Dfp32** pTextureTransform = (CMat4Dfp32**)(pMat + 1);
		memset(pTextureTransform, 0, sizeof(const CMat4Dfp32*) * CRC_MAXTEXCOORDS);
		pTextureTransform[_Context.m_iTexChannel] = pMat;
		_pVB->m_pTextureTransform = (const CMat4Dfp32**) pTextureTransform;
		*pMat = _Mat;
		return true;
	}

//	if (!_pVB->AllocTextureMatrix(_Context.m_pVBM))
//		return false;

	CMat4Dfp32*M_RESTRICT pTMat = const_cast<CMat4Dfp32*> (_pVB->m_pTextureTransform[_Context.m_iTexChannel]);
	if (!pTMat)
	{
		pTMat = _Context.m_pVBM->Alloc_M4();
		if (!pTMat) return false;
		*pTMat = _Mat;
		_pVB->m_pTextureTransform[_Context.m_iTexChannel] = pTMat;
//		pTMat->Unit();
	}
	else
	{
		CMat4Dfp32 Res;
		pTMat->Multiply(_Mat, Res);
		*pTMat = Res;
	}

	return true;
}

bool CXR_VBOperator_Util::UVW_FlattenTransform(CXR_VBOperatorContext& _Context, class CXR_VertexBuffer* _pVB)
{
	MAUTOSTRIP(CXR_VBOperator_Util_UVW_FlattenTransform, false);
	Error("UVW_FlattenTransform", "Not implemented.");
	return true;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_TextureAnim
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_TextureAnim, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_TextureAnim::ms_lParamDesc[] = 
{
	{ "TextureCount", "TextureCount.", 0, 1, _FP32_MAX },
	{ "AnimationTime", "AnimationTime.", 0, 0, _FP32_MAX },
	{ "SubFrames", "SubFrames.", 0, 1, _FP32_MAX },
	{ "TextureWidth", "TextureWidth.", 0, 0, _FP32_MAX },
	{ "TextureHeight", "TextureHeight.", 0, 0, _FP32_MAX },
	{ "SubTextureWidth", "SubTextureWidth.", 0, 0, _FP32_MAX },
	{ "SubTextureHeight", "SubTextureHeight.", 0, 0, _FP32_MAX },
	{ "SubTextureNumHorizontal", "SubTextureNumHorizontal.", 0, 0, _FP32_MAX },
	{ "SubTextureNumVertical", "SubTextureNumVertical.", 0, 0, _FP32_MAX },
	{ "SubTextureStartOffsetX", "SubTextureStartOffsetX.", 0, 0, _FP32_MAX },
	{ "SubTextureStartOffsetY", "SubTextureStartOffsetY.", 0, 0, _FP32_MAX },
	{ "SubTextureAutomaticScale", "SubTextureAutomaticScale.", 0, 0, _FP32_MAX },
	{ "AnimTimeModulus", "AnimTimeModulus.", 0, 0, _FP32_MAX },
	{ "AnimOnce", "Play animation once", 0, 0, _FP32_MAX },
};

#define FRAC(a) ((a) - M_Floor(a))

bool CXR_VBOperator_TextureAnim::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_TextureAnim_OnOperate, false);
//	int nParams = _Oper.m_lParams.Len();

	if (!_pVB->m_pAttrib)
	{
		ConOutD("§cf80WARNING: (CXR_VBOperator_TextureAnim::OnOperate) No attributes.");
		return false;
	}

	int nFrames = RoundToInt(GetParam(_Oper, 2, 0.0f));

	if (nFrames)
	{
		int iParam = 1;	// Skip TextureCount
		fp32 LoopTime = GetParam(_Oper, iParam++, 1.0f);
		iParam++; // Frames
		fp32 TextureWidth = GetParam(_Oper, iParam++, 1.0f);
		fp32 TextureHeight = GetParam(_Oper, iParam++, 1.0f);
		fp32 SubTextureWidth = GetParam(_Oper, iParam++, 1.0f);
		fp32 SubTextureHeight = GetParam(_Oper, iParam++, 1.0f);
		int nHorizontal = Max(1, RoundToInt(GetParam(_Oper, iParam++, 1.0f)));
		int nVertical = Max(1, RoundToInt(GetParam(_Oper, iParam++, 1.0f)));
		fp32 StartOffsetX = GetParam(_Oper, iParam++, 1.0f) / TextureWidth;
		fp32 StartOffsetY = GetParam(_Oper, iParam++, 1.0f) / TextureHeight;
		int bAutomaticScale = RoundToInt(GetParam(_Oper, iParam++, 1.0f));
		fp32 AnimTimeModulus = GetParam(_Oper, iParam++, -1.0f);
		int bAnimOnce = RoundToInt(GetParam(_Oper, iParam++, 0.0f));
		CMTime AnimTime;

		if (AnimTimeModulus > -1.0f)
			AnimTime = _Context.m_AnimTime.Modulus(AnimTimeModulus);
		else
			AnimTime = _Context.m_AnimTime;

		int nPerTexture = nHorizontal * nVertical;		

		fp32 AnimFrac = (bAnimOnce) ? MinMT(1.0f, (1.0f / LoopTime) * AnimTime.GetTime()) : AnimTime.GetTimeFraction(LoopTime);
		int AnimPos = Max(0, Min(nFrames-1, TruncToInt(AnimFrac * fp32(nFrames))));
		int iTexture = AnimPos / nPerTexture;
		int iSubTexture = AnimPos - iTexture * nPerTexture;
		int iSubVertical = iSubTexture / nHorizontal;
		int iSubHorizontal = iSubTexture - iSubVertical * nHorizontal;
		fp32 SubWidth = SubTextureWidth / TextureWidth;
		fp32 SubHeight = SubTextureHeight / TextureHeight;

		CVec3Dfp32 UVWTranslate(StartOffsetX + fp32(iSubHorizontal) * SubWidth, StartOffsetY + fp32(iSubVertical) * SubHeight, 0.0f);
		CMat4Dfp32 Mat;
		Mat.Unit();
		UVWTranslate.SetRow(Mat, 3);
		
		if (bAutomaticScale)
		{
			Mat.k[0][0] = SubWidth;
			Mat.k[1][1] = SubHeight;
		}

		UVW_AddTransform(_Context, _pVB, Mat);		

		int iTxtChannel = _Context.m_iTexChannel;
		int TextureID = _pVB->m_pAttrib->m_TextureID[iTxtChannel] + iTexture;

		CTextureContext* pTC = NULL;
		if (_Context.m_pEngine)
			pTC = _Context.m_pEngine->m_pTC;
		else
		{
			MACRO_GetRegisterObject(CTextureContext, pTCTmp, "SYSTEM.TEXTURECONTEXT");
			pTC = pTCTmp;
		}
		
		if (pTC && pTC->GetTextureFlags(TextureID) & CTC_TXTIDFLAGS_ALLOCATED)
			_pVB->m_pAttrib->m_TextureID[iTxtChannel] = TextureID;
		else
			_pVB->m_pAttrib->m_TextureID[iTxtChannel] = 0;

	}
	else
	{
		int iParam = 0;
		int LoopLen = Max(1, RoundToInt(GetParam(_Oper, iParam++, 1.0f)));
		fp32 LoopTime = GetParam(_Oper, iParam++, 1.0f);
		fp32 AnimFrac = _Context.m_AnimTime.GetTimeFraction(LoopTime);
		int AnimPos = Max(0, Min(LoopLen-1, RoundToInt(AnimFrac * fp32(LoopLen))));

		int iTxtChannel = _Context.m_iTexChannel;
		int TextureID = _pVB->m_pAttrib->m_TextureID[iTxtChannel] + AnimPos;

		CTextureContext* pTC = NULL;
		if (_Context.m_pEngine)
			pTC = _Context.m_pEngine->m_pTC;
		else
		{
			MACRO_GetRegisterObject(CTextureContext, pTCTmp, "SYSTEM.TEXTURECONTEXT");
			pTC = pTCTmp;
		}
		
		if (pTC && pTC->GetTextureFlags(TextureID) & CTC_TXTIDFLAGS_ALLOCATED)
			_pVB->m_pAttrib->m_TextureID[iTxtChannel] = TextureID;
		else
			_pVB->m_pAttrib->m_TextureID[iTxtChannel] = 0;
	}

	return true;
}

bool CXR_VBOperator_TextureAnim::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_TextureAnim_OnTestHWAccelerated, false);
	return true;
}

int CXR_VBOperator_TextureAnim::OnGetTextureCount(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper) const
{
	MAUTOSTRIP(CXR_VBOperator_TextureAnim_OnGetTextureCount, 0);
	int iParam = 0;
	int LoopLen = Max(1, RoundToInt(GetParam(_Oper, iParam++, 1.0f)));
	return LoopLen;
}

int CXR_VBOperator_TextureAnim::OnEnumTextureID(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper, int _iEnum) const
{
	MAUTOSTRIP(CXR_VBOperator_TextureAnim_OnEnumTextureID, 0);
	int iParam = 0;
	int LoopLen = Max(1, RoundToInt(GetParam(_Oper, iParam++, 1.0f)));
	_iEnum = Max(0, Min(LoopLen-1, _iEnum));

	int TextureID = _pSurfLayer->m_TextureID + _iEnum;

	// Validate texture id
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC)
		return 0;

	if (pTC->GetTextureFlags(TextureID) & CTC_TXTIDFLAGS_ALLOCATED)
		return TextureID;
	else
		return 0;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Offset
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_Offset, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_Offset::ms_lParamDesc[] = 
{
	{ "Offset 1", "Component 1 offset.", 0, -_FP32_MAX, _FP32_MAX },
	{ "Offset 2", "Component 2 offset.", 0, -_FP32_MAX, _FP32_MAX },
	{ "Offset n", "Component n offset.", 0, -_FP32_MAX, _FP32_MAX },
};

bool CXR_VBOperator_Offset::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_Offset_OnOperate, false);
//	int nParams = _Oper.m_lParams.Len();

	if (_Oper.m_Components & XW_LAYEROP_TVERTEX)
	{

		int iParam = 0;
		CVec3Dfp32 UVW(0);
		if (_Oper.m_Components & XW_LAYEROP_U) UVW[0] = GetParam(_Oper, iParam++, 0.0f);
		if (_Oper.m_Components & XW_LAYEROP_V) UVW[1] = GetParam(_Oper, iParam++, 0.0f);
		if (_Oper.m_Components & XW_LAYEROP_W) UVW[2] = GetParam(_Oper, iParam++, 0.0f);

		CMat4Dfp32 Mat;
		Mat.Unit();
		UVW.SetRow(Mat, 3);
		UVW_AddTransform(_Context, _pVB, Mat);
	}
	else
	{
		ConOutD(CStrF("§cf80WARNING: (CXR_VBOperator_Offset::OnOperate) Unsupported components: %.4x", _Oper.m_Components));
	}

	return true;
}

bool CXR_VBOperator_Offset::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_Offset_OnTestHWAccelerated, false);
	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Scroll
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_Scroll, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_Scroll::ms_lParamDesc[] = 
{
	{ "Velocity 1", "Component 1 velocity. Units per second.", 0, -_FP32_MAX, _FP32_MAX },
	{ "Velocity 2", "Component 2 velocity. Units per second.", 0, -_FP32_MAX, _FP32_MAX },
	{ "Velocity n", "Component n velocity. Units per second.", 0, -_FP32_MAX, _FP32_MAX },
	{ "Wrap 1", "[Optional] Component 1 wrap range.", 0, 0, 128.0f },
	{ "Wrap 2", "[Optional] Component 2 wrap range.", 0, 0, 128.0f },
	{ "Wrap n", "[Optional] Component n wrap range.", 0, 0, 128.0f },
};

bool CXR_VBOperator_Scroll::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_Scroll_OnOperate, false);
//	int nParams = _Oper.m_lParams.Len();

	if (_Oper.m_Components & XW_LAYEROP_TVERTEX)
	{

		// The wrap stuff is to ensure UVW stays within reasonable precision range.
		int iParam = 0;
		CVec3Dfp32 UVW(0);
		CVec3Dfp32 UVWWrap(64.0f);
		if (_Oper.m_Components & XW_LAYEROP_U) UVW[0] = GetParam(_Oper, iParam++, 0.0f);
		if (_Oper.m_Components & XW_LAYEROP_V) UVW[1] = GetParam(_Oper, iParam++, 0.0f);
		if (_Oper.m_Components & XW_LAYEROP_W) UVW[2] = GetParam(_Oper, iParam++, 0.0f);
		if (_Oper.m_Components & XW_LAYEROP_U) UVWWrap[0] = GetParam(_Oper, iParam++, 64.0f);
		if (_Oper.m_Components & XW_LAYEROP_V) UVWWrap[1] = GetParam(_Oper, iParam++, 64.0f);
		if (_Oper.m_Components & XW_LAYEROP_W) UVWWrap[2] = GetParam(_Oper, iParam++, 64.0f);

//		UVW *= _Context.m_AnimTime;
//		 WrapFloat(UVW[0], UVWWrap[0]);
		if (_Oper.m_Components & XW_LAYEROP_U) UVW[0] = _Context.m_AnimTime.GetTimeModulusScaled(UVW[0], UVWWrap[0]);
		if (_Oper.m_Components & XW_LAYEROP_V) UVW[1] = _Context.m_AnimTime.GetTimeModulusScaled(UVW[1], UVWWrap[1]);
		if (_Oper.m_Components & XW_LAYEROP_W) UVW[2] = _Context.m_AnimTime.GetTimeModulusScaled(UVW[2], UVWWrap[2]);

		CMat4Dfp32 Mat;
		Mat.Unit();
		UVW.SetRow(Mat, 3);
		UVW_AddTransform(_Context, _pVB, Mat);
	}
	else
	{
		ConOutD(CStrF("§cf80WARNING: (CXR_VBOperator_Scroll::OnOperate) Unsupported components: %.4x", _Oper.m_Components));
	}

	return true;
}

bool CXR_VBOperator_Scroll::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_Scroll_OnTestHWAccelerated, false);
	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Scale
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_Scale, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_Scale::ms_lParamDesc[] = 
{
	{ "Scale 1", "Component 1 scale.", 1.0f, -_FP32_MAX, _FP32_MAX },
	{ "Scale 2", "Component 2 scale.", 1.0f, -_FP32_MAX, _FP32_MAX },
	{ "Scale n", "Component n scale.", 1.0f, -_FP32_MAX, _FP32_MAX },
};

bool CXR_VBOperator_Scale::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_Scale_OnOperate, false);
//	int nParams = _Oper.m_lParams.Len();

	if (_Oper.m_Components & XW_LAYEROP_TVERTEX)
	{

		int iParam = 0;
		CVec3Dfp32 Scale(1.0f);
		if (_Oper.m_Components & XW_LAYEROP_U) Scale[0] = GetParam(_Oper, iParam++, 0.0f);
		if (_Oper.m_Components & XW_LAYEROP_V) Scale[1] = GetParam(_Oper, iParam++, 0.0f);
		if (_Oper.m_Components & XW_LAYEROP_W) Scale[2] = GetParam(_Oper, iParam++, 0.0f);

		CMat4Dfp32 Mat;
		Mat.Unit();
		Mat.k[0][0] = Scale[0];
		Mat.k[1][1] = Scale[1];
		Mat.k[2][2] = Scale[2];
		UVW_AddTransform(_Context, _pVB, Mat);
	}
	else
	{
		ConOutD(CStrF("§cf80WARNING: (CXR_VBOperator_Scale::OnOperate) Unsupported components: %.4x", _Oper.m_Components));
	}

	return true;
}

bool CXR_VBOperator_Scale::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_Scale_OnTestHWAccelerated, false);
	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Rotate
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_Rotate, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_Rotate::ms_lParamDesc[] = 
{
	{ "AngularVelocity", "Angular velocity in revolutions per second.", 1.0f, -_FP32_MAX, _FP32_MAX },
	{ "AngularOffset", "Angular offset in revolutions. (1.0 equals 360 degrees)", 0.0f, -_FP32_MAX, _FP32_MAX },
	{ "Origin 1", "Origin of rotation component 1.", 0.0f, -_FP32_MAX, _FP32_MAX },
	{ "Origin 2", "Origin of rotation component 2.", 0.0f, -_FP32_MAX, _FP32_MAX },
	{ "Origin 3", "Origin of rotation component 3.", 0.0f, -_FP32_MAX, _FP32_MAX },
	{ "Axis component 1", "Rotations axis component 1.", 0.0f, -_FP32_MAX, _FP32_MAX },
	{ "Axis component 2", "Rotations axis component 2.", 0.0f, -_FP32_MAX, _FP32_MAX },
	{ "Axis component 3", "Rotations axis component 3.", 1.0f, -_FP32_MAX, _FP32_MAX },
};

bool CXR_VBOperator_Rotate::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_Rotate_OnOperate, false);
//	int nParams = _Oper.m_lParams.Len();

	if (_Oper.m_Components & XW_LAYEROP_TVERTEX)
	{

		int iParam = 0;
		CVec3Dfp32 Origin(0);
		CVec3Dfp32 Axis(0,0,1);

		fp32 Velocity = GetParam(_Oper, iParam++, 1.0f);
		fp32 Offset = GetParam(_Oper, iParam++, 0.0f);
		Origin[0] = GetParam(_Oper, iParam++, 0.0f);
		Origin[1] = GetParam(_Oper, iParam++, 0.0f);
		Origin[2] = GetParam(_Oper, iParam++, 0.0f);
		Axis[0] = GetParam(_Oper, iParam++, 0.0f);
		Axis[1] = GetParam(_Oper, iParam++, 0.0f);
		Axis[2] = GetParam(_Oper, iParam++, 1.0f);

		fp32 AxisLenSqr = Axis.LengthSqr();
		if (AxisLenSqr > Sqr(0.00001f)) Axis *= 1.0f / M_Sqrt(AxisLenSqr);

		fp32 Angle = (_Context.m_AnimTime.GetTimeModulusScaled(Velocity, 1.0f)) + Offset;

		CMat4Dfp32 MatRot;
		CAxisRotfp32 Rot(Axis, Angle);
		Rot.CreateMatrix(MatRot);

		CMat4Dfp32 MatT1, Tmp;
		MatT1.Unit();

		CVec3Dfp32::GetRow(MatT1, 3) = -Origin;
		MatT1.Multiply(MatRot, Tmp);

		CVec3Dfp32::GetRow(MatT1, 3) = Origin;
		Tmp.Multiply(MatT1, MatRot);

		UVW_AddTransform(_Context, _pVB, MatRot);
	}
	else
	{
		ConOutD(CStrF("§cf80WARNING: (CXR_VBOperator_Rotate::OnOperate) Unsupported components: %.4x", _Oper.m_Components));
	}

	return true;
}

bool CXR_VBOperator_Rotate::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_Rotate_OnTestHWAccelerated, false);
	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_GenReflection
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_GenReflection, CXR_VBOperator);

bool CXR_VBOperator_GenReflection::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_GenReflection_OnOperate, false);
	if (/*_pVB->m_VBID &&*/
		_Context.m_pEngine && 
		_Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_CUBEMAP)
	{
		if (_pVB->m_pAttrib)
		{
			_pVB->m_pAttrib->Attrib_TexGen(_Context.m_iTexChannel, CRC_TEXGENMODE_REFLECTION, CRC_TEXGENCOMP_ALL);

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

				_pVB->TextureMatrix_Set(_Context.m_iTexChannel, pW2VInv);
			}
			else
				ConOutD("(CXR_VBOperator_GenReflection::OnOperate) WARNING: No world-2-view transform available.");
		}
	}
	else
	{
/*		if (!pChainSrc->BuildVertexUsage(_Context.m_pVBM)) return false;

		const uint16* piV = pChainSrc->m_piVertUse;
		int nVA = pChainSrc->m_nV;
		int nV = (piV) ? pChainSrc->m_nVertUse : pChainSrc->m_nV;

		//  Environment mapping
		if (_pVB->m_pN)
		{
			if (_Context.m_iFreeTexCoordSet < 0)
			{
				ConOut("§cf80WARNING: (CXR_VBOperator_GenReflection::OnOperate) No free texture-coordinate set.");
				return false;
			}
			
			CVec2Dfp32* pUV = _Context.m_pVBM->Alloc_V2(nVA);
			if (!pUV) return false;


			CalcEnvironmentMappingCheap(_Context.m_pVBHead->m_pTransform, nV, piV, _pVB->m_pN, pUV);

			if (_pVB->m_pAttrib)
				_pVB->m_pAttrib->Attrib_TexCoordSet(_Context.m_iTexChannel, _Context.m_iFreeTexCoordSet);
			_pVB->Geometry_TVertexArray(pUV, _Context.m_iFreeTexCoordSet);
		}
		else
		{*/
			ConOutD("§cf80WARNING: (CXR_VBOperator_GenReflection::OnOperate) Software fallback not implemented.");
//		}

	}
	return true;
}

void CXR_VBOperator_GenReflection::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_GenReflection_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
	_pSurf->m_Requirements |= XW_SURFREQ_CUBEMAP;
}

bool CXR_VBOperator_GenReflection::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_GenReflection_OnTestHWAccelerated, false);
	if (_Context.m_pEngine && 
		_Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_CUBEMAP)
		return true;

#if defined(PLATFORM_DOLPHIN)
	OSReport( "WARNING: VBOperator_GenReflection() failed HW test.\n" );
#endif

	return true;	// We require cube maps so don't fail even if render says there's no cubemap support
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_GenEnv
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_GenEnv, CXR_VBOperator);

void CalcEnvironmentMappingCheap(const CMat4Dfp32* _pMat, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, CVec2Dfp32* _pTV);

bool CXR_VBOperator_GenEnv::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_GenEnv_OnOperate, false);
	if (_pVB->m_pAttrib)
		_pVB->m_pAttrib->Attrib_TexGen(_Context.m_iTexChannel, CRC_TEXGENMODE_ENV, CRC_TEXGENCOMP_ALL);
	return true;
}

void CXR_VBOperator_GenEnv::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_GenEnv_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
}

bool CXR_VBOperator_GenEnv::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_GenEnv_OnTestHWAccelerated, false);
	if (_Context.m_pEngine && 
		_Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_TEXGENMODE_ENV)
		return true;

#if defined(PLATFORM_DOLPHIN)
	if( _Context.m_pEngine )
		OSReport( "WARNING: VBOperator_GenEnv() failed HW test.\n" );
#endif

	return true;	// Failed but return true anyway
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_GenEnv2
|__________________________________________________________________________________________________
\*************************************************************************************************/

#if 1
MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_GenEnv2, CXR_VBOperator_GenEnv);

#else

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_GenEnv2, CXR_VBOperator);

bool CXR_VBOperator_GenEnv2::OnOperate(
	CXR_VBOperatorContext&			_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*			_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_GenEnv2_OnOperate, false);
	
	if( _Context.m_pEngine && _Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_TEXGENMODE_ENV2 )
	{
		if( _pVB->m_pAttrib )
			_pVB->m_pAttrib->Attrib_TexGen(_Context.m_iTexChannel, CRC_TEXGENMODE_ENV2, CRC_TEXGENCOMP_ALL);
	}
	else
	{
		CXR_VBChain *pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
		CXR_VBChain *pChain = _pVB->GetVBChain();

		if (!pChainSrc->BuildVertexUsage(_Context.m_pVBM)) 
			return false;

		const uint16* piV = pChainSrc->m_piVertUse;
		int nVA = pChainSrc->m_nV;
		int nV = (piV) ? pChainSrc->m_nVertUse : pChainSrc->m_nV;

		//  Environment2 mapping
		if (pChainSrc->m_pN)
		{
			if (_Context.m_iFreeTexCoordSet < 0)
			{
				ConOutD("§cf80WARNING: (CXR_VBOperator_GenEnv::OnOperate) No free texture-coordinate set.");
				return false;
			}

			CVec2Dfp32* pUV = _Context.m_pVBM->Alloc_V2(nVA);
			if (!pUV) return false;

			const CMat4Dfp32 *pM2VMat = _Context.m_pVBHead->m_pTransform;

			CMat4Dfp32 VMatInv;
			if (pM2VMat)
				pM2VMat->InverseOrthogonal(VMatInv);
			else
				VMatInv.Unit();

	//		CMat4Dfp32 Mat;
	//		Mat.Unit();
	//		CXR_Util::CalcEnvironmentMapping(CVec3Dfp32::GetMatrixRow(VMatInv, 3), Mat, nV, piV, _pVB->m_pN, _pVB->m_pV, pUV);
			CXR_Util::CalcEnvironmentMapping( CVec3Dfp32::GetMatrixRow(VMatInv, 3), nV, piV, pChain->m_pN, pChain->m_pV, pUV );

			if( _pVB->m_pAttrib )
				_pVB->m_pAttrib->Attrib_TexCoordSet( _Context.m_iTexChannel, _Context.m_iFreeTexCoordSet );

			_pVB->Geometry_TVertexArray(pUV, _Context.m_iFreeTexCoordSet);
		}
		else
		{
			ConOutD("§cf80WARNING: (CXR_VBOperator_GenEnv2::OnOperate) Missing normals.");
		}
	}

	return true;
}

void CXR_VBOperator_GenEnv2::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_GenEnv2_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
}

bool CXR_VBOperator_GenEnv2::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_GenEnv2_OnTestHWAccelerated, false);

#if defined(PLATFORM_DOLPHIN) || defined(PLATFORM_PS2)
	if( _Context.m_pEngine && 
		_Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_TEXGENMODE_ENV2 )
		return( true );

 #ifdef PLATFORM_DOLPHIN
	if( _Context.m_pEngine )
		OSReport( "WARNING: VBOperator_GenEnv2 failed HW test.\n" );
 #endif
#endif

	return false;
}

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Wave
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_Wave, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_Wave::ms_lParamDesc[] = 
{
	{ "Amplitude", "Amplitude in units.", 32, -_FP32_MAX, _FP32_MAX },
	{ "Frequency", "Oscillations per second.", 1, -_FP32_MAX, _FP32_MAX },
	{ "PeriodX", "Distance in units between wave tops projected on the X-Axis.", 256, -_FP32_MAX, _FP32_MAX },
	{ "PeriodY", "Distance in units between wave tops projected on the Y-Axis.", 256, -_FP32_MAX, _FP32_MAX },
	{ "PeriodZ", "Distance in units between wave tops projected on the Z-Axis.", 256, -_FP32_MAX, _FP32_MAX },
};
/*
static void RenderNormals(CXR_VBManager* _pVBM, const CMat4Dfp32& _Mat, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, fp32 _Len, CPixel32 _Color)
{
	MAUTOSTRIP(RenderNormals, MAUTOSTRIP_VOID);
	for(int v = 0; v < _nV; v++)
	{
		// This is _REALLY_ expensive.. becomes 1 CXR_VertexBuffer per wire
		_pVBM->RenderWire(_Mat, _pV[v], _pV[v] + _pN[v] * _Len, _Color);
	}
}
*/
bool CXR_VBOperator_Wave::OnOperate(

	CXR_VBOperatorContext&			_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*			_pVB)
{
	MAUTOSTRIP(CXR_VBOperator_Wave_OnOperate, false);

	bool	bHW = false;

#if defined(PLATFORM_DOLPHIN)
	if( _Context.m_pEngine && _Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_VBOPERATOR_WAVE )
		bHW = true;

	//	Only head of chain.
	if( bHW && _Context.m_VBChainIndex )
		return( true );
#endif

	//	Calculate time.
	fp32 t = _Context.m_AnimTime.GetTimeModulus(20000.0f);

	//	Extract params.
	int nParams = _Oper.m_lParams.Len();
	const fp32* pParams = _Oper.m_lParams.GetBasePtr();
	fp32 Amp = (0 < nParams) ? pParams[0] : 32;
	fp32 Frequency = (1 < nParams) ? pParams[1] : 1;
	fp32 PeriodX = (2 < nParams) ? pParams[2] : 64;
	fp32 PeriodY = (3 < nParams) ? pParams[3] : PeriodX;
	fp32 PeriodZ = (4 < nParams) ? pParams[4] : PeriodY;

	Frequency *= 2.0f * _PI;
	fp32 PeriodXRecp = (PeriodX != 0.0f) ? 1.0f / PeriodX : 0.0f;
	fp32 PeriodYRecp = (PeriodY != 0.0f) ? 1.0f / PeriodY : 0.0f;
	fp32 PeriodZRecp = (PeriodZ != 0.0f) ? 1.0f / PeriodZ : 0.0f;

	//	Let renderer take care of things?
	if( bHW )
	{
#if defined(PLATFORM_DOLPHIN)
		CRC_ExtAttributes_GC_VBWave *pGCExtAttrib = (CRC_ExtAttributes_GC_VBWave *)_Context.m_pVBM->Alloc( sizeof(CRC_ExtAttributes_GC_VBWave) );
		if( !pGCExtAttrib )
			return( false );

		//	Store some info.
		pGCExtAttrib->m_Time		= t;
		pGCExtAttrib->m_oPeriod		= CVec3Dfp32( PeriodXRecp, PeriodYRecp, PeriodZRecp );
		pGCExtAttrib->m_Frequency	= Frequency * 2;
		pGCExtAttrib->m_AttribType	= CRC_ATTRIBTYPE_GC_WAVE;
		pGCExtAttrib->m_Components	= _Oper.m_Components;
		pGCExtAttrib->m_Amp			= Amp * 3;

		if( _pVB->m_pAttrib )
			_pVB->m_pAttrib->m_pExtAttrib = pGCExtAttrib;

		return( true );
#endif
	}


	CXR_VBChain *pChain = _pVB->GetVBChain();

	if( !pChain->BuildVertexUsage( _Context.m_pVBM ) )
		return( false );

	const uint16* piV = pChain->m_piVertUse;
	int nVA = pChain->m_nV;
	int nV = (piV) ? pChain->m_nVertUse : pChain->m_nV;

//	ConOut(CStrF("(CXR_VBOperator_Wave::OnOperate) Comp %.4x, Amp %f, Freq %f, Period %f, %f, %f", _Oper.m_Components, Amp, Frequency, PeriodX, PeriodY, PeriodZ));

	if (pChain->m_pN)
	{
		const CVec3Dfp32* pVOld = pChain->m_pV;
		const CVec3Dfp32* pNOld = pChain->m_pN;
		CVec3Dfp32* pV = pChain->m_pV;
		CVec3Dfp32* pN = pChain->m_pN;
		if (_Context.m_VBArrayReadOnlyMask & 1)
			pV = _Context.m_pVBM->Alloc_V3(nVA);
		if (_Context.m_VBArrayReadOnlyMask & 2)
			pN = _Context.m_pVBM->Alloc_V3(nVA);
		if (!pV || !pN) return false;
/*		CVec3Dfp32* pdVdX = _Context.m_pVBM->Alloc_V3(nVA);
		CVec3Dfp32* pdVdY = _Context.m_pVBM->Alloc_V3(nVA);
		CVec3Dfp32* pdVdZ = _Context.m_pVBM->Alloc_V3(nVA);
		if (!pdVdX || !pdVdY || !pdVdZ) return false;*/

		for(int v = 0; v < nV; v++)
		{
			int iv = (piV) ? piV[v] : v;

			CVec3Dfp32 VOld = pVOld[iv];
			fp32 Displace = 
				QSin(VOld[0] * PeriodXRecp +
					VOld[1] * PeriodYRecp +
					VOld[2] * PeriodZRecp +
					t*Frequency);
			Displace *= Amp;

			for(int k = 0; k < 3; k++)
			{
				if (_Oper.m_Components & (1 << k))
					pV[iv][k] = VOld[k] + pNOld[iv][k] * Displace;
				else
					pV[iv][k] = VOld[k];
			}

			CVec3Dfp32 dVdX;
			pNOld[iv].Scale((Amp * PeriodXRecp * 
					QCos(VOld[0] * PeriodXRecp +
						VOld[1] * PeriodYRecp +
						VOld[2] * PeriodZRecp +
						t*Frequency)), dVdX);

			CVec3Dfp32 dVdY;
			pNOld[iv].Scale((Amp * PeriodYRecp * 
					QCos(VOld[0] * PeriodXRecp +
						VOld[1] * PeriodYRecp +
						VOld[2] * PeriodZRecp +
						t*Frequency
					)), dVdY);

			CVec3Dfp32 dVdZ;
			pNOld[iv].Scale((Amp * PeriodZRecp * 
					QCos(VOld[0] * PeriodXRecp +
						VOld[1] * PeriodYRecp +
						VOld[2] * PeriodZRecp +
						t*Frequency
					)), dVdZ);

/*			CMat4Dfp32 Mat;
			dVdX.SetRow(Mat,0);
			dVdY.SetRow(Mat,1);
			dVdZ.SetRow(Mat,2);
			Mat.Transpose();
			dVdX = -CVec3Dfp32::GetRow(Mat, 0);
			dVdY = -CVec3Dfp32::GetRow(Mat, 1);
			dVdZ = -CVec3Dfp32::GetRow(Mat, 2);
			pN[iv] = dVdZ + dVdX + dVdY + pNOld[iv];
*/
			pN[iv][0] = pNOld[iv][0] - (dVdX[0]+dVdX[1]+dVdX[2]);
			pN[iv][1] = pNOld[iv][1] - (dVdY[0]+dVdY[1]+dVdY[2]);
			pN[iv][2] = pNOld[iv][2] - (dVdZ[0]+dVdZ[1]+dVdZ[2]);

			pN[iv].Normalize();

/*				(2.0f*_PI*m_SPeriod0 * m_Amp0 * M_Cos((Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*2.0f*_PI) +
				2.0f*_PI*m_SPeriod1 * m_Amp1 * M_Cos((Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*2.0f*_PI) +
				2.0f*_PI*m_SPeriod2 * m_Amp2 * M_Cos((Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*2.0f*_PI)) * m_ScaleInv;
*/
		}

//		RenderNormals(_Context.m_pEngine->m_pRender, *_Context.m_pModel2View, nV, pV, pN, 4.0f, 0xff808000);
/*		RenderNormals(_Context.m_pEngine->m_pRender, *_Context.m_pModel2View, nV, pV, pdVdX, 4.0f, 0xff800000);
		RenderNormals(_Context.m_pEngine->m_pRender, *_Context.m_pModel2View, nV, pV, pdVdY, 4.0f, 0xff008000);
		RenderNormals(_Context.m_pEngine->m_pRender, *_Context.m_pModel2View, nV, pV, pdVdZ, 4.0f, 0xff000080);*/

/*{
	for(int v = 0; v < _nV; v++)
		_pRender->Render_Wire(_pV[v], _pV[v] + _pN[v]*_Len, _Color);
}*/

		pChain->m_pV = pV;
		pChain->m_pN = pN;
	}
	else
	{
		ConOutD("§cf80WARNING: (CXR_VBOperator_Wave::OnOperate) Missing normals.");
	}

	return true;
}

bool CXR_VBOperator_Wave::OnOperateFinish(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB)
{
	MAUTOSTRIP(CXR_VBOperator_Wave_OnOperateFinish, false);
	// Vertex and normal arrays are now writeable.
	_Context.m_VBArrayReadOnlyMask &= ~(1 | 2);
	return true;
}

void CXR_VBOperator_Wave::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_Wave_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
}

bool CXR_VBOperator_Wave::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_Wave_OnTestHWAccelerated, false);

#if defined(PLATFORM_DOLPHIN)
	if(	_Context.m_pEngine && 
		_Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_VBOPERATOR_WAVE)
		return( true );

	OSReport( "WARNING: VBOperator_GenEnv() failed HW test.\n" );
#endif

	return false;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_GenColorViewN
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_GenColorViewN, CXR_VBOperator_Util);

CXR_VBOperatorParamDesc CXR_VBOperator_GenColorViewN::ms_lParamDesc[] = 
{
	{ "Scale", "Scale in the expression Color.[r][g][b][a] = 255.0f * (Abs(N.z) * Scale + Offset)", 1.0f, 0, _FP32_MAX },
	{ "Offset", "Offset in the expression Color.[r][g][b][a] = 255.0f * (Abs(N.z) * Scale + Offset)", 0.0f, 0, _FP32_MAX },
};

bool CXR_VBOperator_GenColorViewN::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_GenColorViewN_OnOperate, false);

	CXR_VBChain *pChain = _pVB->GetVBChain();

	if (!pChain->BuildVertexUsage(_Context.m_pVBM)) 
		return false;

	const uint16* piV = pChain->m_piVertUse;
	int nVA = pChain->m_nV;
	int nV = (piV) ? pChain->m_nVertUse : pChain->m_nV;

//	fp32 ProjScale = GetParam(_Oper, 0, 1.0f);
//	fp32 ProjOfs = GetParam(_Oper, 1, 0.0f);

	if (!(_Oper.m_Components & XW_LAYEROP_COLOR))
	{	
		ConOutD(CStrF("§cf80WARNING: (CXR_VBOperator_GenColorViewN::OnOperate) Unsupported components: %.4x", _Oper.m_Components));
		return true;
	}

	if (pChain->m_pN)
	{
		CPixel32* pColCurrent = pChain->m_pCol;
		CPixel32* pCol = (CPixel32*) _Context.m_pVBM->Alloc_Int32(nVA);
		if (!pCol) return false;

		fp32 Scale = GetParam(_Oper, 0, 1.0f);
		fp32 Offset = GetParam(_Oper, 1, 0.0f);

		const CVec3Dfp32* pN = pChain->m_pN;

		for(int v = 0; v < nV; v++)
		{
			int iv = (piV) ? piV[v] : v;

			CVec3Dfp32 N = pN[iv];
			if(_Context.m_pModel2View)
				N.MultiplyMatrix3x3(*_Context.m_pModel2View);

			CPixel32 Col;
			if (pColCurrent)
				Col = pColCurrent[iv];
			else
				Col = _pVB->m_Color;

			int Intens = RoundToInt(Clamp01(Scale * M_Fabs(N[2]) + Offset)*255.0f);
			if (_Oper.m_Components & XW_LAYEROP_R) Col = (Col & 0xff00ffff) + (Intens << 16);
			if (_Oper.m_Components & XW_LAYEROP_G) Col = (Col & 0xffff00ff) + (Intens << 8);
			if (_Oper.m_Components & XW_LAYEROP_B) Col = (Col & 0xffffff00) + (Intens << 0);
			if (_Oper.m_Components & XW_LAYEROP_A) Col = (Col & 0x00ffffff) + (Intens << 24);
			pCol[iv] = Col;
		}

		pChain->m_pCol = pCol;
	}
	else
	{
		ConOutD("§cf80WARNING: (CXR_VBOperator_GenColorViewN::OnOperate) Missing normals.");
	}

	return true;
}

void CXR_VBOperator_GenColorViewN::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_GenColorViewN_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Particles
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_Particles, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_Particles::ms_lParamDesc[] = 
{
	{ "Size", "Particle size in units.", 16.0f, 0, _FP32_MAX },
};

bool CXR_VBOperator_Particles::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_Particles_OnOperate, false);
//	int nParams = _Oper.m_lParams.Len();

	if (_Oper.m_Components & XW_LAYEROP_VERTEX)
	{
		fp32 Size = GetParam(_Oper, 0, 16.0f);

		CXR_VBChain *pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
		CXR_VBChain *pChain = _pVB->GetVBChain();

		if (!pChainSrc->BuildVertexUsage(_Context.m_pVBM))
		{
			ConOutD("(CXR_VBOperator_Particles::OnOperate) Unable to built vertex usage.");
			return false;
		}

		const uint16* piV = pChainSrc->m_piVertUse;
//		int nVA = pChainSrc->m_nV;
		int nV = (piV) ? pChainSrc->m_nVertUse : pChainSrc->m_nV;

		CXR_Particle2* pP = (CXR_Particle2*)_Context.m_pVBM->Alloc(sizeof(CXR_Particle2) * nV);
		if (!pP)
		{
			ConOutD("(CXR_VBOperator_Particles::OnOperate) Unable to allocate particles.");
			return false;
		}

		CVec3Dfp32* pV = pChain->m_pV;

		for(int v = 0; v < nV; v++)
		{
			int iv = (piV) ? piV[v] : v;
			pP[v].m_Pos = pV[iv];
			pP[v].m_Angle = 0;
			pP[v].m_Size = Size;
			pP[v].m_Color = _pVB->m_Color;
		}

		CXR_VertexBuffer VBParticles;

		if (CXR_Util::Render_Particles(_Context.m_pVBM, &VBParticles, *_Context.m_pModel2View, pP, nV))
		{
			CXR_VBChain *pChainPart = VBParticles.GetVBChain();
			pChain->m_nV = pChainPart->m_nV;
			pChain->m_pV = pChainPart->m_pV;
			pChain->m_piPrim = pChainPart->m_piPrim;
			pChain->m_nPrim = pChainPart->m_nPrim;
			pChain->m_PrimType = pChainPart->m_PrimType;
			pChain->m_pTV[0] = pChainPart->m_pTV[0];
			pChain->m_pCol = pChainPart->m_pCol;
			_pVB->m_pTransform = NULL;
			return true;
		}
		else
		{
			ConOutD("(CXR_VBOperator_Particles::OnOperate) CXR_Util::Render_Particles Failed.");
			return false;
		}
	}
	else
	{
		ConOutD(CStrF("§cf80WARNING: (CXR_VBOperator_Particles::OnOperate) Unsupported components: %.4x", _Oper.m_Components));
	}

	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Debug_Normals
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_Debug_Normals, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_Debug_Normals::ms_lParamDesc[] = 
{
	{ "Length", "Normal length.", 8.0, -_FP32_MAX, _FP32_MAX },
};

bool CXR_VBOperator_Debug_Normals::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_Debug_Normals_OnOperate, false);

	CXR_VBChain *pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
	CXR_VBChain *pChain = _pVB->GetVBChain();

	if (!pChainSrc->BuildVertexUsage(_Context.m_pVBM)) 
		return false;

	const uint16* piV = pChainSrc->m_piVertUse;
//	int nVA = pChainSrc->m_nV;
	int nV = (piV) ? pChainSrc->m_nVertUse : pChainSrc->m_nV;

	int nParams = _Oper.m_lParams.Len();
	const fp32* pParams = _Oper.m_lParams.GetBasePtr();

	fp32 Len = (0 < nParams) ? pParams[0] : 8.0f;

//	ConOut(CStrF("(CXR_VBOperator_Debug_Normals::OnOperate) Comp %.4x, Amp %f, Freq %f, Period %f, %f, %f", _Oper.m_Components, Amp, Frequency, PeriodX, PeriodY, PeriodZ));

	if (pChain->m_pN)
	{
		CXR_VBManager* pVBM = _Context.m_pVBM;

		CMat4Dfp32 Mat;
		if (_Context.m_pVBHead->m_pTransform)
			Mat = *_Context.m_pVBHead->m_pTransform;
		else
			Mat.Unit();

		CVec3Dfp32* pV = pChain->m_pV;
		CVec3Dfp32* pN = pChain->m_pN;
		for(int v = 0; v < nV; v++)
		{
			int iv = (piV) ? piV[v] : v;
			pVBM->RenderWire(Mat, pV[iv], pV[iv] + pN[iv] * Len, 0xffffff00);
		}
	}
	else
	{
		ConOutD("§cf80WARNING: (CXR_VBOperator_Debug_Normals::OnOperate) Missing normals.");
	}

	return true;
}

void CXR_VBOperator_Debug_Normals::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_Debug_Normals_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Debug_Tangents
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_Debug_Tangents, CXR_VBOperator);

CXR_VBOperatorParamDesc CXR_VBOperator_Debug_Tangents::ms_lParamDesc[] = 
{
	{ "Length", "Tangents length.", 8.0, -_FP32_MAX, _FP32_MAX },
};

bool CXR_VBOperator_Debug_Tangents::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_Debug_Tangents_OnOperate, false);
	CXR_VBChain *pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
	CXR_VBChain *pChain = _pVB->GetVBChain();

	if (!pChainSrc->BuildVertexUsage(_Context.m_pVBM)) 
		return false;

	const uint16* piV = pChainSrc->m_piVertUse;
//	int nVA = pChainSrc->m_nV;
	int nV = (piV) ? pChainSrc->m_nVertUse : pChainSrc->m_nV;

	int nParams = _Oper.m_lParams.Len();
	const fp32* pParams = _Oper.m_lParams.GetBasePtr();

	fp32 Len = (0 < nParams) ? pParams[0] : 8.0f;

//	ConOutD(CStrF("(CXR_VBOperator_Debug_Tangents::OnOperate) Comp %.4x, Amp %f, Freq %f, Period %f, %f, %f", _Oper.m_Components, Amp, Frequency, PeriodX, PeriodY, PeriodZ));

	CVec3Dfp32* pTgU = (pChain->m_nTVComp[2] == 3) ? (CVec3Dfp32*)pChain->m_pTV[2] : NULL;
	CVec3Dfp32* pTgV = (pChain->m_nTVComp[3] == 3) ? (CVec3Dfp32*)pChain->m_pTV[3] : NULL;

	if (pTgU && pTgV)
	{
		CXR_VBManager* pVBM = _Context.m_pVBM;

		CMat4Dfp32 Mat;
		if (_Context.m_pVBHead->m_pTransform)
			Mat = *_Context.m_pVBHead->m_pTransform;
		else
			Mat.Unit();

		CVec3Dfp32* pV = pChain->m_pV;
		for(int v = 0; v < nV; v++)
		{
			int iv = (piV) ? piV[v] : v;

			pVBM->RenderWire(Mat, pV[iv], pV[iv] + pTgU[iv] * Len, 0xffff0000);
			pVBM->RenderWire(Mat, pV[iv], pV[iv] + pTgV[iv] * Len, 0xff00ff00);
		}
	}
	else
	{
		ConOutD("§cf80WARNING: (CXR_VBOperator_Debug_Tangents::OnOperate) Missing tangents.");
	}

	return true;
}

void CXR_VBOperator_Debug_Tangents::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_Debug_Tangents_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDTANGENTS | XW_SURFFLAGS_NEEDNORMALS;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_MulDecalBlend
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_MulDecalBlend, CXR_VBOperator_Util);

CXR_VBOperator_MulDecalBlend::CXR_VBOperator_MulDecalBlend()
{
	m_lTextureID[0] = 0;
	m_lTextureID[1] = 0;
}

bool CXR_VBOperator_MulDecalBlend::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_MulDecalBlend_OnOperate, false);

	if (!_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_MulDecalBlend::OnOperate) No attributes.");
		return false;
	}

	if(!_Context.m_pEngine)
		return false;

	int CapFlags = _Context.m_pEngine->m_RenderCaps_Flags;

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (0 && CapFlags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		CRC_ExtAttributes_FragmentProgram20* pAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));

		if (!pAttr) 
			return false;

		pAttr->Clear();

		int RasterMode1 = RoundToInt(GetParam(_Oper, 0, CRC_RASTERMODE_NONE));
		switch(RasterMode1)
		{
		case 20 :
			{
			};
			break;

		default:
			ConOut(CStrF("(CXR_VBOperator_MulDecalBlend::OnOperate) Unsupported mode %d", RasterMode1));
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
/*		if (0 && CapFlags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV10)
		{

			CRC_ExtAttributes_NV10* pAttr = (CRC_ExtAttributes_NV10*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
			if (!pAttr) return false;
			pAttr->Clear();

			int RasterMode1 = RoundToInt(GetParam(_Oper, 0, CRC_RASTERMODE_NONE));
			switch(RasterMode1)
			{
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
					ConOut(CStrF("(CXR_VBOperator_MulDecalBlend::OnOperate) Unsupported mode %d", RasterMode1));
				}
			}

			pAttr->SetFinal(NV_INPUT_FOG|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_FOG, NV_INPUT_SPARE0, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0|NV_COMP_ALPHA);
//			pAttr->SetFinal(NV_INPUT_FOG|NV_MAPPING_INVERT|NV_COMP_ALPHA, NV_INPUT_FOG, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0|NV_COMP_ALPHA);
			pAttr->Fixup();

			// Set program
			_pVB->m_pAttrib->m_pExtAttrib = pAttr;
			return true;
	}
	else */
#endif
	if ((CapFlags & CRC_CAPS_FLAGS_TEXENVMODE_COMBINE) && (_Context.m_pEngine->m_RenderCaps_nMultiTextureEnv >= 3))
	{
		if (_pVB->m_pAttrib)
		{
			int RasterMode1 = RoundToInt(GetParam(_Oper, 0, CRC_RASTERMODE_NONE));
			switch(RasterMode1)
			{
				
			case 20 :
				{
					// lrp r0.rgb, 1-v0, t0, t1
					// lrp r0.rgb, 1-v0.a, r0, c0
					
					_pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE_REPLACE);
					_pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE_COMBINE_LERP | CRC_TEXENVMODE_SOURCE2_PRIMARY);
					_pVB->m_pAttrib->Attrib_TexEnvMode(2, CRC_TEXENVMODE_COMBINE_LERP | CRC_TEXENVMODE_SOURCE2_PRIMARY | CRC_TEXENVMODE_RGB_SOURCE2_ALPHA);
					_pVB->m_pAttrib->Attrib_TextureID(2, m_lTextureID[0]);
				}
				break;
				
			case 21 :
				{
					// lrp r0.rgb, 1-v0, t0, t1
					// lrp r0.rgb, 1-v0.a, r0, c0
					
					_pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE_REPLACE);
					_pVB->m_pAttrib->Attrib_TexEnvMode(1, CRC_TEXENVMODE_COMBINE_LERP | CRC_TEXENVMODE_SOURCE2_PRIMARY);
					_pVB->m_pAttrib->Attrib_TexEnvMode(2, CRC_TEXENVMODE_COMBINE_LERP | CRC_TEXENVMODE_SOURCE2_PRIMARY | CRC_TEXENVMODE_RGB_SOURCE2_ALPHA);
					_pVB->m_pAttrib->Attrib_TextureID(2, m_lTextureID[1]);
				}
				break;
				
				
			default:
				ConOut(CStrF("(CXR_VBOperator_MulDecalBlend::OnOperate) Unsupported mode %d", RasterMode1));
			}
			return true;
		}
	}

	return false;
}

bool CXR_VBOperator_MulDecalBlend::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_MulDecalBlend_OnTestHWAccelerated, false);
	return true;
}

void CXR_VBOperator_MulDecalBlend::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_MulDecalBlend_OnInitSurface, MAUTOSTRIP_VOID);
//	_pSurf->m_Requirements |= XW_SURFREQ_NV20;
//	_pSurf->m_Requirements |= XW_SURFREQ_TEXENVMODE2;

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (pTC)
	{
		m_lTextureID[0] = pTC->GetTextureID("SPECIAL_FFFFFF");
		m_lTextureID[1] = pTC->GetTextureID("SPECIAL_808080");
	}
}

int CXR_VBOperator_MulDecalBlend::OnGetTextureCount(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper) const
{
	MAUTOSTRIP(CXR_VBOperator_NV20_Fresnel_OnGetTextureCount, 0);
	return 2;
}

int CXR_VBOperator_MulDecalBlend::OnEnumTextureID(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper, int _iEnum) const
{
	MAUTOSTRIP(CXR_VBOperator_NV20_Fresnel_OnEnumTextureID, 0);
	return m_lTextureID[_iEnum];
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_DarklingWallmark
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_UseShader, CXR_VBOperator);

CXR_VBOperator_UseShader::CXR_VBOperator_UseShader()
{
}


bool CXR_VBOperator_UseShader::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_UseShader_OnOperate, false);

	if (!_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_UseShader::OnOperate) No attributes.");
		return false;
	}

	if(!_Context.m_pEngine)
		return false;

	const int& CapFlags = _Context.m_pEngine->m_RenderCaps_Flags;

//#ifdef SUPPORT_FRAGMENTPROGRAM
	if (CapFlags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
//		const fp32 Range = 512.0f;
//		const fp32 RangeInv = 0.001953125f;
//		const fp32 Offset = 0.5f;
		const CMat4Dfp32* pMat = _pVB->m_pTransform;

		fp32* pTexGenAttr = _Context.m_pVBM->Alloc_fp32( 8 );
		
		_pVB->m_pAttrib->m_TextureID[1] = _Context.m_pEngine->m_pTC->GetTextureID( "frame" );

		CXR_SceneGraphInstance* pSGI = _Context.m_pEngine->m_pSceneGraphInstance;
		int iLightID = pSGI ? pSGI->SceneGraph_Light_GetIndex(0x2347) : 0;
		if(iLightID > 0)
		{
			CXR_Light Light;
			pSGI->SceneGraph_Light_Get(iLightID, Light);

			const CVec3Dfp32& LightDir = CVec3Dfp32::GetMatrixRow(Light.m_Pos, 2);
			const CVec3Dfp32& LightPos = CVec3Dfp32::GetMatrixRow(Light.m_Pos, 3);
			pTexGenAttr[0] = LightPos.k[0] + (LightDir.k[0] * 128.0f);
			pTexGenAttr[1] = LightPos.k[1] + (LightDir.k[1] * 128.0f);
			pTexGenAttr[2] = LightPos.k[2] + (LightDir.k[2] * 128.0f);
			pTexGenAttr[3] = 0.03125f; // 1.0f/32.0f
		}
		else
		{
			pTexGenAttr[0] = 0.0f;
			pTexGenAttr[1] = 0.0f;
			pTexGenAttr[2] = 128.0f;
			pTexGenAttr[3] = 0.03125f; // 1.0f/32.0f
		}

		if(pMat)
		{
			pTexGenAttr[4] = - (pMat->k[3][0]*pMat->k[0][0] + pMat->k[3][1]*pMat->k[0][1] + pMat->k[3][2]*pMat->k[0][2]);
			pTexGenAttr[5] = - (pMat->k[3][0]*pMat->k[1][0] + pMat->k[3][1]*pMat->k[1][1] + pMat->k[3][2]*pMat->k[1][2]);
			pTexGenAttr[6] = - (pMat->k[3][0]*pMat->k[2][0] + pMat->k[3][1]*pMat->k[2][1] + pMat->k[3][2]*pMat->k[2][2]);
		}
		pTexGenAttr[7] = 0.03125f; // 1.0f/32.0f

		_pVB->m_pAttrib->Attrib_TextureID(0, _pVB->m_pAttrib->m_TextureID[0]);	// Normal map
		_pVB->m_pAttrib->Attrib_TextureID(1, _pVB->m_pAttrib->m_TextureID[1]);	// Alpha map

		_pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_MSPOS, 0);
		_pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, 0);
		_pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL );
		_pVB->m_pAttrib->Attrib_TexGen(4, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL );
		_pVB->m_pAttrib->Attrib_TexGenAttr( (fp32*)pTexGenAttr );

        CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));

		if (!pFP) 
			return false;

		pFP->Clear();

		pFP->SetProgram( "WModel_DarklingEffect_FakeSpawn", MHASH8('WMod','el_D','arkl','ingE','ffec','t_Fa','keSp','awn'));

		_pVB->m_pAttrib->m_pExtAttrib = pFP;

		return true;
	}
//#endif

	return false;
}


void CXR_VBOperator_UseShader::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_UseShader_OnInitSurface, MAUTOSTRIP_VOID);
	//_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
}




/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_DepthOffset
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VBOperator_DepthOffset : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

	static CXR_VBOperatorParamDesc ms_lParamDesc[];

public:
	virtual CStr GetDesc() { return "Simulates a depth offset."; };
	virtual int GetNumParams() { return 1; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual void OnInitSurface(CXW_Surface* _pSurf, const CXW_LayerOperation& _Oper)
	{
		_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS | XW_SURFFLAGS_NEEDTANGENTS;
	}

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, CRC_Attributes* _pAttrib, const CXW_LayerOperation& _Oper)
	{
		return true;
	}

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const CXW_LayerOperation& _Oper, CXR_VertexBuffer* _pVB);
};


MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_DepthOffset, CXR_VBOperator);


CXR_VBOperatorParamDesc CXR_VBOperator_DepthOffset::ms_lParamDesc[] = 
{
	{ "Offset", "Offset.", 32.0f, -10000.0f, 10000.0f },
};


bool CXR_VBOperator_DepthOffset::OnOperate(CXR_VBOperatorContext& _Context, 
                                           const CXW_LayerOperation& _Oper, 
                                           CXR_VertexBuffer* _pVB)
{
	if (!_pVB->m_pAttrib)
		return false;

	if (!_Context.m_pModel2View)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_DepthOffset::OnOperate) No M2V matrix.");
		return false;
	}

	// Hardware mode
//	if(OnTestHWAccelerated(_Context, _pVB->m_pAttrib, _Oper)) Pointless since we know it returns true
	{
		fp32 Depth = GetParam(_Oper, 0, 32.0f);

		if(!_Context.m_pEngine || !_pVB->m_pAttrib)
			return false;
		
		// Setup texgen properties to insert
		int lTexGenModes[1] = { CRC_TEXGENMODE_DEPTHOFFSET };
		int lTexGenComp[1] = { CRC_TEXGENCOMP_ALL };
		int lTexGenChannels[1] = { _Context.m_iTexChannel };
		fp32* lpTexGenAttr[1];
    
		// Fetch texture ID and try to insert CRC_TEXGENMODE_DEPTHOFFSET into current set of texture generations
		uint16 TextureID = _pVB->m_pAttrib->m_TextureID[_Context.m_iTexChannel];
	    if(!TextureID || _Context.m_iFreeTexCoordSet < 0 || !_Context.m_pVBM->InsertTexGen(_pVB->m_pAttrib, 1, lTexGenModes, lTexGenComp, lTexGenChannels, &lpTexGenAttr[0]))
		    return false;

		// Get texture description
		int nMipmaps;
		CImage Desc;
		_Context.m_pEngine->m_pTC->GetTextureDesc(TextureID, &Desc, nMipmaps);
		int32 Width = Desc.GetWidth();
		int32 Height = Desc.GetHeight();

		// Calculate UV offset values
/*		enum { TexelsPerUnit = 8 };
		fp32 OffsetU = -(TexelsPerUnit * Depth) / Desc.GetWidth();
		fp32 OffsetV = -(TexelsPerUnit * Depth) / Desc.GetHeight();*/

		// Fetch model to view matrix
		const CMat4Dfp32& M2V = *_Context.m_pModel2View;
		
		// CRC_TEXGENMODE_DEPTHOFFSET
		fp32*M_RESTRICT pTexGenAttr = lpTexGenAttr[0];
		
		// Modelspace camera position
		pTexGenAttr[0] = -(M2V.k[3][0] * M2V.k[0][0] + M2V.k[3][1] * M2V.k[0][1] + M2V.k[3][2] * M2V.k[0][2]);
		pTexGenAttr[1] = -(M2V.k[3][0] * M2V.k[1][0] + M2V.k[3][1] * M2V.k[1][1] + M2V.k[3][2] * M2V.k[1][2]);
		pTexGenAttr[2] = -(M2V.k[3][0] * M2V.k[2][0] + M2V.k[3][1] * M2V.k[2][1] + M2V.k[3][2] * M2V.k[2][2]);
		pTexGenAttr[3] = 0;

		vec128 vDepth = M_VLd32(&Depth);
		vec128 vImgSize = M_VCnv_i32_f32(M_VMrgXY(M_VLdScalar_i32(Width), M_VLdScalar_i32(Height)));
		vec128 vOffset = M_VMul(M_VMul(M_VConst(-8.0f, -8.0f, 0.0f, 0.0f), vDepth), M_VRcp(vImgSize));

		// Offset U and V
		M_VSt(vOffset, pTexGenAttr+4);
/*		pTexGenAttr[4] = OffsetU;
		pTexGenAttr[5] = OffsetV;
		pTexGenAttr[6] = 0;
		pTexGenAttr[7] = 0;*/

		// Priority offset
		_pVB->m_Priority += GetParam(_Oper, 1, 0.0f);

		// Bind tangents texture coordinates
		_pVB->m_pAttrib->Attrib_TexCoordSet(2, 1);
		_pVB->m_pAttrib->Attrib_TexCoordSet(3, 2);

		return true;
	}

	// Software mode
#if 0
	CXR_VBChain* pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
	CXR_VBChain* pChainRef = _pVB->GetVBChain();
	CXR_VBChain* pChain = (CXR_VBChain*) _Context.m_pVBM->Alloc(sizeof(CXR_VBChain));
	if (!pChain)
		return false;
	memcpy(pChain, pChainRef, sizeof(CXR_VBChain));
	_pVB->SetVBChain(pChain);

	const CVec3Dfp32* pVertices = pChain->m_pV;
	const CVec2Dfp32* pSrcUV = (const CVec2Dfp32*)pChain->m_pTV[0];
	const CVec3Dfp32* pNorm = pChain->m_pN;
	uint8 iTangentBase = 1;
//	if (!pChain->m_pTV[3])
//		iTangentBase = 1; // ugly hack to support TangU,TangV stored in either 1,2 or 2,3
	const CVec3Dfp32* pTangU = (const CVec3Dfp32*)pChain->m_pTV[iTangentBase];
	const CVec3Dfp32* pTangV = (const CVec3Dfp32*)pChain->m_pTV[iTangentBase+1];

	if (!pVertices || !pSrcUV || !pNorm || !pTangU || !pTangV || pChain->m_nTVComp[iTangentBase] != 3 || pChain->m_nTVComp[iTangentBase+1] != 3)
		return false;

	uint16 TextureID = _pVB->m_pAttrib->m_TextureID[_Context.m_iTexChannel];
	if (!TextureID)
		return false;

	int nMipmaps;
	CImage Desc;
	_Context.m_pEngine->m_pTC->GetTextureDesc(TextureID, &Desc, nMipmaps);

	enum { TexelsPerUnit = 8 };
	fp32 Depth = GetParam(_Oper, 0, 32.0f);
	fp32 OffsetU = -(TexelsPerUnit * Depth) / Desc.GetWidth();
	fp32 OffsetV = -(TexelsPerUnit * Depth) / Desc.GetHeight();

	if (_Context.m_iFreeTexCoordSet < 0)
		return false;

	if (!pChainSrc->BuildVertexUsage(_Context.m_pVBM)) 
		return false;

	const uint16* piV = pChainSrc->m_piVertUse;
	int nVA = pChainSrc->m_nV;
	int nV = (piV) ? pChainSrc->m_nVertUse : pChainSrc->m_nV;

	const CMat4Dfp32& M2V = *_Context.m_pModel2View;
	CVec3Dfp32 ModelSpaceCamPos;
	ModelSpaceCamPos.k[0] = -(M2V.k[3][0] * M2V.k[0][0] + M2V.k[3][1] * M2V.k[0][1] + M2V.k[3][2] * M2V.k[0][2]);
	ModelSpaceCamPos.k[1] = -(M2V.k[3][0] * M2V.k[1][0] + M2V.k[3][1] * M2V.k[1][1] + M2V.k[3][2] * M2V.k[1][2]);
	ModelSpaceCamPos.k[2] = -(M2V.k[3][0] * M2V.k[2][0] + M2V.k[3][1] * M2V.k[2][1] + M2V.k[3][2] * M2V.k[2][2]);

	CVec2Dfp32* pUV = _Context.m_pVBM->Alloc_V2(nVA);
	if (!pUV) 
		return false;

	if (piV)
	{
		for (int v = 0; v < nV; v++)
		{
			uint16 iV = piV[v];
			const CVec3Dfp32& Pos = pVertices[iV];
			CVec3Dfp32 EyeVec = ModelSpaceCamPos;
			EyeVec -= Pos;
			EyeVec.Normalize();

			const CVec3Dfp32& NBT_x = pNorm[iV];
			const CVec3Dfp32& NBT_y = pTangU[iV];
			const CVec3Dfp32& NBT_z = pTangV[iV];
			CVec3Dfp32 EyeVecTangentSpace;
			EyeVecTangentSpace.k[0] = EyeVec * NBT_x;
			EyeVecTangentSpace.k[1] = EyeVec * NBT_y;
			EyeVecTangentSpace.k[2] = EyeVec * NBT_z;

			pUV[iV].k[0] = pSrcUV[iV].k[0] + EyeVecTangentSpace.k[1] * OffsetU / EyeVecTangentSpace.k[0];
			pUV[iV].k[1] = pSrcUV[iV].k[1] + EyeVecTangentSpace.k[2] * OffsetV / EyeVecTangentSpace.k[0];
		}
	}
	else
	{
		ConOutD("§cf80WARNING: (CXR_VBOperator_DepthOffset::OnOperate) No vertex usage. TODO: Add support for this");
		return false;
	}

	_pVB->m_pAttrib->Attrib_TexCoordSet(_Context.m_iTexChannel, _Context.m_iFreeTexCoordSet);
	_pVB->Geometry_TVertexArray(pUV, _Context.m_iFreeTexCoordSet);
	_pVB->m_Priority += GetParam(_Oper, 1, 0.0f);
	return true;
#endif
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_DrawOrder
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VBOperator_DrawOrder : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

	static CXR_VBOperatorParamDesc ms_lParamDesc[];

public:
	virtual CStr GetDesc() { return "Applies draw order offset."; };
	virtual int GetNumParams() { return 1; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual void OnInitSurface(CXW_Surface* _pSurf, const CXW_LayerOperation& _Oper)
	{
	}

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, CRC_Attributes* _pAttrib, const CXW_LayerOperation& _Oper)
	{
		return true;
	}

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const CXW_LayerOperation& _Oper, CXR_VertexBuffer* _pVB);
};


MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_DrawOrder, CXR_VBOperator);


CXR_VBOperatorParamDesc CXR_VBOperator_DrawOrder::ms_lParamDesc[] = 
{
	{ "DrawOrder", "Offset", 0.0, -10000.0f, 10000.0f },
};


bool CXR_VBOperator_DrawOrder::OnOperate(CXR_VBOperatorContext& _Context, 
                                           const CXW_LayerOperation& _Oper, 
                                           CXR_VertexBuffer* _pVB)
{
	if (!_pVB->m_pAttrib)
		return false;
	_pVB->m_Priority += GetParam(_Oper, 0, 0.0f);
	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NormalTransform
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_NormalTransform : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

	static CXR_VBOperatorParamDesc ms_lParamDesc[];

public:
	virtual CStr GetDesc() { return "Init normal decal render state."; };
	virtual int GetNumParams() { return 1; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual void OnInitSurface(CXW_Surface* _pSurf, const CXW_LayerOperation& _Oper)
	{
	}

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, CRC_Attributes* _pAttrib, const CXW_LayerOperation& _Oper)
	{
		return true;
	}

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const CXW_LayerOperation& _Oper, CXR_VertexBuffer* _pVB);
};


MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_NormalTransform, CXR_VBOperator);


CXR_VBOperatorParamDesc CXR_VBOperator_NormalTransform::ms_lParamDesc[] = 
{
	{ "DecalNormalTransform", "-", 0.0, -10000.0f, 10000.0f },
};


bool CXR_VBOperator_NormalTransform::OnOperate(CXR_VBOperatorContext& _Context, 
                                           const CXW_LayerOperation& _Oper, 
                                           CXR_VertexBuffer* _pVB)
{
	if (!_pVB->m_pAttrib)
		return false;

	CRC_ExtAttributes_FragmentProgram20* pAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
	if (!pAttr)
		return false;

	pAttr->Clear();
	pAttr->SetProgram("XRShader_DecalNormalTransform",MHASH8('XRSh','ader','_Dec','alNo','rmal','Tran','sfor','m'));
	//_pVB->m_pAttrib->Attrib_TexCoordSet(2,1);
	//_pVB->m_pAttrib->Attrib_TexCoordSet(3,2);

	_pVB->m_pAttrib->m_pExtAttrib = pAttr;

	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_FP20_Water
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_FP20_Water, CXR_VBOperator_Util);

CXR_VBOperator_FP20_Water::CXR_VBOperator_FP20_Water()
{
	MAUTOSTRIP(CXR_VBOperator_FP20_Water_ctor, MAUTOSTRIP_VOID);
}

bool CXR_VBOperator_FP20_Water::OnOperate(

	CXR_VBOperatorContext&				_Context, 
	const class CXW_LayerOperation&		_Oper, 
	class CXR_VertexBuffer*				_pVB)

{
	MAUTOSTRIP(CXR_VBOperator_FP20_Water_OnOperate, false);
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
		ConOut("§cf80WARNING: (CXR_VBOperator_FP20_Water::OnOperate) No attributes.");
		return false;
	}

	if (_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		CRC_ExtAttributes_FragmentProgram20* pAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pAttr) 
			return false;

		pAttr->Clear();

		CVec4Dfp32* pParams = _Context.m_pVBM->Alloc_V4(1+4+4);
		if (!pParams) 
			return false;

		CVec4Dfp32* pFPParams = pParams + (1+4);

		pAttr->m_pProgramName = "VBOp_FP20_Water";
		
		int FresnelMul = RoundToInt(GetParam(_Oper, 0, 1.0f) * 127.0f);
		int FresnelAdd = RoundToInt(GetParam(_Oper, 1, 0.0f) * 127.0f);

		pFPParams[0] = FresnelMul / 255.0f;
		pFPParams[1] = FresnelAdd / 255.0f;
		CMTime Time = _Context.m_pEngine->GetEngineTime();
		pFPParams[0][3] = Time.GetTimeModulus(8192.0f);
		pAttr->SetParameters(pFPParams, 4);

		CMat4Dfp32 V2M;
		if (!_Context.m_pModel2View)
		{
			ConOut("§cf80WARNING: (CXR_VBOperator_NV20_GenEnv::OnOperate) No M2V matrix.");
			return false;
		}
		_Context.m_pModel2View->InverseOrthogonal(V2M);

		CVec3Dfp32 Fwd = CVec3Dfp32::GetRow(V2M, 2);
		CVec3Dfp32 Right = CVec3Dfp32::GetRow(V2M, 0);
		CVec3Dfp32 Normal(0,0,1);
		Fwd = Fwd - (Normal*(Fwd*Normal));
		Right = Right - (Normal*(Right*Normal));
		pFPParams[2][0] = Fwd[0]; pFPParams[2][1] = Fwd[1]; pFPParams[2][2] = Fwd[2]; pFPParams[2][3] = 0;
		pFPParams[3][0] = Right[0]; pFPParams[3][1] = Right[1]; pFPParams[3][2] = Right[2]; pFPParams[3][3] = 0;

		// For CRC_TEXGENMODE_TSLV
		pParams[0][0] = V2M.k[3][0];
		pParams[0][1] = V2M.k[3][1];
		pParams[0][2] = V2M.k[3][2];
		pParams[0][3] = 1.0f;

		// For CRC_TEXGENMODE_LINEAR
		_pVB->m_pTransform->Multiply(_Context.m_pEngine->m_Screen_TextureProjection, *((CMat4Dfp32*)&pParams[1]));
		((CMat4Dfp32*)&pParams[1])->Transpose();

		_pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pParams);

		_pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(4, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(5, CRC_TEXGENMODE_NORMALMAP, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(6, CRC_TEXGENMODE_TANG_U, CRC_TEXGENCOMP_ALL);
		_pVB->m_pAttrib->Attrib_TexGen(7, CRC_TEXGENMODE_TANG_V, CRC_TEXGENCOMP_ALL);
//		_pVB->m_pAttrib->Attrib_TexCoordSet(2, 1);
//		_pVB->m_pAttrib->Attrib_TexCoordSet(3, 2);

		_pVB->m_pAttrib->Attrib_TextureID(4, _Context.m_pEngine->m_TextureID_ResolveScreen);
		_Context.m_pEngine->GetVC()->m_bNeedResolve_TempFixMe = 1;

		_pVB->m_pAttrib->m_pExtAttrib = pAttr;
		for(int i = 0; i < 8; i++)
			_pVB->m_pAttrib->m_iTexCoordSet[i] = i;

		_Context.m_iTexChannelNext = 8;
		return true;
	}
	return true;
}

bool CXR_VBOperator_FP20_Water::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_FP20_Water_OnTestHWAccelerated, false);
	return true;
}

void CXR_VBOperator_FP20_Water::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_FP20_Water_OnInitSurface, MAUTOSTRIP_VOID);
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDTANGENTS;
//	_pSurf->m_Requirements |= XW_SURFREQ_NV20;

}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_FP20_Distort
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_FP20_Distort, CXR_VBOperator_Util);


CXR_VBOperatorParamDesc CXR_VBOperator_FP20_Distort::ms_lParamDesc[] = 
{
	{ "Animate", "Animated.", 1.0f, 0.0f, 1.0f },
	{ "SpeedX", "Animation speed in x direction.", 1.0f, -1000.0f, 1000.0f },
	{ "SpeedY", "Animation speed in y direction.", 1.0f, -1000.0f, 1000.0f },
	{ "Distortion", "Distortion magnitude.", 2.0f, 0.0f, 1000.0f },
};


CXR_VBOperator_FP20_Distort::CXR_VBOperator_FP20_Distort()
{
	MAUTOSTRIP(CXR_VBOperator_FP20_Distort_ctor, MAUTOSTRIP_VOID);
}


bool CXR_VBOperator_FP20_Distort::OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB)
{
	MAUTOSTRIP(CXR_VBOperator_FP20_Distort_OnOperate, false);

	// Assign the required geometry. Obsolete since we always copy all texcoord arrays to the final VB.
	if (!_pVB->IsVBIDChain())
	{
		CXR_VBChain *pChain = _pVB->GetVBChain();
		CXR_VBChain *pChainSrc = _Context.m_pVBHeadSrc->GetVBChain();
		if (!pChain->m_pTV[1]) 
			pChain->m_pTV[1] = pChainSrc->m_pTV[2];
	}

	if(!_Context.m_pEngine || !_pVB->m_pAttrib)
	{
		ConOut("§cf80WARNING: (CXR_VBOperator_FP20_Distort::OnOperate) No Engine/Attributes.");
		return false;
	}

	if (!(_Context.m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20))
		return true;

	// Create fragment program
	CXR_Engine* pEngine = _Context.m_pEngine;
	CXR_VBManager* pVBM = _Context.m_pVBM;
	CRC_Attributes* pA = _pVB->m_pAttrib;
	CRC_ExtAttributes_FragmentProgram20* pExtAttr = (CRC_ExtAttributes_FragmentProgram20*)pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
	CVec4Dfp32* pFPParams = pVBM->Alloc_V4(2);

	if(!pExtAttr || !pFPParams)
		return false;

	int bAnimate = TruncToInt(GetParam(_Oper, 0, 1.0f));

	pExtAttr->Clear();
	pExtAttr->m_pProgramName = "VBOp_FP20_Distort";

	// Setup program parameters
	{
		const CRC_Viewport* pVP = pVBM->Viewport_Get();
		const CMat4Dfp32& ProjMat = ((CRC_Viewport*)pVP)->GetProjectionMatrix();
		int bVertFlip = pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
		const CPnt ScreenSize = pEngine->m_pRender->GetDC()->GetScreenSize();
		const CPnt ScreenTexSize = (pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE) ?
			ScreenSize : CPnt(GetGEPow2(ScreenSize.x), GetGEPow2(ScreenSize.y));
		const fp32 sw = (fp32)pVP->GetViewRect().GetWidth();
		const fp32 sh = (fp32)pVP->GetViewRect().GetHeight();
		
		fp32 MaxX = sw / ScreenTexSize.x;
		fp32 MaxY = sh / ScreenTexSize.y;

		// Fragment Param 0, Min/Max xy
		pFPParams[0].k[0] = 0.0f;
		pFPParams[0].k[2] = MaxX;
		if(bVertFlip)
		{
			pFPParams[0].k[1] = (1.0f - MaxY);
			pFPParams[0].k[3] = 1.0f;
		}
		else
		{
			pFPParams[0].k[1] = 1.0f;						
			pFPParams[0].k[3] = (1.0f - MaxY);
		}
		
		// Fragment Param 1, AnimTime, ScrollX, ScrollY, Magnitude
		pFPParams[1].k[0] = bAnimate ? _Context.m_AnimTime.GetTime() : 0.0f;
		pFPParams[1].k[1] = GetParam(_Oper, 1, 1.0f);
		pFPParams[1].k[2] = GetParam(_Oper, 2, 1.0f);
		pFPParams[1].k[3] = GetParam(_Oper, 3, 2.0f);

		pExtAttr->SetParameters(pFPParams, 2);
	}

	// Move texture id's forward to give place for screen texture
	for(uint i = CRC_MAXTEXTURES-1; i >= 1; i--)
		pA->Attrib_TextureID(i, pA->m_TextureID[i-1]);

	CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(pEngine->GetInterface(XR_ENGINE_TCSCREEN));
	int ScreenID = (pTCScreen) ? pTCScreen->GetTextureID(6) : 0;

	// Set screen texture
	pA->Attrib_TextureID(0, ScreenID);

	pA->m_pExtAttrib = pExtAttr;

	for(uint i = 0; i < 8; i++)
		pA->m_iTexCoordSet[i] = i;

	_Context.m_iTexChannelNext = 8;

	return true;
}


bool CXR_VBOperator_FP20_Distort::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_FP20_Distort_OnTestHWAccelerated, false);
	return true;
}


void CXR_VBOperator_FP20_Distort::OnInitSurface(class CXW_Surface* _pSurface, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_FP20_Distort_OnInitSurface, MAUTOSTRIP_VOID);
	
	// Hmm,,, let me think a little while here...
	//_pSurface->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
	//_pSurface->m_Flags |= XW_SURFFLAGS_NEEDTANGENTS;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_TXP1
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_TexEnvProj1 : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "TexEnvProj1."; };
	virtual int GetNumParams() { return 0; };
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, CRC_Attributes* _pAttrib, const CXW_LayerOperation& _Oper)
	{
		return true;
	}

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const CXW_LayerOperation& _Oper, CXR_VertexBuffer* _pVB);
};


MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_TexEnvProj1, CXR_VBOperator);


bool CXR_VBOperator_TexEnvProj1::OnOperate(CXR_VBOperatorContext& _Context, 
											   const CXW_LayerOperation& _Oper, 
											   CXR_VertexBuffer* _pVB)
{
	if (!_pVB->m_pAttrib)
		return false;

	CRC_ExtAttributes_FragmentProgram20* pAttr = (CRC_ExtAttributes_FragmentProgram20*)_Context.m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
	if (!pAttr)
		return false;

	pAttr->Clear();
	pAttr->SetProgram("TexEnvProj1",MHASH3('TexE','nvPr','oj1'));

	_pVB->m_pAttrib->m_pExtAttrib = pAttr;
	return true;
}

