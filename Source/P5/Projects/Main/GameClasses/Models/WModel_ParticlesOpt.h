#ifndef __WModel_ParticlesOpt_h__
#define __WModel_ParticlesOpt_h__

#include "CSurfaceKey.h"
#include "CModelHistory2.h"
#include "CPropertyControl.h"
#include "ModelsMisc.h"

void SIMD_Add_VVc(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32& _Src2, CVec4Dfp32* _pDst );
void SIMD_Set_V(int _nV, const CVec4Dfp32* _pSrc1, CVec4Dfp32* _pDst );
void SIMD_Set_Vc(int _nV, const CVec4Dfp32& _Src1, CVec4Dfp32* _pDst );
void SIMD_Mul_VS(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, CVec4Dfp32* _pDst);
void SIMD_Mul_VSc( int _nV, const CVec4Dfp32* _pSrc1, const fp32& _Src2, CVec4Dfp32* _pDst );
void SIMD_Combine_SSS(int _nV, const fp32* _pSrc1, const fp32* _pSrc2, const fp32* _pSrc3, fp32* _pDst);
void SIMD_Combine_VVS(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32* _pSrc2, const fp32* _pSrc3, CVec4Dfp32* _pDst);
void SIMD_Combine_VcVcVr(int _nV, const CVec4Dfp32& _Src1, const CVec4Dfp32& _Src2, uint32* _pRandSeed, CVec4Dfp32* _pDst);
void SIMD_Combine_VcVcSr(int _nV, const CVec4Dfp32& _Src1, const CVec4Dfp32& _Src2, uint32* _pRandSeed, CVec4Dfp32* _pDst);
void SIMD_Combine_ScScSr(int _nV, fp32 _Src1, fp32 _Src2, uint32* _pRandSeed, fp32* _pDst);
void SIMD_Mul_VMc(int _nV, const CVec4Dfp32* _pSrc1, const CMat4Dfp32& _Mat, CVec4Dfp32* _pDst);
void SIMD_Combine_VVMc(int _nV, const CVec4Dfp32* _pSrc1, const CVec4Dfp32* _pSrc2, const CMat4Dfp32& _Mat, CVec4Dfp32* _pDst);

void SIMD_ParticleQuad_VSVcVc_V3(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, const CVec4Dfp32& _Src3, const CVec4Dfp32& _Src4, CVec3Dfp32* _pDst);
void SIMD_ParticleTriangle_VSVcVc_V3(int _nV, const CVec4Dfp32* _pSrc1, const fp32* _pSrc2, const CVec4Dfp32& _Src3, const CVec4Dfp32& _Src4, CVec3Dfp32* _pDst);

// ----------------------------------------------------------------------

enum
{
	PRIMITIVE_POINT				= 0x00,
	PRIMITIVE_CUBE				= 0x01,
	PRIMITIVE_SPHERE			= 0x02,
	PRIMITIVE_TUBE				= 0x03,
	PRIMITIVE_CYLINDER			= 0x04,
	PRIMITIVE_PYRAMID			= 0x05,
	PRIMITIVE_TETRAID			= 0x06,
};

enum
{
	PARTICLE_FLAGS_FADESTILL				= 0x00000001, // Fade when model reaches a still position.
	PARTICLE_FLAGS_SLOWDOWN					= 0x00000002, // Slow down particle velocity to zero at full duration.
	PARTICLE_FLAGS_NOHISTORY				= 0x00000004, // Use only the latest history matrix, no older ones.
	PARTICLE_FLAGS_QUADS					= 0x00000008, // Render each particle using 2 triangles, as a quad, instead of one clamped triangle.
	PARTICLE_FLAGS_PRIMCENTER				= 0x00000010, // Force particle velocity center to primitive/model center.
	PARTICLE_FLAGS_WORLDROT					= 0x00000020, // Force particle velocity center to primitive/model center.
	PARTICLE_FLAGS_HISTORYFLAGS				= 0x00000040, // Emit particles only when history flags are != 0.
	PARTICLE_FLAGS_SPLINEHISTORY			= 0x00000080, // Use spline interpolation when calculating intermediate history matrices.
	PARTICLE_FLAGS_ALIGN					= 0x00000100, // Align particle rotation with its current movement vector.
	PARTICLE_FLAGS_CONTINUOUSTIME			= 0x00000200, // Use client gametick time istead of enginepath controlled time.
	PARTICLE_FLAGS_FORCEEARLY				= 0x00000400, // Spawn particles as early as the latest tick + iptime, ignore interpolation misses.
	PARTICLE_FLAGS_IHISTORYFLAGS			= 0x00000800, // Emit particles only when history flags are == 0.
	PARTICLE_FLAGS_NOLOD					= 0x00001000, // Turn off loding.
	PARTICLE_FLAGS_LOCALACCELERATION		= 0x00002000, // Apply acceleration in local space instead of in worldspace.
	PARTICLE_FLAGS_STOPMOTION				= 0x00004000, // A better version of slowdown. Particles change velocity and stop after a certain time.
	PARTICLE_FLAGS_NOCLAMPCOLORS			= 0x00008000, // Don't clamp particle color components to 0-255.
	PARTICLE_FLAGS_SHOWOVERFLOW				= 0x00010000, // Use default texture when exceeding maxparticles limit.
	PARTICLE_FLAGS_NOCHANNELEDCOLORNOISE	= 0x00020000, // Don't randomize color channels individually.
	PARTICLE_FLAGS_EMITTERTIME_SIZE			= 0x00040000, // Use emitter time fraction for size interpolations.
	PARTICLE_FLAGS_ALIGN_SAFEQUAD			= 0x00080000, // Quad aligned particles with movement aligned to CameraForward (to avoid thin particles).
	PARTICLE_FLAGS_ALIGN_EMITTERMOVEMENT	= 0x00100000, // Consider emitter movement for alignment.
	PARTICLE_FLAGS_STOPMOTIONOLD			= 0x00200000, // Use old stopmotion formula. It sucks!
	PARTICLE_FLAGS_LIGHTING					= 0x00400000, // Use old stopmotion formula. It sucks!
	PARTICLE_FLAGS_RANDOMSEED				= 0x00800000, // Use creation tick as random seed base

