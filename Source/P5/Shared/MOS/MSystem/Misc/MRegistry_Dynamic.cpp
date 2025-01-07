#include "PCH.h"
#include "MRegistry_Dynamic.h"

#include "MRegistry_Shared.h"


mint CRegistry_Dynamic::ms_lTypeSize[REGISTRY_TYPE_MAX] = {
	0, 
	sizeof(CStr::spCStrData),
	sizeof(TList_Vector<uint8>),
	sizeof(uint8), 
	sizeof(int16), 
	sizeof(int32), 
	sizeof(fp4), 
	sizeof(fp2), 
	sizeof(uint32), 
};

FRegistryTypeDestructor *CRegistry_Dynamic::ms_lTypeDestructor[REGISTRY_TYPE_MAX] = {
	NULL, 
	CRegistry_Shared_Helpers::RegistryTypeDestructor<CStr::spCStrData>, 
	CRegistry_Shared_Helpers::RegistryTypeDestructor<TList_Vector<uint8> >,
	NULL, 
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
	};

FRegistryTypeConstructor *CRegistry_Dynamic::ms_lTypeConstructor[REGISTRY_TYPE_MAX] =
{
	NULL, 
	CRegistry_Shared_Helpers::RegistryTypeConstructor<CStr::spCStrData>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructor<TList_Vector<uint8> >,
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<uint8>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<int16>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<int32>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<fp4>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<fp2>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<uint32>, 
};


FRegistryTypeDestructor *CRegistry_Dynamic::ms_lTypeDestructorPtr[REGISTRY_TYPE_MAX] = {
	NULL, 
	CRegistry_Shared_Helpers::RegistryTypeDestructorPtr<CStr::spCStrData>, 
	CRegistry_Shared_Helpers::RegistryTypeDestructorPtr<TList_Vector<uint8> >,
	CRegistry_Shared_Helpers::RegistryTypeDestructorPtr<uint8>,
	CRegistry_Shared_Helpers::RegistryTypeDestructorPtr<int16>,
	CRegistry_Shared_Helpers::RegistryTypeDestructorPtr<int32>,
	CRegistry_Shared_Helpers::RegistryTypeDestructorPtr<fp4>,
	CRegistry_Shared_Helpers::RegistryTypeDestructorPtr<fp2>,
	CRegistry_Shared_Helpers::RegistryTypeDestructorPtr<uint32>,
	};

FRegistryTypeConstructor *CRegistry_Dynamic::ms_lTypeConstructorPtr[REGISTRY_TYPE_MAX] =
{
	NULL, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorPtr<CStr::spCStrData>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorPtr<TList_Vector<uint8> >,
	CRegistry_Shared_Helpers::RegistryTypeConstructorPtrClear<uint8>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorPtrClear<int16>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorPtrClear<int32>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorPtrClear<fp4>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorPtrClear<fp2>,
	CRegistry_Shared_Helpers::RegistryTypeConstructorPtrClear<uint32>, 
};

FRegistryTypeDestructor *CRegistry_Dynamic::ms_lTypeDestructorAnim[REGISTRY_TYPE_MAX] = {
	NULL, 
	CRegistry_Shared_Helpers::RegistryTypeDestructor<CStr::spCStrData>, 
	CRegistry_Shared_Helpers::RegistryTypeDestructor<TList_Vector<uint8> >,
	NULL, 
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
	};

FRegistryTypeConstructor *CRegistry_Dynamic::ms_lTypeConstructorAnim[REGISTRY_TYPE_MAX] =
{
	NULL, 
	CRegistry_Shared_Helpers::RegistryTypeConstructor<CStr::spCStrData>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructor<TList_Vector<uint8> >,
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<uint8>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<int16>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<int32>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<fp4>, 
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<fp2>,
	CRegistry_Shared_Helpers::RegistryTypeConstructorClear<uint32>
};


FRegistryTypeConvert *CRegistry_Dynamic::ms_lTypeConvert[REGISTRY_TYPE_MAX][REGISTRY_TYPE_MAX] = 
{
	// Void
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertDefault<CStr::spCStrData>,
		CRegistry_Shared_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistry_Shared_Helpers::RegistryConvertClear<uint8>,
		CRegistry_Shared_Helpers::RegistryConvertClear<int16>,
		CRegistry_Shared_Helpers::RegistryConvertClear<int32>,
		CRegistry_Shared_Helpers::RegistryConvertClear<fp4>,
		CRegistry_Shared_Helpers::RegistryConvertClear<fp2>,
		CRegistry_Shared_Helpers::RegistryConvertClear<uint32>
	}
	,
	// Str
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertStrToStr,
		CRegistry_Shared_Helpers::RegistryConvertStrToData,
		CRegistry_Shared_Helpers::RegistryConvertStrToInt<uint8>,
		CRegistry_Shared_Helpers::RegistryConvertStrToInt<int16>,
		CRegistry_Shared_Helpers::RegistryConvertStrToInt<int32>,
		CRegistry_Shared_Helpers::RegistryConvertStrToFloat<fp4, fp4>,
		CRegistry_Shared_Helpers::RegistryConvertStrToFloat<fp2, fp4>,
		CRegistry_Shared_Helpers::RegistryConvertStrToInt<uint32>
	}
	,
	// Data
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertDataToStr,
		CRegistry_Shared_Helpers::RegistryConvertCopyListVector,
		CRegistry_Shared_Helpers::RegistryConvertClear<uint8>,
		CRegistry_Shared_Helpers::RegistryConvertClear<int16>,
		CRegistry_Shared_Helpers::RegistryConvertClear<int32>,
		CRegistry_Shared_Helpers::RegistryConvertClear<fp4>,
		CRegistry_Shared_Helpers::RegistryConvertClear<fp2>,
		CRegistry_Shared_Helpers::RegistryConvertClear<uint32>
	}
	,
	// uint8
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertIntToStr<uint8>,
		CRegistry_Shared_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistry_Shared_Helpers::RegistryConvertSame<uint8>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint8, int16>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint8, int32>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint8, fp4>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint8, fp2>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint8, uint32>
	}
	,
	// int16
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertIntToStr<int16>,
		CRegistry_Shared_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int16, uint8>,
		CRegistry_Shared_Helpers::RegistryConvertSame<int16>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int16, int32>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int16, fp4>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int16, fp2>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int16, uint32>
	}
	,
	// int32
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertIntToStr<int32>,
		CRegistry_Shared_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int32, uint8>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int32, int16>,
		CRegistry_Shared_Helpers::RegistryConvertSame<int32>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int32, fp4>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int32, fp2>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<int32, uint32>
	}
	,
	// fp4
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertFloatToStr<fp4>,
		CRegistry_Shared_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp4, uint8>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp4, int16>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp4, int32>,
		CRegistry_Shared_Helpers::RegistryConvertSame<fp4>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp4, fp2>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp4, uint32>,
	}
	,
	// fp2
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertFloatToStr<fp2>,
		CRegistry_Shared_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp2, uint8>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp2, int16>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp2, int32>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp2, fp4>,
		CRegistry_Shared_Helpers::RegistryConvertSame<fp2>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<fp2, int32>,
	}
	,
	// uint32
	{
		CRegistry_Shared_Helpers::RegistryConvertDummy,
		CRegistry_Shared_Helpers::RegistryConvertIntToStrHex<uint32>,
		CRegistry_Shared_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint32, uint8>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint32, int16>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint32, int32>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint32, fp4>,
		CRegistry_Shared_Helpers::RegistryConvertCompatible<uint32, fp2>,
		CRegistry_Shared_Helpers::RegistryConvertSame<uint32>,
	}
};

void CRegistry_Dynamic::Private_Anim_Destroy(CAnimationData &_Data)
{
	CAnimationData &AnimData = _Data;
	for (int i	= 0; i < AnimData.m_Sequences.Len(); ++i)
	{
		CAnimationSequence *pSeq = AnimData.m_Sequences[i];
		if (m_Type != REGISTRY_TYPE_VOID)
		{
			if ((AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed) && (pSeq->m_pData))
				Private_Anim_DestroyDataTimed(pSeq->m_pData, 0, pSeq->m_nKeyFrames, m_Dimensions+1);
			else if (pSeq->m_pData)
				Private_Anim_DestroyData(pSeq->m_pData, 0, pSeq->m_nKeyFrames, m_Dimensions+1);
		}
		delete pSeq;
	}
	AnimData.~CAnimationData();
}

void CRegistry_Dynamic::Type_Clear()
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
	{
		CAnimationData &AnimData = Type_GetAnimationData();
		Private_Anim_Destroy(AnimData);

		m_Type = REGISTRY_TYPE_VOID;
		m_Dimensions = 0;

		m_InternalFlags &= ~ERegistryDynamicFlags_Animated;
	}
	else
	{
		if (m_Type != REGISTRY_TYPE_VOID)
		{
			Type_CallDestructor(m_Type, m_Value, m_Dimensions+1);
			m_Type = REGISTRY_TYPE_VOID;
			m_Dimensions = 0;
		}
	}
}

void CRegistry_Dynamic::Type_SetNew(int _Type, int _nDim)
{
	Private_CheckDim(_nDim);
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
	{
		M_ASSERT(0, "Should not be used in this mode");
	}
	else
	{
		Type_Clear();
		m_Type = _Type;
		m_Dimensions = _nDim - 1;
		if (m_Type != REGISTRY_TYPE_VOID)
		{
			Type_CallConstructor(m_Type, m_Value, m_Dimensions+1);
		}
	}
}

void CRegistry_Dynamic::Type_Set(int _Type, int _nDim)
{
	Private_CheckDim(_nDim);
	if (m_Type != _Type || _nDim != _nDim)
	{
		if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		{			
			int OldType = m_Type;
			m_Type = _Type;
			CAnimationData &AnimData = Type_GetAnimationData();
			for (int i = 0; i < AnimData.m_Sequences.Len(); ++i)
			{
				CAnimationSequence *pSeq = AnimData.m_Sequences[i];
				int nNew = pSeq->m_nKeyFrames;
				if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
				{
					mint NewItemSize = ms_lTypeSize[_Type] * _nDim + sizeof(fp4);
					mint NewSize = nNew * NewItemSize;
					
					uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
					uint8 *pOldData = (uint8 *)pSeq->m_pData;
					
//					fp4 LastTime = 0;

					Private_Anim_CopyDataTimedTyped(pNewData, pOldData, 0, 0, _nDim, m_Dimensions+1, nNew, true, OldType);

					MRTC_GetMemoryManager()->Free(pOldData);
					pSeq->m_pData = pNewData;
				}
				else
				{
					mint NewItemSize = ms_lTypeSize[_Type] * _nDim;
					mint NewSize = nNew * NewItemSize;
					
					uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
					uint8 *pOldData = (uint8 *)pSeq->m_pData;
					
//					fp4 LastTime = 0;

					Private_Anim_CopyDataTyped(pNewData, pOldData, 0, 0, _nDim, m_Dimensions+1, nNew, true, OldType);

					MRTC_GetMemoryManager()->Free(pOldData);
					pSeq->m_pData = pNewData;
				}
			}
			m_Dimensions = _nDim-1;
		}
		else
		{
			if (m_Type == REGISTRY_TYPE_VOID || _Type == REGISTRY_TYPE_VOID)
			{
				Type_SetNew(_Type, _nDim);
			}
			else
			{
				uint8 Temp[EDataStorage];
				// Copy to temp

				Type_CallConstructor(_Type, Temp, _nDim);
				ms_lTypeConvert[m_Type][_Type](Type_GetValue(), m_Dimensions+1, Type_GetValue(Temp, _Type, _nDim), _nDim);
				Type_CallDestructor(m_Type, m_Value, m_Dimensions+1);

				// Copy temp to value
				Type_CallConstructor(_Type, m_Value, _nDim);
				m_Dimensions = _nDim-1;

				ms_lTypeConvert[_Type][_Type](Type_GetValue(Temp, _Type, _nDim), _nDim, Type_GetValue(), _nDim);
				Type_CallDestructor(_Type, Temp, _nDim);

				m_Type = _Type;
			}
		}
	}
}

void CRegistry_Dynamic::ConvertToType(int _Type, int _nDim)
{
	if (m_Type == REGISTRY_TYPE_STR && _nDim == -2 && m_Dimensions == 0)
	{
		// Try to determine dimensions
		CStr Data;
		GetThisValuea(1, &Data);

		if (Data.IsAnsi())
		{
			const char *pParse = Data.Str();
			_nDim = 1;
			while (*pParse)
			{
				if (*pParse == ',')
					++_nDim;
				++pParse;
			}
		}
		else
		{
			const wchar *pParse = Data.StrW();
			_nDim = 1;
			while (*pParse)
			{
				if (*pParse == ',')
					++_nDim;
				++pParse;
			}
		}

	}
	if (_nDim < 0)
		Type_Set(_Type, m_Dimensions+1);
	else
		Type_Set(_Type, _nDim);
}

int CRegistry_Dynamic::MRTC_AddRef()
{
	if (m_RefCount == (DBit(11) - 1))
	{
		Error_static("CRegistry_Dynamic::MRTC_AddRef", "Registry dynamic only supports 2047 references");
	}
	return (++m_RefCount);
}

int CRegistry_Dynamic::MRTC_DelRef()
{
	return (--m_RefCount);
}

int CRegistry_Dynamic::MRTC_ReferenceCount() const
{
	return m_RefCount;
}

MRTC_IMPLEMENT( CRegistry_Dynamic, CRegistry );


void CRegistry_Dynamic::Private_CopyValueFromRegistryDynamic(const CRegistry_Dynamic& _Reg)
{
	MAUTOSTRIP(CRegistry_Dynamic_operator_assign, MAUTOSTRIP_VOID);

	Type_Clear();

	if (_Reg.m_InternalFlags & ERegistryDynamicFlags_Animated)
	{
		Private_Anim_Promote();
		m_Type = _Reg.m_Type;
		m_Dimensions = _Reg.m_Dimensions;
		CAnimationData &AnimData = Type_GetAnimationData();
		const CAnimationData &AnimDataSrc = _Reg.Type_GetAnimationData();
		AnimData.m_AnimFlags = AnimDataSrc.m_AnimFlags;
		AnimData.m_AnimIPType = AnimDataSrc.m_AnimIPType;
		AnimData.m_InternalAnimFlags = AnimDataSrc.m_InternalAnimFlags;
		AnimData.m_Sequences.SetLen(AnimDataSrc.m_Sequences.Len());
		for (int i = 0; i < AnimDataSrc.m_Sequences.Len(); ++i)
		{
			CAnimationSequence *pSeq;
			const CAnimationSequence *pSeqSrc = AnimDataSrc.m_Sequences[i];
			if (i == 0)
				pSeq = AnimData.m_Sequences[i];
			else
				pSeq = AnimData.m_Sequences[i] = DNew(CAnimationSequence) CAnimationSequence;
			pSeq->m_nKeyFrames = pSeqSrc->m_nKeyFrames;
			pSeq->m_LoopStart = pSeqSrc->m_LoopStart;
			pSeq->m_LoopEnd = pSeqSrc->m_LoopEnd;
			mint Size;
			if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
				Size = ms_lTypeSize[m_Type] * (m_Dimensions+1) + sizeof(fp4);
			else
				Size = ms_lTypeSize[m_Type] * (m_Dimensions+1);
			pSeq->m_pData = M_ALLOC(Size * pSeqSrc->m_nKeyFrames);
			
			if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
				Private_Anim_CopyDataTimed(pSeq->m_pData, pSeqSrc->m_pData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), pSeqSrc->m_nKeyFrames, false);
			else
				Private_Anim_CopyData(pSeq->m_pData, pSeqSrc->m_pData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), pSeqSrc->m_nKeyFrames, false);
		}
	}
	else
	{
		Type_SetNew(_Reg.m_Type, _Reg.m_Dimensions+1);

		if (_Reg.m_Type != REGISTRY_TYPE_VOID)
		{
			ms_lTypeConvert[_Reg.m_Type][_Reg.m_Type](_Reg.Type_GetValue(), (m_Dimensions+1), Type_GetValue(), (m_Dimensions+1));
		}
	}

	m_InternalFlags = _Reg.m_InternalFlags;
	m_UserFlags = _Reg.m_UserFlags;
	m_spName = _Reg.m_spName;
	Private_Hash_Invalidate();

	CRegistry *pParent = GetParent();
	if (pParent)
		pParent->Hash_Invalidate();

}

void CRegistry_Dynamic::operator= (const CRegistry_Dynamic& _Reg)
{
	MAUTOSTRIP(CRegistry_Dynamic_operator_assign, MAUTOSTRIP_VOID);

	Private_CopyValueFromRegistryDynamic(_Reg);

	if (_Reg.GetNumChildren())
	{
		int nChildren = _Reg.GetNumChildren();
		m_lChildren.SetLen(nChildren);
		for(int i = 0; i < nChildren; i++)
		{
			m_lChildren[i].m_spReg = _Reg.GetChild(i)->Duplicate();
			m_lChildren[i].m_spReg->SetParent(this);
		}
	}
	else
		m_lChildren.Clear();
}


void CRegistry_Dynamic::CopyValueFromRegistry(const CRegistry& _Reg)
{
	Type_Clear();

	if (_Reg.Anim_ThisGetAnimated())
	{
		Private_Anim_Promote();
		m_Type = _Reg.GetType();
		m_Dimensions = _Reg.GetDimensions()-1;
		CAnimationData &AnimData = Type_GetAnimationData();
		AnimData.m_AnimFlags = _Reg.Anim_ThisGetFlags();
		int nData = 0;
		AnimData.m_AnimIPType = _Reg.Anim_ThisGetInterpolate(NULL, nData);
		AnimData.m_InterpolateData.SetLen(nData);
		_Reg.Anim_ThisGetInterpolate(AnimData.m_InterpolateData.GetBasePtr(), nData);
		AnimData.m_InternalAnimFlags = _Reg.Anim_ThisGetEnableTimed() ? ERegistryDynamicAnimFlag_Timed : 0;
		AnimData.m_Sequences.SetLen(_Reg.Anim_ThisGetNumSeq());
		for (int i = 0; i < AnimData.m_Sequences.Len(); ++i)
		{
			CAnimationSequence *pSeq;
			if (i == 0)
				pSeq = AnimData.m_Sequences[i];
			else
				pSeq = AnimData.m_Sequences[i] = DNew(CAnimationSequence) CAnimationSequence;
			pSeq->m_nKeyFrames = _Reg.Anim_ThisGetNumKF(i);
			pSeq->m_LoopStart = _Reg.Anim_ThisGetSeqLoopStart(i);
			pSeq->m_LoopEnd = _Reg.Anim_ThisGetSeqLoopEnd(i);
			mint Size;
			if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
				Size = ms_lTypeSize[m_Type] * (m_Dimensions+1) + sizeof(fp4);
			else
				Size = ms_lTypeSize[m_Type] * (m_Dimensions+1);
			pSeq->m_pData = M_ALLOC(Size * pSeq->m_nKeyFrames);

			if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
				Private_Anim_ConstructDataTimed(pSeq->m_pData, 0, pSeq->m_nKeyFrames, (m_Dimensions+1));
			else
				Private_Anim_ConstructData(pSeq->m_pData, 0, pSeq->m_nKeyFrames, (m_Dimensions+1));
			
			switch (m_Type)
			{
			case REGISTRY_TYPE_VOID:
				break;
			case REGISTRY_TYPE_STR:
				{
					CStr Data[EMaxDimensions];
					CStr::spCStrData spData[EMaxDimensions];
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						_Reg.Anim_ThisGetKFValuea(i, j, (m_Dimensions+1), Data);
						{
							for (int i = 0; i < (m_Dimensions+1); ++i)
								spData[i] = Data[i].GetStrData();
						}

						Private_Anim_SetItem(pSeq, j, spData, REGISTRY_TYPE_STR, (m_Dimensions+1));
						if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
							Private_Anim_SetKFTime(pSeq->m_pData, j, _Reg.Anim_ThisGetKFTime(i, j));
					}
				}
				break;
			case REGISTRY_TYPE_DATA:
				{
					TArray<uint8> Data[EMaxDimensions];
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						_Reg.Anim_ThisGetKFValuead(i, j, (m_Dimensions+1), Data);
						Private_Anim_SetItem(pSeq, j, Data, REGISTRY_TYPE_DATA, (m_Dimensions+1));
						if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
							Private_Anim_SetKFTime(pSeq->m_pData, j, _Reg.Anim_ThisGetKFTime(i, j));
					}
				}
				break;
			case REGISTRY_TYPE_UINT8:
			case REGISTRY_TYPE_INT16:
			case REGISTRY_TYPE_INT32:
			case REGISTRY_TYPE_UINT32:
				{
					int32 Data[EMaxDimensions];
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						_Reg.Anim_ThisGetKFValueai(i, j, (m_Dimensions+1), Data);
						Private_Anim_SetItem(pSeq, j, Data, REGISTRY_TYPE_INT32, (m_Dimensions+1));
						if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
							Private_Anim_SetKFTime(pSeq->m_pData, j, _Reg.Anim_ThisGetKFTime(i, j));
					}
				}
				break;
			case REGISTRY_TYPE_FP4:
			case REGISTRY_TYPE_FP2:
				{
					fp4 Data[EMaxDimensions];
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						_Reg.Anim_ThisGetKFValueaf(i, j, (m_Dimensions+1), Data);
						Private_Anim_SetItem(pSeq, j, Data, REGISTRY_TYPE_FP4, (m_Dimensions+1));
						if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
							Private_Anim_SetKFTime(pSeq->m_pData, j, _Reg.Anim_ThisGetKFTime(i, j));
					}
				}
				break;
			}				
		}
	}
	else
	{
		int nDim = _Reg.GetDimensions();
		Type_SetNew(_Reg.GetType(), nDim);

		switch (m_Type)
		{
		case REGISTRY_TYPE_VOID:
			break;
		case REGISTRY_TYPE_STR:
			{
				CStr Data[EMaxDimensions];
				CStr::spCStrData spData[EMaxDimensions];
				_Reg.GetThisValuea(nDim, Data);
				for (int i = 0; i < nDim; ++i)
					spData[i] = Data[i].GetStrData();

				ms_lTypeConvert[REGISTRY_TYPE_STR][REGISTRY_TYPE_STR](spData, nDim, Type_GetValue(), nDim);
			}
			break;
		case REGISTRY_TYPE_DATA:
			{
				TArray<uint8> Array[EMaxDimensions];
				_Reg.GetThisValuead(nDim, Array);
				ms_lTypeConvert[REGISTRY_TYPE_DATA][REGISTRY_TYPE_DATA](Array, nDim, Type_GetValue(), nDim);
			}
			break;
		case REGISTRY_TYPE_UINT8:
		case REGISTRY_TYPE_INT16:
		case REGISTRY_TYPE_INT32:
		case REGISTRY_TYPE_UINT32:
			{
				int32 Data[EMaxDimensions];
				_Reg.GetThisValueai(nDim, Data);
				ms_lTypeConvert[REGISTRY_TYPE_INT32][m_Type](Data, nDim, Type_GetValue(), nDim);
			}
			break;
		case REGISTRY_TYPE_FP4:
		case REGISTRY_TYPE_FP2:
			{
				fp4 Data[EMaxDimensions];
				_Reg.GetThisValueaf(nDim, Data);
				ms_lTypeConvert[REGISTRY_TYPE_FP4][m_Type](Data, nDim, Type_GetValue(), nDim);
			}
		}
	}



}

void CRegistry_Dynamic::operator= (const CRegistry& _Reg)
{
	MAUTOSTRIP(CRegistry_Dynamic_operator_assign_2, MAUTOSTRIP_VOID);

	m_InternalFlags = _Reg.Anim_ThisGetDisableAutoDemote() ? ERegistryDynamicFlags_DisableAnimAutoDemote : 0;
	m_UserFlags = _Reg.GetThisUserFlags();

	CopyValueFromRegistry(_Reg);

	CRegistry *pParent = GetParent();
	if (pParent)
		pParent->Hash_Invalidate();

	m_spName = _Reg.GetThisName().GetStrData();


	if (_Reg.GetNumChildren())
	{
		int nChildren = _Reg.GetNumChildren();
		m_lChildren.SetLen(nChildren);
		for(int i = 0; i < nChildren; i++)
		{
			m_lChildren[i].m_spReg = _Reg.GetChild(i)->Duplicate();
			m_lChildren[i].m_spReg->SetParent(this);
		}
	}
	else
		m_lChildren.Clear();

	Private_Hash_Invalidate();
}

void CRegistry_Dynamic::SetChild(int _iChild, CRegistry*_pReg)
{
	m_lChildren[_iChild].m_spReg = _pReg;
	Private_Hash_Invalidate();
}

spCRegistry CRegistry_Dynamic::Duplicate() const
{
	MAUTOSTRIP(CRegistry_Dynamic_Duplicate, NULL);
	spCRegistry_Dynamic spR = DNew(CRegistry_Dynamic) CRegistry_Dynamic;
	if (!spR) MemError_static("Duplicate");
	*spR = *this;
	return (CRegistry*)spR;
}

CRegistry_Dynamic::CRegistry_Dynamic()
{
	MAUTOSTRIP(CRegistry_Dynamic_ctor, MAUTOSTRIP_VOID);

	m_Dimensions = 0;
	m_InternalFlags = ERegistryDynamicFlags_HashDirty;		// 4
	m_RefCount = 0;
	m_UserFlags = 0;
	m_pParent = NULL;
	m_Type = REGISTRY_TYPE_VOID;
}

CRegistry_Dynamic::~CRegistry_Dynamic()
{
	MAUTOSTRIP(CRegistry_Dynamic_dtor, MAUTOSTRIP_VOID);
	m_pParent = NULL;
	Clear();
}

