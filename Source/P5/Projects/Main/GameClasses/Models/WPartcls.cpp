#include "PCH.h"

#include "WPartcls.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XREngineVar.h"


// -------------------------------------------------------------------
//  CXR_Model_BasicParticles
// -------------------------------------------------------------------
bool CXR_Model_ParticleSystem::ms_bRandomInitialized = false;
fp32 CXR_Model_ParticleSystem::ms_lRandom[CXR_PARTICLES_NUMRANDOM];
int CXR_Model_ParticleSystem::ms_lRandomInt[CXR_PARTICLES_NUMRANDOM];

//spCTextureContainer_Plain CXR_Model_ParticleSystem::ms_spTC;

int CXR_Model_ParticleSystem::GetRenderPass(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_ParticleSystem_GetRenderPass, 0);
	return m_RenderPass;
}

fp32 CXR_Model_ParticleSystem::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_ParticleSystem_GetBound_Sphere, 0.0f);
	return m_BoundR;
}

fp32 CXR_Model_ParticleSystem::GetBound_Animated_Sphere(const CXR_AnimState* _pAnimState)
{ 
	MAUTOSTRIP(CXR_Model_ParticleSystem_GetBound_Animated_Sphere, 0.0f);
	return GetBound_Sphere();
}

CXR_Model_ParticleSystem::CXR_Model_ParticleSystem()
:m_RenderInfo(NULL)
{
	MAUTOSTRIP(CXR_Model_ParticleSystem_ctor, MAUTOSTRIP_VOID);
	m_RenderPass = 1;
	m_BoundR = 16.0f;

	if (!ms_bRandomInitialized)
	{
		for(int i = 0; i < CXR_PARTICLES_NUMRANDOM; i++)
		{
			ms_lRandom[i] = Random;
			ms_lRandomInt[i] = (int)(Random*255.0f);
		}
		ms_bRandomInitialized = true;
	}

	MACRO_GetRegisterObject(CTextureContext, pTContext, "SYSTEM.TEXTURECONTEXT");
	if (!pTContext) Error("-", "No texture-context available.");

//	m_TextureID = pTContext->GetTextureID("EXPLOSION05");
	m_TextureID = pTContext->GetTextureID("P_SOFT1");
	if (m_TextureID)
	{
		CTextureContainer* pTC = pTContext->GetTextureContainer(m_TextureID);
		if (pTC->MRTC_ReferenceCount() < 1) Error("-", "Particle-texture's TC have zero owners.");
		m_spTC = pTC;
	}
	else
		LogFile("§cf80WARNING: Particle-texture not found (PARTICLE_SOFT1)");

	// ACHTUNG!, sp i global-scope -> förstörs EFTER g_pOS & TextureContext !!!!
/*	if (ms_spTC == NULL)
	{
		ms_spTC = DNew(CTextureContainer_Plain) CTextureContainer_Plain;
		if (ms_spTC == NULL) MemError("-");

		CDataFile DFile;
		DFile.Open("Particle.XTC");
		if (!DFile.GetNext("IMAGELIST")) Error("-", "Invalid XTC.");
		if (!DFile.GetSubDir()) Error("-", "Invalid XTC.");
		ms_spTC->AddFromImageList(&DFile);
		DFile.Close();
	}*/
}

void CXR_Model_ParticleSystem::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_ParticleSystem_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_ParticleSystem::OnRender);
	CMat4Dfp32 L2VMat;
	_WMat.Multiply(_VMat, L2VMat);
//LogFile("(CXR_Model_ParticleSystem) 1");
	// Got a ViewClip interface?
	if (_pViewClip)
	{
		// Get bound-sphere, get the CRC_ClipView
		fp32 BoundR = GetBound_Animated_Sphere(_pAnimState);		// Local
		m_RenderInfo.m_pCurrentEngine = _pEngine;
		if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), BoundR, 0, 0, NULL, &m_RenderInfo)) return;
	}

	m_pEngine = _pEngine;
	if ((m_RenderInfo.m_Flags & CXR_RENDERINFO_VERTEXFOG) && m_pEngine->m_pCurrentFogState)
	{
		m_pEngine->m_pCurrentFogState->SetTransform(NULL);
		m_pEngine->m_pCurrentFogState->VertexFog_Eval(1, &CVec3Dfp32::GetMatrixRow(_WMat, 3), NULL, &m_CurrentFog, 0, 0);
		if (m_CurrentFog.GetA() == 255) return;
	}
	else
		m_CurrentFog = 0;

	// Damn!, it's visible, Let's render...
/*	_pRender->Attrib_Push();
	_pRender->Matrix_Push();
	_pRender->Matrix_Set(L2VMat);

	_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	_pRender->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pRender->Attrib_TextureID(0, m_TextureID);*/
	RenderParticles(_pRender, _spWLS, _pVBM, _pAnimState, _WMat, _VMat, L2VMat, _Flags);

/*	_pRender->Matrix_Pop();
	_pRender->Attrib_Pop();*/
}

// -------------------------------------------------------------------
//  CXR_Model_GenericParticles
// -------------------------------------------------------------------

/*
void CXR_Model_Particles_Generic::GetBound_Animated_Sphere(const CXR_AnimState* _pAnimState, CVec3Dfp32& _v0, fp32& _Radius)
{
	MAUTOSTRIP(CXR_Model_Particles_Generic_GetBound_Animated_Sphere, MAUTOSTRIP_VOID);
	CGenericAttributes* pA = &m_ParticleAttr;

}
*/
void CXR_Model_Particles_Generic::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_Generic_RenderParticles, MAUTOSTRIP_VOID);
/*
	CGenericAttributes* pA = &m_ParticleAttr;
	
	fp32 Frame = 0;

	if (!_pAnimState) return;

	Frame = _pAnimState->m_AnimTime0.GetTime();
	int iRand = _pAnimState->m_Anim0 & 0xff;
	int nParticles = (_pAnimState->m_Anim0 >> 8) & 0xff;

	if (nParticles <= 0) return;
	if (nParticles > 32) return;
	CRC_Particle lParticles[32];

	_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	fp32 size = fp32((_pAnimState->m_Anim1 >> 8) & 0xff);

	int iParticle = 0;
	for(int iBatch = 0; iBatch < pA->m_nClusters; iBatch++)
	{
		fp32 drift = 1.0f + Frame * fp32((_pAnimState->m_Anim1 & 0xff)/256.0f);
		fp32 s = size + drift;

		for(int iP = 0; iP < nParticles; iP++)
		{
			fp32 x = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * s;
			fp32 y = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * s;
			fp32 z = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * s;

			iParticle++;
		}

//		_pRender->RenderParticles(&lParticles[0], np, lPartSizes[iParticleBatch]);
	}
*/
};


#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
//  CXR_Model_Particles_TorchFire
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_TorchFire, CXR_Model_ParticleSystem);

void CXR_Model_Particles_TorchFire::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_TorchFire_RenderParticles, MAUTOSTRIP_VOID);

	// Anim0:
	//  0 = ZAxis
	//  1 = XAxis
	//  2 = YAxis

//	ConOutL(CStrF("%f %s", _pAnimState->m_AnimTime0, (char *)CVec3Dfp32::GetMatrixRow(_WMat, 3).GetString()));

	int iAxis = 0;
	int bFlip = 0;
	fp32 Frame;
	if (_pAnimState)
	{
		Frame = _pAnimState->m_AnimTime0.GetTime() * 0.5f * 20.0f;
		iAxis = _pAnimState->m_Anim0 & 0xff;
		bFlip = _pAnimState->m_Anim0 & 0x100;
	}
	else
		Frame = CMTime::GetCPU().GetTimeModulusScaled(15.0f, 15.0f);

	int iRand = 80;

	int m_nParticles = 20;
	if (_L2VMat.k[3][2] < 256)
		m_nParticles += 20;
	if (_L2VMat.k[3][2] < 128)
		m_nParticles += 20;
	if (_L2VMat.k[3][2] < 96)
		m_nParticles += 20;
	if (_L2VMat.k[3][2] < 64)
		m_nParticles += 20;
	if (_L2VMat.k[3][2] < 48)
		m_nParticles += 20;

	fp32 lPartSizes[6] = { 14, 8, 5, 2.0f, 1.5f, 1.0f };

//	CRC_Particle lParticles[20];
	CXR_Particle2 lParticles[120];

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);

	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
	if (!pVB) return;
	pVB->m_pAttrib = pA;
	pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

	int iP = 0;
	int iParticleBatch = 0;
	int nleft = m_nParticles;
	int iParticle = 0;
	while(nleft > 0)
	{
		int np = Min(nleft, 20);
		fp32 zoffs =  Frame + iParticleBatch*6;
		fp32 Size = lPartSizes[iParticleBatch] * 1.5f;
		for(int i = 0; i < np; i++)
		{
//			fp32 z = M_FMod(ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 16.0f + zoffs, 16.f);
			fp32 z = fp32(((ms_lRandomInt[(iRand++) & CXR_PARTICLES_NUMAND] << 4) + RoundToInt(256.0f*zoffs)) & 0x0fff) / 256.0f;
			z = z*0.5f + 2.0f*z*z/16.0f;
//			fp32 r = M_Sin(z*2.0f / (z + 2.0f) * (1.0f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]) )*(3.0f * (32.0f-z) / 32.0f);
			fp32 r = QSin(z*2.0f / (z + 2.0f) * (1.0f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]) ) * (3.0f * (32.0f-z) / 32.0f);

			fp32 offs = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 6.0f + Frame/27.0f;
//			fp32 x = sin(offs + Sqr(z) * 0.01f) * r;
//			fp32 y = cos(offs + Sqr(z) * 0.01f) * r;

			fp32 x, y;
			QSinCos(offs + Sqr(z) * 0.01f, x, y);
			x *= r;
			y *= r;

			int alpha = Max(0.0f, (32.0f - z)) * 8.0f;
			alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;
			
			fp32 w = (bFlip) ? (-z * 0.8f) : (z * 0.8f);

			if (iAxis == 1)
				{ z = x; x = w; }
			else if (iAxis == 2)
				{ z = y; y = w; };

			lParticles[iP].m_Pos.k[0] = x;
			lParticles[iP].m_Pos.k[1] = y;
			lParticles[iP].m_Pos.k[2] = z;
			lParticles[iP].m_Size = Size;

//			lParticles[i].m_Angle = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]-0.5f) * Frame / 20.0f;
//			lParticles[i].m_Size = 1.0f;
//			lParticles[i].m_Color = 
//				0x00808080 + (alpha << 24);

			int c = (ms_lRandomInt[(iRand++) & CXR_PARTICLES_NUMAND]);

			lParticles[iP].m_Color = 
				(((255-c) >> 5) << 16) + (c << 16) + 
				(((c >> 1) + (c >> 2)) << 8) +
				((255-c) >> 3) + (c >> 2) +
				0x00000000 + (alpha << 24);
			iP++;
		}
//		CXR_Util::Render_Particles(m_pEngine, _pRender, &lParticles[0], np, lPartSizes[iParticleBatch]);

		
//		_pRender->Render_Particles(&lParticles[0], np, lPartSizes[iParticleBatch]);

		iParticle += np;
		nleft -= np;
		iParticleBatch++;
	}

	if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], iP, NULL, CXR_PARTICLETYPE_TRIANGLE))
		_pVBM->AddVB(pVB);
}
#endif

#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
//  CXR_Model_Particles_FireWall
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_FireWall, CXR_Model_ParticleSystem);

