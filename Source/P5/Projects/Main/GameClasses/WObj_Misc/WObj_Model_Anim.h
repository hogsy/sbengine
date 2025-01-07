#ifndef _INC_WOBJ_MODEL_ANIM
#define _INC_WOBJ_MODEL_ANIM

#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

#define CWObject_Model_Anim_Parent CWObject_Model
class CWObject_Model_Anim : public CWObject_Model_Anim_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	enum
	{
		OBJMSG_ANIMMODEL_PLAYANIM			= 0x2000,

		MODEL_ANIM_FLAG_SKIPROTOFFSET	= 1 << 0,
		MODEL_ANIM_FLAG_SKIPPOSOFFSET	= 1 << 1,
		MODEL_ANIM_FLAG_PHYSICS			= 1 << 2,
	};
protected:
	// No prediction, just blending...
	class CAnimInstance
	{
	public:
		CXR_Anim_SequenceData* m_pSeq;
		int32 m_StartTick;
		int16 m_iAnimContainerResource;
		int16 m_iAnimSeq; // Within container.

		fp32 m_TimeScale;
//		fp32 m_Duration;
		fp32 m_BlendDuration;
		fp32 m_SkeletonScale;
		int8 m_bUseAnimPhys;

		CAnimInstance()
		{
			Clear();
		}

		void Clear()
		{
			m_pSeq = NULL;
			m_iAnimContainerResource = 0;
			m_iAnimSeq = -1;
			m_TimeScale = 1.0f;
			m_StartTick = 0;
//			m_Duration = 0.0f;
			m_BlendDuration = 0.0f;
			m_SkeletonScale = 0.0f;
			m_bUseAnimPhys = false;
		}

		bool IsValid() const
		{
			return ((m_iAnimContainerResource != 0) && (m_iAnimSeq >= 0));
		}

		CXR_Anim_SequenceData* GetAnimSequenceData(CMapData* _pMapData)
		{
			if (m_pSeq)
				return m_pSeq;

			if (!IsValid())
				return NULL;

			CWResource* pAnimContainerResource = _pMapData->GetResource(m_iAnimContainerResource);
			if (pAnimContainerResource == NULL || pAnimContainerResource->GetClass() != WRESOURCE_CLASS_XSA)
				return NULL;

			CWRes_Anim* pAnimRes = (CWRes_Anim*)pAnimContainerResource;
			//pAnimRes->m_TouchTime = _pMapData->m_TouchTime; // Added by Anton

			CXR_Anim_Base* pAnimBase = pAnimRes->GetAnim();
			if (pAnimBase == NULL)
				return NULL;

			m_pSeq = (CXR_Anim_SequenceData*)pAnimBase->GetSequence(m_iAnimSeq);

			return m_pSeq;
		}
	};

	// Queue up animations that you want to play
	class CAnimInstanceQueue
	{
	public:
		TArray<CAnimInstance> m_lAnimations;

		void QueueAnimation(const CAnimInstance& _AnimInstance);
		int32 GetAnimLayers(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CXR_AnimLayer* _pLayers, fp32* _pRetScale, fp32 _Frac = 0.0f);
		bool GetAnimPhysLayer(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CXR_AnimLayer& _Layer, fp32 _Frac = 0.0f);
		bool UsingAnimPhys(CWObject_CoreData* _pObj);
		
		int OnCreateClientUpdate(const uint8* _pData) const;
		int OnClientUpdate(const uint8* _pData);

		// true if something has changed (removed old anim for instance)
		bool Refresh(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj);
		
	};

	class CAnimModelClientData : public CReferenceCount
	{
	public:
		CVec3Dfp32 m_StartPos;
		CQuatfp32  m_StartRot;
		CAnimInstanceQueue m_AnimQueue;

		CAnimModelClientData()
		{
			m_StartPos = 0.0f;
			m_StartRot.Unit();
		}
	};

	static CStr m_sAttachObjectName;
	CMat4Dfp32 m_AnimBaseMat;
	CVec3Dfp32 m_PosOffset;
	CQuatfp32  m_RotOffset;

	int32 m_iAttachObject;
	// Operational flags
	uint8	m_AnimModelFlags;

public:
	CWObject_Model_Anim();

	virtual void OnCreate();
	virtual void OnSpawnWorld();
	
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	virtual aint OnMessage(const CWObject_Message &_Msg);

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	virtual void OnRefresh();

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);

	static CAnimModelClientData* GetAnimModelClientData(CWObject_CoreData* _pObj);
	static const CAnimModelClientData* GetAnimModelClientData(const CWObject_CoreData* _pObj);

	int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);

	void MoveObject(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj);
};

#endif