void CRegistry_Dynamic::Clear()
{
	MAUTOSTRIP(CRegistry_Dynamic_Clear, MAUTOSTRIP_VOID);
	Type_Clear();
	m_InternalFlags = ERegistryDynamicFlags_HashDirty;		// 4
	m_spName = NULL;

	CRegistry *pParent = GetParent();
	if (pParent)
		pParent->Hash_Invalidate();

	int nChildren = m_lChildren.Len();
	for (int i = 0; i < nChildren; ++i)
	{
		if (m_lChildren[i].m_spReg->GetParent() == this)
			m_lChildren[i].m_spReg->SetParent(NULL);
	}
	m_lChildren.Clear();
}

bint CRegistry_Dynamic::ValidChild(int _iChild) const
{
	return m_lChildren.ValidPos(_iChild);
}

int CRegistry_Dynamic::GetNumChildren() const
{
	return m_lChildren.Len();
}

void CRegistry_Dynamic::SetNumChildren(int _nChildren)
{
	MAUTOSTRIP(CRegistry_Dynamic_SetNumChildren, MAUTOSTRIP_VOID);

	if (!_nChildren)
	{
		m_lChildren.Clear();
	}
	else
	{
		int nCh = m_lChildren.Len();
		m_lChildren.SetLen(_nChildren);
		for(int i = nCh; i < _nChildren; i++)
		{
			if (m_lChildren[i].m_spReg == NULL)
			{
				spCRegistry spReg = REGISTRY_CREATE;
				spReg->SetParent(this);

				if (!spReg) 
					MemError_static("SetNumChildren");

				m_lChildren[i].m_spReg = spReg;
			}
			else
				m_lChildren[i].m_spReg->Clear();
		}
	}
	Private_Hash_Invalidate();
}

CRegistry* CRegistry_Dynamic::GetChild(int _iChild)
{
	if (!m_lChildren.ValidPos(_iChild)) 
		Error_static("CRegistry_Dynamic::GetChild", CStrF("Index out of range. (%d/%d)", _iChild, m_lChildren.Len()));
	return m_lChildren[_iChild].m_spReg;
}

const CRegistry* CRegistry_Dynamic::GetChild(int _iChild) const
{
	if (!m_lChildren.ValidPos(_iChild)) 
		Error_static("CRegistry_Dynamic::GetChild", CStrF("Index out of range. (%d/%d)", _iChild, m_lChildren.Len()));
	return m_lChildren[_iChild].m_spReg;
}

const CRegistry* CRegistry_Dynamic::GetChildUnsafe(int _iChild) const
{
	MAUTOSTRIP(CRegistry_Dynamic_GetChild2, NULL);
	M_ASSERT( m_lChildren.ValidPos(_iChild), "Index out of range!! This method should only be used when iterating children..");
	return m_lChildren[_iChild].m_spReg;
}

int CRegistry_Dynamic::GetType() const
{
	return m_Type;
}

int CRegistry_Dynamic::GetDimensions() const
{
	return (m_Dimensions+1);
}

// Sort register
void CRegistry_Dynamic::Sort(bint _bReversedOrder, bint _bSortByName)
{
#ifdef REGISTRY_COMPILEDORDERSIMULATE
	if (m_InternalFlags & ERegistryDynamicFlags_SimulateRegistryCompiled)
		Error_static(M_FUNCTION, "Not possible");
#endif
	MAUTOSTRIP(CRegistry_Dynamic_Sort, MAUTOSTRIP_VOID);
	// NOTE: _bReverseOrder and _bSortByName are ignored

	qsort(m_lChildren.GetBasePtr(), GetNumChildren(), sizeof(CPtrKeyHash), CRegistry_Shared_Helpers::RegistryCompare);
	for(int i = 0; i < GetNumChildren(); i++)
		GetChild(i)->Sort(_bReversedOrder, _bSortByName);
	Private_Hash_Invalidate();
}


void CRegistry_Dynamic::Hash_Init()
{
	if (m_InternalFlags & ERegistryDynamicFlags_HashDirty)
	{
		int nChildren = m_lChildren.Len();
#ifdef _DEBUG
		m_nChildrenLastHashCreate = nChildren;
#endif
		for (int i = 0; i < nChildren; ++i)
		{
#ifdef _DEBUG
			CRegistry *pParent = m_lChildren[i].m_spReg->GetParent();
			if (pParent != this && pParent)
			{
				CRegistry *pReg = m_lChildren[i].m_spReg;
				M_TRACEALWAYS("No parent for: %s = %s\n", pReg->GetThisName().Str(), pReg->GetThisValue().Str());
				M_ASSERT(0, "Must be parent to this registry");
			}
#endif

			m_lChildren[i].m_Hash = m_lChildren[i].m_spReg->GetThisNameHash();
			m_lChildren[i].m_iHashKey = i;
		}

		CRegistry_Shared_Helpers::QSortHash_r(this, 0, nChildren-1);
		m_InternalFlags &= ~ERegistryDynamicFlags_HashDirty;
	}
}

int CRegistry_Dynamic::FindIndex(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue) const
{
	int iIndex = FindIndexInternal(_pKey);
	if (iIndex < 0)
		return -1;

	CFStr Upper;
	Upper.Capture(_pKey);
	Upper = Upper.UpperCase();
	
	int nChildren = m_lChildren.Len();

	const CRegistry *pReg = m_lChildren[m_lChildren[iIndex].m_iHashKey].m_spReg;
	while (1)
	{
		if (_bCaseSensitiveValue)
		{ 
			if (pReg->GetThisValue() == _pValue) 
				return m_lChildren[iIndex].m_iHashKey;
		}
		else
		{ 
			if (pReg->GetThisValue().CompareNoCase(_pValue) == 0) 
				return m_lChildren[iIndex].m_iHashKey;
		}
		++iIndex;

		if (iIndex >= nChildren)
			return -1;

		pReg = m_lChildren[m_lChildren[iIndex].m_iHashKey].m_spReg;

		if (pReg->GetThisName().CompareNoCase(Upper.Str()) != 0)
			return -1;
	}

}

int CRegistry_Dynamic::FindIndexInternal(const char* _pKey) const
{
	MAUTOSTRIP(CRegistry_Dynamic_FindIndex, 0);
	const_cast<CRegistry_Dynamic*>(this)->Hash_Init();

	// Do a binary search for the hash value
	CFStr Upper;
	Upper.Capture(_pKey);
	Upper = Upper.UpperCase();
	uint32 Hash = StringToHash(_pKey);
	int nChildren = m_lChildren.Len();
	int iIndexSearch = 0;

	int Low = 0;
	int High = nChildren;
	while(Low < High)
	{
		iIndexSearch = (Low + High) >> 1;

		uint32 CurrHash = m_lChildren[m_lChildren[iIndexSearch].m_iHashKey].m_Hash;

		if (CurrHash < Hash)
			Low = iIndexSearch + 1;
		else if (CurrHash > Hash)
			High = iIndexSearch;
		else
			break;
	}

	if (iIndexSearch < nChildren)
	{
		if (m_lChildren[m_lChildren[iIndexSearch].m_iHashKey].m_Hash == Hash)
		{
			while (iIndexSearch >= 0 && m_lChildren[m_lChildren[iIndexSearch].m_iHashKey].m_Hash == Hash)
				--iIndexSearch;
			++iIndexSearch;
			while (iIndexSearch < nChildren && m_lChildren[m_lChildren[iIndexSearch].m_iHashKey].m_Hash == Hash)
			{
				if (m_lChildren[m_lChildren[iIndexSearch].m_iHashKey].m_spReg->GetThisName().CompareNoCase(Upper.Str()) == 0)
				{
					return iIndexSearch;
				}
				++iIndexSearch;
			}
			return -1;
		}
		else
			return -1;
	}
	return -1;
}

// Search children
int CRegistry_Dynamic::FindIndex(const char* _pKey) const
{
	int iIndex = FindIndexInternal(_pKey);
	if (iIndex < 0)
		return -1;
	return m_lChildren[iIndex].m_iHashKey;
}


#ifdef REGISTRY_COMPILEDORDERSIMULATE
void CRegistry_Dynamic::SimulateRegistryCompiled(bint _Enable)
{
	if (_Enable)
		m_InternalFlags |= ERegistryDynamicFlags_SimulateRegistryCompiled;
	else
		m_InternalFlags &= ~ERegistryDynamicFlags_SimulateRegistryCompiled;

	for (int i = 0; i < m_lChildren.Len(); ++i)
	{
		m_lChildren[i].m_spReg->SimulateRegistryCompiled(_Enable);
	}
	Private_UpdateCompiledOrder();
}

void CRegistry_Dynamic::Private_UpdateCompiledOrder()
{
	CRegistry *pReg = this;
	while (pReg)
	{
		if (pReg->SimulateRegistryCompiled())
			break;

		pReg = pReg->GetParent();
	}
	if (!pReg)
		return;

	for (int i = 0; i < m_lChildren.Len(); ++i)
	{
		m_lChildren[i].m_iHashKey = i;
		m_lChildren[i].m_Hash = StringToHash(m_lChildren[i].m_spReg->GetThisName().UpperCase()) & 0xffff;;
	}

	m_InternalFlags |= ERegistryDynamicFlags_HashDirty;

	qsort(m_lChildren.GetBasePtr(), GetNumChildren(), sizeof(CPtrKeyHash), CRegistry_Shared_Helpers::RegistryCompiledSimulate_Compare);
}
#endif

CRegistry* CRegistry_Dynamic::CreateDir(const char* _pPath)
{
	CFStr Dir;
	Dir.Capture(_pPath);
	CRegistry* pReg = this;
	while(Dir != "")
	{
		CFStr Name = Dir.GetStrSep("\\");
		CRegistry* pChild = pReg->Find(Name);
		if (!pChild)
		{
			spCRegistry_Dynamic spReg = REGISTRY_CREATE;
			if (!spReg) 
				MemError_static("CreateDir");
			spReg->SetThisName(Name);
			pReg->AddReg((CRegistry *)spReg);
			pChild = spReg;
		}
		pReg = pChild;
	}
	return pReg;
}

bint CRegistry_Dynamic::DeleteDir(const char* _pPath)
{
	CFStr Dir = _pPath;
	CRegistry* pReg = this;
	while(Dir != "")
	{
		CFStr Name = Dir.GetStrSep("\\");
		int iChild = pReg->FindIndex(Name);
		if (iChild < 0) return false;
		CRegistry* pChild = pReg->GetChild(iChild);
		if (Dir == "")
		{
			pReg->DeleteKey(iChild);
			return true;
		}

		pReg = pChild;
	}

	// Can't delete this
	return false;
}

void CRegistry_Dynamic::CopyValue(const CRegistry* _pReg)
{
	CopyValueFromRegistry(*_pReg);
}

void CRegistry_Dynamic::CopyDir(const CRegistry* _pReg, bint _bRecursive)
{
	if (_pReg->GetThisValue() != "")
		CopyValueFromRegistry(*_pReg);

	m_spName = _pReg->GetThisName().GetStrData();

	CRegistry *pParent = GetParent();
	if (pParent)
		pParent->Hash_Invalidate();

	int nCh = _pReg->GetNumChildren();

	for(int i = 0; i < nCh; i++)
	{
		const CRegistry* pChild = _pReg->GetChild(i);
		int iKey = FindIndex(pChild->GetThisName());
		if (iKey >= 0)
		{
			if (_bRecursive)
				GetChild(iKey)->CopyDir(pChild, _bRecursive);
			else
			{
				GetChild(iKey)->SetThisName(pChild->GetThisName());
				GetChild(iKey)->CopyValue(pChild);
			}
		}
		else
			AddKey(pChild);
	}

}

void CRegistry_Dynamic::AddReg(spCRegistry _spReg)
{
	_spReg->SetParent(this);
	m_lChildren.SetGrow(GetGEPow2(m_lChildren.Len()) >> 0);
	m_lChildren.Add(CPtrKeyHash(_spReg));	
	Private_Hash_Invalidate();
}

CRegistry *CRegistry_Dynamic::CreateNewChild()
{
	spCRegistry_Dynamic spReg = REGISTRY_CREATE;
	if (!spReg) 
		MemError_static("CreateDir");
	AddReg((CRegistry *)spReg);
	return (CRegistry *)spReg;
}

void CRegistry_Dynamic::DeleteKey(int _iKey)
{
	MAUTOSTRIP(CRegistry_Dynamic_DeleteKey_2, MAUTOSTRIP_VOID);
	if (!m_lChildren.ValidPos(_iKey)) 
		Error_static("CRegistry_Dynamic::DeleteKey", "Index out of range.");

	m_lChildren.Del(_iKey);
	Private_Hash_Invalidate();
}

// Setting
void CRegistry_Dynamic::SetThisName(const char* _pName)
{
	MAUTOSTRIP(CRegistry_Dynamic_RenameThisKey, MAUTOSTRIP_VOID);
	CStr Name;
	Name.Capture(_pName);
	m_spName = Name.GetStrData();

	CRegistry *pParent = GetParent();
	if (pParent)
		pParent->Hash_Invalidate();
}

void CRegistry_Dynamic::SetThisValueConvert(CStr _Value, int _nDim, int _Type)
{
	Type_SetNew(_Type, _nDim);

	CStr::spCStrData spSource = _Value.GetStrData();

	if (_nDim == 1 && _Type == REGISTRY_TYPE_STR && spSource)
	{
		if (spSource->IsAnsi())
		{
			const ch8 *pParse = spSource->Str();
			CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse));
			spSource = Dst.GetStrData();
		}
		else
		{
			const wchar *pParse = spSource->StrW();
			CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse));
			spSource = Dst.GetStrData();
		}
	}

	ms_lTypeConvert[REGISTRY_TYPE_STR][_Type](&spSource, 1, Type_GetValue(), (m_Dimensions+1));
}

void CRegistry_Dynamic::SetThisValue(const char* _pValue)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(REGISTRY_TYPE_STR, 1);
	CStr Value;
	Value.Capture(_pValue);
	Type_Get_Str() = Value.GetStrData();
}

void CRegistry_Dynamic::SetThisValue(const wchar* _pValue)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(REGISTRY_TYPE_STR, 1);
	CStr Value;
	Value.Capture(_pValue);
	Type_Get_Str() = Value.GetStrData();
}

void CRegistry_Dynamic::SetThisValue(CStr _Value)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(REGISTRY_TYPE_STR, 1);
	Type_Get_Str() = _Value.GetStrData();
}

void CRegistry_Dynamic::SetThisValuei(int32 _Value, int _StoreType)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(_StoreType, 1);
	switch (_StoreType)
	{
	case REGISTRY_TYPE_INT32:
		Type_Get_int32() = _Value;
		break;
	case REGISTRY_TYPE_UINT32:
		Type_Get_uint32() = _Value;
		break;
	case REGISTRY_TYPE_INT16:
		Type_Get_int16() = _Value;
		break;
	case REGISTRY_TYPE_UINT8:
		Type_Get_uint8() = _Value;
		break;
	}
}

void CRegistry_Dynamic::SetThisValuef(fp4 _Value, int _StoreType)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(_StoreType, 1);
	switch (_StoreType)
	{
	case REGISTRY_TYPE_FP4:
		Type_Get_fp4() = _Value;
		break;
	}
}

void CRegistry_Dynamic::SetThisValued(const uint8* _pValue, int _Size, bint _bQuick)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(REGISTRY_TYPE_DATA, 1);
	if (_bQuick)
		Type_Get_Data().SetLen(_Size);
	else
		Type_Get_Data().QuickSetLen(_Size);

	memcpy(Type_Get_Data().GetBasePtr(), _pValue, _Size);
}

void CRegistry_Dynamic::SetThisValued(TArray<uint8> _lValue, bint _bReference)
{
	if (_bReference)
	{
		Private_Anim_CheckAnimDisabled();
		Type_SetNew(REGISTRY_TYPE_DATA, 1);
		Type_Get_Data() = _lValue;		
	}
	else
		SetThisValued(_lValue.GetBasePtr(), _lValue.Len(), false);
}

void CRegistry_Dynamic::SetThisValuea(int _nDim, const CStr *_Value)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(REGISTRY_TYPE_STR, _nDim);

	CStr::spCStrData Data[EMaxDimensions];
	for (int i = 0; i < _nDim; ++i)
		Data[i] = _Value[i].GetStrData();

	ms_lTypeConvert[REGISTRY_TYPE_STR][REGISTRY_TYPE_STR](Data, _nDim, Type_GetValue(), (m_Dimensions+1));
}

void CRegistry_Dynamic::SetThisValuead(int _nDim, const TArray<uint8> *_lValue, bint _bReference)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(REGISTRY_TYPE_DATA, _nDim);

	TArray<uint8> Data[EMaxDimensions];
	if (_bReference)
	{
		for (int i = 0; i < _nDim; ++i)
			Data[i] = _lValue[i];
	}
	else
	{
		for (int i = 0; i < _nDim; ++i)
		{
			mint Len = _lValue[i].Len();
			Data[i].SetLen(Len );
			memcpy(Data[i].GetBasePtr(), _lValue[i].GetBasePtr(), Len);
		}
	}

	ms_lTypeConvert[REGISTRY_TYPE_DATA][REGISTRY_TYPE_DATA](Data, _nDim, Type_GetValue(), (m_Dimensions+1));
}

void CRegistry_Dynamic::SetThisValueai(int _nDim, const int32 *_Value, int _StoreType)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(_StoreType, _nDim);

	ms_lTypeConvert[REGISTRY_TYPE_INT32][_StoreType](_Value, _nDim, Type_GetValue(), (m_Dimensions+1));
}

void CRegistry_Dynamic::SetThisValueaf(int _nDim, const fp4 *_Value, int _StoreType)
{
	Private_Anim_CheckAnimDisabled();
	Type_SetNew(_StoreType, _nDim);

	ms_lTypeConvert[REGISTRY_TYPE_FP4][_StoreType](_Value, _nDim, Type_GetValue(), (m_Dimensions+1));
}



// Getting value from this

// Get/Set user flags, user flags are 8-bit
void CRegistry_Dynamic::SetThisUserFlags(uint32 _Value)
{
	m_UserFlags = _Value;
}

uint32 CRegistry_Dynamic::GetThisUserFlags() const
{
	return m_UserFlags;
}

CStr CRegistry_Dynamic::GetThisName() const
{
	return CStr(m_spName.p);	
}

uint32 CRegistry_Dynamic::GetThisNameHash() const
{
	return StringToHash(m_spName->Str());
}

const char* CRegistry_Dynamic::GetThisNameUnsafe() const
{
	if (m_spName)
		return m_spName->Str();
	else
		return "";
}

CStr CRegistry_Dynamic::GetThisValue() const
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValue(0, 0);

	if (m_Type == REGISTRY_TYPE_STR)
		return CStr(Type_Get_Str().p);
	
	CStr::spCStrData Ret;
	ms_lTypeConvert[m_Type][REGISTRY_TYPE_STR](Type_GetValue(), (m_Dimensions+1), &Ret, 1);
	return CStr(Ret.p);
}

int32 CRegistry_Dynamic::GetThisValuei() const
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValuei(0, 0);

	if (m_Type == REGISTRY_TYPE_INT32)
		return Type_Get_int32();
	int32 Ret;
	ms_lTypeConvert[m_Type][REGISTRY_TYPE_INT32](Type_GetValue(), (m_Dimensions+1), &Ret, 1);
	return Ret;
}

fp4 CRegistry_Dynamic::GetThisValuef() const
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValuef(0, 0);

	if (m_Type == REGISTRY_TYPE_FP4)
		return Type_Get_fp4();
	fp4 Ret;
	ms_lTypeConvert[m_Type][REGISTRY_TYPE_FP4](Type_GetValue(), (m_Dimensions+1), &Ret, 1);
	return Ret;
}

const TArray<uint8> CRegistry_Dynamic::GetThisValued() const
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValued(0, 0);

	if (m_Type == REGISTRY_TYPE_DATA)
		return Type_Get_Data();
	TArray<uint8> Ret;
	ms_lTypeConvert[m_Type][REGISTRY_TYPE_DATA](Type_GetValue(), (m_Dimensions+1), &Ret, 1);
	return Ret;
}

TArray<uint8> CRegistry_Dynamic::GetThisValued()
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValued(0, 0);

	if (m_Type == REGISTRY_TYPE_DATA)
		return Type_Get_Data();
	TArray<uint8> Ret;
	ms_lTypeConvert[m_Type][REGISTRY_TYPE_DATA](Type_GetValue(), (m_Dimensions+1), &Ret, 1);
	return Ret;
}

void CRegistry_Dynamic::GetThisValuea(int _nDim, CStr *_pDest) const
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValuea(0, 0, _nDim, _pDest);
	Private_CheckDim(_nDim);

	CStr::spCStrData Data[EMaxDimensions];
	ms_lTypeConvert[m_Type][REGISTRY_TYPE_STR](Type_GetValue(), (m_Dimensions+1), Data, _nDim);
	for (int i = 0; i < _nDim; ++i)
		_pDest[i] = CStr(Data[i].p);
}

void CRegistry_Dynamic::GetThisValueai(int _nDim, int32 *_pDest) const
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValueai(0, 0, _nDim, _pDest);
	Private_CheckDim(_nDim);

	ms_lTypeConvert[m_Type][REGISTRY_TYPE_INT32](Type_GetValue(), (m_Dimensions+1), _pDest, _nDim);
}

void CRegistry_Dynamic::GetThisValueaf(int _nDim, fp4 *_pDest) const
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValueaf(0, 0, _nDim, _pDest);
	Private_CheckDim(_nDim);

	ms_lTypeConvert[m_Type][REGISTRY_TYPE_FP4](Type_GetValue(), (m_Dimensions+1), _pDest, _nDim);
}

void CRegistry_Dynamic::GetThisValuead(int _nDim, TArray<uint8> *_pDest) const
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		return Anim_ThisGetKFValuead(0, 0, _nDim, _pDest);
	Private_CheckDim(_nDim);

	ms_lTypeConvert[m_Type][REGISTRY_TYPE_DATA](Type_GetValue(), (m_Dimensions+1), _pDest, _nDim);
}

////////////////// IO


template <typename t_CChar>
void ParseToComma(const t_CChar *&_pStr)
{
	while (*_pStr && *_pStr != ',')
		++_pStr;
}	

template <typename t_CChar>
const t_CChar *GetStrPtr(CStr &_String)
{
	if (sizeof(t_CChar) == 1)
		return (const t_CChar *)_String.Str();
	else
		return (const t_CChar *)_String.StrW();
}	

extern bint g_RegistryTypesSave[];
extern const char *g_RegistryTypes[];
extern const char *g_RegistryAnimIP[];

class CRegistry_DynamicParseKeyOptions
{
public:
	CRegistry_DynamicParseKeyOptions()
	{
		Reset();
	}

	int m_Dimensions;
	int m_Type;
	uint32 m_UserFlags;
	int m_AnimIP;
	fp4 m_AnimIPData[16];
	TThinArray<fp4> m_lAnimIPData;
	int m_nAnimIPData;
	uint32 m_AnimFlags;
	bint m_bValueEscaped;

	void Reset()
	{
		m_Dimensions = 1;
		m_Type = REGISTRY_TYPE_STR;
		m_UserFlags = 0;
		m_AnimIP = REGISTRY_ANIMIP_NONE;
		m_lAnimIPData.Clear();
		m_nAnimIPData = 0;
		m_AnimFlags = 0;
		m_bValueEscaped = false;
	}

	template <typename t_CChar>
	const char * Parse(const t_CChar *_pParse)
	{
		const t_CChar *pParse = _pParse;
		while (*pParse)
		{
			NScript::ParseWhiteSpace(pParse);

			const t_CChar *pIdent = pParse;
			NScript::ParseIdentifier(pParse);
			CStr Ident;
			Ident.Capture(pIdent, pParse - pIdent);
			Ident.MakeLowerCase();
			NScript::ParseWhiteSpace(pParse);
			if (*pParse != '=')
				return "Type options identifier should be followed by a '='";
			++pParse;
			NScript::ParseWhiteSpace(pParse);
			const t_CChar *pData = pParse;
			ParseToComma(pParse);
			CStr Data;
			Data.Capture(pData, pParse - pData);
			if (*pParse == ',')
				++pParse;

			if (Ident == "t")
			{
				// Type
				const t_CChar *pParse2 = GetStrPtr<t_CChar>(Data);
				const t_CChar *pType = pParse2;
				NScript::ParseAlphaNumeric(pParse2);
				CStr Type;
				Type.Capture(pType, pParse2 - pType);
				Type.MakeLowerCase();

				for (int i = 0; i < REGISTRY_TYPE_MAX; ++i)
				{
					if (Type == g_RegistryTypes[i])
					{
						m_Type = i;
						break;
					}
				}
				NScript::ParseWhiteSpace(pParse2);
                if (*pParse2 == '[')
				{
					++pParse2;
					m_Dimensions = NStr::StrToInt(pParse2, 0, ";");
					if (m_Dimensions < 1 || m_Dimensions > REGISTRY_MAX_DIMENSIONS)
						return "Invalid number of dimensions";
				}
			}
			else if (Ident == "uf")
			{
				m_UserFlags = NStr::StrToInt(GetStrPtr<t_CChar>(Data), 0);
			}
			else if (Ident == "ip")
			{
				Data.RTrim();
				for (int i = 0; i < REGISTRY_ANIMIP_MAX; ++i)
				{
					if (Data == g_RegistryAnimIP[i])
					{
						m_AnimIP= i;
						break;
					}
				}
			}
			else if (Ident == "id")
			{
				const t_CChar *pParse2 = GetStrPtr<t_CChar>(Data);
				int nFloat = 0;
				while (*pParse2)
				{
					fp4 Float = CMClosestAssign::Assign((fp4)0, NStr::StrToFloatParse(pParse2, -465465.0, " ;"));
					if (Float != -465465.0f)
						++nFloat;

					if (*pParse2)
						++pParse2;
				}

				pParse2 = GetStrPtr<t_CChar>(Data);

				int iFloat = 0;
				if (nFloat > 16)
					m_lAnimIPData.SetLen(nFloat);

				while (*pParse2)
				{
					fp4 Float = CMClosestAssign::Assign((fp4)0, NStr::StrToFloatParse(pParse2, -465465.0, " ;"));
					if (Float != -465465.0f)
					{
						if (nFloat >= 16)
							m_lAnimIPData[iFloat] = Float;
						else
							m_AnimIPData[iFloat] = Float;
						++iFloat;
					}

					if (*pParse2)
						++pParse2;
				}
				m_nAnimIPData = nFloat;
			}
			else if (Ident == "af")
			{
				const t_CChar *pParse2 = GetStrPtr<t_CChar>(Data);
				while (*pParse2)
				{
					NScript::ParseWhiteSpace(pParse2);
					switch (*pParse2)
					{
					case 'L':
					case 'l':
						m_AnimFlags |= REGISTRY_ANIMFLAGS_LOOP;
						break;
					case 'P':
					case 'p':
						m_AnimFlags |= REGISTRY_ANIMFLAGS_LOOP_PINGPONG;
						break;
					case 'C':
					case 'c':
						m_AnimFlags |= REGISTRY_ANIMFLAGS_CONTIGUOUS;
						break;
					}

					if (*pParse2)
						++pParse2;
				}
			}
			else
			{
				// Unrecognized Ident
				return "Unrecognized key options identifier";
			}
		}
		return NULL;
	}
};


