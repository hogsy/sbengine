// -------------------------------------------------------------------
//  CFStrBase
// -------------------------------------------------------------------
char CFStrBase::operator[](int i) const
{
	int MaxLen = GetMax();
	if (i < 0 || i >= MaxLen) return 0;
	return GetStr()[i];
}

void CFStrBase::Clear()
{
	GetStr()[0] = 0;
}

int CFStrBase::Len() const
{
	return StrLen(GetStr());
}

void CFStrBase::Capture(const char* _pStr)
{
	Capture(_pStr, StrLen(_pStr));
}

void CFStrBase::Capture(const char* _pStr, int _Len)
{
	int Len = Min(GetMax()-1, _Len);
	char* pDest = GetStr();
	memcpy(pDest, _pStr, Len);
	pDest[Len] = 0;
}

void CFStrBase::CaptureFormated(const char* x, ...)
{
	char buffer[4096];

	if (x)
	{
		va_list arg;
		va_start(arg,x);
		vsnprintf((char*) &buffer, 4095, x, arg); 
		buffer[4095] = 0;
	}
	else
		buffer[0] = 0;

	Capture(buffer);
}

CFStrBase::CFStrBase()
{
}

void CFStrBase::operator=(const char* s)
{
	Capture(s);
}

void CFStrBase::operator+=(const char* s)
{
	int ls = StrLen(s);
	if(ls == 0)
		return;
	int l = Len();
	int lnew = Min(GetMax()-1, l + ls);
	char* p = GetStr();
	memcpy(&p[l], s, lnew - l + 1);
	p[lnew] = 0;
}

void CFStrBase::Copy(int start, int len, char* _pTarget) const
{
	int orglen = Len();
	if (len < 0) len = 0;
	if (start < 0) { len+=start; start = 0; };
	if (len > orglen) len = orglen;
	if (start > orglen) start = orglen;
	if (start+len > orglen) len = orglen-start;
	if (len)
	{
		memcpy(_pTarget, &GetStr()[start], len);
		_pTarget[len] = 0;
	}
}

void CFStrBase::Read(CCFile* _pFile)
{
	Clear();
	int32 len;
	_pFile->ReadLE(len);

	if (len)
	{
		if (len & 0x80000000)
			Error("Read", "CFStr cannot be Unicode!");

		if (len >= GetMax())
			Error("Read", CStrF("String to long for static string (%d/%d)", len, GetMax()));
		char Junk[4];
		_pFile->Read(GetStr(), len);
		if (len & 3) _pFile->Read(Junk, 4-(len & 3));
		GetStr()[len] = 0;
	}
}

void CFStrBase::Write(CCFile* _pFile) const
{
	// dword: Length;
	// char:  String[((Length + 3) & 0xfffffffc)]
	int32 len = Len();
	_pFile->WriteLE(len);
	if (len)
	{
		_pFile->Write(GetStr(), len);
		if (len & 3) _pFile->Write("   ", 4-(len & 3));
	}
}

