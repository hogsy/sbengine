#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

enum { MAX_TRACKED_IMPULSES = 4 };

class CWObject_ObjAnim : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	class CKeyFrame
	{
	public:
		int m_iAction;
		CStr m_Cmd;
		CStr m_Target;
		fp32 m_Value;
	};

	TList_Vector<CKeyFrame> m_KeyFrames;
	int32 m_iIndex;
	int32 m_Wait;
	int32 m_ImpulseWait;
	int32 m_CriticalSection;

	int32 m_nCurImpulses;
	int32 m_CurImpulse[MAX_TRACKED_IMPULSES];
	int32 m_ImpulseTime[MAX_TRACKED_IMPULSES];

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	int GetObjectIndexes(const CStr _Name, const int16 **_pSel);
	virtual void OnRefresh();
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
};