template<typename t_CChar>
void ParsePastEscSeq(const t_CChar* &_pParse, bint _bUseSlash = true)
{
	const t_CChar *&pParse = _pParse;
	while(*pParse)
	{
		if (_bUseSlash && *pParse == '\\')
		{
			++pParse;

			if (!(*pParse)) 
				Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (1).");

			switch(*pParse)
			{
			case '\\' :
			case '"' :
			case 'n' :
			case 'r' :
				break;

			default :
				Error_static("::ParseEscSeq", CStr("Invalid escape-code."));
			}
			++pParse;
		}
		else if (*pParse == '"')
		{
			++pParse;
			return;
		}
		else 
			++pParse;
	}

	Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (2).");
}

template<typename t_CChar>
int ParseNumSeq(const t_CChar* _pParse)
{
	int nSeq = 1;
	const t_CChar *pParse = _pParse;
	NScript::ParseWhiteSpace(pParse);
	int Mode = 0;
	if (*pParse == '<')
		Mode = 1;

	while(*pParse)
	{
		if (*pParse == '"')
		{
			++pParse;
			ParsePastEscSeq(pParse);
		}
		else if (*pParse == '<')
		{
			if (Mode == 0)
			{
				++nSeq;
				Mode = 1;
			}
			++pParse;
		}
		else if (*pParse == '>')
		{
			if (Mode == 1)
			{
				Mode = 0;
			}
			++pParse;
		}
		else
			++pParse;
	}

	return nSeq;
}

template<typename t_CChar>
int ParseNumKF(const t_CChar* _pParse)
{
	int nKF = 1;
	const t_CChar *pParse = _pParse;
	NScript::ParseWhiteSpace(pParse);
	if (*pParse == '<')
	{
		while (*pParse && *pParse != '>')
			++pParse;
		if (*pParse != '>')
			++pParse;
	}

	while(*pParse)
	{
		if (*pParse == '"')
		{
			++pParse;
			ParsePastEscSeq(pParse);
		}
		else if (*pParse == '<')
		{
			return nKF;
			++pParse;
		}
		else if (*pParse == ';')
		{
			++nKF;
			++pParse;
		}
		else
			++pParse;
	}

	return nKF;
}

template<typename t_CChar>
int ParseIsTimed(const t_CChar* _pParse)
{
	const t_CChar *pParse = _pParse;
	while(*pParse)
	{
		if (*pParse == '"')
		{
			++pParse;
			ParsePastEscSeq(pParse);
		}
		else if (*pParse == '@')
		{
			return true;
		}
		else
			++pParse;
	}

	return false;
}
spCRegistry CRegistry_Dynamic::XRG_AddParseKey(CStr _Key, CStr _Value, CRegistry_ParseContext& _ParseContext, CRegistry_DynamicParseKeyOptions &_KeyOptions)
{
	MAUTOSTRIP(CRegistry_Dynamic_XRG_AddParseKey, NULL);
	spCRegistry_Dynamic spReg = REGISTRY_CREATE;
	if (!spReg) MemError_static("XRG_AddParseKey");

	// Auto convert to ANSI if unicode is not needed.
	if (_Value.IsPureAnsi())
		_Value = _Value.Ansi();

	spReg->m_spName = _Key.Ansi().GetStrData();	// <--- NOTE that key name is truncated to ANSI.

	int nSeq;
	if (_Value.IsAnsi())
		nSeq = ParseNumSeq(_Value.Str());
	else
		nSeq = ParseNumSeq(_Value.StrW());

	spReg->SetThisUserFlags(_KeyOptions.m_UserFlags);

	int Type = _KeyOptions.m_Type;
	if (!_KeyOptions.m_bValueEscaped && Type == REGISTRY_TYPE_STR && nSeq == 1)
	{
		// Auto detect type
		if (_Value.IsAnsi())
		{
			const ch8 *pStr = _Value.Str();
			NScript::ParseWhiteSpace(pStr);
			if (*pStr == 0)
				Type = 0;
			else
			{
				int32 Int = NStr::StrToIntParse(pStr, (int32)-654654," ");
				
				if (Int != -654654 && *pStr == 0)
				{
					pStr = _Value.Str();
					NScript::ParseWhiteSpace(pStr);
					if (pStr[0] == '0' && pStr[1] == 'x')
						Type = REGISTRY_TYPE_UINT32;
					else
						Type = REGISTRY_TYPE_INT32;
				}
				else
				{
					pStr = _Value.Str();
					fp4 Float = NStr::StrToFloatParse(pStr, -654654.0," ");
					if (Float != -654654.0f && *pStr == 0)
						Type = REGISTRY_TYPE_FP4;
				}
			}
		}
		else
		{
			const wchar *pStr = _Value.StrW();
			NScript::ParseWhiteSpace(pStr);
			if (*pStr == 0)
				Type = 0;
			else
			{
				int32 Int = NStr::StrToIntParse(pStr, (int32)-654654," ");
				
				if (Int != -654654 && *pStr == 0)
				{
					pStr = _Value.StrW();
					NScript::ParseWhiteSpace(pStr);
					if (pStr[0] == '0' && pStr[1] == 'x')
						Type = REGISTRY_TYPE_UINT32;
					else
						Type = REGISTRY_TYPE_INT32;
				}
				else
				{
					pStr = _Value.StrW();
					fp4 Float = NStr::StrToFloatParse(pStr, -654654.0," ");
					if (Float != -654654.0f && *pStr == 0)
						Type = REGISTRY_TYPE_FP4;
				}
			}
		}
	}

	int nKF;
	if (_Value.IsAnsi())
		nKF = ParseNumKF(_Value.Str());
	else
		nKF = ParseNumKF(_Value.StrW());

	if (nSeq > 1 || nKF > 1)
	{
		bint bIsTimed;
		if (_Value.IsAnsi())
			bIsTimed = ParseIsTimed(_Value.Str());
		else
			bIsTimed = ParseIsTimed(_Value.StrW());

		spReg->Anim_ThisSetAnimated(true);
		spReg->Anim_ThisSetEnableTimed(bIsTimed);
		if (_KeyOptions.m_nAnimIPData > 16)
			spReg->Anim_ThisSetInterpolate(_KeyOptions.m_AnimIP, _KeyOptions.m_lAnimIPData.GetBasePtr(), _KeyOptions.m_nAnimIPData);
		else
			spReg->Anim_ThisSetInterpolate(_KeyOptions.m_AnimIP, _KeyOptions.m_AnimIPData, _KeyOptions.m_nAnimIPData);
		spReg->Anim_ThisSetFlags(_KeyOptions.m_AnimFlags);

		spReg->Anim_ThisSetNumSeq(nSeq);

		CStr Ansi = _Value.Ansi();
		const ch8 *pParse = Ansi.Str();
		const ch8 *pParseStart = pParse;
		const wchar *pParseW = NULL;
		const wchar *pParseWStart = NULL;
		int StringLen = Ansi.Len();
		if (!_Value.IsAnsi())
			pParseWStart = pParseW = _Value.StrW();

        for (int s = 0; s < nSeq; ++s)
		{
			NScript::ParseWhiteSpace(pParse);

			fp4 SeqStart = -1;
			fp4 SeqEnd = -1;
			if (*pParse == '<')
			{
				++pParse;
				while (*pParse && *pParse != '>')
				{
					NScript::ParseWhiteSpace(pParse);

					const ch8 *pIdent = pParse;
					NScript::ParseIdentifier(pParse);
					CFStr Ident;
					Ident.Capture(pIdent, pParse - pIdent);
					Ident.MakeLowerCase();
					NScript::ParseWhiteSpace(pParse);
					if (*pParse != '=')
					{
						// Just parse till end
						while (*pParse && *pParse != '>')
							++pParse;
						break;
					}
					++pParse;

					if (Ident == "ls")
					{
						SeqStart = CMClosestAssign::Assign((fp4)0, NStr::StrToFloatParse(pParse, 0.0, ",>"));
					}
					else if (Ident == "le")
					{
						SeqEnd = CMClosestAssign::Assign((fp4)0, NStr::StrToFloatParse(pParse, 0.0, ",>"));
					}
					if (*pParse == ',')
						++pParse;
				}
	
				if (*pParse == '>')
					++pParse;
				NScript::ParseWhiteSpace(pParse);
			}

			spReg->Anim_ThisSetSeqLoopStart(s, SeqStart);
			spReg->Anim_ThisSetSeqLoopEnd(s, SeqEnd);

			NScript::ParseWhiteSpace(pParse);

			if (*pParse && *pParse != '<')
			{

				nKF = ParseNumKF(pParse);
				spReg->Anim_ThisSetNumKF(s, nKF);

				fp4 Time = 0;
				for (int i = 0; i < nKF; ++i)
				{
					CStr Value;
					const ch8 *pParseS = pParse;

					if (pParseW)
					{
						pParseW = pParseWStart + (pParse - pParseStart);
						Value = ParseEsqSeqIntactCompatible(pParseW, StringLen - (pParse - pParseStart), true, ";<@");
						pParse = pParseStart + (pParseW - pParseWStart);
					}
					else
					{
						Value = ParseEsqSeqIntactCompatible(pParse, StringLen - (pParse - pParseStart), true, ";<@");
					}
					if (pParse != pParseS)
					{
						if (*(pParse-1) == '<')
							--pParse;
						NScript::ParseWhiteSpace(pParse);
						if (*(pParse-1) == '@')
						{
							NScript::ParseWhiteSpace(pParse);
							// Parse time
							fp4 NewTime = CMClosestAssign::Assign((fp4)0, NStr::StrToFloatParse(pParse, 0.0, ";<"));

							if (NewTime < Time)
								Time += 1.0f;
							else
								Time = NewTime;
							NScript::ParseWhiteSpace(pParse);
						}
						else
							Time += 1.0f;

					}
					if (i == 0)
						Time = 0;

					if (*pParse == ';')
						++pParse;

					spReg->Anim_ThisSetKFValueConvert(s, i, Value, _KeyOptions.m_Dimensions, Type, Time);
				}
			}
		}
	}
	else
	{
		// No auto detection
		spReg->SetThisValueConvert(_Value, _KeyOptions.m_Dimensions, Type);
	}

	// Check for include
	if (spReg->GetThisName() == "INCLUDE" && _ParseContext.m_ThisFileName != "")
	{
		CStr FileName = _ParseContext.m_ThisFileName.GetPath() + spReg->GetThisValue();
		M_TRY
		{
			XRG_Read(FileName, _ParseContext.m_lDefines);
		}
		M_CATCH(
		catch(CCException)
		{
			Error_static("CRegistry_Dynamic::XRG_AddParseKey", CStrF("Error including file %s from file %s", FileName.Str(), _ParseContext.m_ThisFileName.Str()));
		}
		)
	}
	else
	{
		AddReg((CRegistry *)spReg);
	}

	return (CRegistry *)spReg;
}

template<class T>
static void Parse_Opt(const T* p, int& _Pos, int _Len)
{
	int End = FindChar(p + _Pos, _Len - _Pos, '>');	// EOL can be 13, 10 or just 10.
	if (End < 0)
		_Pos = _Len;
	else
		_Pos += End;
}

template <typename t_CChar>
int CRegistry_Dynamic::XRG_ParseImp(t_CChar* _pOrgData, t_CChar* _pStr, int _Size, CRegistry_ParseContext& _ParseContext)
{
	MAUTOSTRIP(CRegistry_Dynamic_XRG_Parse, 0);
	MSCOPESHORT(CRegistry_Dynamic::XRG_Parse); //AR-SCOPE

//	bint _bSlashIsEscape = true;

	CStr Q[2];
	int nQ = 0;
	int _Len = _Size;

	int Pos = 0;

	static t_CChar Text0_0[] = {'%', 's', '%', 's'};
	static t_CChar Text0_1[] = {'%', 'l', 's', '%', 'l', 's'};
	static t_CChar Text1_0[] = {'%', 's', ' ', '%', 's'};
	static t_CChar Text1_1[] = {'%', 'l', 's', ' ', '%', 'l', 's'};

	t_CChar *Text0;
	t_CChar *Text1;
	if (sizeof(t_CChar) == 1)
	{
		Text0 = Text0_0;
		Text1 = Text1_0;
	}
	else
	{
		Text0 = Text0_1;
		Text1 = Text1_1;
	}
	
	M_TRY
	{
		CRegistry_DynamicParseKeyOptions KeyOpt;
		KeyOpt.Reset();
		while(Pos < _Len)
		{
			Parse_WhiteSpace(_pStr, Pos, _Len);
			if (Pos >= _Len) break;

			// Comment
			else if (_pStr[Pos] == '/' && _pStr[Pos+1] == '/')
				ParseComment_SlashSlash(_pStr, Pos, _Len);

			// Comment
			else if (_pStr[Pos] == '/' && _pStr[Pos+1] == '*')
				ParseComment_SlashAstrix(_pStr, Pos, _Len, (char *)NULL);

			// Syntax-check
			else if (
				_pStr[Pos] == '/'/* ||
				_pStr[Pos] == '\\'*/)
			{
#if M_EXCEPTIONS
				int iLine = GetLineNumber(_pOrgData, &_pStr[Pos], _Size + (_pStr - _pOrgData)) + 1;
				Error_static("CRegistry_Dynamic::XRG_Parse", CStrF("Syntax error at line %d. Illegal slash outside string. (Last key parsed was %s=%s)", iLine, Q[0].Str(), Q[1].Str()));
#endif
			}

			// Begin scope
			else if (_pStr[Pos] == '{')
			{
				Pos++;

				spCRegistry spNew;

				if (nQ == 2)
					spNew = XRG_AddParseKey(Q[0], Q[1], _ParseContext, KeyOpt);
				else if (nQ == 1)
					spNew = XRG_AddParseKey(Q[0], "", _ParseContext, KeyOpt);
				else
				{
#if M_EXCEPTIONS
					int iLine = GetLineNumber(_pOrgData, &_pStr[Pos], _Size + (_pStr - _pOrgData)) + 1;
					Error_static("CRegistry_Dynamic::XRG_Parse", CStrF("Syntax error at line %d. Cannot begin scope without key (Last key parsed was %s=%s)", iLine, Q[0].Str(), Q[1].Str()));
#endif
				}
				nQ = -1;

				Pos += spNew->XRG_Parse(_pOrgData, &_pStr[Pos], _Size - Pos, _ParseContext /*, _bSlashIsEscape*/);
			}

			// End scope
			else if (_pStr[Pos] == '}')
			{
				Pos++;
				break;
			}

			// Begin key / key-separator
			else if (_pStr[Pos] == '*')
			{
				Pos++;
				if (nQ == 2)
					XRG_AddParseKey(Q[0], Q[1], _ParseContext, KeyOpt);
				else if (nQ == 1)
					XRG_AddParseKey(Q[0], "", _ParseContext, KeyOpt);

				KeyOpt.Reset();
				Parse_WhiteSpace(_pStr, Pos, _Len);
				if (_pStr[Pos] == '<')
				{
					++Pos;
					int StartOpt = Pos;
					Parse_Opt(_pStr, Pos, _Len);
					if (Pos == _Len)
					{
#if M_EXCEPTIONS
						int iLine = GetLineNumber(_pOrgData, &_pStr[Pos], _Size + (_pStr - _pOrgData)) + 1;
						Error_static("CRegistry_Dynamic::XRG_Parse", CStrF("Syntax error at line %d. Unmatched key option (mising '>')", iLine));
#endif
					}

					_pStr[Pos] = 0;
					const ch8 *pError = KeyOpt.Parse(_pStr + StartOpt);
					if (pError)
					{
#if M_EXCEPTIONS
						int iLine = GetLineNumber(_pOrgData, &_pStr[Pos], _Size + (_pStr - _pOrgData)) + 1;
						Error_static("CRegistry_Dynamic::XRG_Parse", CStrF("Syntax error parsing key options at line %d. %s.", iLine, pError));
#endif
					}
					_pStr[Pos] = '>';
					++Pos;
				}

				nQ = 0;
			}

			// Escape-sequence word
			else if (_pStr[Pos] == '"')			//"
			{
				// Parse word expressed with escape-sequence
				if (nQ == -1)
				{
#if M_EXCEPTIONS
					int iLine = GetLineNumber(_pOrgData, &_pStr[Pos], _Size + (_pStr - _pOrgData)) + 1;
					Error_static("CRegistry_Dynamic::XRG_Parse", CStrF("Syntax error at line %d. String after {} scope (Last key parsed was %s=%s)", iLine, Q[0].Str(), Q[1].Str()));
#endif
				}

				KeyOpt.m_bValueEscaped = true;
				Pos++;

				if (nQ == 0)
				{
					CStr Word = ParseEscSeq(_pStr, Pos, _Len);
					Word.MakeUpperCase();
					Q[0] = Word;
					Q[1].Clear();
					nQ++;
				}
				else if (nQ >= 1)
				{
					CStr Word = ParseEscSeqIntact(_pStr, Pos, _Len);
					if (Word != "")
					{
						if (Q[1] != "" && Word != "")
							Q[1] = Q[1] + Word;
						else
							Q[1] = Word;
						if (nQ == 1) nQ++;
					}
				}
			}
			else
			{
				// Regular syntax-killer word
				if (nQ == -1) 
				{
#if M_EXCEPTIONS
					int iLine = GetLineNumber(_pOrgData, &_pStr[Pos], _Size + (_pStr - _pOrgData)) + 1;
					Error_static("CRegistry_Dynamic::XRG_Parse", CStrF("Syntax error at line %d. String after {} scope (Last key parsed was %s=%s)", iLine, Q[0].Str(), Q[1].Str()));
#endif
				}

				CStr Word = XRG_ParseWord(_pStr, Pos, _Len);

				if (nQ == 0)
				{
					Word.MakeUpperCase();
					Q[0] = Word;
					Q[1].Clear();
					nQ++;
				}
				else if (nQ >= 1)
				{
					if (Word != "")
					{
						if (Q[1] != "" && Word != "")
							Q[1] = Q[1] + " " + Word;
						else
							Q[1] = Word;
						if (nQ == 1) nQ++;
					}
				}
			}
		}

		{
			if (nQ == 2)
				XRG_AddParseKey(Q[0], Q[1], _ParseContext, KeyOpt);
			else if (nQ == 1)
				XRG_AddParseKey(Q[0], "", _ParseContext, KeyOpt);

			nQ = 0;
		}

	}
#if M_EXCEPTIONS
	M_CATCH(
	catch(CCException)
	{
		int iLine = GetLineNumber(_pOrgData, &_pStr[Pos], _Size + (_pStr - _pOrgData)) + 1;
		Error_static("CRegistry_Dynamic::XRG_Parse", CStrF("Exception at line %d. (See previous errors)", iLine));
	}
	)
#endif
//	if (!_bEnterScope && bInScope)
//		Error_static("CRegistry_Dynamic::ReadFromMemory_r", "Unexpected end-of-file in scope.");

	m_lChildren.OptimizeMemory();

	return Pos;
}

int CRegistry_Dynamic::XRG_Parse(char* _pOrgData, char* _pStr, int _Size, CRegistry_ParseContext& _ParseContext)
{
	return XRG_ParseImp(_pOrgData, _pStr, _Size, _ParseContext);
}

int CRegistry_Dynamic::XRG_Parse(wchar* _pOrgData, wchar* _pStr, int _Size, CRegistry_ParseContext& _ParseContext)
{
	return XRG_ParseImp(_pOrgData, _pStr, _Size, _ParseContext);
}

#define XRG_PREPROCESS_SCOPEENABLE	1
#define XRG_PREPROCESS_IFTAKEN		2
#define XRG_PREPROCESS_PARENTSCOPEENABLE	4

bint CRegistry_Dynamic::XRG_ParseDefine(CStr _Line, TArray<CStr> _lDefines)
{
	bint bMatch = false;
	int LineLen = _Line.Len();
	int LinePos = 0;
	int iDef = 0;
	const char* pLine = _Line.Str();
	while(LinePos < LineLen && !bMatch)
	{
		Parse_WhiteSpace(pLine, LinePos, LineLen);
		if (LinePos >= LineLen)
			break;

		if (pLine[LinePos] == ',')
		{
			LinePos++;
			continue;
		}

		CStr Def = XRG_ParseWord(pLine, LinePos, LineLen);
		if (Def != "")
		{
			int nDef = _lDefines.Len();
			CStr* lDef = _lDefines.GetBasePtr();
			for(int i = 0; i < nDef; i++)
				if (lDef[i] == Def)
				{
					bMatch = true;
					break;
				}

			iDef++;
		}
	}

	return bMatch;
}

bint CRegistry_Dynamic::XRG_ParseDefineW(CStr _Line, TArray<CStr> _lDefines)
{
	bint bMatch = false;
	int LineLen = _Line.Len();
	int LinePos = 0;
	int iDef = 0;
	const wchar* pLine = _Line.StrW();
	while(LinePos < LineLen && !bMatch)
	{
		Parse_WhiteSpace(pLine, LinePos, LineLen);
		if (LinePos >= LineLen)
			break;

		if (pLine[LinePos] == ',')
		{
			LinePos++;
			continue;
		}

		CStr Def = XRG_ParseWord(pLine, LinePos, LineLen);
		if (Def != "")
		{
			int nDef = _lDefines.Len();
			CStr* lDef = _lDefines.GetBasePtr();
			for(int i = 0; i < nDef; i++)
				if (lDef[i] == Def)
				{
					bMatch = true;
					break;
				}

			iDef++;
		}
	}

	return bMatch;
}

int CRegistry_Dynamic::XRG_Preprocess(const char* _pSrc, int _Len, char* _pDst, TArray<CStr> _lDefines)
{
	// Aliasing _pDst and _pSrc is allowed
	const int nMaxStack = 32;
	uint8 lStack[nMaxStack];
	int iStack = 0;

	int Pos = 0;
	int DstPos = 0;
	lStack[0] = XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE;

	while(Pos < _Len)
	{
		Parse_WhiteSpace(_pSrc, Pos, _Len);
		if (Pos >= _Len) break;

		if (_pSrc[Pos] == '#')
		{
			Pos++;
			CStr Word = XRG_ParseWord(_pSrc, Pos, _Len);
			if (Word == "define")
			{
				if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
				{
					Parse_WhiteSpace(_pSrc, Pos, _Len);
					CStr DefName = XRG_ParseWord(_pSrc, Pos, _Len);

					if (DefName != "")
					{
						int nDef = _lDefines.Len();
						int i = 0;
						for(; i < nDef; i++)
							if (_lDefines[i] == DefName)
								break;
						if (i == nDef)
							_lDefines.Add(DefName);
					}
				}
			}
			else if (Word == "undef")
			{
				if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
				{
					Parse_WhiteSpace(_pSrc, Pos, _Len);
					CStr DefName = XRG_ParseWord(_pSrc, Pos, _Len);

					if (DefName != "")
					{
						int nDef = _lDefines.Len();
						int i = 0;
						for(; i < nDef; i++)
							if (_lDefines[i] == DefName)
							{
								_lDefines.Del(i);
								break;
							}
					}
				}
			}
			else if (Word == "ifdef")
			{
				if (iStack >= nMaxStack-1)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#if[n]def nesting is too deep.");
				}

				CStr Line = XRG_ParseLine(_pSrc, Pos, _Len);
				bint bResult = XRG_ParseDefine(Line, _lDefines);

				lStack[iStack+1] = 0;
				if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
					lStack[iStack+1] = XRG_PREPROCESS_PARENTSCOPEENABLE;
				if (bResult)
					lStack[iStack+1] |= XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_IFTAKEN;
				iStack++;
			}
			else if (Word == "ifndef")
			{
				if (iStack >= nMaxStack-1)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#if[n]def nesting is too deep.");
				}

				CStr Line = XRG_ParseLine(_pSrc, Pos, _Len);
				bint bResult = XRG_ParseDefine(Line, _lDefines);

				lStack[iStack+1] = 0;
				if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
					lStack[iStack+1] = XRG_PREPROCESS_PARENTSCOPEENABLE;
				if (!bResult)
					lStack[iStack+1] |= XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_IFTAKEN;
				iStack++;
			}
			else if (Word == "elif")
			{
				if (!iStack)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#elif without matching #if[n]def");
				}

				if (lStack[iStack] & XRG_PREPROCESS_IFTAKEN)
				{
					lStack[iStack] &= ~XRG_PREPROCESS_SCOPEENABLE;
				}
				else
				{
					CStr Line = XRG_ParseLine(_pSrc, Pos, _Len);
					bint bResult = XRG_ParseDefine(Line, _lDefines);

					iStack--;
					lStack[iStack+1] = 0;
					if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
						lStack[iStack+1] = XRG_PREPROCESS_PARENTSCOPEENABLE;
					if (bResult)
						lStack[iStack+1] |= XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_IFTAKEN;
					iStack++;
				}
			}
			else if (Word == "else")
			{
				if (!iStack)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#else without matching #if[n]def");
				}
				lStack[iStack] = lStack[iStack] ^ XRG_PREPROCESS_SCOPEENABLE;
			}
			else if (Word == "endif")
			{
				if (!iStack)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#endif without matching #if[n]def");
				}
				iStack--;
			}
		}
		else
		{
			// Not a preprocessor directive, skip or copy rest of the line to dest
			int EOL = FindChar(_pSrc + Pos, _Len - Pos, 10);
			if (EOL < 0)
				EOL = _Len - Pos;
			else
				EOL++;

			if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
			{
				memmove(_pDst+DstPos, _pSrc + Pos, EOL);
				DstPos += EOL;
			}
			Pos += EOL;
		}
	}

	return DstPos;
}

