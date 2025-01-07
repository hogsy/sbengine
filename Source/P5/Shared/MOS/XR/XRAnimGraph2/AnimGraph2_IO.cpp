//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "../../MSystem/MSystem.h"
#include "AnimGraph2.h"

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

#ifndef	PLATFORM_CONSOLE
static void WriteFStr(CCFile* _pFile, CFStr& _Str)
{
	uint32 Len = Min(200, _Str.Len());
	_pFile->WriteLE(Len);
	_pFile->WriteLE((uint8*)_Str.Str(), Len + 1);
}
#endif	// PLATFORM_CONSOLE

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

template<class T, int _WholeStruct>
void ReadArray2Imp(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer)
{
	CCFile* pFile = _pDFile->GetFile();
	
	uint32 ElementSize = 0;
	int Version = _pDFile->GetUserData2();
	if (Version >= XR_ANIMGRAPH2_VERSION)
		pFile->ReadLE(ElementSize);

	_lArray.SetLen(_pDFile->GetUserData());
	T* pArray = _lArray.GetBasePtr();
	if ((_lArray.ListSize() == (_pDFile->GetEntrySize()-4)) && 
		(_pDFile->GetUserData2() == _CurrentVer) && _WholeStruct)
	{
		M_ASSERT(ElementSize == sizeof(T), "Must be same");
		pFile->Read(pArray, _lArray.ListSize());
#ifndef CPU_LITTLEENDIAN
		SwapLEArray2(_lArray);
#endif
	}
	else
	{
		ReadArray2_PerElementFallback2(_lArray, _pDFile, ElementSize);
	}
}

template<class T>
void ReadArray2(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer)
{
	ReadArray2Imp<T, T::EIOWholeStruct>(_lArray, _pDFile, _CurrentVer);
}

template<class T>
void ReadArray2WholeStruct(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer)
{
	ReadArray2Imp<T, 1>(_lArray, _pDFile, _CurrentVer);
}

template<class T>
void ReadArray2FallBack(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer)
{
	ReadArray2Imp<T, 0>(_lArray, _pDFile, _CurrentVer);
}

//--------------------------------------------------------------------------------
//- class T ----------------------------------------------------------------------
//--------------------------------------------------------------------------------

template<class T>
void ReadArray2_PerElementFallback2(TThinArray<T>& _lArray, CDataFile* _pDFile, uint32 _ElementSize)
{
	int Version = _pDFile->GetUserData2();
	CCFile* pFile = _pDFile->GetFile();

	T* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
	{
		int Pos = pFile->Pos();
		pArray[i].Read(pFile, Version);
		if (Version >= XR_ANIMGRAPH2_VERSION && T::EIOWholeStruct)
		{
			int Pad = _ElementSize - (pFile->Pos() - Pos);
			uint8 Temp[32] = {0};
			pFile->Read(Temp, Pad);
		}
	}
}

//--------------------------------------------------------------------------------

template<class T>
void WriteArray2_PerElementFallback2(TThinArray<T>& _lArray, CDataFile* _pDFile)
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

