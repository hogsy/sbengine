// -------------------------------------------------------------------
//  CStr
// -------------------------------------------------------------------

int CStr::CStrData::MRTC_AddRef()
{
	MAUTOSTRIP(CStr_CStrData_MRTC_AddRef, 0);
	M_LOCK(*(MRTC_GOM()->m_pGlobalStrLock));
	int nRet = f_MRTC_ReferenceCount() + 1;
	f_MRTC_ReferenceCount(nRet);
	return nRet;
}
int CStr::CStrData::MRTC_DelRef()
{
	MAUTOSTRIP(CStr_CStrData_MRTC_DelRef, 0);
	M_LOCK(*(MRTC_GOM()->m_pGlobalStrLock));
	int nRet = f_MRTC_ReferenceCount() - 1;
	f_MRTC_ReferenceCount(nRet);
	return nRet;
}

int CStr::CStrData::MRTC_ReferenceCount() const 
{
	MAUTOSTRIP(CStr_CStrData_MRTC_ReferenceCount, 0);
	M_LOCK(*(MRTC_GOM()->m_pGlobalStrLock));
	return f_MRTC_ReferenceCount();
};

void CStr::CStrData::MRTC_Delete()
{
	MAUTOSTRIP(CStr_CStrData_MRTC_Delete, MAUTOSTRIP_VOID);
	delete this;
}

mint CStr::CStrData::GetNumAlloc() const
{
	MAUTOSTRIP(CStr_CStrData_GetNumAlloc, 0);
	if (f_bIsAllocated())
	{
		return (f_bIsUnicode()) ? 
			(MRTC_GetMemoryManager()->MemorySize(this) - sizeof(*this)) >> 1 : 
			MRTC_GetMemoryManager()->MemorySize(this) - sizeof(*this);
	}
	else
	{
		if (IsUnicode())
		{
			return wcslen((const wchar*)(this+1))+1;
		}
		else
		{
			return strlen((const char*)(this+1))+1;
		}
	}
}

//#ifdef PLATFORM_WIN_PC
#define OPTIMIZE_CSTR_FOR_SIZE
//#endif

CStr::CStrData* CStr::CStrData::Create(int _Len, int _Format)
{
	MAUTOSTRIP(CStr_CStrData_Create, NULL);
	if (_Format == CSTR_FMT_UNICODE)
	{
		return CreateW(_Len);
	}

	if (!_Len) return NULL;
//	MSCOPESHORT(AllocStr);

	int Size = sizeof(CStrData) + _Len*sizeof(char);

	CStrData* pStrData = (CStrData*) DNew(uint8) uint8[Size];
	if (!pStrData) Error_static("CStrData::Create", "Out of memory.");

	::new ((void*)pStrData) CStrData;	// Construct using placement new


	pStrData->f_bIsUnicode(0);
	pStrData->f_bIsAllocated(1);
	if (_Len)
	{
		pStrData->f_bIsEmpty(0);
		pStrData->Str()[0] = 0;
	}
	else
		pStrData->f_bIsEmpty(1);

	return pStrData;
}

CStr::CStrData* CStr::CStrData::CreateW(int _Len)
{
	MAUTOSTRIP(CStr_CStrData_CreateW, NULL);
	if (!_Len) return NULL;

	int Size = sizeof(CStrData) + _Len*sizeof(wchar);
//	MSCOPESHORT(AllocStrW);

	CStrData* pStrData = (CStrData*) DNew(uint8) uint8[Size];
	if (!pStrData) Error_static("CStrData::Create", "Out of memory.");

	::new ((void*)pStrData) CStrData;	// Construct using placement new


	pStrData->f_bIsUnicode(1);
	pStrData->f_bIsAllocated(1);
	if (_Len)
	{
		pStrData->f_bIsEmpty(0);
		pStrData->StrW()[0] = 0;
	}
	else
		pStrData->f_bIsEmpty(1);

	return pStrData;
}

mint CStr::CStrData::Len() const
{
	MAUTOSTRIP(CStr_CStrData_Len, 0);
	if (IsEmpty())
		return 0;

	if (IsUnicode())
	{
		return wcslen(StrW());
	}
	else
	{
		return strlen(Str());
	}
}

char CStr::operator[](int i) const
{
	MAUTOSTRIP(CStr_operator_index, 0);
	int l = Len();
	if (i < 0 || i >= l) return 0;

	if (IsAnsi())
		return m_spD->Str()[i];
	else if (IsUnicode())
		return m_spD->StrW()[i];
	else
		return 0;
};

CStr::operator char* ()
{
	MAUTOSTRIP(CStr_operator_pchar, NULL);
	if (!m_spD) return NULL;
	return(m_spD->Str());
};

CStr::operator const char* () const
{
	MAUTOSTRIP(CStr_operator_const_pchar, NULL);
	if (!m_spD) return NULL;
	return(m_spD->Str());
};

char* CStr::GetStr()
{
	MAUTOSTRIP(CStr_GetStr, NULL);
	if (!m_spD) return NULL;
	return(m_spD->Str());
}

