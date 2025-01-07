
#include "PCH.h"

#include "XRVBContext.h"
#include "../MSystem/Raster/MRCCore.h"

// ----------------------------------------------------------------
// CXR_VBInfo
// ----------------------------------------------------------------
CXR_VBInfo::CXR_VBInfo()
{
	MAUTOSTRIP(CXR_VBInfo_ctor, MAUTOSTRIP_VOID);
	m_Stuff = 0;
	m_iLocal = 0;
}

int CXR_VBInfo::GetVBClass()
{ 
	MAUTOSTRIP(CXR_VBInfo_GetVBClass, 0);
	return m_Stuff & CXR_VBINFO_CLASSMASK; 
}

void CXR_VBInfo::SetVBClass(int _iClass) 
{ 
	MAUTOSTRIP(CXR_VBInfo_SetVBClass, MAUTOSTRIP_VOID);

	M_ASSERT(_iClass <= CXR_VBINFO_CLASSMASK, "Too many VBClasses");

	m_Stuff = 
		(m_Stuff & CXR_VBINFO_FLAGSMASK) + 
		(_iClass & CXR_VBINFO_CLASSMASK); 
}

int CXR_VBInfo::GetFlags()
{ 
	MAUTOSTRIP(CXR_VBInfo_GetFlags, 0);
	return (m_Stuff & CXR_VBINFO_FLAGSMASK) >> CXR_VBINFO_FLAGSSHIFT; 
}

void CXR_VBInfo::SetFlags(int _Flags) 
{ 
	MAUTOSTRIP(CXR_VBInfo_SetFlags, MAUTOSTRIP_VOID);

	M_ASSERT((_Flags << CXR_VBINFO_FLAGSSHIFT) <= CXR_VBINFO_FLAGSMASK, "Too many VBFlags");
	m_Stuff = 
		((_Flags << CXR_VBINFO_FLAGSSHIFT) & CXR_VBINFO_FLAGSMASK) + 
		(m_Stuff & CXR_VBINFO_CLASSMASK); 
}


// ----------------------------------------------------------------
//  CXR_VBContext
// ----------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_VBContext, CReferenceCount);

IMPLEMENT_OPERATOR_NEW(CXR_VBContext);


CXR_VBContext::CXR_VBContext()
{
	MAUTOSTRIP(CXR_VBContext_ctor, MAUTOSTRIP_VOID);
	m_IDCapacity = 0;
}

CXR_VBContext::~CXR_VBContext()
{
	MAUTOSTRIP(CXR_VBContext_dtor, MAUTOSTRIP_VOID);
	m_lpRC.Clear();
	m_lVBIDInfo.Clear();
};

void CXR_VBContext::Create(int _IDCapacity)
{
	MAUTOSTRIP(CXR_VBContext_Create, MAUTOSTRIP_VOID);
	m_spIDHeap = MNew(CIDHeap);
	if (!m_spIDHeap) MemError("Create");
	
	m_spIDHeap->Create(_IDCapacity);

	// Alloc an ID and check that it's zero, Index 0 is not to be used for allocations 
	// since it represents 'no vb'.
	if (m_spIDHeap->AllocID() != 0)
		Error("-", "Internal error.");

	m_IDCapacity = _IDCapacity;

	// Alloc vbinfo
	m_lVBIDInfo.SetLen(m_IDCapacity);
	
	const int MaxVBVContainers = 4096;
	m_lpVBC.SetLen(MaxVBVContainers);
	MemSet(m_lpVBC.GetBasePtr(), 0, 4 * MaxVBVContainers);
}

int CXR_VBContext::AllocID(int _iTC, int _iLocal)
{
	MAUTOSTRIP(CXR_VBContext_AllocID, 0);
	int ID = m_spIDHeap->AllocID();
	if (ID < 0) Error("AllocID", "Out of VBIDs.");
	CXR_VBInfo* pID = &m_lVBIDInfo[ID];
	pID->SetFlags(CXR_VBFLAGS_ALLOCATED);
	pID->SetVBClass(_iTC);
	pID->m_iLocal = _iLocal;
//LogFile(CStrF("AllocID (ID %d, iTC %d, iL %d, Flg %d)", ID, pID->GetVBClass(), pID->m_iLocal, pID->GetFlags()));
	return ID;
};

void CXR_VBContext::FreeID(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_FreeID, MAUTOSTRIP_VOID);
	if ((_ID < 0) || (_ID >= m_IDCapacity)) return;
	VB_MakeDirty(_ID);
	CXR_VBInfo* pID = &m_lVBIDInfo[_ID];
