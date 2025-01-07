#ifndef __WRPGCORE_H
#define __WRPGCORE_H

#include "MCC.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

class CRPG_Object;
typedef TPtr<CRPG_Object> spCRPG_Object;

// -------------------------------------------------------------------
//  CRPG_Attrib
// -------------------------------------------------------------------
class CRPG_Attrib
{
public:
	CRPG_Attrib()							{ m_Value = 0; /*m_Current = 0;*/ }
//	CRPG_Attrib(int _Value, int _Current)	{ m_Value = _Value; m_Current = _Current; }

	CRPG_Attrib(const CRPG_Attrib& _Attr)
	{
		m_Value = _Attr.m_Value;
//		m_Current = _Attr.m_Current;
	}

	int16 m_Value;
//	int16 m_Current;

	void operator= (const CRPG_Attrib& _Attr)
	{
		m_Value = _Attr.m_Value;
//		m_Current = _Attr.m_Current;
	}

//	void operator =(int _Value)				{ int Delta = _Value - m_Value; *this += Delta; }
	void operator =(int _Value)				{ m_Value = _Value; };
	
/*	void operator +=(int _Delta)			{ m_Value += _Delta; m_Current += _Delta; }
	void operator -=(int _Delta)			{ *this += -_Delta; }
	void operator *=(fp32 _Mul)				{ m_Value *= _Mul; m_Current *= _Mul; }

	int operator ++(int)					{ *this += 1; return m_Current - 1; }
	int operator --(int)					{ *this -= 1; return m_Current + 1; }

	operator int() const					{ return m_Current; }*/
	operator int() const					{ return m_Value; }
};
/*
class CRPG_Attrib_fixed_8_8
{
public:
	CRPG_Attrib *m_pAttrib;

	CRPG_Attrib_fixed_8_8(CRPG_Attrib &_Attrib)
	{
		m_pAttrib = &_Attrib;
	}

	void operator= (fp32 _Value)
	{
		*m_pAttrib = _Value * 256;
	}

	void operator +=(int _Delta)			{ *m_pAttrib += _Delta * 256; }
	void operator -=(int _Delta)			{ *m_pAttrib -= _Delta * 256; }
	void operator *=(int _Mul)				{ *m_pAttrib *= _Mul; }

	operator fp32() const					{ return fp32(*m_pAttrib) / 256; }
};

class CRPG_Attrib_int
{
public:
	CRPG_Attrib *m_pAttrib0;
	CRPG_Attrib *m_pAttrib1;

	CRPG_Attrib_int(CRPG_Attrib &_Attrib0, CRPG_Attrib &_Attrib1)
	{
		m_pAttrib0 = &_Attrib0;
		m_pAttrib1 = &_Attrib1;
	}

	void operator =(int _Value)
	{
		int Delta = _Value - *this;
		*this += Delta;
	}
	
	void operator +=(int _Delta)
	{
		SetCurrent(*this + _Delta);
		SetValue(GetValue() + _Delta);
	}
	void operator -=(int _Delta)			{ *this += -_Delta; }

	int operator ++(int)					{ *this += 1; return *this - 1; }
	int operator --(int)					{ *this -= 1; return *this + 1; }

	operator int() const
	{
		return (uint16(*m_pAttrib1) << 16) | uint16(*m_pAttrib0);
	}

	int GetValue()
	{
		return (uint16(m_pAttrib1->m_Value) << 16) | uint16(m_pAttrib0->m_Value);
	}

	void SetCurrent(int _Value)
	{
		m_pAttrib1->m_Current = _Value >> 16;
		m_pAttrib0->m_Current = _Value;
	}

	void SetValue(int _Value)
	{
		m_pAttrib1->m_Value = _Value >> 16;
		m_pAttrib0->m_Value = _Value;
	}
};

class CRPG_Attrib_fp32
{
public:
	//Note: When using this, the attributes will NOT be set to 0 during initializing (*(fp32 *)&(int(0)) != 0.0f)

	CRPG_Attrib *m_pAttrib0;
	CRPG_Attrib *m_pAttrib1;

	CRPG_Attrib_fp32(CRPG_Attrib &_Attrib0, CRPG_Attrib &_Attrib1)
	{
		m_pAttrib0 = &_Attrib0;
		m_pAttrib1 = &_Attrib1;
	}

	void operator =(fp32 _Value)
	{
		fp32 Delta = _Value - *this;
		*this += Delta;
	}
	
	void operator +=(fp32 _Delta)
	{
		SetCurrent(*this + _Delta);
		SetValue(GetValue() + _Delta);
	}

	void operator -=(fp32 _Delta)			{ *this += -_Delta; }

	fp32 operator ++(int)					{ *this += 1; return *this - 1; }
	fp32 operator --(int)					{ *this -= 1; return *this + 1; }

	operator fp32() const
	{
		int i = (uint16(*m_pAttrib1) << 16) | uint16(*m_pAttrib0);
		return *(fp32 *)&i;
	}

	fp32 GetValue()
	{
		int i = (uint16(m_pAttrib1->m_Value) << 16) | uint16(m_pAttrib0->m_Value);
		return *(fp32 *)&i;
	}

	void SetCurrent(fp32 _Value)
	{
		int i = *(int *)&_Value;

		m_pAttrib1->m_Current = i >> 16;
		m_pAttrib0->m_Current = i;
	}

	void SetValue(fp32 _Value)
	{
		int i = *(int *)&_Value;

		m_pAttrib1->m_Value = i >> 16;
		m_pAttrib0->m_Value = i;
	}
};
*/
// -------------------------------------------------------------------
//  CRPG_Object
// -------------------------------------------------------------------
class CRPG_Object : public CReferenceCount
{
	MRTC_DECLARE;

public:
	enum
	{
		TYPE_OBJECT = 0,
		TYPE_CHAR,
		TYPE_ITEM,
		TYPE_INVENTORY,
	};

