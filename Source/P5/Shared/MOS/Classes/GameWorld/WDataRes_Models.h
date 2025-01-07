#ifndef __WDATARES_MODELS_H
#define __WDATARES_MODELS_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc model resources classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWRes_Model_XMD
					CWRes_Model_Custom
					CWRes_Model_Custom_File
\*____________________________________________________________________________________________*/

#include "WDataRes_Core.h"

class CXR_Model_VariationProxy;
class CXR_Model_TriangleMesh;

// -------------------------------------------------------------------
//  XMD-MODEL
// -------------------------------------------------------------------
class CWRes_Model_XMD : public CWRes_Model
{
	MRTC_DECLARE;
	// FIXME:
	CWorldData* m_pWData;
	
	TPtr<CXR_Model_TriangleMesh> m_spModel;
	TPtr<CXR_Model_VariationProxy> m_spModelProxy;

	bool m_bExists;
	uint8 m_iVariation;


public:
	CWRes_Model_XMD();
	virtual CXR_Model* GetModel();
	virtual void ReadModel();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnUnload();
	virtual void OnPrecache(class CXR_Engine* _pEngine);
	virtual void OnPostPrecache(class CXR_Engine* _pEngine);
	virtual void OnRefresh();
	virtual void OnHibernate();
};

// -------------------------------------------------------------------
//  CUSTOM-MODEL
// -------------------------------------------------------------------
class CWRes_Model_Custom : public CWRes_Model
{
	MRTC_DECLARE;

protected:
	spCXR_Model m_spModel;

public:
	virtual CXR_Model* GetModel();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
};

// -------------------------------------------------------------------
//  CUSTOM-MODEL FILE
// -------------------------------------------------------------------
class CWRes_Model_Custom_File : public CWRes_Model_Custom
{
	MRTC_DECLARE;

	// FIXME:
	CWorldData* m_pWData;

public:
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnUnload();
};

#endif // _INC_WDATARES_MODELS
