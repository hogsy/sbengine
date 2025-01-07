#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_System.h"

// -------------------------------------------------------------------
//  Func_Float
// -------------------------------------------------------------------


class CWObject_Func_Float : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	class CWaveform
	{
	public:
		int8 m_iType;
		fp32 m_Amplitude;
		fp32 m_Duration;
	};

	TList_Vector<CWaveform> m_Waveforms;
	CMat43fp32 m_OrgMat;

public:
	CWObject_Func_Float();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);	
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnRefresh();
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
};