template<class T>
static void Parse_NextLine(const T* p, int& _Pos, int _Len)
{
	while((_Pos < _Len) && CStr::IsWhiteSpace(p[_Pos])) _Pos++;
}

int CRegistry_Dynamic::XRG_Preprocess(const wchar* _pSrc, int _Len, wchar* _pDst, TArray<CStr> _lDefines)
{
	// Aliasing _pDst and _pSrc is allowed
	const int nMaxStack = 32;
	uint8 lStack[nMaxStack];
	int iStack = 0;

	int Pos = 0;
	int DstPos = 0;
	lStack[0] = XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE;

	while(Pos < _Len)
	{
		Parse_WhiteSpace(_pSrc, Pos, _Len);
		if (Pos >= _Len) break;

		if (_pSrc[Pos] == '#')
		{
			Pos++;
			CStr Word = XRG_ParseWord(_pSrc, Pos, _Len);
			if (Word == "define")
			{
				if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
				{
					Parse_WhiteSpace(_pSrc, Pos, _Len);
					CStr DefName = XRG_ParseWord(_pSrc, Pos, _Len);

					if (DefName != "")
					{
						int nDef = _lDefines.Len();
						int i = 0;
						for(; i < nDef; i++)
							if (_lDefines[i] == DefName)
								break;
						if (i == nDef)
							_lDefines.Add(DefName);
					}
				}
			}
			else if (Word == "undef")
			{
				if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
				{
					Parse_WhiteSpace(_pSrc, Pos, _Len);
					CStr DefName = XRG_ParseWord(_pSrc, Pos, _Len);

					if (DefName != "")
					{
						int nDef = _lDefines.Len();
						int i = 0;
						for(; i < nDef; i++)
							if (_lDefines[i] == DefName)
							{
								_lDefines.Del(i);
								break;
							}
					}
				}
			}
			else if (Word == "ifdef")
			{
				if (iStack >= nMaxStack-1)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#if[n]def nesting is too deep.");
				}

				CStr Line = XRG_ParseLine(_pSrc, Pos, _Len);
				bint bResult = XRG_ParseDefine(Line, _lDefines);

				lStack[iStack+1] = 0;
				if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
					lStack[iStack+1] = XRG_PREPROCESS_PARENTSCOPEENABLE;
				if (bResult)
					lStack[iStack+1] |= XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_IFTAKEN;
				iStack++;
			}
			else if (Word == "ifndef")
			{
				if (iStack >= nMaxStack-1)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#if[n]def nesting is too deep.");
				}

				CStr Line = XRG_ParseLine(_pSrc, Pos, _Len);
				bint bResult = XRG_ParseDefine(Line, _lDefines);

				lStack[iStack+1] = 0;
				if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
					lStack[iStack+1] = XRG_PREPROCESS_PARENTSCOPEENABLE;
				if (!bResult)
					lStack[iStack+1] |= XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_IFTAKEN;
				iStack++;
			}
			else if (Word == "elif")
			{
				if (!iStack)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#elif without matching #if[n]def");
				}

				if (lStack[iStack] & XRG_PREPROCESS_IFTAKEN)
				{
					lStack[iStack] &= ~XRG_PREPROCESS_SCOPEENABLE;
				}
				else
				{
					CStr Line = XRG_ParseLine(_pSrc, Pos, _Len);
					bint bResult = XRG_ParseDefine(Line, _lDefines);

					iStack--;
					lStack[iStack+1] = 0;
					if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
						lStack[iStack+1] = XRG_PREPROCESS_PARENTSCOPEENABLE;
					if (bResult)
						lStack[iStack+1] |= XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_IFTAKEN;
					iStack++;
				}
			}
			else if (Word == "else")
			{
				if (!iStack)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#else without matching #if[n]def");
				}
				lStack[iStack] = lStack[iStack] ^ XRG_PREPROCESS_SCOPEENABLE;
			}
			else if (Word == "endif")
			{
				if (!iStack)
				{
					Error_static("CRegistry_Dynamic::XRG_Preprocess", "#endif without matching #if[n]def");
				}
				iStack--;
			}
		}
		else
		{
			// Not a preprocessor directive, skip or copy rest of the line to dest
			int EOL = FindChar(_pSrc + Pos, _Len - Pos, 10);
			if (EOL < 0)
				EOL = _Len - Pos;
			else
				EOL++;

			if ((lStack[iStack] & (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE)) == (XRG_PREPROCESS_SCOPEENABLE | XRG_PREPROCESS_PARENTSCOPEENABLE))
			{
				memcpy(_pDst+DstPos, _pSrc + Pos, EOL*sizeof(wchar));
				DstPos += EOL;
			}
			Pos += EOL;
		}
	}

	return DstPos;
}

void CRegistry_Dynamic::XRG_Read(CCFile *_pFile, CStr _ThisFileName, TArray<CStr> _lDefines, bint _bSlashIsEscape)
{
	MAUTOSTRIP(CRegistry_Dynamic_XRG_Read, MAUTOSTRIP_VOID);
	MSCOPE(CRegistry_Dynamic::XRG_Read, RES_REGISTRY);
//LogFile("ReadASE...");
	int Size = _pFile->Length();
//LogFile(CStrF("ReadASE...  %d", Size));
	char* pData = DNew(char) char[Size+2];
	if (!pData) MemError_static("ReadASE");
	M_TRY
	{
		_pFile->Read(pData, Size);
		_pFile->Close();
		pData[Size] = 0;
		pData[Size+1] = 0;

		CRegistry_ParseContext Context;
		Context.m_ThisFileName = _ThisFileName;
		Context.m_lDefines = _lDefines;

		if (Size >= 2 && pData[0] == char(0xff) && pData[1] == char(0xfe))
		{
			// It's a unicode file, skip header and cast to wchar
			wchar* pWData = (wchar*)(pData+2);
			int Len = (Size-2) >> 1;
			M_ASSERT(sizeof(wchar) == sizeof(uint16), "wrong typedef");
			SwitchArrayLE_uint16((uint16*)pWData, Len);

			Len = XRG_Preprocess(pWData, Len, pWData, _lDefines);
#if 0 //!defined(M_RTM)	// Not thread-safe
			MACRO_GetSystemEnvironment(pEnv);
			if (_ThisFileName != "" && pEnv && pEnv->GetValuei("REGISTRYDEBUG"))
			{
				LogFile(CStrF("Preprocessed: %s", _ThisFileName.Str() ));
				CCFile PPF;
				PPF.Open(_ThisFileName + ".preprocess.txt", CFILE_UNICODE | CFILE_WRITE);
				PPF.Write(pWData, Len*2);
				PPF.Writeln("-----------------------------------------------------------");
				PPF.Writeln("Defines after preprocessing:");
				PPF.Writeln("-----------------------------------------------------------");
				for(int i = 0; i < _lDefines.Len(); i++)
					PPF.Writeln(_lDefines[i]);
				PPF.Close();
			}
#endif

			XRG_Parse(pWData, pWData, Len, Context /*, _bSlashIsEscape*/);
		}
		else
		{
			// It's an ANSI file
			Size = XRG_Preprocess(pData, Size, pData, _lDefines);
#if 0 //!defined(M_RTM)	// Not thread-safe
			MACRO_GetSystemEnvironment(pEnv);
			if (_ThisFileName != "" && pEnv && pEnv->GetValuei("REGISTRYDEBUG"))
			{
				LogFile(CStrF("Preprocessed: %s", _ThisFileName.Str() ));
				CCFile PPF;
				PPF.Open(_ThisFileName + ".preprocess.txt", CFILE_BINARY | CFILE_WRITE);
				PPF.Write(pData, Size);
				PPF.Writeln("-----------------------------------------------------------");
				PPF.Writeln("Defines after preprocessing:");
				PPF.Writeln("-----------------------------------------------------------");
				for(int i = 0; i < _lDefines.Len(); i++)
					PPF.Writeln(_lDefines[i]);
				PPF.Close();
			}
#endif
			XRG_Parse(pData, pData, Size, Context /*, _bSlashIsEscape*/);
		}
	}
	M_CATCH(
	catch(CCException)
	{
		delete[] pData;
		throw;
//		Error_static("CRegistry_Dynamic::XRG_Read", CStrF("Exception reading file %s", _Filename.Str()));
	}
	)
	delete[] pData;
//LogFile("ReadASE done. ");
}


void CRegistry_Dynamic::XRG_Read(const CStr& _Filename, TArray<CStr> _lDefines, bint _bSlashIsEscape)
{
	MAUTOSTRIP(CRegistry_Dynamic_XRG_Read, MAUTOSTRIP_VOID);
	MSCOPE(CRegistry_Dynamic::XRG_Read, RES_REGISTRY);
	CCFile File;
	File.Open(_Filename, CFILE_READ | CFILE_BINARY);
	XRG_Read(&File, _Filename, _lDefines, _bSlashIsEscape);

#if 0 //!defined(M_RTM)	// Not thread-safe
	MACRO_GetSystemEnvironment(pEnv);
	if (pEnv && pEnv->GetValuei("REGISTRYDEBUG"))
	{
		XRG_Write(_Filename + ".read.txt");
	}
#endif
}

void CRegistry_Dynamic::XRG_Read(const CStr& _Filename)
{
	XRG_Read(_Filename, TArray<CStr>::TArray(), true);
}

void CRegistry_Dynamic::ReadSimple(CCFile* _pFile)
{
	MSCOPE(CRegistry_Dynamic::ReadSimple, RES_REGISTRY);
	while (!(_pFile->EndOfFile()))
	{
		CStr s = _pFile->Readln();
//LogFile(s);
		CStr Name = s.GetStrSep("=").UpperCase();
		CStr Val = s.GetStrSep("=");
		if (Name != "")
		{
//		LogFile(("NAME=" + Name).Ansi());
//		LogFile(("VAL=" + Val).Ansi());
			if (Val.IsPureAnsi())
				Val = Val.Ansi();
			SetValue(Name.Ansi(), Val);
		}
	};
}

void CRegistry_Dynamic::ReadSimple(CStr _FileName)
{
	MSCOPE(CRegistry_Dynamic::ReadSimple, RES_REGISTRY);
	TPtr<CCFile> spF = MNew(CCFile);
	if (spF == NULL) MemError_static("Load");

	spF->Open(_FileName, CFILE_READ);
	ReadSimple(spF);
	spF->Close();
}

void CRegistry_Dynamic::ReadRegistryDir(CStr _Dir, CCFile* _pFile)
{
	CreateDir(_Dir);
	spCRegistry spReg = GetDir(_Dir);
	if (spReg != NULL)
	{
		spReg->ReadSimple(_pFile);
	}
}

void CRegistry_Dynamic::ReadRegistryDir(CStr _Dir, CStr _Filename)
{
	MAUTOSTRIP(CRegistry_Dynamic_ReadRegistryDir, MAUTOSTRIP_VOID);
	CreateDir(_Dir);
	spCRegistry spReg = GetDir(_Dir);
	if (spReg!=NULL)
	{
		spReg->ReadSimple(_Filename);
	}
}

// -------------------------------------------------------------------
//  Binary IO
// -------------------------------------------------------------------
void CRegistry_Dynamic::Read(CCFile* _pFile, int _Version)
{
	MSCOPE(CRegistry_Dynamic::Read, RES_REGISTRY);
	MAUTOSTRIP(CRegistry_Dynamic_Read, MAUTOSTRIP_VOID);
	
	if (_Version == 0)
	{

		int16 Flags;
		int16 Type;
		CStr Name;
		_pFile->ReadLE(Flags);
		_pFile->ReadLE(Type);
		Name.Read(_pFile);
		m_spName = Name.GetStrData();

		m_InternalFlags = 0;
		m_UserFlags = (Flags & DBitRange(8,15)) >> 8;


		switch(Type)
		{
		case 1: //REGISTRY_TYPE_STR
		case 2: // REGISTRY_TYPE_WSTR
			{
				CStr Value;
				Value.Read(_pFile);
				Type_SetNew(REGISTRY_TYPE_STR, 1);
				Type_Get_Str() = Value.GetStrData();
			}
			break;

		case 3: //REGISTRY_TYPE_INT
			{
				Type_SetNew(REGISTRY_TYPE_INT32, 1);
				_pFile->ReadLE(Type_Get_int32());
			}
			break;

		case 4: //REGISTRY_TYPE_FLOAT
			{
				Type_SetNew(REGISTRY_TYPE_FP4, 1);
				_pFile->ReadLE(Type_Get_fp4());
			}
			break;
		default :
			Error_static("CRegistry_Dynamic::Read", CStrF("Unknown data-type. %d, Key %s", m_Type, (char*)CStr(m_spName.p)));
		}

		int16 nCh;
		_pFile->ReadLE(nCh);

		SetNumChildren(nCh);
		for(int i = 0; i < nCh; i++)
			m_lChildren[i].m_spReg->Read(_pFile, _Version);

		Private_Hash_Invalidate();
	}
	else if (_Version == 0x201)
	{
		uint32 Flags;
		_pFile->ReadLE(Flags);
		uint32 NewInternal = Flags & DBitRange(0,3);
		m_InternalFlags = (m_InternalFlags & ~(ERegistryDynamicFlags_HashDirty)) | (NewInternal & (ERegistryDynamicFlags_Animated | ERegistryDynamicFlags_DisableAnimAutoDemote));
		m_UserFlags = (Flags & DBitRange(8,15)) >> 8;
		int Type = (Flags & DBitRange(16,19)) >> 16;


		CStr Name;
		Name.Read(_pFile);
		m_spName = Name.GetStrData();

		Type_SetNew(Type, 1);

		switch(m_Type)
		{
		case REGISTRY_TYPE_VOID:
			{
			}
			break;
		case REGISTRY_TYPE_STR:
			{
				CStr Value;
				Value.Read(_pFile);
				Type_Get_Str() = Value.GetStrData();
			}
			break;
		case REGISTRY_TYPE_DATA:
			{
				uint32 Size;
				_pFile->ReadLE(Size);
				Type_Get_Data().SetLen(Size);
				_pFile->Read(Type_Get_Data().GetBasePtr(), Size);
			}
			break;
		case REGISTRY_TYPE_UINT8:
			{
				_pFile->ReadLE(Type_Get_uint8());
			}
			break;
		case REGISTRY_TYPE_INT16:
			{
				_pFile->ReadLE(Type_Get_int16());
			}
			break;
		case REGISTRY_TYPE_INT32:
			{
				_pFile->ReadLE(Type_Get_int32());
			}
			break;
		case REGISTRY_TYPE_UINT32:
			{
				_pFile->ReadLE(Type_Get_uint32());
			}
			break;
		case REGISTRY_TYPE_FP4:
			{
				_pFile->ReadLE(Type_Get_fp4());
			}
			break;
		default :
			Error_static("CRegistry_Dynamic::Read", CStrF("Unknown data-type. %d, Key %s", m_Type, GetThisName().Str()));
		}

		uint16 nCh;
		_pFile->ReadLE(nCh);

		SetNumChildren(nCh);
		for(int i = 0; i < nCh; i++)
			m_lChildren[i].m_spReg->Read(_pFile, _Version);
		Private_Hash_Invalidate();
	}
	else if (_Version >= 0x202)
	{
		Type_Clear();

		uint32 Flags;
		_pFile->ReadLE(Flags);
		int nDim = 0;
		uint32 Internal = 0;
		int Type = 0;
		if (_Version == 0x202)
		{
			nDim = Flags & DBitRange(0,3);
			Internal = (Flags & DBitRange(4,7)) >> 4;
			m_InternalFlags = (m_InternalFlags & ~(ERegistryDynamicFlags_HashDirty)) | (Internal & (ERegistryDynamicFlags_Animated | ERegistryDynamicFlags_DisableAnimAutoDemote));
			m_UserFlags = (Flags & DBitRange(8,15)) >> 8;
			Type = (Flags & DBitRange(16,19)) >> 16;
		}
		else if (_Version == 0x203)
		{
			nDim = Flags & DBitRange(0,4);
			Internal = (Flags & DBitRange(5,8)) >> 5;
			m_InternalFlags = (m_InternalFlags & ~(ERegistryDynamicFlags_HashDirty)) | (Internal & (ERegistryDynamicFlags_Animated | ERegistryDynamicFlags_DisableAnimAutoDemote));
			m_UserFlags = (Flags & DBitRange(9,16)) >> 9;
			Type = (Flags & DBitRange(17,20)) >> 17;
		}
		else if (_Version >= 0x204)
		{
			nDim = (Flags & DBitRange(0,4)) + 1;
			Internal = (Flags & DBitRange(5,8)) >> 5;
			m_InternalFlags = (m_InternalFlags & ~(ERegistryDynamicFlags_HashDirty)) | (Internal & (ERegistryDynamicFlags_Animated | ERegistryDynamicFlags_DisableAnimAutoDemote));
			m_UserFlags = (Flags & DBitRange(9,16)) >> 9;
			Type = (Flags & DBitRange(17,20)) >> 17;
		}
		else
		{
			M_ASSERT(0, "");
		}

		

		CStr Name;
		Name.Read(_pFile);
		m_spName = Name.GetStrData();

		if (m_InternalFlags & ERegistryDynamicFlags_Animated)
		{
			m_InternalFlags &= ~ERegistryDynamicFlags_Animated;
			Private_Anim_Promote();
			m_Type = Type;
			m_Dimensions = nDim-1;
			CAnimationData &AnimData = Type_GetAnimationData();

			uint32 AnimFlags;
			uint32 nSeq;
			_pFile->ReadLE(AnimFlags);

			uint32 nIPData;
			_pFile->ReadLE(nIPData);
			AnimData.m_InterpolateData.SetLen(nIPData);
			_pFile->ReadLE(AnimData.m_InterpolateData.GetBasePtr(), nIPData);

			_pFile->ReadLE(nSeq);

			AnimData.m_AnimFlags = (AnimFlags & DBitRange(4,19)) >> 4;
			AnimData.m_AnimIPType = (AnimFlags & DBitRange(20,27)) >> 20;
			AnimData.m_InternalAnimFlags = AnimFlags & DBitRange(0,3);
			AnimData.m_Sequences.SetLen(nSeq);

			for (int i = 0; i < nSeq; ++i)
			{
				CAnimationSequence *pSeq;
				if (i == 0)
					pSeq = AnimData.m_Sequences[i];
				else
					pSeq = AnimData.m_Sequences[i] = DNew(CAnimationSequence) CAnimationSequence;

				_pFile->ReadLE(pSeq->m_LoopStart);
				_pFile->ReadLE(pSeq->m_LoopEnd);
				uint32 nKF;
				_pFile->ReadLE(nKF);

				pSeq->m_nKeyFrames = nKF;

				mint Size;
				if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
					Size = ms_lTypeSize[m_Type] * nDim + sizeof(fp4);
				else
					Size = ms_lTypeSize[m_Type] * nDim;
				pSeq->m_pData = M_ALLOC(Size * pSeq->m_nKeyFrames);

				bint bTimed = AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed;

				if (bTimed)
					Private_Anim_ConstructDataTimed(pSeq->m_pData, 0, pSeq->m_nKeyFrames, nDim);
				else
					Private_Anim_ConstructData(pSeq->m_pData, 0, pSeq->m_nKeyFrames, nDim);

				switch(m_Type)
				{
				case REGISTRY_TYPE_VOID:
					{
					}
					break;
				case REGISTRY_TYPE_STR:
					{
						for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
						{
							CStr::spCStrData *pData = (CStr::spCStrData *)Private_Anim_GetKFData(pSeq, j);
							for (int i = 0; i < nDim; ++i)
							{
								CStr Value;
								Value.Read(_pFile);
								pData[i] = Value.GetStrData();
							}
							if (bTimed)
							{
								fp4 Time;
								_pFile->ReadLE(Time);
								Private_Anim_SetKFTime(pSeq->m_pData, j, Time);
							}
						}
					}
					break;
				case REGISTRY_TYPE_DATA:
					{
						for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
						{
							TArray<uint8> *pData = (TArray<uint8> *)Private_Anim_GetKFData(pSeq, j);
							for (int i = 0; i < nDim; ++i)
							{
								uint32 Size;
								_pFile->ReadLE(Size);
								pData[i].SetLen(Size);
								_pFile->Read(pData[i].GetBasePtr(), Size);
							}
							if (bTimed)
							{
								fp4 Time;
								_pFile->ReadLE(Time);
								Private_Anim_SetKFTime(pSeq->m_pData, j, Time);
							}
						}
					}
					break;
				case REGISTRY_TYPE_UINT8:
					{
						for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
						{
							void *pData = Private_Anim_GetKFData(pSeq, j);
							_pFile->ReadLE(((uint8 *)pData), nDim);
							if (bTimed)
							{
								fp4 Time;
								_pFile->ReadLE(Time);
								Private_Anim_SetKFTime(pSeq->m_pData, j, Time);
							}
						}
					}
					break;
				case REGISTRY_TYPE_INT16:
					{
						for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
						{
							void *pData = Private_Anim_GetKFData(pSeq, j);
							_pFile->ReadLE(((int16 *)pData), nDim);
							if (bTimed)
							{
								fp4 Time;
								_pFile->ReadLE(Time);
								Private_Anim_SetKFTime(pSeq->m_pData, j, Time);
							}
						}
					}
					break;
				case REGISTRY_TYPE_INT32:
					{
						for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
						{
							void *pData = Private_Anim_GetKFData(pSeq, j);
							_pFile->ReadLE(((int32 *)pData), nDim);
							if (bTimed)
							{
								fp4 Time;
								_pFile->ReadLE(Time);
								Private_Anim_SetKFTime(pSeq->m_pData, j, Time);
							}
						}
					}
					break;
				case REGISTRY_TYPE_UINT32:
					{
						for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
						{
							void *pData = Private_Anim_GetKFData(pSeq, j);
							_pFile->ReadLE(((uint32 *)pData), nDim);
							if (bTimed)
							{
								fp4 Time;
								_pFile->ReadLE(Time);
								Private_Anim_SetKFTime(pSeq->m_pData, j, Time);
							}
						}
					}
					break;
				case REGISTRY_TYPE_FP4:
					{
						for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
						{
							void *pData = Private_Anim_GetKFData(pSeq, j);
							_pFile->ReadLE(((fp4 *)pData), nDim);
							if (bTimed)
							{
								fp4 Time;
								_pFile->ReadLE(Time);
								Private_Anim_SetKFTime(pSeq->m_pData, j, Time);
							}
						}
					}
					break;
				case REGISTRY_TYPE_FP2:
					{
						for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
						{
							void *pData = Private_Anim_GetKFData(pSeq, j);
							_pFile->ReadLE((((uint16 *)pData)), nDim);
							if (bTimed)
							{
								fp4 Time;
								_pFile->ReadLE(Time);
								Private_Anim_SetKFTime(pSeq->m_pData, j, Time);
							}
						}
					}
					break;
				default :
					Error_static("CRegistry_Dynamic::Read", CStrF("Unknown data-type. %d, Key %s", m_Type, GetThisName().Str()));
				}
			}
		}
		else
		{
			if (nDim < 1)
				nDim = 1;
			Type_SetNew(Type, nDim);

			switch(m_Type)
			{
			case REGISTRY_TYPE_VOID:
				{
				}
				break;
			case REGISTRY_TYPE_STR:
				{
					CStr::spCStrData *pData = (CStr::spCStrData *)Type_GetValue();
					for (int i = 0; i < nDim; ++i)
					{
						CStr Value;
						Value.Read(_pFile);
						pData[i] = Value.GetStrData();
					}
				}
				break;
			case REGISTRY_TYPE_DATA:
				{
					TArray <uint8> *pArrays = (TArray <uint8> *)Type_GetValue();
					for (int i = 0; i < nDim; ++i)
					{
						uint32 Size;
						_pFile->ReadLE(Size);
						pArrays[i].SetLen(Size);
						_pFile->Read(pArrays[i].GetBasePtr(), Size);
					}
				}
				break;
			case REGISTRY_TYPE_UINT8:
				{
					_pFile->ReadLE((uint8 *)Type_GetValue(), nDim);
				}
				break;
			case REGISTRY_TYPE_INT16:
				{
					_pFile->ReadLE((int16 *)Type_GetValue(), nDim);
				}
				break;
			case REGISTRY_TYPE_INT32:
				{
					_pFile->ReadLE((int32 *)Type_GetValue(), nDim);
				}
				break;
			case REGISTRY_TYPE_UINT32:
				{
					_pFile->ReadLE((uint32 *)Type_GetValue(), nDim);
				}
				break;
			case REGISTRY_TYPE_FP4:
				{
					_pFile->ReadLE((fp4 *)Type_GetValue(), nDim);
				}
				break;
			case REGISTRY_TYPE_FP2:
				{
					_pFile->ReadLE((uint16 *)Type_GetValue(), nDim);
				}
				break;
			default :
				Error_static("CRegistry_Dynamic::Read", CStrF("Unknown data-type. %d, Key %s", m_Type, GetThisName().Str()));
			}
		}

		uint16 nCh;
		_pFile->ReadLE(nCh);

		SetNumChildren(nCh);
		for(int i = 0; i < nCh; i++)
			m_lChildren[i].m_spReg->Read(_pFile, _Version);

		Private_Hash_Invalidate();
	}
	else
		Error_static("CRegistry_Dynamic::Read", "Unsupported file version found");
}