const char* CStr::GetStr() const
{
	MAUTOSTRIP(CStr_GetStr_2, NULL);
	if (!m_spD) return NULL;
	return m_spD->Str();
}

static char g_NullStr[] = { 0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55 };

const char* CStr::Str() const
{
	MAUTOSTRIP(CStr_Str, NULL);
	if (!m_spD)
		return &g_NullStr[8];
	if (!m_spD->Str())
		return &g_NullStr[8];
	return m_spD->Str();
}

// -------------------------------------------------------------------
CStr::operator wchar* ()
{
	MAUTOSTRIP(CStr_operator_pwchar, NULL);
	if (!m_spD) return NULL;
	return(m_spD->StrW());
};

CStr::operator const wchar* () const
{
	MAUTOSTRIP(CStr_operator_const_pwchar, NULL);
	if (!m_spD) return NULL;
	return(m_spD->StrW());
};

wchar* CStr::GetStrW()
{
	MAUTOSTRIP(CStr_GetStrW, NULL);
	if (!m_spD) return NULL;
	return(m_spD->StrW());
}

const wchar* CStr::GetStrW() const
{
	MAUTOSTRIP(CStr_GetStrW_2, NULL);
	if (!m_spD) return NULL;
	return m_spD->StrW();
}

static wchar g_NullStrW[] = { 0x5555,0x5555,0x5555,0x5555,0x5555,0x5555,0x5555,0x5555,0,0x5555,0x5555,0x5555,0x5555,0x5555,0x5555,0x5555,0x5555 };

const void* CStr::StrData() const
{
	MAUTOSTRIP(CStr_StrW, NULL);
	if (IsAnsi())
	{
		if (!m_spD)
			return &g_NullStr[8];
		if (!m_spD->StrData())
			return &g_NullStr[8];
		return m_spD->StrData();
	}
	else
	{
		if (!m_spD)
			return &g_NullStrW[8];
		if (!m_spD->StrData())
			return &g_NullStrW[8];
		return m_spD->StrData();
	}
}

const wchar* CStr::StrW() const
{
	MAUTOSTRIP(CStr_StrW, NULL);
	if (!m_spD)
		return &g_NullStrW[8];
	if (!m_spD->StrW())
		return &g_NullStrW[8];
	return m_spD->StrW();
}

// -------------------------------------------------------------------
void* CStr::StrAny(int _Pos)
{
	MAUTOSTRIP(CStr_StrAny, NULL);
	return (m_spD != NULL) ? m_spD->StrAny(_Pos) : NULL;
}

const void* CStr::StrAny(int _Pos) const
{
	MAUTOSTRIP(CStr_StrAny_2, NULL);
	return (m_spD != NULL) ? m_spD->StrAny(_Pos) : NULL;
}

void CStr::SetChar(int _Pos, int _Char)
{
	MAUTOSTRIP(CStr_SetChar, MAUTOSTRIP_VOID);
	int l = Len();
	if (_Pos < 0 || _Pos >= l) return;

	MakeUnique();

	if (IsAnsi() && GetStr())
		GetStr()[_Pos] = _Char;
	else if (IsUnicode() && GetStrW())
		GetStrW()[_Pos] = _Char;
}

// -------------------------------------------------------------------
int CStr::IsAnsi() const
{
	MAUTOSTRIP(CStr_IsAnsi, 0);
	if (!m_spD) return true;	// An uncreated string is considered to be both Ansi AND unicode.
	return m_spD->IsAnsi();
}

int CStr::IsUnicode() const
{
	MAUTOSTRIP(CStr_IsUnicode, 0);
	if (!m_spD) return true;	// An uncreated string is considered to be both Ansi AND unicode.
	return m_spD->IsUnicode();
}

int CStr::IsPureAnsi() const
{
	MAUTOSTRIP(CStr_IsPureAnsi, 0);
	if (IsAnsi())
		return true;
	else
	{
		const wchar* pStr = StrW();
		while(*pStr != 0)
		{
			if ((uint32)*pStr > 255)
				return false;
			pStr++;
		}

		return true;
	}
}


CStr CStr::Ansi() const
{
	MAUTOSTRIP(CStr_Ansi, CStr());
	if (!m_spD) return *this;

	if (IsAnsi())
		return *this;
	else
	{
		CStr Ret;

		int l = Len();
		spCStrData spD = CStrData::Create(l+1);

		char* pD = spD->Str();
		wchar* pS = m_spD->StrW();

		if (pD && pS)
		{
			mfsncpy(pD, CSTR_FMT_ANSI, pS, CSTR_FMT_UNICODE, l);
			pD[l] = 0;
		}

		Ret.m_spD = spD;
		return Ret;
	}
}

CStr CStr::Unicode() const
{
	MAUTOSTRIP(CStr_Unicode, CStr());
	if (!m_spD) return *this;

	if (IsUnicode())
		return *this;
	else
	{
		CStr Ret;
		int l = Len();
		spCStrData spD = CStrData::CreateW(l+1);

		wchar* pD = spD->StrW();
		char* pS = m_spD->Str();

		if (pD && pS)
		{
			mfsncpy(pD, CSTR_FMT_UNICODE, pS, CSTR_FMT_ANSI, l);
			pD[l] = 0;
		}

		Ret.m_spD = spD;
		return Ret;
	}
}

