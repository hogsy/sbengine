
#ifndef _INC_XRPCONTAINER
#define _INC_XRPCONTAINER

#include "XRClass.h"
#include "XREngine.h"
#include "XRUtil.h"

class CPC_Particle
{
public:
	fp32 m_Time;
	
	CPC_Particle()
	{
		m_Time = 0;
	}
};

class CXR_ParticleContainer : public CXR_Model
{
	MRTC_DECLARE;

protected:
	TArray<CXR_Particle2> m_lParticles;
	TArray<CPC_Particle> m_lParticleInfo;

	TArray<uint32> m_liParticles;

	int m_iHead;
	int m_iTail;

	int m_TextureID;

public:
	virtual void Create(const char* _pParam);

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);

	virtual void Evolve(fp32 _dTime);
	virtual void AddParticle(const CXR_Particle2& _P, fp32 _Time);
};

#endif
