#include "PCH.h"
#include "WObj_AutoVar.h"
#include "MDA_Hash.h"

#ifdef AUTOVAR_LOG

class CAutoVarLogger
{
private:
	static CAutoVarLogger*	s_pAutoVarLogger;
	struct Element
	{
		int		m_SortKey;
		CStr	m_Name;
		Element() : m_SortKey(0) {}
	};

	CStringHash			m_pHash;
	TThinArray<Element>	m_lCounters;

	CAutoVarLogger();

public:
	static CAutoVarLogger& GetAutoVarLogger();
	~CAutoVarLogger();

	void DumpLog();
	void Log(const char* _Str);
};


CAutoVarLogger* CAutoVarLogger::s_pAutoVarLogger = NULL;

CAutoVarLogger::CAutoVarLogger()
{
	m_pHash.Create(20000); 
	m_lCounters.SetLen(20000);
}

CAutoVarLogger& CAutoVarLogger::GetAutoVarLogger() 
{ 
	if(!s_pAutoVarLogger) 
		s_pAutoVarLogger = DNew(CAutoVarLogger) CAutoVarLogger; 
	return *s_pAutoVarLogger;
}

CAutoVarLogger::~CAutoVarLogger()
{
}

void CAutoVarLogger::DumpLog()
{
	RadixSort(m_lCounters.GetBasePtr(), 20000);

	ConOutL("Dumping AutoVarLog");
	LogFile("--------------");
	LogFile("- AutoVarLog -");
	LogFile("--------------");
	for(int i = 19999; i > -1; i--)
	{
		if(m_lCounters[i].m_SortKey > 0)
		{
			CStr str = CStrF("%i - %s", m_lCounters[i].m_SortKey, m_lCounters[i].m_Name.Str());
			LogFile(str.Str());
			m_lCounters[i].m_SortKey = 0;
			m_lCounters[i].m_Name.Clear();
		}
	}
	ConOutL("Done");
}

void CAutoVarLogger::Log(const char* _Str)
{
	int index = m_pHash.GetIndex(_Str); 
	if(index == -1)
	{
		int i = 0;
		for(; i < 20000; i++)
		{
			if(m_lCounters[i].m_SortKey == 0)
			{
				break;
			}
		}
		m_pHash.Insert(i, _Str);
		index = m_pHash.GetIndex(_Str);
	}
	if(index < 20000 && index != -1) 
	{
		m_lCounters[index].m_SortKey++; 
		m_lCounters[index].m_Name = _Str;
	}
}

void AutoVarLog(const char* _str)
{
	CAutoVarLogger::GetAutoVarLogger().Log(_str);
}

void DumpAutoVarLog()
{
	CAutoVarLogger::GetAutoVarLogger().DumpLog();
}

#endif


bool CAutoVarContainer::AutoVar_Pack(int _DirtyMask, uint8*& _pD, CMapData* _pMapData, int _Debug) const
{
	MSCOPESHORT(AutoVar_Pack);
	if (_DirtyMask == 0)
		return false;

	uint8*&pD = const_cast<uint8*&>(_pD);

	int Mask = _DirtyMask;
	while (true)
	{
		uint8 PackedMask = Mask & 0x7f;
		if (Mask >= 128)
			PackedMask |= 0x80;
		PTR_PUTINT8(pD, PackedMask);
		Mask >>= 7;
		if (Mask == 0)
			break;
	}

	//This function should not be const. But CWObject::OnCreateClientUpdate is, so for now...
	void* p = &_pD;
	const_cast<CAutoVarContainer*>(this)->AutoVar_OnOperate(AUTOVAR_TYPE_PACK | _Debug, _DirtyMask, (void*)p, _pMapData);
	
	return true;
}


