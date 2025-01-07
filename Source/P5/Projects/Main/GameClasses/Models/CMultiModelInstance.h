#ifndef CMultiModelInstance_h
#define CMultiModelInstance_h

//----------------------------------------------------------------------

#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
/*
#include "../../../../Shared/MOS/XR/XRCustomModel.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WClass.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WPhysState.h"
*/

//----------------------------------------------------------------------

class CMultiModelInstance : public CReferenceCount
{
	public:

		virtual uint32 GetNumInstances() pure;
		virtual bool GetInstanceAnimState(uint32 _iInstance, fp32 _GameTime, CXR_AnimState* _pAnimState) pure;
		virtual bool GetInstanceWorldMatrix(uint32 _iInstance, CMat43fp32* _pMatrix) pure;
		virtual bool GetInstance(uint32 _iInstance, fp32 _GameTime, CXR_AnimState* _pAnimState, CMat43fp32* _pMatrix) pure;
};

//----------------------------------------------------------------------

#endif /* CMultiModelInstance_h */