//LogFile(CStrF("FreeID (ID %d, iTC %d, iL %d)", _ID, pID->GetVBClass(), pID->m_iLocal));
	pID->SetVBClass(0);
	pID->SetFlags(0);
	pID->m_iLocal = 0;
	m_spIDHeap->FreeID(_ID);
};

bool CXR_VBContext::IsValidID(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_IsValidID, false);
	int iTC = m_lVBIDInfo[_ID].GetVBClass();
	int iL = m_lVBIDInfo[_ID].m_iLocal;
	if (!m_lpVBC.ValidPos(iTC)) return false;
	if (!m_lpVBC[iTC]) return false;
	int nLocal = m_lpVBC[iTC]->GetNumLocal();
	if (iL < 0 || iL >= nLocal) return false;
	return true;
}

void CXR_VBContext::VB_SetFlags(int _ID, int _Flags)
{
	MAUTOSTRIP(CXR_VBContext_VB_SetFlags, MAUTOSTRIP_VOID);
	m_lVBIDInfo[_ID].SetFlags(_Flags);
//LogFile(CStrF("VB flags %d == %d", _ID, _Flags));
}

int CXR_VBContext::VB_GetFlags(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_VB_GetFlags, 0);
	if (_ID > m_lVBIDInfo.Len())
	{
		LogFile(CStrF("FNISS!!  %d, %d", _ID, m_lVBIDInfo.Len()));
		return 0;
	}
	return m_lVBIDInfo[_ID].GetFlags();
}

int CXR_VBContext::VB_GetID(const char* _pTxtName)
{
	MAUTOSTRIP(CXR_VBContext_VB_GetID, 0);
	int nTC = m_lpVBC.Len();
	int iLocal = 0;
	CXR_VBContainer** ppTC = m_lpVBC.GetBasePtr();
	for(int i = 0; i < nTC; i++)
		if (ppTC[i]) if ((iLocal = ppTC[i]->GetLocal(_pTxtName)) >= 0)
		{
			int TxtID = ppTC[i]->GetID(iLocal);
			return TxtID;
		}

	return 0;
}

CFStr CXR_VBContext::VB_GetName(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_VB_GetName, CFStr());
	int iTC = m_lVBIDInfo[_ID].GetVBClass();
	int iL = m_lVBIDInfo[_ID].m_iLocal;
	if (!m_lpVBC[iTC]) Error("VB_GetName", CStrF("Invalid ID %d", _ID));
	return m_lpVBC[iTC]->GetName(iL);
};

int CXR_VBContext::VB_GetLocal(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_VB_GetLocal, 0);
	return m_lVBIDInfo[_ID].m_iLocal;
}

CXR_VBContainer* CXR_VBContext::VB_GetContainer(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_VB_GetContainer, NULL);
	return m_lpVBC[m_lVBIDInfo[_ID].GetVBClass()];
}

uint32 CXR_VBContext::VB_GetVBFlags(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_VB_GetVBFlags, MAUTOSTRIP_VOID);
	M_ASSERT(_ID >= 0, "Index out of range!");
	int iTC = m_lVBIDInfo[_ID].GetVBClass();
	int iL = m_lVBIDInfo[_ID].m_iLocal;
	if (!m_lpVBC[iTC]) Error("VB_Get", CStrF("Invalid VBID (ID %d, iVBC %d, iL %d)", _ID, iTC, iL));
	return m_lpVBC[iTC]->GetFlags(iL);
}

void CXR_VBContext::VB_Get(int _ID, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	MAUTOSTRIP(CXR_VBContext_VB_Get, MAUTOSTRIP_VOID);
	M_ASSERT(_ID >= 0, "Index out of range!");
	int iTC = m_lVBIDInfo[_ID].GetVBClass();
	int iL = m_lVBIDInfo[_ID].m_iLocal;
	if (!m_lpVBC[iTC]) Error("VB_Get", CStrF("Invalid VBID (ID %d, iVBC %d, iL %d)", _ID, iTC, iL));

/*	if (!(_Flags & VB_GETFLAGS_FALLBACK))
	{
		CFStr Name = VB_GetName(_ID);
		M_TRACEALWAYS("VB_Get %d, %d, %s\n", _ID, _Flags, Name.Str());
	}*/

	m_lpVBC[iTC]->Get(iL, _VB, _Flags);
}