void CXR_Model_Particles_FireWall::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_FireWall_RenderParticles, MAUTOSTRIP_VOID);
	// Anim0:
	//  0 = ZAxis
	//  1 = XAxis
	//  2 = YAxis

	int iAxis = 0;
	int bFlip = 0;
	fp32 Frame;
	if (_pAnimState)
	{
		Frame = _pAnimState->m_AnimTime0.GetTime() * 0.5f * 20.0f;
		iAxis = _pAnimState->m_Anim0 & 0xff;
		bFlip = _pAnimState->m_Anim0 & 0x100;
	}
	else
		Frame = CMTime::GetCPU().GetTimeModulusScaled(15.0f, 15.0f);

	int iRand = 80;

	const int BatchSize = 20;
	int m_nParticles = BatchSize;
	if (_L2VMat.k[3][2] < 512)
		m_nParticles += BatchSize;
	if (_L2VMat.k[3][2] < 256)
		m_nParticles += BatchSize;
	if (_L2VMat.k[3][2] < 128)
		m_nParticles += BatchSize;
	if (_L2VMat.k[3][2] < 96)
		m_nParticles += BatchSize;
	if (_L2VMat.k[3][2] < 64)
		m_nParticles += BatchSize;

	fp32 lPartSizes[6] = { 16, 12, 6, 0.5f, 0.4f, 0.25f };
	CRC_Particle lParticles[BatchSize];

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);

	int iParticleBatch = 0;
	int nleft = m_nParticles;
	int iParticle = 0;
	while(nleft > 0)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;
		int np = Min(nleft, BatchSize);
		fp32 zoffs =  Frame*0.1f + iParticleBatch*6;
		for(int i = 0; i < np; i++)
		{
			fp32 Height = 60.0f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 40.0f;
			fp32 z = M_FMod(ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] + zoffs, 1.0f);
			z = (z + z*z)*0.5f;
			fp32 r = M_Sin(z*2.0f*_PI / (z + 1.0f) * (1.0f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]) )*(8.0f * (1.0f-z));
			fp32 offs = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 6.0f + Frame/27.0f;
			fp32 x = M_Sin(offs + Sqr(z) * 0.01f) * r + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]*32.0f - 16.0f;
			fp32 y = M_Cos(offs + Sqr(z) * 0.01f) * r;
			x += ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 16.0f - 8.0f;
			y += ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 16.0f - 8.0f;
			int alpha = Max(0.0f, (1.0f - z)) * 255.0f;

			z *= Height;
			fp32 w = (bFlip) ? (-z * 0.8f) : (z * 0.8f);
			z -= 8.0f;

			if (iAxis == 1)
				{ z = x; x = w; }
			else if (iAxis == 2)
				{ z = y; y = w; };

			lParticles[i].m_Pos.k[0] = x;
			lParticles[i].m_Pos.k[1] = y;
			lParticles[i].m_Pos.k[2] = z;

			int c = (ms_lRandomInt[(iRand++) & CXR_PARTICLES_NUMAND]);
			lParticles[i].m_Color = 
				(((255-c) >> 5) << 16) + (c << 16) + 
				(((c >> 1) + (c >> 2)) << 8) +
				((255-c) >> 3) + (c >> 2) +
				0x00000000 + (alpha << 24);
		}
		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], np, lPartSizes[iParticleBatch] * 4.0f))
			_pVBM->AddVB(pVB);
//		_pRender->Render_Particles(&lParticles[0], np, lPartSizes[iParticleBatch] * 4.0f);

		iParticle += np;
		nleft -= np;
		iParticleBatch++;
	}
}
#endif

#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
//  CXR_Model_Particles_SmallFire
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_SmallFire, CXR_Model_ParticleSystem);

void CXR_Model_Particles_SmallFire::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_SmallFire_RenderParticles, MAUTOSTRIP_VOID);
	// Anim0:
	//  0 = ZAxis
	//  1 = XAxis
	//  2 = YAxis

	int iAxis = 0;
	int bFlip = 0;
	fp32 Frame;
	if (_pAnimState)
	{
		Frame = _pAnimState->m_AnimTime0.GetTime() * 0.5f * 20.0f;
		iAxis = _pAnimState->m_Anim0 & 0xff;
		bFlip = _pAnimState->m_Anim0 & 0x100;
	}
	else
		Frame = CMTime::GetCPU().GetTimeModulusScaled(15.0f, 15.0f);

	fp32 Time_Render = 0;
	int iRand = 80;

	int m_nParticles = 20;
	if (_L2VMat.k[3][2] < 96)
		m_nParticles += 20;
	if (_L2VMat.k[3][2] < 64)
		m_nParticles += 20;
	if (_L2VMat.k[3][2] < 48)
		m_nParticles += 20;

	fp32 lPartSizes[4] = { 3, 1.5f, 1.0f, 0.5f };

	CRC_Particle lParticles[20];

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);

	int iParticleBatch = 0;
	int nleft = m_nParticles;
	int iParticle = 0;
	while(nleft > 0)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		int np = Min(nleft, 20);
		fp32 zoffs =  Frame*0.1f + iParticleBatch*6;
		for(int i = 0; i < np; i++)
		{
			fp32 Height = 6.0f;
			fp32 z = M_FMod(ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] + zoffs, 1.0f);
			z = (z + z*z)*0.5f;
			fp32 r = M_Sin(z*2.0f*1.0f*_PI / (z + 1.0f) * (1.0f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]) )*(1.0f * (1.0f-z));
			fp32 offs = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 6.0f + Frame/27.0f;
			fp32 x = M_Sin(offs + Sqr(z) * 0.01f) * r;
			fp32 y = M_Cos(offs + Sqr(z) * 0.01f) * r;
			int alpha = Max(0.0f, (1.0f - z)) * 255.0f;
			alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;
			
			z *= Height;
			fp32 w = (bFlip) ? -z : z;

			if (iAxis == 1)
				{ z = x; x = w; }
			else if (iAxis == 2)
				{ z = y; y = w; };

			lParticles[i].m_Pos.k[0] = x;
			lParticles[i].m_Pos.k[1] = y;
			lParticles[i].m_Pos.k[2] = z;

			int c = (ms_lRandomInt[(iRand++) & CXR_PARTICLES_NUMAND]);
			lParticles[i].m_Color = 
				(((255-c) >> 5) << 16) + (c << 16) + 
				(((c >> 1) + (c >> 2)) << 8) +
				((255-c) >> 3) + (c >> 2) +
				0x00000000 + (alpha << 24);
		}
		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], np, lPartSizes[iParticleBatch], NULL, CXR_PARTICLETYPE_TRIANGLE))
			_pVBM->AddVB(pVB);
//		_pRender->Render_Particles(&lParticles[0], np, lPartSizes[iParticleBatch]);

		iParticle += np;
		nleft -= np;
		iParticleBatch++;
	}
}
#endif

#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
//  CXR_Model_Particles_InfernoFlare
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_InfernoFlare, CXR_Model_ParticleSystem);

void CXR_Model_Particles_InfernoFlare::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_InfernoFlare_RenderParticles, MAUTOSTRIP_VOID);

	fp32 Frame;
	int iRand = 0;
	if (_pAnimState)
	{
		Frame = _pAnimState->m_AnimTime0.GetTime() * 20.0f;
		iRand = _pAnimState->m_Anim0;
	}
	else
		Frame = CMTime::GetCPU().GetTimeModulusScaled(15.0f, 15.0f);

	CMat4Dfp32 WInv;
	_WMat.InverseOrthogonal(WInv);
	CVec3Dfp32 VUp(WInv.k[2][0], WInv.k[2][1], WInv.k[2][2]);

	int nParticles = 20;

	CRC_Particle lParticles[50];

//	_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
/*	for(int iBatch = 0; iBatch < 3; iBatch++)
	{
		fp32 t = Frame;
		fp32 Drift = 2.0f + fp32(iBatch+1)*6.0f*sqrt(Frame/m_TotTime);

		int alpha = Max(0.0f, 0.4f*Min(255.0f - t/m_TotTime*255.0f, t*16.0f));

		for(int i = 0; i < nParticles; i++)
		{
			fp32 x = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;
			fp32 y = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;
			fp32 z = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;

			fp32 s = t*0.02f + Sqr(t*(0.06f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]*0.03f));

			x += VUp.k[0]*s;
			y += VUp.k[1]*s;
			z += VUp.k[2]*s;

			lParticles[i].m_Pos.k[0] = x;
			lParticles[i].m_Pos.k[1] = y;
			lParticles[i].m_Pos.k[2] = z;

			lParticles[i].m_Color = (alpha << 24) + 0x8080ff;
		}

Time_Render -= (fp32)GetCPUClock();
		_pRender->RenderParticles(&lParticles[0], nParticles, 8.0f + t/40.0f*4.0f);
Time_Render += (fp32)GetCPUClock();
	}
*/

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);

	fp32 lPartSizes[8] = { 8, 6, 4, 3, 3, 3, 2, 2 };
//	_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	nParticles = 20;
	for(int iBatch = 0; iBatch < 8; iBatch++)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		fp32 Drift = 2.0f + fp32(iBatch*0.1f+1.0f)*6.0f;
		int alpha = 255;
		alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;
		fp32 vadd = Frame * 0.01f;

		for(int i = 0; i < nParticles; i++)
		{
			fp32 v = vadd + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 2.0f * _PI;
			fp32 x = M_Sin(v) * Drift;
			fp32 y = M_Cos(v) * Drift;
			fp32 z = 0;
//			fp32 z = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;

//				y += y*Drift;

/*			x += VUp.k[0]*s;
			y += VUp.k[1]*s;
			z += VUp.k[2]*s;*/

			lParticles[i].m_Pos.k[0] = x;
			lParticles[i].m_Pos.k[1] = y;
			lParticles[i].m_Pos.k[2] = z;

//			int glow = Max(0.0f, (255.0f - t*8))*(ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]);
			int glow = 255;

			lParticles[i].m_Color = 
				((glow >> 2) << 16) + 
				((glow >> 2) << 8) + 
				(glow >> 0) +
				(alpha << 24);
		}

		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], nParticles, lPartSizes[iBatch]))
			_pVBM->AddVB(pVB);
//			_pRender->Render_Particles(&lParticles[0], nParticles, lPartSizes[iBatch]);
	}

//	LogFile(T_String("Particles", Time) + T_String(" (Eval", Time - Time_Render) + T_String(", Render", Time_Render) + ")");
}
#endif

#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CXR_Model_Particles_Sparkles
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_Sparkles, CXR_Model_ParticleSystem);

void CXR_Model_Particles_Sparkles::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_Sparkles_RenderParticles, MAUTOSTRIP_VOID);

	fp32 Frame;
	int iBaseRand = 0;
	if (_pAnimState)
	{
		Frame = _pAnimState->m_AnimTime0.GetTime() * 20.0f;
		iBaseRand = _pAnimState->m_Anim0;
	}
	else
		Frame = CMTime::GetCPU().GetTimeModulusScaled(15.0f, 15.0f);

	if (Frame > 36) return;

	CMat4Dfp32 WInv;
	_WMat.InverseOrthogonal(WInv);
	CVec3Dfp32 VUp(WInv.k[2][0], WInv.k[2][1], WInv.k[2][2]);

	int nParticles = 4;
	int iRand = iBaseRand;

	CXR_Particle2 lParticles[6*4*2];

	int nBatch = 4;
	if (_L2VMat.k[3][2] > 256)
		nBatch = 1;
	else if (_L2VMat.k[3][2] > 192)
		nBatch = 2;
	else if (_L2VMat.k[3][2] > 128)
		nBatch = 3;

	//----------------------
	// SMOKE
	//----------------------
	{
		int iP = 0;
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if (!pA) return;
		pA->SetDefault();
		if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pA->Attrib_TextureID(0, m_TextureID);

		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

	//	_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		int iBatch;
		for(iBatch = 0; iBatch < nBatch; iBatch++)
		{
			fp32 tgrav = (Frame - 8.0f - iBatch);
			fp32 t = tgrav*32.0f / 24.0f;
			fp32 zadd = 16.0f*iBatch;
			fp32 Drift = Frame / 200.0f;
			if ((t >= 0) && (t < 32.0f))
			{
				iRand = iBatch*23 + iBaseRand*5 + 317;
				int alpha = Max(0.0f, Min((255.0f - t*8), t*32.0f) * 0.3f);
				alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;
				int Color = (alpha << 24) + 0x404040*0;

				fp32 Size = (8.0f + t/32.0f*4.0f) * 2.0f;

				int nP = (_L2VMat.k[3][2] < 32.0f) ? 1 : nParticles;
				for(int i = 0; i < nP; i++)
				{
					fp32 z = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 2.0f - 1.0f;
					fp32 y = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 2.0f - 1.0f;
					fp32 x = zadd + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 16.0f - 8.0f;

					z += z*Drift;
	//				y += y*Drift;
					fp32 s = tgrav*0.02f + Sqr(tgrav*(0.06f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]*0.03f));

					x += VUp.k[0]*s;
					y += VUp.k[1]*s;
					z += VUp.k[2]*s;

					lParticles[iP].m_Pos.k[0] = x;
					lParticles[iP].m_Pos.k[1] = y;
					lParticles[iP].m_Pos.k[2] = z;

					lParticles[iP].m_Color = Color;
					lParticles[iP].m_Size = Size;
					iP++;
				}

	//			_pRender->Render_Particles(&lParticles[0], nP, 8.0f + t/32.0f*4.0f);
//				if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], nP, ))
//					_pVBM->AddVB(pVB);

			}
		}
		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, lParticles, iP, NULL, CXR_PARTICLETYPE_TRIANGLE)) _pVBM->AddVB(pVB);
	}

	//----------------------
	// SPARKLES
	//----------------------
	{
		int iP = 0;
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if (!pA) return;
		pA->SetDefault();
		if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pA->Attrib_TextureID(0, m_TextureID);

		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		nParticles = 6;
		for(int iSize = 0; iSize < 2; iSize++)
		{
			for(int iBatch = 0; iBatch < nBatch; iBatch++)
			{

				fp32 tgrav = (Frame - iBatch);
				fp32 t = tgrav*(1 + iSize);
				fp32 zadd = 16.0f*iBatch;
				fp32 Drift = Frame / 200.0f;
				int alpha = Max(0.0f, (255.0f - t*8));
				alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;
				if ((t >= 0) && (t < 32))
				{
					iRand = iBatch*27 + iBaseRand*7 + iSize*57;
					fp32 Size = (1.0f + iSize*4.0f)*1.5f;
					for(int i = 0; i < nParticles; i++)
					{
						fp32 z = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 2.0f - 1.0f;
						fp32 y = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 2.0f - 1.0f;
						fp32 x = zadd + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 16.0f - 8.0f;

						z += z*Drift;
		//				y += y*Drift;
						fp32 s = tgrav*0.02f + Sqr(tgrav*(0.04f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]*0.02f));

						x += VUp.k[0]*s;
						y += VUp.k[1]*s;
						z += VUp.k[2]*s;

						lParticles[iP].m_Pos.k[0] = x;
						lParticles[iP].m_Pos.k[1] = y;
						lParticles[iP].m_Pos.k[2] = z;

						int glow = Max(0.0f, (255.0f - t*8))*(ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]);
						lParticles[iP].m_Color = 
							(glow << 16) + 
							(((glow >> 1) + (glow >> 2)) << 8) + 
							(glow >> 2) +
							(alpha << 24);
						lParticles[iP].m_Size = Size;
						iP++;
					}

	//				_pRender->Render_Particles(&lParticles[0], nParticles, 2.0f + iSize*4.0f);
	//				if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], nParticles, 2.0f + iSize*4.0f))
	//					_pVBM->AddVB(pVB);
				}
			}
		}
		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, lParticles, iP, NULL, CXR_PARTICLETYPE_TRIANGLE)) _pVBM->AddVB(pVB);
	}

