
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CArrayCore
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef _DEBUG
//#define DASSERT(pos, expr) if (expr) { 	LogFile(CStrF("(CArrayCore::%s) NULL-data, throwing exception now.", (char*) pos)); };
#define DASSERT(pos, expr) if (expr) { 	LogFile(CStrF("(CArrayCore::%s) NULL-data, throwing exception now.", (char*) pos)); Error_static((char*)pos, #expr) };
#else
#define DASSERT(pos, expr)
#endif

#define ALLOCCOUNTWARNING 16

template<>
int CArrayCore<DLVCImp>::m_GlobalFlags = 0;

template<>
void CArrayCore<DLVCImp>::SetGlobalFlags(int _Flg)
{
	m_GlobalFlags = _Flg;
}

template<>
int CArrayCore<DLVCImp>::GetGlobalFlags()
{
	return m_GlobalFlags;
}

template<>
void* CArrayCore<DLVCImp>::GetElement(void* m_pList, int _iElem)
{
	DASSERT("CArrayCore<DLVCImp>::GetElement", !m_pData);
	return &(((char*)m_pList)[_iElem*m_pData->m_ElemSize]);
}

template<>
void CArrayCore<DLVCImp>::CopyElements(const void* _pSrc, void* _pDest, int _nElems) const
{
	DASSERT("CArrayCore<DLVCImp>::CopyElement", !m_pData);
	char* pSrc = (char*) _pSrc;
	char* pDest = (char*) _pDest;
	for(int i = 0; i < _nElems; i++) 
		m_pData->Copy(&(pSrc[i*m_pData->m_ElemSize]), &(pDest[i*m_pData->m_ElemSize]));
}

#ifdef M_BOUNDCHECK
template<>
void CArrayCore<DLVCImp>::RangeCheckError(int _Pos) const
{
//	DASSERT("CArrayCore<DLVCImp>::RangeCheckError", !m_pData);
	{
	}

	if (m_GlobalFlags & 4)
#ifdef CPU_INTELP5
	{
		if (!m_pData)
			LogFile(CStrF("(CArrayCore<DLVCImp>::RangeCheckError) Range check error on NULL array, invoking breakpoint interrupt now. (Index %d)", _Pos));
		else
			LogFile(CStrF("(CArrayCore<DLVCImp>::RangeCheckError) Range check error, invoking breakpoint interrupt now. (Index %d, Len %d, Alloc %d, pList %.8x, Class %s, ElemSize %d", 
				_Pos, m_pData->m_Len, m_pData->m_AllocLen, m_pData->m_pList, (char*) m_pData->ClassName(), m_pData->m_ElemSize));
		M_ASSERT(0, "RangeCheckError");
	}
#else
	#ifdef CPU_SH4
		breakpoint();
	#endif
#endif

#ifdef DEBUG
	if (!m_pData)
		LogFile(CStrF("(CArrayCore<DLVCImp>::RangeCheckError) Range check error on NULL array, invoking breakpoint interrupt now. (Index %d)", _Pos));
	else
		LogFile(CStrF("(CArrayCore<DLVCImp>::RangeCheckError) Range check error, invoking breakpoint interrupt now. (Index %d, Len %d, Alloc %d, pList %.8x, Class %s, ElemSize %d", 
			_Pos, m_pData->m_Len, m_pData->m_AllocLen, m_pData->m_pList, (char*) m_pData->ClassName(), m_pData->m_ElemSize));
	#ifdef CPU_INTELP5
		M_ASSERT(0, "RangeCheckError");
	#else
		#ifdef CPU_SH4
			breakpoint();
		#endif
	#endif
#else
	if (!m_pData)
	{
		LogFile(CStrF("(CArrayCore<DLVCImp>::RangeCheckError) Range check error on NULL array, invoking breakpoint interrupt now. (Index %d)", _Pos));
		Error_static("CArrayCore<DLVCImp>::operator[]", CStrF("Index NULL array. (%d)", _Pos));
	}
	else
	{
		LogFile(CStrF("(CArrayCore<DLVCImp>::RangeCheckError) Range check error, throwing exception now. (Index %d, Len %d, Alloc %d, pList %.8x, Class %s, ElemSize %d", 
			_Pos, m_pData->m_Len, m_pData->m_AllocLen, m_pData->m_pList, (char*) m_pData->ClassName(), m_pData->m_ElemSize));
		Error_static("CArrayCore<DLVCImp>::operator[]", CStrF("Index out of range. (%d, %d)", _Pos, m_pData->m_Len));
	}
#endif
}
#endif

