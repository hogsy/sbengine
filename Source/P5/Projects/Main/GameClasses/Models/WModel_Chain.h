#ifndef __WModel_Chain_h__
#define __WModel_Chain_h__

#include "ModelsMisc.h"
#include "CSurfaceKey.h"

class CXR_Model_Chain : public CXR_Model_Custom
{
	MRTC_DECLARE;

	public:
		virtual void Create(const char* _pParam);
		virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
		virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = 0);
		virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = 0);
		//virtual aint GetParam(int _Param);
		//virtual void SetParam(int _Param, aint _Value);

		CXR_RenderInfo& GetRenderInfo(CXR_Model_Custom_RenderParams* _pRenderParams) { return _pRenderParams->m_RenderInfo; }

		//bool NeedCollisionUpdate();

		//virtual TPtr<CXR_ModelInstance> CreateModelInstance();

		// Called from effect system object (GameObject, an effect system with collision info has to exist as an game object!)
		//static void OnCollisionRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Model_EffectSystem& _FXModel, CEffectSystemHistory& _History, const int& _iOwner = 0);

		//CEffectSystemRenderData* GetRenderData() { return m_pRD; }
		//uint32 GetNumFXParticles() { return m_lFXParticles.Len(); }
		//CFXDataFXParticle* GetFXParticles() { return m_lFXParticles.GetBasePtr(); }
		
		//static CEffectSystemHistory::CEffectSystemHistoryObject* GetFXParticleHistory(CXR_Model_EffectSystem* _pFXSystem, const CFXDataFXParticle& FXParticle)			{ return _pFXSystem->m_pRD->m_pHistory->GetHistory(FXParticle.m_UseHistoryObject); }
		//static CEffectSystemHistory::CEffectSystemHistoryObject* GetFXParticleHistory(CXR_Model_EffectSystem* _pFXSystem, const CFXDataFXParticle& FXParticle) const	{ return _pFXSystem->m_pRD->m_pHistory->GetHistory(FXParticle.m_UseHistoryObject); }

		//uint8						m_RenderingObject;
		
		CSurfaceKey m_SK;

	//private:
	//	static void	Init();
		//CFXDataObject* GetDataObject(const uint32& _iObject, const uint8* _pObjectsType, const uint8* _piObjects);
		//void EvalRegisterObject(const CRegistry* _pReg, CFXDataCollect& _DataStorage);
};

#endif
