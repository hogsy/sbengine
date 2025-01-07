#ifndef __WDATARES_XW_H
#define __WDATARES_XW_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Resources found in XW-files

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWRes_XWIndex
					CWRes_Model_XW
					CWRes_Model_XW2
					CWRes_Model_XW3
					CWRes_XWResource
					CWRes_XWNavGrid
					CWRes_XWNavGraph
					CWRes_XW

	History:		
		020811:		Created File
\*____________________________________________________________________________________________*/


#include "WDataRes_Core.h"

// #include "../../XRModels/Model_BSP/WBSPModel.h"
// #include "../../XRModels/Model_BSP2/WBSP2Model.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-Index
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for a CXR_Model_BSP2
\*____________________________________________________________________*/

class CWRes_XWIndex : public CWResource
{
	MRTC_DECLARE;

	TArray<uint32> m_liModelOffsets;	

public:
	virtual int GetModelFileOffset(int _iModel);
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
};

typedef TPtr<CWRes_XWIndex> spCWRes_XWIndex;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-Model
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for a CXR_Model_BSP
\*____________________________________________________________________*/

class CWRes_Model_XW : public CWRes_Model
{
	MRTC_DECLARE;

protected:
	TPtr<CXR_Model> m_spModel;
	CWorldData* m_pWData;

	virtual void CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const class CBSP_CreateInfo& _CreateInfo);
	
public:
	virtual CXR_Model* GetModel();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass, spCXR_Model _spModel);
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual bool ReadAllModels(CWorldData* _pWData, const char* _pClassName);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW2-Model
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for a CXR_Model_BSP2
\*____________________________________________________________________*/

class CWRes_Model_XW2 : public CWRes_Model_XW
{
	MRTC_DECLARE;

protected:
	virtual void CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const class CBSP_CreateInfo& _CreateInfo);

public:
	virtual void OnLoad();
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for a CXR_Model_BSP3
\*____________________________________________________________________*/

class CWRes_Model_XW3 : public CWRes_Model_XW
{
	MRTC_DECLARE;

protected:
	virtual void CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const class CBSP_CreateInfo& _CreateInfo);

public:
	virtual void OnLoad();
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for a CXR_Model_BSP4
\*____________________________________________________________________*/

class CWRes_Model_XW4 : public CWRes_Model_XW
{
	MRTC_DECLARE;

protected:
	virtual void CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const class CBSP_CreateInfo& _CreateInfo);

public:
	virtual void OnLoad();
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for a CXR_Model_BSP4Glass
\*____________________________________________________________________*/
class CWRes_Model_Glass : public CWRes_Model_XW
{
	MRTC_DECLARE;

protected:
	virtual void CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const class CBSP_CreateInfo& _CreateInfo);

public:
	virtual void OnLoad();
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-Resource
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource wrapper for a XW-file resource
\*____________________________________________________________________*/

class CWRes_XWResource : public CWResource
{
	MRTC_DECLARE;

public:
	uint8* m_pData;
	int m_Size;
	bool m_bExists;

public:
	CWRes_XWResource();
	~CWRes_XWResource();
	virtual void* GetData();
	virtual int GetDataSize();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);

	TAP<const uint8> GetResourceData(uint _iResData);

// Tool stuff (for XWC & Ogier)
#ifndef PLATFORM_CONSOLE
	typedef TArray<uint8> CResourceData;
	static const char* ReadResources(CCFile& _File, TArray<CResourceData>& _lRet);
	static void WriteResources(const TArray<CResourceData>& _lResources, CCFile& _File);
#endif
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-Navgrid
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource wrapper for a XW navigation grid
\*____________________________________________________________________*/

class CWRes_XWNavGrid : public CWResource
{
	MRTC_DECLARE;

	TPtr<class CXR_BlockNav_Grid_GameWorld> m_spNavGrid;
	CWorldData* m_pWData;
	
public:
	~CWRes_XWNavGrid();
	virtual class CXR_BlockNav_Grid_GameWorld* GetNavGrid();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnUnload();
};

typedef TPtr<CWRes_XWNavGrid> spCWRes_XWNavGrid;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-Navgraph
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource wrapper for a XW second layer navigation graph
\*__________________________________________________________________________________*/

class CWRes_XWNavGraph : public CWResource
{
	MRTC_DECLARE;

	TPtr<class CXR_NavGraph> m_spNavGraph;
	CWorldData* m_pWData;
	
public:
	virtual class CXR_NavGraph* GetNavGraph();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnUnload();
};

typedef TPtr<CWRes_XWNavGraph> spCWRes_XWNavGraph;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-File
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for a XW-file
\*____________________________________________________________________*/

class CWRes_XW : public CWResource
{
	MRTC_DECLARE;

public:
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for the "OBJECTATTRIBS" chunk in a XW-file
\*_____________________________________________________________________________________*/
class CWRes_ObjectAttribs : public CWResource
{
	MRTC_DECLARE;

	CRegistryDirectory *m_pRegDir;
	TArray<spCRegistry> m_lspObjectRegs;
	CRegistryCompiled m_CompiledReg;
	spCRegistry m_spCompiledRegRoot;

public:
	CWRes_ObjectAttribs()
	{
		m_pRegDir = NULL;
	}
	~CWRes_ObjectAttribs()
	{
		if (m_pRegDir)
			delete m_pRegDir;
	}


	class CRegDirConst : public CRegistryDirectory
	{
	public:
		CRegDirConst(CWRes_ObjectAttribs *_pThis)
		{
			m_pThis = _pThis;
		}
		CWRes_ObjectAttribs *m_pThis;
		virtual int NumReg()
		{
			return m_pThis->m_spCompiledRegRoot->GetNumChildren();
		}

		virtual CRegistry *GetReg(int _iReg)
		{
			return m_pThis->m_spCompiledRegRoot->GetChild(_iReg);
		}
	};
	class CRegDirNormal : public CRegistryDirectory
	{
	public:
		CRegDirNormal(CWRes_ObjectAttribs *_pThis)
		{
			m_pThis = _pThis;
		}
		CWRes_ObjectAttribs *m_pThis;
		virtual int NumReg()
		{
			return m_pThis->m_lspObjectRegs.Len();
		}

		virtual CRegistry *GetReg(int _iReg)
		{
			return m_pThis->m_lspObjectRegs[_iReg];
		}
	};

	CRegistryDirectory *GetRegDir()
	{
		return m_pRegDir;
	}

	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
};



#endif // _INC_WDATARES_XW