//	LogFile(T_String("Particles", Time) + T_String(" (Eval", Time - Time_Render) + T_String(", Render", Time_Render) + ")");
}
#endif

#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CXR_Model_Particles_Explosion
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_Explosion, CXR_Model_ParticleSystem);

void CXR_Model_Particles_Explosion::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_Explosion_RenderParticles, MAUTOSTRIP_VOID);

	int nBatch = 8;
	fp32 Frame;
	fp32 Size = 1.0f;
	int iRand = 0;
	int bDoSmoke = 1;

	if (_pAnimState)
	{
		Frame = _pAnimState->m_AnimTime0.GetTime() * 20.0f;
		iRand = _pAnimState->m_Anim0;
		Size = (_pAnimState->m_Anim1 & 0xff) + 1.0f;
		nBatch = Min(8, (_pAnimState->m_Anim1 >> 8) & 0x0f);
		bDoSmoke = !(_pAnimState->m_Anim1 & 0x8000);
	}
	else
		Frame = CMTime::GetCPU().GetTimeModulusScaled(15.0f, 15.0f);

	if (Frame > 40) return;

	CMat4Dfp32 WInv;
	_WMat.InverseOrthogonal(WInv);
	CVec3Dfp32 VUp(WInv.k[2][0], WInv.k[2][1], WInv.k[2][2]);

	int nParticles = 20;

	CRC_Particle lParticles[50];

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);

	if (bDoSmoke)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		for(int iBatch = 0; iBatch < 2 /*3*/; iBatch++)
		{
			fp32 t = Frame;
			fp32 Drift = 2.0f + fp32(iBatch+1)*16.0f*M_Sqrt(Frame*(1.0f/40.0f))*Size;

			int alpha = Max(0.0f, 0.3f*Min(255.0f - t*(1.0f/40.0f)*255.0f, t*16.0f));
			alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;

			for(int i = 0; i < nParticles; i++)
			{
				fp32 x = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;
				fp32 y = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;
				fp32 z = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;

				fp32 s = t*0.02f + Sqr(t*(0.06f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]*0.03f));

				x += VUp.k[0]*s;
				y += VUp.k[1]*s;
				z += VUp.k[2]*s;

				lParticles[i].m_Pos.k[0] = x;
				lParticles[i].m_Pos.k[1] = y;
				lParticles[i].m_Pos.k[2] = z;

				lParticles[i].m_Color = (alpha << 24) + 0x404040*0;
			}

		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], nParticles, 8.0f + t/40.0f*16.0f*Size, NULL, CXR_PARTICLETYPE_TRIANGLE))
			_pVBM->AddVB(pVB);
//			_pRender->Render_Particles(&lParticles[0], nParticles, 8.0f + t/40.0f*16.0f*Size);
		}
	}

	fp32 lPartSizes[8] = { 16*2, 10, 5, 3, 3, 3, 2, 2 };

	pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);


nBatch = 2;
	nParticles = 20;
	for(int iBatch = 0; iBatch < nBatch; iBatch++)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		fp32 t = Frame - iBatch;
		fp32 Drift = 8.0f + fp32(iBatch+1)*40.0f*M_Sqrt(Frame*(1.0f/40.0f))*Size;
		int alpha = Max(0.0f, (255.0f - t*(1.0f/40.0f)*255.0f));
		alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;

		for(int i = 0; i < nParticles; i++)
		{
			fp32 x = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;
			fp32 y = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;
			fp32 z = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;

//				y += y*Drift;
			fp32 s = t*0.02f + Sqr(t*(0.04f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]*0.02f));

/*			x += VUp.k[0]*s;
			y += VUp.k[1]*s;
			z += VUp.k[2]*s;*/

			z -= Sqr(t*0.1f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]*0.1f);

			lParticles[i].m_Pos.k[0] = x;
			lParticles[i].m_Pos.k[1] = y;
			lParticles[i].m_Pos.k[2] = z;

			int glow = Max(0.0f, (255.0f - t*8))*(ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]);

			lParticles[i].m_Color = 
				(glow << 16) + 
				(((glow >> 1) + (glow >> 2)) << 8) + 
				(glow >> 2) +
				(alpha << 24);
		}

		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], nParticles, lPartSizes[iBatch]*Size, NULL, CXR_PARTICLETYPE_TRIANGLE))
			_pVBM->AddVB(pVB);
//			_pRender->Render_Particles(&lParticles[0], nParticles, lPartSizes[iBatch]*Size);
	}

//	LogFile(T_String("Particles", Time) + T_String(" (Eval", Time - Time_Render) + T_String(", Render", Time_Render) + ")");
}
#endif


#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CXR_Model_Particles_Teleporter
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_Teleporter, CXR_Model_ParticleSystem);

void CXR_Model_Particles_Teleporter::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_Teleporter_RenderParticles, MAUTOSTRIP_VOID);
	bool bZFlip = false;
	fp32 Frame;
	if (_pAnimState)
	{
		Frame = _pAnimState->m_AnimTime0.GetTime() * 0.5f * 20.0f;
		bZFlip = (_pAnimState->m_Anim0 == 1);
	}
	else
		Frame = CMTime::GetCPU().GetTimeModulusScaled(15.0f, 15.0f);

	fp32 Time_Render = 0;
	int iRand = 80;

	int m_nParticles = 1;
	if (_L2VMat.k[3][2] < 356)
		m_nParticles++;
	if (_L2VMat.k[3][2] < 250)
		m_nParticles++;
	if (_L2VMat.k[3][2] < 160)
		m_nParticles++;
	if (_L2VMat.k[3][2] < 140)
		m_nParticles++;
	if (_L2VMat.k[3][2] < 128)
		m_nParticles++;
	if (_L2VMat.k[3][2] < 112)
		m_nParticles++;
	if (_L2VMat.k[3][2] < 96)
		m_nParticles++;

	m_nParticles *= 40;

	fp32 lPartSizes[8] = { 16, 6, 10, 8, 4, 4, 1.5f, 1.0f };
	fp32 lSinOffsScale[8] = { 1.5f, 0.1f, 0.5f, 0.4f, 0.2f, 0.2f, 0.4f, 0.3f };
	fp32 lOffsAdd[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	fp32 lRadius[8] = { 2, 13, 4, 8, 11, 10, 9, 16 };

	fp32 Height = 128.0f;
	fp32 HeightHalva = Height * 0.5f;
	fp32 HeightHalvaInv = 1.0f / HeightHalva;

	CRC_Particle lParticles[40];

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);

	int iParticleBatch = 0;
	int nleft = m_nParticles;
	int iParticle = 0;
	while(nleft > 0)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		int np = Min(nleft, 40);
		fp32 zoffs =  Frame + iParticleBatch*6;
		for(int i = 0; i < np; i++)
		{
			fp32 z = M_FMod(ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * Height + zoffs, Height) - HeightHalva;
//			z = z*0.5f + 2.0f*z*z/16.0f;
			fp32 r = M_Cos(z * HeightHalvaInv * _PIHALF) * (1.0f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND])*lRadius[iParticleBatch];
			fp32 offs = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * lSinOffsScale[iParticleBatch] + Frame*(1.0f/27.0f);
			offs += lOffsAdd[iParticleBatch];
			fp32 x = M_Sin(offs + Sqr(M_Fabs(z)) * 0.005f) * r;
			fp32 y = M_Cos(offs + Sqr(M_Fabs(z)) * 0.005f) * r;

			int alpha = Max(0, Min(192, int(Min((HeightHalva + z) * 4.0f, (HeightHalva - z) * 4.0f))));
			alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;

			fp32 sign = (z >= 0.0f) ? HeightHalva : -HeightHalva;
			z = Sqr(z * HeightHalvaInv) * sign;

//			alpha = 0xff000000;

			lParticles[i].m_Pos.k[0] = x;
			lParticles[i].m_Pos.k[1] = y;
			if (bZFlip)
				lParticles[i].m_Pos.k[2] = -z * 0.8f;
			else
				lParticles[i].m_Pos.k[2] = z * 0.8f;

			int c = (ms_lRandomInt[(iRand++) & CXR_PARTICLES_NUMAND]);
			lParticles[i].m_Color = 
				(alpha << 24) + 0x8080ff;

		}
		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], np, lPartSizes[iParticleBatch]))
			_pVBM->AddVB(pVB);
//		_pRender->Render_Particles(&lParticles[0], np, lPartSizes[iParticleBatch]);

		iParticle += np;
		nleft -= np;
		iParticleBatch++;
	}

}
#endif

#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
//  CXR_Model_Particles_Explosion2
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_Explosion2, CXR_Model_ParticleSystem);

CXR_Model_Particles_Explosion2::CXR_Model_Particles_Explosion2()
{
	MAUTOSTRIP(CXR_Model_Particles_Explosion2_ctor, MAUTOSTRIP_VOID);
	m_Radius1 = 12.0f;
	m_Radius2 = 6.0f;
	m_SizeScale = 1.0f;
	m_TotTime = 40.0f;
}

