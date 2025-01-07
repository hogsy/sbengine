#include "PCH.h"

#include "XRPContainer.h"
#include "XRVertexBuffer.h"
#include "XRVBManager.h"

MRTC_IMPLEMENT_DYNAMIC(CXR_ParticleContainer, CXR_Model);

void CXR_ParticleContainer::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_ParticleContainer_Create, MAUTOSTRIP_VOID);
	int nParticles = 512;
	m_lParticles.SetLen(nParticles);
	m_lParticleInfo.SetLen(nParticles);
	m_liParticles.SetLen(nParticles);

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");

	m_TextureID = pTC->GetTextureID("P_SMOKE02abc");

	m_iHead = 0;
	m_iTail = 0;
}

void CXR_ParticleContainer::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_ParticleContainer_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_ParticleContainer::OnRender);

	CXR_Particle2* pP = m_lParticles.GetBasePtr();
	CPC_Particle* pPI = m_lParticleInfo.GetBasePtr();
	uint32* piP = m_liParticles.GetBasePtr();

	int nP = m_lParticles.Len();
// ConOutL(CStrF("(CXR_ParticleContainer::Render) nP %d, Head %d, Tail %d", nP, m_iHead, m_iTail));
	int iP = m_iTail;
	int nUsedP = 0;
	for(;iP != m_iHead; iP = (iP+1 >= nP) ? 0 : iP+1)
	{
		if (pPI[iP].m_Time) piP[nUsedP++] = iP;
	}

	if (nUsedP)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;

		CRC_Attributes* pA = pVB->m_pAttrib;
		if (!pA) return;
		pA->SetDefault();
		if (_pEngine) _pEngine->SetDefaultAttrib(pA);

		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
		pA->Attrib_TextureID(0, m_TextureID);
		pVB->m_Priority = 10.0f;

		if (CXR_Util::Render_Particles(_pVBM, pVB, _VMat, pP, nUsedP, NULL, CXR_PARTICLETYPE_TRIANGLE, piP))
		{
// ConOutL(CStrF("(CXR_ParticleContainer::Render) nUsed %d", nUsedP));
			_pVBM->AddVB(pVB);
//			CXR_Util::VB_RenderDebug(_pVBM, pVB, CXR_DEBUGVB_VERTICES, 0xff00ff00);
		}
	}
}

void CXR_ParticleContainer::Evolve(fp32 _dTime)
{
	MAUTOSTRIP(CXR_ParticleContainer_Evolve, MAUTOSTRIP_VOID);
	int nP = m_lParticles.Len();
	CXR_Particle2* pP = m_lParticles.GetBasePtr();
	CPC_Particle* pPI = m_lParticleInfo.GetBasePtr();

	int dAlpha = (int)(_dTime * 500);

	int iP = m_iTail;
	for(;iP != m_iHead; iP = (iP+1 >= nP) ? 0 : iP+1)
	{
		if (pPI[iP].m_Time)
		{
			pPI[iP].m_Time -= _dTime;
			if (pPI[iP].m_Time < 0.0f) pPI[iP].m_Time = 0;

			int Alpha = (pP[iP].m_Color >> 24) - dAlpha;
			if (Alpha < 0) Alpha = 0;
			pP[iP].m_Color = (pP[iP].m_Color & 0xffffff) + (Alpha << 24);
		}
	}

	int nDeleted = 0;
	iP = m_iTail;
	for(;iP != m_iHead; iP = (iP+1 >= nP) ? 0 : iP+1)
	{
		if (pPI[iP].m_Time > 0.0f) break;

		nDeleted++;
		m_iTail++;
		if (m_iTail >= nP) m_iTail = 0;
	}

//ConOutL(CStrF("(CXR_ParticleContainer::Evolve) dTime %f, nDel %d", _dTime, nDeleted));
}

void CXR_ParticleContainer::AddParticle(const CXR_Particle2& _P, fp32 _Time)
{
	MAUTOSTRIP(CXR_ParticleContainer_AddParticle, MAUTOSTRIP_VOID);
	int nP = m_lParticles.Len();
	int iNext = (m_iHead+1 == nP) ? 0 : m_iHead+1;
	if (iNext == m_iTail) return;

	CXR_Particle2* pP = m_lParticles.GetBasePtr();
	CPC_Particle* pPI = m_lParticleInfo.GetBasePtr();
	pP[m_iHead] = _P;
	pPI[m_iHead].m_Time = _Time;

	m_iHead = iNext;
//ConOut(CStrF("(CXR_ParticleContainer::AddParticle) iHead %d, iTail %d, Pos %s", m_iHead, m_iTail, (char*)_P.m_Pos.GetString() ));
}