// -------------------------------------------------------------------
CStr::spCStrData CStr::CreateUniqueStr(int _Len, CStrData* _pCurrent)
{
	MAUTOSTRIP(CStr_CreateUniqueStr, NULL);

	if (_pCurrent != NULL && 
		_pCurrent->IsAnsi() &&
		_pCurrent->MRTC_ReferenceCount() == 1
		)
	{
		const int NumAlloc = _pCurrent->GetNumAlloc();
		if( _Len <= NumAlloc
#ifdef OPTIMIZE_CSTR_FOR_SIZE
			&& NumAlloc - _Len < 4
#else
			&& NumAlloc - _Len < 64
#endif
		   )
		{
			return _pCurrent;
		}
	}

	return CStrData::Create(_Len);
}

CStr::spCStrData CStr::CreateUniqueStrW(int _Len, CStrData* _pCurrent)
{
	MAUTOSTRIP(CStr_CreateUniqueStrW, NULL);

	if (_pCurrent != NULL && 
		_pCurrent->IsUnicode() &&
		_pCurrent->MRTC_ReferenceCount() == 1
		)
	{
		const int NumAlloc = _pCurrent->GetNumAlloc();
		if( _Len <= NumAlloc
#ifdef OPTIMIZE_CSTR_FOR_SIZE
			&& NumAlloc - _Len < 2
#else
			&& NumAlloc - _Len < 64
#endif
			)
		{
			return _pCurrent;
		}
	}

	return CStrData::CreateW(_Len);
}

void CStr::Clear()
{
	MAUTOSTRIP(CStr_Clear, MAUTOSTRIP_VOID);
	m_spD = NULL;
}

int CStr::Len() const
{
	MAUTOSTRIP(CStr_Len, 0);
	if (!m_spD)
		return 0;
	else
		return m_spD->Len();
};

void CStr::Capture(const char* _pStr)
{
	MAUTOSTRIP(CStr_Capture, MAUTOSTRIP_VOID);
	int Len = StrLen(_pStr);
	Capture(_pStr, Len);
}

void CStr::Capture(const char* _pStr, int _Len)
{
	MAUTOSTRIP(CStr_Capture_2, MAUTOSTRIP_VOID);
	if (!_Len)
	{
		Clear();
		return;
	}
	if (!_pStr) Error("Capture", "NULL pointer but non-zero length");

	spCStrData spD = CreateUniqueStr(_Len+1, m_spD);
	memcpy(spD->Str(), _pStr, _Len);
	spD->Str()[_Len] = 0;
	m_spD = spD;
}

char *CStr::GetBuffer(int _Len)
{
	MAUTOSTRIP(CStr_GetBuffer, NULL);
	spCStrData spD = CreateUniqueStr(_Len+1, m_spD);
	spD->Str()[_Len] = 0;
	m_spD = spD;

	return spD->Str();
}

wchar *CStr::GetBufferW(int _Len)
{
	MAUTOSTRIP(CStr_GetBuffer, NULL);
	spCStrData spD = CreateUniqueStrW(_Len+1, m_spD);
	spD->StrW()[_Len] = 0;
	m_spD = spD;

	return spD->StrW();
}



void CStr::CaptureFormated(const char* x, ...)
{
	MAUTOSTRIP(CStr_CaptureFormated, MAUTOSTRIP_VOID);
	char buffer[4096];

	if (x)
	{
		va_list arg;
		va_start(arg,x);
		vsnprintf((char*) buffer, 4095, x, arg); 
		buffer[4095] = 0;
	}
	else
		buffer[0] = 0;

	Capture(buffer);
}

void CStr::Capture(const wchar* _pStr)
{
	MAUTOSTRIP(CStr_Capture_3, MAUTOSTRIP_VOID);
	int Len = StrLen(_pStr);
	Capture(_pStr, Len);
}

void CStr::Capture(const wchar* _pStr, int _Len)
{
	MAUTOSTRIP(CStr_Capture_4, MAUTOSTRIP_VOID);
	if (!_Len)
	{
		Clear();
		return;
	}
	if (!_pStr) Error("Capture", "NULL pointer but non-zero length");

	spCStrData spD = CreateUniqueStrW(_Len+1, m_spD);
	memcpy(spD->StrW(), _pStr, _Len*sizeof(wchar));
	spD->StrW()[_Len] = 0;
	m_spD = spD;

	Str();
	StrW();
}

void CStr::CaptureFormated(const wchar* x, ...)
{
	MAUTOSTRIP(CStr_CaptureFormated_2, MAUTOSTRIP_VOID);
	wchar buffer[4096];

	if (x)
	{
		va_list arg;
		va_start(arg,x);
#if defined(COMPILER_GNU)
		vswprintf((wchar*) buffer, 4095, x, arg);		
#elif defined(COMPILER_CODEWARRIOR)
		std::vswprintf((wchar*) buffer, 4095, x, arg); 
#elif defined(PLATFORM_WIN32)
		::vswprintf((wchar_t*) buffer, (const wchar_t*) x, arg); 
#else
		::vswprintf((wchar*) buffer, x, arg); 
#endif
		buffer[4095] = 0;
	}
	else
		buffer[0] = 0;

	Capture(buffer);
}

