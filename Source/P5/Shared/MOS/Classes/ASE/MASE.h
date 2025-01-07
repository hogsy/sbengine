
#ifndef _INC_MASE
#define _INC_MASE

#include "../MCC/MCC.h"

// -------------------------------------------------------------------
class CParamNode;
typedef TPtr<CParamNode> spCParamNode;

class CParamNode : public CReferenceCount
{
public:
	CStr m_Key;
	TArray<CStr> m_lParams;

	TArray<spCParamNode> m_lspChildren;

	int GetChildIndex(char* _pName);
	CParamNode* GetChild(char* _pName);

protected:
	CStr ASE_GetLine(char* _pData, int _Size, int& _Pos);
	CStr ASE_GetWord(char* _pData, int _Size, int& _Pos);

	int ASE_Parse(char* _pData, int _Size);		// Ret. parsed len.

public:
	void ASE_LogDump(int _Level);
	void ReadASE(CStr& _Filename);
};


// -------------------------------------------------------------------

#endif
