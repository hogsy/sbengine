
#ifndef __INC_XWTRACE
#define __INC_XWTRACE

#include "MCC.h"
#include "../../MSystem/Raster/MImageCubemapSampler.h"

// -------------------------------------------------------------------
// No idea why these enums exist, they are the same as XR_LIGHTYPE
#define XWC_LIGHTTYPE_POINT				0
#define XWC_LIGHTTYPE_FAKESKYRADIOSITY	1
#define XWC_LIGHTTYPE_PARALLELL			2
#define XWC_LIGHTTYPE_SPOT				5

#define XWC_SKYLIGHT_PHISTEPS 12
#define XWC_SKYLIGHT_THETASTEPS 3

class CXWC_Tracer_Light
{
public:
	int m_Type;
	int m_Flags;
	fp32 m_MaxRange;
	fp32 m_SpotWidth;
	fp32 m_SpotHeight;
	fp32 m_TraceLen;
	int32 m_LightID;
	CMat4Dfp32 m_Pos;
//	CVec3Dfp32 m_Pos;
	CVec3Dfp32 m_Intensity;

	CMat4Dfp32 m_LocalPos4;
	CMat4Dfp64 m_LocalPos;
//	CVec3Dfp64 m_LocalPos;

	CStr      m_ProjMapSurface;
	CVec3Dfp64 m_ProjMapOrigin;
	TPtr<class CImageCubemapSampler> m_spProjMapSampler;

	CVec3Dfp64& GetLocalPos() { return CVec3Dfp64::GetMatrixRow(m_LocalPos, 3); };
	CVec3Dfp32& GetPos() { return CVec3Dfp32::GetMatrixRow(m_Pos, 3); };
	const CVec3Dfp64& GetLocalPos() const { return CVec3Dfp64::GetMatrixRow(m_LocalPos, 3); };
	const CVec3Dfp32& GetPos() const { return CVec3Dfp32::GetMatrixRow(m_Pos, 3); };

	void CopyToLocal(const CMat4Dfp32& _Mat)
	{
		m_LocalPos4 = _Mat;

		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				m_LocalPos.k[i][j] = _Mat.k[i][j];
	}

	CXWC_Tracer_Light()
	{
		m_Type = XWC_LIGHTTYPE_POINT;
		m_Flags = 0;
		m_MaxRange = 512.0;
		m_TraceLen = 1024;
		m_LightID = 0;
		m_Pos.Unit();
		m_Intensity = 1.0;
		m_ProjMapOrigin = 0;
		m_SpotWidth = 1.0f;
		m_SpotHeight = 1.0f;
	}

	void SetType(int _Type);

	void Parse(class CRegistry *_pReg);

	bool CanIlluminate(const CBox3Dfp32& _Box) const;
	bool CanCastShadow(const CBox3Dfp32& _Box, const CBox3Dfp32 _ShadowObj) const;
};

// -------------------------------------------------------------------
class CXWC_Tracer;

class CXWC_TracerObject : public CReferenceCount
{
public:
	fp32 m_BoundRadius;
	int m_TraceLineMedium;
	CBox3Dfp32 m_Bound;
	CBox3Dfp32 m_LocalBound;

//	virtual bool InitTraceBound_Polyhedron(const CVec3Dfp64* _pV, int _nVertices, const CPlane3Dfp64* _pP, int _nPlanes) pure;
	virtual void InitTraceBound(CXWC_Tracer* _pTracer, const CBox3Dfp32& _Box);
	virtual void ReleaseTraceBound() pure;
	virtual int TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, CVec3Dfp32& _Color) pure;
	virtual int TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, int _MediumFlags, CCollisionInfo* _pCInfo = NULL) pure;
	virtual bool PotentialIntersection(const CBox3Dfp32& _Box) { return false; };

	virtual int GetMedium(const CBox3Dfp64& _Box, CXR_MediumDesc& _Medium) const { return 0; }
};

typedef TPtr<CXWC_TracerObject> spCXWC_TracerObject;

// -------------------------------------------------------------------
class CXWC_Tracer_Bound : public CReferenceCount
{
public:
	TArray<int> m_liActiveObjects;
	TArray<CXWC_Tracer_Light> m_lActiveLights;
	int m_nActiveObjects;
	int m_nActiveLights;
	CBox3Dfp32 m_Bound;

	CXWC_Tracer_Bound(int _nObj, int _nLight)
	{
		m_liActiveObjects.SetLen(_nObj);
		m_lActiveLights.SetLen(_nLight);
		m_nActiveObjects = 0;
		m_nActiveLights = 0;
		m_Bound.m_Min = -_FP32_MAX;
		m_Bound.m_Max = _FP32_MAX;
	}
};

typedef TPtr<CXWC_Tracer_Bound> spCXWC_Tracer_Bound;

// -------------------------------------------------------------------
class CXWC_Tracer : public CReferenceCount /* : public CXWC_TracerObject*/
{
	TArray<spCXWC_TracerObject> m_lspObjects;
	TArray<CMat4Dfp64> m_lObjPos;
	TArray<CMat4Dfp64> m_lLocalPos;
	TArray<CXWC_Tracer_Light> m_lLights;

	TArray<spCXWC_Tracer_Bound> m_lBoundStack;
	int m_iStack;
	bool m_bSkipTrace;

public:
	int m_TraceLineMedium;


	TPtr<CXWC_Tracer> Duplicate();

	CXWC_Tracer();
	int GetNumObjects() { return m_lspObjects.Len(); }
	virtual void AddLight(const CXWC_Tracer_Light& _Light);
	virtual void AddObject(spCXWC_TracerObject _spObject, const CMat4Dfp64& _Pos);
	virtual void SetLocalSystem(CMat4Dfp64 _Mat);
//	virtual bool InitTraceBound_Polyhedron(const CVec3Dfp64* _pV, int _nVertices, const CPlane3Dfp64* _pP, int _nPlanes);
	virtual void InitStack();
/*	virtual bool PushTraceBound_Sphere(const CVec3Dfp64 _Pos, fp64 _Radius);
	virtual bool PushTraceBound_Plane(const CPlane3Dfp64& _pP);
	virtual bool PushTraceBound_Planes(const CPlane3Dfp64 *_pP, int _nPlanes);
	virtual bool PushTraceBound_Polyhedron(const CVec3Dfp64* _pV, int _nVertices, const CPlane3Dfp64* _pP, int _nPlanes);*/
	virtual bool PushTraceBound_Box(const CBox3Dfp32& _Box);
	virtual void PopTraceBound();

	virtual bool BoxAffectsRendering(const CBox3Dfp32& _Box);
	virtual bool PotentialIntersection(const CBox3Dfp32& _Box);
	virtual void InitTraceBound();
	virtual void ReleaseTraceBound();

	virtual int TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, CVec3Dfp32& _Color);
	virtual int TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, int _MediumFlags, CCollisionInfo* _pCInfo = NULL);
//	virtual CXW_Surface* TraceLine(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, CVec3Dfp32& _Color);

	virtual CXWC_Tracer_Light* GetLightPtr() { return m_lBoundStack[m_iStack]->m_lActiveLights.GetBasePtr(); };
	virtual int GetLightCount() { return m_lBoundStack[m_iStack]->m_nActiveLights; };
	virtual int GetObjectCount() { return m_lBoundStack[m_iStack]->m_nActiveObjects; };

	virtual void SetSkipTrace(bool _bSkip = true) { m_bSkipTrace = _bSkip; }

	virtual int GetMedium(const CBox3Dfp64& _Box, CXR_MediumDesc& _Medium) const;
};

#endif // __INC_XWTRACE
