#include "PCH.h"

#include "WModel_Primitives.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WClass.h"

CXR_Model_PhysicsPrim::CXR_Model_PhysicsPrim()
{
	SetThreadSafe(true);
}

void CXR_Model_PhysicsPrim::RenderPrimitive(CXR_Model_Custom_RenderParams* _pRenderParams, const CWO_PhysicsPrim &_Prim, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	CMat4Dfp32 L2V;
	CMat4Dfp32 WMat;
	if(!(_pAnimState->m_Anim0 & OBJECT_PHYSFLAGS_ROTATION))
	{
		WMat.Unit();
		CVec3Dfp32::GetRow(_WMat, 3).SetRow(WMat, 3);
	}
	else
		WMat = _WMat;

	if(_pAnimState->m_Anim0 & OBJECT_PHYSFLAGS_OFFSET)
	{
		CVec3Dfp32 Pos;
		_Prim.GetOffset().MultiplyMatrix(WMat, Pos);
		Pos.SetRow(WMat, 3);
	}

	WMat.Multiply(_VMat, L2V);

	if(_Prim.m_PrimType == OBJECT_PRIMTYPE_SPHERE)
	{
		CXR_VertexBuffer *pVB = CXR_Util::Create_Sphere(_pRenderParams->m_pVBM, L2V, _Prim.GetRadius(), 0xffffc020);
		if(pVB)
			_pRenderParams->m_pVBM->AddVB(pVB);
	}
	else if(_Prim.m_PrimType == OBJECT_PRIMTYPE_BOX)
	{
		CBox3Dfp32 Box;
		Box.m_Max = _Prim.GetDim();
		Box.m_Min = -Box.m_Max;

		CXR_VertexBuffer *pVB = CXR_Util::Create_Box(_pRenderParams->m_pVBM, L2V, Box, 0xffffc020);
		if(pVB)
			_pRenderParams->m_pVBM->AddVB(pVB);
	}
	else if(_Prim.m_PrimType == OBJECT_PRIMTYPE_POINT)
	{
		CXR_VertexBuffer *pVB = CXR_Util::Create_Star(_pRenderParams->m_pVBM, L2V, 8, 0xffffc020);
		if(pVB)
			_pRenderParams->m_pVBM->AddVB(pVB);
	}
}

void CXR_Model_PhysicsPrim::OnCreate(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_PhysicsPrim_OnCreate, MAUTOSTRIP_VOID);
	m_BoundRadius = 64;
}

void CXR_Model_PhysicsPrim::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_PhysicsPrim_Render, MAUTOSTRIP_VOID);

	CWObject_CoreData* pObj = (CWObject_CoreData*)_pAnimState->m_pContext;
	if (!pObj)
	{
		if(_pAnimState->m_Data[0] != -1)
			RenderPrimitive(_pRenderParams, *(CWO_PhysicsPrim *)_pAnimState->m_Data[0], _pAnimState, _WMat, _VMat);
		return;
	}
	
	{
		CBox3Dfp32 Box = *pObj->GetAbsBoundBox();
		CXR_VertexBuffer *pVB = CXR_Util::Create_Box(_pRenderParams->m_pVBM, _VMat, Box, 0xff208020);
		if(pVB)
			_pRenderParams->m_pVBM->AddVB(pVB);
	}
	
	{
		CBox3Dfp32 Box;
		pObj->GetAbsVisBoundBox(Box);
		CXR_VertexBuffer *pVB = CXR_Util::Create_Box(_pRenderParams->m_pVBM, _VMat, Box, 0xff002080);
		if(pVB)
			_pRenderParams->m_pVBM->AddVB(pVB);
	}
	
	
	const CWO_PhysicsState *pPhys = &pObj->GetPhysState();
	for(int iPrim = 0; iPrim < pPhys->m_nPrim; iPrim++)
		RenderPrimitive(_pRenderParams, pPhys->m_Prim[iPrim], _pAnimState, _WMat, _VMat);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_PhysicsPrim, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_Line
// -------------------------------------------------------------------
CXR_Model_Line::CXR_Model_Line()
{
	SetThreadSafe(true);
}

fp32 CXR_Model_Line::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Line_GetBound_Sphere, 0.0f);
	if(_pAnimState)
		return _pAnimState->m_AnimTime0.GetTime() * 2; // CMTIMEFIX
	else
		return 64;
}

void CXR_Model_Line::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Line_GetBound_Box, MAUTOSTRIP_VOID);
	if(_pAnimState)
	{
		_Box.m_Min = -_pAnimState->m_AnimTime0.GetTime() * 2; // CMTIMEFIX
		_Box.m_Max = _pAnimState->m_AnimTime0.GetTime() * 2; // CMTIMEFIX
	}
	else
	{
		_Box.m_Min = -64;
		_Box.m_Max = 64;
	}
}

// Render
void CXR_Model_Line::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Line_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_Line::OnRender);

	if(!_pRender)
		return;
	
	_pRender->Attrib_Push();
	_pRender->Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
	_pRender->Attrib_TextureID(0, 0);
	_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	
	CMat4Dfp32 Mat;
	_WMat.Multiply(_VMat, Mat);
	_pRender->Matrix_PushMultiply(Mat);
	
	CVec3Dfp32 Pos0(0);
	CVec3Dfp32 Pos1(_pAnimState->m_AnimTime0.GetTime(), 0, 0); // CMTIMEFIX
	_pRender->Render_Wire(Pos0, Pos1, _pAnimState->m_Data[0]);
	
	_pRender->Matrix_Pop();
	_pRender->Attrib_Pop();
}


MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Line, CXR_Model);