template<>
CArrayCore<DLVCImp>::CArrayCore()
{
	m_pData = NULL;
//	Init();
//	m_pData->m_nRef = 1;
}

template<>
CArrayCore<DLVCImp>::~CArrayCore()
{
	if (m_pData)
	{
		if (!m_pData->m_nRef)
			if (m_pData->m_pList)
			{
				m_pData->FreeScalar(m_pData->m_pList, m_pData->m_AllocLen);
				m_pData->m_pList = NULL;
			}
//			if (m_pData->m_pList) Error_static("CArrayCore<DLVCImp>::~", "m_pList != NULL, cannot delete at this point.");

		if (!m_pData->m_nRef)
		{
			m_pData->Delete();
			m_pData = NULL;
		}
	}
}

template<>
void CArrayCore<DLVCImp>::Clear()
{
	if (m_pData)
	{
		if (m_pData->m_pList) { m_pData->FreeScalar(m_pData->m_pList, m_pData->m_AllocLen); m_pData->m_pList = NULL; }
		m_pData->m_AllocLen = 0;
		m_pData->m_Len = 0;
		if (m_pData->m_nRef <= 1)
		{
			m_pData->Delete();
			m_pData = NULL;
		}
	}
}

template<>
void CArrayCore<DLVCImp>::InsertxBlank(int _Pos, int _nElem, bool _bRespectGrow)
{
	DASSERT("CArrayCore<DLVCImp>::InsertxBlank", !m_pData);

#ifdef M_BOUNDCHECK
	if ((_Pos < 0) || (_Pos > m_pData->m_Len))
		Error_static("CArrayCore<DLVCImp>::InsertxBlank", CStrF("Invalid position: %d/%d", _Pos, m_pData->m_Len));
#endif
	if (!_nElem) return;

	void* pListNew = NULL;
	int AllocLenNew = 0;
	int RequiredSize = m_pData->m_Len + _nElem;
	if (RequiredSize > m_pData->m_AllocLen)
	{
		if(_bRespectGrow)
		{
			int nGrow = m_pData->m_nGrow;
			DASSERT("CArrayCore<DLVCImp>::InsertxBlank", !nGrow);
			// Always respect grow
			while(AllocLenNew < RequiredSize)
				AllocLenNew += nGrow;	
		}
		else
		{
			AllocLenNew = RequiredSize;
			if (!m_pData->m_Len && (_nElem < m_pData->m_nGrow)) AllocLenNew += m_pData->m_nGrow;
		}
		pListNew = m_pData->AllocObjects(AllocLenNew);
		if (!pListNew)
		{
			MemError_static(CFStrF("InsertxBlank (Ins %d, Resize %d -> %d, Grow %d)", _nElem, m_pData->m_Len, AllocLenNew, m_pData->m_nGrow));
		}

		char* pSrc = (char*) m_pData->m_pList;
		char* pDest = (char*) pListNew;
		for(int i = 0; i < _Pos; i++) 
		{
			m_pData->Copy(&(pSrc[i*m_pData->m_ElemSize]), &(pDest[i*m_pData->m_ElemSize]));
//			pSrc = NextElement(pSrc);
//			pDest = NextElement(pDest);
		}

#ifndef M_RTM
		m_pData->m_nAllocCount++;
		if(m_pData->m_nAllocCount >= ALLOCCOUNTWARNING)
		{
			if(!(m_pData->m_nAllocCount % ALLOCCOUNTWARNING))
				M_TRACEALWAYS("Array 0x%.8X has been allocated %d times\r\n", m_pData, m_pData->m_nAllocCount);
		}
#endif
	} 
	else
	{
		pListNew = m_pData->m_pList;
		AllocLenNew = m_pData->m_AllocLen;
	}

	{ 
		for (int i = (m_pData->m_Len - _Pos - 1); i >= 0; i--) 
			m_pData->Copy(&(((char*)m_pData->m_pList)[(_Pos+i) * m_pData->m_ElemSize]),
				&(((char*)pListNew)[(i+_Pos+_nElem) * m_pData->m_ElemSize])); 
	};

	m_pData->m_Len += _nElem;
	if (m_pData->m_pList != pListNew) m_pData->FreeScalar(m_pData->m_pList, m_pData->m_AllocLen);
	m_pData->m_pList = pListNew;
	m_pData->m_AllocLen = AllocLenNew;
}

