/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WModel_Drain.cpp

Author:			Patrik Willbo

Copyright:		2006 Starbreeze Studios AB

Contents:		CXR_Model_Drain
CXR_Model_Drain_Instance

Comments:

History:
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WModel_Drain.h"
#include "../WObj_Misc/WObj_TentacleSystem_ClientData.h"


MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Drain, CXR_Model_Custom);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Drain_Instance
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Model_Drain_Instance::CXR_Model_Drain_Instance()
{
	m_bLast = false;
}


CXR_Model_Drain_Instance::~CXR_Model_Drain_Instance()
{
}


void CXR_Model_Drain_Instance::Create(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
}


void CXR_Model_Drain_Instance::OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32* _pMat, int _nMat, int _Flags)
{
}


bool CXR_Model_Drain_Instance::NeedRefresh(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
	return false;
}


TPtr<CXR_ModelInstance> CXR_Model_Drain_Instance::Duplicate() const
{
	CXR_Model_Drain_Instance* pDuplicated = MNew(CXR_Model_Drain_Instance);
	return TPtr<CXR_ModelInstance>(pDuplicated);
}


void CXR_Model_Drain_Instance::operator = (const CXR_ModelInstance& _Instance)
{
	const CXR_Model_Drain_Instance& Src = *safe_cast<const CXR_Model_Drain_Instance>(&_Instance);
	*this = Src;
}


void CXR_Model_Drain_Instance::operator = (const CXR_Model_Drain_Instance& _Instance)
{
}


void CXR_Model_Drain_Instance::SetLastEntry(uint _iEntry, const CVec3Dfp32& _EntryPos, const CVec3Dfp32& _EntryTng)
{
	m_LastPos[_iEntry] = _EntryPos;
	m_LastTng[_iEntry] = _EntryTng;
}


CVec3Dfp32 CXR_Model_Drain_Instance::GetLerpPos(uint _iEntry, const fp32 _t, const CVec3Dfp32& _Pos)
{
	CVec3Dfp32 LerpPos;
	m_LastPos[_iEntry].Lerp(_Pos, _t, LerpPos);
	return LerpPos;
}


