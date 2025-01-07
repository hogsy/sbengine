#ifndef __WOBJ_PRIMITIVES_H
#define __WOBJ_PRIMITIVES_H

#ifdef M_Profile
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Helper class for rendering simple primitives

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWObject_CoordinateSystem
					CWObject_Line
\*____________________________________________________________________________________________*/

// -------------------------------------------------------------------
//  CWObject_CoordinateSystem
// -------------------------------------------------------------------
class CWObject_CoordinateSystem : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	int32 m_Time;

public:
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);

	virtual void OnCreate();

	static void OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);

	virtual void OnRefresh();

	static void OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);

	virtual void OnLoad(CCFile* _pFile);

	virtual void OnSave(CCFile* _pFile);
};


// -------------------------------------------------------------------
//  CWObject_Line
// -------------------------------------------------------------------
class CWObject_Line : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	virtual void OnCreate();

	virtual void OnRefresh();

	virtual aint OnMessage(const CWObject_Message& _Msg);

	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

#endif

#endif
