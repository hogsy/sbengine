#ifndef _INC_MREGISTRY_DYNAMIC
#define _INC_MREGISTRY_DYNAMIC

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CRegistry
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios 2002

 	Creation Date:	2002-04-12

	Contents:		CRegistry
					
	Comments:		
\*____________________________________________________________________________________________*/

#include "MRegistry.h"
#include "MMath_fp2.h"

//#define REGISTRY_COMPILEDORDERSIMULATE

class CRegistry_Shared_Helpers;
class CRegistry_DynamicParseKeyOptions;
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CRegistry_Dynamic : public CRegistry
{
	MRTC_DECLARE;
	friend class CRegistry_Shared_Helpers;
	friend class CRegistry_DynamicParseKeyOptions;
private:

	class CPtrKeyHash	// 12
	{
	public:
		spCRegistry m_spReg;
		uint32 m_Hash;			// Ha dessa två som 16bit * 2 ?
		uint32 m_iHashKey;

		CPtrKeyHash()
		{
		}
		CPtrKeyHash(spCRegistry _spReg)
		{
			m_spReg = _spReg;
		}

		CPtrKeyHash &operator = (CPtrKeyHash & _Source)
		{
			m_Hash = _Source.m_Hash;
			m_iHashKey = _Source.m_iHashKey;
			m_spReg.p = _Source.m_spReg.p; // Copy pointer without changing refcount to be more threadsafe.
			_Source.m_spReg.p = NULL;
			return *this;
		}
	};

	class CAnimationSequence
	{
	public: 
		CAnimationSequence()
		{
			m_pData = NULL;
			m_nKeyFrames = 0;
			m_LoopStart = -1;
			m_LoopEnd = -1;
		}
		~CAnimationSequence()
		{
			if (m_pData)
				MRTC_GetMemoryManager()->Free(m_pData);
		}
		void *m_pData;
		int m_nKeyFrames;
		fp4 m_LoopStart;
		fp4 m_LoopEnd;
	};

	enum
	{
		ERegistryDynamicFlags_Animated = DBit(0),
		ERegistryDynamicFlags_HashDirty = DBit(1),
		ERegistryDynamicFlags_DisableAnimAutoDemote = DBit(2),
#ifdef REGISTRY_COMPILEDORDERSIMULATE
		ERegistryDynamicFlags_SimulateRegistryCompiled = DBit(3),
#endif

		ERegistryDynamicAnimFlag_Timed = DBit(0),

		EMaxDimensions = 32,
	};

	class CAnimationData
	{
	public:
		uint32 m_InternalAnimFlags:4;
		uint32 m_AnimFlags:16;
		uint32 m_AnimIPType:8;
		TThinArray<CAnimationSequence *> m_Sequences;
		TThinArray<fp4> m_InterpolateData;

		M_INLINE bint IsValidSequenceKeyframe(int _iSeq, int _iKF) const
		{
			if (_iSeq < 0 || _iSeq >= m_Sequences.Len())
				return false;
			if (_iKF < 0 || _iKF >= m_Sequences[_iSeq]->m_nKeyFrames)
				return false;
			return true;
		}
	};

	uint32 m_InternalFlags : 4;		// 4
	uint32 m_Dimensions : 5;
//	uint32 m_Flags : 4;		// 4
//	uint32 m_AnimFlags : 4;
	uint32 m_Type : 4;
	uint32 m_UserFlags : 8;
	uint32 m_RefCount : 11;

	CRegistry *m_pParent;
#ifdef _DEBUG
	int m_nChildrenLastHashCreate;
#endif

	CStr::spCStrData m_spName;	// 4
	TList_Vector<CPtrKeyHash, TList_Vector_AutoAlign<>, CListVectorCoreDummy> m_lChildren;	// 4

	enum
	{
		ETempStorage0 = sizeof(void *) * 2,
		ETempStorage1 = sizeof(fp4) * 3 > ETempStorage0 ? sizeof(fp4) * 3 : ETempStorage0,
		ETempStorage2 = sizeof(CAnimationData) > ETempStorage1 ? sizeof(CAnimationData) : ETempStorage1,
		EDataStorage = ETempStorage2
	};

	uint8 m_Value[EDataStorage];		// Ingen större poäng med att ta upp 16-bytes här när inte CStrData kan ligga där

	M_INLINE static mint Type_GetSize(int _Type, int _nDim)
	{
		return ms_lTypeSize[_Type] * _nDim;
	}

	M_INLINE void *Type_GetValue()
	{
		mint Size = Type_GetSize(m_Type, m_Dimensions+1);
		if (Size > EDataStorage)
			return *(void **)m_Value;
		else
			return m_Value;
	}

	M_INLINE const void *Type_GetValue() const
	{
		mint Size = Type_GetSize(m_Type, m_Dimensions+1);
		if (Size > EDataStorage)
			return *(const void **)m_Value;
		else
			return m_Value;
	}

	M_INLINE void *Type_GetValue(void *_pData, int _Type, int _nDim)
	{
		mint Size = Type_GetSize(_Type, _nDim);
		if (Size > EDataStorage)
			return *(void **)_pData;
		else
			return _pData;
	}


	static void Type_CallConstructor(int _Type, void *_pDest, int _nDim)
	{
		mint Size = Type_GetSize(_Type, _nDim);
		if (Size > EDataStorage)
		{
			if (ms_lTypeConstructorPtr[_Type])
				ms_lTypeConstructorPtr[_Type](_pDest, _nDim);
		}
		else
		{
			if (ms_lTypeConstructor[_Type])
				ms_lTypeConstructor[_Type](_pDest, _nDim);
		}
	}

	static void Type_CallDestructor(int _Type, void *_pDest, int _nDim)
	{
		mint Size = Type_GetSize(_Type, _nDim);
		if (Size > EDataStorage)
		{
			if (ms_lTypeDestructorPtr[_Type])
				ms_lTypeDestructorPtr[_Type](_pDest, _nDim);
		}
		else
		{
			if (ms_lTypeDestructor[_Type])
				ms_lTypeDestructor[_Type](_pDest, _nDim);
		}
	}

	static FRegistryTypeDestructor *ms_lTypeDestructor[REGISTRY_TYPE_MAX];
	static FRegistryTypeConstructor *ms_lTypeConstructor[REGISTRY_TYPE_MAX];

	static FRegistryTypeDestructor *ms_lTypeDestructorPtr[REGISTRY_TYPE_MAX];
	static FRegistryTypeConstructor *ms_lTypeConstructorPtr[REGISTRY_TYPE_MAX];

	static FRegistryTypeDestructor *ms_lTypeDestructorAnim[REGISTRY_TYPE_MAX];
	static FRegistryTypeConstructor *ms_lTypeConstructorAnim[REGISTRY_TYPE_MAX];

	static FRegistryTypeConvert *ms_lTypeConvert[REGISTRY_TYPE_MAX][REGISTRY_TYPE_MAX];
	static mint ms_lTypeSize[REGISTRY_TYPE_MAX];

#ifdef REGISTRY_COMPILEDORDERSIMULATE
	void Private_UpdateCompiledOrder();
	void SimulateRegistryCompiled(bint _Enable);
	bint SimulateRegistryCompiled()
	{
		return (m_InternalFlags & ERegistryDynamicFlags_SimulateRegistryCompiled) != 0;
	}

#endif

	void Private_Hash_Invalidate()
	{
#ifdef REGISTRY_COMPILEDORDERSIMULATE
		Private_UpdateCompiledOrder();
#endif
		m_InternalFlags |= ERegistryDynamicFlags_HashDirty;
	}

	M_INLINE bint Private_Anim_IsAnimated() const
	{
		return m_InternalFlags & ERegistryDynamicFlags_Animated;
	}

	void Private_Anim_DestroyData(void *_pDst, int _iStartDst, int _nItems, int _nDim);
	void Private_Anim_ConstructData(void *_pDst, int _iStartDst, int _nItems, int _nDim);

	void Private_Anim_DestroyDataTimed(void *_pDst, int _iStartDst, int _nItems, int _nDim);
	void Private_Anim_ConstructDataTimed(void *_pDst, int _iStartDst, int _nItems, int _nDim);

	void Private_Anim_CopyData(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct);
	void Private_Anim_CopyDataSrcTimed(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct);
	void Private_Anim_CopyDataDstTimed(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct);
	void Private_Anim_CopyDataTimed(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct);

	void Private_Anim_CopyDataTyped(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct, int _Type);
	void Private_Anim_CopyDataSrcTimedTyped(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct, int _Type);
	void Private_Anim_CopyDataDstTimedTyped(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct, int _Type);
	void Private_Anim_CopyDataTimedTyped(void *_pDst, void *_pSrc, int _iStartDst, int _iStartSrc, int _nDimDst, int _nDimSrc, int _nItems, bint _bDestruct, int _Type);

	void Private_Anim_DestroySequence(CAnimationSequence *_pSeq);

	int Private_Anim_InsertItem(CAnimationSequence *_pSeq, fp4 _Time, const void *_pData, int _Type, int _nDim);
	void Private_Anim_SetItem(CAnimationSequence *_pSeq, int _iKF, const void *_pData, int _Type, int _nDim);
	void Private_Anim_GetItem(const CAnimationSequence *_pSeq, int _iKF, void *_pData, int _Type, int _nDim) const;

	int Private_Anim_GetKFInsertPos(const CAnimationSequence *_pSeq, fp4 _Time) const;
	fp4 Private_Anim_GetKFTimeTimed(const CAnimationSequence *_pSeq, int _iKeyFrame) const;
	fp4 Private_Anim_GetKFTime(const CAnimationSequence *_pSeq, int _iKeyFrame) const;
	const void *Private_Anim_GetKFData(const CAnimationSequence *_pSeq, int _iKeyFrame) const;
	void *Private_Anim_GetKFData(CAnimationSequence *_pSeq, int _iKeyFrame);

	M_INLINE void Private_Anim_SetKFTime(void *_pData, int _Type, int _nDim, int _iKeyFrame, fp4 _Time)
	{
		*((fp4 *)(((uint8 *)_pData) + (ms_lTypeSize[_Type]*_nDim + sizeof(fp4)) * (_iKeyFrame + 1) - sizeof(fp4))) = _Time;
	}

	M_INLINE void Private_Anim_SetKFTime(void *_pData, int _iKeyFrame, fp4 _Time)
	{
		*((fp4 *)(((uint8 *)_pData) + (ms_lTypeSize[m_Type]*(m_Dimensions+1) + sizeof(fp4)) * (_iKeyFrame + 1) - sizeof(fp4))) = _Time;
	}

	fp4 Private_Anim_GetSequenceLength(const CAnimationSequence *_pSeq) const;
	fp4 Private_Anim_GetWrappedTime(const CAnimationSequence *_pSeq, const CMTime &_Time, uint32 _Flags) const;
	void Private_Anim_ThisGetKF(const CAnimationSequence *_pSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost, int _Flags) const;
	void Private_Anim_ThisGetKF(const CAnimationSequence *_pSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, uint32 *_pDeltasCalc, int _nPre, int _nPost, int _Flags) const;
	int Private_Anim_FindKeyByTime(const CAnimationSequence *_pSeq, fp4 _Time) const;


	void Private_Anim_Promote();
	void Private_Anim_Demote();
	void Private_Anim_CheckAutoDemote();

	M_INLINE void Private_CheckDim(int _nDim) const
	{
		if (_nDim < 1 || _nDim > EMaxDimensions)
			Error_static(M_FUNCTION, "Invalid number of dimensions.");
	}

	M_INLINE void Private_Anim_CheckAnimEnabled() const
	{
		if (!Private_Anim_IsAnimated())
			Error_static(M_FUNCTION, "The register must be animated to be able to set this option.");
	}

	M_INLINE void Private_Anim_CheckAnimDisabled() const
	{
		if (Private_Anim_IsAnimated())
			Error_static(M_FUNCTION, "The register must not be animated to be able to set this option.");
	}

	M_INLINE void Private_Anim_CheckValidSequence(int _iSeq) const
	{
		Private_Anim_CheckAnimEnabled();
		const CAnimationData &AnimData = Type_GetAnimationData();
		if (_iSeq < 0 || _iSeq >= AnimData.m_Sequences.Len())
			Error_static(M_FUNCTION, "Invalid sequence specified.");
	}

	M_INLINE void Private_Anim_CheckValidSequenceKeyFrame(int _iSeq, int _iKeyFrame) const
	{
		Private_Anim_CheckAnimEnabled();
		Private_Anim_CheckValidSequence(_iSeq);
		const CAnimationData &AnimData = Type_GetAnimationData();
		if (_iKeyFrame < 0 || _iKeyFrame >= AnimData.m_Sequences[_iSeq]->m_nKeyFrames)
			Error_static(M_FUNCTION, "Invalid key frame specified.");
	}


	void Private_Anim_TimedPromote();
	void Private_Anim_TimedDemote();
	void Private_Anim_Destroy(CAnimationData &_Data);

	void Hash_Init();
	void Type_Clear();
	void Type_SetNew(int _Type, int _nDim);
	void Type_Set(int _Type, int _nDim);

	CAnimationData &Type_GetAnimationData()
	{
		M_ASSERT(((m_InternalFlags & ERegistryDynamicFlags_Animated)), "");
		return *((CAnimationData *)m_Value);
	}

	const CAnimationData &Type_GetAnimationData() const
	{
		M_ASSERT(((m_InternalFlags & ERegistryDynamicFlags_Animated)), "");
		return *((const CAnimationData *)m_Value);
	}

	CStr::spCStrData &Type_Get_Str()
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_STR, "");
		return *((CStr::spCStrData *)m_Value);
	}
	TArray<uint8> &Type_Get_Data()
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_DATA, "");
		return *((TArray<uint8> *)m_Value);
	}
	uint8 &Type_Get_uint8()
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_UINT8, "");
		return *((uint8 *)m_Value);
	}
	int16 &Type_Get_int16()
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_INT16, "");
		return *((int16 *)m_Value);
	}
	int32 &Type_Get_int32()
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_INT32, "");
		return *((int32 *)m_Value);
	}
	uint32 &Type_Get_uint32()
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_INT32, "");
		return *((uint32 *)m_Value);
	}
	fp4 &Type_Get_fp4()
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_FP4, "");
		return *((fp4 *)m_Value);
	}
	fp2 &Type_Get_fp2()
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_FP4, "");
		return *((fp2 *)m_Value);
	}

	const CStr::spCStrData &Type_Get_Str() const
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_STR, "");
		return *((const CStr::spCStrData *)m_Value);
	}
	const TArray<uint8> &Type_Get_Data() const
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_DATA, "");
		return *((const TArray<uint8> *)m_Value);
	}
	const uint8 &Type_Get_uint8() const
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_UINT8, "");
		return *((const uint8 *)m_Value);
	}
	const int16 &Type_Get_int16() const
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_INT16, "");
		return *((const int16 *)m_Value);
	}
	const int32 &Type_Get_int32() const
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_INT32, "");
		return *((const int32 *)m_Value);
	}
	const uint32 &Type_Get_uint32() const
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_INT32, "");
		return *((const uint32 *)m_Value);
	}
	const fp4 &Type_Get_fp4() const
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_FP4, "");
		return *((const fp4 *)m_Value);
	}

	const fp2 &Type_Get_fp2() const
	{
		M_ASSERT((!(m_InternalFlags & ERegistryDynamicFlags_Animated)) && m_Type == REGISTRY_TYPE_FP4, "");
		return *((const fp2 *)m_Value);
	}

	int FindIndexInternal(const char* _pKey) const ;

	void CopyValueFromRegistry(const CRegistry& _Reg);
	void Private_CopyValueFromRegistryDynamic(const CRegistry_Dynamic& _Reg);

	bint XRG_ParseDefine(CStr _Line, TArray<CStr> _lDefines);
	bint XRG_ParseDefineW(CStr _Line, TArray<CStr> _lDefines);
	int XRG_Preprocess(const char* _pSrc, int _Len, char* _pDst, TArray<CStr> _lDefines);
	int XRG_Preprocess(const wchar* _pSrc, int _Len, wchar* _pDst, TArray<CStr> _lDefines);
	spCRegistry XRG_AddParseKey(CStr _Key, CStr _Value, CRegistry_ParseContext& _ParseContext, CRegistry_DynamicParseKeyOptions &_KeyOptions);

