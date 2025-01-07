
#ifndef _INC_XRVBCONTEXT
#define _INC_XRVBCONTEXT

#include "XR.h"

/*
// +-----------------------------------------------------------------+
// | VB-Context classes (XR core components)                         |
// +-----------------------------------------------------------------+
// | Creator:          Magnus Högdahl                                |
// | Created:          2000-11-06                                    |
// | Last modified:    2000-11-06                                    |
// |                                                                 |
// | Description:                                                    |
// |                                                                 |
// | bla...                                                          |
// |                                                                 |
// +-----------------------------------------------------------------+
// | Copyright O3 Games AB 2000                                      |
// +-----------------------------------------------------------------+
*/


/*
	Naming abbrevations related to XRVBContext

	CRC,RC		CRenderContext
	VB			CRC_VertexBuffer, (not to be confused with CXR_VertexBuffer)
	VBC			CXR_VBContainer
	VBCtx		CXR_VBContext

*/

// ----------------------------------------------------------------
//  CXR_VBContext
// ----------------------------------------------------------------
#define CXR_VBINFO_CLASSMASK		0x0fff
#define CXR_VBINFO_FLAGSMASK		0xf000
#define CXR_VBINFO_FLAGSSHIFT		12

#define CXR_VBFLAGS_PRECACHE		1
#define CXR_VBFLAGS_ALLOCATED		2

class CXR_VBInfo
{
	int16 m_Stuff;				// Reg. texture constructor class.

public:
	uint16 m_iLocal;			// VBClass local texture index

	CXR_VBInfo();

	int GetVBClass();
	void SetVBClass(int _iClass);
	int GetFlags();
	void SetFlags(int _Flags);
};

// ----------------------------------------------------------------
class CXR_VBContainer;

class CXR_VBContext : public CReferenceCount
{
	MRTC_DECLARE;

	TArray<CXR_VBContainer*> m_lpVBC;
	TArray<CXR_VBInfo> m_lVBIDInfo;
	int m_IDCapacity;
	TPtr<CIDHeap> m_spIDHeap;

	TList_Vector<CRenderContext*> m_lpRC;

	int AllocRCID(int _iRC, int _tnr);
	void FreeRCID(int _iRC, int _ID);

public:

	DECLARE_OPERATOR_NEW


	CXR_VBContext();
	~CXR_VBContext();

	virtual void Create(int _IDCapacity);

	// VB ID Allocation. For VB Container use only.
	virtual int GetIDCapacity() { return m_IDCapacity; };
	virtual int AllocID(int _iTC, int _iLocal);
	virtual void FreeID(int _ID);
	virtual bool IsValidID(int _ID);

	virtual int AddVBContainer(CXR_VBContainer* _pTClass);
	virtual void RemoveVBContainer(int _iTClass);

	// Get/Set VB properties
	virtual void VB_SetFlags(int _ID, int _Flags);
	virtual int VB_GetFlags(int _ID);
	virtual void VB_MakeDirty(int _ID);
	virtual CFStr VB_GetName(int _ID);

	// Get VB
	virtual int VB_GetID(const char* _pTxtName);
	virtual void VB_Get(int _ID, CRC_BuildVertexBuffer& _VB, int _Flags);
	virtual uint32 VB_GetVBFlags(int _ID);
	virtual void VB_Release(int _ID);

	// Get local & container from global ID.
	virtual int VB_GetLocal(int _ID);
	dllvirtual CXR_VBContainer* VB_GetContainer(int _ID);

	virtual void Refresh();

	// Add/Remove render-context. For RC use only.
	virtual int AddRenderContext(CRenderContext* _pRC);
	virtual void RemoveRenderContext(int _iRC);
	virtual CRenderContext* GetRenderContext(int _iRC);

	virtual void Precache_Flush();
};

class CPrecacheVBCompare
{
public:
	static int Compare(CXR_VBContext* _pContext, uint16 _i0, uint16 _i1)
	{
		CXR_VBContainer *pTCFirst = _pContext->VB_GetContainer(_i0);
		CXR_VBContainer *pTCSecond = _pContext->VB_GetContainer(_i1);
		if (pTCFirst != pTCSecond)
		{
			const char *pFirstStr = pTCFirst->GetContainerSortName();
			const char *pSecondStr = pTCSecond->GetContainerSortName();
			if (pFirstStr != pSecondStr)
			{
				int iCmp = CStrBase::stricmp(pFirstStr, pSecondStr);
				if (iCmp != 0)
					return iCmp;
			}
		}

		int iLocalFirst = _pContext->VB_GetLocal(_i0);
		int iLocalSecond = _pContext->VB_GetLocal(_i1);
		if (iLocalFirst > iLocalSecond)
			return 1;
		else if (iLocalFirst < iLocalSecond)
			return -1;

		return 0;
	}
};

typedef TPtr<CXR_VBContext> spCXR_VBContext;

#endif // _INC_XRVBCONTEXT