void CRegistry_Dynamic::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CRegistry_Dynamic_Write, MAUTOSTRIP_VOID);
	
	uint32 Flags = m_Dimensions;
	Flags |= m_InternalFlags << 5;
	Flags |= m_UserFlags << 9;
	Flags |= m_Type << 17;
	_pFile->WriteLE(Flags);
	CStr(m_spName.p).Write(_pFile);

	if (m_InternalFlags & ERegistryDynamicFlags_Animated) 
	{
		const CAnimationData &AnimData = Type_GetAnimationData();
		uint32 AnimFlags = AnimData.m_InternalAnimFlags;
		AnimFlags |= AnimData.m_AnimFlags << 4;
		AnimFlags |= AnimData.m_AnimIPType << 20;
		_pFile->WriteLE(AnimFlags);
		_pFile->WriteLE((uint32)AnimData.m_InterpolateData.Len());
		_pFile->WriteLE(AnimData.m_InterpolateData.GetBasePtr(), AnimData.m_InterpolateData.Len());
		_pFile->WriteLE((uint32)AnimData.m_Sequences.Len());

		bint bTimed = AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed;

		for (int i = 0; i < AnimData.m_Sequences.Len(); ++i)
		{
			const CAnimationSequence *pSeq = AnimData.m_Sequences[i];
			_pFile->WriteLE(pSeq->m_LoopStart);
			_pFile->WriteLE(pSeq->m_LoopEnd);
			_pFile->WriteLE((uint32)pSeq->m_nKeyFrames);
			switch(m_Type)
			{
			case REGISTRY_TYPE_VOID:
				{
				}
				break;
			case REGISTRY_TYPE_STR:
				{
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						CStr::spCStrData *pData = (CStr::spCStrData *)Private_Anim_GetKFData(pSeq, j);
						for (int i = 0; i < (m_Dimensions+1); ++i)
						{
							CStr Value(pData[i]);
							Value.Write(_pFile);
						}
						if (bTimed)
							_pFile->WriteLE(Private_Anim_GetKFTimeTimed(pSeq, j));
					}
				}
				break;
			case REGISTRY_TYPE_DATA:
				{
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						const TArray<uint8> *pData = (const TArray<uint8> *)Private_Anim_GetKFData(pSeq, j);
						for (int i = 0; i < (m_Dimensions+1); ++i)
						{
							uint32 Size = pData[i].Len();
							_pFile->WriteLE(Size);
							_pFile->Write(pData[i].GetBasePtr(), Size);
						}
						if (bTimed)
							_pFile->WriteLE(Private_Anim_GetKFTimeTimed(pSeq, j));
					}
				}
				break;
			case REGISTRY_TYPE_UINT8:
				{
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						const void *pData = Private_Anim_GetKFData(pSeq, j);
						_pFile->WriteLE(((uint8 *)pData), (m_Dimensions+1));
						if (bTimed)
							_pFile->WriteLE(Private_Anim_GetKFTimeTimed(pSeq, j));
					}
				}
				break;
			case REGISTRY_TYPE_INT16:
				{
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						const void *pData = Private_Anim_GetKFData(pSeq, j);
						_pFile->WriteLE(((int16 *)pData), (m_Dimensions+1));
						if (bTimed)
							_pFile->WriteLE(Private_Anim_GetKFTimeTimed(pSeq, j));
					}
				}
				break;
			case REGISTRY_TYPE_INT32:
				{
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						const void *pData = Private_Anim_GetKFData(pSeq, j);
						_pFile->WriteLE(((int32 *)pData), (m_Dimensions+1));
						if (bTimed)
							_pFile->WriteLE(Private_Anim_GetKFTimeTimed(pSeq, j));
					}
				}
				break;
			case REGISTRY_TYPE_UINT32:
				{
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						const void *pData = Private_Anim_GetKFData(pSeq, j);
						_pFile->WriteLE(((uint32 *)pData), (m_Dimensions+1));
						if (bTimed)
							_pFile->WriteLE(Private_Anim_GetKFTimeTimed(pSeq, j));
					}
				}
				break;
			case REGISTRY_TYPE_FP4:
				{
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						const void *pData = Private_Anim_GetKFData(pSeq, j);
						_pFile->WriteLE(((fp4 *)pData), (m_Dimensions+1));
						if (bTimed)
							_pFile->WriteLE(Private_Anim_GetKFTimeTimed(pSeq, j));
					}
				}
				break;
			case REGISTRY_TYPE_FP2:
				{
					for (int j = 0; j < pSeq->m_nKeyFrames; ++j)
					{
						const void *pData = Private_Anim_GetKFData(pSeq, j);
						_pFile->WriteLE(((uint16 *)pData), (m_Dimensions+1));
						if (bTimed)
							_pFile->WriteLE(Private_Anim_GetKFTimeTimed(pSeq, j));
					}
				}
				break;
			default :
				Error_static("CRegistry_Dynamic::Read", CStrF("Unknown data-type. %d, Key %s", m_Type, GetThisName().Str()));
			}
		}
	}
	else
	{
		switch(m_Type)
		{
		case REGISTRY_TYPE_VOID:
			{
			}
			break;
		case REGISTRY_TYPE_STR:
			{
				const CStr::spCStrData *pData = (const CStr::spCStrData *)Type_GetValue();
				for (int i = 0; i < (m_Dimensions+1); ++i)
				{
					CStr Value(pData[i].p);
					Value.Write(_pFile);
				}
			}
			break;
		case REGISTRY_TYPE_DATA:
			{
				const TArray <uint8> *pArrays = (const TArray <uint8> *)Type_GetValue();
				for (int i = 0; i < (m_Dimensions+1); ++i)
				{
					uint32 Size = pArrays[i].Len();
					_pFile->WriteLE(Size);
					_pFile->Write(pArrays[i].GetBasePtr(), Size);
				}
			}
			break;
		case REGISTRY_TYPE_UINT8:
			{
				_pFile->WriteLE((uint8 *)Type_GetValue(), (m_Dimensions+1));
			}
			break;
		case REGISTRY_TYPE_INT16:
			{
				_pFile->WriteLE((int16 *)Type_GetValue(), (m_Dimensions+1));
			}
			break;
		case REGISTRY_TYPE_INT32:
			{
				_pFile->WriteLE((int32 *)Type_GetValue(), (m_Dimensions+1));
			}
			break;
		case REGISTRY_TYPE_UINT32:
			{
				_pFile->WriteLE((uint32 *)Type_GetValue(), (m_Dimensions+1));
			}
			break;
		case REGISTRY_TYPE_FP4:
			{
				_pFile->WriteLE((fp4 *)Type_GetValue(), (m_Dimensions+1));
			}
			break;
		case REGISTRY_TYPE_FP2:
			{
				_pFile->WriteLE((uint16 *)Type_GetValue(), (m_Dimensions+1));
			}
			break;
		default :
			Error_static("CRegistry_Dynamic::Read", CStrF("Unknown data-type. %d, Key %s", m_Type, GetThisName().Str()));
		}
	}

	uint16 nCh = GetNumChildren();
	_pFile->WriteLE(nCh);

	for(int i = 0; i < nCh; i++)
		m_lChildren[i].m_spReg->Write(_pFile);
}

bint CRegistry_Dynamic::Read(CDataFile* _pDFile, const char* _pEntryName)
{
	MSCOPE(CRegistry_Dynamic::Read, RES_REGISTRY);
	MAUTOSTRIP(CRegistry_Dynamic_Read_2, false);
	_pDFile->PushPosition();
	if (!_pDFile->GetNext(_pEntryName))
	{
		_pDFile->PopPosition();
		return false;
	}
	Read(_pDFile->GetFile(), _pDFile->GetUserData());
	_pDFile->PopPosition();
	return true;
}

void CRegistry_Dynamic::Write(CDataFile* _pDFile, const char* _pEntryName) const
{
	MAUTOSTRIP(CRegistry_Dynamic_Write_2, MAUTOSTRIP_VOID);
	_pDFile->BeginEntry(_pEntryName);
	Write(_pDFile->GetFile());
	_pDFile->EndEntry(REGISTRY_BINARY_VERSION);
}

void CRegistry_Dynamic::Read(const char* _pFileName)
{
	MSCOPE(CRegistry_Dynamic::Read, RES_REGISTRY);
	MAUTOSTRIP(CRegistry_Dynamic_Read_3, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Open(_pFileName);
	Read(&DFile, "REGISTRY");
	DFile.Close();
}

void CRegistry_Dynamic::Write(const char* _pFileName) const
{
	MAUTOSTRIP(CRegistry_Dynamic_Write_3, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Create(_pFileName);
	Write(&DFile, "REGISTRY");
	DFile.Close();
}


/////////////////////////////////////////////////////////////////
// Animation
////////////////////////////////////////////////////////////////


void CRegistry_Dynamic::Private_Anim_DestroyData(void *_pDst, int _iStartDst, int _nItems, int _nDim)
{
	if (ms_lTypeDestructorAnim[m_Type])
	{
		int iEnd = _iStartDst + _nItems;
		mint ItemSize = ms_lTypeSize[m_Type] * _nDim;
		for (int i = _iStartDst; i < iEnd; ++i)
		{
			uint8 *pDstItem = (uint8 *)_pDst + ItemSize * (i);
			ms_lTypeDestructorAnim[m_Type](pDstItem, _nDim);
		}
	}
}

void CRegistry_Dynamic::Private_Anim_ConstructData(void *_pDst, int _iStartDst, int _nItems, int _nDim)
{
	if (ms_lTypeConstructorAnim[m_Type])
	{
		int iEnd = _iStartDst + _nItems;
		mint ItemSize = ms_lTypeSize[m_Type] * _nDim;
		for (int i = _iStartDst; i < iEnd; ++i)
		{
			uint8 *pDstItem = (uint8 *)_pDst + ItemSize * (i);
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDim);
		}
	}
}

void CRegistry_Dynamic::Private_Anim_DestroyDataTimed(void *_pDst, int _iStartDst, int _nItems, int _nDim)
{
	if (ms_lTypeDestructorAnim[m_Type])
	{
		int iEnd = _iStartDst + _nItems;
		mint ItemSize = ms_lTypeSize[m_Type] * _nDim + sizeof(fp4);
		for (int i = _iStartDst; i < iEnd; ++i)
		{
			uint8 *pDstItem = (uint8 *)_pDst + ItemSize * (i);
			ms_lTypeDestructorAnim[m_Type](pDstItem, _nDim);
		}
	}
}

void CRegistry_Dynamic::Private_Anim_ConstructDataTimed(void *_pDst, int _iStartDst, int _nItems, int _nDim)
{
	if (ms_lTypeConstructorAnim[m_Type])
	{
		int iEnd = _iStartDst + _nItems;
		mint ItemSize = ms_lTypeSize[m_Type] * _nDim + sizeof(fp4);
		for (int i = _iStartDst; i < iEnd; ++i)
		{
			uint8 *pDstItem = (uint8 *)_pDst + ItemSize * (i);
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDim);
		}
	}
}

void CRegistry_Dynamic::Private_Anim_CopyData(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct)
{
	int iDiff = _iStartSrc - _iStartDst;
	int iEnd = _iStartDst + _nItems;
	mint ItemSizeSrc = ms_lTypeSize[m_Type] * _nDimSrc;
	mint ItemSizeDst = ms_lTypeSize[m_Type] * _nDimDst;
	for (int i = _iStartDst; i < iEnd; ++i)
	{
		uint8 *pSrcItem = (uint8 *)_pSrc + ItemSizeSrc * (i + iDiff);
		uint8 *pDstItem = (uint8 *)_pDst + ItemSizeDst * (i);
		if (ms_lTypeConstructorAnim[m_Type])
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDimDst);
		ms_lTypeConvert[m_Type][m_Type](pSrcItem, _nDimSrc, pDstItem, _nDimDst);
		if (_bDestruct && ms_lTypeDestructorAnim[m_Type])
			ms_lTypeDestructorAnim[m_Type](pSrcItem, _nDimSrc);
	}
}

void CRegistry_Dynamic::Private_Anim_CopyDataSrcTimed(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct)
{
	int iDiff = _iStartSrc - _iStartDst;
	int iEnd = _iStartDst + _nItems;
	mint ItemSizeDst = ms_lTypeSize[m_Type] * _nDimDst;
	mint ItemSizeSrc = ms_lTypeSize[m_Type] * _nDimSrc + sizeof(fp4);
	for (int i = _iStartDst; i < iEnd; ++i)
	{
		uint8 *pSrcItem = (uint8 *)_pSrc + ItemSizeSrc * (i + iDiff);
		uint8 *pDstItem = (uint8 *)_pDst + ItemSizeDst * (i);
		
		if (ms_lTypeConstructorAnim[m_Type])
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDimDst);
		ms_lTypeConvert[m_Type][m_Type](pSrcItem, _nDimSrc, pDstItem, _nDimDst);
		if (_bDestruct && ms_lTypeDestructorAnim[m_Type])
			ms_lTypeDestructorAnim[m_Type](pSrcItem, _nDimSrc);
	}
}

void CRegistry_Dynamic::Private_Anim_CopyDataDstTimed(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct)
{
	int iDiff = _iStartSrc - _iStartDst;
	int iEnd = _iStartDst + _nItems;
	mint ItemSizeDst = ms_lTypeSize[m_Type] * _nDimDst + sizeof(fp4);
	mint ItemSizeSrc = ms_lTypeSize[m_Type] * _nDimSrc;
	for (int i = _iStartDst; i < iEnd; ++i)
	{
		uint8 *pSrcItem = (uint8 *)_pSrc + ItemSizeSrc * (i + iDiff);
		uint8 *pDstItem = (uint8 *)_pDst + ItemSizeDst * (i);
		fp4 *pTimeDst = (fp4 *)(pDstItem + ItemSizeDst - sizeof(fp4));
		*pTimeDst = i;
		
		if (ms_lTypeConstructorAnim[m_Type])
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDimDst);
		ms_lTypeConvert[m_Type][m_Type](pSrcItem, _nDimSrc, pDstItem, _nDimDst);
		if (_bDestruct && ms_lTypeDestructorAnim[m_Type])
			ms_lTypeDestructorAnim[m_Type](pSrcItem, _nDimSrc);
	}
}

void CRegistry_Dynamic::Private_Anim_CopyDataTimed(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct)
{
	int iDiff = _iStartSrc - _iStartDst;
	int iEnd = _iStartDst + _nItems;
	mint ItemSizeDst = ms_lTypeSize[m_Type] * _nDimDst + sizeof(fp4);
	mint ItemSizeSrc = ms_lTypeSize[m_Type] * _nDimSrc + sizeof(fp4);
	for (int i = _iStartDst; i < iEnd; ++i)
	{
		uint8 *pSrcItem = (uint8 *)_pSrc + ItemSizeSrc * (i + iDiff);
		uint8 *pDstItem = (uint8 *)_pDst + ItemSizeDst * (i);
		fp4 *pTimeSrc = (fp4 *)(pSrcItem + ItemSizeSrc - sizeof(fp4));
		fp4 *pTimeDst = (fp4 *)(pDstItem + ItemSizeDst - sizeof(fp4));
		*pTimeDst = *pTimeSrc;
		
		if (ms_lTypeConstructorAnim[m_Type])
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDimDst);
		ms_lTypeConvert[m_Type][m_Type](pSrcItem, _nDimSrc, pDstItem, _nDimDst);
		if (_bDestruct && ms_lTypeDestructorAnim[m_Type])
			ms_lTypeDestructorAnim[m_Type](pSrcItem, _nDimSrc);
	}
}


void CRegistry_Dynamic::Private_Anim_CopyDataTyped(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct, int _Type)
{
	int iDiff = _iStartSrc - _iStartDst;
	int iEnd = _iStartDst + _nItems;
	mint ItemSizeDst = ms_lTypeSize[m_Type] * _nDimDst;
	mint ItemSizeSrc = ms_lTypeSize[_Type] * _nDimSrc;
	for (int i = _iStartDst; i < iEnd; ++i)
	{
		uint8 *pSrcItem = (uint8 *)_pSrc + ItemSizeSrc * (i + iDiff);
		uint8 *pDstItem = (uint8 *)_pDst + ItemSizeDst * (i);
		if (ms_lTypeConstructorAnim[m_Type])
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDimDst);
		ms_lTypeConvert[_Type][m_Type](pSrcItem, _nDimSrc, pDstItem, _nDimDst);
		if (_bDestruct && ms_lTypeDestructorAnim[_Type])
			ms_lTypeDestructorAnim[_Type](pSrcItem, _nDimSrc);
	}
}

void CRegistry_Dynamic::Private_Anim_CopyDataSrcTimedTyped(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct, int _Type)
{
	int iDiff = _iStartSrc - _iStartDst;
	int iEnd = _iStartDst + _nItems;
	mint ItemSizeDst = ms_lTypeSize[m_Type] * _nDimDst;
	mint ItemSizeSrc = ms_lTypeSize[_Type] * _nDimSrc + sizeof(fp4);
	for (int i = _iStartDst; i < iEnd; ++i)
	{
		uint8 *pSrcItem = (uint8 *)_pSrc + ItemSizeSrc * (i + iDiff);
		uint8 *pDstItem = (uint8 *)_pDst + ItemSizeDst * (i);
		
		if (ms_lTypeConstructorAnim[m_Type])
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDimDst);
		ms_lTypeConvert[_Type][m_Type](pSrcItem, _nDimSrc, pDstItem, _nDimDst);
		if (_bDestruct && ms_lTypeDestructorAnim[_Type])
			ms_lTypeDestructorAnim[_Type](pSrcItem, _nDimSrc);
	}
}

void CRegistry_Dynamic::Private_Anim_CopyDataDstTimedTyped(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct, int _Type)
{
	int iDiff = _iStartSrc - _iStartDst;
	int iEnd = _iStartDst + _nItems;
	mint ItemSizeDst = ms_lTypeSize[m_Type] * _nDimDst + sizeof(fp4);
	mint ItemSizeSrc = ms_lTypeSize[_Type] * _nDimSrc;
	for (int i = _iStartDst; i < iEnd; ++i)
	{
		uint8 *pSrcItem = (uint8 *)_pSrc + ItemSizeSrc * (i + iDiff);
		uint8 *pDstItem = (uint8 *)_pDst + ItemSizeDst * (i);
		fp4 *pTimeDst = (fp4 *)(pDstItem + ItemSizeDst - sizeof(fp4));
		*pTimeDst = i;
		
		if (ms_lTypeConstructorAnim[m_Type])
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDimDst);
		ms_lTypeConvert[_Type][m_Type](pSrcItem, _nDimSrc, pDstItem, _nDimDst);
		if (_bDestruct && ms_lTypeDestructorAnim[_Type])
			ms_lTypeDestructorAnim[_Type](pSrcItem, _nDimSrc);
	}
}

void CRegistry_Dynamic::Private_Anim_CopyDataTimedTyped(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct, int _Type)
{
	int iDiff = _iStartSrc - _iStartDst;
	int iEnd = _iStartDst + _nItems;
	mint ItemSizeDst = ms_lTypeSize[m_Type] * _nDimDst + sizeof(fp4);
	mint ItemSizeSrc = ms_lTypeSize[_Type] * _nDimSrc + sizeof(fp4);
	for (int i = _iStartDst; i < iEnd; ++i)
	{
		uint8 *pSrcItem = (uint8 *)_pSrc + ItemSizeSrc * (i + iDiff);
		uint8 *pDstItem = (uint8 *)_pDst + ItemSizeDst * (i);
		fp4 *pTimeSrc = (fp4 *)(pSrcItem + ItemSizeSrc - sizeof(fp4));
		fp4 *pTimeDst = (fp4 *)(pDstItem + ItemSizeDst - sizeof(fp4));
		*pTimeDst = *pTimeSrc;
		
		if (ms_lTypeConstructorAnim[m_Type])
			ms_lTypeConstructorAnim[m_Type](pDstItem, _nDimDst);
		ms_lTypeConvert[_Type][m_Type](pSrcItem, _nDimSrc, pDstItem, _nDimDst);
		if (_bDestruct && ms_lTypeDestructorAnim[_Type])
			ms_lTypeDestructorAnim[_Type](pSrcItem, _nDimSrc);
	}
}




void CRegistry_Dynamic::Private_Anim_Promote()
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
	{
		CAnimationSequence *pAnimationSequence = DNew(CAnimationSequence) CAnimationSequence;

		if (m_Type != REGISTRY_TYPE_VOID)
		{
			pAnimationSequence->m_pData = M_ALLOC(ms_lTypeSize[m_Type] * (m_Dimensions+1));
			pAnimationSequence->m_nKeyFrames = 1;

			if (ms_lTypeConstructorAnim[m_Type])
				ms_lTypeConstructorAnim[m_Type](pAnimationSequence->m_pData, (m_Dimensions+1));

			ms_lTypeConvert[m_Type][m_Type](Type_GetValue(), (m_Dimensions+1), pAnimationSequence->m_pData, (m_Dimensions+1));

			if (ms_lTypeDestructor[m_Type])
				ms_lTypeDestructor[m_Type](m_Value, (m_Dimensions+1));
		}

		new (m_Value) CAnimationData;
		m_InternalFlags |= ERegistryDynamicFlags_Animated;
		CAnimationData &AnimData = Type_GetAnimationData();
		AnimData.m_InternalAnimFlags = 0;
		AnimData.m_AnimFlags = 0;
		AnimData.m_AnimIPType = REGISTRY_ANIMIP_NONE;
		AnimData.m_Sequences.SetLen(1);
		AnimData.m_Sequences[0] = pAnimationSequence;
	}
}

void CRegistry_Dynamic::Private_Anim_Demote()
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
	{
		CAnimationData &AnimData = Type_GetAnimationData();
		if (m_Type != REGISTRY_TYPE_VOID && AnimData.m_Sequences.Len() > 0 && AnimData.m_Sequences[0]->m_nKeyFrames > 0)
		{
			void *pData = AnimData.m_Sequences[0]->m_pData;
			int nKeyFrames = AnimData.m_Sequences[0]->m_nKeyFrames;
			uint32 Flags = AnimData.m_InternalAnimFlags;
			AnimData.m_Sequences[0]->m_pData = NULL;
			Private_Anim_Destroy(AnimData);

			Type_CallConstructor(m_Type, m_Value, (m_Dimensions+1));
			ms_lTypeConvert[m_Type][m_Type](pData, (m_Dimensions+1), Type_GetValue(), (m_Dimensions+1));

			if (Flags & ERegistryDynamicAnimFlag_Timed)
				Private_Anim_DestroyDataTimed(pData, 0, nKeyFrames, m_Dimensions+1);
			else
				Private_Anim_DestroyData(pData, 0, nKeyFrames, m_Dimensions+1);
			
		}
		else
		{
			Private_Anim_Destroy(AnimData);
			m_Type = REGISTRY_TYPE_VOID;
		}

		m_InternalFlags &= ~ERegistryDynamicFlags_Animated;
	}
}

void CRegistry_Dynamic::Anim_ThisSetAnimated(bint _bEnable)
{
	if (_bEnable)
		Private_Anim_Promote();
	else
		Private_Anim_Demote();
}

void CRegistry_Dynamic::Private_Anim_CheckAutoDemote()
{
	if (m_InternalFlags & ERegistryDynamicFlags_Animated)
	{
		CAnimationData &AnimData = Type_GetAnimationData();
		if (AnimData.m_Sequences.Len() < 1 || (AnimData.m_Sequences.Len() == 1 && AnimData.m_Sequences[0]->m_nKeyFrames <= 1))
			Private_Anim_Demote();
	}
}

void CRegistry_Dynamic::Private_Anim_TimedPromote()
{
	CAnimationData &AnimData = Type_GetAnimationData();
	if (!(AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed))
	{
		int nSeq = AnimData.m_Sequences.Len();
		for (int i = 0; i < nSeq; ++i)
		{
			CAnimationSequence *pSeq = AnimData.m_Sequences[i];
			mint ItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1);
			mint NewItemSize = ItemSize + sizeof(fp4);
			mint NewSize = pSeq->m_nKeyFrames * NewItemSize;
			
			uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
			uint8 *pOldData = (uint8 *)pSeq->m_pData;
			
			int nKeys = pSeq->m_nKeyFrames;

			Private_Anim_CopyDataDstTimed(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), nKeys, true);

			MRTC_GetMemoryManager()->Free(pOldData);
			pSeq->m_pData = pNewData;
		}
		AnimData.m_InternalAnimFlags |= ERegistryDynamicAnimFlag_Timed;
	}
}