void CXR_Model_Particles_Explosion2::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_Explosion2_RenderParticles, MAUTOSTRIP_VOID);

	fp32 Frame;
	int iRand = 0;
	if (_pAnimState)
	{
		Frame = _pAnimState->m_AnimTime0.GetTime() * 20.0f;
		iRand = _pAnimState->m_Anim0;
	}
	else
		Frame = CMTime::GetCPU().GetTimeModulusScaled(15.0f, 15.0f);

	if (Frame > 40) return;

	fp32 Time_Render = 0;

	CMat4Dfp32 WInv;
	_WMat.InverseOrthogonal(WInv);
	CVec3Dfp32 VUp(WInv.k[2][0], WInv.k[2][1], WInv.k[2][2]);

	int nParticles = 20;

	CRC_Particle lParticles[50];

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);

	int iBatch;
	for(iBatch = 0; iBatch < 3; iBatch++)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		fp32 t = Frame;
		fp32 Drift = 2.0f + fp32(iBatch+1)*m_Radius2*M_Sqrt(Frame/m_TotTime);

		int alpha = Max(0.0f, 0.4f*Min(255.0f - t/m_TotTime*255.0f, 255.0f/*t*16.0f*/));
		alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;

		for(int i = 0; i < nParticles; i++)
		{
			fp32 x = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;
			fp32 y = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;
			fp32 z = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;

			fp32 s = t*0.02f + Sqr(t*(0.06f + ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]*0.03f));

			x += VUp.k[0]*s;
			y += VUp.k[1]*s;
			z += VUp.k[2]*s;

			lParticles[i].m_Pos.k[0] = x;
			lParticles[i].m_Pos.k[1] = y;
			lParticles[i].m_Pos.k[2] = z;

			lParticles[i].m_Color = (alpha << 24) + 0x8080ff;
		}

		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], nParticles, (8.0f + t/m_TotTime*4.0f) * m_SizeScale))
			_pVBM->AddVB(pVB);

//		_pRender->Render_Particles(&lParticles[0], nParticles, (8.0f + t/m_TotTime*4.0f) * m_SizeScale);
	}

	fp32 lPartSizes[8] = { 8, 6, 4, 3, 3, 3, 2, 2 };
//	_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	nParticles = 20;
	for(iBatch = 0; iBatch < 8; iBatch++)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		fp32 t = Frame - iBatch;
		fp32 Drift = 2.0f + fp32(iBatch*0.1f+1.0f)*m_Radius1*M_Sqrt(Frame/m_TotTime);
		int alpha = Max(0.0f, (255.0f - t/m_TotTime*255.0f));
		alpha = (alpha * (255 - m_CurrentFog.GetA())) >> 8;

		for(int i = 0; i < nParticles; i++)
		{
			fp32 v = ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] * 2.0f * _PI;
			fp32 x = M_Sin(v) * Drift;
			fp32 y = M_Cos(v) * Drift;
			fp32 z = 0;
//			fp32 z = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * Drift;

//				y += y*Drift;

/*			x += VUp.k[0]*s;
			y += VUp.k[1]*s;
			z += VUp.k[2]*s;*/

			lParticles[i].m_Pos.k[0] = x;
			lParticles[i].m_Pos.k[1] = y;
			lParticles[i].m_Pos.k[2] = z;

			int glow = Max(0.0f, (255.0f - t*8))*(ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND]);

			lParticles[i].m_Color = 
				((glow >> 2) << 16) + 
				((glow >> 2) << 8) + 
				(glow >> 0) +
				(alpha << 24);
		}

		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], nParticles, m_SizeScale*lPartSizes[iBatch]))
			_pVBM->AddVB(pVB);
//			_pRender->Render_Particles(&lParticles[0], nParticles, m_SizeScale*lPartSizes[iBatch]);
	}

//	LogFile(T_String("Particles", Time) + T_String(" (Eval", Time - Time_Render) + T_String(", Render", Time_Render) + ")");
}
#endif

// -------------------------------------------------------------------
#ifndef M_DISABLE_TODELETE
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_Explosion2b, CXR_Model_Particles_Explosion2);

CXR_Model_Particles_Explosion2b::CXR_Model_Particles_Explosion2b()
{
	MAUTOSTRIP(CXR_Model_Particles_Explosion2b_ctor, MAUTOSTRIP_VOID);
	m_Radius1 = 12.0f*3;
	m_Radius2 = 6.0f*2;
	m_SizeScale = 3.0f;
	m_TotTime = 25.0f;
}
#endif

#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CXR_Model_Particles_Blood
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Particles_Blood, CXR_Model_ParticleSystem);

void CXR_Model_Particles_Blood::RenderParticles(CRenderContext* _pRender, CXR_WorldLightState* _pWLS, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, 
	const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _L2VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Particles_Blood_RenderParticles, MAUTOSTRIP_VOID);
//	return;

	fp32 Frame = 0;

	int iRand = 0;
	if (!_pAnimState) return;

	Frame = _pAnimState->m_AnimTime0.GetTime() * 20.0f;
	iRand = _pAnimState->m_Anim0 & 0xff;
//	int nParticles = (_pAnimState->m_Anim0 >> 8) & 0xff;
	int nParticles = 5;

	if (nParticles <= 0) return;
	if (nParticles > 32) return;
	CRC_Particle lParticles[32];

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if (m_pEngine) m_pEngine->SetDefaultAttrib(pA);

	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_TextureID(0, m_TextureID);

	fp32 size = 3;

	int iParticle = 0;
	for(int iBatch = 0; iBatch < 1; iBatch++)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		if (!pVB) return;
		pVB->m_pAttrib = pA;
		pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;

		fp32 drift = Frame / 20.0f;
		fp32 s = size + drift;

		for(int iP = 0; iP < nParticles; iP++)
		{
			lParticles[iP].m_Pos.k[0] = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * s;
			lParticles[iP].m_Pos.k[1] = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * s;
			lParticles[iP].m_Pos.k[2] = (ms_lRandom[(iRand++) & CXR_PARTICLES_NUMAND] - 0.5f) * s;

			lParticles[iP].m_Color = 0xff600000;
		}
//		_pRender->Render_Particles(&lParticles[0], nParticles, 3);

		if (CXR_Util::Render_Particles(_pVBM, pVB, _L2VMat, &lParticles[0], nParticles, 3))
			_pVBM->AddVB(pVB);
//		_pRender->RenderParticles(&lParticles[0], np, lPartSizes[iParticleBatch]);
	}
};
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_WaterTile
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_WaterTile, CXR_Model);

CXR_Model_WaterTile::CXR_Model_WaterTile()
{
	MAUTOSTRIP(CXR_Model_WaterTile_ctor, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");

	if (!pTC) Error("-", "No texture-context.");

/*	SS: Not used, so why still here?
	int i;
	for(i = 0; i < WATER_DIFFANIMLEN; i++)
		m_DiffTxt[i] = pTC->GetTextureID( CStrF("WATERDIFF%.2X", i) );
	for(i = 0; i < WATER_SPECANIMLEN; i++)
		m_SpecTxt[i] = pTC->GetTextureID( CStrF("WATERSPEC%.2X", i) );
*/

/*	m_SPeriod0 = 0.1;
	m_SPeriod1 = 1.0;
	m_SPeriod2 = -1.0;
	m_TPeriod0 = 1.0;
	m_TPeriod1 = 2.0;
	m_TPeriod2 = 3.0;*/

	fp32 AmpScale = 15.0f;
	m_Amp0 = AmpScale;
//	m_Amp1 = AmpScale;
	m_Amp1 = AmpScale * 0.33f;
	m_Amp2 = AmpScale * 0.2f;
	m_Frequency0 = 0.5f;
	m_Frequency1 = -0.3f;
	m_Frequency2 = 1.0f;

	m_MaxTess = 12;
	m_MinTess = 4;

	m_Scale = 512.0f;
	m_TexScale = 1;
	m_PeriodScale = 2.0f;

	m_SurfaceID = 0;
}

void CXR_Model_WaterTile::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Create, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_WaterTile::OnPostCreate, CUSTOMMODELS);
	// ModelName: WaterTile:Size,Amplitude,Surface,MinTess,MaxTess,Period
	
	if (_pParam)
	{
		CFStr s(_pParam);
		CFStr sScale = s.GetStrSep(",");
		CFStr sAmpScale = s.GetStrSep(",");
		CFStr sSurface = s.GetStrSep(",");
		CFStr sMinTess = s.GetStrSep(",");
		CFStr sMaxTess = s.GetStrSep(",");
		CFStr sPeriod = s.GetStrSep(",");

		if(sScale != "") m_Scale = sScale.Val_fp64();
		if(sAmpScale != "")
		{
			fp32 AmpScale = (fp32)sAmpScale.Val_fp64();
			m_Amp0 = AmpScale;
			m_Amp1 = AmpScale * 0.33f;
			m_Amp2 = AmpScale * 0.2f;
		}
		if(sSurface != "")
		{
			m_Surface = sSurface;
			m_Surface.Trim();
			m_Surface.MakeUpperCase();
		}
		if(sMinTess != "") m_MinTess = sMinTess.Val_fp64();
		if(m_MinTess == CVec2Dfp32(0)) m_MinTess.SetScalar(4.0f);
		if(sMaxTess != "") m_MaxTess = sMaxTess.Val_fp64();
		if(m_MaxTess == CVec2Dfp32(0)) m_MaxTess.SetScalar(12.0f);
		if(sPeriod != "") m_PeriodScale = sPeriod.Val_fp64();
	}

	OnPostCreate();
}

void CXR_Model_WaterTile::OnPostCreate()
{
	MAUTOSTRIP(CXR_Model_WaterTile_OnPostCreate, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_WaterTile::OnPostCreate, CUSTOMMODELS);
	m_ScaleInv[0] = 1.0f / m_Scale[0];
	m_ScaleInv[1] = 1.0f / m_Scale[1];

	m_MinTess[0] = Min(Max(m_MinTess[0], 2.0f), 64.0f);
	m_MinTess[1] = Min(Max(m_MinTess[1], 2.0f), 64.0f);
	m_MaxTess[0] = Min(Max(m_MaxTess[0], m_MinTess[0]), 64.0f);
	m_MaxTess[1] = Min(Max(m_MaxTess[1], m_MinTess[1]), 64.0f);

	m_SPeriod0 = 0.1f * m_PeriodScale[0];
	m_SPeriod1 = 0.65f * m_PeriodScale[0];
	m_SPeriod2 = (-1.0f + 0.31f) * m_PeriodScale[0];
	m_TPeriod0 = 1.0f*0.9f * m_PeriodScale[1];
	m_TPeriod1 = -(2.0f - 0.22f) * m_PeriodScale[1];
	m_TPeriod2 = 3.0f*1.1f * m_PeriodScale[1];

	if (m_Surface == "") m_Surface = "WATERTILE3";

	MRTC_SAFECREATEOBJECT_NOEX(spTSK, "CXW_SurfaceKeyFrame", CXW_SurfaceKeyFrame);
	if (!spTSK) Error("OnPostCreate", "No CXW_SurfaceKeyFrame available.");
	m_spTmpSurfKey = spTSK;

	m_WLS.Create(256, 4, 4);

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfCtx, "SYSTEM.SURFACECONTEXT");
	if (!pSurfCtx) Error("OnPostCreate", "No surface-context.");

	m_SurfaceID = pSurfCtx->GetSurfaceID(m_Surface);
	if (!m_SurfaceID)
		ConOutL("§cf80WARNING: (CXR_Model_WaterTile::OnPostCreate) Undefined surface " + m_Surface);

	m_MediumFlags = pSurfCtx->GetSurface(m_SurfaceID)->GetBaseFrame()->m_Medium.m_MediumFlags;
	m_MediumFlags |= XW_MEDIUM_WATER;

/*LogFile(CStrF("(CXR_Model_WaterTile::OnPostCreate) Surface %s, Size %s, TexScale %s, Period %s, MinTess %s, MaxTess %s, ", 
		m_Surface.Str(), m_Scale.GetString().Str(), m_TexScale.GetString().Str(), m_PeriodScale.GetString().Str(), 
		m_MinTess.GetString().Str(), m_MaxTess.GetString().Str() ));*/
}

void CXR_Model_WaterTile::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_WaterTile_OnPrecache, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfCtx, "SYSTEM.SURFACECONTEXT");
	if (!pSurfCtx) Error("OnPrecache", "No surface-context.");

	m_SurfaceID = pSurfCtx->GetSurfaceID(m_Surface);
	if (!m_SurfaceID)
		ConOutL("§cf80WARNING: (CXR_Model_WaterTile::OnPrecache) Undefined surface " + m_Surface);
	else
	{
		CXW_Surface *pSurf = pSurfCtx->GetSurface(m_SurfaceID);
		pSurf = pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
		if(pSurf)
			pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}
}


fp32 CXR_Model_WaterTile::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_WaterTile_GetBound_Sphere, 0.0f);
	fp32 zMax = 16.0f * 2.5f + 8.0f;
	return Length3(m_Scale[0]*0.5f,m_Scale[1]*0.5f,zMax);
}