template<>
void CArrayCore<DLVCImp>::InsertxElements(int _Pos, const  void* _pElems, int _nElem, bool _bInitGrow)
{
	DASSERT("CArrayCore<DLVCImp>::InsertxElements", !m_pData);

#ifdef M_BOUNDCHECK
	if ((_Pos < 0) || (_Pos > m_pData->m_Len))
		Error_static("CArrayCore<DLVCImp>::InsertxElements", CStrF("Invalid position: %d/%d", _Pos, m_pData->m_Len));
#endif
	if (!_nElem) return;

	void* pListNew = NULL;
	int AllocLenNew = 0;
	int RequiredSize = m_pData->m_Len + _nElem;
	if (RequiredSize > m_pData->m_AllocLen)
	{
		if (m_pData->m_AllocLen < _Pos)
			Error_static("CArrayCore<DLVCImp>::InsertxElements", "Internal error. (3)");

		bool bGrow = (m_pData->m_nGrow > 0) && (_bInitGrow || (m_pData->m_Len > 0));
		int nNewLen = RequiredSize;
		if (bGrow)
		{
			AllocLenNew = m_pData->m_AllocLen;
			while (AllocLenNew < nNewLen)
				AllocLenNew += m_pData->m_nGrow;
		}
		else
		{
			AllocLenNew = nNewLen;
		}

		pListNew = m_pData->AllocObjects(AllocLenNew);
		if (!pListNew) MemError_static(CStrF("InsertxElements (Ins %d, Resize %d -> %d, Grow %d)", _nElem, m_pData->m_Len, AllocLenNew, m_pData->m_nGrow));

		if (m_pData->m_AllocLen < _Pos)
			Error_static("CArrayCore<DLVCImp>::InsertxElements", "Internal error. (4)");
//		if (RequiredSize + m_pData->m_nGrow < _Pos)
//			Error_static("CArrayCore<DLVCImp>::InsertxElements", "Internal error. (5)");

		char* pSrc = (char*) m_pData->m_pList;
		char* pDest = (char*) pListNew;
		for(int i = 0; i < _Pos; i++) 
			m_pData->Copy(&(pSrc[i*m_pData->m_ElemSize]), &(pDest[i*m_pData->m_ElemSize]));
#ifndef M_RTM
		m_pData->m_nAllocCount++;
		if(m_pData->m_nAllocCount >= ALLOCCOUNTWARNING)
		{
			if(!(m_pData->m_nAllocCount % ALLOCCOUNTWARNING))
				M_TRACEALWAYS("Array 0x%.8X has been allocated %d times\r\n", m_pData, m_pData->m_nAllocCount);
		}
#endif
	} 
	else
	{
		pListNew = m_pData->m_pList;
		AllocLenNew = m_pData->m_AllocLen;
	}


	if (!pListNew) 
		Error_static("CArrayCore<DLVCImp>::InsertxElements", "Internal error.");

	{ 
		for (int i = (m_pData->m_Len - _Pos - 1); i >= 0; i--) 
			m_pData->Copy(&(((char*)m_pData->m_pList)[(_Pos+i) * m_pData->m_ElemSize]),
				&(((char*)pListNew)[(i+_Pos+_nElem) * m_pData->m_ElemSize])); 
	};

	{
		for (int i = 0; i < _nElem; i++) 
			m_pData->Copy(&(((char*)_pElems)[i * m_pData->m_ElemSize]),
				&(((char*)pListNew)[(i+_Pos) * m_pData->m_ElemSize]));
	};
	m_pData->m_Len += _nElem;
	if (m_pData->m_pList != pListNew) m_pData->FreeScalar(m_pData->m_pList, m_pData->m_AllocLen);
	m_pData->m_pList = pListNew;
	m_pData->m_AllocLen = AllocLenNew;
}

