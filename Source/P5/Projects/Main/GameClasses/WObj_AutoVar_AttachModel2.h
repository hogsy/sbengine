#ifndef __WOBJ_AUTOVAR_ATTACHMODEL2_H
#define __WOBJ_AUTOVAR_ATTACHMODEL2_H

#include "MRTC.h"
#include "MMath.h"
#include "WObj_AutoVar_AttachModel.h"

//AR-CHANGE: Added forward declarations..
class CMapData;
class CWorld_Client;
class CWObject_Client;
class CXR_Anim_SequenceData;
class CXR_AnimState;
class CXR_Engine;
class CXR_Model;
class CXR_Skeleton;
class CXR_SkeletonInstance;
class CXR_ModelInstance;


/////////////////////////////////////////////////////////////////
// AutoVar_AttachModel
class CAutoVar_AttachModel2
{
public:
	enum
	{
		FLAGS_SERVERANIMTIME = 1,
	};

	uint16 m_iModel[ATTACHMODEL_NUMMODELS];									// Replicated
	uint8 m_iAttachPoint[ATTACHMODEL_NUMMODELS];							// Replicated
	int8 m_ModelFlags[ATTACHMODEL_NUMMODELS];

	uint8 m_Flags;															// Replicated
	uint8 m_iAttachRotTrack;												// Replicated

	// Client only information
	TPtr<CXR_ModelInstance> m_lspModelInstances[ATTACHMODEL_NUMMODELS];

//	fp32 m_AC_LastKeyframe;
//	int m_AC_LastUpdate;
//	uint32 m_AC_iSequence;

	// Test for skeleton animated items (ex bow in templar), maximum of two animations at 
	// any time (blend between them....)
	// Needs to be copied over network, should perhaps have flag and stuff (for blending)
	// but it might not need to be very advanced?
	CSkelAnimInstance m_lSkelAnim[SKELANIM_NUMBEROFANIMS];
	uint16	m_SkelAnimFlags; // Flags for serverside pack (if start times should be sent)
	int8    m_iSkelAnimLastInsertion;

	TPtr<CXR_SkeletonInstance> m_spSkeletonInstance;
	
	CAutoVar_AttachModel2();
	void Clear();
	
	bool IsValid();

	void UpdateModel(const CAutoVar_AttachModel2& _Model);

	void SetBaseModel(int _iModel, int _iAttachPoint, int _iAttachRotTrack);
	void SetModel(int _Index, int _iModel, int _iAttachPoint);
	void SetFlags(int _Index, int _Flags);

	// Test function for skeleton animating a model
	bool SetSkelAnim(int _Anim, const CMTime& _GameTime, uint16 _Flags);
	void CreateClientSkelInstance();
	int FindNextSkelAnimSlot(uint8 _Flags);
	bool DoSkelAnim(CXR_AnimState* _pAnimState, CMat43fp32* _pMatrix, CWorld_PhysState* _pWPhys, const CMTime& _Time);

	virtual bool GetModel0_RenderMatrix(CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, CXR_Model *_pModel,
										int _iRotTrack, int _iAttachPoint, CMat43fp32 &_Mat);

	virtual bool GetModel0_RenderInfo(CMapData *_pMapData, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, const CMTime& _Time,
									  CXR_AnimState &_RetAnim, CMat43fp32 &_RetPos, CXR_Model *&_pRetModel, CWorld_PhysState* _pWPhys);

	virtual void RenderExtraModels(CMapData *_pMapData, CXR_Engine* _pEngine,
								   CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel,
								   const CMat43fp32 &_Mat, const CMTime& _Time, uint16 _iObject);

	virtual void RenderAll(CMapData *_pMapData, CXR_Engine* _pEngine,
						   CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel,
						   const CMat43fp32 &_Mat, const CMTime& _Time, CWorld_PhysState* _pWPhys, uint16 _iObject);
	
	virtual void RefreshModelInstances(CWObject_Client *_pObj, CWorld_Client *_pWClient, int _GameTick);
	virtual void Refresh(CWObject_Client *_pObj, CWorld_Client *_pWClient, int _GameTick);
	
	virtual void EvalAnimKey(int _iController, const CXR_Anim_DataKey *_pKey, int _Tick);

	static void SCopyFrom(void* _pThis, const void* _pFrom);
	static void SPack(const void* _pThis, uint8 *&_pD, CMapData* _pMapData);
	static void SUnpack(void* _pThis, const uint8 *&_pD, CMapData* _pMapData);
};

#endif