	PARTICLE_FLAGS_USECUSTOMUV				= 0x20000000, // Texture animated using a texture map
	PARTICLE_FLAGS_USEBOXES					= 0x40000000, // Spawns inside given bounding boxes
	PARTICLE_FLAGS_USESKELETON				= 0x80000000, // Spawns on skeleton joints

	PARTICLE_MAXPARTICLEID				= 5,
};

#define TORAD 1.0f

//----------------------------------------------------------------------

#define HISTORY_LENGTH					(20)
#define MAXTIMEJUMP						(0.1f)

//----------------------------------------------------------------------

typedef CModelHistoryFX CMHistory;

//----------------------------------------------------------------------
// CXR_Model_Particles
//----------------------------------------------------------------------

#ifdef TESTNEWPARTICLES
	#define ParticlesOptClassName CXR_Model_Particles
#else
	#define ParticlesOptClassName CXR_Model_ParticlesOpt
#endif

class ParticlesOptClassName : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:
	ParticlesOptClassName();

protected:

	//----------------------------------------------------------------------


	fp32					m_SinRandTimeSpread;

	class PerRenderVariables
	{
		public:

		int					m_RandseedBase;

		fp32					m_Time; // Total lifetime of model.
		fp32					m_Duration, m_InvDuration;
		fp32					m_TimeFraction;

		fp32					m_DurationFade;

		fp32					m_lIDFade[PARTICLE_MAXPARTICLEID];
		int					m_IDDisableMask;

		CVec3Dfp32			m_CameraFwd, m_CameraLeft, m_CameraUp, m_CameraPos;

		fp32					m_ViewportXScale;

		int32				m_NumParticles;
		int					m_NumParallelParticles;

		int					m_NumCellsToBacktrace;
		int					m_NumTotalCells;

		CSurfaceKey			m_SK;
//		CXR_VertexBuffer*	m_pVB;
		CXR_VBChain*		m_pChain;

//		CWireContainer*		m_pWC;

		CMHistory*			m_pHistory;
		bool				m_bOverflow;

		int					m_Flags;

		CMTime				m_TimeDebug;
		int					m_DebugCount;

		// Moved from effect-system
		// Only valid if WObj_EffectSystem added them in animstate data
		CXR_SkeletonInstance*			m_pSkeletonInst;
		uint8							m_SkeletonType;
		CXR_Skeleton*					m_pSkeleton;

		CBox3Dfp32*						m_pSpawnBoxes;
		uint32							m_nSpawnBoxes;
	};


	int					m_MaxNumParticles;

	int					m_iSurface;
	fp32					m_RenderPriorityBias;