void CXR_Model_WaterTile::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_WaterTile_GetBound_Box, MAUTOSTRIP_VOID);
	fp32 zMax = 16.0f * 2.5f + 8.0f;
	_Box.m_Min.k[0] = -m_Scale[0]*0.5f;
	_Box.m_Min.k[1] = -m_Scale[1]*0.5f;
	_Box.m_Min.k[2] = -zMax;
	_Box.m_Max.k[0] = m_Scale[0]*0.5f;
	_Box.m_Max.k[1] = m_Scale[1]*0.5f;
	_Box.m_Max.k[2] = zMax;
}

void RenderNormals(CRenderContext* _pRender, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, fp32 _Len, CPixel32 _Color);
void RenderNormals(CRenderContext* _pRender, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, fp32 _Len, CPixel32 _Color)
{
	MAUTOSTRIP(RenderNormals, MAUTOSTRIP_VOID);
	for(int v = 0; v < _nV; v++)
		_pRender->Render_Wire(_pV[v], _pV[v] + _pN[v]*_Len, _Color);
}

// Prototype for function in MImage.cpp
void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_FlipBGR_RGBA32(int _nV, CPixel32* _pCol);
void PPA_FlipBGR_RGBA32(int _nV, CPixel32* _pCol)
{
	MAUTOSTRIP(PPA_FlipBGR_RGBA32, MAUTOSTRIP_VOID);
#ifdef CPU_INTELP5
	__asm
	{
		mov ecx, _nV
		mov esi, [_pCol];
Lp:
		mov eax, [esi]
		bswap eax
		ror eax, 8
		mov [esi], eax
		dec ecx
		lea esi, [esi+4]
		jnz Lp
	}
#else
	// {b,g,r,a} -> {r,g,b,a}
	CPixel32 TmpCol;
	for (int i = 0 ; i < _nV ; i++)
	{
		uint8 b = _pCol[i].GetB(); 
		_pCol[i].B() = _pCol[i].GetR();
		_pCol[i].R() = b;
	}
#endif
}

#define WATERTILE_QSIN
//#define WATERTILE_TRANSFORM
#define WATERTILE_TRANSFORM_WORLD

void CXR_Model_WaterTile::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_WaterTile_PreRender, MAUTOSTRIP_VOID);
	if (!_pAnimState) return;

	const_cast<CXR_AnimState*>(_pAnimState)->m_Data[0] = -1;
	const_cast<CXR_AnimState*>(_pAnimState)->m_Data[1] = -1;

/*	m_Amp0 = 0;
	m_Amp1 = 0;
	m_Amp2 = 0;*/

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfCtx, "SYSTEM.SURFACECONTEXT");
	if (!pSurfCtx) return;

	int RenderSurfOptions = (_pEngine) ? _pEngine->GetVar(XR_ENGINE_SURFOPTIONS) : 0;
	int RenderSurfCaps = (_pEngine) ? _pEngine->GetVar(XR_ENGINE_SURFCAPS) : 0;

	CXW_Surface* pSurf = pSurfCtx->GetSurface(m_SurfaceID)->GetSurface(RenderSurfOptions, RenderSurfCaps);

	if (pSurf->m_Flags & (XW_SURFFLAGS_NEEDREFLECTIONMAP | XW_SURFFLAGS_NEEDREFRACTIONMAP))
	{
		fp32 SizeX = m_Scale[0]*0.5f;
		fp32 SizeY = m_Scale[1]*0.5f;
		CVec3Dfp32 Verts[32];
		Verts[0] = CVec3Dfp32(SizeX, -SizeY, 0);
		Verts[1] = CVec3Dfp32(-SizeX, -SizeY, 0);
		Verts[2] = CVec3Dfp32(-SizeX, SizeY, 0);
		Verts[3] = CVec3Dfp32(SizeX, SizeY, 0);
		for(int i = 0; i < 4; i++) Verts[i] *= _WMat;

		if (_pViewClip)
		{
			// Get bound-box, check visibility.
			CVec3Dfp32 ObjWPos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
			CBox3Dfp32 Box;
			GetBound_Box(Box, _pAnimState);
			CBox3Dfp32 BoxW;
			Box.Transform(_WMat, BoxW);
			if (!_pViewClip->View_GetClip_Box(BoxW.m_Min, BoxW.m_Max, 0, 0, NULL, NULL)) return;
		}

		if (pSurf->m_Flags & XW_SURFFLAGS_NEEDREFLECTIONMAP)
			const_cast<CXR_AnimState*>(_pAnimState)->m_Data[0] = _pEngine->Render_AddMirror_Texture(Verts, 4, _WMat, _VMat);

		CMat4Dfp32 Mat;
		Mat.Unit();
	//	_pEngine->GetVC()->m_CameraWMat.InverseOrthogonal(Mat);
		if (pSurf->m_Flags & XW_SURFFLAGS_NEEDREFRACTIONMAP)
			const_cast<CXR_AnimState*>(_pAnimState)->m_Data[1] = _pEngine->Render_AddPortal_Texture(Verts, 4, Mat, _WMat, _VMat);
	}
}


void CXR_Model_WaterTile::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_WaterTile_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_WaterTile::OnRender);
	if (!_pRender) return;
	if (!_pAnimState) return;
	CMat4Dfp32 CurL2VMat, CurV2LMat;
	_WMat.Multiply(_VMat, CurL2VMat);
	CurL2VMat.InverseOrthogonal(CurV2LMat);

//	CVec3Dfp32::GetMatrixRow(CurV2LMat, 3) = CVec3Dfp32::GetMatrixRow(_VMat, 3) - CVec3Dfp32::GetMatrixRow(_WMat, 3);

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfCtx, "SYSTEM.SURFACECONTEXT");
	if (!pSurfCtx) return;

	int RenderSurfOptions = (_pEngine) ? _pEngine->GetVar(XR_ENGINE_SURFOPTIONS) : 0;
	int RenderSurfCaps = (_pEngine) ? _pEngine->GetVar(XR_ENGINE_SURFCAPS) : 0;

	fp32 FlipMul = 1.0f;
	if (_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP)
		FlipMul = -1.0f;

	CXW_Surface* pSurf = pSurfCtx->GetSurface(m_SurfaceID)->GetSurface(RenderSurfOptions, RenderSurfCaps);

	CXR_Portal* pPReflect = _pEngine->GetPortal(_pAnimState->m_Data[0]);
	CXR_Portal* pPRefract = _pEngine->GetPortal(_pAnimState->m_Data[1]);
	if (!pPReflect && (pSurf->m_Flags & XW_SURFFLAGS_NEEDREFLECTIONMAP)) return;
	if (!pPRefract && (pSurf->m_Flags & XW_SURFFLAGS_NEEDREFRACTIONMAP)) return;

	if (pPReflect && !pPReflect->IsTexture()) return;
	if (pPRefract && !pPRefract->IsTexture()) return;

	CXR_RenderInfo RenderInfo(_pEngine);


	// Note: This visibility test assumes the watertile to be unrotated. 
	// Rotation by even 90 degrees along Z-axis is permitted.
	CVec3Dfp32 ObjWPos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	if (_pViewClip)
	{
		// Get bound-box, check visibility.
		CBox3Dfp32 Box;
		GetBound_Box(Box, _pAnimState);
		CBox3Dfp32 BoxW;
		Box.Transform(_WMat, BoxW);
		if (!_pViewClip->View_GetClip_Box(BoxW.m_Min, BoxW.m_Max, 0, 0, NULL, &RenderInfo)) return;
	}
	else
	{
	}

	CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(0, _pAnimState->m_AnimTime0, m_spTmpSurfKey);

	int bLighting = pSurf->m_Flags & XW_SURFFLAGS_LIGHTING;
	int bTangents = pSurf->m_Flags & XW_SURFFLAGS_NEEDTANGENTS;

// FIXME:
//bLighting = 0;
	int bTexturePortal = 1;

	// Get lights.
	if (bLighting)
	{
		m_WLS.PrepareFrame();
		if (_spWLS)
		{
			m_WLS.CopyAndCull(_spWLS, GetBound_Sphere(), CVec3Dfp32::GetMatrixRow(_WMat, 3), 4, 3);
			//m_WLS.AddLightVolume(RenderInfo.m_pLightVolume, CVec3Dfp32::GetMatrixRow(_WMat, 3));
		}

		// Transform lights
		{
	#ifdef WATERTILE_TRANSFORM_WORLD
			m_WLS.Optimize(CVec3Dfp32::GetMatrixRow(_WMat, 3), GetBound_Sphere(), 0.8f, NULL);

	#elif defined WATERTILE_TRANSFORM
			m_WLS.Optimize(CVec3Dfp32::GetMatrixRow(_WMat, 3), GetBound_Sphere(), 0.8f, &_VMat);

	#else
			CMat4Dfp32 LTransform;
			_WMat.InverseOrthogonal(LTransform);
			m_WLS.Optimize(CVec3Dfp32::GetMatrixRow(_WMat, 3), GetBound_Sphere(), 0.8f, &LTransform);

	#endif
		}
	}

	CXR_FogState* pFog = (_pEngine) ? _pEngine->m_pCurrentFogState : NULL;

// URGENTFIXME: Fix da' damn fog!, will ya?
//bool bFog = false;
	
//	const CVec3Dfp32& p = CVec3Dfp32::GetMatrixRow(_WMat, 3);


	fp32 SizeX = m_Scale[0]*0.5f;
	fp32 SizeY = m_Scale[1]*0.5f;
	CVec3Dfp32 EdgeY1(SizeX, 0, 0);
	CVec3Dfp32 EdgeY2(-SizeX, 0, 0);
	CVec3Dfp32 EdgeX1(0, -SizeY, 0);
	CVec3Dfp32 EdgeX2(0, SizeY, 0);
	EdgeX1 *= CurL2VMat;
	EdgeX2 *= CurL2VMat;
	EdgeY1 *= CurL2VMat;
	EdgeY2 *= CurL2VMat;

	fp32 SubLen = Length3(SizeX,SizeY,32.0f);
	fp32 LenX1 = Max(0.0f, EdgeX1.Length() - SubLen);
	fp32 LenX2 = Max(0.0f, EdgeX2.Length() - SubLen);
	fp32 LenY1 = Max(0.0f, EdgeY1.Length() - SubLen);
	fp32 LenY2 = Max(0.0f, EdgeY2.Length() - SubLen);

	fp32 fnS1 = Max(m_MinTess[0], m_MaxTess[0] - 1.0f - (LenX1 * (1.0f / 64.0f)));
	fp32 fnS2 = Max(m_MinTess[0], m_MaxTess[0] - 1.0f - (LenX2 * (1.0f / 64.0f)));
	fp32 fnT1 = Max(m_MinTess[1], m_MaxTess[1] - 1.0f - (LenY1 * (1.0f / 64.0f)));
	fp32 fnT2 = Max(m_MinTess[1], m_MaxTess[1] - 1.0f - (LenY2 * (1.0f / 64.0f)));
/*fnS1 = MaxV-1.0f;
fnS2 = MaxV-1.0f;
fnT1 = MaxV-1.0f;
fnT2 = MaxV-1.0f;*/

	CMTime t = CMTime() - _pAnimState->m_AnimTime0; //  (1.5f)
	fp32 Phase0 = t.GetTimeModulusScaled(m_Frequency0 * 1.5f, 1.0f) + (ObjWPos[0]*m_ScaleInv[0]*m_SPeriod0 + ObjWPos[1]*m_ScaleInv[1]*m_TPeriod0);
	fp32 Phase1 = t.GetTimeModulusScaled(m_Frequency1 * 1.5f, 1.0f) + (ObjWPos[0]*m_ScaleInv[0]*m_SPeriod1 + ObjWPos[1]*m_ScaleInv[1]*m_TPeriod1);
	fp32 Phase2 = t.GetTimeModulusScaled(m_Frequency2 * 1.5f, 1.0f) + (ObjWPos[0]*m_ScaleInv[0]*m_SPeriod2 + ObjWPos[1]*m_ScaleInv[1]*m_TPeriod2);

	fp32 sStep1 = 1.0f/fnS1;
	fp32 sStep2 = 1.0f/fnS2;
	fp32 tStep1 = 1.0f/fnT1;
	fp32 tStep2 = 1.0f/fnT2;
	int _nS = (int)(M_Floor(Max(fnS1, fnS2) - 0.001f) + 1);
	int _nT = (int)(M_Floor(Max(fnT1, fnT2) - 0.001f) + 1);

	_nS++;
	_nT++;