bool CAutoVarContainer::AutoVar_Unpack(const uint8*& _pD, CMapData* _pMapData, int _Debug)
{
	MSCOPESHORT(AutoVar_Unpack);
	int DirtyMask = 0;
	int iShift = 0;
	while (true)
	{
		uint8 PackedMask;
		PTR_GETINT8(_pD, PackedMask);
		DirtyMask |= (PackedMask & 0x7f) << iShift;
		iShift += 7;
		if (!(PackedMask & 0x80))
			break;
	}

	if (DirtyMask == 0)
		return false;

	//Because we want to use the same function for Pack and Unpack we have to cast it in this way
	void* p = &_pD;
	AutoVar_OnOperate(AUTOVAR_TYPE_UNPACK | _Debug, DirtyMask, p, _pMapData);
	return true;
}


void CAutoVarContainer::AutoVar_Write(CCFile* _pFile, CMapData* _pMapData)
{
	MSCOPESHORT(AutoVar_Write);

	uint32 DirtyMask = 0;
	AutoVar_OnOperate(AUTOVAR_TYPE_GETDIRTYMASK, 0, &DirtyMask, _pMapData);

	const uint BufSize = 4096;
	uint8 Data[BufSize];
	uint8* pData = Data; 
	TAutoVar_Pack(DirtyMask, pData);
	AutoVar_OnOperate(AUTOVAR_TYPE_PACK, DirtyMask, &pData, _pMapData);

	uint32 Size = pData - (uint8*)&Data;
	if (Size > BufSize)
		Error_static("CAutoVarContainer::AutoVar_Write", "Write-buffer too small");

	_pFile->WriteLE(Size);
	_pFile->WriteLE(Data, Size);
}


void CAutoVarContainer::AutoVar_Read(CCFile* _pFile, CMapData* _pMapData)
{
	MSCOPESHORT(AutoVar_Read);

	const uint BufSize = 4096;
	uint8 Data[BufSize];

	uint32 Size;
	_pFile->ReadLE(Size);
	if (Size > BufSize)
		Error_static("CAutoVarContainer::AutoVar_Read", "Read-buffer too small");

	_pFile->ReadLE(Data, Size);
	const uint8* pData = Data;

	uint32 DirtyMask;
	TAutoVar_Unpack(DirtyMask, pData);
	AutoVar_OnOperate(AUTOVAR_TYPE_UNPACK, DirtyMask, &pData, _pMapData);
}



//old stuff:
/////////////////////////////////////////////////////////////////
// These macros are used to automatically create the OnPack function
/*
#define AUTOVAR_PACK_BEGIN(Class, BaseClass) \
										void AutoVar_CopyFrom(const Class& _CD)\
										{\
											AutoVar_OnOperate(AUTOVAR_TYPE_COPY, 0, const_cast<Class*>(&_CD), NULL);\
										}\
\
										virtual void AutoVar_OnOperate(int _iType, int _DirtyMask, void* _pContext, CMapData* _pMapData)\
										{\
											BaseClass::AutoVar_OnOperate(_iType, _DirtyMask, _pContext, _pMapData);\
											Class* pObj = (Class *)_pContext;
#define AUTOVAR_PACK_VAR(Var) \
											if(_iType == AUTOVAR_TYPE_PACK)\
											{\
												uint8*& pD = *(uint8**)_pContext; \
												if(_DirtyMask & Var.GetDirtyMask())\
													Var.Pack(pD, _pMapData);\
											}\
											else if(_iType == AUTOVAR_TYPE_UNPACK)\
											{\
												const uint8*& pD = *(const uint8**)_pContext; \
												if(_DirtyMask & Var.GetDirtyMask())\
													Var.Unpack(pD, _pMapData);\
											}\
											else if(_iType == AUTOVAR_TYPE_COPY)\
												Var.CopyFrom(pObj->Var);\
											else if(_iType == AUTOVAR_TYPE_GETDIRTYMASK)\
												(*(int *)_pContext) |= Var.GetDirtyMask();
#define AUTOVAR_PACK_END \
										}
*/