template<>
void CArrayCore<DLVCImp>::Delx(int _Pos, int _nDel, bool _bAlwaysKeepData)
{
	if (!_nDel) return;

	// Valid indata?
#ifdef M_BOUNDCHECK
	if (!m_pData || (_Pos < 0) || (_Pos + _nDel > m_pData->m_Len))
		Error_static("CArrayCore<DLVCImp>::Delx", CStrF("Invalid position: %d/%d", _Pos, Len()));
#endif

	// Check if there will be anything left.
	if (m_pData->m_Len - _nDel == 0)
	{
		if(_bAlwaysKeepData)
		{
			if(m_pData)
			{
				if (m_pData->m_pList) { m_pData->FreeScalar(m_pData->m_pList, m_pData->m_AllocLen); m_pData->m_pList = NULL; }
				m_pData->m_AllocLen = 0;
				m_pData->m_Len = 0;
			}
		}
		else
		Clear();
		return;
	};

	// Realloc?
	void* pListNew = NULL;
	int AllocLenNew = 0;

	if ((m_pData->m_AllocLen-m_pData->m_Len-_nDel) > m_pData->m_nGrow)
	{
		pListNew = m_pData->AllocObjects(m_pData->m_Len-_nDel);
		if (!pListNew) MemError_static(CStrF("Delx (Del %d, Resize %d -> %d, Grow %d)", _nDel, m_pData->m_Len, AllocLenNew, m_pData->m_nGrow));
		AllocLenNew = m_pData->m_Len-_nDel;

		for(int i = 0; i < _Pos; i++)
			m_pData->Copy(&(((char*)m_pData->m_pList)[i*m_pData->m_ElemSize]), &(((char*)pListNew)[i*m_pData->m_ElemSize]));

#ifndef M_RTM
		m_pData->m_nAllocCount++;
		if(m_pData->m_nAllocCount >= ALLOCCOUNTWARNING)
		{
			if(!(m_pData->m_nAllocCount % ALLOCCOUNTWARNING))
				M_TRACEALWAYS("Array 0x%.8X has been allocated %d times\r\n", m_pData, m_pData->m_nAllocCount);
		}
#endif
//		for(int i = 0; i < _Pos; i++) pListNew[i] = p->list[i];
		//Move(p->list, listnew, _Pos*sizeof(T));
	}
	else
	{
		pListNew = m_pData->m_pList;
		AllocLenNew = m_pData->m_AllocLen;
		m_pData->ResetObjects((uint8 *)m_pData->m_pList + m_pData->m_ElemSize * _Pos, _nDel);
	}


	// Flytta
	for (int i = 0; i < (m_pData->m_Len - _Pos - _nDel); i++)
		m_pData->Copy(&(((char*)m_pData->m_pList)[(i+_Pos+_nDel) * m_pData->m_ElemSize]),
		&(((char*)pListNew)[(i+_Pos) * m_pData->m_ElemSize]));
//		pListNew[i+_Pos] = m_pData->m_pList[i+_Pos+_nDel];
	// Move(&(p->list[_Pos+_nDel]), &(listnew[_Pos]), (m_pData->m_Len-_Pos-_nDel)*sizeof(T));
	m_pData->m_Len -= _nDel;

	if (m_pData->m_pList != pListNew) m_pData->FreeScalar(m_pData->m_pList, m_pData->m_AllocLen);
	m_pData->m_pList = pListNew;
	m_pData->m_AllocLen = AllocLenNew;
}

template<>
void CArrayCore<DLVCImp>::Del(int _Pos)
{
	Delx(_Pos, 1);
}