void CStr::Capture(const void* _pStr, int _Len, int _Fmt)
{
	MAUTOSTRIP(CStr_Capture_5, MAUTOSTRIP_VOID);
	if (_Fmt == CSTR_FMT_ANSI)
		Capture((const char*) _pStr, _Len);
	else if (_Fmt == CSTR_FMT_UNICODE)
		Capture((const wchar*) _pStr, _Len);
}

void CStr::MakeUnique()
{
	MAUTOSTRIP(CStr_MakeUnique, MAUTOSTRIP_VOID);
	if (!m_spD) return;
	if (m_spD->MRTC_ReferenceCount() > 1)
	{
		if (m_spD->IsAnsi())
		{
			spCStrData spD = CreateUniqueStr(m_spD->GetNumAlloc(), m_spD);
			if (spD->GetNumAlloc())
				strcpy(spD->Str(), m_spD->Str());
			m_spD = spD;
		}
		else
		{
			spCStrData spD = CreateUniqueStrW(m_spD->GetNumAlloc(), m_spD);
			if (spD->GetNumAlloc())
				wcscpy(spD->StrW(), m_spD->StrW());
			m_spD = spD;
		}
	}
}

int operator==(const CStr &x, const char* s)
{
	MAUTOSTRIP(operator_eq, 0);
	return CStrBase::Compare(x, s) == 0;
}

int operator!=(const CStr &x, const char* s)
{
	MAUTOSTRIP(operator_noteq, 0);
	return CStrBase::Compare(x, s) != 0;
}

int operator==(const CStr &x, const CStr& y)
{
	MAUTOSTRIP(operator_eq_2, 0);
	return CStrBase::Compare(x, y) == 0;
}

int operator!=(const CStr &x, const CStr& y)
{
	MAUTOSTRIP(operator_noteq_2, 0);
	return CStrBase::Compare(x, y) != 0;
}

int operator<(const CStr& x, const CStr& y)
{
	MAUTOSTRIP(operator_lt, 0);
	return CStrBase::Compare(x, y) == -1;
}

int operator>(const CStr& x, const CStr& y)
{
	MAUTOSTRIP(operator_gt, 0);
	return CStrBase::Compare(x, y) == 1;
}

int operator<=(const CStr& x, const CStr& y)
{
	MAUTOSTRIP(operator_lteq, 0);
	return CStrBase::Compare(x, y) <= 0;
}

int operator>=(const CStr& x, const CStr& y)
{
	MAUTOSTRIP(operator_gteq, 0);
	return CStrBase::Compare(x, y) >= 0;
}

// -------------------------------------------------------------------
CStr::CStr()
{
	MAUTOSTRIP(CStr_ctor, MAUTOSTRIP_VOID);
};

CStr::CStr(const char* x)
{
	MAUTOSTRIP(CStr_ctor2, MAUTOSTRIP_VOID);
	Capture(x);
}

CStr::CStr(const wchar* x)
{
	MAUTOSTRIP(CStr_ctor3, MAUTOSTRIP_VOID);
	Capture(x);
}

CStr::CStr(char ch)
{
	MAUTOSTRIP(CStr_ctor_4, MAUTOSTRIP_VOID);
	m_spD = CStrData::Create(2);
	m_spD->Str()[0] = ch;
	m_spD->Str()[1] = 0;
};

CStr::CStr(wchar ch)
{
	MAUTOSTRIP(CStr_ctor_5, MAUTOSTRIP_VOID);
	m_spD = CStrData::CreateW(2);
	m_spD->StrW()[0] = ch;
	m_spD->StrW()[1] = 0;
};

CStr::CStr(char ch, int n)
{
	MAUTOSTRIP(CStr_ctor_6, MAUTOSTRIP_VOID);
	if (n <= 0) return;
	m_spD = CStrData::Create(n+1);
	memset(m_spD->Str(), ch, n);
	m_spD->Str()[n] = 0;
};

CStr::CStr(wchar ch, int n)
{
	MAUTOSTRIP(CStr_ctor_7, MAUTOSTRIP_VOID);
	if (n <= 0) return;
	m_spD = CStrData::CreateW(n+1);
	MemSetW(m_spD->StrW(), ch, n);
	m_spD->StrW()[n] = 0;
};

CStr::CStr(const CStr& x)
{
	MAUTOSTRIP(CStr_ctor_8, MAUTOSTRIP_VOID);
	if (x.m_spD != NULL && !x.m_spD->f_bIsAllocated())
		this->operator=((const CStrBase&)x);
	else
		m_spD = x.m_spD;
};

CStr::CStr(const CStrBase& x)
{
	MAUTOSTRIP(CStr_ctor_9, MAUTOSTRIP_VOID);
	if (x.IsAnsi())
		Capture(x.Str());
	else if (x.IsUnicode())
		Capture(x.StrW());
};