void CXR_VBContext::VB_Release(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_VB_Release, MAUTOSTRIP_VOID);
	int iTC = m_lVBIDInfo[_ID].GetVBClass();
	int iL = m_lVBIDInfo[_ID].m_iLocal;
	if (!m_lpVBC[iTC]) Error("VB_Release", CStrF("Invalid VBID (ID %d, iVBC %d, iL %d)", _ID, iTC, iL));
	m_lpVBC[iTC]->Release(iL);
}

void CXR_VBContext::VB_MakeDirty(int _ID)
{
	MAUTOSTRIP(CXR_VBContext_VB_MakeDirty, MAUTOSTRIP_VOID);
	int nRC = m_lpRC.Len();
	for(int i = 0; i < nRC; i++)
		if (m_lpRC[i]) m_lpRC[i]->VB_GetVBIDInfo()[_ID].m_Fresh &= ~1;
}

void CXR_VBContext::Refresh()
{
	MAUTOSTRIP(CXR_VBContext_Refresh, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lpVBC.Len(); i++)
		if (m_lpVBC[i]) m_lpVBC[i]->OnRefresh();
}

int CXR_VBContext::AddRenderContext(CRenderContext* _pRC)
{
	MAUTOSTRIP(CXR_VBContext_AddRenderContext, 0);
	int iRC = -1;
	int nRC = m_lpRC.Len();
	for (int i = 0; ((i < nRC) && (iRC < 0)); i++)
		if (m_lpRC[i] == NULL) iRC = i;

	if (iRC < 0) 
		iRC = m_lpRC.Add(_pRC);
	else
		m_lpRC[iRC] = _pRC;

	return iRC;
};

void CXR_VBContext::RemoveRenderContext(int _iRC)
{
	MAUTOSTRIP(CXR_VBContext_RemoveRenderContext, MAUTOSTRIP_VOID);
	m_lpRC[_iRC] = NULL;
};

CRenderContext* CXR_VBContext::GetRenderContext(int _iRC)
{
	MAUTOSTRIP(CXR_VBContext_GetRenderContext, NULL);
	if( ( _iRC < 0 ) || ( _iRC >= m_lpRC.Len() ) )
		return NULL;

	return m_lpRC[_iRC];
}

int CXR_VBContext::AddVBContainer(CXR_VBContainer* _pVBC)
{
	MAUTOSTRIP(CXR_VBContext_AddVBContainer, 0);
	MSCOPE(CXR_VBContext::AddVBContainer, VBCONTEXT);
	int iVBC = -1;
	int nVBC = m_lpVBC.Len();
	for (int i = 0; i < nVBC; i++) 
		if (m_lpVBC[i] == NULL)
		{
			iVBC = i;
			break;
		}

	if (iVBC < 0) 
	{
		MSCOPESHORT(Add);
		ConOutL("(CXR_VBContext::AddVBContainer) Out of VBContainers, resize needed.");
		iVBC = m_lpVBC.Add(_pVBC);
	}
	else
		m_lpVBC[iVBC] = _pVBC;

	return iVBC;
};

void CXR_VBContext::RemoveVBContainer(int _iVBC)
{
	MAUTOSTRIP(CXR_VBContext_RemoveVBContainer, MAUTOSTRIP_VOID);
	m_lpVBC[_iVBC] = NULL;
};

void CXR_VBContext::Precache_Flush()
{
	for( int i = 0; i < m_lpRC.Len(); i++ )
		if( m_lpRC[i] )
		{
			m_lpRC[i]->Render_PrecacheFlush();
			m_lpRC[i]->Texture_PrecacheFlush();
			m_lpRC[i]->Geometry_PrecacheFlush();
		}
}


void CXR_VBContext::LogUsed(CStr _FileName)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
	{
		// Write resourse log
		CCFile ResourceLog;
		
		if (!_FileName.Len())
			_FileName = pSys->m_ExePath + "\\VBs.vul";

		ResourceLog.Open(_FileName, CFILE_WRITE);

		uint32 NumFound = 0;
		ResourceLog.WriteLE(NumFound);
		
		for (int i = 0; i < GetIDCapacity(); ++i)
		{
			if(VB_GetFlags(i) & CXR_VBFLAGS_PRECACHE)
			{
				CStr str = VB_GetName(i);
				str.Write(&ResourceLog);
			}
		}
		// Rewrite num found
		ResourceLog.Seek(0);
		ResourceLog.WriteLE(NumFound);
	}
}