//	if (_nS*_nT > Sqr(MaxV)) return;

	int nX = _nS;
	int nY = _nT;
	int nV = nX*nY;

//	CVec3Dfp32 NVerts[MaxV*MaxV];
	CVec3Dfp32* NVerts = _pVBM->Alloc_V3(nV);
	CVec3Dfp32* pVerts = _pVBM->Alloc_V3(nV);
	CVec2Dfp32* pTVerts = _pVBM->Alloc_V2(nV);

	CVec3Dfp32* pTangU = (bTangents) ? _pVBM->Alloc_V3(nV) : NULL;
	CVec3Dfp32* pTangV = (bTangents) ? _pVBM->Alloc_V3(nV) : NULL;
	if (bTangents && (!pTangU || !pTangV)) return;

//ConOut(CStrF("WaterTile Arrays %.8x, %.8x, %.8x", pVerts, NVerts, pTVerts));
//	CVec2Dfp32* pTVerts2 = _pVBM->Alloc_V2(nV);
	CPixel32* pLVerts = NULL;
	CPixel32* pSpecVerts = NULL;

	if (!pVerts || !pTVerts/* || !pTVerts2*/) return;

	int bNormals = bTexturePortal || bLighting || (pSurf->m_Flags & XW_SURFFLAGS_NEEDNORMALS);

	int CntS = _nS;
	int SStep, TStep, s;
	fp32 fS1;
	fp32 fS2;
	
	SStep = 1;
	s = 0;
	fS1 = 0.0f;
	fS2 = 0.0f;

	TStep = 1;

	// Tesselation loops
	while(CntS--)
	{
		fp32 fT1;
		fp32 fT2;
		int t;
		t = 0;
		fT1 = 0.0f;
		fT2 = 0.0f;

		int CntT = _nT;
		while(CntT--)
		{
			int iv = t*_nS + s;

			fp32 ds = fS2-fS1;
			fp32 dt = fT1-fT2;
			fp32 fX = (fS1 + fT2*ds) / (1.0f - ds*dt);
			fp32 fY = (fT2 + fX*dt);

			pTVerts[iv].k[0] = fX * m_TexScale[0];
			pTVerts[iv].k[1] = fY * m_TexScale[1];
			pVerts[iv].k[0] = (fX-0.5f) * m_Scale[0];
			pVerts[iv].k[1] = (fY-0.5f) * m_Scale[1];

#ifdef WATERTILE_QSIN
			CVec3Dfp32 Angles;
			int iAngles[3];

			Angles[0] = (Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*_PI2;
			Angles[1] = (Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*_PI2;
			Angles[2] = (Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*_PI2;
			Angles *= 65536.0f* (1.0f/_PI2);
			iAngles[0] = RoundToInt(Angles[0]);
			iAngles[1] = RoundToInt(Angles[1]);
			iAngles[2] = RoundToInt(Angles[2]);
//			RoundToInt(Angles.k, iAngles, 3);

			fp32 sina0, sina1, sina2, cosa0, cosa1, cosa2;
			QSinCosi(iAngles[0], sina0, cosa0);
			QSinCosi(iAngles[1], sina1, cosa1);
			QSinCosi(iAngles[2], sina2, cosa2);

			pVerts[iv].k[2] =
				m_Amp0 * sina0 +
				m_Amp1 * sina1 +
				m_Amp2 * sina2;

	#ifdef WATERTILE_TRANSFORM_WORLD
			pVerts[iv].MultiplyMatrix(_WMat);
	#elif defined WATERTILE_TRANSFORM
			pVerts[iv].MultiplyMatrix(CurL2VMat);
	#endif

			if (bNormals)
			{
				fp32 dX =
					(_PI2*m_SPeriod0 * m_Amp0 * cosa0 +
					_PI2*m_SPeriod1 * m_Amp1 * cosa1 +
					_PI2*m_SPeriod2 * m_Amp2 * cosa2) * m_ScaleInv[0];
				fp32 dY =
					(_PI2*m_TPeriod0 * m_Amp0 * cosa0 +
					_PI2*m_TPeriod1 * m_Amp1 * cosa1 +
					_PI2*m_TPeriod2 * m_Amp2 * cosa2) * m_ScaleInv[1];

#else
			pVerts[iv].k[2] =
				m_Amp0 * M_Sin((Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*_PI2) +
				m_Amp1 * M_Sin((Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*_PI2) +
				m_Amp2 * M_Sin((Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*_PI2);

	#ifdef WATERTILE_TRANSFORM_WORLD
			pVerts[iv].MultiplyMatrix(_WMat);
	#elif defined WATERTILE_TRANSFORM
			pVerts[iv].MultiplyMatrix(CurL2VMat);
	#endif

			if (bNormals)
			{
				// Calculate dX/dS and dY/dT differentials for normal. (yeah!, finally some use for multi-variable analysis.  :) )
				fp32 dX =
					(_PI2*m_SPeriod0 * m_Amp0 * M_Cos((Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*_PI2) +
					_PI2*m_SPeriod1 * m_Amp1 * M_Cos((Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*_PI2) +
					_PI2*m_SPeriod2 * m_Amp2 * M_Cos((Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*_PI2)) * m_ScaleInv[0];
				fp32 dY =
					(_PI2*m_TPeriod0 * m_Amp0 * M_Cos((Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*_PI2) +
					_PI2*m_TPeriod1 * m_Amp1 * M_Cos((Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*_PI2) +
					_PI2*m_TPeriod2 * m_Amp2 * M_Cos((Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*_PI2)) * m_ScaleInv[1];
#endif

				fp32 LenSqr = Sqr(dX) + Sqr(dY) + 1.0f;
				fp32 LenInv = M_InvSqrt(LenSqr);
				NVerts[iv].k[0] = -dX*LenInv;
				NVerts[iv].k[1] = -dY*LenInv;
				NVerts[iv].k[2] = LenInv;

	#ifdef WATERTILE_TRANSFORM_WORLD
				NVerts[iv].MultiplyMatrix3x3(_WMat);
	#elif defined WATERTILE_TRANSFORM
				NVerts[iv].MultiplyMatrix3x3(CurL2VMat);
	#endif
				if (bTangents)
				{
					fp32 LenSqrU = Sqr(dX) + 1.0f;
					fp32 LenURecp = M_InvSqrt(LenSqrU);
					pTangU[iv].k[0] = 0;
					pTangU[iv].k[1] = LenURecp;
					pTangU[iv].k[2] = dX * LenURecp;
					fp32 LenSqrV = Sqr(dY) + 1.0f;
					fp32 LenVRecp = M_InvSqrt(LenSqrV);
					pTangV[iv].k[0] = LenVRecp;
					pTangV[iv].k[1] = 0;
					pTangV[iv].k[2] = dY * LenVRecp;

		#ifdef WATERTILE_TRANSFORM_WORLD
					pTangU[iv].MultiplyMatrix3x3(_WMat);
					pTangV[iv].MultiplyMatrix3x3(_WMat);
		#elif defined WATERTILE_TRANSFORM
					pTangU[iv].MultiplyMatrix3x3(CurL2VMat);
					pTangV[iv].MultiplyMatrix3x3(CurL2VMat);
		#endif
				}
			}


			fT1 = Clamp01(fT1 + tStep1);
			fT2 = Clamp01(fT2 + tStep2);
			t += TStep;
		}
		fS1 = Clamp01(fS1 + sStep1);
		fS2 = Clamp01(fS2 + sStep2);
		s += SStep;
	}

	// Perform lighting if needed.
	fp32 LightScale = (_pEngine) ? _pEngine->m_LightScale : 1.0f;
	if (bLighting && _spWLS)
	{
		pLVerts = _pVBM->Alloc_CPixel32(nV);
		pSpecVerts = _pVBM->Alloc_CPixel32(nV);
		if (!pLVerts || !pSpecVerts) return;

		CXR_WorldLightState::LightDiffuse(m_WLS.GetFirst(), nV, pVerts, NVerts, 0, pLVerts, 255, LightScale);
		// PPA_Mul_RGBA32(0xff8f8f9f, pLVerts, pLVerts, nV);
		//	PPA_FlipBGR_RGBA32(nX*nY, LVerts);

#ifdef WATERTILE_TRANSFORM_WORLD
		// Fixme:
		CXR_WorldLightState::LightSpecular(m_WLS.GetFirst(), nV, pVerts, NVerts, 2, pSpecVerts, 0, 255, LightScale*1.5f);
#elif defined WATERTILE_TRANSFORM
		CXR_WorldLightState::LightSpecular(m_WLS.GetFirst(), nV, pVerts, NVerts, 2, pSpecVerts, 0, 255, LightScale*1.5f);
#else
		CXR_WorldLightState::LightSpecular(m_WLS.GetFirst(), nV, pVerts, NVerts, 2, pSpecVerts, CVec3Dfp32::GetMatrixRow(CurV2LMat, 3), 255, LightScale*1.5f);
#endif
		//	PPA_FlipBGR_RGBA32(nX*nY, SpecVerts);
	}	

	// Build primitive-batch
	int nPrim = (nY-1)*(2 + nX*2) + 1;
	uint16* pPrim = _pVBM->Alloc_Int16(nPrim);
	if (!pPrim) return;

//	const int MaxPrim = 2048;
//	uint16 Prim[MaxPrim];

	int iP = 0;
	{
		for(int t = 0; t < nY-1; t++)
		{
//			if (iP > MaxPrim-nX*2-8) break;
			pPrim[iP++] = CRC_RIP_TRISTRIP + ((nX*2+2) << 8);
			pPrim[iP++] = nX*2;
			for(int u = 0; u < nX; u++)
			{
				pPrim[iP++] = t*nX + u;
				pPrim[iP++] = (t+1)*nX + u;
			}
		}
		pPrim[iP++] = CRC_RIP_END + (1 << 8);
	}

	if (iP > nPrim) Error("Render", "Internal error.");

	CXR_VertexBuffer VB;
	if (!VB.AllocVBChain(_pVBM, false))
		return;
	VB.Geometry_VertexArray(pVerts, nV, true);
	VB.Geometry_TVertexArray(pTVerts, 0);
//	VB.Geometry_TVertexArray(pTVRefl, 1);
//	VB.Geometry_TVertexArray(pTVRefr, 2);
	VB.Render_IndexedPrimitives(pPrim, iP);
	CXR_VBChain *pChain = VB.GetVBChain();

	// Create texture coordinates for geometry projected on the reflection/refraction textures.
	bool bProjectiveTV = true;
	
	if (bProjectiveTV)
	{
		// -------------------------------------------------------------------
		//  PROJECTIVE TEXTURING
		// -------------------------------------------------------------------
		CVec4Dfp32* pTVRefl = NULL;
		CVec4Dfp32* pTVRefr = NULL;
		if (pPReflect)
		{
			pTVRefl = _pVBM->Alloc_V4(nV);
			if (!pTVRefl) return;
		}

		if (pPRefract)
		{
			pTVRefr = _pVBM->Alloc_V4(nV);
			if (!pTVRefr) return;
		}

		CPixel32* pColRef = _pVBM->Alloc_CPixel32(nV);
		if (!pColRef) return;

		fp32 Scale = 0.5f;

		if (pPReflect || pPRefract)
		{
			CVec3Dfp32 N(0,0,1);
	#ifdef WATERTILE_TRANSFORM_WORLD
			N.MultiplyMatrix3x3(_WMat);
	#elif defined WATERTILE_TRANSFORM
			N.MultiplyMatrix3x3(CurL2VMat);
	#endif
//			fp32* pVZRecp = (fp32*) _pVBM->Alloc_Int32(nV);

			if (pPReflect)
			{
				fp32 Distort = 0.1f * 32.0f;
				fp32 xScale = pPReflect->m_Viewport.GetScale() * 0.5 * Scale;
				fp32 yScale = xScale * pPReflect->m_Viewport.GetAspectRatio();

				for(int v = 0; v < nV; v++)
				{
					CVec3Dfp32 V = pVerts[v];
	#ifdef WATERTILE_TRANSFORM_WORLD
					V *= _VMat;
	#elif defined WATERTILE_TRANSFORM
					V *= CurL2VMat;
	#endif
	//				pVZRecp[v] = 1.0f / V[2];

					pTVRefl[v][0] = V[0] /* pVZRecp[v] */ * xScale + 0.5f*V[2];
					pTVRefl[v][1] = FlipMul * V[1] /* pVZRecp[v] */ * yScale + 0.5f*V[2];
					pTVRefl[v][2] = 0;
					pTVRefl[v][3] = V[2];

					CVec3Dfp32 dN;
					NVerts[v].Sub(N, dN);

					pTVRefl[v][0] += dN[0] * Distort;// * pVZRecp[v];
					pTVRefl[v][1] += FlipMul * dN[1] * Distort;// * pVZRecp[v];
				}
			}

			if (pPRefract)
			{
				fp32 Distort = 0.4f * 32.0f;
				fp32 xScale = pPRefract->m_Viewport.GetScale() * 0.5 * Scale;
				fp32 yScale = xScale * pPRefract->m_Viewport.GetAspectRatio();

				for(int v = 0; v < nV; v++)
				{
					CVec3Dfp32 V = pVerts[v];
		#ifdef WATERTILE_TRANSFORM_WORLD
					V *= _VMat;
		#elif defined WATERTILE_TRANSFORM
					V *= CurL2VMat;
		#endif
					pTVRefr[v][0] = V[0] /* pVZRecp[v] */ * xScale + 0.5f*V[2];
					pTVRefr[v][1] = FlipMul * V[1] /* pVZRecp[v] */ * yScale + 0.5f*V[2];
					pTVRefr[v][2] = 0;
					pTVRefr[v][3] = V[2];


					CVec3Dfp32 dN;
					NVerts[v].Sub(N, dN);

					pTVRefr[v][0] -= dN[0] * Distort;// * pVZRecp[v];
					pTVRefr[v][1] -= FlipMul * dN[1] * Distort;// * pVZRecp[v];

					V.Normalize();
					fp32 dot = V*NVerts[v];
					
					int Alpha = RoundToInt((Clamp01(1.0f - M_Fabs(dot)))*255.0f);
					pColRef[v] = 0xffffff + (Alpha << 24);
				}
			}
		}
		pChain->m_pTV[1] = (fp32*)pTVRefl;	pChain->m_nTVComp[1] = 4;
		pChain->m_pTV[2] = (fp32*)pTVRefr;	pChain->m_nTVComp[2] = 4;
	}
	else
	{
		// -------------------------------------------------------------------
		//  2D TEXTURING
		// -------------------------------------------------------------------
		CVec2Dfp32* pTVRefl = NULL;
		CVec2Dfp32* pTVRefr = NULL;
		if (pPReflect)
		{
			pTVRefl = _pVBM->Alloc_V2(nV);
			if (!pTVRefl) return;
		}

		if (pPRefract)
		{
			pTVRefr = _pVBM->Alloc_V2(nV);
			if (!pTVRefr) return;
		}

		CPixel32* pColRef = _pVBM->Alloc_CPixel32(nV);
		if (!pColRef) return;

		if (pPReflect || pPRefract)
		{
			CVec3Dfp32 N(0,0,1);
	#ifdef WATERTILE_TRANSFORM_WORLD
			N.MultiplyMatrix3x3(_WMat);
	#elif defined WATERTILE_TRANSFORM
			N.MultiplyMatrix3x3(CurL2VMat);
	#endif
			fp32* pVZRecp = (fp32*) _pVBM->Alloc_Int32(nV);

			if (pPReflect)
			{
				fp32 Distort = 0.1f * 32.0f;
				fp32 xScale = pPReflect->m_Viewport.GetScale() * 0.25f;
				fp32 yScale = xScale * pPReflect->m_Viewport.GetAspectRatio();

				for(int v = 0; v < nV; v++)
				{
					CVec3Dfp32 V = pVerts[v];
	#ifdef WATERTILE_TRANSFORM_WORLD
					V *= _VMat;
	#elif defined WATERTILE_TRANSFORM
					V *= CurL2VMat;
	#endif
					pVZRecp[v] = 1.0f / V[2];

					pTVRefl[v][0] = V[0] * pVZRecp[v] * xScale + 0.5f;
					pTVRefl[v][1] = FlipMul * V[1] * pVZRecp[v] * yScale + 0.5f;

					CVec3Dfp32 dN;
					NVerts[v].Sub(N, dN);

					pTVRefl[v][0] += dN[0] * Distort * pVZRecp[v];
					pTVRefl[v][1] += FlipMul * dN[1] * Distort * pVZRecp[v];
				}
			}

			if (pPRefract)
			{
				fp32 Distort = 0.4f * 32.0f;
				fp32 xScale = pPRefract->m_Viewport.GetScale() * 0.25f;
				fp32 yScale = xScale * pPRefract->m_Viewport.GetAspectRatio();

				for(int v = 0; v < nV; v++)
				{
					CVec3Dfp32 V = pVerts[v];
		#ifdef WATERTILE_TRANSFORM_WORLD
					V *= _VMat;
		#elif defined WATERTILE_TRANSFORM
					V *= CurL2VMat;
		#endif
					pTVRefr[v][0] = V[0] * pVZRecp[v] * xScale + 0.5f;
					pTVRefr[v][1] = FlipMul * V[1] * pVZRecp[v]* yScale + 0.5f;


					CVec3Dfp32 dN;
					NVerts[v].Sub(N, dN);

					pTVRefr[v][0] -= dN[0] * Distort * pVZRecp[v];
					pTVRefr[v][1] -= FlipMul * dN[1] * Distort * pVZRecp[v];

					V.Normalize();
					fp32 dot = V*NVerts[v];
					
					int Alpha = RoundToInt((Clamp01(1.0f - M_Fabs(dot)))*255.0f);
					pColRef[v] = 0xffffff + (Alpha << 24);
				}
			}
		}
		pChain->m_pTV[1] = (fp32*)pTVRefl;	pChain->m_nTVComp[1] = 2;
		pChain->m_pTV[2] = (fp32*)pTVRefr;	pChain->m_nTVComp[2] = 2;
	}

	// Create vertex-buffer
#ifdef WATERTILE_TRANSFORM_WORLD
	CMat4Dfp32* pVBMat = _pVBM->Alloc_M4(_VMat);
	if (!pVBMat) return;

#elif defined WATERTILE_TRANSFORM
	CMat4Dfp32* pVBMat = NULL;

#else
	CMat4Dfp32* pVBMat = _pVBM->Alloc_M4(CurL2VMat);
	if (!pVBMat) return;
#endif

	int Flags = RENDERSURFACE_MATRIXSTATIC_M2V;// | RENDERSURFACE_VERTEXFOG;

	const fp32 PriorityOffset = 0.001f;
	CMat4Dfp32 VMatInv;
	_VMat.InverseOrthogonal(VMatInv);
	pChain->m_pN = (bNormals) ? NVerts : NULL;
	pChain->m_pCol = pLVerts;
	pChain->m_pSpec = pSpecVerts;

	if (bTangents)
	{
		if (!pChain->m_pTV[1])
			VB.Geometry_TVertexArray(pTangU, 1);
		VB.Geometry_TVertexArray(pTangU, 2);
		VB.Geometry_TVertexArray(pTangV, 3);
	}

	CXR_RenderSurfExtParam Params;
	Params.m_TextureIDReflectionMap = (pPReflect) ? pPReflect->m_TextureID : 0;
	Params.m_TextureIDRefractionMap = (pPRefract) ? pPRefract->m_TextureID : 0;

	fp32 Priority = (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT) ? 
		RenderInfo.m_BasePriority_Transparent :
		RenderInfo.m_BasePriority_Opaque;

	{
//		for(int i = 0; i < 32; i++)
		CMat4Dfp32 M2W;
		M2W.Unit();

		CXR_Util::Render_Surface(Flags, _pAnimState->m_AnimTime0, pSurf, pSurfKey, _pEngine, _pVBM, &M2W, &_VMat, pVBMat, &VB, 
			Priority, PriorityOffset, &Params);
	}

	// Vertex-fog
/*	if (pFog && pFog->VertexFog())
	{
		CMat4Dfp32 VMatInv;
		_VMat.InverseOrthogonal(VMatInv);
		pFog->SetTransform(&VMatInv);

		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;
		pVB->Geometry_VertexArray(pVerts, nV);
		CRC_Attributes* pA = pVB->m_pAttrib;
		if (_pEngine) _pEngine->SetDefaultAttrib(pA);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE);

		pVB->m_Priority = RenderInfo.m_BasePriority_Transparent + 0.0002f;
		pVB->Matrix_Set(pVBMat);
		pVB->Render_IndexedPrimitives(pPrim, iP);

		if (pFog->VertexFog_Eval(_pVBM, pVB, true))
		{
//			ConOut("Water fog.");
			_pVBM->AddVB(pVB);
		}
		else
			return;
	}
*/

	// NHF VB
	if (pFog && (RenderInfo.m_Flags & CXR_RENDERINFO_NHF))
	{
		CMat4Dfp32 VMatInv;
#ifdef WATERTILE_TRANSFORM
		_VMat.InverseOrthogonal(VMatInv);
		pFog->SetTransform(&VMatInv);
#else
		pFog->SetTransform(&_WMat);
#endif

		CBox3Dfp32 Box;
		GetBound_Box(Box, _pAnimState);
		Box.m_Min += ObjWPos;
		Box.m_Max += ObjWPos;

		pFog->TraceBound(Box);

		CPixel32* pFogCol = _pVBM->Alloc_CPixel32(nV);
		CVec2Dfp32* pFogUV = _pVBM->Alloc_V2(nV);
		if (!pFog || !pFogUV) return;

		if (pFog->Trace(Box, nV, pVerts, pFogCol, pFogUV, false))
		{
			CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
			if (!pVB) return;
			CRC_Attributes* pA = _pVBM->Alloc_Attrib();
			if (!pA) return;
			pA->SetDefault();
			if (_pEngine) _pEngine->SetDefaultAttrib(pA);
			pVB->m_pAttrib = pA;
			pVB->Matrix_Set(pVBMat);

			pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE);
			pA->Attrib_TextureID(0, pFog->m_FogTableTextureID);

			if (!pVB->AllocVBChain(_pVBM, false))
				return;
			pVB->Geometry_VertexArray(pVerts, nV, true);
			pVB->Geometry_TVertexArray(pFogUV, 0);
			pVB->Geometry_ColorArray(pFogCol);

			pVB->Render_IndexedPrimitives(pPrim, iP);
			pVB->m_Priority = Priority + PriorityOffset * CXR_VBPRIORITY_VOLUMETRICFOG;
			_pVBM->AddVB(pVB);
		}

		pFog->TraceBoundRelease();
	}


/*	_pRender->Matrix_PushMultiply(CurL2VMat);
	_pRender->Attrib_Push();

		// Render diffuse
		_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		_pRender->Attrib_Disable(CRC_FLAGS_CULL);
		_pRender->Geometry_Color(0xffffffff);
		_pRender->Geometry_VertexArray(Verts, nV);
		_pRender->Geometry_TVertexArray(TVerts, 0);

		_pRender->Attrib_TextureID(0, m_DiffTxt[(iTxt1 & 0x7fff) % WATER_DIFFANIMLEN]);

		if (_pWLS) 
			_pRender->Geometry_ColorArray(LVerts);
		else
		{
			_pRender->Geometry_ColorArray(NULL);
			_pRender->Geometry_Color(0xa08f8f9f);
		}

		if (pFog) pFog->SetDepthFog(_pRender, 0, CRC_RASTERMODE_ALPHABLEND);
		_pRender->Render_IndexedPrimitives(Prim, iP);

		// Render specular
		_pRender->Attrib_TextureID(0, m_SpecTxt[(iTxt2 & 0x7fff) % WATER_SPECANIMLEN]);
		_pRender->Geometry_TVertexArray(TVerts2, 0);
		_pRender->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		if (_pWLS) 
			_pRender->Geometry_ColorArray(SpecVerts);
		else
			_pRender->Geometry_Color(0xffffffff);

		if (pFog) pFog->SetDepthFog(_pRender, 0, CRC_RASTERMODE_ADD);
		_pRender->Render_IndexedPrimitives(Prim, iP);

		// Render additional fog
		if (bFog)
		{
			fp32 oow = pFog->m_FogTableWidthInv;
			fp32 omw = pFog->m_FogTableWidth - 1;
			for(int v = 0; v < nV; v++)
			{
				CPixel32 F = FogVerts[v];
				FogVerts[v] = int(F) | 0xff000000;
				fp32 fFog = fp32(F.GetA()) / 255.0f;
				fFog = (fFog * omw + 0.5f) * oow;
				TVerts2[v].k[0] = fFog;
				TVerts2[v].k[1] = 0.0f;
			}

			_pRender->Geometry_TVertexArray(TVerts2, 0);
			_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			_pRender->Attrib_TextureID(0, pFog->m_FogTableTextureID);
			_pRender->Geometry_ColorArray(FogVerts);
			_pRender->Render_IndexedPrimitives(Prim, iP);
		}

		_pRender->Geometry_VertexArray(NULL);
		_pRender->Geometry_TVertexArray(NULL, 0);
		_pRender->Geometry_ColorArray(NULL);
	_pRender->Attrib_Pop();*/
//	RenderNormals(_pRender, nX*nY, pVerts, NVerts, 14.0f, 0xffffffff);
//	_pRender->Matrix_Pop();

}

// Bounding volumes are in world-space.
void CXR_Model_WaterTile::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Phys_GetBound_Sphere, MAUTOSTRIP_VOID);
	fp32 r = GetBound_Sphere();
	_Radius = r;
	_RetPos = CVec3Dfp32::GetMatrixRow(_Pos, 3);
}

void CXR_Model_WaterTile::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Phys_GetBound_Box, MAUTOSTRIP_VOID);
	GetBound_Box(_RetBox, NULL);
	_RetBox.m_Min += CVec3Dfp32::GetMatrixRow(_Pos, 3);
	_RetBox.m_Max += CVec3Dfp32::GetMatrixRow(_Pos, 3);
}

// Collision services. All indata is in world coordinates.
void CXR_Model_WaterTile::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Phys_Init, MAUTOSTRIP_VOID);
//	m_pPhysAnimState = _pAnimState;
//	m_PhysWMat = _WMat;
//	m_pPhysWC = _pWC;
//	_WMat.InverseOrthogonal(m_PhysWMatInv);
}

int CXR_Model_WaterTile::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Phys_GetMedium, 0);
	if (m_MediumFlags == XW_MEDIUM_AIR) return XW_MEDIUM_AIR;
	CVec3Dfp32 v;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v);
	fp32 fX = v.k[0]*m_ScaleInv[0] + 0.5f;
	fp32 fY = v.k[1]*m_ScaleInv[1] + 0.5f;
	if (fX < 0.0f || fX > 1.0f || 
		fY < 0.0f || fY > 1.0f) return XW_MEDIUM_AIR;

	CMTime t = (_pPhysContext->m_pAnimState) ? CMTime() - _pPhysContext->m_pAnimState->m_AnimTime0 : CMTime(); //  (1.5f)
	fp32 Phase0 = t.GetTimeModulusScaled(m_Frequency0 * 1.5f, 1.0f) + (_pPhysContext->m_WMat.k[3][0] * m_ScaleInv[0] * m_SPeriod0 + _pPhysContext->m_WMat.k[3][1] * m_ScaleInv[1] * m_TPeriod0);
	fp32 Phase1 = t.GetTimeModulusScaled(m_Frequency1 * 1.5f, 1.0f) + (_pPhysContext->m_WMat.k[3][0] * m_ScaleInv[0] * m_SPeriod1 + _pPhysContext->m_WMat.k[3][1] * m_ScaleInv[1] * m_TPeriod1);
	fp32 Phase2 = t.GetTimeModulusScaled(m_Frequency2 * 1.5f, 1.0f) + (_pPhysContext->m_WMat.k[3][0] * m_ScaleInv[0] * m_SPeriod2 + _pPhysContext->m_WMat.k[3][1] * m_ScaleInv[1] * m_TPeriod2);

	fp32 z =
		m_Amp0 * M_Sin((Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*_PI2) +
		m_Amp1 * M_Sin((Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*_PI2) +
		m_Amp2 * M_Sin((Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*_PI2);

//ConOut("(CXR_Model_WaterTile::Phys_GetMedium) " + v.GetString() + CStrF("  %f", z));

	return (v.k[2] < z) ? m_MediumFlags : XW_MEDIUM_AIR;
}

void CXR_Model_WaterTile::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Phys_GetMedium_2, MAUTOSTRIP_VOID);
	if (Phys_GetMedium(_pPhysContext, _v0) & XW_MEDIUM_WATER)
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfCtx, "SYSTEM.SURFACECONTEXT");
		if (!pSurfCtx)
		{
			_RetMedium.SetAir();
			return;
		}

		CXW_Surface* pSurf = pSurfCtx->GetSurface(m_SurfaceID);
		_RetMedium = pSurf->GetBaseFrame()->m_Medium;
	}
	else
		_RetMedium.SetAir();
}

bool CXR_Model_WaterTile::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Phys_IntersectLine, false);
	MSCOPESHORT(CXR_Model_WaterTile::Phys_IntersectLine);

	if(!(_MediumFlags & m_MediumFlags))
		return false;