CStr::CStr(CStrData* _pStrData)
{
	MAUTOSTRIP(CStr_ctor_10, MAUTOSTRIP_VOID);
	m_spD = _pStrData;
}

CStr::~CStr()
{
	MAUTOSTRIP(CStr_dtor, MAUTOSTRIP_VOID);
	m_spD = NULL;
};

CStr& CStr::operator=(const char* s)
{
	MAUTOSTRIP(CStr_operator_assign, *this);
	Capture(s);
	return *this;
};

CStr& CStr::operator=(const wchar* s)
{
	MAUTOSTRIP(CStr_operator_assign_2, *this);
	Capture(s);
	return *this;
};

CStr& CStr::operator=(const CStrBase& x)
{
	MAUTOSTRIP(CStr_operator_assign_3, *this);
	if (x.IsAnsi())
		Capture(x.Str());
	else if (x.IsUnicode())
		Capture(x.StrW());

	return *this;
};

CStr& CStr::operator=(const CStr& x)
{
	MAUTOSTRIP(CStr_operator_assign_4, *this);
	if (x.m_spD != NULL && !x.m_spD->f_bIsAllocated())
		this->operator=((const CStrBase&)x);
	else
	{
		if (x.m_spD != NULL)
		{
			if (x.m_spD->MRTC_ReferenceCount() < 8190)
				m_spD = x.m_spD;
			else
				Capture(x.m_spD->StrAny(), x.m_spD->Len(), x.m_spD->GetFmt());
		}
		else
			m_spD = NULL;
	}
	return *this;
};

void CStr::SetCharNoCheck(int _Pos, int _Char)
{
	MAUTOSTRIP(CStr_SetCharNoCheck, MAUTOSTRIP_VOID);
	if (GetFmt() == CSTR_FMT_ANSI)
		GetStr()[_Pos] = _Char;
	else
		GetStrW()[_Pos] = _Char;
}

int CStr::GetCombineFmt(const CStr& _s0, const CStr& _s1)
{
	MAUTOSTRIP(CStr_GetCombineFmt, 0);
	int Fmt = CSTR_FMT_ANSI;
	if (_s0.m_spD != NULL && _s0.m_spD->Len() && _s0.m_spD->IsUnicode()) Fmt = CSTR_FMT_UNICODE;
	if (_s1.m_spD != NULL && _s1.m_spD->Len() && _s1.m_spD->IsUnicode()) Fmt = CSTR_FMT_UNICODE;
	return Fmt;
}

CStr& CStr::operator+=(const CStr& x)
{
	MAUTOSTRIP(CStr_operator_assign_5, *this);
	int l2 = x.Len();
	if (!l2) return *this;
	int l1 = Len();

	spCStrData spD = CStrData::Create(l1+l2+1, GetCombineFmt(*this, x));

	if (m_spD != NULL && m_spD->StrAny()) mfscpy(spD->StrAny(), spD->GetFmt(), m_spD->StrAny(), m_spD->GetFmt());
	if (x.m_spD != NULL && x.m_spD->StrAny()) mfscpy(spD->StrAny(l1), spD->GetFmt(), x.m_spD->StrAny(), x.m_spD->GetFmt());

	m_spD = spD;
	return *this;
};

CStr operator+ (const CStr& _s1, const CStr& _s2)
{
	MAUTOSTRIP(operator_add, CStr());
	CStr Tmp;
	int l1 = _s1.Len();
	int l2 = _s2.Len();

	if(l1 || l2)
	{
		Tmp.m_spD = CStr::CStrData::Create(l1+l2+1, CStr::GetCombineFmt(_s1, _s2));

		if (l1 && _s1.StrAny() && Tmp.StrAny()) 
			CStr::mfscpy(Tmp.StrAny(), Tmp.GetFmt(), _s1.StrAny(), _s1.GetFmt());
		if (l2 && _s2.StrAny() && Tmp.StrAny()) 
			CStr::mfscpy(Tmp.StrAny(l1), Tmp.GetFmt(), _s2.StrAny(), _s2.GetFmt());
	}
	return Tmp;
};

CStr CStr::Copy(int start, int len) const
{
	MAUTOSTRIP(CStr_Copy, CStr());
	CStr Tmp;

	int orglen = Len();
	if (len < 0) len = 0;
	if (start < 0) { len+=start; start = 0; };
	if (len > orglen) len = orglen;
	if (start > orglen) start = orglen;
	if (start+len > orglen) len = orglen-start;

	if (len) Tmp.Capture(StrAny(start), len, GetFmt());
	return Tmp;
};

CStr CStr::Left(int len) const
{
	MAUTOSTRIP(CStr_Left, CStr());
	CStr Tmp;

	int orglen = Len();
	if (len < 0) len = 0;
	if (len > orglen) len = orglen;

	if (len) Tmp.Capture(StrAny(), len, GetFmt());
	return Tmp;
}

CStr CStr::CopyFrom(int start) const
{
	MAUTOSTRIP(CStr_CopyFrom, CStr());
	return Copy(start, 100000000);
}

