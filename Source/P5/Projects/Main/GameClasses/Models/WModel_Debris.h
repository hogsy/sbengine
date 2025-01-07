#ifndef __WMODEL_DEBRIS_H
#define __WMODEL_DEBRIS_H

#define MAXSAMPLES 80

#include "../../../../Shared/MOS/XR/XRCustomModel.h"

class CWorld_Client;

// -------------------------------------------------------------------
//  CDebrisTrails
// -------------------------------------------------------------------
class CDebrisTrails : public CXR_ModelInstance
{
	struct CTrail
	{
		CVec3Dfp32 m_SamplePos[MAXSAMPLES];
		CVec3Dfp32 m_Angles[MAXSAMPLES];
		int m_NumSamples;
	};

	TArray<CTrail> m_Trails;
	fp32 m_SampleRate;

public:
	CDebrisTrails() {}
	void Create(uint32 _iRand, int _NumDebris, fp32 _Duration, const CVec3Dfp32 &_Dir, fp32 _Speed, fp32 _Scatter, fp32 _SamplePeriod, int _MaxCollisions, const CMat4Dfp32& _WMat, CWorld_Client *_pWClient);

	int GetNumSamples(int _iTrail);
	int GetIndexFromTime(int _iTrail, fp32 _Time, fp32 &_Frac);
	CVec3Dfp32 GetTrailPos(int _iTrail, fp32 _Time, CVec3Dfp32 *_pAngles = NULL, fp32 *_pSpeedFactor = NULL);
	int GetNumTrails() { return m_Trails.Len(); }

	virtual TPtr<CXR_ModelInstance> Duplicate() const;
	virtual void operator= (const CXR_ModelInstance &_Instance);
};

typedef TPtr<CDebrisTrails> spCDebrisTrails;


// -------------------------------------------------------------------
//  CXR_Model_Debris
// -------------------------------------------------------------------
class CXR_Model_Debris : public CXR_Model_Custom
{
	MRTC_DECLARE;

protected:
	int m_NumDebris;

	fp32 m_Scatter;
	fp32 m_Duration;
	fp32 m_Speed;
	fp32 m_SamplePeriod;
	bool m_bUseDirection;
	int m_MaxCollisions;

public:
	CXR_Model_Debris();
	spCDebrisTrails Compile(int _iRand, const CMat4Dfp32& _WMat, CWorld_Client *_pWClient);
};


#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
//  CXR_Model_DebrisTest
// -------------------------------------------------------------------
class CXR_Model_DebrisTest : public CXR_Model_Debris
{
	MRTC_DECLARE;

public:
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
};
#endif



#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CXR_Model_DebrisTest
// -------------------------------------------------------------------
class CXR_Model_RocketDebris : public CXR_Model_Debris
{
	MRTC_DECLARE;

public:
	int m_iTexture;

	TPtr<CXW_SurfaceKeyFrame> m_spTmpSurfKey;
	int m_iDefSurface;
	int m_iDefTrailSurface;

	void Create(const char* _pParam);

	virtual void OnCreate(const char *_pParams);

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
};
#endif

#endif
