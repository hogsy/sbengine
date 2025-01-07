
#include "../../../../Shared/MOS/XR/XRClass.h"

#ifndef _INC_WPARTCLS
#define _INC_WPARTCLS

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_ParticleSystem
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define CXR_PARTICLES_NUMRANDOM 512
#define CXR_PARTICLES_NUMAND 511

class CXR_Model_ParticleSystem : public CXR_Model
{
protected:
	static bool ms_bRandomInitialized;
	static fp32 ms_lRandom[CXR_PARTICLES_NUMRANDOM];
	static int ms_lRandomInt[CXR_PARTICLES_NUMRANDOM];

	spCTextureContainer m_spTC;
	int m_TextureID;
	int m_RenderPass;		// Defaulting to 1
	fp32 m_BoundR;			// Defaulting to 16
	CPixel32 m_CurrentFog;
	CXR_Engine* m_pEngine;
	CXR_RenderInfo m_RenderInfo;

public:
	CXR_Model_ParticleSystem();

	// Bounding volumes in model-space,
	virtual int GetRenderPass(const CXR_AnimState* _pAnimState = NULL);
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual fp32 GetBound_Animated_Sphere(const CXR_AnimState* _pAnimState);
	virtual void GetBound_Animated_Box(const CXR_AnimState* _pAnimState, CBox3Dfp32& _Box) { GetBound_Box(_Box); };

	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags) pure;
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_GenericParticles
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_Generic : public CXR_Model_ParticleSystem
{
protected:
	class CGenericAttributes
	{
	public:
		int m_RandomSeed;
		int m_nParticles;
		int m_nClusters;
		fp32 m_CluserSizeBase;
		fp32 m_CluserSizeRandomAmp;
		fp32 m_CluserSizeIncrement;

		CVec3Dfp32 m_PosRandomAmp;
		CVec3Dfp32 m_PosBase;
		CVec3Dfp32 m_SpeedRandomAmp;
		CVec3Dfp32 m_SpeedBase;
		CVec3Dfp32 m_AccelRandomAmp;
		CVec3Dfp32 m_AccelBase;
		
		fp32 m_SpawnDuration;	// Particles are created linearly spaced during this time.
		fp32 m_DestroyTime;		// Particles start beeing destroyed at this time.
		fp32 m_DestroyDuration;	// Particles are beeing destroyed linearly spaced during this time.

		CPixel32 m_ColorBegin;
		CPixel32 m_ColorMid;
		CPixel32 m_ColorEnd;
		CPixel32 m_ColorRandomAmp;
		CPixel32 m_ClusterIncrement;

		CGenericAttributes()
		{
		}
	};

	CGenericAttributes m_ParticleAttr;

public:	
//	virtual void GetBound_Animated_Sphere(const CXR_AnimState* _pAnimState, CVec3Dfp32& _v0, fp32& _Radius);

	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};

#ifndef M_DISABLE_TODELETE

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_TorchFire
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_TorchFire : public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

public:
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_FireWall
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef M_DISABLE_TODELETE
class CXR_Model_Particles_FireWall : public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

public:
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

#ifndef M_DISABLE_TODELETE

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_SmallFire
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_SmallFire : public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

public:
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

#ifndef M_DISABLE_TODELETE

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_InfernoFlare
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_InfernoFlare : public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

public:
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_Sparkles
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef M_DISABLE_TODELETE
class CXR_Model_Particles_Sparkles: public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

public:
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

#ifndef M_DISABLE_TODELETE
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_Explosion
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_Explosion : public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

public:
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

#ifndef M_DISABLE_TODELETE

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_Teleporter
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_Teleporter : public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

public:
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

#ifndef M_DISABLE_TODELETE
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_Explosion2
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_Explosion2 : public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

protected:
	fp32 m_Radius1;
	fp32 m_Radius2;
	fp32 m_SizeScale;
	fp32 m_TotTime;

public:
	CXR_Model_Particles_Explosion2();
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

#ifndef M_DISABLE_TODELETE
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_Explosion2b
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_Explosion2b : public CXR_Model_Particles_Explosion2
{
	MRTC_DECLARE;

public:
	CXR_Model_Particles_Explosion2b();
};
#endif

#ifndef M_DISABLE_TODELETE
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Particles_Blood
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Particles_Blood : public CXR_Model_ParticleSystem
{
	MRTC_DECLARE;

public:
	virtual void RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags);
};
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_WaterTile
|__________________________________________________________________________________________________
\*************************************************************************************************/

#define WATER_DIFFANIMLEN 16
#define WATER_SPECANIMLEN 16

class CXR_Model_WaterTile : public CXR_PhysicsModel
{
	MRTC_DECLARE;

protected:
	CXR_WorldLightState m_WLS;
	TPtr<CXW_SurfaceKeyFrame> m_spTmpSurfKey;

	CStr m_Surface;
	int m_SurfaceID;
	int m_MediumFlags;

	CVec2Dfp32 m_Scale;
	CVec2Dfp32 m_ScaleInv;
	CVec2Dfp32 m_TexScale;
	CVec2Dfp32 m_PeriodScale;
	CVec2Dfp32 m_MaxTess;
	CVec2Dfp32 m_MinTess;

	//int m_DiffTxt[WATER_DIFFANIMLEN];
	//int m_SpecTxt[WATER_SPECANIMLEN];

	fp32 m_SPeriod0;
	fp32 m_SPeriod1;
	fp32 m_SPeriod2;
	fp32 m_TPeriod0;
	fp32 m_TPeriod1;
	fp32 m_TPeriod2;
	fp32 m_Amp0;
	fp32 m_Amp1;
	fp32 m_Amp2;
	fp32 m_Frequency0;
	fp32 m_Frequency1;
	fp32 m_Frequency2;

public:
	CXR_Model_WaterTile();
	virtual int GetRenderPass(const CXR_AnimState* _pAnimState = NULL) { return 1; };
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);

	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);

	// -------------------------------
	// Physics overrides.
protected:
//	CMat4Dfp32 m_PhysWMat;
//	CMat4Dfp32 m_PhysWMatInv;
//	CWireContainer* m_pPhysWC;
//	const CXR_AnimState* m_pPhysAnimState;

public:
	virtual void Create(const char* _pParam);
	virtual void OnPostCreate();
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);

	// Bounding volumes are in world-space.
	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius);
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox);

	// Collision services. All indata is in world coordinates.
//	virtual void Phys_Init(const CMat4Dfp32& _WMat, const CXR_AnimState* _pAnimState = NULL, CRenderContext* _pRC = NULL);
	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext);
	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0);
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium);
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags );
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_WaterTile2
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_WaterTile2 : public CXR_Model_WaterTile
{
	MRTC_DECLARE;

public:
	virtual void Create(const char* _pParam);
};


#endif // _INC_WPARTCLS