CStr CStr::Right(int len) const
{
	MAUTOSTRIP(CStr_Right, CStr());
	CStr Tmp;

	int orglen = Len();
	if (len < 0) len = 0;
	if (len > orglen) len = orglen;

	if (len) Tmp.Capture(StrAny(orglen-len), len, GetFmt());
	return Tmp;
}

// Inclusive
CStr CStr::LeftTo(int pos) const
{
	return Left(pos + 1);
}

// Inclusive
CStr CStr::RightFrom(int pos) const
{
	return Right(Len() - pos);
}

CStr CStr::SubStr(int pos, int len) const
{
	return Copy(pos, len);
}

// Inclusive
CStr CStr::DelTo(int pos) const
{
	return Del(0, pos + 1);
}

// Inclusive
CStr CStr::DelFrom(int pos) const
{
	return Del(pos, Len() - pos);
}

CStr CStr::Del(int start, int len) const
{
	MAUTOSTRIP(CStr_Del, CStr());
	CStr Tmp;
	int orglen = Len();

	if (len < 0) len = 0;
	if (start < 0) 
	{ 
		len += start; 
		start = 0; 
	};
	if (len > orglen) len = orglen;
	if (start > orglen) start = orglen;
	if (start+len > orglen) len = orglen-start;

	if (orglen-len > 0)
	{
		Tmp.m_spD = CStrData::Create(orglen-len+1, GetFmt());
		mfsncpy(Tmp.StrAny(0), Tmp.GetFmt(), StrAny(0), GetFmt(), start);
		mfsncpy(Tmp.StrAny(start), Tmp.GetFmt(), StrAny(start+len), GetFmt(), orglen-start-len);
		Tmp.SetCharNoCheck(orglen-len, 0);
	}
	return Tmp;
};

CStr CStr::Insert(int pos, const CStr &x) const
{
	MAUTOSTRIP(CStr_Insert, CStr());
	// TODO: Optimize.

	CStr Tmp = Copy(0,pos) + x + Copy(pos, Len() - pos);
	return Tmp;
};

CStr CStr::Separate(const char* _pSeparator)
{
	MAUTOSTRIP(CStr_Separate, CStr());
	CStr tmp;
	int pos = Find(_pSeparator);

	if (pos < 0)
	{
		tmp = *this;
		pos = Len();
	}
	else
	{
		tmp = Copy(0, pos);
		pos += CStr::StrLen(_pSeparator);
	};

	*this = Del(0, pos);
	return tmp;
};

CStr CStr::UpperCase() const
{
	MAUTOSTRIP(CStr_UpperCase, CStr());
	CStr s = *this;
	s.MakeUpperCase();
	return s;
};

CStr CStr::LowerCase() const
{
	MAUTOSTRIP(CStr_LowerCase, CStr());
	CStr s = *this;
	s.MakeLowerCase();
	return s;
};

CStr CStr::GetFilenameExtenstion() const
{
	MAUTOSTRIP(CStr_GetFilenameExtenstion, CStr());
	int p = FindReverse(".");
	if (p < 0) return "";
	return Copy(p+1, Len());
};

CStr CStr::GetFilename() const
{
	MAUTOSTRIP(CStr_GetFilename, CStr());
	int o = FindReverse("/");
	int p = FindReverse("\\");
	if(o > p) p = o;
	if (p < 0) p = FindReverse(":");
	return Copy(p+1, Len());
}

CStr CStr::GetFilenameNoExt() const
{
	MAUTOSTRIP(CStr_GetFilenameNoExt, CStr());
	int l = FindReverse(".");
	if (l < 0) l = Len();
	int o = FindReverse("/");
	int p = FindReverse("\\");
	if(o > p) p = o;
	if (p < 0) p = FindReverse(":");
	p++;
	l -= p;
	return Copy(p, l);
}

CStr CStr::GetPath() const
{
	MAUTOSTRIP(CStr_GetPath, CStr());
	int o = FindReverse("/");
	int p = FindReverse("\\");
	if(o > p) p = o;
	if (p < 0) return "";
	return Copy(0, p+1);
}

CStr CStr::GetDevice() const
{
	MAUTOSTRIP(CStr_GetDevice, CStr());
	int p = FindReverse(":");
	if (p < 0)
	{
		p = Find("\\\\");
		if (p == 0)
		{
			CStr s(*this);
			s = s.Del(0, 2);
			p = s.Find("\\");
			if (p < 0)
				return "\\\\" + s + "\\";
			else
				return "\\\\" + s.Copy(0, p+1);
		}
		else
			return CStr();
	}
	else
		return Copy(0, p+2);
}