template<class T>
void WriteArray2(TThinArray<T>& _lArray, CDataFile* _pDFile, int _CurrentVer, const char* _pEntryName)
{
	CCFile* pFile = _pDFile->GetFile();

	_pDFile->BeginEntry(_pEntryName);
	uint32 ElementSize = sizeof(T);
	pFile->WriteLE(ElementSize);
	WriteArray2_PerElementFallback2(_lArray, _pDFile);
	_pDFile->EndEntry(_lArray.Len(), _CurrentVer);
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
template<class T>
void SwapLEArray2(TThinArray<T>& _lArray)
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
void ReadArray2_PerElementFallback2(TThinArray<int16>& _lArray, CDataFile* _pDFile, uint32 _ElementSize)
{
	CCFile* pFile = _pDFile->GetFile();
	int16* pArray = _lArray.GetBasePtr();
	pFile->ReadLE(pArray, _lArray.Len());
}

//--------------------------------------------------------------------------------

template<>
void WriteArray2_PerElementFallback2(TThinArray<int16>& _lArray, CDataFile* _pDFile)
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
void SwapLEArray2(TThinArray<int16>& _lArray)
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
void ReadArray2_PerElementFallback2(TThinArray<fp32>& _lArray, CDataFile* _pDFile, uint32 _ElementSize)
{
	CCFile* pFile = _pDFile->GetFile();
	fp32* pArray = _lArray.GetBasePtr();
	pFile->ReadLE((uint32*)pArray, _lArray.Len());
}

//--------------------------------------------------------------------------------

template<>
void WriteArray2_PerElementFallback2(TThinArray<fp32>& _lArray, CDataFile* _pDFile)
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
void SwapLEArray2(TThinArray<fp32>& _lArray)
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
void ReadArray2_PerElementFallback2(TThinArray<CFStr>& _lArray, CDataFile* _pDFile, uint32 _ElementSize)
{
	CCFile* pFile = _pDFile->GetFile();
	CFStr* pArray = _lArray.GetBasePtr();
	for(int i = 0; i < _lArray.Len(); i++)
		ReadFStr(pFile, pArray[i]);
}

//--------------------------------------------------------------------------------

template<>
void WriteArray2_PerElementFallback2(TThinArray<CFStr>& _lArray, CDataFile* _pDFile)
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
void SwapLEArray2(TThinArray<CFStr>& _lArray)
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

void CXRAG2_EffectInstance::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_iParams);
			_pFile->ReadLE(m_ID);
			_pFile->ReadLE(m_nParams);
		}
		break;

		default:
			Error_static("CXRAG2_AnimInstance::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_EffectInstance::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_iParams);
	_pFile->WriteLE(m_ID);
	_pFile->WriteLE(m_nParams);
#endif	// PLATFORM_CONSOLE
}
#ifndef CPU_LITTLEENDIAN
void CXRAG2_EffectInstance::SwapLE()
{
	::SwapLE(m_iParams);
	::SwapLE(m_ID);
	::SwapLE(m_nParams);
}
#endif
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2_StateConstant::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_Value);
			_pFile->ReadLE(m_ID);
		}
		break;

		default:
			Error_static("CXRAG2_AnimInstance::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_StateConstant::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_Value);
	_pFile->WriteLE(m_ID);
#endif	// PLATFORM_CONSOLE
}

#ifndef CPU_LITTLEENDIAN
void CXRAG2_StateConstant::SwapLE()
{
	::SwapLE(m_Value);
	::SwapLE(m_ID);
}
#endif
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2_ConditionNode::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_Data.m_BitUnion);
			_pFile->ReadLE(m_Constant);
		}
		break;

		default:
			Error_static("CXRAG2_ConditionNode::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_ConditionNode::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_Data.m_BitUnion);
	_pFile->WriteLE(m_Constant);
#endif	// PLATFORM_CONSOLE
}
#ifndef CPU_LITTLEENDIAN
void CXRAG2_ConditionNode::SwapLE()
{
	m_Data.SwapLE();
	::SwapLE(m_Constant);
}
#endif

void CXRAG2_ConditionNodeV2::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_Data.m_BitUnion);
			_pFile->ReadLE(m_ConstantInt);
		}
		break;

	default:
		Error_static("CXRAG2_ConditionNodeV2::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_ConditionNodeV2::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_Data.m_BitUnion);
	_pFile->WriteLE(m_ConstantInt);
#endif	// PLATFORM_CONSOLE
}
#ifndef CPU_LITTLEENDIAN
void CXRAG2_ConditionNodeV2::SwapLE()
{
	m_Data.SwapLE();
	::SwapLE(m_ConstantInt);
}
#endif


//--------------------------------------------------------------------------------

void CXRAG2_ActionHashEntry::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_HashKey);
			_pFile->ReadLE(m_iAction);
		}
		break;

	default:
		Error_static("CXRAG2_Action::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_ActionHashEntry::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_HashKey);
	_pFile->WriteLE(m_iAction);
#endif	// PLATFORM_CONSOLE
}