//	CStr				m_Keys;

	int					m_SystemFlags;

	fp32					m_FadeInTime, m_FadeOutTime;

	fp32					m_TimeScale;
	fp32					m_TimeOffset;

	fp32					m_ParticleDuration, m_ParticleDurationNoise;
	fp32					m_ParticleOffset, m_ParticleOffsetNoise;
	fp32					m_ParticleVelocity, m_ParticleVelocityNoise;

	fp32					m_Particle_StopMotionTime, m_Particle_InvStopMotionTime;

	fp32					m_ParticleSlowdownPower, m_InvParticleSlowdownPower;
	fp32					m_InvParticleSlowdownPowerPlusOne, m_InvInvParticleSlowdownPowerPlusOne;

	CVec3Dfp32			m_LocalPositionOffset;
	CVec3Dfp32			m_ParticleAcceleration, m_ParticleAccelerationNoise;
	fp32					m_AngleSpreadA, m_AngleSpreadB;
	fp32					m_AngleOffsetA, m_AngleOffsetB;
	fp32					m_AngleOffsetChangeA, m_AngleOffsetChangeB;
	CVec4Dfp32			m_ParticleColor, m_ParticleColorNoise;
	fp32					m_ParticleFadeInTime, m_InvParticleFadeInTime;
	fp32					m_ParticleFadeStillThreshold;

	fp32					m_ParticleAlignLengthScale;
	fp32					m_EmitterSpeedAlignScale;

	CPropertyControl	m_MoveCtrl, m_RotCtrl, m_SizeCtrl, m_AlphaCtrl;

	fp32					m_MaxParticleDuration;
	fp32					m_HistoryDuration, m_InvHistoryDuration;

	fp32					m_ParticleEmissionProbability;
	fp32					m_ParticleEmissionProbability_TimeScale;
	fp32					m_ParticleTimecellDuration;
	int					m_NumParticlesPerTimecell;
	fp32					m_ParticleTimecellSpread;
	fp32					m_ParticleEmissionStop;

	fp32					m_HistoryEntryDelay;

	CVec3Dfp32			m_DistributionSize;
	CVec3Dfp32			m_DistributionRotation;

	uint32				m_SeedCtrl;

	uint32				m_UVMapFrames;
	fp32				m_UVMapFramesMul;
	fp32				m_UVMapFramesU;
	fp32				m_UVMapFramesV;
	
//	CMat4Dfp32			m_DistRotMat;
//	bool				m_bApplyDistRotMat;

/*
	bool				m_bAngles;
	bool				m_bOffset;
	bool				m_bVelocity;
	bool				m_bAccelerate;
	bool				m_bNonUnitDistributionSize;
	bool				m_bLocalPositionOffset;
	bool				m_bColorNoise;
*/

	uint8				m_DistributionPrimitive;
	bool				m_bHollowDistribution;
	uint8				m_OptFlags;
//	uint8				m_Pad__;

	enum
	{
		OPTFLAGS_ANGLES					= 0x01,
		OPTFLAGS_OFFSET					= 0x02,
		OPTFLAGS_VELOCITY				= 0x04,
		OPTFLAGS_ACCELERATION			= 0x08,
		OPTFLAGS_NONUNITDISTSIZE		= 0x10,
		OPTFLAGS_LOCALOFFSET			= 0x20,
		OPTFLAGS_COLORNOISE				= 0x40,
		OPTFLAGS_DISTRIBUTIONROTATION	= 0x80,
//		OPTFLAGS_ISALIVE				= 0x80,
	};

	#define PARALELLPARTICLES_WINDOWSIZE 64

protected:
	class CBatch
	{
	public:
		uint32				m_ID[PARALELLPARTICLES_WINDOWSIZE];
		uint32				m_iRandseed[PARALELLPARTICLES_WINDOWSIZE];
		fp32					m_EmissionTime[PARALELLPARTICLES_WINDOWSIZE];
		fp32					m_Time[PARALELLPARTICLES_WINDOWSIZE];
		fp32					m_TimeFraction[PARALELLPARTICLES_WINDOWSIZE];
		fp32					m_Offset[PARALELLPARTICLES_WINDOWSIZE];

		CVec4Dfp32			m_Pos[PARALELLPARTICLES_WINDOWSIZE]; // Final
		CVec4Dfp32			m_Dir[PARALELLPARTICLES_WINDOWSIZE]; // Final
		CVec4Dfp32			m_Movement[PARALELLPARTICLES_WINDOWSIZE]; // Final

		fp32					m_Rot[PARALELLPARTICLES_WINDOWSIZE]; // Final
		fp32					m_Size[PARALELLPARTICLES_WINDOWSIZE]; // Final
		fp32					m_Alpha[PARALELLPARTICLES_WINDOWSIZE]; // Final. RGB is stored in m_Dir.
		fp32					m_TempScalar[PARALELLPARTICLES_WINDOWSIZE]; // Final
		CMat4Dfp32			m_LocalToWorld;
	};

public:
//	CBatch*				m_pBatch;
//	TThinArray<uint8>	m_lBatchData;

	//----------------------------------------------------------------------
	enum
	{
		GENERATE_FLAGS_TOTALOFFSET = 1,
		GENERATE_FLAGS_POSSET = 2,
		GENERATE_FLAGS_MOVEMENTSET = 4,
	};

	TPtr<CXR_ModelInstance> CreateModelInstance();