#ifndef M_RTM
	if (M_Fabs(_v0[0] - _v1[0]) > _FP32_EPSILON || M_Fabs(_v0[1] - _v1[1]) > _FP32_EPSILON )
	{
		//M_TRACEALWAYS("CXR_Model_WaterTile::Phys_IntersectLine, Non-vertical lines is NOT supported!");
		return false;
	}
#endif

	CVec3Dfp32 Slask;
	CBox3Dfp32 Box, WorldBox;
	GetBound_Box(Box, _pPhysContext->m_pAnimState);

	Box.Transform(_pPhysContext->m_WMat, WorldBox);

	if (WorldBox.IntersectLine(_v0, _v1, Slask))
	{
		CVec3Dfp32 v, v1;
		_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v);
		_v1.MultiplyMatrix(_pPhysContext->m_WMatInv, v1);
		fp32 fX = v.k[0]*m_ScaleInv[0] + 0.5f;
		fp32 fY = v.k[1]*m_ScaleInv[1] + 0.5f;
		if (fX < 0.0f || fX > 1.0f || 
			fY < 0.0f || fY > 1.0f) return false;

		CMTime t = (_pPhysContext->m_pAnimState) ? CMTime() - _pPhysContext->m_pAnimState->m_AnimTime0 : CMTime(); //  (1.5f)
		fp32 Phase0 = t.GetTimeModulusScaled(m_Frequency0 * 1.5f, 1.0f) + (_pPhysContext->m_WMat.k[3][0] * m_ScaleInv[0] * m_SPeriod0 + _pPhysContext->m_WMat.k[3][1] * m_ScaleInv[1] * m_TPeriod0);
		fp32 Phase1 = t.GetTimeModulusScaled(m_Frequency1 * 1.5f, 1.0f) + (_pPhysContext->m_WMat.k[3][0] * m_ScaleInv[0] * m_SPeriod1 + _pPhysContext->m_WMat.k[3][1] * m_ScaleInv[1] * m_TPeriod1);
		fp32 Phase2 = t.GetTimeModulusScaled(m_Frequency2 * 1.5f, 1.0f) + (_pPhysContext->m_WMat.k[3][0] * m_ScaleInv[0] * m_SPeriod2 + _pPhysContext->m_WMat.k[3][1] * m_ScaleInv[1] * m_TPeriod2);

		fp32 z =
			m_Amp0 * M_Sin((Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*_PI2) +
			m_Amp1 * M_Sin((Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*_PI2) +
			m_Amp2 * M_Sin((Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*_PI2);


		fp32 dX =
			(_PI2*m_SPeriod0 * m_Amp0 * M_Cos((Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*_PI2) +
			_PI2*m_SPeriod1 * m_Amp1 * M_Cos((Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*_PI2) +
			_PI2*m_SPeriod2 * m_Amp2 * M_Cos((Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*_PI2)) * m_ScaleInv[0];
		fp32 dY =
			(_PI2*m_TPeriod0 * m_Amp0 * M_Cos((Phase0 + fX*m_SPeriod0 + fY*m_TPeriod0)*_PI2) +
			_PI2*m_TPeriod1 * m_Amp1 * M_Cos((Phase1 + fX*m_SPeriod1 + fY*m_TPeriod1)*_PI2) +
			_PI2*m_TPeriod2 * m_Amp2 * M_Cos((Phase2 + fX*m_SPeriod2 + fY*m_TPeriod2)*_PI2)) * m_ScaleInv[1];

		fp32 LenSqr = Sqr(dX) + Sqr(dY) + 1.0f;
		fp32 LenInv = 1.0f / M_Sqrt(LenSqr);

		CVec3Dfp32 Normal(-dX*LenInv, -dY*LenInv, LenInv); 
		//ConOut("(CXR_Model_WaterTile::Phys_GetMedium) " + v.GetString() + CStrF("  %f", z));

		if ( (v.k[2] < z && v1.k[2] > z) || (v.k[2] > z && v1.k[2] < z) )
		{
			if (_pCollisionInfo)
			{
				CVec3Dfp32 LocalPos = v;
				LocalPos[2] = z;

				_pCollisionInfo->m_bIsCollision = 1;
				_pCollisionInfo->m_bIsValid = 1;
				_pCollisionInfo->m_bIsContact = 1;
				_pCollisionInfo->m_LocalPos = LocalPos;
				_pCollisionInfo->m_Pos = LocalPos * _pPhysContext->m_WMat;
				_pCollisionInfo->m_Plane = CPlane3Dfp32(Normal, _pCollisionInfo->m_Pos);
			}

			return true;
		}
	}
	return false;
}

bool CXR_Model_WaterTile::Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Phys_IntersectSphere, false);
	return false;
}

bool CXR_Model_WaterTile::Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_WaterTile_Phys_IntersectBox, false);
	return false;
}