void CRegistry_Dynamic::Private_Anim_TimedDemote()
{
	CAnimationData &AnimData = Type_GetAnimationData();
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
	{
		int nSeq = AnimData.m_Sequences.Len();
		for (int i = 0; i < nSeq; ++i)
		{
			CAnimationSequence *pSeq = AnimData.m_Sequences[i];
//			mint ItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1) + sizeof(fp4);
			mint NewItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1);
			mint NewSize = pSeq->m_nKeyFrames * NewItemSize;
			
			uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
			uint8 *pOldData = (uint8 *)pSeq->m_pData;
			
			int nKeys = pSeq->m_nKeyFrames;

			Private_Anim_CopyDataSrcTimed(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), nKeys, true);

			MRTC_GetMemoryManager()->Free(pOldData);
			pSeq->m_pData = pNewData;
		}
		AnimData.m_InternalAnimFlags &= ~ERegistryDynamicAnimFlag_Timed;
	}
}


bint CRegistry_Dynamic::Anim_ThisGetAnimated() const
{
	return m_InternalFlags & ERegistryDynamicFlags_Animated;
}

void CRegistry_Dynamic::Anim_ThisSetDisableAutoDemote(bint _bEnable)
{
	m_InternalFlags = (m_InternalFlags & (~ERegistryDynamicFlags_DisableAnimAutoDemote)) | (_bEnable ? ERegistryDynamicFlags_DisableAnimAutoDemote : 0);
}

bint CRegistry_Dynamic::Anim_ThisGetDisableAutoDemote() const
{
	return m_InternalFlags & ERegistryDynamicFlags_DisableAnimAutoDemote;
}

void CRegistry_Dynamic::Anim_ThisSetEnableTimed(bint _bEnable)
{
	Private_Anim_CheckAnimEnabled();

	if (_bEnable)
		Private_Anim_TimedPromote();
	else
		Private_Anim_TimedDemote();
}

bint CRegistry_Dynamic::Anim_ThisGetEnableTimed() const
{
	Private_Anim_CheckAnimEnabled();

	const CAnimationData &AnimData = Type_GetAnimationData();
	return AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed;
}

void CRegistry_Dynamic::Anim_ThisSetFlags(uint32 _Flags)
{
	Private_Anim_CheckAnimEnabled();

	CAnimationData &AnimData = Type_GetAnimationData();
	AnimData.m_AnimFlags = _Flags;
}

uint32 CRegistry_Dynamic::Anim_ThisGetFlags() const
{
	Private_Anim_CheckAnimEnabled();
	const CAnimationData &AnimData = Type_GetAnimationData();
	return AnimData.m_AnimFlags;
}

void CRegistry_Dynamic::Anim_ThisSetInterpolate(uint32 _InterpolateType, const fp4 *_pParams, int _nParams)
{
	Private_Anim_CheckAnimEnabled();

	CAnimationData &AnimData = Type_GetAnimationData();
	AnimData.m_AnimIPType = _InterpolateType;
	AnimData.m_InterpolateData.SetLen(_nParams);
	memcpy(AnimData.m_InterpolateData.GetBasePtr(), _pParams, _nParams * sizeof(fp4));
}

uint32 CRegistry_Dynamic::Anim_ThisGetInterpolate(fp4 *_pParams, int &_nParams) const
{
	Private_Anim_CheckAnimEnabled();
	const CAnimationData &AnimData = Type_GetAnimationData();
	if (_pParams)
	{
		memcpy(_pParams, AnimData.m_InterpolateData.GetBasePtr(), Min(_nParams, AnimData.m_InterpolateData.Len()) * sizeof(fp4));
	}
	_nParams = AnimData.m_InterpolateData.Len();
	return AnimData.m_AnimIPType;
}

bint CRegistry_Dynamic::Anim_ThisIsValidSequenceKeyframe(int _iSeq, int _iKF) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return false;

	const CAnimationData &AnimData = Type_GetAnimationData();
	return AnimData.IsValidSequenceKeyframe(_iSeq, _iKF);
}

int CRegistry_Dynamic::Anim_ThisAddSeq()
{
	Private_Anim_Promote();

	CAnimationData &AnimData = Type_GetAnimationData();
	int iSeq = AnimData.m_Sequences.Len();
	AnimData.m_Sequences.SetLen(iSeq + 1);
	AnimData.m_Sequences[iSeq] = DNew(CAnimationSequence) CAnimationSequence();
	return iSeq;
}

int CRegistry_Dynamic::Anim_ThisInsertSeq(int _iSeqAfter)
{
	Private_Anim_Promote();

	CAnimationData &AnimData = Type_GetAnimationData();
	int Len = AnimData.m_Sequences.Len();
	int iSeq = Min(Max(0, _iSeqAfter), Len);
	++Len;
	AnimData.m_Sequences.SetLen(Len);
	if (iSeq + 1 < Len)
		memmove(AnimData.m_Sequences.GetBasePtr() + iSeq + 1, AnimData.m_Sequences.GetBasePtr() + iSeq, Len - (iSeq+1));

	AnimData.m_Sequences[iSeq] = DNew(CAnimationSequence) CAnimationSequence();
	return iSeq;
}

void CRegistry_Dynamic::Private_Anim_DestroySequence(CAnimationSequence *_pSeq)
{
	if (!ms_lTypeDestructorAnim[m_Type])
		return;
//	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = _pSeq;

	Private_Anim_DestroyData(pSeq->m_pData, 0, pSeq->m_nKeyFrames, (m_Dimensions+1));
}

void CRegistry_Dynamic::Anim_ThisDeleteSeq(int _iSeq)
{
	Private_Anim_CheckValidSequence(_iSeq);

	CAnimationData &AnimData = Type_GetAnimationData();
	int Len = AnimData.m_Sequences.Len();
	if ((!(m_InternalFlags & ERegistryDynamicFlags_DisableAnimAutoDemote)) && (Len <= 2))
	{
		int iSeq = _iSeq;
		if (iSeq == 0 && Len > 1)
			iSeq = 1;
		else
			iSeq = 0;

		if (AnimData.m_Sequences[iSeq]->m_nKeyFrames == 1)
		{
			Private_Anim_Demote();
			return;
		}
	}

	--Len;
	Private_Anim_DestroySequence(AnimData.m_Sequences[_iSeq]);
	delete AnimData.m_Sequences[_iSeq];
	if ((Len - _iSeq) > 0)
		memmove(AnimData.m_Sequences.GetBasePtr() + _iSeq, AnimData.m_Sequences.GetBasePtr() + _iSeq + 1, Len - _iSeq);
	AnimData.m_Sequences.SetLen(Len);

}

void CRegistry_Dynamic::Anim_ThisSetNumSeq(int _nSeq)
{
	if (_nSeq == 0)
	{
		Private_Anim_Demote();
		return;
	}
	else
	{
		Private_Anim_Promote();
		CAnimationData &AnimData = Type_GetAnimationData();
		int OldLen = AnimData.m_Sequences.Len();
		if (_nSeq < OldLen)
		{
			for (int i = _nSeq; i < OldLen; ++i)
			{
				delete AnimData.m_Sequences[i];
			}
		}
		AnimData.m_Sequences.SetLen(_nSeq);

		for (int i = OldLen; i < _nSeq; ++i)
		{
			AnimData.m_Sequences[i] = DNew(CAnimationSequence) CAnimationSequence();
		}
	}
}

int CRegistry_Dynamic::Anim_ThisGetNumSeq() const
{
	if (!Private_Anim_IsAnimated())
		return 0;

	const CAnimationData &AnimData = Type_GetAnimationData();
	return AnimData.m_Sequences.Len();
}

fp4 CRegistry_Dynamic::Anim_ThisGetSeqLoopStart(int _iSeq) const
{
	Private_Anim_CheckValidSequence(_iSeq);

	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	return pSeq->m_LoopStart;
}


fp4 CRegistry_Dynamic::Anim_ThisGetSeqLength(int _iSeq) const
{
	Private_Anim_CheckValidSequence(_iSeq);

	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
		return Private_Anim_GetKFTimeTimed(pSeq, pSeq->m_nKeyFrames - 1);
	else
		return pSeq->m_nKeyFrames;
}

fp4 CRegistry_Dynamic::Anim_ThisGetSeqLoopEnd(int _iSeq) const
{
	Private_Anim_CheckValidSequence(_iSeq);

	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	return pSeq->m_LoopEnd;
}

void CRegistry_Dynamic::Anim_ThisSetSeqLoopStart(int _iSeq, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);

	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	pSeq->m_LoopStart = _Time;
}

void CRegistry_Dynamic::Anim_ThisSetSeqLoopEnd(int _iSeq, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);

	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	pSeq->m_LoopEnd = _Time;
}


fp4 CRegistry_Dynamic::Private_Anim_GetKFTimeTimed(const CAnimationSequence *_pSeq, int _iKeyFrame) const
{
	M_ASSERT(Type_GetAnimationData().m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed && _iKeyFrame >= 0 && _iKeyFrame < _pSeq->m_nKeyFrames, "");
	return *((fp4 *)(((uint8 *)_pSeq->m_pData) + (ms_lTypeSize[m_Type] * (m_Dimensions+1) + sizeof(fp4)) * _iKeyFrame + ms_lTypeSize[m_Type] * (m_Dimensions+1)));
}

fp4 CRegistry_Dynamic::Private_Anim_GetKFTime(const CAnimationSequence *_pSeq, int _iKeyFrame) const
{
	M_ASSERT(_iKeyFrame >= 0 && _iKeyFrame < _pSeq->m_nKeyFrames, "");
	if (Type_GetAnimationData().m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
		return *((fp4 *)(((uint8 *)_pSeq->m_pData) + (ms_lTypeSize[m_Type] * (m_Dimensions+1) + sizeof(fp4)) * _iKeyFrame + ms_lTypeSize[m_Type] * (m_Dimensions+1)));
	else
		return _iKeyFrame;
}

const void *CRegistry_Dynamic::Private_Anim_GetKFData(const CAnimationSequence *_pSeq, int _iKeyFrame) const
{
	const CAnimationData &AnimData = Type_GetAnimationData();
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
		return (((uint8 *)_pSeq->m_pData) + (ms_lTypeSize[m_Type] * (m_Dimensions+1) + sizeof(fp4)) * _iKeyFrame);
	else
		return (((uint8 *)_pSeq->m_pData) + (ms_lTypeSize[m_Type] * (m_Dimensions+1)) * _iKeyFrame);
}

void *CRegistry_Dynamic::Private_Anim_GetKFData(CAnimationSequence *_pSeq, int _iKeyFrame)
{
	CAnimationData &AnimData = Type_GetAnimationData();
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
		return (((uint8 *)_pSeq->m_pData) + (ms_lTypeSize[m_Type] * (m_Dimensions+1) + sizeof(fp4)) * _iKeyFrame);
	else
		return (((uint8 *)_pSeq->m_pData) + (ms_lTypeSize[m_Type] * (m_Dimensions+1)) * _iKeyFrame);
}

fp4 CRegistry_Dynamic::Private_Anim_GetSequenceLength(const CAnimationSequence *_pSeq) const
{
	if (_pSeq->m_nKeyFrames < 1)
		return 0;

	const CAnimationData &AnimData = Type_GetAnimationData();
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed && _pSeq->m_nKeyFrames)
		return Private_Anim_GetKFTimeTimed(_pSeq, _pSeq->m_nKeyFrames - 1);
	else
		return _pSeq->m_nKeyFrames;
}

int CRegistry_Dynamic::Private_Anim_FindKeyByTime(const CAnimationSequence *_pSeq, fp4 _Time) const
{
	const CAnimationData &AnimData = Type_GetAnimationData();
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
	{
		int nKF = _pSeq->m_nKeyFrames;
		int iIndexSearch = 0;

		fp4 Hash = _Time;

		int Low = 0;
		int High = nKF;
		while(Low < High)
		{
			iIndexSearch = (Low + High) >> 1;

			fp4 CurrHash = Private_Anim_GetKFTimeTimed(_pSeq, iIndexSearch);

			if (CurrHash < Hash)
				Low = iIndexSearch + 1;
			else if (CurrHash > Hash)
				High = iIndexSearch;
			else
				break;
		}

		if (iIndexSearch > High)
			iIndexSearch = High;
		if (iIndexSearch < Low)
			iIndexSearch = Low;
		if (iIndexSearch >= nKF)
			iIndexSearch = nKF-1;
		if (iIndexSearch < 0)
			iIndexSearch = 0;

		while (iIndexSearch < (nKF-1) && Private_Anim_GetKFTimeTimed(_pSeq, iIndexSearch) < Hash)
			++iIndexSearch;
		while (iIndexSearch > 0 && Private_Anim_GetKFTimeTimed(_pSeq, iIndexSearch) > Hash)
			--iIndexSearch;
		return iIndexSearch;
	}
	else
	{
        return Max(Min((int)_Time,_pSeq->m_nKeyFrames-1), 0);
	}
}

void CRegistry_Dynamic::Private_Anim_ThisGetKF(const CAnimationSequence *_pSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost, int _Flags) const
{
	if (_pTimeDeltas)
	{
		const CAnimationSequence *pSeq = _pSeq;
		fp4 SeqLen = Private_Anim_GetSequenceLength(_pSeq);
		fp4 LoopStart = (_pSeq->m_LoopStart < 0 ? 0 : _pSeq->m_LoopStart);
		fp4 LoopEnd = (_pSeq->m_LoopEnd < 0 ? SeqLen : _pSeq->m_LoopEnd);
		M_ASSERT(LoopEnd > LoopStart, "");
		if (LoopEnd <= LoopStart)
			return;

		uint32 DeltasCalc[256];
		int nTimeDeltas = _nPre+_nPost;
		
		if (nTimeDeltas > 128)
		{
			fp4 *pDeltas = new fp4[nTimeDeltas * 2];
			
			Private_Anim_ThisGetKF(_pSeq, _Time, _Fraction, _pKeys, pDeltas, _nPre, _nPost, _Flags);
			for (int i = 0; i < nTimeDeltas; ++i)
			{
				_pTimeDeltas[i] = CRegistry_Shared_Helpers::Private_Anim_GetKFDelta(this, pSeq, (uint32)pDeltas[i*2], (uint32)pDeltas[i*2+1], SeqLen, LoopEnd, LoopStart);
			}

			delete [] pDeltas;
		}
		else
		{
			Private_Anim_ThisGetKF(_pSeq, _Time, _Fraction, _pKeys, DeltasCalc, _nPre, _nPost, _Flags);
			for (int i = 0; i < nTimeDeltas; ++i)
			{
				_pTimeDeltas[i] = CRegistry_Shared_Helpers::Private_Anim_GetKFDelta(this, pSeq, DeltasCalc[i*2], DeltasCalc[i*2+1], SeqLen, LoopEnd, LoopStart);
			}
		}
	}
	else
	{
		Private_Anim_ThisGetKF(_pSeq, _Time, _Fraction, _pKeys, (uint32 *)NULL, _nPre, _nPost, _Flags);
	}
}