CStr CStr::ConcatRelativePath() const
{
	MAUTOSTRIP(CStr_ConcatRelativePath, CStr());
	CStr NewPath;
	CStr OldPath;
	OldPath = *this;

	char *CurrentSep = strrchr(OldPath, '\\');
	if(!CurrentSep)	CurrentSep = strrchr(OldPath, '/');

	int NumRemove = 0;

	while (CurrentSep)
	{
		++CurrentSep;
		if (CStr(CurrentSep) == "..")
			++NumRemove;
		else
		{
			if (NumRemove)
			{
				--NumRemove;
			}
			else
			{
				--CurrentSep;
				NewPath = CStr(CurrentSep) + NewPath;
				++CurrentSep;
			}
		}
		--CurrentSep;
		*CurrentSep = 0;
		
		CurrentSep = strrchr(OldPath, '\\');
		if(!CurrentSep)	CurrentSep = strrchr(OldPath, '/');
	}

	NewPath = OldPath + NewPath;

	return NewPath;
}

CStr CStr::LTrim() const
{
	MAUTOSTRIP(CStr_LTrim, CStr());
	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return CStr();

		int l = StrLen(p);
		int i = 0;
		while((i < l) && (IsWhiteSpace(p[i]))) i++;
		return Copy(i, l-i);
	}
	else if (IsUnicode())
	{
		const wchar* p = GetStrW();
		if (!p) return CStr();

		int l = StrLen(p);
		int i = 0;
		while((i < l) && (IsWhiteSpace(p[i]))) i++;
		return Copy(i, l-i);
	}
	else
		return CStr();
}

CStr CStr::RTrim() const
{
	MAUTOSTRIP(CStr_RTrim, CStr());
	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return CStr();

		int l = StrLen(p);
		int i = l-1;
		while((i > 1) && (IsWhiteSpace(p[i]))) i--;
		return Copy(0, i+1);
	}
	else if (IsUnicode())
	{
		const wchar* p = GetStrW();
		if (!p) return CStr();

		int l = StrLen(p);
		int i = l-1;
		while((i > 1) && (IsWhiteSpace(p[i]))) i--;
		return Copy(0, i+1);
	}
	else
		return CStr();
}

CStr CStr::GetStrSep(const char* _pSeparator)
{
	MAUTOSTRIP(CStr_GetStrSep, CStr());
	int i = Find(_pSeparator);
	int sl = StrLen(_pSeparator);
	if (i >= 0)
	{
		CStr v = Copy(0, i);

		*this = Del(0, i+sl);
		return v;
	}
	else
	{
		CStr v = *this;
		Clear();
		return v;
	}
}

CStr CStr::GetStrMSep(const char* _pSeparators)
{
	MAUTOSTRIP(CStr_GetStrMSep, CStr());
	int i = FindOneOf(_pSeparators);
	int sl = 1;
	if (i >= 0)
	{
		CStr v;
		v.Capture(GetStr(), i);
		*this = Del(0, i+sl);
		return v;
	}
	else
	{
		CStr v = *this;
		Clear();
		return v;
	}
}

CStr CStr::GetBounded(const char* _pBound)
{
	MAUTOSTRIP(CStr_GetBounded, CStr());
	int p1 = Find(_pBound);
	int BoundLen = StrLen(_pBound);
	CStr _s2 = Del(0, p1 + BoundLen);
	int p2 = _s2.Find(_pBound);
	if (p2 < 0) return CStr("");
	CStr sRet = _s2.Copy(0, p2);
	*this = this->Del(0, p1 + p2 + BoundLen*2);
	return sRet;
}
/*
int CStr::Scan(const char* _pFormat, ...) const
{
	MAUTOSTRIP(CStr_Scan, 0);
	const char* p = GetStr();
	if (!p) return 0;

	va_list arg;
	va_start(arg, _pFormat);

	errno = 0;
	return sscanf(p, _pFormat, arg);
}

int CStr::Scan(const wchar* _pFormat, ...) const
{
	MAUTOSTRIP(CStr_Scan_2, 0);
	const wchar* p = GetStrW();
	if (!p) return 0;

	va_list arg;
	va_start(arg, _pFormat);

	errno = 0;
	return swscanf(p, _pFormat, arg);
}
*/
// -------------------------------------------------------------------
CStr CStr::GetFilteredString(fp8 _f, int _iDecimals)
{
	MAUTOSTRIP(CStr_GetFilteredString, CStr());
	CStr s;
	switch(_iDecimals)
	{
	case 0:	s = CStrF("%.0f", _f); break;
	case 1:	s = CStrF("%.1f", _f); break;
	case 2:	s = CStrF("%.2f", _f); break;
	case 3:	s = CStrF("%.3f", _f); break;
	case 4:	s = CStrF("%.4f", _f); break;
	case 5:	s = CStrF("%.5f", _f); break;
	case 6:	s = CStrF("%.6f", _f); break;
	case 7:	s = CStrF("%.7f", _f); break;
	case 8:	s = CStrF("%.8f", _f); break;
	case 9:	s = CStrF("%.9f", _f); break;
	case 10: s = CStrF("%.10f", _f); break;
	case 11: s = CStrF("%.11f", _f); break;
	case 12: s = CStrF("%.12f", _f); break;
	case 13: s = CStrF("%.13f", _f); break;
	case 14: s = CStrF("%.14f", _f); break;
	case 15: s = CStrF("%.15f", _f); break;
	case 16: s = CStrF("%.16f", _f); break;
	case 17: s = CStrF("%.17f", _f); break;
	case 18: s = CStrF("%.18f", _f); break;
	default: s = CStrF("%f", _f); break;
	}

	int dot = s.Find(".");
	if (dot >= 0)
	{
		int l = s.Len();
		int i;
		for(i = l-1; s[i] == '0'; i--) {};
		if (i == dot) i--;
		s = s.Copy(0, i+1);
	}
	return s;
}