CVec3Dfp32 CXR_Model_Drain_Instance::GetLerpTng(uint _iEntry, const fp32 _t, const CVec3Dfp32& _Tng)
{
	CVec3Dfp32 LerpTng;
	m_LastTng[_iEntry].Lerp(_Tng, _t, LerpTng);
	return LerpTng;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Drain
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Model_Drain::CXR_Model_Drain()
{
}


TPtr<CXR_ModelInstance> CXR_Model_Drain::CreateModelInstance()
{
	return MNew(CXR_Model_Drain_Instance);
}


void CXR_Model_Drain::OnCreate(const char* _pParam)
{
	ParseKeys(_pParam);
}


void CXR_Model_Drain::OnEvalKey(const CRegistry *_pReg)
{
	CStr KeyName = _pReg->GetThisName();
	CStr KeyValue = _pReg->GetThisValue();
	uint32 KeyHash = StringToHash(KeyName);

	switch(KeyHash)
	{
	case MHASH1('SU'):
		{
			m_iSurfaceID = GetSurfaceID(KeyValue);
			break;
		}

	default:
		{
			CXR_Model_Custom::OnEvalKey(_pReg);
			break;
		}
	}
}


CVec3Dfp32 CXR_Model_Drain::GetOffset(fp32 _Time, uint32 _Seed, const CVec3Dfp32& _Speed, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max)
{
	CVec3Dfp32 MinInv = CVec3Dfp32(Clamp01(1.0f-_Min.k[0]), Clamp01(1.0f-_Min.k[1]), Clamp01(1.0f-_Min.k[1]));
	switch (_Seed % 4)
	{
	case 1:
		{
			return CVec3Dfp32(
				 M_Sin(((MFloat_GetRand(_Seed+0) * MinInv.k[0]) + _Min.k[0]) * _Time * _Speed.k[0]) * _Max.k[0],
				 M_Cos(((MFloat_GetRand(_Seed+1) * MinInv.k[1]) + _Min.k[1]) * _Time * _Speed.k[1]) * _Max.k[1],
				-M_Sin(((MFloat_GetRand(_Seed+2) * MinInv.k[2]) + _Min.k[2]) * _Time * _Speed.k[2]) * _Max.k[2]);
		}
	case 2:
		{
			return CVec3Dfp32(
				 M_Cos(((MFloat_GetRand(_Seed+0) * MinInv.k[0]) + _Min.k[0]) * _Time * _Speed.k[0]) * _Max.k[0],
				-M_Sin(((MFloat_GetRand(_Seed+1) * MinInv.k[1]) + _Min.k[1]) * _Time * _Speed.k[1]) * _Max.k[1],
				-M_Cos(((MFloat_GetRand(_Seed+2) * MinInv.k[2]) + _Min.k[2]) * _Time * _Speed.k[2]) * _Max.k[2]);
		}
	case 3:
		{
			return CVec3Dfp32(
				-M_Sin(((MFloat_GetRand(_Seed+0) * MinInv.k[0]) + _Min.k[0]) * _Time * _Speed.k[0]) * _Max.k[0],
				 M_Cos(((MFloat_GetRand(_Seed+1) * MinInv.k[1]) + _Min.k[1]) * _Time * _Speed.k[1]) * _Max.k[1],
				 M_Sin(((MFloat_GetRand(_Seed+2) * MinInv.k[2]) + _Min.k[2]) * _Time * _Speed.k[2]) * _Max.k[2]);
		}
	default:
		{
			return CVec3Dfp32(
				 M_Cos(((MFloat_GetRand(_Seed+0) * MinInv.k[0]) + _Min.k[0]) * _Time * _Speed.k[0]) * _Max.k[0],
				 M_Sin(((MFloat_GetRand(_Seed+1) * MinInv.k[1]) + _Min.k[1]) * _Time * _Speed.k[1]) * _Max.k[1],
				-M_Cos(((MFloat_GetRand(_Seed+2) * MinInv.k[2]) + _Min.k[2]) * _Time * _Speed.k[2]) * _Max.k[2]);
		}
	}
}


void CXR_Model_Drain::OnRender(CXR_Engine *_pEngine, CRenderContext *_pRender, CXR_VBManager *_pVBM, CXR_ViewClipInterface *_pViewClip, 
							   spCXR_WorldLightState _spWLS, const CXR_AnimState *_pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, int _Flags)
{
	if (_pAnimState->m_pModelInstance)
	{
		fp32 AnimTime = _pAnimState->m_AnimTime0.GetTime();
		CXR_Model_Drain_Instance* pHistory = (CXR_Model_Drain_Instance*)_pAnimState->m_pModelInstance;

		uint Rand = (aint)pHistory;
		CSpline_BeamStrip BeamSpline(6);
		CFXVBMAllocUtil AllocUtil;

		CMat4Dfp32 Mat;
		Mat.Unit();

		fp32 Fade = _pAnimState->m_AnimTime1.GetTime();
		uint8 Point0 = MinMT(255, TruncToInt(255.0f * Clamp01((Fade - 0.5f) * 2.0f)));
		uint8 Point1 = MinMT(170, TruncToInt(170.0f * Clamp01((Fade - 0.25f) * 2.0f)));
		uint8 Point2 = MinMT( 85, TruncToInt( 85.0f * Clamp01(Fade * 4.0f)));

		for (uint i = 0; i < 6; i++)
		{
			uint iH = i * 3;
			
			fp32 iFp32 = fp32(i*10);
			CVec3Dfp32 Dst0 = _WMat.GetRow(3) + (_WMat.GetRow(0) *  8.0f) + GetOffset(AnimTime+(iFp32), uint32(Rand + (iH*16)), CVec3Dfp32(0.5f), CVec3Dfp32(0.2f), CVec3Dfp32(1.0f));
			CVec3Dfp32 Dst1 = _WMat.GetRow(3) + (_WMat.GetRow(0) * 16.0f) + GetOffset(AnimTime+(iFp32*2), uint32(Rand + (iH*32)), CVec3Dfp32(1.0f), CVec3Dfp32(0.5f), CVec3Dfp32(3.0f));
			CVec3Dfp32 Dst2 = _WMat.GetRow(3) + (_WMat.GetRow(0) * 24.0f) + GetOffset(AnimTime+(iFp32*4), uint32(Rand + (iH*64)), CVec3Dfp32(1.5f), CVec3Dfp32(0.8f), CVec3Dfp32(5.0f));
			if (!pHistory->m_bLast)
			{
				pHistory->m_LastPos[iH+0] = Dst0;
				pHistory->m_LastPos[iH+1] = Dst1;
				pHistory->m_LastPos[iH+2] = Dst2;
			}

			fp32 UOffset = AnimTime * ((MFloat_GetRand(i*0xdead) * 0.333f) + 0.333f);
			fp32 Width0 = 5.0f;// + M_Fabs(M_Cos(AnimTime*(((MFloat_GetRand((i*0xfab5)+0) - 1.0f) * 1.0f) * 4.0f)));
			fp32 Width1 = 5.0f;// + M_Fabs(M_Cos(AnimTime*(((MFloat_GetRand((i*0xfab5)+1) - 1.0f) * 1.0f) * 4.0f)));
			fp32 Width2 = 5.0f;// + M_Fabs(M_Cos(AnimTime*(((MFloat_GetRand((i*0xfab5)+2) - 1.0f) * 1.0f) * 4.0f)));
			fp32 Width3 = 5.0f;// + M_Fabs(M_Cos(AnimTime*(((MFloat_GetRand((i*0xfab5)+3) - 1.0f) * 1.0f) * 4.0f)));

			Mat.GetRow(3) = _WMat.GetRow(3);
			BeamSpline.AddBeamData(Mat.GetRow(3), _WMat.GetRow(0), CPixel32(255,255,255,Point0), Width0*0.1, 0.0f + UOffset, i);
			
			CVec3Dfp32 Tng = (pHistory->m_LastPos[iH] - Mat.GetRow(3)).Normalize();
			Mat.GetRow(3) = pHistory->m_LastPos[iH];
			pHistory->m_LastPos[iH].Lerp(Dst0, 0.8f, pHistory->m_LastPos[iH]);
			BeamSpline.AddBeamData(Mat.GetRow(3), _WMat.GetRow(0), CPixel32(255,255,255,Point1), Width1*0.75, 0.333f + UOffset, i);

			Tng = (pHistory->m_LastPos[++iH] - Mat.GetRow(3)).Normalize();
			Mat.GetRow(3) = pHistory->m_LastPos[iH];
			pHistory->m_LastPos[iH].Lerp(Dst1, 0.35f, pHistory->m_LastPos[iH]);
			BeamSpline.AddBeamData(Mat.GetRow(3), Tng, CPixel32(255,255,255,Point2), Width2, 0.666f + UOffset, i);

			Tng = (pHistory->m_LastPos[++iH] - Mat.GetRow(3)).Normalize();
			Mat.GetRow(3) = pHistory->m_LastPos[iH];
			pHistory->m_LastPos[iH].Lerp(Dst2, 0.15f, pHistory->m_LastPos[iH]);
			BeamSpline.AddBeamData(Mat.GetRow(3), Tng, CPixel32(255,255,255,0), Width3, 1.0f + UOffset, i);

			// Perhaps we should have some variations ?
			BeamSpline.SetSurface(i, m_iSurfaceID);
		}

		if (!pHistory->m_bLast)
			pHistory->m_bLast = true;

		BeamSpline.VBMem_Calculate(&AllocUtil);
		if (AllocUtil.Alloc(_pVBM) && BeamSpline.Finalize(&AllocUtil, _VMat))
		{
			CXR_RenderInfo RenderInfo(_pEngine);
			BeamSpline.Render(GetSurfaceContext(), _pEngine, _pVBM, &RenderInfo, _pAnimState->m_AnimTime0, 0, NULL);
			//MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
			//if (pWC)
			//	BeamSpline.Debug_Render(pWC);
		}
	}
}

