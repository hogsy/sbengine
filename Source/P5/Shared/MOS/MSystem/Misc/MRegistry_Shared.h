

class CRegistry_Shared_Helpers
{
public:

	template <typename t_CType>
	static void RegistryTypeDestructor(void *_pData, int _nDim)
	{
		for (int i = 0; i < _nDim; ++i)
			((t_CType *)_pData)[i].~t_CType();	
	}

	template <typename t_CType>
	static void RegistryTypeConstructor(void *_pData, int _nDim)
	{
		for (int i = 0; i < _nDim; ++i)
			new((void *)((uint8 *)_pData + sizeof(t_CType) * i)) t_CType();	
	}

	template <typename t_CType>
	static void RegistryTypeConstructorPtr(void *_pData, int _nDim)
	{
		*(((t_CType **)_pData)) = DNew(t_CType) t_CType[_nDim];	
	}

	template <typename t_CType>
	static void RegistryTypeConstructorPtrClear(void *_pData, int _nDim)
	{
		*(((t_CType **)_pData)) = DNew(t_CType) t_CType[_nDim];
		t_CType *pData = *(((t_CType **)_pData)); 
		for (int i = 0; i < _nDim; ++i)
		{
			pData[i] = 0;
		}
	}

	template <typename t_CType>
	static void RegistryTypeDestructorPtr(void *_pData, int _nDim)
	{
		delete [] *((t_CType **)_pData);
	}

	static void RegistryConvertDummy(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
	}