template<>
void CArrayCore<DLVCImp>::Core_SetLen(int _Len)
{
	DASSERT("CArrayCore<DLVCImp>::SetLen", !m_pData);

	if (_Len == Len() &&
		_Len == m_pData->m_AllocLen) return;

	MSCOPESHORT(CArrayCore::Core_SetLen);
	if (_Len != m_pData->m_Len)
	{
		int OldGrow = m_pData->m_nGrow;
		m_pData->m_nGrow = 0;
		M_TRY
		{
			int Diff = _Len - m_pData->m_Len;
			if (Diff == 0) return;
			if (Diff >= 0)
				InsertxBlank(Len(), Diff);
			else
				Delx(Len()+Diff, -Diff, true);
		}
		M_CATCH(
		catch(CCException)
		{
			m_pData->m_nGrow = OldGrow;
			throw;
		}
		)
		m_pData->m_nGrow = OldGrow;
	}
	else
	{
		if (!_Len) return;

		if (_Len != m_pData->m_Len)
			Error_static("CArrayCore<DLVCImp>::SetLen", "Internal error.");

		void* pListNew = m_pData->AllocObjects(_Len);
		if (!pListNew) MemError_static(CStrF("SetLen (Resize %d -> %d, Grow %d)", m_pData->m_Len, _Len, m_pData->m_nGrow));

		for(int i = 0; i < _Len; i++)
			m_pData->Copy(&(((char*)m_pData->m_pList)[i*m_pData->m_ElemSize]), &(((char*)pListNew)[i*m_pData->m_ElemSize]));

		m_pData->FreeScalar(m_pData->m_pList, m_pData->m_AllocLen);
		m_pData->m_pList = pListNew;
		m_pData->m_AllocLen = _Len;

#ifndef M_RTM
		m_pData->m_nAllocCount++;
		if(m_pData->m_nAllocCount >= ALLOCCOUNTWARNING)
		{
			if(!(m_pData->m_nAllocCount % ALLOCCOUNTWARNING))
				M_TRACEALWAYS("Array 0x%.8X has been allocated %d times\r\n", m_pData, m_pData->m_nAllocCount);
		}
#endif
	}

/*	int Diff = _Len - m_pData->m_Len;
	if (Diff == 0) return;
	if (Diff >= 0)
		InsertxBlank(Len(), Diff);
	else
		Delx(Len()+Diff, -Diff);*/
}

template<>
void CArrayCore<DLVCImp>::Core_GrowLen(int _Len)
{
	DASSERT("CArrayCore<DLVCImp>::GrowLen", !m_pData);

	if (_Len == Len()) return;

	if (_Len != m_pData->m_Len)
	{
		M_TRY
		{
			int Diff = _Len - m_pData->m_Len;
			if (Diff == 0) return;
			if (Diff >= 0)
				InsertxBlank(Len(), Diff, true);
			else
				Delx(Len()+Diff, -Diff, true);
		}
		M_CATCH(
		catch(CCException)
		{
			throw;
		}
		)
	}
	else
	{
		if (!_Len) return;

		if (_Len != m_pData->m_Len)
			Error_static("CArrayCore<DLVCImp>::SetLen", "Internal error.");

		void* pListNew = m_pData->AllocObjects(_Len);
		if (!pListNew) MemError_static(CStrF("SetLen (Resize %d -> %d, Grow %d)", m_pData->m_Len, _Len, m_pData->m_nGrow));

		for(int i = 0; i < _Len; i++)
			m_pData->Copy(&(((char*)m_pData->m_pList)[i*m_pData->m_ElemSize]), &(((char*)pListNew)[i*m_pData->m_ElemSize]));

		m_pData->FreeScalar(m_pData->m_pList, m_pData->m_AllocLen);
		m_pData->m_pList = pListNew;
		m_pData->m_AllocLen = _Len;

#ifndef M_RTM
		m_pData->m_nAllocCount++;
		if(m_pData->m_nAllocCount >= ALLOCCOUNTWARNING)
		{
			if(!(m_pData->m_nAllocCount % ALLOCCOUNTWARNING))
				M_TRACEALWAYS("Array 0x%.8X has been allocated %d times\r\n", m_pData, m_pData->m_nAllocCount);
		}
#endif
	}

/*	int Diff = _Len - m_pData->m_Len;
	if (Diff == 0) return;
	if (Diff >= 0)
		InsertxBlank(Len(), Diff);
	else
		Delx(Len()+Diff, -Diff);*/
}

template<>
void CArrayCore<DLVCImp>::Core_QuickSetLen(int _Len)
{
	DASSERT("CArrayCore<DLVCImp>::QuickSetLen", !m_pData);

#ifdef M_BOUNDCHECK
	if (_Len < 0)
		Error_static("CArrayCore<DLVCImp>::QuickSetLen", CStrF("Invalid length %d", _Len));
#endif

	if (_Len > m_pData->m_AllocLen)
		Core_SetLen(_Len);
	else
		m_pData->m_Len = _Len;
}

template<>
void CArrayCore<DLVCImp>::OptimizeMemory()
{
	if (!m_pData) return;
	Core_SetLen(Len());
}

template<>
int CArrayCore<DLVCImp>::ListAllocatedSize() const
{
	DASSERT("CArrayCore<DLVCImp>::ListAllocatedSize", !m_pData);

	if (!m_pData) return 0;
	return m_pData->m_AllocLen*m_pData->m_ElemSize;
}


#undef DASSERT
