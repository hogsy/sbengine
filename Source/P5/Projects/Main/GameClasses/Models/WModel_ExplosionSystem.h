#ifndef __WModel_ExplosionSystem_h__
#define __WModel_ExplosionSystem_h__

#include "CSurfaceKey.h"
#include "CModelHistory2.h"
#include "CPropertyControl.h"
#include "ModelsMisc.h"

#include "../../../../Shared/MOS/XR/XRUtil.h"

enum
{
	EXPLOSIONSYSTEM_FLAGS_DEPTHSORT		= M_Bit(0),
	EXPLOSIONSYSTEM_FLAGS_LIGHTING		= M_Bit(1),

	EXPLOSIONSYSTEM_GEN_ROTATION		= M_Bit(0),
};

class CXR_Model_ExplosionSystem
	: public CXR_Model_Custom
{
	MRTC_DECLARE;

//	typedef TVector4<uint16>	_V4Dui16;

	template<class T> class TMinMaxCtrl
	{
	public:
		T	m_Min;
		T	m_Max;

		T GetRandomInt(int32& _RandSeed)
		{
			return (T)((T(m_Max - m_Min) + 0.99f) * TruncToInt(MFloat_GetRand(_RandSeed++)) + m_Min);
		}

		T GetRandomFp(int32& _RandSeed)
		{
			return (((m_Max - m_Min) * MFloat_GetRand(_RandSeed++)) + m_Min);
		}

		T GetRandomFp(int32& _RandSeed, int32 _Offset)
		{
			return (((m_Max - m_Min) * MFloat_GetRand((_RandSeed++) + _Offset)) + m_Min);
		}

		CVec2Dfp32 GetRandomFpVec2(int32& _RandSeed, int32 _Offset)
		{
			CVec2Dfp32 Ret;
			CVec2Dfp32 Min = m_Min;
			CVec2Dfp32 Max = m_Max;
			Ret.k[0] = ((Min.k[1] - Min.k[0]) * MFloat_GetRand((_RandSeed++) + _Offset)) + Min.k[0];
			Ret.k[1] = ((Max.k[1] - Max.k[0]) * MFloat_GetRand((_RandSeed++) + _Offset)) + Max.k[0];
			return Ret;
		}

		CVec2Dfp32 GetRandomFpVec2(int32& _RandSeed)
		{
			CVec2Dfp32 Ret;
			CVec2Dfp32 Min = m_Min;
			CVec2Dfp32 Max = m_Max;
			Ret.k[0] = ((Min.k[1] - Min.k[0]) * MFloat_GetRand(_RandSeed++)) + Min.k[0];
			Ret.k[1] = ((Max.k[1] - Max.k[0]) * MFloat_GetRand(_RandSeed++)) + Max.k[0];
			return Ret;
		}

		void SetRange(const T& _Min, const T& _Max)
		{
			m_Min = _Min;
			m_Max = _Max;
		}
	};

	class CSmokePillar
	{
	public:
		TMinMaxCtrl<uint8>		m_Num;
		TMinMaxCtrl<CVec2Dfp32>	m_Force;
		TMinMaxCtrl<CVec4Dfp32>	m_Color;
		TMinMaxCtrl<CVec2Dfp32>	m_Rot;
		TMinMaxCtrl<fp32>		m_Size;
		fp32						m_SizeGrow;
		fp32						m_ColorFadeTime;
		uint8					m_NumTimeCellParticles;
		fp32						m_TimeCell;
		fp32						m_LifeTime;
		int						m_SurfaceID;
		fp32						m_FadeInTime;
		fp32						m_FadeOutTime;
		fp32						m_Gravity;
		fp32						m_EmissionStop;
		fp32						m_ZSize;
	};

	class CRenderParams
	{
	public:
		int32			m_Rand;
		fp32				m_Time;
		uint8			m_Smoke_NumPillars;
		uint32			m_nParticles;
		
		CVec3Dfp32		m_CameraLeft;
		CVec3Dfp32		m_CameraUp;
		CVec3Dfp32		m_CameraPos;

		CXR_VBManager*	m_pVBM;
        CXR_VBChain*	m_pChain;
		
		CVec3Dfp32*		m_pV;
		CPixel32*		m_pC;
		fp32*			m_pTempDepth;	// Temporary storage pointing nParticles into m_pC
		int32*			m_pTempRand;	// Temporary randomizing for pillars
		
		// Temporaries
		TThinArray<fp32>	m_lSize;
		TThinArray<fp32>	m_lRot;

		CSurfaceKey		m_SK;
	};

public:
	CXR_Model_ExplosionSystem();

	CSmokePillar		m_SmokePillar;
	fp32					m_Duration;
	fp32					m_AlphaFadeOut;
	fp32					m_TimeOffset;
	fp32					m_TimeScale;
	uint32				m_Flags;

	virtual void OnCreate(const char* _pParam);
	virtual void OnEvalKey(const CRegistry* _pReg);
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState);

private:

	void	SetDefaultParameters();

	void	VB_CalcSize(CRenderParams* M_RESTRICT _pRP);
	void	Generate_SmokePillars(CRenderParams* M_RESTRICT _pRP);
	void	Generate_Render(CRenderParams* M_RESTRICT _pRP, uint8 _GenerateFlags);
	void	Generate_Render_Rotation(CRenderParams* M_RESTRICT _pRP);
	void	Generate_Render_Simple(CRenderParams* M_RESTRICT _pRP);
	void	QuickSortParticles_r(CRenderParams* M_RESTRICT _pRP, uint32 _Offset, uint32 _nParticles);
};

#endif