private:
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	virtual aint GetParam(int _Param);
	CModelData* AllocData();
public:
	CMHistory* AllocHistory();
private:
	CModelData*	GetData(const CXR_AnimState* _pAnimState) const;
	CMHistory* GetHistory(const CXR_AnimState* _pAnimState) const;
	CModelData* ForceGetData(const CXR_AnimState* _pAnimState);
	CMHistory* ForceGetHistory(const CXR_AnimState* _pAnimState);

	void GrowBoxAcceleration(CBox3Dfp32& _Box, const CModelData* _pData) const;
	void GrowBoxAngledOffset(CBox3Dfp32& _Box, fp32 _MaxOffset) const;

	void GetCubePoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	const fp32 GetRandSRadius(uint32& _iRandseed) const;
	void GetSpherePoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetCylinderSlicePoint(CVec3Dfp32& _point, fp32 _height, uint32& _iRandseed) const;
	void GetTubePoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetCylinderPoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetPyramidPoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetTetraidPoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetHollowCubePoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetHollowSpherePoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetHollowTubePoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetHollowCylinderPoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetHollowPyramidPoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	void GetHollowTetraidPoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	
	void RenderParticleWH_ZeroRot(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up);
	void RenderParticleWH_Rot(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up);
	void RenderParticleWH(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up);
	void RenderQuadParticleWH_ZeroRot(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up);
	void RenderQuadParticleWH_Rot(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up);
	void RenderQuadParticleWH(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up);
	void RenderQuadParticleArrayWH(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up);
	void RenderParticleArrayWH(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CVec3Dfp32& _Left, const CVec3Dfp32& _Up);
	void RenderParticleArray(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch);
	void RenderParticle(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle);
	void RenderQuadParticle(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle);
	void RenderQuadParticleArray(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch);
	void RenderParticleAligned(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle);
	void RenderQuadParticleAligned(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle);
	void RenderParticleAlignedSafeQuad(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle);
	void RenderQuadParticleAlignedSafeQuad(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const int _iParticle);
	static fp32 GetDepthFade(fp32 _Distance, fp32 _Start, fp32 _End);

	static uint RemoveParticle(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles);
	static uint RemoveParticle2(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles);
	static uint RemoveParticle21(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles);
	static uint RemoveParticle3(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles);
	static uint RemoveParticle4(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, uint _iParticle, uint _nParallelParticles);
	static void MoveParticle3(CBatch* M_RESTRICT _pBatch, uint _iFrom, uint _iTo);

	bool Generate_StandardSetup(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags);
	void Generate_HistoryCulling(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags);
	void Generate_Velocity(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags);
	void Generate_SizeControl(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags);
	void Generate_Colors(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int _GenerateFlags);
	bool GenerateParallelParticles(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CXR_Model_Custom_RenderParams* M_RESTRICT _pRenderParams);

	bool GenerateParticle(const CXR_Model_Custom_RenderParams* M_RESTRICT _pRenderParams, 
	                      PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch,
						  int _iParticleID, fp32 _EmissionTime, fp32 _ParticleTime, int _iRandseed);

	void ModifyEntry(CMat4Dfp32& _Matrix, fp32& _Time);
public:
	int M_INLINE GetMaxNumParticles() { return m_MaxNumParticles; }

	virtual void GetDistributedPoint(CVec3Dfp32& _point, uint32& _iRandseed) const;
	virtual void Generate_Distribution_Point(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int& _GenerateFlags);
	virtual void Generate_Distribution(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int& _GenerateFlags);
	virtual bool GenerateParticles(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CXR_Model_Custom_RenderParams* M_RESTRICT _pRenderParams);
	virtual bool Generate_Render(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CXR_Model_Custom_RenderParams* M_RESTRICT _pRenderParams, int _GenerateFlags);
	virtual void Generate_UVQuads(CXR_VBChain* _pVBChain, PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch);
	//virtual void Generate_UVTriangles(CXR_VBChain* _pVBChain, PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch);

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState);
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void RenderPerRenderVariables(const CXR_AnimState* _pAnimState, PerRenderVariables* M_RESTRICT _pPR);

	virtual void OnCreate(const char *_keys);
	inline virtual void SetLocalToWorld(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, const CMat4Dfp32& _WMat) { _pBatch->m_LocalToWorld = _WMat; }

private:
	void OnEvalKey(const CRegistry *_pReg);
	void ComputeParameters();
	void SetParameters_Default();

	friend class CFXLayer;
};

#endif