void CStr::ReadFromFile( CStr _Filename )
{
	CCFile File;

	File.Open( _Filename, CFILE_BINARY | CFILE_READ );
	int Length = File.Length();

	GetBuffer( Length + 1 );
	File.Read( GetStr(), Length );
	GetStr()[Length] = 0;

	File.Close();
}

void CStr::WriteToFile( CStr _Filename ) const
{
	CCFile File;

	File.Open( _Filename, CFILE_BINARY | CFILE_WRITE );

	File.Write( Str(), Len() );

	File.Close();
}

void CStr::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CStr_Read, MAUTOSTRIP_VOID);
//	Clear();
//	m_spD = DNew(CStrData) CStrData;
//	if (!m_spD) MemError("Read");

	int32 len;
	_pFile->ReadLE(len);

	int bUnicode = (len & 0x80000000) != 0;
	len &= 0x7fffffff;

	if (len)
	{
		int l = (len+1 + 3) & 0xfffffffc;
//		if (!m_spD->Alloc(l)) MemError("Read");
		if (bUnicode)
		{
			m_spD = CreateUniqueStrW(l, m_spD);
			_pFile->Read(m_spD->StrW(), ((len + 3) & 0xfffffffc) * sizeof(wchar));
			m_spD->StrW()[len] = 0;
		}
		else
		{
			m_spD = CreateUniqueStr(l, m_spD);
			_pFile->Read(m_spD->Str(), (len + 3) & 0xfffffffc);
			m_spD->Str()[len] = 0;
		}
	}
	else
		m_spD = NULL;
}

void CStr::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CStr_Write, MAUTOSTRIP_VOID);
	// dword: Length;
	// char:  String[((Length + 3) & 0xfffffffc)]
	int32 len = Len();
	int32 wrlen = len;
	if (!IsAnsi()) wrlen |= 0x80000000;
	_pFile->WriteLE(wrlen);
	if (len)
	{
		if (!IsAnsi())
		{
			_pFile->Write(m_spD->StrW(), len * sizeof(wchar));
			if (len & 3) _pFile->Write("\0\0\0\0\0\0\0", (4-(len & 3))* sizeof(wchar));
		}
		else
		{
			_pFile->Write(m_spD->Str(), len);
			if (len & 3) _pFile->Write("   ", 4-(len & 3));
		}
	}
}

void CStr::DummyRead(CCFile* _pFile) const
{
	int32 len;
	_pFile->ReadLE(len);
	
	int bUnicode = (len & 0x80000000) != 0;
	len &= 0x7fffffff;
	
	if (len)
	{
		if (bUnicode)
			_pFile->RelSeek(((len + 3) & 0xfffffffc) * sizeof(wchar));
		else
			_pFile->RelSeek((len + 3) & 0xfffffffc);
	}
}

CStr CStrF(const char* x, ...)
{
	MAUTOSTRIP(CStrF, MAUTOSTRIP_VOID);
	char buffer[4096];

	if (x)
	{
		va_list arg;
		va_start(arg,x);
		CStrBase::vsnprintf((char*) &buffer, 4095, x, arg); 
		buffer[4095] = 0;
	}
	else
		buffer[0] = 0;

	CStr Str;
	Str.Capture(buffer, CStr::StrLen(buffer));
	return Str;
}

CStr CStrF(const wchar* x, ...)
{
	MAUTOSTRIP(CStrF_2, MAUTOSTRIP_VOID);
	wchar buffer[4096];

	if (x)
	{
		va_list arg;
		va_start(arg,x);
#if defined(COMPILER_GNU)
		CStrBase::vswprintf((wchar*) &buffer, 4095, x, arg);
#elif defined(COMPILER_CODEWARRIOR)
		std::vswprintf((wchar*) &buffer, 4095, x, arg); 
#elif defined(PLATFORM_WIN32)
		::vswprintf((wchar_t*) buffer, (const wchar_t*) x, arg); 
#else
		::vswprintf((wchar*) &buffer, x, arg); 
#endif
		buffer[4095] = 0;
	}
	else
		buffer[0] = 0;

	CStr Str;
	Str.Capture(buffer, CStr::StrLen(buffer));
	return Str;
}

CFStr CFStrF(const char* Format, ...)
{
	CFStr Str;
	Str.GetStr()[0] = 0;
	if (Format)
	{
		va_list arg;
		va_start(arg,Format);
		char buffer[CFStr::TMaxLen];
		CStrBase::vsnprintf((char*) &buffer, CFStr::TMaxLen, Format, arg); 

		const char *i = buffer;
		const char *maxl = i + CFStr::TMaxLen-1;
		for(; i < maxl; ++i)
			if((*i) == 0) break;

		Str.Capture(buffer, i - buffer);
	}

	return Str;
}

