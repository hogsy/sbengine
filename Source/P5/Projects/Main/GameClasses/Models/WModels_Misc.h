#ifndef __WMODELS_MISC_H
#define __WMODELS_MISC_H

#include "../WObj_Weapons/WObj_Spells.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

class CClientData_Trail : public CXR_ModelInstance
{
public:
	enum
	{
		MAXLEN = 10,
	};

	CMat43fp32 m_History[MAXLEN];
	int m_Flags[MAXLEN];
	int m_LastGameTick;
	int m_nHistory;
	int m_iCurPos;
	CBox3Dfp32 m_BoundBox;

	CMat43fp32 m_LastIWMat;

	CClientData_Trail();
	void Clear();
	void AddPosition(int _GameTick, const CMat43fp32 &_WMat, const CMat43fp32 &_Pos, int _Flags = 0);

	virtual bool NeedRefresh(class CXR_Model* _pModel, void* _pContext0, void* _pContext1);
	virtual void OnRefresh(void* _pContext0, void* _pContext1, int _GameTick, const CMat43fp32 *_pMat = NULL, int _nMat = 0, int _Flags = 0);

	virtual TPtr<CXR_ModelInstance> Duplicate() const;
	virtual void operator=(const CXR_ModelInstance &_Instance);
};

/*class CClientData_Trail : public CReferenceCount
{
	enum
	{
		MAXLEN = 256,
	};

public:
	CVec3Dfp32 m_History[MAXLEN];
	CBox3Dfp32 m_BoundBox;
	int m_LastGameTick;
	int m_nHistory;
	CMat4Dfp32 m_LastWMat;

	CClientData_Trail()
	{
		Clear();
	}

	void Clear()
	{
		m_LastGameTick = -1;
		m_nHistory = 0;
	}

	void AddPosition(int _GameTick, const CVec3Dfp32 &_Pos)
	{
		if(m_nHistory < MAXLEN)
		{
			if(_GameTick != m_LastGameTick + 1)
				//There has been a "skip", reset content
				Clear();

			if(m_nHistory == 0)
			{
				m_BoundBox.m_Min = _Pos;
				m_BoundBox.m_Max = _Pos;
			}
			else
			{
				m_BoundBox.m_Min[0] = Min(m_BoundBox.m_Min[0], _Pos[0]);
				m_BoundBox.m_Min[1] = Min(m_BoundBox.m_Min[1], _Pos[1]);
				m_BoundBox.m_Min[2] = Min(m_BoundBox.m_Min[2], _Pos[2]);
				m_BoundBox.m_Max[0] = Max(m_BoundBox.m_Max[0], _Pos[0]);
				m_BoundBox.m_Max[1] = Max(m_BoundBox.m_Max[1], _Pos[1]);
				m_BoundBox.m_Max[2] = Max(m_BoundBox.m_Max[2], _Pos[2]);
			}

			m_History[m_nHistory++] = _Pos;
			m_LastGameTick = _GameTick;
		}
	}

	static void OnRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
	{
		if(_pWClient->GetClientMode() != WCLIENT_MODE_MIRROR)
		{
			if(!_pObj->m_lspClientObj[0])
			{
				TPtr<CClientData_Trail> spTrail = DNew(CClientData_Trail) CClientData_Trail;
//				spTrail->AddPosition(_pWClient->GetGameTick(), _pObj->GetPosition());
				_pObj->m_lspClientObj[0] = spTrail;
			}

			CClientData_Trail *pTrail = (CClientData_Trail *)(CReferenceCount *)_pObj->m_lspClientObj[0];
			if(!pTrail)
				return;

			CMat4Dfp32 Mat = _pObj->GetPositionMatrix();

			//Fix: We don't know this is a projectile
			if(_pObj->m_ClientFlags & CWObject_Projectile::CLIENTFLAGS_RENDERHANSOFFSET)
			{
				CVec3Dfp32 RelPos = CWObject_Projectile::GetHandsDistance(_pObj, _pObj->AnimTime(_pWClient) * SERVER_TIMEPERFRAME + _pWClient->GetInterpolateTime());
				RelPos.MultiplyMatrix3x3(Mat);
				CVec3Dfp32::GetMatrixRow(Mat, 3) += RelPos;
			}

			pTrail->AddPosition(_pWClient->GetGameTick(), CVec3Dfp32::GetMatrixRow(Mat, 3));
		}
	}

	static void InitRender(CWObject_Client* _pObj, CWorld_Client* _pWClient)
	{
		CClientData_Trail *pTrail = (CClientData_Trail *)(CReferenceCount *)_pObj->m_lspClientObj[0];
		if(!pTrail)
			return;

		fp32 TickFrac = _pWClient->GetRenderTickFrac();
		fp32 IPTime = _pWClient->GetInterpolateTime();
		_pWClient->Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), pTrail->m_LastWMat, IPTime);
	}
};

class CClientData_DoubleTrail : public CReferenceCount
{
public:
	enum
	{
		MAXLEN = 10,
	};

	CVec3Dfp32 m_History0[MAXLEN];
	CVec3Dfp32 m_History1[MAXLEN];
	fp32 m_Speed0[MAXLEN];
	fp32 m_Speed1[MAXLEN];
	fp32 m_CurSpeed0;
	fp32 m_CurSpeed1;
	CBox3Dfp32 m_BoundBox;
	int m_LastGameTick;
	int m_nHistory;
	int m_iCurPos;

	CMat4Dfp32 m_LastIWMat;

	void operator=(const CClientData_DoubleTrail& _CD)
	{
		memcpy(this, &_CD, sizeof(_CD));
	}

	CClientData_DoubleTrail()
	{
		Clear();
	}

	void Clear()
	{
		m_LastGameTick = -1;
		m_nHistory = 0;
		m_iCurPos = 0;
		m_CurSpeed0 = 0;
		m_CurSpeed1 = 0;
	}

	void AddPosition(int _GameTick, const CMat4Dfp32 &_WMat, const CVec3Dfp32 &_Pos0, const CVec3Dfp32 &_Pos1)
	{
//		if(m_nHistory < MAXLEN)
		{
			if(_GameTick > m_LastGameTick + 3)
				Clear();

			m_BoundBox.m_Min = _Pos0 - CVec3Dfp32(64);
			m_BoundBox.m_Max = _Pos0 + CVec3Dfp32(64);

			int iLastIndex = -1;
			int iIndex;
			if(m_nHistory == MAXLEN)
			{
				iLastIndex = m_iCurPos - 1;
				if(iLastIndex < 0)
					iLastIndex += MAXLEN;
				iIndex = m_iCurPos++;
				if(m_iCurPos == MAXLEN)
					m_iCurPos = 0;
			}
			else
			{
				if(m_nHistory > 0)
					iLastIndex = m_nHistory - 1;
				iIndex = m_nHistory++;
			}

			m_History0[iIndex] = _Pos0;
			m_History1[iIndex] = _Pos1;

			m_Speed0[iIndex] = m_CurSpeed0;
			m_Speed1[iIndex] = m_CurSpeed1;
			
			CMat4Dfp32 IWMat;
			_WMat.InverseOrthogonal(IWMat);
			if(iLastIndex != -1)
			{
				m_CurSpeed0 = (_Pos0 * IWMat - m_History0[iLastIndex] * m_LastIWMat).Length();
				m_CurSpeed1 = (_Pos1 * IWMat - m_History1[iLastIndex] * m_LastIWMat).Length();
			}
			else
			{
				m_CurSpeed0 = 0;
				m_CurSpeed1 = 0;
			}
			
			m_LastIWMat = IWMat;
			m_LastGameTick = _GameTick;
		}
	}
};*/