void CRegistry_Dynamic::Private_Anim_ThisGetKF(const CAnimationSequence *_pSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, uint32 *_pDeltasCalc, int _nPre, int _nPost, int _Flags) const
{
//	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = _pSeq;

	fp4 SeqLen = Private_Anim_GetSequenceLength(_pSeq);
	fp4 Time = 0;
	int nLoops = 0;
	fp4 LoopStart;
	fp4 LoopEnd = 0;
	fp4 LoopLength;
	fp4 LoopTime = -1;
	if (_Flags & REGISTRY_ANIMFLAGS_LOOP)
	{
		LoopStart = (_pSeq->m_LoopStart < 0 ? 0 : _pSeq->m_LoopStart);
		LoopEnd = (_pSeq->m_LoopEnd < 0 ? SeqLen : _pSeq->m_LoopEnd);
		M_ASSERT(LoopEnd > LoopStart, "");
		if (LoopEnd <= LoopStart)
			return;
		LoopLength = LoopEnd - LoopStart;
		if (_Time.Compare(SeqLen) > 0)
		{

			CMTime WrapTime;
			if (_Flags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
			{
				WrapTime = _Time - (CMTime::CreateFromSeconds(SeqLen + (SeqLen - LoopEnd)));
			}
			else
				WrapTime = _Time - (CMTime::CreateFromSeconds(SeqLen));

			LoopTime = WrapTime.GetTimeModulus(LoopLength);
			nLoops = WrapTime.GetNumModulus(LoopLength) + 1;

			if (_Flags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
			{
				if (WrapTime.Compare(0) < 0)
				{
					Time = SeqLen - WrapTime.GetTime();
				}
				else
				{
					if (nLoops & 1)
						Time = LoopEnd - LoopTime;
					else
						Time = LoopStart + LoopTime;
				}
			}
			else
				Time = LoopStart + LoopTime;
		}
		else
			Time = _Time.GetTime();
	}
	else if (_Flags & REGISTRY_ANIMFLAGS_CONTIGUOUS)
	{
		LoopStart = (_pSeq->m_LoopStart < 0 ? 0 : _pSeq->m_LoopStart);
		LoopEnd = (_pSeq->m_LoopEnd < 0 ? SeqLen : _pSeq->m_LoopEnd);
		M_ASSERT(LoopEnd > LoopStart, "");
		if (LoopEnd <= LoopStart)
			return;
		LoopLength = LoopEnd - LoopStart;
		if (_Time.Compare(SeqLen) > 0)
		{
			CMTime WrapTime;
			WrapTime = _Time - (CMTime::CreateFromSeconds(SeqLen));
			LoopTime = WrapTime.GetTimeModulus(LoopLength);
			nLoops = WrapTime.GetNumModulus(LoopLength) + 1;
			Time = LoopStart + LoopTime;
		}
		else
			Time = _Time.GetTime();
	}
	else
	{
		if (_Time.Compare(SeqLen) > 0)
			Time = SeqLen;
		else
			Time = _Time.GetTime();
	}

	fp4 LowTime = Private_Anim_GetKFTime(pSeq, 0);
	if (Time < LowTime)
		Time = LowTime;

	int nKF = pSeq->m_nKeyFrames;

	int iStart = Private_Anim_FindKeyByTime(pSeq, Time);
	_pKeys[_nPre] = iStart;

	if (_Flags & REGISTRY_ANIMFLAGS_LOOP)
	{
		int iLoopStart = Private_Anim_FindKeyByTime(pSeq, LoopStart);
		int iLoopEnd = Private_Anim_FindKeyByTime(pSeq, LoopEnd);
		while (iLoopStart < (nKF-1) && Private_Anim_GetKFTime(_pSeq, iLoopStart) < LoopStart)
			++iLoopStart;

		_Fraction = 0;

		int iLoopStartIndex = iLoopStart;

		int iCur = iStart;
		int iLoops = nLoops;
		// Walk backwards
		int iPre = _nPre;
		if (_Flags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
		{
			if (_Flags & REGISTRY_ANIMFLAGS_CONTIGUOUS && !iLoops)
			{
				iLoops = 2;
			}
			if (iLoops && iLoops & 1)
			{
				++iStart;
				if (iStart > iLoopEnd)
					iStart = iLoopEnd;
				_pKeys[_nPre] = iCur = iStart;
			}
			else if (!iLoops && LoopTime >= 0)
			{
				++iStart;
				if (iStart >= nKF)
					iStart = nKF-1;
				_pKeys[_nPre] = iCur = iStart;
			}
//			LoopStartTime = Private_Anim_GetKFTime(pSeq, iLoopStart) - LoopStart;
			// Walk backwards through loops
			while (iLoops && iPre)
			{
				--iPre;
				if (iLoops & 1)
				{
					if (iCur == iLoopEnd)
					{
						--iLoops;
						if (iLoops)
						{
							_pDeltasCalc[iPre*2] = iCur;
							iCur = iCur - 1;
							if (iCur < iLoopStart)
								iCur = iLoopStart;
							_pDeltasCalc[iPre*2+1] = iCur;
						}
						else
						{
							++iPre;
							break;
						}

					}
					else
					{
						_pDeltasCalc[iPre*2+1] = iCur;
						iCur = iCur + 1;
						if (iCur > iLoopEnd)
							iCur = iLoopEnd;
						_pDeltasCalc[iPre*2] = iCur;
					}
				}
				else
				{
					if (iCur == iLoopStart)
					{
						--iLoops;
						_pDeltasCalc[iPre*2+1] = iCur;
						iCur = iCur + 1;
						if (iCur > iLoopEnd)
							iCur = iLoopEnd;
						_pDeltasCalc[iPre*2] = iCur;
					}
					else
					{
						_pDeltasCalc[iPre*2] = iCur;
						iCur = iCur - 1;
						if (iCur < iLoopStart)
							iCur = iLoopStart;
						_pDeltasCalc[iPre*2+1] = iCur;
					}
				}
				_pKeys[iPre] = iCur;

			}
			if (iPre)
			{
				// Walk towards end
				if (LoopTime >= 0)
				{
					while (iPre && iCur < (nKF-1))
					{
						--iPre;
						_pDeltasCalc[iPre*2+1] = iCur;
						iCur = iCur + 1;
						_pDeltasCalc[iPre*2] = iCur;
						_pKeys[iPre] = iCur;
					}
				}

				// Walk towards beginning
				while (iPre)
				{
					--iPre;
					_pDeltasCalc[iPre*2] = iCur;
					iCur = Max(iCur - 1, 0);
					_pDeltasCalc[iPre*2+1] = iCur;
					_pKeys[iPre] = iCur;
				}
			}
		}
		else
		{
			if (AlmostEqual(Private_Anim_GetKFTime(pSeq, iLoopStartIndex), LoopStart, 0.0001f))
			{
				++iLoopStartIndex;
				if (iLoopStartIndex > iLoopEnd)
					iLoopStartIndex = iLoopEnd;
			}
			if (!iLoops && _Flags & REGISTRY_ANIMFLAGS_CONTIGUOUS)
				iLoops = 1;
			if (iLoops && iStart < iLoopStartIndex)
			{	
				if (iLoops == 1)
					iCur = iStart = nKF-1;
				else
					iCur = iStart = iLoopEnd;
				_pKeys[_nPre] = iStart;
			}

			// Walk backwards through loops
			while (iLoops && iPre)
			{
				--iPre;
				if (iCur == iLoopStartIndex)
				{
					if (iLoops == 1)
					{
						iCur = nKF-1;
						_pDeltasCalc[iPre*2] = iCur | (EGetKFFlags_VSeqEnd << EGetKFFlags_TypeShift);
						_pDeltasCalc[iPre*2+1] = iLoopStartIndex;
					}
					else
					{
						iCur = iLoopEnd;
						_pDeltasCalc[iPre*2] = iLoopEnd | (EGetKFFlags_VLoopEnd << EGetKFFlags_TypeShift);
						_pDeltasCalc[iPre*2+1] = iLoopStartIndex;
					}
					--iLoops;
				}
				else
				{
			//		M_ASSERT(iCur > iLoopStartIndex, "");
					_pDeltasCalc[iPre*2] = iCur;
					iCur = iCur - 1;
					_pDeltasCalc[iPre*2+1] = iCur;
				}
				_pKeys[iPre] = iCur;
			}
			if (iPre)
			{
				// Walk towards beginning
				{
					while (iPre)
					{
						--iPre;
						_pDeltasCalc[iPre*2] = iCur;
						iCur = Max(iCur - 1, 0);
						_pDeltasCalc[iPre*2+1] = iCur;
						_pKeys[iPre] = iCur;
					}
				}
			}
		}

        // Calc fraction
		iCur = iStart;
		iLoops = nLoops;
		// Walk forward
		int nPost = _nPost;
		int iPost = _nPre+1;
		if (_Flags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
		{
			if (!iLoops)
			{
				// Walk to end of sequence
				while (nPost)
				{
					if (iCur == (nKF - 1))
					{
						++iLoops;
						_pDeltasCalc[iPost*2-2] = iCur;
						iCur = iCur - 1;
						if (iCur < iLoopStart)
							iCur = iLoopStart;
						_pDeltasCalc[iPost*2-1] = iCur;
						_pKeys[iPost++] = iCur;
						--nPost;
						break;
					}
					_pDeltasCalc[iPost*2-1] = iCur;
					iCur = iCur + 1;
					_pDeltasCalc[iPost*2-2] = iCur;
					_pKeys[iPost++] = iCur;
					--nPost;
				}

				// Walk backwards to loop end
				while (nPost && iCur > iLoopEnd)
				{
					_pDeltasCalc[iPost*2-2] = iCur;
					iCur = iCur - 1;
					_pDeltasCalc[iPost*2-1] = iCur;
					_pKeys[iPost++] = iCur;
					--nPost;
				}
			}

			// Walk through loop
			while (nPost)
			{
				if (iLoops & 1)
				{
					if (iCur <= iLoopStart)
					{
						++iLoops;
						_pDeltasCalc[iPost*2-1] = iCur;
						iCur = iCur + 1;
						if (iCur >= nKF)
							iCur = nKF-1;
						_pDeltasCalc[iPost*2-2] = iCur;
					}
					else
					{
						_pDeltasCalc[iPost*2-2] = iCur;
						iCur = iCur - 1;
						_pDeltasCalc[iPost*2-1] = iCur;
					}
					_pKeys[iPost++] = iCur;
					--nPost;
				}
				else
				{
					if (iCur >= iLoopEnd)
					{
						++iLoops;
						_pDeltasCalc[iPost*2-2] = iCur;
						iCur = iCur - 1;
						if (iCur < 0)
							iCur = 0;
						_pDeltasCalc[iPost*2-1] = iCur;
					}
					else
					{
						_pDeltasCalc[iPost*2-1] = iCur;
						iCur = iCur + 1;
						_pDeltasCalc[iPost*2-2] = iCur;
					}
					_pKeys[iPost++] = iCur;
					--nPost;
				}
			}
		}
		else
		{
			if (!iLoops)
			{
				// Walk to end of sequence
				while (nPost)
				{
					if (iCur == (nKF - 1))
					{
						++iLoops;

						_pDeltasCalc[iPost*2-2] = iCur | (EGetKFFlags_VSeqEnd << EGetKFFlags_TypeShift);
						_pDeltasCalc[iPost*2-1] = iLoopStartIndex;

						iCur = iLoopStartIndex;
						_pKeys[iPost++] = iCur;
						--nPost;
						break;
					}
					_pDeltasCalc[iPost*2-1] = iCur;
					iCur = iCur + 1;
					_pDeltasCalc[iPost*2-2] = iCur;
					_pKeys[iPost++] = iCur;
					--nPost;
				}
			}

			// Walk through loop
			while (nPost)
			{
				if (iCur >= iLoopEnd)
				{
					if (iCur == iLoopEnd)
					{
						_pDeltasCalc[iPost*2-2] = iCur | (EGetKFFlags_VLoopEnd << EGetKFFlags_TypeShift);
						_pDeltasCalc[iPost*2-1] = iLoopStartIndex;

					}
					else
					{
						_pDeltasCalc[iPost*2-2] = iCur | (EGetKFFlags_VSeqEnd << EGetKFFlags_TypeShift);
						_pDeltasCalc[iPost*2-1] = iLoopStartIndex;
					}
					iCur = iLoopStartIndex;
					++iLoops;
				}
				else
				{
					int iLast = iCur;
					iCur = iCur + 1;
					_pDeltasCalc[iPost*2-2] = iCur;
					_pDeltasCalc[iPost*2-1] = iLast;
				}
				_pKeys[iPost++] = iCur;
				--nPost;
			}
		}

		fp4 LowTime = Private_Anim_GetKFTime(pSeq, iStart);
		if (_nPost)
		{
			fp4 Duration = CRegistry_Shared_Helpers::Private_Anim_GetKFDelta(this, pSeq, _pDeltasCalc[_nPre*2], _pDeltasCalc[_nPre*2+1], SeqLen, LoopEnd, LoopStart);

			if (_Flags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
			{
				if (Duration > 0)
				{
					fp4 Time0 = Private_Anim_GetKFTime(pSeq, _pKeys[_nPre]);
					fp4 Time1 = Private_Anim_GetKFTime(pSeq, _pKeys[_nPre+1]);
					if (Time1 > Time0)
					{
						_Fraction = (Time - Time0) / Duration;
					}
					else
					{
						_Fraction = (Time0 - Time) / Duration;
					}
				}
				else
					_Fraction = 0;
			}
			else
			{
				if (Duration > 0)
				{
					if (LowTime > Time)
					{
						if (iStart == iLoopEnd)
							_Fraction = ((Time - LoopStart) - (LowTime - LoopEnd)) / Duration;
						else
							_Fraction = ((Time - LoopStart) - (LowTime - SeqLen)) / Duration;
					}
					else
						_Fraction = (Time - LowTime) / Duration;
				}
				else
					_Fraction = 0;
			}
		}
		else
		{
			fp4 HighTime = Private_Anim_GetKFTime(pSeq, Min(iStart + 1, nKF - 1));
			Time = Min(Max(LowTime, Time), HighTime);
			if (HighTime != LowTime)
				_Fraction = (Time - LowTime) / (HighTime - LowTime);
		}
	}
	else if (_Flags & REGISTRY_ANIMFLAGS_CONTIGUOUS)
	{
		_Fraction = 0;
		int iCur = iStart;
		if (iCur == 0)
			_pKeys[_nPre] = iStart = iCur = nKF - 1;
		// Walk backwards
		int iPre = _nPre;
		while (iPre)
		{
			--iPre;
			iCur = iCur - 1;
			if (iCur <= 0)
			{
				_pDeltasCalc[iPre*2] = (iCur+1) | (EGetKFFlags_VAssign << EGetKFFlags_TypeShift);
				_pDeltasCalc[iPre*2+1] = 0;
				iCur = nKF - 1;
			}
			else
			{
				_pDeltasCalc[iPre*2] = iCur+1;
				_pDeltasCalc[iPre*2+1] = iCur;
			}
			_pKeys[iPre] = iCur;
		}
		iCur = iStart;

		// Walk forward
		for (int i = 0; i < _nPost; ++i)
		{
			iCur = iCur + 1;
			if (iCur >= nKF)
			{
				iCur = 1;
				if (iCur >= nKF)
					iCur = 0;

				_pDeltasCalc[(_nPre+i)*2] = (iCur) | (EGetKFFlags_VAssign << EGetKFFlags_TypeShift);
				_pDeltasCalc[(_nPre+i)*2+1] = 0;
			}
			else
			{
				_pDeltasCalc[(_nPre+i)*2] = iCur;
				_pDeltasCalc[(_nPre+i)*2+1] = iCur-1;
			}
			_pKeys[_nPre+i+1] = iCur;
		}

		fp4 LowTime = Private_Anim_GetKFTime(pSeq, iStart);
		if (_nPost)
		{
			fp4 Duration = CRegistry_Shared_Helpers::Private_Anim_GetKFDelta(this, pSeq, _pDeltasCalc[_nPre*2], _pDeltasCalc[_nPre*2+1], SeqLen, LoopEnd, LoopStart);
			if (Duration > 0)
			{
				if (LowTime > Time)
				{
					_Fraction = (Time - (LowTime - SeqLen)) / Duration;
				}
				else
					_Fraction = (Time - LowTime) / Duration;
			}
			else
				_Fraction = 0;
		}
		else
		{
			fp4 HighTime = Private_Anim_GetKFTime(pSeq, Min(iStart + 1, nKF - 1));
			Time = Min(Max(LowTime, Time), HighTime);
			if (HighTime != LowTime)
				_Fraction = (Time - LowTime) / (HighTime - LowTime);
		}
	}
	else
	{
		_Fraction = 0;
		int iCur = iStart;
		// Walk backwards
		int iPre = _nPre;
		while (iPre)
		{
			--iPre;
			iCur = Max(iCur - 1, 0);
			_pKeys[iPre] = iCur;
		}
		iCur = iStart;

		fp4 LowTime = Private_Anim_GetKFTime(pSeq, iStart);
		fp4 HighTime = Private_Anim_GetKFTime(pSeq, Min(iStart + 1, nKF - 1));
		Time = Min(Max(LowTime, Time), HighTime);
		if (HighTime != LowTime)
			_Fraction = (Time - LowTime) / (HighTime - LowTime);

		// Walk forward
		for (int i = 0; i < _nPost; ++i)
		{
			iCur = Min(iCur + 1, nKF - 1);
			_pKeys[_nPre+i+1] = iCur;
		}
		
		if (_pDeltasCalc)
		{
			int nDeltas = _nPre+_nPost;
			for (int i = 0; i < nDeltas; ++i)
			{
				_pDeltasCalc[i*2] = _pKeys[i+1];
				_pDeltasCalc[i*2+1] = _pKeys[i];
			}
		}
	}
}

fp4 CRegistry_Dynamic::Private_Anim_GetWrappedTime(const CAnimationSequence *_pSeq, const CMTime &_Time, uint32 _Flags) const
{
	fp4 SeqLen = Private_Anim_GetSequenceLength(_pSeq);

	if (_Time.Compare(SeqLen) > 0)
	{
		if (_Flags & REGISTRY_ANIMFLAGS_LOOP)
		{
			CMTime WrapTime = _Time - CMTime::CreateFromSeconds(SeqLen);
			
			fp4 LoopStart = (_pSeq->m_LoopStart < 0 ? 0 : _pSeq->m_LoopStart);
			fp4 LoopEnd = (_pSeq->m_LoopEnd < 0 ? SeqLen : _pSeq->m_LoopEnd);
			M_ASSERT(LoopEnd > LoopStart, "");

			fp4 LoopLength = LoopEnd - LoopStart;
			fp4 LoopTime = WrapTime.GetTimeModulus(LoopLength);

			if (_Flags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
			{
				int nMod = WrapTime.GetNumModulus(LoopLength);
				if (nMod & 1)
					return LoopStart + LoopTime;
				else
					return LoopEnd - LoopTime;
			}
			else
				return LoopStart + LoopTime;
		}
		else
			return SeqLen;
	}
	else
		return _Time.GetTime();

}

void CRegistry_Dynamic::Anim_ThisGetKF(int _iSeq, fp4 _Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const
{
	Private_Anim_CheckValidSequence(_iSeq);
	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	return Private_Anim_ThisGetKF(pSeq, CMTime::CreateFromSeconds(_Time), _Fraction, _pKeys, _pTimeDeltas, _nPre, _nPost, AnimData.m_AnimFlags);
}

void CRegistry_Dynamic::Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const
{
	Private_Anim_CheckValidSequence(_iSeq);
	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
//	fp4 WrappedTime = Private_Anim_GetWrappedTime(pSeq, _Time, AnimData.m_AnimFlags);
	return Private_Anim_ThisGetKF(pSeq, _Time, _Fraction, _pKeys, _pTimeDeltas, _nPre, _nPost, AnimData.m_AnimFlags);
}

void CRegistry_Dynamic::Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, uint32 *_pDeltasCalc, int _nPre, int _nPost) const
{
	Private_Anim_CheckValidSequence(_iSeq);
	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
//	fp4 WrappedTime = Private_Anim_GetWrappedTime(pSeq, _Time, AnimData.m_AnimFlags);
	return Private_Anim_ThisGetKF(pSeq, _Time, _Fraction, _pKeys, _pDeltasCalc, _nPre, _nPost, AnimData.m_AnimFlags);
}

void CRegistry_Dynamic::Anim_ThisSetNumKF(int _iSeq, int _nKF)
{
	Private_Anim_CheckValidSequence(_iSeq);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
	{
		mint ItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1);
		mint NewItemSize = ItemSize + sizeof(fp4);
		mint NewSize = _nKF * NewItemSize;
		
		uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
		uint8 *pOldData = (uint8 *)pSeq->m_pData;
		
		int nKeys = Min(pSeq->m_nKeyFrames, _nKF);

//		fp4 LastTime = 0;

		Private_Anim_CopyDataTimed(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), nKeys, true);
		Private_Anim_DestroyDataTimed(pOldData, nKeys, pSeq->m_nKeyFrames - nKeys, (m_Dimensions+1));
		MRTC_GetMemoryManager()->Free(pOldData);
		Private_Anim_ConstructDataTimed(pNewData, nKeys, _nKF - nKeys, (m_Dimensions+1));

		pSeq->m_pData = pNewData;
		pSeq->m_nKeyFrames = _nKF;
	}
	else
	{
		mint ItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1);
		mint NewItemSize = ItemSize;
		mint NewSize = _nKF * NewItemSize;
		
		uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
		uint8 *pOldData = (uint8 *)pSeq->m_pData;
		
		int nKeys = Min(pSeq->m_nKeyFrames, _nKF);

		Private_Anim_CopyData(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), nKeys, true);
		Private_Anim_DestroyData(pOldData, nKeys, pSeq->m_nKeyFrames - nKeys, (m_Dimensions+1));
		MRTC_GetMemoryManager()->Free(pOldData);
		Private_Anim_ConstructData(pNewData, nKeys, _nKF - nKeys, (m_Dimensions+1));

		pSeq->m_pData = pNewData;
		pSeq->m_nKeyFrames = _nKF;
	}
}

int CRegistry_Dynamic::Anim_ThisGetNumKF(int _iSeq) const
{
	Private_Anim_CheckValidSequence(_iSeq);
	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	return pSeq->m_nKeyFrames;
}


void CRegistry_Dynamic::Anim_ThisDeleteKF(int _iSeq, int _iKF)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	int nNew = pSeq->m_nKeyFrames - 1;
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
	{
		mint ItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1);
		mint NewItemSize = ItemSize + sizeof(fp4);
		mint NewSize = nNew * NewItemSize;
		
		uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
		uint8 *pOldData = (uint8 *)pSeq->m_pData;
		
//		fp4 LastTime = 0;

		Private_Anim_CopyDataTimed(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), _iKF, true);
		Private_Anim_DestroyDataTimed(pOldData, _iKF, 1, (m_Dimensions+1));
		Private_Anim_CopyDataTimed(pNewData, pOldData, _iKF, _iKF + 1, (m_Dimensions+1), (m_Dimensions+1), nNew - _iKF, true);

		MRTC_GetMemoryManager()->Free(pOldData);

		pSeq->m_pData = pNewData;
		pSeq->m_nKeyFrames = nNew;
	}
	else
	{
		mint ItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1);
		mint NewItemSize = ItemSize;
		mint NewSize = nNew * NewItemSize;
		
		uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
		uint8 *pOldData = (uint8 *)pSeq->m_pData;
		
//		fp4 LastTime = 0;

		Private_Anim_CopyData(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), _iKF, true);
		Private_Anim_DestroyData(pOldData, _iKF, 1, (m_Dimensions+1));
		Private_Anim_CopyData(pNewData, pOldData, _iKF, _iKF + 1, (m_Dimensions+1), (m_Dimensions+1), nNew - _iKF, true);

		MRTC_GetMemoryManager()->Free(pOldData);

		pSeq->m_pData = pNewData;
		pSeq->m_nKeyFrames = nNew;
	}
}

int CRegistry_Dynamic::Anim_ThisSetKFTime(int _iSeq, int _iKF, fp4 _Time) 
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	if (!(AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed))
		Error_static(M_FUNCTION, "The registry key isn't timed");

	if (Private_Anim_GetKFTimeTimed(pSeq, _iKF) == _Time)
		return _iKF;

	int iNewPos = Private_Anim_GetKFInsertPos(pSeq, _Time);

	Private_Anim_SetKFTime(pSeq->m_pData, _iKF, _Time);

	if (iNewPos == _iKF)
	{
		return _iKF;
	}

	int nNew = pSeq->m_nKeyFrames;
	mint ItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1) + sizeof(fp4);
	mint NewSize = nNew * ItemSize;
	
	uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
	uint8 *pOldData = (uint8 *)pSeq->m_pData;
	
	int iOldPos = _iKF;

	Private_Anim_CopyDataTimed(pNewData, pOldData, iNewPos, iOldPos, (m_Dimensions+1), (m_Dimensions+1), 1, true);
	if (iNewPos > iOldPos)
	{
		// Copy data
		Private_Anim_CopyDataTimed(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), iOldPos, true);
		Private_Anim_CopyDataTimed(pNewData, pOldData, iOldPos, iOldPos + 1, (m_Dimensions+1), (m_Dimensions+1), iNewPos - iOldPos, true);
		Private_Anim_CopyDataTimed(pNewData, pOldData, iNewPos+1, iNewPos+1, (m_Dimensions+1), (m_Dimensions+1), nNew - (iNewPos+1), true);
	}
	else
	{
		Private_Anim_CopyDataTimed(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), iNewPos, true);
		Private_Anim_CopyDataTimed(pNewData, pOldData, iNewPos+1, iNewPos, (m_Dimensions+1), (m_Dimensions+1), (iOldPos + 1) - (iNewPos + 1), true);
		Private_Anim_CopyDataTimed(pNewData, pOldData, iOldPos+1, iOldPos+1, (m_Dimensions+1), (m_Dimensions+1), nNew - (iOldPos+1), true);
	}

	MRTC_GetMemoryManager()->Free(pOldData);
	pSeq->m_pData = pNewData;
	pSeq->m_nKeyFrames = nNew;
	return iNewPos;
}

fp4 CRegistry_Dynamic::Anim_ThisGetKFTime(int _iSeq, int _iKF) const
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	return Private_Anim_GetKFTime(pSeq, _iKF);
}


fp4 CRegistry_Dynamic::Anim_ThisGetWrappedTime(const CMTime &_Time, int _iSeq) const
{
	Private_Anim_CheckValidSequence(_iSeq);
	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	return Private_Anim_GetWrappedTime(pSeq, _Time, AnimData.m_AnimFlags);
}


int CRegistry_Dynamic::Private_Anim_GetKFInsertPos(const CAnimationSequence *_pSeq, fp4 _Time) const
{
	if (_pSeq->m_nKeyFrames < 1)
		return 0;

	fp4 Frac;
	int Key;
	Private_Anim_ThisGetKF(_pSeq, CMTime::CreateFromSeconds(_Time), Frac, &Key, (uint32 *)NULL, 0, 0, 0);
	return Min(Key + 1, _pSeq->m_nKeyFrames);
}

void CRegistry_Dynamic::Private_Anim_SetItem(CAnimationSequence *_pSeq, int _iKF, const void *_pData, int _Type, int _nDim)
{
//	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = _pSeq;

	void *pData = Private_Anim_GetKFData(pSeq, _iKF);
	ms_lTypeConvert[_Type][m_Type](_pData, _nDim, pData, (m_Dimensions+1));
	
}

int CRegistry_Dynamic::Private_Anim_InsertItem(CAnimationSequence *_pSeq, fp4 _Time, const void *_pData, int _Type, int _nDim)
{
	CAnimationData &AnimData = Type_GetAnimationData();
	int iInsert = Private_Anim_GetKFInsertPos(_pSeq, _Time);
	
	CAnimationSequence *pSeq = _pSeq;

	int nNew = pSeq->m_nKeyFrames + 1;
	mint ItemSize = ms_lTypeSize[m_Type] * (m_Dimensions+1);
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
		ItemSize +=  sizeof(fp4);

	mint NewSize = nNew * ItemSize;

	uint8 *pNewData = (uint8 *)M_ALLOC(NewSize);
	uint8 *pOldData = (uint8 *)pSeq->m_pData;
	
	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
	{
		// Insert new item
		Private_Anim_CopyDataDstTimedTyped(pNewData, (void *)_pData, iInsert, 0, (m_Dimensions+1), _nDim, 1, false, _Type );
		// Set Time
		Private_Anim_SetKFTime(pNewData, m_Type, (m_Dimensions+1), iInsert, _Time);

		// Copy data
		Private_Anim_CopyDataTimed(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), iInsert, true);
		Private_Anim_CopyDataTimed(pNewData, pOldData, iInsert + 1, iInsert, (m_Dimensions+1), (m_Dimensions+1), nNew - (iInsert + 1), true);
	}
	else
	{
		// Insert new item
		Private_Anim_CopyDataTyped(pNewData, (void *)_pData, iInsert, 0, (m_Dimensions+1), _nDim, 1, false, _Type);
		// Copy data
		Private_Anim_CopyData(pNewData, pOldData, 0, 0, (m_Dimensions+1), (m_Dimensions+1), iInsert, true);
		Private_Anim_CopyData(pNewData, pOldData, iInsert + 1, iInsert, (m_Dimensions+1), (m_Dimensions+1), nNew - (iInsert + 1), true);
	}

	MRTC_GetMemoryManager()->Free(pOldData);
	pSeq->m_pData = pNewData;
	pSeq->m_nKeyFrames = nNew;

	return iInsert;
}

// Adds
int CRegistry_Dynamic::Anim_ThisAddKF(int _iSeq, CStr _Val, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);
	Type_Set(REGISTRY_TYPE_STR, 1);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	CStr::spCStrData spData = _Val.GetStrData();
	int iInsert = Private_Anim_InsertItem(pSeq, _Time, &spData, REGISTRY_TYPE_STR, 1);
	return iInsert;
}

int CRegistry_Dynamic::Anim_ThisAddKFi(int _iSeq, int32 _Val, int _StoreType, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);
	Type_Set(_StoreType, 1);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	int iInsert = Private_Anim_InsertItem(pSeq, _Time, &_Val, REGISTRY_TYPE_INT32, 1);
	return iInsert;
}

int CRegistry_Dynamic::Anim_ThisAddKFf(int _iSeq, fp4 _Val, int _StoreType, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);
	Type_Set(_StoreType, 1);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	int iInsert = Private_Anim_InsertItem(pSeq, _Time, &_Val, REGISTRY_TYPE_FP4, 1);
	return iInsert;
}

int CRegistry_Dynamic::Anim_ThisAddKFd(int _iSeq, const uint8* _pValue, int _Size, bint _bQuick, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);
	Type_Set(REGISTRY_TYPE_DATA, 1);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	TArray<uint8> lValue;
	lValue.SetLen(_Size);

	memcpy(lValue.GetBasePtr(), _pValue, _Size);

	int iInsert = Private_Anim_InsertItem(pSeq, _Time, &lValue, REGISTRY_TYPE_DATA, 1);
	return iInsert;
}

int CRegistry_Dynamic::Anim_ThisAddKFd(int _iSeq, TArray<uint8> _lValue, bint _bReference, fp4 _Time)
{
	if (_bReference)
	{
		Private_Anim_CheckValidSequence(_iSeq);
		Type_Set(REGISTRY_TYPE_DATA, 1);
		CAnimationData &AnimData = Type_GetAnimationData();
		CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

		int iInsert = Private_Anim_InsertItem(pSeq, _Time, &_lValue, REGISTRY_TYPE_DATA, 1);
		return iInsert;
	}
	else
		return Anim_ThisAddKFd(_iSeq, _lValue.GetBasePtr(), _lValue.Len(), false);
}

int CRegistry_Dynamic::Anim_ThisAddKFa(int _iSeq, int _nDim, const CStr *_pVal, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);
	Type_Set(REGISTRY_TYPE_STR, _nDim);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	CStr::spCStrData Data[EMaxDimensions];
	for (int i = 0; i < _nDim; ++i)
	{
		Data[i] = _pVal[i].GetStrData();
	}
	int iInsert = Private_Anim_InsertItem(pSeq, _Time, Data, REGISTRY_TYPE_STR, _nDim);
	return iInsert;
}

int CRegistry_Dynamic::Anim_ThisAddKFai(int _iSeq, int _nDim, const int32 *_pVal, int _StoreType, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);
	Type_Set(_StoreType, _nDim);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	int iInsert = Private_Anim_InsertItem(pSeq, _Time, _pVal, REGISTRY_TYPE_INT32, _nDim);
	return iInsert;
}

int CRegistry_Dynamic::Anim_ThisAddKFaf(int _iSeq, int _nDim, const fp4 *_pVal, int _StoreType, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);
	Type_Set(_StoreType, _nDim);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	int iInsert = Private_Anim_InsertItem(pSeq, _Time, _pVal, REGISTRY_TYPE_FP4, _nDim);
	return iInsert;
}

int CRegistry_Dynamic::Anim_ThisAddKFad(int _iSeq, int _nDim, const TArray<uint8> *_lValue, bint _bReference, fp4 _Time)
{
	Private_Anim_CheckValidSequence(_iSeq);
	Type_Set(REGISTRY_TYPE_DATA, _nDim);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	int iInsert = Private_Anim_InsertItem(pSeq, _Time, _lValue, REGISTRY_TYPE_DATA, _nDim);
	return iInsert;
}

// Set
void CRegistry_Dynamic::Anim_ThisSetKFValueConvert(int _iSeq, int _iKF, CStr _Val, int _nDim, int _StoreType, fp4 _Time)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);

	Type_Set(_StoreType, _nDim);

	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	CStr::spCStrData spData = _Val.GetStrData();

	if (_nDim == 1 && _StoreType == REGISTRY_TYPE_STR && spData)
	{
		if (spData->IsAnsi())
		{
			const ch8 *pParse = spData->Str();
			CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse));
			spData = Dst.GetStrData();
		}
		else
		{
			const wchar *pParse = spData->StrW();
			CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse));
			spData = Dst.GetStrData();
		}
	}

	Private_Anim_SetItem(pSeq, _iKF, &spData, REGISTRY_TYPE_STR, 1);

	if (AnimData.m_InternalAnimFlags & ERegistryDynamicAnimFlag_Timed)
		Private_Anim_SetKFTime(pSeq->m_pData, _iKF, _Time);
}

void CRegistry_Dynamic::Anim_ThisSetKFValue(int _iSeq, int _iKF, CStr _Val)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	Type_Set(REGISTRY_TYPE_STR, 1);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	CStr::spCStrData spData = _Val.GetStrData();
	Private_Anim_SetItem(pSeq, _iKF, &spData, REGISTRY_TYPE_STR, 1);
}

void CRegistry_Dynamic::Anim_ThisSetKFValuei(int _iSeq, int _iKF, int32 _Val, int _StoreType)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	Type_Set(_StoreType, 1);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	Private_Anim_SetItem(pSeq, _iKF, &_Val, REGISTRY_TYPE_INT32, 1);
}

void CRegistry_Dynamic::Anim_ThisSetKFValuef(int _iSeq, int _iKF, fp4 _Val, int _StoreType)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	Type_Set(_StoreType, 1);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	Private_Anim_SetItem(pSeq, _iKF, &_Val, REGISTRY_TYPE_FP4, 1);
}

void CRegistry_Dynamic::Anim_ThisSetKFValued(int _iSeq, int _iKF, const uint8* _pValue, int _Size, bint _bQuick)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	Type_Set(REGISTRY_TYPE_DATA, 1);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	TArray<uint8> lValue;
	lValue.SetLen(_Size);

	memcpy(lValue.GetBasePtr(), _pValue, _Size);


	Private_Anim_SetItem(pSeq, _iKF, &lValue, REGISTRY_TYPE_DATA, 1);
}

