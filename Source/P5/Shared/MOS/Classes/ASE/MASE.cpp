#include "PCH.h"

#include "MASE.h"
#include "../../XWC/XWC_Win32.h"

static int g_ASESize = 0;

int CParamNode::GetChildIndex(char* _pName)
{
	for(int i = 0; i < m_lspChildren.Len(); i++)
		if (m_lspChildren[i]->m_Key.Compare(_pName) == 0) return i;
	return -1;
}

CParamNode* CParamNode::GetChild(char* _pName)
{
	for(int i = 0; i < m_lspChildren.Len(); i++)
		if (m_lspChildren[i]->m_Key.Compare(_pName) == 0) return m_lspChildren[i];
	return NULL;
}


CStr CParamNode::ASE_GetLine(char* _pData, int _Size, int& _Pos)
{
	int PosStart = _Pos;
	for(; _Pos < _Size; _Pos++)
	{
		if (_pData[_Pos] == char(13))
		{
			int PosEnd = _Pos;
			while ((_Pos < _Size) && ((_pData[_Pos] == 13) || (_pData[_Pos] == 10))) _Pos++;
			CStr Ret = CStr(' ', PosEnd - PosStart);
			memcpy((char*) Ret, &_pData[PosStart], PosEnd - PosStart);
			return Ret;
		}
	}
	return "";
}

CStr CParamNode::ASE_GetWord(char* _pData, int _Size, int& _Pos)
{
	while ((_Pos < _Size) && CStr::IsWhiteSpace(_pData[_Pos])) _Pos++;
	if (_Pos >= _Size) return "";

	int PosStart = _Pos;
	while(1)
	{
		if (_pData[_Pos] == '"')
		{
//			PosStart++;
			_Pos++;
			PosStart = _Pos;
			while ((_Pos < _Size) && (_pData[_Pos] != '"')) _Pos++;

			CStr Ret = CStr(' ', _Pos - PosStart);
			memcpy((char*) Ret, &_pData[PosStart], _Pos - PosStart);
			_Pos++;
			return Ret;
		}
		else
			if (CStr::IsWhiteSpace(_pData[_Pos])) break;

		_Pos++;
		if (_Pos >= _Size) break;
	}

	CStr Ret = CStr(' ', _Pos - PosStart);
	memcpy((char*) Ret, &_pData[PosStart], _Pos - PosStart);
	return Ret;
}

int CParamNode::ASE_Parse(char* _pData, int _Size)
{
	m_lspChildren.SetGrow(48);
	spCParamNode spNew;


	int Pos = 0;
	MRTC_SetProgress(1.0f - fp32(_Size - Pos) / fp32(g_ASESize));

	while(Pos < _Size)
	{
		CStr Word = ASE_GetWord(_pData, _Size, Pos);
//		Ln.Trim();
//		LogFile(Word);
		if (Word == "}") break;
		if (Word == "{")
		{
			Pos += spNew->ASE_Parse(&_pData[Pos], _Size - Pos);
		}
		else if (Word[0] == '*')
		{
			if (spNew != NULL) m_lspChildren.Add(spNew);

			spNew = MNew(CParamNode);
			if (!spNew) MemError("ASE_Parse");

			spNew->m_Key = Word.Copy(1, 1000);
			spNew->m_Key.MakeUpperCase();
		}
		else
		{
			if (spNew != NULL) spNew->m_lParams.Add(Word);
		}
	}
	if (spNew != NULL)
	{
		m_lspChildren.Add(spNew);
		m_lspChildren.SetGrow(Max(1, m_lspChildren.Len() / 2));
	}

	return Pos;
}

/*
int CParamNode::ASE_Parse(char* _pData, int _Size)
{
	m_lspChildren.SetGrow(48);
	int Pos = 0;
	while(Pos < _Size)
	{
		CStr Ln = ASE_GetLine(_pData, _Size, Pos).LTrim().RTrim();
//		Ln.Trim();
		LogFile(Ln);
		if (Ln == "}") return Pos;

		spCParamNode spNew = DNew(CParamNode) CParamNode;
		if (!spNew) MemError("ASE_Parse");

		int iKeyWord = Ln.Find("*");
		if (iKeyWord < 0) continue;



//	LogFile(CStrF("%d, %s", Ln.FindOneOf(CStr::GetWhiteSpaces()), CStr::GetWhiteSpaces()));
//	LogFile(CStrF("%d", Ln.FindOneOf(" ")));
		CStr kw = Ln.GetStrMSep(CStr::ms_WhiteSpaces);
//	LogFile("KeyWord: " + kw);
		kw.MakeUpperCase();

	LogFile("KeyWord: " + kw);
		spNew->m_Key = kw;
		while(Ln.Len())
		{
			const char snuff[] = { 34, 0 };
			CStr s;
			if (Ln[0] == '"')
				s = Ln.GetBounded(CStr::ms_CitationMark);
			else
				s = Ln.GetStrMSep(CStr::ms_WhiteSpaces);

			if (s.Len()) 
			{
	LogFile("Param: " + s);
				spNew->m_lParams.Add(s);
			}
		}

		m_lspChildren.Add(spNew);
		if (spNew->m_lParams.Len())
			if (spNew->m_lParams[0] == "{")
			{
				spNew->m_lParams.Del(0);
				Pos += spNew->ASE_Parse(&_pData[Pos], _Size - Pos);
			}
	}

	return Pos;
}
*/

void CParamNode::ASE_LogDump(int _Level)
{
	if (_Level == 0)
	{
		LogFile("-------------------------------------------------------------------");
		LogFile("ASE_LOGDUMP");
		LogFile("-------------------------------------------------------------------");
	}

	CStr s = CStr(' ', _Level*4);
	s += CStrF("%-24s", (char*) m_Key);

	CStr s2 = CStrF("%-40s", (char*)s);

	{ 
		for(int i = 0; i < m_lParams.Len(); i++)
		{
			int l1 = m_lParams[i].Len();
			int l2 = (l1 + 7) & 0xfffffff8;

			s2 += ", " + m_lParams[i] + CStr(' ', l2-l1); 
		}
	}
	LogFile(s2);

	{ for(int i = 0; i < m_lspChildren.Len(); i++)
		m_lspChildren[i]->ASE_LogDump(_Level + 1); }
}

void CParamNode::ReadASE(CStr& _Filename)
{
LogFile("ReadASE...");
	CCFile File;
	File.Open(_Filename, CFILE_READ);
	int Size = File.Length();
LogFile(CStrF("ReadASE...  %d", Size));
	char* pData = DNew(char) char[Size];
	if (!pData) MemError("ReadASE");
	try
	{
		File.Read(pData, Size);
		File.Close();
		g_ASESize = Size;
		ASE_Parse(pData, Size);
	}
	catch(CCException)
	{
		delete[] pData;
		throw;
	}
	delete[] pData;
LogFile("ReadASE done. ");
}


