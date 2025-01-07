#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

// -------------------------------------------------------------------
//  Func_FunBox
// -------------------------------------------------------------------
class CWObject_Func_FunBox : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	fp32 m_Speed;
	fp32 m_Pos;
	fp32 m_MoveRange;
	int32 m_Mode;
	CVec3Dfp32 m_Startpos;
	CVec3Dfp32 m_Direction;

public:
	CWObject_Func_FunBox();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnLoad(CCFile* _pFile, CMapData* _pWData, int _Flags);
	virtual void OnSave(CCFile* _pFile, CMapData* _pWData, int _Flags);
};

// -------------------------------------------------------------------
//  FUNC_BUTTON
// -------------------------------------------------------------------
class CWObject_Func_Button : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	CVec3Dfp32 m_StartPos;
	CVec3Dfp32 m_RotAngles;
	CVec3Dfp32 m_MoveDirection;

	fp32 m_Speed;
	fp32 m_MoveRange;
	fp32 m_Pos;

	int32 m_Mode;

	int32 m_Wait;
	int32 m_WaitCount;

	int32 m_Param;

	int32 m_bRotate;

	TList_Vector<CStr> m_Targets;

public:
	CWObject_Func_Button();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	void SetPosition();	
	virtual void OnRefresh();
	void TriggerButton();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnLoad(CCFile* _pFile, CMapData* _pWData, int _Flags);
	virtual void OnSave(CCFile* _pFile, CMapData* _pWData, int _Flags);
};