	static void RegistryConvertStrToStr(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		if (_nDimSrc == 1 && _nDimDst > 1)
		{
			CStr::spCStrData &Source = ((CStr::spCStrData *)_pSource)[0];
			if (Source)
			{
				if (Source->IsAnsi())
				{
					const ch8 *pParse = Source->Str();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr::spCStrData &Dest = ((CStr::spCStrData *)_pDest)[i];
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						Dest = Dst.GetStrData();
					}
				}
				else
				{
					const wchar *pParse = Source->StrW();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr::spCStrData &Dest = ((CStr::spCStrData *)_pDest)[i];
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						Dest = Dst.GetStrData();
					}
				}	
			}
			else
			{
				for (int i = 0; i < _nDimDst; ++i)
					(((CStr::spCStrData *)(_pDest)))[i] = CStr::spCStrData();
			}
		}
		else
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				CStr::spCStrData &Source = ((CStr::spCStrData *)_pSource)[i];
				CStr::spCStrData &Dest = ((CStr::spCStrData *)_pDest)[i];
				Dest = Source;
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((CStr::spCStrData *)(_pDest)))[i] = CStr::spCStrData();
		}
	}


	template <typename t_CType>
	static void RegistryTypeConstructorClear(void *_pData, int _nDim)
	{
		for (int i = 0; i < _nDim; ++i)
			((t_CType *)(_pData))[i] = 0;	
	}

	template <typename t_CType>
	static void RegistryConvertDefault(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		for(int i = 0; i < _nDimDst; ++i)
			((t_CType *)(_pDest))[i] = t_CType();
	}

	template <typename t_CType>
	static void RegistryConvertClear(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		for (int i = 0; i < _nDimDst; ++i)
			((t_CType *)(_pDest))[i] = 0;
	}

	static void RegistryConvertDataToStr(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		typedef const TList_Vector<uint8> t_CType;
		int nMin = Min(_nDimSrc, _nDimDst);
		for (int i = 0; i < nMin; ++i)
		{
			t_CType &Source = ((t_CType *)_pSource)[i];

			CStr::spCStrData &dDest = ((CStr::spCStrData *)_pDest)[i];
			CStr Dest = Base64EncodeData(Source.GetBasePtr(), Source.Len());
			dDest = Dest.GetStrData();
		}
	}
	
	static void RegistryConvertStrToData(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		typedef TList_Vector<uint8> t_CType;
		if (_nDimSrc == 1 && _nDimDst > 1)
		{
			CStr::spCStrData &Source = ((CStr::spCStrData *)_pSource)[0];
			CStr Src(Source.p);
			Src = Src.Ansi();

			const ch8 *pParse = Src.Str();
			for (int i = 0; i < _nDimDst; ++i)
			{
				CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
				if (Dst != "")
					(((t_CType *)(_pDest)))[i] = Base64DecodeData(Dst);
				else
					(((t_CType *)(_pDest)))[i].Clear();
			}
		}
		else
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				CStr::spCStrData &Source = ((CStr::spCStrData *)_pSource)[i];
				CStr Src(Source.p);
				Src = Src.Ansi();

				if (Src != "")
				{
					(((t_CType *)(_pDest)))[i] = Base64DecodeData(Src);
				}
				else
				{
					(((t_CType *)(_pDest)))[i].Clear();
				}
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((t_CType *)(_pDest)))[i].Clear();
		}
	}

	template <typename t_CType>
	static void RegistryConvertStrToInt(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		if (_nDimSrc == 1 && _nDimDst > 1)
		{
			CStr::spCStrData &Source = ((CStr::spCStrData *)_pSource)[0];
			if (Source)
			{
				if (Source->IsAnsi())
				{
					const ch8 *pParse = Source->Str();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						(((t_CType *)(_pDest)))[i] = NStr::StrToInt(Dst.Str(),(t_CType)0);
					}
				}
				else
				{
					const wchar *pParse = Source->StrW();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						(((t_CType *)(_pDest)))[i] = NStr::StrToInt(Dst.StrW(),(t_CType)0);
					}
				}
			}
			else
			{
				for (int i = 0; i < _nDimDst; ++i)
					(((t_CType *)(_pDest)))[i] = 0;
			}
		}
		else
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				CStr::spCStrData &Source = ((CStr::spCStrData *)_pSource)[i];
				if (Source)
				{
					if (Source->IsAnsi())
						((t_CType *)(_pDest))[i] = NStr::StrToInt(Source->Str(), (t_CType)0);
					else	
						((t_CType *)(_pDest))[i] = NStr::StrToInt(Source->StrW(), (t_CType)0);
				}
				else
					((t_CType *)(_pDest))[i] = 0;
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((t_CType *)(_pDest)))[i] = 0;

		}
	}

	template <typename t_CType, typename t_CConvertType>
	static void RegistryConvertStrToFloat(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		if (_nDimSrc == 1 && _nDimDst > 1)
		{
			CStr::spCStrData &Source = ((CStr::spCStrData *)_pSource)[0];
			if (Source)
			{
				if (Source->IsAnsi())
				{
					const ch8 *pParse = Source->Str();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						(((t_CType *)(_pDest)))[i] = CMClosestAssign::Assign((t_CType)0, NStr::StrToFloat(Dst.Str(),(fp8)0));
					}
				}
				else
				{
					const wchar *pParse = Source->StrW();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						(((t_CType *)(_pDest)))[i] = CMClosestAssign::Assign((t_CType)0, NStr::StrToFloat(Dst.StrW(),(fp8)0));
					}
				}
			}
			else
			{
				for (int i = 0; i < _nDimDst; ++i)
					(((t_CType *)(_pDest)))[i] = 0;
			}
		}
		else
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				CStr::spCStrData &Source = ((CStr::spCStrData *)_pSource)[i];
				if (Source)
				{
					if (Source->IsAnsi())
						((t_CType *)(_pDest))[i] = CMClosestAssign::Assign((t_CType)0, NStr::StrToFloat(Source->Str(), (fp8)0));
					else
						((t_CType *)(_pDest))[i] = CMClosestAssign::Assign((t_CType)0, NStr::StrToFloat(Source->StrW(), (fp8)0));
				}
				else
					((t_CType *)(_pDest))[i] = 0;
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((t_CType *)(_pDest)))[i] = 0;
		}
	}


	template <typename t_CType>
	static void RegistryConvertIntToStr(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		if (_nDimSrc > 1 && _nDimDst == 1)
		{
			CStr Dest;
			for (int i = 0; i < _nDimSrc; ++i)
			{
				t_CType &Source = ((t_CType *)_pSource)[i];

				if (i == 0)
					Dest = CStrF("%d", (int)(Source));
				else
					Dest += CStrF(",%d", (int)(Source));
			}
			CStr::spCStrData &dDest = ((CStr::spCStrData *)_pDest)[0];
			dDest = Dest.GetStrData();
		}
		else
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				t_CType &Source = ((t_CType *)_pSource)[i];

				CStr::spCStrData &dDest = ((CStr::spCStrData *)_pDest)[i];
				CStr Dest = CStrF("%d", (int)(Source));
				dDest = Dest.GetStrData();
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((t_CType *)(_pDest)))[i] = 0;
		}
	}

	template <typename t_CType>
	static void RegistryConvertIntToStrHex(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		if (_nDimSrc > 1 && _nDimDst == 1)
		{
			CStr Dest;
			for (int i = 0; i < _nDimSrc; ++i)
			{
				t_CType &Source = ((t_CType *)_pSource)[i];

				if (i == 0)
					Dest = CStrF("0x%08x", (int)(Source));
				else
					Dest += CStrF(",0x%08x", (int)(Source));
			}
			CStr::spCStrData &dDest = ((CStr::spCStrData *)_pDest)[0];
			dDest = Dest.GetStrData();
		}
		else
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				t_CType &Source = ((t_CType *)_pSource)[i];

				CStr::spCStrData &dDest = ((CStr::spCStrData *)_pDest)[i];
				CStr Dest = CStrF("0x%08x", (int)(Source));
				dDest = Dest.GetStrData();
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((t_CType *)(_pDest)))[i] = 0;
		}
	}

	template <typename t_CType>
	static void RegistryConvertFloatToStr(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		if (_nDimSrc > 1 && _nDimDst == 1)
		{
			CStr Dest;
			for (int i = 0; i < _nDimSrc; ++i)
			{
				t_CType &Source = ((t_CType *)_pSource)[i];

				if (i == 0)
					Dest = CStrF("%f", (fp4)(Source));
				else
					Dest += CStrF(",%f", (fp4)(Source));
			}
			CStr::spCStrData &dDest = ((CStr::spCStrData *)_pDest)[0];
			dDest = Dest.GetStrData();
		}
		else
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				t_CType &Source = ((t_CType *)_pSource)[i];

				CStr::spCStrData &dDest = ((CStr::spCStrData *)_pDest)[i];
				CStr Dest = CStrF("%f", (fp4)(Source));
				dDest = Dest.GetStrData();
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((t_CType *)(_pDest)))[i] = 0;
		}
	}

	template <typename t_CType0, typename t_CType1>
	static void RegistryConvertCompatible(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		int nMin = Min(_nDimSrc, _nDimDst);
		for (int i = 0; i < nMin; ++i)
		{
			t_CType0 &Source = ((t_CType0 *)_pSource)[i];
			t_CType1 &Dest = ((t_CType1 *)_pDest)[i];
			Dest = (t_CType1)Source;
		}
		for (int i = nMin; i < _nDimDst; ++i)
			(((t_CType1 *)(_pDest)))[i] = t_CType1();
	}

	template <typename t_CType0>
	static void RegistryConvertSame(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		int nMin = Min(_nDimSrc, _nDimDst);
		for (int i = 0; i < nMin; ++i)
		{
			t_CType0 &Source = ((t_CType0 *)_pSource)[i];
			t_CType0 &Dest = ((t_CType0 *)_pDest)[i];
			Dest = Source;
		}
		for (int i = nMin; i < _nDimDst; ++i)
			(((t_CType0 *)(_pDest)))[i] = t_CType0();
	}

	static void RegistryConvertCopyListVector(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
	{
		int nMin = Min(_nDimSrc, _nDimDst);
		for (int i = 0; i < nMin; ++i)
		{
			TList_Vector<uint8> &Source = ((TList_Vector<uint8> *)_pSource)[i];
			TList_Vector<uint8> &Dest = ((TList_Vector<uint8> *)_pDest)[i];

			mint Len = Source.Len();
			Dest.SetLen(Len);
			memcpy(Dest.GetBasePtr(), Source.GetBasePtr(), Len);
		}
		for (int i = nMin; i < _nDimDst; ++i)
			((TList_Vector<uint8> *)_pDest)[i].Clear();
	}


	static int M_CDECL RegistryCompare(const void *_p1, const void *_p2)
	{
		const CRegistry_Dynamic::CPtrKeyHash *pReg1 = (const CRegistry_Dynamic::CPtrKeyHash *)_p1;
		const CRegistry_Dynamic::CPtrKeyHash *pReg2 = (const CRegistry_Dynamic::CPtrKeyHash *)_p2;
		return pReg1->m_spReg->GetThisName().CompareNoCase(pReg2->m_spReg->GetThisName());
	}

	static int M_CDECL RegistryCompiledSimulate_Compare(const void *_p1, const void *_p2)
	{
		const CRegistry_Dynamic::CPtrKeyHash *pReg1 = (const CRegistry_Dynamic::CPtrKeyHash *)_p1;
		const CRegistry_Dynamic::CPtrKeyHash *pReg2 = (const CRegistry_Dynamic::CPtrKeyHash *)_p2;

		if (pReg1->m_Hash > pReg2->m_Hash)
			return 1;
		else if (pReg1->m_Hash < pReg2->m_Hash)
			return -1;
		else
		{
			if (pReg1->m_iHashKey > pReg2->m_iHashKey)
				return 1;
			else if (pReg1->m_iHashKey < pReg2->m_iHashKey)
				return -1;
			else
			{
				M_ASSERT(0, "Should not happen");
				return 0;
			}
		}
	}

	static void QSortHashSwap(CRegistry_Dynamic *_pReg, int _i0, int _i1)
	{
		uint32 Temp = _pReg->m_lChildren[_i0].m_iHashKey;
		_pReg->m_lChildren[_i0].m_iHashKey = _pReg->m_lChildren[_i1].m_iHashKey;
		_pReg->m_lChildren[_i1].m_iHashKey = Temp;
	}
	static void QSortHash_r(CRegistry_Dynamic *_pReg, int _iStart, int _iEnd)
	{
		MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_QSortNCs_r, MAUTOSTRIP_VOID);
		//Check for final case
		if (_iStart >= _iEnd)
		{
			return;
		}

		//Get pivot value
		uint32 Pivot = _pReg->m_lChildren[_pReg->m_lChildren[((_iEnd - _iStart) / 2) + _iStart].m_iHashKey].m_Hash;

		//Loop through list until indices cross
		int iStart = _iStart;
		int iEnd = _iEnd;
		while (iStart <= iEnd)
		{
			//Find the first value that is greater than or equal to the pivot .
			while( (iStart < _iEnd ) && (_pReg->m_lChildren[_pReg->m_lChildren[iStart].m_iHashKey].m_Hash < Pivot) )
			iStart++;

			//Find the last value that is smaller than or equal to the pivot .
			while( (iEnd > _iStart) && (_pReg->m_lChildren[_pReg->m_lChildren[iEnd].m_iHashKey].m_Hash > Pivot) )
			iEnd--;

			//If the indexes have not crossed, swap stuff
			if( iStart <= iEnd ) 
			{
				QSortHashSwap(_pReg, iStart, iEnd);
				iStart++;
				iEnd--;
			}
		}

		//Sort left partition if end index hasn't reached start
		if( _iStart < iEnd )
			QSortHash_r(_pReg, _iStart, iEnd);

		//Sort right partition if start index hasn't reached end
		if( iStart < _iEnd )
			QSortHash_r(_pReg, iStart, _iEnd);
	};

	static fp4 Private_Anim_GetKFDelta(const CRegistry_Dynamic *_pReg, const CRegistry_Dynamic::CAnimationSequence *_pSeq, uint32 _Calc0, uint32 _Calc1, fp4 _SecLen, fp4 _LoopEnd, fp4 _LoopStart)
	{
		fp4 Duration = 0;
		if (_Calc0 & EGetKFFlags_Type)
		{
			uint32 Value0 = _Calc0 & EGetKFFlags_Value;
			uint32 Type0 = _Calc0 >> EGetKFFlags_TypeShift;
			uint32 Value1 = _Calc1 & EGetKFFlags_Value;
			switch(Type0)
			{
			case EGetKFFlags_VSeqEnd:
				Duration = (_SecLen - _pReg->Private_Anim_GetKFTime(_pSeq, Value0)) + (_pReg->Private_Anim_GetKFTime(_pSeq, Value1) - _LoopStart);
				break;
			case EGetKFFlags_VLoopEnd:
				Duration = (_LoopEnd - _pReg->Private_Anim_GetKFTime(_pSeq, Value0)) + (_pReg->Private_Anim_GetKFTime(_pSeq, Value1) - _LoopStart);
				break;
			case EGetKFFlags_VAssign:
				Duration = _pReg->Private_Anim_GetKFTime(_pSeq, Value0);
				break;
			}				
		}
		else
		{
			Duration = _pReg->Private_Anim_GetKFTime(_pSeq, _Calc0) - _pReg->Private_Anim_GetKFTime(_pSeq, _Calc1);
		}
		return Duration;
	}


};