void CRegistry_Dynamic::Anim_ThisSetKFValued(int _iSeq, int _iKF, TArray<uint8> _lValue, bint _bReference)
{
	if (_bReference)
	{
		Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
		Type_Set(REGISTRY_TYPE_DATA, 1);
		CAnimationData &AnimData = Type_GetAnimationData();
		CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

		Private_Anim_SetItem(pSeq, _iKF, &_lValue, REGISTRY_TYPE_DATA, 1);
	}
	else
		return Anim_ThisSetKFValued(_iSeq, _iKF, _lValue.GetBasePtr(), _lValue.Len(), false);
}

void CRegistry_Dynamic::Anim_ThisSetKFValuea(int _iSeq, int _iKF, int _nDim, const CStr *_pVal)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	Type_Set(REGISTRY_TYPE_STR, _nDim);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	CStr::spCStrData Data[EMaxDimensions];
	for (int i = 0; i < _nDim; ++i)
		Data[i] = _pVal[i].GetStrData();
	Private_Anim_SetItem(pSeq, _iKF, Data, REGISTRY_TYPE_STR, _nDim);
}

void CRegistry_Dynamic::Anim_ThisSetKFValueai(int _iSeq, int _iKF, int _nDim, const int32 *_pVal, int _StoreType)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	Type_Set(_StoreType, _nDim);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	Private_Anim_SetItem(pSeq, _iKF, _pVal, REGISTRY_TYPE_INT32, _nDim);
}

void CRegistry_Dynamic::Anim_ThisSetKFValueaf(int _iSeq, int _iKF, int _nDim, const fp4 *_pVal, int _StoreType)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	Type_Set(_StoreType, _nDim);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	Private_Anim_SetItem(pSeq, _iKF, _pVal, REGISTRY_TYPE_FP4, _nDim);
}

void CRegistry_Dynamic::Anim_ThisSetKFValuead(int _iSeq, int _iKF, int _nDim, const TArray<uint8> *_lValue, bint _bReference)
{
	Private_Anim_CheckValidSequenceKeyFrame(_iSeq, _iKF);
	Type_Set(REGISTRY_TYPE_DATA, _nDim);
	CAnimationData &AnimData = Type_GetAnimationData();
	CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];

	Private_Anim_SetItem(pSeq, _iKF, _lValue, REGISTRY_TYPE_DATA, _nDim);
}


void CRegistry_Dynamic::Private_Anim_GetItem(const CAnimationSequence *_pSeq, int _iKF, void *_pData, int _Type, int _nDim) const
{
//	const CAnimationData &AnimData = Type_GetAnimationData();
	const CAnimationSequence *pSeq = _pSeq;
	const void *pData = Private_Anim_GetKFData(pSeq, _iKF);
	ms_lTypeConvert[m_Type][_Type](pData, (m_Dimensions+1), _pData, _nDim);
}

// Get Value
CStr CRegistry_Dynamic::Anim_ThisGetKFValue(int _iSeq, int _iKF) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return GetThisValue();
	const CAnimationData &AnimData = Type_GetAnimationData();
	if (!AnimData.IsValidSequenceKeyframe(_iSeq, _iKF))
		return CStr();
	
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	CStr::spCStrData spData;
	Private_Anim_GetItem(pSeq, _iKF, &spData, REGISTRY_TYPE_STR, 1);
	return CStr(spData.p);
}

int32 CRegistry_Dynamic::Anim_ThisGetKFValuei(int _iSeq, int _iKF) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return GetThisValuei();
	const CAnimationData &AnimData = Type_GetAnimationData();
	if (!AnimData.IsValidSequenceKeyframe(_iSeq, _iKF))
		return 0;
	
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	int32 Val;
	Private_Anim_GetItem(pSeq, _iKF, &Val, REGISTRY_TYPE_INT32, 1);
	return Val;
}

fp4 CRegistry_Dynamic::Anim_ThisGetKFValuef(int _iSeq, int _iKF) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return GetThisValuef();
	const CAnimationData &AnimData = Type_GetAnimationData();
	if (!AnimData.IsValidSequenceKeyframe(_iSeq, _iKF))
		return 0;
	
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	fp4 Val;
	Private_Anim_GetItem(pSeq, _iKF, &Val, REGISTRY_TYPE_FP4, 1);
	return Val;
}


TArray<uint8> CRegistry_Dynamic::Anim_ThisGetKFValued(int _iSeq, int _iKF) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return GetThisValued();
	const CAnimationData &AnimData = Type_GetAnimationData();
	if (!AnimData.IsValidSequenceKeyframe(_iSeq, _iKF))
		return TArray<uint8>();
	
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	TArray<uint8> Val;
	Private_Anim_GetItem(pSeq, _iKF, &Val, REGISTRY_TYPE_DATA, 1);
	return Val;
}


void CRegistry_Dynamic::Anim_ThisGetKFValuea(int _iSeq, int _iKF, int _nDim, CStr *_pDest) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return GetThisValuea(_nDim, _pDest);

	Private_CheckDim(_nDim);

	const CAnimationData &AnimData = Type_GetAnimationData();
	if (!AnimData.IsValidSequenceKeyframe(_iSeq, _iKF))
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = CStr();

		return;
	}
	
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	CStr::spCStrData Data[EMaxDimensions];
	Private_Anim_GetItem(pSeq, _iKF, Data, REGISTRY_TYPE_STR, _nDim);
	for (int i = 0; i < _nDim; ++i)
	{
        _pDest[i] = CStr(Data[i].p);
	}	
}

void CRegistry_Dynamic::Anim_ThisGetKFValueai(int _iSeq, int _iKF, int _nDim, int32 *_pDest) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return GetThisValueai(_nDim, _pDest);

	Private_CheckDim(_nDim);

	const CAnimationData &AnimData = Type_GetAnimationData();
	if (!AnimData.IsValidSequenceKeyframe(_iSeq, _iKF))
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = 0;

		return;
	}
	
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	Private_Anim_GetItem(pSeq, _iKF, _pDest, REGISTRY_TYPE_INT32, _nDim);
}

void CRegistry_Dynamic::Anim_ThisGetKFValueaf(int _iSeq, int _iKF, int _nDim, fp4 *_pDest) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return GetThisValueaf(_nDim, _pDest);

	Private_CheckDim(_nDim);

	const CAnimationData &AnimData = Type_GetAnimationData();
	if (!AnimData.IsValidSequenceKeyframe(_iSeq, _iKF))
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = 0;

		return;
	}
	
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	Private_Anim_GetItem(pSeq, _iKF, _pDest, REGISTRY_TYPE_FP4, _nDim);
}

void CRegistry_Dynamic::Anim_ThisGetKFValuead(int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest) const
{
	if (!(m_InternalFlags & ERegistryDynamicFlags_Animated))
		return GetThisValuead(_nDim, _pDest);

	Private_CheckDim(_nDim);

	const CAnimationData &AnimData = Type_GetAnimationData();
	if (!AnimData.IsValidSequenceKeyframe(_iSeq, _iKF))
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = TArray<uint8> ();

		return;
	}
	
	const CAnimationSequence *pSeq = AnimData.m_Sequences[_iSeq];
	Private_Anim_GetItem(pSeq, _iKF, _pDest, REGISTRY_TYPE_DATA, _nDim);
}




class SYSTEMDLLEXPORT CRegistry_Dynamic_ThreadSafe : public CRegistry_Dynamic
{
	MRTC_DECLARE;
private:

protected:

	mutable MRTC_CriticalSection m_Lock;

	int MRTC_AddRef()
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::MRTC_AddRef();
	}

	int MRTC_DelRef()
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::MRTC_DelRef();
	}

	int MRTC_ReferenceCount() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::MRTC_ReferenceCount();
	}

public:

	void Hash_Invalidate()
	{
		M_LOCK(m_Lock);
		CRegistry_Dynamic::Hash_Invalidate();
	}

	virtual void operator= (const CRegistry& _Reg)
	{
		M_LOCK(m_Lock);
		CRegistry_Dynamic::operator= (_Reg);
	}

	spCRegistry Duplicate() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Duplicate();
	}

	void Clear()
	{
		M_LOCK(m_Lock);
		CRegistry_Dynamic::Clear();
	}


	CRegistry *GetParent() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetParent();
	}

	void SetParent(CRegistry *_pParent)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetParent(_pParent);
	}

	bint ValidChild(int _iChild) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::ValidChild(_iChild);
	}

	virtual int GetNumChildren() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetNumChildren();
	}
	virtual void SetNumChildren(int _nChildren)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetNumChildren(_nChildren);
	}

	virtual CRegistry* GetChild(int _iChild)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetChild(_iChild);
	}

	virtual const CRegistry* GetChild(int _iChild) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetChild(_iChild);
	}
	virtual const CRegistry* GetChildUnsafe(int _iChild) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetChildUnsafe(_iChild);
	}

	virtual int GetType() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetType();
	}
	virtual int GetDimensions() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetDimensions();
	}
	virtual void ConvertToType(int _Type, int _nDim)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::ConvertToType(_Type, _nDim);
	}

	// Sort register
	virtual void Sort(bint _bReversedOrder = false, bint _bSortByName = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Sort(_bReversedOrder, _bSortByName);
	}

	// Search children
	virtual int FindIndex(const char* _pKey) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::FindIndex(_pKey);
	}
	virtual int FindIndex(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue = true) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::FindIndex(_pKey, _pValue, _bCaseSensitiveValue);
	}

	virtual CRegistry* CreateDir(const char* _pPath)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::CreateDir(_pPath);
	}
	virtual bint DeleteDir(const char* _pPath)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::DeleteDir(_pPath);
	}
	virtual void CopyDir(const CRegistry* _pReg, bint _bRecursive = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::CopyDir(_pReg, _bRecursive);
	}

	virtual void AddReg(spCRegistry _spReg)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::AddReg(_spReg);
	}

	virtual CRegistry *CreateNewChild()
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::CreateNewChild();
	}

	virtual void DeleteKey(int _iKey)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::DeleteKey(_iKey);
	}

	// Setting
	virtual void SetThisName(const char* _pName)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisName(_pName);
	}

	virtual void SetThisValueConvert(CStr _Value, int _nDim, int _Type)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValueConvert(_Value, _nDim, _Type);
	}

	virtual void SetThisValue(const char* _pValue)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValue(_pValue);
	}
	virtual void SetThisValue(const wchar* _pValue)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValue(_pValue);
	}
	virtual void SetThisValue(CStr _Value)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValue(_Value);
	}
	virtual void SetThisValuei(int32 _Value, int _StoreType = REGISTRY_TYPE_INT32)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValuei(_Value, _StoreType);
	}
	virtual void SetThisValuef(fp4 _Value, int _StoreType = REGISTRY_TYPE_FP4)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValuef(_Value, _StoreType);
	}
	virtual void SetThisValued(const uint8* _pValue, int _Size, bint _bQuick = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValued(_pValue, _Size, _bQuick);
	}
	virtual void SetThisValued(TArray<uint8> _lValue, bint _bReference = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValued(_lValue, _bReference);
	}

	virtual void SetThisValuea(int _nDim, const CStr *_Value)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValuea(_nDim, _Value);
	}
	virtual void SetThisValuead(int _nDim, const TArray<uint8> *_lValue, bint _bReference = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValuead(_nDim, _lValue, _bReference);
	}
	virtual void SetThisValueai(int _nDim, const int32 *_Value, int _StoreType = REGISTRY_TYPE_INT32)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValueai(_nDim, _Value, _StoreType);
	}
	virtual void SetThisValueaf(int _nDim, const fp4 *_Value, int _StoreType = REGISTRY_TYPE_FP4)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisValueaf(_nDim, _Value, _StoreType);
	}


	// Getting value from this

	// Get/Set user flags, user flags are 8-bit
	virtual void SetThisUserFlags(uint32 _Value)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetThisUserFlags(_Value);
	}
	virtual uint32 GetThisUserFlags() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisUserFlags();
	}

	virtual CStr GetThisName() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisName();
	}
	virtual uint32 GetThisNameHash() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisNameHash();
	}
	virtual const char* GetThisNameUnsafe() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisNameUnsafe();
	}

	virtual CStr GetThisValue() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValue();
	}
	virtual int32 GetThisValuei() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValuei();
	}
	virtual fp4 GetThisValuef() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValuef();
	}
	virtual const TArray<uint8> GetThisValued() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValued();
	}
	virtual TArray<uint8> GetThisValued()
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValued();
	}

	virtual void GetThisValuea(int _nDim, CStr *_pDest) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValuea(_nDim, _pDest);
	}
	virtual void GetThisValueai(int _nDim, int32 *_pDest) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValueai(_nDim, _pDest);
	}
	virtual void GetThisValueaf(int _nDim, fp4 *_pDest) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValueaf(_nDim, _pDest);
	}
	virtual void GetThisValuead(int _nDim, TArray<uint8> *_pDest) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::GetThisValuead(_nDim, _pDest);
	}

	/////////////////////////////////////////////////////////////////
	// Animation
	////////////////////////////////////////////////////////////////

	virtual void Anim_ThisSetAnimated(bint _bEnable)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetAnimated(_bEnable);
	}
	virtual bint Anim_ThisGetAnimated() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetAnimated();
	}

	virtual void Anim_ThisSetDisableAutoDemote(bint _bEnable)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetDisableAutoDemote(_bEnable);
	}
	virtual bint Anim_ThisGetDisableAutoDemote() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetDisableAutoDemote();
	}

	virtual void Anim_ThisSetEnableTimed(bint _bEnable)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetEnableTimed(_bEnable);
	}
	virtual bint Anim_ThisGetEnableTimed() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetEnableTimed();
	}

	virtual void Anim_ThisSetFlags(uint32 _Flags)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetFlags(_Flags);
	}
	virtual uint32 Anim_ThisGetFlags() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetFlags();
	}

	virtual void Anim_ThisSetInterpolate(uint32 _InterpolateType, const fp4 *_pParams, int _nParams)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetInterpolate(_InterpolateType, _pParams, _nParams);
	}
	virtual uint32 Anim_ThisGetInterpolate(fp4 *_pParams, int &_nParams) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetInterpolate(_pParams, _nParams);
	}

	virtual bint Anim_ThisIsValidSequenceKeyframe(int _iSec, int _iKF) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisIsValidSequenceKeyframe(_iSec, _iKF);
	}

	virtual int Anim_ThisAddSeq()
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddSeq();
	}
	virtual int Anim_ThisInsertSeq(int _iSeqAfter)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisInsertSeq(_iSeqAfter);
	}
	virtual void Anim_ThisDeleteSeq(int _iSeq)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisDeleteSeq(_iSeq);
	}
	virtual void Anim_ThisSetNumSeq(int _nSeq)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetNumSeq(_nSeq);
	}
	virtual int Anim_ThisGetNumSeq() const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetNumSeq();
	}

	virtual fp4 Anim_ThisGetSeqLoopStart(int _iSeq) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetSeqLoopStart(_iSeq);
	}
	virtual fp4 Anim_ThisGetSeqLoopEnd(int _iSeq) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetSeqLoopEnd(_iSeq);
	}

	virtual fp4 Anim_ThisGetSeqLength(int _iSeq) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetSeqLength(_iSeq);
	}

	virtual void Anim_ThisSetSeqLoopStart(int _iSeq, fp4 _Time)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetSeqLoopStart(_iSeq, _Time);
	}
	virtual void Anim_ThisSetSeqLoopEnd(int _iSeq, fp4 _Time)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetSeqLoopEnd(_iSeq, _Time);
	}

	virtual void Anim_ThisGetKF(int _iSeq, fp4 _Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKF(_iSeq, _Time, _Fraction, _pKeys, _pTimeDeltas, _nPre, _nPost);
	}
	virtual void Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKF(_iSeq, _Time, _Fraction, _pKeys, _pTimeDeltas, _nPre, _nPost);
	}

	virtual void Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, uint32 *_pDeltasCalc, int _nPre, int _nPost) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKF(_iSeq, _Time, _Fraction, _pKeys, _pDeltasCalc, _nPre, _nPost);
	}

	virtual void Anim_ThisSetNumKF(int _iSeq, int _nKF)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetNumKF(_iSeq, _nKF);
	}
	virtual int Anim_ThisGetNumKF(int _iSeq = 0) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetNumKF(_iSeq);
	}

	virtual void Anim_ThisDeleteKF(int _iSeq, int _iKF)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisDeleteKF(_iSeq, _iKF);
	}

	virtual int Anim_ThisSetKFTime(int _iSeq, int _iKF, fp4 _Time)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFTime(_iSeq, _iKF, _Time);
	}
	virtual fp4 Anim_ThisGetKFTime(int _iSeq, int _iKF) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFTime(_iSeq, _iKF);
	}

	virtual fp4 Anim_ThisGetWrappedTime(const CMTime &_Time, int _iSeq = 0) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetWrappedTime(_Time, _iSeq);
	}

	// Adds
	virtual int Anim_ThisAddKF(int _iSeq, CStr _Val, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKF(_iSeq, _Val, _Time);
	}
	virtual int Anim_ThisAddKFi(int _iSeq, int32 _Val, int _StoreType = REGISTRY_TYPE_INT32, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKFi(_iSeq, _Val, _StoreType, _Time);
	}
	virtual int Anim_ThisAddKFf(int _iSeq, fp4 _Val, int _StoreType = REGISTRY_TYPE_FP4, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKFf(_iSeq, _Val, _StoreType, _Time);
	}
	virtual int Anim_ThisAddKFd(int _iSeq, const uint8* _pValue, int _Size, bint _bQuick = true, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKFd(_iSeq, _pValue, _Size, _bQuick, _Time);
	}
	virtual int Anim_ThisAddKFd(int _iSeq, TArray<uint8> _lValue, bint _bReference = true, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKFd(_iSeq, _lValue, _bReference, _Time);
	}

	virtual int Anim_ThisAddKFa(int _iSeq, int _nDim, const CStr *_pVal, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKFa(_iSeq, _nDim, _pVal, _Time);
	}
	virtual int Anim_ThisAddKFai(int _iSeq, int _nDim, const int32 *_pVal, int _StoreType = REGISTRY_TYPE_INT32, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKFai(_iSeq, _nDim, _pVal, _StoreType, _Time);
	}
	virtual int Anim_ThisAddKFaf(int _iSeq, int _nDim, const fp4 *_pVal, int _StoreType = REGISTRY_TYPE_FP4, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKFaf(_iSeq, _nDim, _pVal, _StoreType, _Time);
	}
	virtual int Anim_ThisAddKFad(int _iSeq, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true, fp4 _Time = -1)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisAddKFad(_iSeq, _nDim, _lValue, _bReference, _Time);
	}

	// Set
	virtual void Anim_ThisSetKFValueConvert(int _iSeq, int _iKF, CStr _Val, int _nDim, int _StoreType, fp4 _Time)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValueConvert(_iSeq, _iKF, _Val, _nDim, _StoreType, _Time);
	}

	virtual void Anim_ThisSetKFValue(int _iSeq, int _iKF, CStr _Val)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValue(_iSeq, _iKF, _Val);
	}
	virtual void Anim_ThisSetKFValuei(int _iSeq, int _iKF, int32 _Val, int _StoreType = REGISTRY_TYPE_INT32)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValuei(_iSeq, _iKF, _Val, _StoreType);
	}
	virtual void Anim_ThisSetKFValuef(int _iSeq, int _iKF, fp4 _Val, int _StoreType = REGISTRY_TYPE_FP4)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValuef(_iSeq, _iKF, _Val, _StoreType);
	}
	virtual void Anim_ThisSetKFValued(int _iSeq, int _iKF, const uint8* _pValue, int _Size, bint _bQuick = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValued(_iSeq, _iKF, _pValue, _Size, _bQuick);
	}
	virtual void Anim_ThisSetKFValued(int _iSeq, int _iKF, TArray<uint8> _lValue, bint _bReference = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValued(_iSeq, _iKF, _lValue, _bReference);
	}

	virtual void Anim_ThisSetKFValuea(int _iSeq, int _iKF, int _nDim, const CStr *_pVal)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValuea(_iSeq, _iKF, _nDim, _pVal);
	}
	virtual void Anim_ThisSetKFValueai(int _iSeq, int _iKF, int _nDim, const int32 *_pVal, int _StoreType = REGISTRY_TYPE_INT32)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValueai(_iSeq, _iKF, _nDim, _pVal, _StoreType);
	}
	virtual void Anim_ThisSetKFValueaf(int _iSeq, int _iKF, int _nDim, const fp4 *_pVal, int _StoreType = REGISTRY_TYPE_FP4)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValueaf(_iSeq, _iKF, _nDim, _pVal, _StoreType);
	}
	virtual void Anim_ThisSetKFValuead(int _iSeq, int _iKF, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisSetKFValuead(_iSeq, _iKF, _nDim, _lValue, _bReference);
	}

	// Get Value
	virtual CStr Anim_ThisGetKFValue(int _iSeq, int _iKF) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFValue(_iSeq, _iKF);
	}
	virtual int32 Anim_ThisGetKFValuei(int _iSeq, int _iKF) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFValuei(_iSeq, _iKF);
	}
	virtual fp4 Anim_ThisGetKFValuef(int _iSeq, int _iKF) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFValuef(_iSeq, _iKF);
	}
	virtual TArray<uint8> Anim_ThisGetKFValued(int _iSeq, int _iKF) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFValued(_iSeq, _iKF);
	}

	virtual void Anim_ThisGetKFValuea(int _iSeq, int _iKF, int _nDim, CStr *_pDest) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFValuea(_iSeq, _iKF, _nDim, _pDest);
	}
	virtual void Anim_ThisGetKFValueai(int _iSeq, int _iKF, int _nDim, int32 *_pDest) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFValueai(_iSeq, _iKF, _nDim, _pDest);
	}
	virtual void Anim_ThisGetKFValueaf(int _iSeq, int _iKF, int _nDim, fp4 *_pDest) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFValueaf(_iSeq, _iKF, _nDim, _pDest);
	}
	virtual void Anim_ThisGetKFValuead(int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Anim_ThisGetKFValuead(_iSeq, _iKF, _nDim, _pDest);
	}

	////////////////// IO
	
	virtual int XRG_Parse(char* _pOrgData, char* _pData, int _Size, CRegistry_ParseContext& _ParseContext)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::XRG_Parse(_pOrgData, _pData, _Size, _ParseContext);
	}
	virtual int XRG_Parse(wchar* _pOrgData, wchar* _pData, int _Size, CRegistry_ParseContext& _ParseContext)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::XRG_Parse(_pOrgData, _pData, _Size, _ParseContext);
	}
	virtual void XRG_Read(const CStr& _Filename, TArray<CStr> _lDefines, bint _bSlashIsEscape = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::XRG_Read(_Filename, _lDefines, _bSlashIsEscape);
	}
	virtual void XRG_Read(CCFile *_pFile, CStr _ThisFileName, TArray<CStr> _lDefines, bint _bSlashIsEscape = true)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::XRG_Read(_pFile, _ThisFileName, _lDefines, _bSlashIsEscape);
	}
	virtual void XRG_Read(const CStr& _Filename)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::XRG_Read(_Filename);
	}

	virtual void ReadSimple(CCFile* _pFile)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::ReadSimple(_pFile);
	}
	virtual void ReadSimple(CStr _FileName)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::ReadSimple(_FileName);
	}
	virtual void ReadRegistryDir(CStr _Dir, CCFile* _pFile)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::ReadRegistryDir(_Dir, _pFile);
	}
	virtual void ReadRegistryDir(CStr _Dir, CStr _Filename)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::ReadRegistryDir(_Dir, _Filename);
	}

	// -------------------------------------------------------------------
	// Binary IO
	virtual void Read(CCFile* _pFile, int _Version)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Read(_pFile, _Version);
	}
	virtual void Write(CCFile* _pFile) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Write(_pFile);
	}

	// Read/Write from data-file entry.
	virtual bint Read(CDataFile* _pDFile, const char* _pEntryName)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Read(_pDFile, _pEntryName);
	}
	virtual void Write(CDataFile* _pDFile, const char* _pEntryName) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Write(_pDFile, _pEntryName);
	}

	// Read/Write datafile.
	virtual void Read(const char* _pFileName)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Read(_pFileName);
	}
	virtual void Write(const char* _pFileName) const
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::Write(_pFileName);
	}
	virtual void SetChild(int _iChild, CRegistry*_pReg)
	{
		M_LOCK(m_Lock);
		return CRegistry_Dynamic::SetChild(_iChild, _pReg);
	}
};

spCRegistry CRegistry_Dynamic::MakeThreadSafe()
{
	// Make sure that this will not be deleted while we are working
	spCRegistry spThis = this;
	spCRegistry spNew;
	{
		CRegistry_Dynamic_ThreadSafe *pNew = DNew(CRegistry_Dynamic_ThreadSafe) CRegistry_Dynamic_ThreadSafe;
		spNew = pNew;

		CRegistry *pParent = GetParent();
		if (pParent)
		{
			int nChildren = pParent->GetNumChildren();
			for (int i = 0; i < nChildren; ++i)
			{
				CRegistry *pChild = pParent->GetChild(i);
				if (pChild == this)
				{
					pParent->SetChild(i, pNew);
				}
			}
		}

		pNew->m_pParent = pParent;

		// Copy stuff
		{
			pNew->Private_CopyValueFromRegistryDynamic(*this);

			if (GetNumChildren())
			{
				int nChildren = GetNumChildren();
				pNew->m_lChildren.SetLen(nChildren);
				for(int i = 0; i < nChildren; i++)
				{
					pNew->m_lChildren[i].m_spReg = m_lChildren[i].m_spReg;
					pNew->m_lChildren[i].m_spReg->SetParent(pNew);
				}
			}
			else
				pNew->m_lChildren.Clear();
		}
	}
	return spNew;
}

MRTC_IMPLEMENT( CRegistry_Dynamic_ThreadSafe, CRegistry_Dynamic );
