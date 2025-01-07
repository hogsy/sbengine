//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "../../MSystem/MSystem.h"
#include "AnimGraph.h"

//--------------------------------------------------------------------------------

static void ReadFStr(CCFile* _pFile, CFStr& _Str)
{
	char Temp[256];
	uint32 Len = 0;
	_pFile->ReadLE(Len);
	Len = Min((uint32)200, Len);
	_pFile->ReadLE((uint8*)Temp, Len + 1);
	_Str = (char*)Temp;
}

//--------------------------------------------------------------------------------

static void WriteFStr(CCFile* _pFile, CFStr& _Str)
{
#ifndef	PLATFORM_CONSOLE
	uint32 Len = Min(200, _Str.Len());
	_pFile->WriteLE(Len);
	_pFile->WriteLE((uint8*)_Str.Str(), Len + 1);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

template<class T>
void ReadArray(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer)
{
	ReadArrayImp<T, T::EIOWholeStruct>(_lArray, _pDFile, _CurrentVer);
}

template<class T>
void ReadArrayWholeStruct(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer)
{
	ReadArrayImp<T, 1>(_lArray, _pDFile, _CurrentVer);
}

template<class T>
void ReadArrayFallBack(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer)
{
	ReadArrayImp<T, 0>(_lArray, _pDFile, _CurrentVer);
}

template<class T, int _WholeStruct>
void ReadArrayImp(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer)
{
	CCFile* pFile = _pDFile->GetFile();
	
	uint32 ElementSize = 0;
	int Version = _pDFile->GetUserData2();
	if (Version > 0x0100)
		pFile->ReadLE(ElementSize);

	_lArray.SetLen(_pDFile->GetUserData());
	T* pArray = _lArray.GetBasePtr();
	if ((_lArray.ListSize() == (_pDFile->GetEntrySize()-4)) && 
		(_pDFile->GetUserData2() == _CurrentVer) && _WholeStruct)
	{
		M_ASSERT(ElementSize == sizeof(T), "Must be same");
		pFile->Read(pArray, _lArray.ListSize());
#ifndef CPU_LITTLEENDIAN
		SwapLEArray(_lArray);
#endif
	}
	else
	{
		ReadArray_PerElementFallback(_lArray, _pDFile, ElementSize);
	}
}

//--------------------------------------------------------------------------------

template<class T>
void WriteArray(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer, const char* _pEntryName)
{
	CCFile* pFile = _pDFile->GetFile();

	_pDFile->BeginEntry(_pEntryName);
	uint32 ElementSize = sizeof(T);
	pFile->WriteLE(ElementSize);
	WriteArray_PerElementFallback(_lArray, _pDFile);
	_pDFile->EndEntry(_lArray.Len(), _CurrentVer);
}

//--------------------------------------------------------------------------------
//- class T ----------------------------------------------------------------------
//--------------------------------------------------------------------------------

template<class T>
void ReadArray_PerElementFallback(TThinArray<T>& _lArray, CDataFile* _pDFile, uint32 _ElementSize)
{
	int Version = _pDFile->GetUserData2();
	CCFile* pFile = _pDFile->GetFile();

	T* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
	{
		int Pos = pFile->Pos();
		pArray[i].Read(pFile, Version);
		if (Version > 0x0100 && T::EIOWholeStruct)
		{
			int Pad = _ElementSize - (pFile->Pos() - Pos);
			uint8 Temp[32] = {0};
			pFile->Read(Temp, Pad);
		}
	}
}

//--------------------------------------------------------------------------------

template<class T>
void WriteArray_PerElementFallback(TThinArray<T>& _lArray, CDataFile* _pDFile)
{
	CCFile* pFile = _pDFile->GetFile();
	T* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
	{
		int Pos = pFile->Pos();
		pArray[i].Write(pFile);
		if (T::EIOWholeStruct)
		{
			mint Pad = sizeof(T) - (pFile->Pos() - Pos);
			uint8 Temp[32] = {0};
			pFile->Write(Temp, Pad); // Pad struct
		}
	}
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
template<class T>
void SwapLEArray(TThinArray<T>& _lArray)
{
	T* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
		pArray[i].SwapLE();
}
#endif

//--------------------------------------------------------------------------------
//- int16 -----------------------------------------------------------------------
//--------------------------------------------------------------------------------

template<>
void ReadArray_PerElementFallback(TThinArray<int16>& _lArray, CDataFile* _pDFile, uint32 _ElementSize)
{
	CCFile* pFile = _pDFile->GetFile();
	int16* pArray = _lArray.GetBasePtr();
	pFile->ReadLE(pArray, _lArray.Len());
}

//--------------------------------------------------------------------------------

template<>
void WriteArray_PerElementFallback(TThinArray<int16>& _lArray, CDataFile* _pDFile)
{
#ifndef	PLATFORM_CONSOLE
	CCFile* pFile = _pDFile->GetFile();
	int16* pArray = _lArray.GetBasePtr();
	pFile->WriteLE(pArray, _lArray.Len());
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
template<>
void SwapLEArray(TThinArray<int16>& _lArray)
{
	int16* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
		::SwapLE(pArray[i]);
}
#endif

//--------------------------------------------------------------------------------
//- fp32 --------------------------------------------------------------------------
//--------------------------------------------------------------------------------

template<>
void ReadArray_PerElementFallback(TThinArray<fp32>& _lArray, CDataFile* _pDFile, uint32 _ElementSize)
{
	CCFile* pFile = _pDFile->GetFile();
	fp32* pArray = _lArray.GetBasePtr();
	pFile->ReadLE((uint32*)pArray, _lArray.Len());
}

//--------------------------------------------------------------------------------

template<>
void WriteArray_PerElementFallback(TThinArray<fp32>& _lArray, CDataFile* _pDFile)
{
#ifndef	PLATFORM_CONSOLE
	CCFile* pFile = _pDFile->GetFile();
	fp32* pArray = _lArray.GetBasePtr();
	pFile->WriteLE((uint32*)pArray, _lArray.Len());
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
template<>
void SwapLEArray(TThinArray<fp32>& _lArray)
{
	fp32* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
		::SwapLE(pArray[i]);
}
#endif

//--------------------------------------------------------------------------------
//- CFStr ------------------------------------------------------------------------
//--------------------------------------------------------------------------------

template<>
void ReadArray_PerElementFallback(TThinArray<CFStr>& _lArray, CDataFile* _pDFile, uint32 _ElementSize)
{
	CCFile* pFile = _pDFile->GetFile();
	CFStr* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
		ReadFStr(pFile, pArray[i]);
}

//--------------------------------------------------------------------------------

template<>
void WriteArray_PerElementFallback(TThinArray<CFStr>& _lArray, CDataFile* _pDFile)
{
#ifndef	PLATFORM_CONSOLE
	CCFile* pFile = _pDFile->GetFile();
	CFStr* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
		WriteFStr(pFile, pArray[i]);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
template<>
void SwapLEArray(TThinArray<CFStr>& _lArray)
{
// No need to swap endian for single bytes.
/*	
	CFStr* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
		for (int c = 0; c < pArray[i].Len(); c++)
			::SwapLE((uint8)(pArray[i].Str()[c]));
*/
}
#endif

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_EffectInstance::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		{
			_pFile->ReadLE(m_ID);
			_pFile->ReadLE(m_iParams);
			_pFile->ReadLE(m_nParams);
			uint8 Pad;
			_pFile->ReadLE(Pad);
		}
		break;

		case 0x0101:
		{
			_pFile->ReadLE(m_iParams);
			_pFile->ReadLE(m_ID);
			_pFile->ReadLE(m_nParams);
		}
		break;

		default:
			Error_static("CXRAG_AnimInstance::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG_EffectInstance::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_iParams);
	_pFile->WriteLE(m_ID);
	_pFile->WriteLE(m_nParams);
#endif	// PLATFORM_CONSOLE
}
#ifndef CPU_LITTLEENDIAN
void CXRAG_EffectInstance::SwapLE()
{
	::SwapLE(m_iParams);
	::SwapLE(m_ID);
	::SwapLE(m_nParams);
}
#endif
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_StateConstant::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		{
			uint16 Pad0;
			_pFile->ReadLE(Pad0);
			uint8 Pad1;
			_pFile->ReadLE(Pad1);
			_pFile->ReadLE(m_ID);
			_pFile->ReadLE(m_Value);
		}
		break;
		case 0x0101:
		{
			_pFile->ReadLE(m_Value);
			_pFile->ReadLE(m_ID);
		}
		break;

		default:
			Error_static("CXRAG_AnimInstance::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG_StateConstant::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_Value);
	_pFile->WriteLE(m_ID);
#endif	// PLATFORM_CONSOLE
}

#ifndef CPU_LITTLEENDIAN
void CXRAG_StateConstant::SwapLE()
{
	::SwapLE(m_Value);
	::SwapLE(m_ID);
}
#endif
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_ConditionNode::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		case 0x0101:
		{
			_pFile->ReadLE(m_Shared32);
			_pFile->ReadLE(m_Constant);
		}
		break;

		default:
			Error_static("CXRAG_AnimInstance::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG_ConditionNode::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_Shared32);
	_pFile->WriteLE(m_Constant);
#endif	// PLATFORM_CONSOLE
}
#ifndef CPU_LITTLEENDIAN
void CXRAG_ConditionNode::SwapLE()
{
	::SwapLE(m_Shared32);
	::SwapLE(m_Constant);
}
#endif
//--------------------------------------------------------------------------------

void CXRAG_ActionHashEntry::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case 0x0100:
		{
			_pFile->ReadLE(m_HashKey);
			_pFile->ReadLE(m_iAction);
			uint16 Pad;
			_pFile->ReadLE(Pad);
		}
		break;
	case 0x0101:
		{
			_pFile->ReadLE(m_HashKey);
			_pFile->ReadLE(m_iAction);
		}
		break;

	default:
		Error_static("CXRAG_Action::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG_ActionHashEntry::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_HashKey);
	_pFile->WriteLE(m_iAction);
#endif	// PLATFORM_CONSOLE
}


#ifndef CPU_LITTLEENDIAN
void CXRAG_ActionHashEntry::SwapLE()
{
	::SwapLE(m_HashKey);
	::SwapLE(m_iAction);
}
#endif
//--------------------------------------------------------------------------------

void CXRAG_NameAndID::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case 0x0100:
		{
			ReadFStr(_pFile, m_Name);
			_pFile->ReadLE(m_ID);
			uint8 Pad0;
			_pFile->ReadLE(Pad0);
			uint16 Pad1;
			_pFile->ReadLE(Pad1);
		}
		break;
	case 0x0101:
		{
			ReadFStr(_pFile, m_Name);
			_pFile->ReadLE(m_ID);
		}
		break;

	default:
		Error_static("CXRAG_Action::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG_NameAndID::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	WriteFStr(_pFile, m_Name);
	_pFile->WriteLE(m_ID);
#endif	// PLATFORM_CONSOLE
}

#ifndef CPU_LITTLEENDIAN
void CXRAG_NameAndID::SwapLE()
{
	::SwapLE(m_ID); // Redundant but consistent.
}
#endif
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG::Read(CDataFile* _pDFile)
{
	CCFile* pFile = _pDFile->GetFile();

	if(!_pDFile->GetNext("NAME"))
		Error("Read", "No NAME entry found.");
	{
		char pName[1024];
		pFile->Read(pName, Min((int32)1023, _pDFile->GetEntrySize()));
		m_Name = pName;
	}

	// States
	if(!_pDFile->GetNext("FULLSTATES"))
		Error("Read", "No FULLSTATES entry found.");
	ReadArray(m_lFullStates, _pDFile, XR_ANIMGRAPH_VERSION);

	if(!_pDFile->GetNext("SMALLSTATES"))
		Error("Read", "No SMALLSTATES entry found.");
	ReadArray(m_lSmallStates, _pDFile, XR_ANIMGRAPH_VERSION);

	// AnimLayers
	if(!_pDFile->GetNext("FULLANIMLAYERS"))
		Error("Read", "No FULLANIMLAYERS entry found.");
	ReadArray(m_lFullAnimLayers, _pDFile, XR_ANIMGRAPH_VERSION);

	if(!_pDFile->GetNext("SMALLANIMLAYERS"))
		Error("Read", "No SMALLANIMLAYERS entry found.");
	ReadArray(m_lSmallAnimLayers, _pDFile, XR_ANIMGRAPH_VERSION);

	if(!_pDFile->GetNext("ANIMLAYERSMAP"))
		Error("Read", "No ANIMLAYERSMAP entry found.");
	ReadArrayWholeStruct(m_lAnimLayersMap, _pDFile, XR_ANIMGRAPH_VERSION);

	// Actions
	if(!_pDFile->GetNext("FULLACTIONS"))
		Error("Read", "No FULLACTIONS entry found.");
	ReadArray(m_lFullActions, _pDFile, XR_ANIMGRAPH_VERSION);

	if(!_pDFile->GetNext("SMALLACTIONS"))
		Error("Read", "No SMALLACTIONS entry found.");
	ReadArray(m_lSmallActions, _pDFile, XR_ANIMGRAPH_VERSION);

	if(!_pDFile->GetNext("ACTIONSMAP"))
		Error("Read", "No ACTIONSMAP entry found.");
	ReadArrayWholeStruct(m_lActionsMap, _pDFile, XR_ANIMGRAPH_VERSION);

	// Nodes
	if(!_pDFile->GetNext("NODES"))
		Error("Read", "No NODES entry found.");
	ReadArray(m_lNodes, _pDFile, XR_ANIMGRAPH_VERSION);

	// Effects
	if(!_pDFile->GetNext("EFFECTS"))
		Error("Read", "No EFFECTS entry found.");
	ReadArray(m_lEffectInstances, _pDFile, XR_ANIMGRAPH_VERSION);

	// CallbackParams
	if(!_pDFile->GetNext("CALLBACKPARAMS"))
		Error("Read", "No CALLBACKPARAMS entry found.");
	ReadArrayWholeStruct(m_lCallbackParams, _pDFile, XR_ANIMGRAPH_VERSION);

	// StateConstants
	if(!_pDFile->GetNext("STATECONSTANTS"))
		Error("Read", "No STATECONSTANTS entry found.");
	ReadArray(m_lStateConstants, _pDFile, XR_ANIMGRAPH_VERSION);

	// ActionHashEntries
	if(!_pDFile->GetNext("ACTIONHASHENTRIES"))
		Error("Read", "No ACTIONHASHENTRIES entry found.");
	ReadArray(m_lActionHashEntries, _pDFile, XR_ANIMGRAPH_VERSION);

#ifndef M_RTM
	// ExportedNames (Optional)
	if(_pDFile->GetNext("EXPORTEDACTIONNAMES"))
		ReadArrayFallBack(m_lExportedActionNames, _pDFile, XR_ANIMGRAPH_VERSION);

	if(_pDFile->GetNext("EXPORTEDSTATENAMES"))
		ReadArrayFallBack(m_lExportedStateNames, _pDFile, XR_ANIMGRAPH_VERSION);

	if(_pDFile->GetNext("EXPORTEDPROPERTYNAMES"))
		ReadArray(m_lExportedPropertyNames, _pDFile, XR_ANIMGRAPH_VERSION);

	if(_pDFile->GetNext("EXPORTEDOPERATORNAMES"))
		ReadArray(m_lExportedOperatorNames, _pDFile, XR_ANIMGRAPH_VERSION);

	if(_pDFile->GetNext("EXPORTEDEFFECTNAMES"))
		ReadArray(m_lExportedEffectNames, _pDFile, XR_ANIMGRAPH_VERSION);

	if(_pDFile->GetNext("EXPORTEDSTATECONSTNAMES"))
		ReadArray(m_lExportedStateConstantNames, _pDFile, XR_ANIMGRAPH_VERSION);
#endif
}

//--------------------------------------------------------------------------------

void CXRAG::Write(CDataFile* _pDFile)
{
#ifndef	PLATFORM_CONSOLE
	CCFile* pFile = _pDFile->GetFile();

	{
		_pDFile->BeginEntry("NAME");
		pFile->Write(m_Name.Str(), m_Name.Len() + 1);
		_pDFile->EndEntry(m_Name.Len(), XR_ANIMGRAPH_VERSION);
	}

	WriteArray(m_lFullStates, _pDFile, XR_ANIMGRAPH_VERSION, "FULLSTATES");
	WriteArray(m_lSmallStates, _pDFile, XR_ANIMGRAPH_VERSION, "SMALLSTATES");

	WriteArray(m_lFullAnimLayers, _pDFile, XR_ANIMGRAPH_VERSION, "FULLANIMLAYERS");
	WriteArray(m_lSmallAnimLayers, _pDFile, XR_ANIMGRAPH_VERSION, "SMALLANIMLAYERS");
	WriteArray(m_lAnimLayersMap, _pDFile, XR_ANIMGRAPH_VERSION, "ANIMLAYERSMAP");

	WriteArray(m_lFullActions, _pDFile, XR_ANIMGRAPH_VERSION, "FULLACTIONS");
	WriteArray(m_lSmallActions, _pDFile, XR_ANIMGRAPH_VERSION, "SMALLACTIONS");
	WriteArray(m_lActionsMap, _pDFile, XR_ANIMGRAPH_VERSION, "ACTIONSMAP");

	WriteArray(m_lNodes, _pDFile, XR_ANIMGRAPH_VERSION, "NODES");
	WriteArray(m_lEffectInstances, _pDFile, XR_ANIMGRAPH_VERSION, "EFFECTS");
	WriteArray(m_lCallbackParams, _pDFile, XR_ANIMGRAPH_VERSION, "CALLBACKPARAMS");
	WriteArray(m_lStateConstants, _pDFile, XR_ANIMGRAPH_VERSION, "STATECONSTANTS");

	// FIXME: Sort by HashKey, to support binary search in the other end...
	WriteArray(m_lActionHashEntries, _pDFile, XR_ANIMGRAPH_VERSION, "ACTIONHASHENTRIES");

#ifndef	M_RTM
	WriteArray(m_lExportedActionNames, _pDFile, XR_ANIMGRAPH_VERSION, "EXPORTEDACTIONNAMES");
	WriteArray(m_lExportedStateNames, _pDFile, XR_ANIMGRAPH_VERSION, "EXPORTEDSTATENAMES");
	WriteArray(m_lExportedPropertyNames, _pDFile, XR_ANIMGRAPH_VERSION, "EXPORTEDPROPERTYNAMES");
	WriteArray(m_lExportedOperatorNames, _pDFile, XR_ANIMGRAPH_VERSION, "EXPORTEDOPERATORNAMES");
	WriteArray(m_lExportedEffectNames, _pDFile, XR_ANIMGRAPH_VERSION, "EXPORTEDEFFECTNAMES");
	WriteArray(m_lExportedStateConstantNames, _pDFile, XR_ANIMGRAPH_VERSION, "EXPORTEDSTATECONSTNAMES");
#endif	// M_RTM
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
void CXRAG::SwapLE()
{
	SwapLEArray(m_lFullStates);
	SwapLEArray(m_lSmallStates);

	SwapLEArray(m_lFullAnimLayers);
	SwapLEArray(m_lSmallAnimLayers);
	SwapLEArray(m_lAnimLayersMap);

	SwapLEArray(m_lFullActions);
	SwapLEArray(m_lSmallActions);
	SwapLEArray(m_lActionsMap);

	SwapLEArray(m_lNodes);
	SwapLEArray(m_lEffectInstances);
	SwapLEArray(m_lCallbackParams);
	SwapLEArray(m_lStateConstants);

	SwapLEArray(m_lActionHashEntries);

#ifndef M_RTM
	SwapLEArray(m_lExportedActionNames);
	SwapLEArray(m_lExportedStateNames);
	SwapLEArray(m_lExportedPropertyNames);
	SwapLEArray(m_lExportedOperatorNames);
	SwapLEArray(m_lExportedEffectNames);
	SwapLEArray(m_lExportedStateConstantNames);
#endif
}
#endif
//--------------------------------------------------------------------------------

void CXRAG::Read(const char* _pFileName, bool bCreateLog)
{
	CStr FileName = _pFileName;

	M_TRY
	{
		CDataFile DFile;
		DFile.Open(FileName);
		Read(&DFile);
		DFile.Close();

		CheckConsistency(true);
	}
	M_CATCH(
	catch(...)
	{
		Clear();
		ConOutL(CStrF("Error while reading '%s'.", FileName));
	}
	)

/*	{
		CStr LogFileName = FileName.GetPath() + FileName.GetFilenameNoExt() + " (Read).log";
		LogDump(LogFileName, AG_DUMPFLAGS_ALL);
	}*/
}

//--------------------------------------------------------------------------------

void CXRAG::Write(const char* _pFileName, bool bCreateLog)
{
#ifndef	PLATFORM_CONSOLE
	CStr FileName = _pFileName;
	{
		CStr LogFileName = FileName.GetPath() + FileName.GetFilenameNoExt() + " (Write).log";
#ifndef	M_RTM
		LogDump(LogFileName, AG_DUMPFLAGS_ALL);
#endif
	}

	try
	{
		CDataFile DFile;
		DFile.Create(FileName,CDATAFILE_VERSION,NO_COMPRESSION,NORMAL_COMPRESSION,EDataFileFlag_NoHidePos);
		Write(&DFile);
		DFile.Close();
	}
	catch(...)
	{
		CStr AltFileName = FileName.GetPath() + FileName.GetFilenameNoExt() + " (Copy)." + FileName.GetFilenameExtenstion();
		ConOutL(CStrF("Error while writing '%s'. Trying alternate name '%s'.", FileName.Str(), AltFileName.Str()));
		try
		{
			CDataFile DFile;
			DFile.Create(AltFileName);
			Write(&DFile);
			DFile.Close();
		}
		catch(...)
		{
			ConOutL(CStrF("Error while writing '%s'.", AltFileName.Str()));
		}
	}
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
