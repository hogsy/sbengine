/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WModel_Drain.h

Author:			Patrik Willbo

Copyright:		2006 Starbreeze Studios AB

Contents:		CXR_Model_Drain
				CXR_Model_Drain_Instance

Comments:

History:
\*____________________________________________________________________________________________*/
#ifndef __WModel_Drain_h__
#define __WModel_Drain_h__


#include "WModel_EffectSystem.h"
#include "WModel_BloodEffect.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Drain_Instance
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Drain_Instance : public CXR_ModelInstance
{
public:
	CXR_Model_Drain_Instance();
	~CXR_Model_Drain_Instance();

	virtual void Create(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);
	virtual void OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32* _pMat = NULL, int _nMat = 0, int _Flags = 0);
	virtual bool NeedRefresh(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);

	virtual TPtr<CXR_ModelInstance> Duplicate() const;
	virtual void operator = (const CXR_ModelInstance& _Instance);
	virtual void operator = (const CXR_Model_Drain_Instance& _Instance);

	void SetLastEntry(uint _iEntry, const CVec3Dfp32& _EntryPos, const CVec3Dfp32& _EntryTng);
	CVec3Dfp32 GetLerpPos(uint _iEntry, const fp32 _t, const CVec3Dfp32& _Pos);
	CVec3Dfp32 GetLerpTng(uint _iEntry, const fp32 _t, const CVec3Dfp32& _Tng);

	// Temporary, just allow a couple of strips, fix this later!!
	CVec3Dfp32	m_LastPos[18];
	CVec3Dfp32	m_LastTng[18];
	bool		m_bLast;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Drain
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Drain : public CXR_Model_Custom
{
	MRTC_DECLARE;

	uint16 m_iSurfaceID;

	CVec3Dfp32	GetOffset(fp32 _Time, uint32 _Seed, const CVec3Dfp32& _Speed, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max);
public:
	CXR_Model_Drain();

	virtual TPtr<CXR_ModelInstance> CreateModelInstance();
	virtual void OnEvalKey(const CRegistry *_pReg);

	virtual void OnCreate(const char* _pParam);
	virtual void OnRender(CXR_Engine *_pEngine, CRenderContext *_pRender, CXR_VBManager *_pVBM, CXR_ViewClipInterface *_pViewClip,
						  spCXR_WorldLightState _spWLS, const CXR_AnimState *_pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, int _Flags);
};


#endif