void CXR_Model_WaterTile::CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags )
{
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_WaterTile2
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_WaterTile2, CXR_Model_WaterTile);

void CXR_Model_WaterTile2::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_WaterTile2_Create, MAUTOSTRIP_VOID);
	// ModelName: WaterTile2:Surface,Amplitude,SizeX,SizeY,TexScaleU,TexScaleV,PeriodX,PeriodY,MinTessU,MinTessV,MaxTessY,MaxTessV
	
	if (_pParam)
	{
		CFStr s(_pParam);
		CFStr sSurface = s.GetStrSep(",");
		CFStr sAmplitude = s.GetStrSep(",");
		CFStr sSizeX = s.GetStrSep(",");
		CFStr sSizeY = s.GetStrSep(",");
		CFStr sTexScaleX = s.GetStrSep(",");
		CFStr sTexScaleY = s.GetStrSep(",");
		CFStr sPeriodX = s.GetStrSep(",");
		CFStr sPeriodY = s.GetStrSep(",");
		CFStr sMinTessX = s.GetStrSep(",");
		CFStr sMinTessY = s.GetStrSep(",");
		CFStr sMaxTessX = s.GetStrSep(",");
		CFStr sMaxTessY = s.GetStrSep(",");

		// Surface
		if (sSurface != "")
		{
			m_Surface = sSurface;
			m_Surface.Trim();
			m_Surface.MakeUpperCase();
		}

		// Amplitude
		if (sAmplitude != "")
		{
			fp32 AmpScale = sAmplitude.Val_fp64();
			m_Amp0 = AmpScale;
			m_Amp1 = AmpScale * 0.33f;
			m_Amp2 = AmpScale * 0.2f;
		}

		// Size
		if (sSizeX != "") m_Scale[0] = sSizeX.Val_fp64();
		if (sSizeY != "") m_Scale[1] = sSizeY.Val_fp64();

		// TexScale
		if (sTexScaleX != "") m_TexScale[0] = sTexScaleX.Val_fp64();
		if (sTexScaleY != "") m_TexScale[1] = sTexScaleY.Val_fp64();

		// PeriodScale
		if (sPeriodX != "") m_PeriodScale[0] = sPeriodX.Val_fp64();
		if (sPeriodY != "") m_PeriodScale[1] = sPeriodY.Val_fp64();

		// Tess
		if (sMinTessX != "") m_MinTess[0] = sMinTessX.Val_fp64();
		if (sMinTessY != "") m_MinTess[1] = sMinTessY.Val_fp64();
		if (sMaxTessX != "") m_MaxTess[0] = sMaxTessX.Val_fp64();
		if (sMaxTessY != "") m_MaxTess[1] = sMaxTessY.Val_fp64();

		if (m_MinTess[0] == 0) m_MinTess = 4;
		if (m_MaxTess[0] == 0) m_MaxTess = 12;
	}

	OnPostCreate();
}

