#ifndef _INC_WCLIENTMODWND
#define _INC_WCLIENTMODWND

#include "WClientMod.h"

// -------------------------------------------------------------------
class CMWnd_ClientInfo
{
public:
	class CWFrontEnd_Mod* m_pFrontEnd;
	class CWClient_Mod* m_pClient;

	CMWnd_ClientInfo()
	{
		m_pFrontEnd = NULL;
		m_pClient = NULL;
	}

	virtual void OnSetContext() {};
	virtual void OnSetClient() {};
	virtual void OnSetFrontEnd() {};

	void SetContext(class CWFrontEnd_Mod* _pFrontEnd, class CWClient_Mod* _pC)
	{
		m_pFrontEnd = _pFrontEnd;
		m_pClient = _pC;

		if (_pFrontEnd) OnSetFrontEnd();
		if (_pC) OnSetClient();
		OnSetContext();
	}
};

#define MACRO_SAFEGETITEM(Class, Ptr, Name) Class* Ptr = safe_cast<Class>(GetItem(Name))

// -------------------------------------------------------------------
//  CMWnd_ModSelectClass
// -------------------------------------------------------------------
class CMWnd_ModSelectClass : public CMWnd
{
	MRTC_DECLARE;
public:
	CMWnd_ClientInfo m_ClientInfo;
	CMWnd_ClientInfo *GetClientInfo()
	{
		return &m_ClientInfo;
	}

	CMWnd_ModSelectClass::CMWnd_ModSelectClass();

	int GetItemState(const char* _pName);
	void SetItemState(const char* _pName, int _Value);
	virtual void OnCreate();
	virtual void UpdateCharacterList();

	virtual aint OnMessage(const CMWnd_Message* _pMsg);

	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);

private:
	CStr	m_SelectionFilter;
};

// -------------------------------------------------------------------
//  CMWnd_ModSelectTeam
// -------------------------------------------------------------------
class CMWnd_ModSelectTeam : public CMWnd
{
	MRTC_DECLARE;
public:
	CMWnd_ClientInfo m_ClientInfo;
	CMWnd_ClientInfo *GetClientInfo()
	{
		return &m_ClientInfo;
	}

	CMWnd_ModSelectTeam();

	int GetItemState(const char* _pName);
	void SetItemState(const char* _pName, int _Value);
	virtual void OnCreate();
	virtual void UpdateCharacterList();

	void OnRefresh();

	virtual aint OnMessage(const CMWnd_Message* _pMsg);

private:
	int		m_LastUpdateTick;
};

class CListItem_Char
{
public:
	CStr m_Class;
	CStr m_Name;
	CStr m_Darkling;

	CListItem_Char() {};
	CListItem_Char(CStr _Name, CStr _Class, CStr _Darkling) :
		m_Class(_Class),
		m_Name(_Name),
		m_Darkling(_Darkling) {};
};

class CMWnd_CharacterList : public CMWnd_List2
{
	MRTC_DECLARE;

public:
	TArray<CListItem_Char> m_lChars;

	CMWnd_CharacterList();
	virtual void SetItemCount(int _nItems);
	virtual void AddItem(const CListItem_Char& _Item);
	virtual void OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, int _iItem);
	void OnTouchItem(int _iItem, int _bSelected);
};

class CListItem_Team
{
public:
	CStr m_TeamName;
	uint8 m_Value;
	uint8 m_Players;

	CListItem_Team() {};
	CListItem_Team(CStr _TeamName, uint8 _Value, uint8 _Players) :
	m_TeamName(_TeamName),
		m_Value(_Value),
		m_Players(_Players) {};
};

class CMWnd_TeamList : public CMWnd_List2
{
	MRTC_DECLARE;

public:
	TArray<CListItem_Team> m_lChars;

	CMWnd_TeamList();
	virtual void SetItemCount(int _nItems);
	virtual void AddItem(const CListItem_Team& _Item);
	virtual void OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, int _iItem);
	void OnTouchItem(int _iItem, int _bSelected);
};

#endif // _INC_WCLIENTMODWND
