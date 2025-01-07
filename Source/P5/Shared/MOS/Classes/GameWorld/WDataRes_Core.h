#ifndef __WDATARES_CORE_H
#define __WDATARES_CORE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc resources classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWRes_Model
					CWRes_DLL
					CWRes_Class
					CWRes_Template
					CWRes_Registry
\*____________________________________________________________________________________________*/

#include "WData.h"
#include "../../MSystem/Misc/MRegistry_Compiled.h"

// -------------------------------------------------------------------
//  Resource class enums for native resources.
//  Must match CWD_ResourceClassDesc list created in CWorldData::Create()
// -------------------------------------------------------------------
enum
{
	WRESOURCE_CLASS_NULL = -1,
	WRESOURCE_CLASS_DLL = 0,
#ifndef MDISABLE_CLIENT_SERVER_RES
	WRESOURCE_CLASS_WOBJECTCLASS,
#endif
	WRESOURCE_CLASS_MODEL_XW,
	WRESOURCE_CLASS_MODEL_XMD,
	WRESOURCE_CLASS_MODEL_CUSTOM,
	WRESOURCE_CLASS_MODEL_CUSTOM_FILE,
	WRESOURCE_CLASS_XSA,
	WRESOURCE_CLASS_XTC,
	WRESOURCE_CLASS_SOUND,
	WRESOURCE_CLASS_WAVE,
	WRESOURCE_CLASS_XWFILEINDEX,
	WRESOURCE_CLASS_SURFACE,
	WRESOURCE_CLASS_REGISTRY,
	WRESOURCE_CLASS_XFC,
	WRESOURCE_CLASS_XWDATA,
#ifndef MDISABLE_CLIENT_SERVER_RES
	WRESOURCE_CLASS_TEMPLATE,
#endif
	WRESOURCE_CLASS_XWNAVGRID,
	WRESOURCE_CLASS_MODEL_XW2,
#ifndef M_DISABLE_TODELETE
	WRESOURCE_CLASS_ANIMLIST,
#endif
	WRESOURCE_CLASS_DIALOGUE,
	WRESOURCE_CLASS_XWNAVGRAPH,
	WRESOURCE_CLASS_OBJECTATTRIBS,
	WRESOURCE_CLASS_FACIALSETUP,

	// AGMERGE
	WRESOURCE_CLASS_XAH,

	WRESOURCE_CLASS_MODEL_XW3,
	WRESOURCE_CLASS_MODEL_XW4,

	WRESOURCE_CLASS_MODEL_GLASS,

	WRESOURCE_NUMPREFIXES,
};

// -------------------------------------------------------------------
//  CWRes_Model, abstract base class for CXR_Model resources.
// -------------------------------------------------------------------
class CWRes_Model : public CWResource
{
	MRTC_DECLARE;
public:
	virtual CXR_Model* GetModel() { return NULL; }
	virtual void OnPrecache(class CXR_Engine* _pEngine);
	virtual void OnPostPrecache(class CXR_Engine* _pEngine);
};


// -------------------------------------------------------------------
//  DLL, STATIC LIBRARY
// -------------------------------------------------------------------

#ifdef WCLIENT_STATICLIBRARY

// The DLL resource is just a dummy when not using dynamic-libraries.

class CWRes_DLL : public CWResource
{
	MRTC_DECLARE;

	CStr m_FileName;

public:
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
};


#endif

// -------------------------------------------------------------------
//  DLL, DYNAMIC LIBRARY
// -------------------------------------------------------------------

#ifndef WCLIENT_STATICLIBRARY

class CWRes_DLL : public CWResource
{
	MRTC_DECLARE;

	CStr m_FileName;

public:
	~CWRes_DLL();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
};

#endif

#ifndef MDISABLE_CLIENT_SERVER_RES

// -------------------------------------------------------------------
//  CLASS (WObject class)
// -------------------------------------------------------------------
class CWRes_Class : public CWResource
{
	MRTC_DECLARE;

	class MRTC_CRuntimeClass_WObject* m_pRTC;
	CWorldData::CWD_ClassStatistics m_Stats;

public:

	CWRes_Class();
	virtual MRTC_CRuntimeClass_WObject* GetRuntimeClass() const;
	virtual void ClearStats();
	virtual CWorldData::CWD_ClassStatistics* GetStats();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
};

// -------------------------------------------------------------------
//  TEMPLATE
// -------------------------------------------------------------------
class CWRes_Template : public CWResource
{
	MRTC_DECLARE;

	bool Load();

public:
	CWorldData* m_pWData;
	TThinArray<CMat4Dfp32> m_lObjectMat;
	TThinArray<spCRegistry> m_lspObjectReg;
#ifdef M_ENABLE_REGISTRYCOMPILED
	TThinArray<TPtr<CRegistryCompiled> > m_lspCompiledObjectReg;
#endif

	CWRes_Template();
	~CWRes_Template();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnUnload();
};

#endif

// -------------------------------------------------------------------
//  REGISTRY
// -------------------------------------------------------------------
class CWRes_Registry : public CWResource
{
	MRTC_DECLARE;

	CWorldData* m_pWData;
	spCRegistry m_spReg;
#ifdef M_ENABLE_REGISTRYCOMPILED
	TPtr<CRegistryCompiled> m_spRegCompiled;
#endif

	bool Load();

public:
	CWRes_Registry();
	~CWRes_Registry();
	virtual CRegistry* GetRegistry();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnUnload();
};


#endif // _INC_WDATARES_CORE