#ifndef CPU_LITTLEENDIAN
void CXRAG2_ActionHashEntry::SwapLE()
{
	::SwapLE(m_HashKey);
	::SwapLE(m_iAction);
}
#endif
//--------------------------------------------------------------------------------

void CXRAG2_NameAndID::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION:
		{
			ReadFStr(_pFile, m_Name);
			_pFile->ReadLE(m_ID);
		}
		break;

	default:
		Error_static("CXRAG2_NameAndID::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_NameAndID::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	WriteFStr(_pFile, m_Name);
	_pFile->WriteLE(m_ID);
#endif	// PLATFORM_CONSOLE
}

#ifndef CPU_LITTLEENDIAN
void CXRAG2_NameAndID::SwapLE()
{
	::SwapLE(m_ID); // Redundant but consistent.
}
#endif

void CXRAG2_NameAndValue::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION:
		{
			ReadFStr(_pFile, m_Name);
			_pFile->ReadLE(m_Value);
		}
		break;

	default:
		Error_static("CXRAG2_NameAndValue::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_NameAndValue::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	WriteFStr(_pFile, m_Name);
	_pFile->WriteLE(m_Value);
#endif	// PLATFORM_CONSOLE
}

#ifndef CPU_LITTLEENDIAN
void CXRAG2_NameAndValue::SwapLE()
{
	::SwapLE(m_Value);
}
#endif
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2::Read(CDataFile* _pDFile)
{
	CCFile* pFile = _pDFile->GetFile();

	int32 AGVersion;
	if(!_pDFile->GetNext("NAME"))
		Error("Read", "No NAME entry found.");
	{
		char pName[1024];
		pFile->Read(pName, Min((int32)1023,(int32) (_pDFile->GetEntrySize() - sizeof(int32))));
		m_Name = pName;
		pFile->ReadLE(AGVersion);
	}

	if (AGVersion != XR_ANIMGRAPH2_VERSION)
		return;

	// States
	if(!_pDFile->GetNext("GRAPHBLOCKS"))
		Error("Read", "No GRAPHBLOCKS entry found.");
	ReadArray2(m_lGraphBlocks, _pDFile, XR_ANIMGRAPH2_VERSION);

	// States
	if(!_pDFile->GetNext("FULLSTATES"))
		Error("Read", "No FULLSTATES entry found.");
	ReadArray2(m_lFullStates, _pDFile, XR_ANIMGRAPH2_VERSION);

	// States
	if(!_pDFile->GetNext("SWSTATES"))
		Error("Read", "No SWSTATES entry found.");
	ReadArray2(m_lSwitchStates, _pDFile, XR_ANIMGRAPH2_VERSION);

	// AnimLayers
	if(!_pDFile->GetNext("FULLANIMLAYERS"))
		Error("Read", "No FULLANIMLAYERS entry found.");
	ReadArray2(m_lFullAnimLayers, _pDFile, XR_ANIMGRAPH2_VERSION);

	if(!_pDFile->GetNext("ANIMNAMES"))
		Error("Read", "No ANIMNAMES entry found.");
	ReadArray2(m_lAnimNames, _pDFile, XR_ANIMGRAPH2_VERSION);
	
	if(!_pDFile->GetNext("ANIMCONTAINERNAMES"))
	{
		Error("Read", "No ANIMCONTAINERNAMES entry found.");
	}
	else
	{
		CCFile* pFile = _pDFile->GetFile();
		m_AnimContainerNames.Read(pFile, XR_ANIMGRAPH2_VERSION);
	}

	// Actions
	if(!_pDFile->GetNext("FULLACTIONS"))
		Error("Read", "No FULLACTIONS entry found.");
	ReadArray2(m_lFullActions, _pDFile, XR_ANIMGRAPH2_VERSION);

	// Actions
	if(!_pDFile->GetNext("MOVETOKENS"))
		Error("Read", "No MOVETOKENS entry found.");
	ReadArray2(m_lMoveTokens, _pDFile, XR_ANIMGRAPH2_VERSION);

	// Actions
	if(!_pDFile->GetNext("MOVEANIMGRAPH2"))
		Error("Read", "No MOVEANIMGRAPH2 entry found.");
	ReadArray2(m_lMoveAnimGraphs, _pDFile, XR_ANIMGRAPH2_VERSION);

	// Actions
	if(!_pDFile->GetNext("FULLREACTIONS"))
		Error("Read", "No FULLREACTIONS entry found.");
	ReadArray2(m_lFullReactions, _pDFile, XR_ANIMGRAPH2_VERSION);

	// Actionvals
	if(!_pDFile->GetNext("SWACTIONVALS"))
		Error("Read", "No SWACTIONVALS entry found.");
	ReadArray2(m_lSwitchStateActionVals, _pDFile, XR_ANIMGRAPH2_VERSION);

	// Nodes
	if(!_pDFile->GetNext("NODESV2"))
		Error("Read", "No NODESV2 entry found.");
	ReadArray2(m_lNodesV2, _pDFile, XR_ANIMGRAPH2_VERSION);

	// Effects
	if(!_pDFile->GetNext("EFFECTS"))
		Error("Read", "No EFFECTS entry found.");
	ReadArray2(m_lEffectInstances, _pDFile, XR_ANIMGRAPH2_VERSION);

	// CallbackParams
	if(!_pDFile->GetNext("CALLBACKPARAMS"))
		Error("Read", "No CALLBACKPARAMS entry found.");
	ReadArray2WholeStruct(m_lCallbackParams, _pDFile, XR_ANIMGRAPH2_VERSION);

	// StateConstants
	if(!_pDFile->GetNext("STATECONSTANTS"))
		Error("Read", "No STATECONSTANTS entry found.");
	ReadArray2(m_lStateConstants, _pDFile, XR_ANIMGRAPH2_VERSION);

	// ActionHashEntries
	if(!_pDFile->GetNext("ACTIONHASHENTRIES"))
		Error("Read", "No ACTIONHASHENTRIES entry found.");
	ReadArray2(m_lActionHashEntries, _pDFile, XR_ANIMGRAPH2_VERSION);

#ifndef M_RTM
	if (!D_MXDFCREATE)
	{
		// ExportedNames (Optional)
		if(_pDFile->GetNext("EXPORTEDGRAPHBLOCKNAMES"))
			ReadArray2FallBack(m_lExportedGraphBlockNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDACTIONNAMES"))
			ReadArray2FallBack(m_lExportedActionNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDREACTIONNAMES"))
			ReadArray2FallBack(m_lExportedReactionNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDSTATENAMES"))
			ReadArray2FallBack(m_lExportedStateNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDSWSTNAMES"))
			ReadArray2FallBack(m_lExportedSwitchStateNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDAG2NAMES"))
			ReadArray2FallBack(m_lExportedAnimGraphNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDPROPERTYFNNAMES"))
			ReadArray2FallBack(m_lExportedPropertyFunctionNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDPROPERTYFNAMES"))
			ReadArray2(m_lExportedPropertyFloatNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDPROPERTYINAMES"))
			ReadArray2(m_lExportedPropertyIntNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDPROPERTYBNAMES"))
			ReadArray2(m_lExportedPropertyBoolNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDOPERATORNAMES"))
			ReadArray2(m_lExportedOperatorNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDEFFECTNAMES"))
			ReadArray2(m_lExportedEffectNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDSTATECONSTNAMES"))
			ReadArray2(m_lExportedStateConstantNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDIMPULSETNAMES"))
			ReadArray2(m_lExportedImpulseTypeNames, _pDFile, XR_ANIMGRAPH2_VERSION);

		if(_pDFile->GetNext("EXPORTEDIMPULSEVNAMES"))
			ReadArray2(m_lExportedImpulseValueNames, _pDFile, XR_ANIMGRAPH2_VERSION);
	}
#endif
}

//--------------------------------------------------------------------------------

void CXRAG2::Write(CDataFile* _pDFile)
{
#ifndef	PLATFORM_CONSOLE
	CCFile* pFile = _pDFile->GetFile();

	{
		_pDFile->BeginEntry("NAME");
		pFile->Write(m_Name.Str(), m_Name.Len() + 1);
		int32 AGVersion = XR_ANIMGRAPH2_VERSION;
		pFile->WriteLE(AGVersion);
		_pDFile->EndEntry(m_Name.Len(), XR_ANIMGRAPH2_VERSION);
	}

	WriteArray2(m_lGraphBlocks, _pDFile, XR_ANIMGRAPH2_VERSION, "GRAPHBLOCKS");

	WriteArray2(m_lFullStates, _pDFile, XR_ANIMGRAPH2_VERSION, "FULLSTATES");
	WriteArray2(m_lSwitchStates, _pDFile, XR_ANIMGRAPH2_VERSION, "SWSTATES");

	WriteArray2(m_lFullAnimLayers, _pDFile, XR_ANIMGRAPH2_VERSION, "FULLANIMLAYERS");
	WriteArray2(m_lAnimNames, _pDFile, XR_ANIMGRAPH2_VERSION, "ANIMNAMES");
	{
		CCFile* pFile = _pDFile->GetFile();
		_pDFile->BeginEntry("ANIMCONTAINERNAMES");
		m_AnimContainerNames.Write(pFile);
		_pDFile->EndEntry(1, XR_ANIMGRAPH2_VERSION);
	}

	WriteArray2(m_lFullActions, _pDFile, XR_ANIMGRAPH2_VERSION, "FULLACTIONS");
	WriteArray2(m_lMoveTokens, _pDFile, XR_ANIMGRAPH2_VERSION, "MOVETOKENS");
	WriteArray2(m_lMoveAnimGraphs, _pDFile, XR_ANIMGRAPH2_VERSION, "MOVEANIMGRAPH2");
	WriteArray2(m_lFullReactions, _pDFile, XR_ANIMGRAPH2_VERSION, "FULLREACTIONS");
	WriteArray2(m_lSwitchStateActionVals, _pDFile, XR_ANIMGRAPH2_VERSION, "SWACTIONVALS");

	WriteArray2(m_lNodesV2, _pDFile, XR_ANIMGRAPH2_VERSION, "NODESV2");
	WriteArray2(m_lEffectInstances, _pDFile, XR_ANIMGRAPH2_VERSION, "EFFECTS");
	WriteArray2(m_lCallbackParams, _pDFile, XR_ANIMGRAPH2_VERSION, "CALLBACKPARAMS");
	WriteArray2(m_lStateConstants, _pDFile, XR_ANIMGRAPH2_VERSION, "STATECONSTANTS");

	// FIXME: Sort by HashKey, to support binary search in the other end...
	WriteArray2(m_lActionHashEntries, _pDFile, XR_ANIMGRAPH2_VERSION, "ACTIONHASHENTRIES");

#ifndef	M_RTM
	WriteArray2(m_lExportedGraphBlockNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDGRAPHBLOCKNAMES");
	WriteArray2(m_lExportedActionNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDACTIONNAMES");
	WriteArray2(m_lExportedReactionNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDREACTIONNAMES");
	WriteArray2(m_lExportedStateNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDSTATENAMES");
	WriteArray2(m_lExportedSwitchStateNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDSWSTNAMES");
	WriteArray2(m_lExportedAnimGraphNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDAG2NAMES");
	WriteArray2(m_lExportedPropertyFunctionNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDPROPERTYFNNAMES");
	WriteArray2(m_lExportedPropertyFloatNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDPROPERTYFNAMES");
	WriteArray2(m_lExportedPropertyIntNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDPROPERTYINAMES");
	WriteArray2(m_lExportedPropertyBoolNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDPROPERTYBNAMES");
	WriteArray2(m_lExportedOperatorNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDOPERATORNAMES");
	WriteArray2(m_lExportedEffectNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDEFFECTNAMES");
	WriteArray2(m_lExportedStateConstantNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDSTATECONSTNAMES");
	WriteArray2(m_lExportedImpulseTypeNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDIMPULSETNAMES");
	WriteArray2(m_lExportedImpulseValueNames, _pDFile, XR_ANIMGRAPH2_VERSION, "EXPORTEDIMPULSEVNAMES");
	
#endif	// M_RTM
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
void CXRAG2::SwapLE()
{
	SwapLEArray2(m_lGraphBlocks);
	SwapLEArray2(m_lFullStates);
	SwapLEArray2(m_lSwitchStates);
	SwapLEArray2(m_lFullAnimLayers);
	SwapLEArray2(m_lAnimNames);
	SwapLEArray2(m_lFullActions);
	SwapLEArray2(m_lFullReactions);
	SwapLEArray2(m_lSwitchStateActionVals);
	SwapLEArray2(m_lMoveTokens);
	SwapLEArray2(m_lMoveAnimGraphs);
	SwapLEArray2(m_lNodesV2);
	SwapLEArray2(m_lEffectInstances);
	SwapLEArray2(m_lStateConstants);
	SwapLEArray2(m_lCallbackParams);
	SwapLEArray2(m_lActionHashEntries);

#ifndef M_RTM
	if (!D_MXDFCREATE)
	{
		SwapLEArray2(m_lExportedGraphBlockNames);
		SwapLEArray2(m_lExportedActionNames);
		SwapLEArray2(m_lExportedReactionNames);
		SwapLEArray2(m_lExportedStateNames);
		SwapLEArray2(m_lExportedPropertyFloatNames);
		SwapLEArray2(m_lExportedPropertyIntNames);
		SwapLEArray2(m_lExportedPropertyBoolNames);
		SwapLEArray2(m_lExportedOperatorNames);
		SwapLEArray2(m_lExportedEffectNames);
		SwapLEArray2(m_lExportedStateConstantNames);
		SwapLEArray2(m_lExportedImpulseTypeNames);
		SwapLEArray2(m_lExportedImpulseValueNames);
	}
#endif
}
#endif
//--------------------------------------------------------------------------------

void CXRAG2::Read(const char* _pFileName, bool bCreateLog)
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
		LogDump(LogFileName, AG2_DUMPFLAGS_ALL);
	}*/
}

//--------------------------------------------------------------------------------

void CXRAG2::Write(const char* _pFileName, bool bCreateLog)
{
#ifndef	PLATFORM_CONSOLE
	CStr FileName = _pFileName;
	/*{
		CStr LogFileName = FileName.GetPath() + FileName.GetFilenameNoExt() + " (Write).log";
#ifndef	M_RTM
		LogDump(LogFileName, AG2_DUMPFLAGS_ALL);
#endif
	}*/

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


void CXRAG2_AnimNames::Read(CCFile* _pFile, int _Ver)
{
	switch (_Ver)
	{
	case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_iAnimSeq);
			_pFile->ReadLE(m_iContainerName);
		}
		break;
	default:
		Error_static("CXRAG2_AnimNames::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_AnimNames::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_iAnimSeq);
	_pFile->WriteLE(m_iContainerName);
#endif
}

#ifndef CPU_LITTLEENDIAN
void CXRAG2_AnimNames::SwapLE()
{
	::SwapLE(m_iAnimContainerResource);
	::SwapLE(m_iAnimSeq);
	::SwapLE(m_iContainerName);
}
#endif



void CXRAG2_AnimContainerNames::Read(CCFile* _pFile, int _Ver)
{
	switch (_Ver)
	{
	case XR_ANIMGRAPH2_VERSION:
		{
			int16 Len;
			_pFile->ReadLE(Len);
			m_lNames.SetLen(Len);
			for (int32 i = 0; i < Len; i++)
				m_lNames[i] = _pFile->Readln();
		}
		break;
	default:
		Error_static("CXRAG2_AnimContainerNames::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_AnimContainerNames::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	int16 Len = m_lNames.Len();
	_pFile->WriteLE(Len);
	for (int32 i = 0; i < Len; i++)
		_pFile->Writeln(m_lNames[i]);
#endif
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