// -------------------------------------------------------------------
//  CXR_Model_Smoke
// -------------------------------------------------------------------
class CXR_Model_Smoke : public CXR_Model_Custom
{
	MRTC_DECLARE;
	
	int m_iSmokeTexture;

	virtual void OnCreate(const char *);
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
};


// -------------------------------------------------------------------
//  CXR_Model_Flames
// -------------------------------------------------------------------

class CXR_Model_Flames : public CXR_Model_Custom
{
	MRTC_DECLARE;

	int m_iTexture;

public:
	uint32 m_Color[3];
	uint32 m_TransColor[3];
	fp32 m_Width;
	fp32 m_Height;
	fp32 m_Time;
	fp32 m_Spline[4];
	fp32 m_WaveTime;

	virtual void OnCreate(const char *);
	void CreateFlame(CVec3Dfp32 _Pos, const CVec3Dfp32 &_Dir, fp32 _Force, int &_iRand, CXR_BeamStrip *pStrip, int &_iBeam);
	bool RenderFlames(CXR_VertexBuffer *_pVB, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat, CXR_BeamStrip *pStrip, int _iBeam);
};


// -------------------------------------------------------------------
//  CXR_Model_Fire
// -------------------------------------------------------------------
class CXR_Model_Fire : public CXR_Model_Flames
{
	MRTC_DECLARE;

protected:
	virtual void OnCreate(const char *_pParams);
	virtual void OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat);
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
};