	// -------------------------------------------------------------------
	//  Creation
	spCRPG_Object Duplicate() const;
	virtual void OnDuplicate(const CRPG_Object *_pSource) {}
	
	spCRPG_Object CreateObject(const char *_pName) { return CRPG_Object::CreateObject(_pName, m_pWServer); }
	static spCRPG_Object CreateObject(const char *_pName, CWorld_Server *_pWServer);
	virtual spCRPG_Object CreateObjectVirtual(const char *_pName, CWorld_Server *_pWServer);	// Same as CreateObject, but can be called from modules that can access an existing CRPG_Object and use it as a class factory.
	static spCRPG_Object CreateObject(const CRegistry* _pKeys, CWorld_Server *_pWServer);
	static spCRPG_Object CreateObject(CCFile* _pFile, CWorld_Server *_pWServer);
	static spCRegistry GetEvaledRegistry(const char *_pName, CWorld_Server *_pWServer);
	void Write(CCFile* _pFile);

	static void IncludeRPGClass(const char *_pName, CMapData *_pMapData, CWorld_Server *_pWServer, bool _bPrecacheForPlayer = false, TArray<int32>* _plAnimTypesNeeded = NULL);
	static void IncludeRPGRegistry(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer) {}

	static void IncludeAnimFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData);
	static void IncludeModelFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData);
	static void IncludeShellTypeFromKey(const CFStr& _Key, const CRegistry* _pReg, CMapData* _pMapData);
	static void IncludeSoundFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData);
	static void IncludeClassFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData);
	static void IncludeRPGClassFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData, CWorld_Server *_pWServer, bool _bPrecacheForPlayer = false);
	static void IncludeSurfaceFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData);

	static int32 GetItemAnimProperty(const CStr& _Str);

	virtual void OnCreate();

	// -------------------------------------------------------------------
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys() {}

	virtual void AddDesc(CStr &) const {}
	virtual CStr GetDesc(int _i) const { if( _i == 0 ) return m_Name; else return CStr(""); }

	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iOwner, int _Input) { return false; }

	virtual bool Process(CRPG_Object *_pRoot, int);
	virtual bool OnProcess(CRPG_Object *_pRoot, int)		{ return false; }
	virtual bool PreProcess(CRPG_Object *_pRoot, int)		{ return false; }

	virtual int GetType() const							{ return TYPE_OBJECT; }
	virtual int GetModel(int _iModel, int _SetNr = -1) const { return -1; }

	virtual aint OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner);
	// -------------------------------------------------------------------
	// Hierarchy management
	int GetNumChildren() const							{ return m_lspItems.Len(); }
	void SetNumChildren(int _Len)						{ m_lspItems.SetLen(_Len); }
	spCRPG_Object &Child(int _iIndex)					{ return m_lspItems[_iIndex]; }
	CRPG_Object *GetChild(int _iIndex)					{ return m_lspItems[_iIndex]; }
	const CRPG_Object *GetChild(int _iIndex) const		{ return m_lspItems[_iIndex]; }
	void SetChild(int _Index, CRPG_Object *_pObj)		{ m_lspItems[_Index] = _pObj; }
	CRPG_Object *Find(const char *_pName);
	CRPG_Object *GetChild(const char *_pName);
	int GetChildIndex(const char *_pName);

	int AddChild(CRPG_Object *_pObject)					{ return m_lspItems.Add(_pObject); }
	int AddChild(const CStr _Name)						{ return m_lspItems.Add(CRPG_Object::CreateObject(_Name, m_pWServer)); }
	void DelChild(int _iIndex)							{ m_lspItems[_iIndex] = NULL; m_lspItems.Del(_iIndex); }

	int GetNumAttribs()	const							{ return m_lAttribs.Len(); }
	void SetNumAttribs(int _Len)						{ m_lAttribs.SetLen(_Len); }
	CRPG_Attrib &Attrib(int _iIndex)					{ return m_lAttribs[_iIndex]; }
	const CRPG_Attrib &Attrib(int _iIndex) const		{ return m_lAttribs[_iIndex]; }

	int GetNumStrings()	const							{ return m_lStrings.Len(); }
	void SetNumStrings(int _Len)						{ m_lStrings.SetLen(_Len); }
	CStr &String(int _iIndex)							{ return m_lStrings[_iIndex]; }
	const CStr &String(int _iIndex) const 				{ return m_lStrings[_iIndex]; }

	// -------------------------------------------------------------------
	// Server helpers
	bool TraceRay(const CVec3Dfp32 &_Pos0, const CVec3Dfp32 &_Pos1, CCollisionInfo* _pCInfo, int _CollisionObjects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER, int _CollisionMediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, int _iExclude = 0);
	bool TraceRay(const CMat4Dfp32 &_Mat, float _Range, CCollisionInfo* _pCInfo, int _CollisionObjects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER, int _CollisionMediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, int _iExclude = 0);

	int SendMsg(int _iObject, int _iMsg, int _Param0 = 0, int _Param1 = 0, int _iSender = -1);
	int SendMsg(int _iObject, int _iMsg, const CVec3Dfp32 &, int _Param0 = 0, int _iSender = -1);
	int SendMsg(int _iObject, int _iMsg, int _Param0, int _Param1, int16 _iSender, int16 _Reason, const CVec3Dfp32& _VecParam0 = 0, const CVec3Dfp32& _VecParam1 = 0, void* _pData = NULL, int _DataSize = 0);

	int GetRegValuei(const char *_Key, int _Default);
	fp32 GetRegValuef(const char *_Key, fp32 _Default);
	char *GetRegValue(const char *_Key, char *_Default);

	// -------------------------------------------------------------------
	// Debug
	virtual void Dump(int _Level = 0);

	// -------------------------------------------------------------------
	CStr m_Name;
	CWorld_Server *m_pWServer;

	static const char* ms_DamageTypeStr[];
	static const char* ms_AnimProperty[];

private:
	static spCRPG_Object CreateRuntimeClassObject(const char *_pName, CWorld_Server *_pWServer);
	static bool IncludeRPGEvaledRegistry(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer, TArray<int32>* _plAnimTypesNeeded = NULL);
	
	TArray<spCRPG_Object> m_lspItems;
	TArray<CRPG_Attrib> m_lAttribs;
	TArray<CStr> m_lStrings;

public:
	// Hack to avoid precaching of all iconsurfaces that the player never will see.
	static bool m_bPrecacheForPlayerUse;
};

#endif