protected:
//	virtual CRegistry_Dynamic* GetChildDyn(int _iChild);
//	virtual const CRegistry_Dynamic* GetChildDyn(int _iChild) const;

	int MRTC_AddRef();
	int MRTC_DelRef();
	int MRTC_ReferenceCount() const;


public:

	CRegistry_Dynamic();
	~CRegistry_Dynamic();
    	
	void Hash_Invalidate()
	{
		Private_Hash_Invalidate();
	}


	DECLARE_OPERATOR_NEW

	void operator= (const CRegistry_Dynamic& _Reg);

	/// Interface
	virtual void operator= (const CRegistry& _Reg) ;

	spCRegistry Duplicate() const;

	virtual void Clear() ;

	virtual spCRegistry MakeThreadSafe();
	virtual void SetChild(int _iChild, CRegistry*_pReg);

	CRegistry *GetParent() const
	{
		return m_pParent;
	}

	void SetParent(CRegistry *_pParent)
	{
		m_pParent = _pParent;
	}

	bint ValidChild(int _iChild) const;

	virtual int GetNumChildren() const ;
	virtual void SetNumChildren(int _nChildren) ;
	virtual CRegistry* GetChild(int _iChild) ;
	virtual const CRegistry* GetChild(int _iChild) const ;
	virtual const CRegistry* GetChildUnsafe(int _iChild) const ;

	virtual int GetType() const ;
	virtual int GetDimensions() const;	
	virtual void ConvertToType(int _Type, int _nDim = -1);

	// Sort register
	virtual void Sort(bint _bReversedOrder = false, bint _bSortByName = true) ;

	// Search children
	virtual int FindIndex(const char* _pKey) const ;
	virtual int FindIndex(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue = true) const;

	virtual CRegistry* CreateDir(const char* _pPath) ;
	virtual bint DeleteDir(const char* _pPath) ;
	virtual void CopyDir(const CRegistry* _pReg, bint _bRecursive = true) ;
	void CopyValue(const CRegistry* _pReg);

	virtual void AddReg(spCRegistry _spReg) ;

	virtual CRegistry *CreateNewChild() ;

	virtual void DeleteKey(int _iKey) ;

	// Setting
	virtual void SetThisName(const char* _pName) ;

	virtual void SetThisValueConvert(CStr _Value, int _nDim, int _Type);

	virtual void SetThisValue(const char* _pValue) ;
	virtual void SetThisValue(const wchar* _pValue) ;
	virtual void SetThisValue(CStr _Value) ;
	virtual void SetThisValuei(int32 _Value, int _StoreType = REGISTRY_TYPE_INT32) ;
	virtual void SetThisValuef(fp4 _Value, int _StoreType = REGISTRY_TYPE_FP4) ;
	virtual void SetThisValued(const uint8* _pValue, int _Size, bint _bQuick = true) ;
	virtual void SetThisValued(TArray<uint8> _lValue, bint _bReference = true) ;

	virtual void SetThisValuea(int _nDim, const CStr *_Value);
	virtual void SetThisValuead(int _nDim, const TArray<uint8> *_lValue, bint _bReference = true);
	virtual void SetThisValueai(int _nDim, const int32 *_Value, int _StoreType = REGISTRY_TYPE_INT32);
	virtual void SetThisValueaf(int _nDim, const fp4 *_Value, int _StoreType = REGISTRY_TYPE_FP4);


	// Getting value from this

	// Get/Set user flags, user flags are 8-bit
	virtual void SetThisUserFlags(uint32 _Value) ;
	virtual uint32 GetThisUserFlags() const ;

	virtual CStr GetThisName() const ;
	virtual uint32 GetThisNameHash() const;
	virtual const char* GetThisNameUnsafe() const ;

	virtual CStr GetThisValue() const ;
	virtual int32 GetThisValuei() const ;
	virtual fp4 GetThisValuef() const ;
	virtual const TArray<uint8> GetThisValued() const ;
	virtual TArray<uint8> GetThisValued() ;

	virtual void GetThisValuea(int _nDim, CStr *_pDest) const;
	virtual void GetThisValueai(int _nDim, int32 *_pDest) const;
	virtual void GetThisValueaf(int _nDim, fp4 *_pDest) const;
	virtual void GetThisValuead(int _nDim, TArray<uint8> *_pDest) const;



	/////////////////////////////////////////////////////////////////
	// Animation
	////////////////////////////////////////////////////////////////

	virtual void Anim_ThisSetAnimated(bint _bEnable);
	virtual bint Anim_ThisGetAnimated() const;

	virtual void Anim_ThisSetDisableAutoDemote(bint _bEnable);
	virtual bint Anim_ThisGetDisableAutoDemote() const;

	virtual void Anim_ThisSetEnableTimed(bint _bEnable);
	virtual bint Anim_ThisGetEnableTimed() const;

	virtual void Anim_ThisSetFlags(uint32 _Flags);
	virtual uint32 Anim_ThisGetFlags() const;

	virtual void Anim_ThisSetInterpolate(uint32 _InterpolateType, const fp4 *_pParams, int _nParams);
	virtual uint32 Anim_ThisGetInterpolate(fp4 *_pParams, int &_nParams) const;

	virtual bint Anim_ThisIsValidSequenceKeyframe(int _iSec, int _iKF) const;

	virtual int Anim_ThisAddSeq();
	virtual int Anim_ThisInsertSeq(int _iSeqAfter);
	virtual void Anim_ThisDeleteSeq(int _iSeq);
	virtual void Anim_ThisSetNumSeq(int _nSeq);
	virtual int Anim_ThisGetNumSeq() const;

	virtual fp4 Anim_ThisGetSeqLoopStart(int _iSeq) const;
	virtual fp4 Anim_ThisGetSeqLoopEnd(int _iSeq) const;

	virtual fp4 Anim_ThisGetSeqLength(int _iSeq) const;

	virtual void Anim_ThisSetSeqLoopStart(int _iSeq, fp4 _Time);
	virtual void Anim_ThisSetSeqLoopEnd(int _iSeq, fp4 _Time);

	virtual void Anim_ThisGetKF(int _iSeq, fp4 _Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const;
	virtual void Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const;
	virtual void Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, uint32 *_pDeltasCalc, int _nPre, int _nPost) const;

	virtual void Anim_ThisSetNumKF(int _iSeq, int _nKF);
	virtual int Anim_ThisGetNumKF(int _iSeq = 0) const;

	virtual void Anim_ThisDeleteKF(int _iSeq, int _iKF);

	virtual int Anim_ThisSetKFTime(int _iSeq, int _iKF, fp4 _Time) ;
	virtual fp4 Anim_ThisGetKFTime(int _iSeq, int _iKF) const;

	virtual fp4 Anim_ThisGetWrappedTime(const CMTime &_Time, int _iSeq = 0) const;

	// Adds
	virtual int Anim_ThisAddKF(int _iSeq, CStr _Val, fp4 _Time = -1);
	virtual int Anim_ThisAddKFi(int _iSeq, int32 _Val, int _StoreType = REGISTRY_TYPE_INT32, fp4 _Time = -1);
	virtual int Anim_ThisAddKFf(int _iSeq, fp4 _Val, int _StoreType = REGISTRY_TYPE_FP4, fp4 _Time = -1);
	virtual int Anim_ThisAddKFd(int _iSeq, const uint8* _pValue, int _Size, bint _bQuick = true, fp4 _Time = -1);
	virtual int Anim_ThisAddKFd(int _iSeq, TArray<uint8> _lValue, bint _bReference = true, fp4 _Time = -1);

	virtual int Anim_ThisAddKFa(int _iSeq, int _nDim, const CStr *_pVal, fp4 _Time = -1);
	virtual int Anim_ThisAddKFai(int _iSeq, int _nDim, const int32 *_pVal, int _StoreType = REGISTRY_TYPE_INT32, fp4 _Time = -1);
	virtual int Anim_ThisAddKFaf(int _iSeq, int _nDim, const fp4 *_pVal, int _StoreType = REGISTRY_TYPE_FP4, fp4 _Time = -1);
	virtual int Anim_ThisAddKFad(int _iSeq, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true, fp4 _Time = -1);

	// Set
	virtual void Anim_ThisSetKFValueConvert(int _iSeq, int _iKF, CStr _Val, int _nDim, int _StoreType, fp4 _Time);

	virtual void Anim_ThisSetKFValue(int _iSeq, int _iKF, CStr _Val);
	virtual void Anim_ThisSetKFValuei(int _iSeq, int _iKF, int32 _Val, int _StoreType = REGISTRY_TYPE_INT32);
	virtual void Anim_ThisSetKFValuef(int _iSeq, int _iKF, fp4 _Val, int _StoreType = REGISTRY_TYPE_FP4);
	virtual void Anim_ThisSetKFValued(int _iSeq, int _iKF, const uint8* _pValue, int _Size, bint _bQuick = true);
	virtual void Anim_ThisSetKFValued(int _iSeq, int _iKF, TArray<uint8> _lValue, bint _bReference = true);

	virtual void Anim_ThisSetKFValuea(int _iSeq, int _iKF, int _nDim, const CStr *_pVal);
	virtual void Anim_ThisSetKFValueai(int _iSeq, int _iKF, int _nDim, const int32 *_pVal, int _StoreType = REGISTRY_TYPE_INT32);
	virtual void Anim_ThisSetKFValueaf(int _iSeq, int _iKF, int _nDim, const fp4 *_pVal, int _StoreType = REGISTRY_TYPE_FP4);
	virtual void Anim_ThisSetKFValuead(int _iSeq, int _iKF, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true);

	// Get Value
	virtual CStr Anim_ThisGetKFValue(int _iSeq, int _iKF) const;
	virtual int32 Anim_ThisGetKFValuei(int _iSeq, int _iKF) const;
	virtual fp4 Anim_ThisGetKFValuef(int _iSeq, int _iKF) const;
	virtual TArray<uint8> Anim_ThisGetKFValued(int _iSeq, int _iKF) const;

	virtual void Anim_ThisGetKFValuea(int _iSeq, int _iKF, int _nDim, CStr *_pDest) const;
	virtual void Anim_ThisGetKFValueai(int _iSeq, int _iKF, int _nDim, int32 *_pDest) const;
	virtual void Anim_ThisGetKFValueaf(int _iSeq, int _iKF, int _nDim, fp4 *_pDest) const;
	virtual void Anim_ThisGetKFValuead(int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest) const;

	////////////////// IO

	template <typename t_CChar>
	int XRG_ParseImp(t_CChar* _pOrgData, t_CChar* _pData, int _Size, CRegistry_ParseContext& _ParseContext) ;
	
	virtual int XRG_Parse(char* _pOrgData, char* _pData, int _Size, CRegistry_ParseContext& _ParseContext) ;
	virtual int XRG_Parse(wchar* _pOrgData, wchar* _pData, int _Size, CRegistry_ParseContext& _ParseContext) ;
	virtual void XRG_Read(const CStr& _Filename, TArray<CStr> _lDefines, bint _bSlashIsEscape = true) ;	// Unicode detection is automatic.
	virtual void XRG_Read(CCFile *_pFile, CStr _ThisFileName, TArray<CStr> _lDefines, bint _bSlashIsEscape = true) ;	// Unicode detection is automatic.
	virtual void XRG_Read(const CStr& _Filename) ;								// XRG_Read(_FileName, TArray<CStr>::TArray<CStr>(), true)

	virtual void ReadSimple(CCFile* _pFile) ;									// Unicode detection is automatic.
	virtual void ReadSimple(CStr _FileName) ;									// Unicode detection is automatic.
	virtual void ReadRegistryDir(CStr _Dir, CCFile* _pFile) ;
	virtual void ReadRegistryDir(CStr _Dir, CStr _Filename) ;

	// -------------------------------------------------------------------
	// Binary IO
	virtual void Read(CCFile* _pFile, int _Version) ;
	virtual void Write(CCFile* _pFile) const ;

	// Read/Write from data-file entry.
	virtual bint Read(CDataFile* _pDFile, const char* _pEntryName) ;
	virtual void Write(CDataFile* _pDFile, const char* _pEntryName) const ;

	// Read/Write datafile.
	virtual void Read(const char* _pFileName) ;
	virtual void Write(const char* _pFileName) const ;
};

typedef TPtr2<CRegistry_Dynamic, CVirtualRefCount> spCRegistry_Dynamic;


/*class spCRegistry_Dynamic : public TPtr<CRegistry_Dynamic>
{
public:
	operator TPtr<CRegistry>()
	{
		return NULL; //(CRegistry*)m_p;
	};
};*/

/*
TPtr<CRegistry_Dynamic>::operator TPtr<CRegistry>()
{
	return NULL;
};
*/

#endif // _INC_MREGISTRY_DYNAMIC