// -------------------------------------------------------------------
//  CXR_Model_TraceLight2
// -------------------------------------------------------------------
class CXR_Model_TraceLight2 : public CXR_Model_Custom
{
	MRTC_DECLARE;

	int m_iTexture;

	virtual void OnCreate(const char *);
	virtual void OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat);
	void RenderRings(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat);
};


// -------------------------------------------------------------------
//  CXR_Model_BoltTrail
// -------------------------------------------------------------------
class CXR_Model_BoltTrail : public CXR_Model_Custom
{
	MRTC_DECLARE;

protected:
//	int m_iTexture; // Removed by Mondelore.
	int m_iSurface; // Added by Mondelore.

	virtual void OnCreate(const char *);
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);
	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat);
};


// -------------------------------------------------------------------
//  CXR_Model_Trail
// -------------------------------------------------------------------
class CXR_Model_Trail : public CXR_Model_Custom
{
	MRTC_DECLARE;

	int m_iTexture;

	virtual void OnCreate(const char *);
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	CBox3Dfp32 GetAppliedBound(const CBox3Dfp32 &_Bound, const CMat43fp32 &_Mat);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);
	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat);
};


// -------------------------------------------------------------------
//  CXR_Model_DoubleTrail
// -------------------------------------------------------------------
class CXR_Model_DoubleTrail : public CXR_Model_Custom
{
	MRTC_DECLARE;

	int m_iSurface;
	int m_iAttach0;
	int m_iAttach1;

	virtual void OnCreate(const char *);

	void VSpline(CVec3Dfp32 *pMoveA0, CVec3Dfp32 *pMoveA1, CVec3Dfp32 *pMoveA2,
							 CVec3Dfp32 *pMoveB0, CVec3Dfp32 *pMoveB1, CVec3Dfp32 *pMoveB2, 
							 CVec3Dfp32* _pDest, fp32 _tFrac, fp32 _tA0, fp32 _tA1, fp32 _tB0, fp32 _tB1, int _nV);

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat);

	virtual TPtr<CXR_ModelInstance> CreateModelInstance();
};


// -------------------------------------------------------------------
//  CXR_Model_MagicTrail
// -------------------------------------------------------------------
class CXR_Model_MagicTrail : public CXR_Model_Custom
{
	MRTC_DECLARE;

	int m_iTexture;

	virtual void OnCreate(const char *);
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	CBox3Dfp32 GetAppliedBound(const CBox3Dfp32 &_Bound, const CMat43fp32 &_Mat);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);
	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat);
};



#endif
