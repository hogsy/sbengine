
#ifndef _INC_MSURFACE
#define _INC_MSURFACE

#include "MCC.h"
#include "XRSurf.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VBOperatorContext
{
public:
	class CXR_Engine* m_pEngine;
	class CXR_VBManager* m_pVBM;
	class CXR_VertexBuffer* m_pVBHead;
	class CXR_VertexBufferGeometry* m_pVBHeadSrc;
	CMTime m_AnimTime;
	CMTime m_AnimTimeWrapped;
//	int m_VBChainIndex;
	int m_iTexChannel;
	int m_iTexChannelNext;
	int m_iFreeTexCoordSet;
	int m_VBArrayReadOnlyMask;
	const CMat4Dfp32* m_pModel2World;
	const CMat4Dfp32* m_pWorld2View;
	const CMat4Dfp32* m_pModel2View;
};

// -------------------------------------------------------------------
class CXR_VBOperatorParamDesc
{
public:
	const char* m_pName;
	const char* m_pDesc;
	fp32 m_Default;
	fp32 m_Min;
	fp32 m_Max;
};

// -------------------------------------------------------------------
class CXR_VBOperator : public CReferenceCount
{
public:
	// Editor support
	virtual CStr GetDesc() pure;
	virtual int GetNumParams() pure;
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() pure;

	// Operate
	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, 
		class CXR_VertexBuffer* _pVB) pure;
	virtual bool OnOperateFinish(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, 
		class CXR_VertexBuffer* _pVB) { return true; };
	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
	
	virtual int OnGetTextureCount(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper) const;
	virtual int OnEnumTextureID(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper, int _iEnum) const;

	virtual CStr OnDebugDump(class CXR_Engine* _pEngine, const class CXW_LayerOperation& _Oper);
};

typedef TPtr<CXR_VBOperator> spCXR_VBOperator;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SurfaceContext
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CTextureContext;

class CXW_Surface;
typedef TPtr<CXW_Surface> spCXW_Surface;

class CXW_SurfaceKeyFrame;
typedef TPtr<CXW_SurfaceKeyFrame> spCXW_SurfaceKeyFrame;

class CXR_SurfaceContext : public CReferenceCount
{
	MRTC_DECLARE;

protected:
	MRTC_CriticalSection m_Lock;

	TArray<spCXW_Surface> m_lspSurfaces;

	spCMap32 m_spHash;
	int m_HashCapacity;

	dllvirtual void CreateHashTableSafe();	// If you _MUST_ use the CreateHashTable function outside of CXR_SurfaceContext then use this one.
private:
	dllvirtual void CreateHashTable();	// This function is not thread-safe so don't call it outside of CXR_SurfaceContext (all usage of it has been threadsafed)
	TMRTC_ThreadLocal<CXW_SurfaceKeyFrame> m_TempKeyFrame;

public:
	CXR_SurfaceContext();
	virtual ~CXR_SurfaceContext();

	dllvirtual void Create();
	
	dllvirtual int GetNumSurfaces();
	dllvirtual void UpdateTextureIDs(int _SurfaceID = -1);

	dllvirtual spCKeyContainerNode LoadSurfaceScript(const char* _pFileName, bool _bIgnoreParseOption = false);
	dllvirtual TArray<spCXW_Surface> CreateSurfaces(CKeyContainerNode* _pNode);
	dllvirtual TArray<spCXW_Surface> ScanDirectory(CStr _Path, bool _bIgnoreParseOption);

	dllvirtual void AddSurfaces(TArray<spCXW_Surface> _lspSurfaces);
	dllvirtual void AddSurfaces(CCFile* _pFile);
	dllvirtual void AddSurfaces(CStr _FileName);
	dllvirtual void AddSurfaces(CKeyContainerNode* _pRoot);
	dllvirtual void AddDirectory(CStr _Path);

	dllvirtual void UpdateSurfaces(CStr _Path);

	dllvirtual CStr GetSurfaceName(int _SurfaceID);
	dllvirtual int GetSurfaceID(const char* _pName);
	dllvirtual int GetSurfaceID(uint32 _NameHash);

	dllvirtual CXW_Surface* GetSurface(int _SurfaceID);
	dllvirtual CXW_Surface* GetSurface(const char* _pName);
	dllvirtual CXW_Surface* GetSurfaceVersion(int _SurfaceID, class CXR_Engine *_pEngine);
	dllvirtual CXW_Surface* GetSurfaceVersion(int _SurfaceID, int _SurfOptions, int _SurfCaps);
	dllvirtual CXW_SurfaceKeyFrame* GetSurfaceKey(int _SurfaceID, int _SurfOptions, int _SurfCaps, CMTime _Time = CMTime(), int _iSeq = 0);
	dllvirtual CXW_SurfaceKeyFrame* GetSurfaceKey(int _SurfaceID, class CXR_Engine *_pEngine, CMTime _Time = CMTime(), int _iSeq = 0);

	dllvirtual CXW_SurfaceKeyFrame& GetTempSurfaceKeyFrame();

	// -------------------------------------------------------------------
	//  This stuff doesn't really belong here, but since this object is always created and globaly registered
	//  it was convenient to put it here. Also, the SurfaceContext has the ability to force-initialized all 
	//  surfaces and that may come in handy if the VB Class ID need to be updated in the CXW_LayerOperation:s
	//  in the surfaces.
protected:
	TArray<spCXR_VBOperator> m_lspVBOperators;
	TArray<CStr> m_lspVBOperatorNames;

public:
	virtual int VBOperator_GetOperatorID(const char* _pClassName);
	virtual CXR_VBOperator* VBOperator_Get(int _VBOperatorID);
};

typedef TPtr<CXR_SurfaceContext> spCXR_SurfaceContext;

#endif


