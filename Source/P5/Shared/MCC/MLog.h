#ifdef PLATFORM_SHINOBI
#include <usrsnasm.h>
#endif

class CLogFile : public CReferenceCount
{
	CStr m_FileName;
	CFile m_File;

public:
	CLogFile(CStr _FileName, bool _bAppend = false);
	~CLogFile();

	virtual void Log(const CStr& _s);
	virtual void SetFilename(const CStr& _s, bool _bAppend = false);
};


CLogFile::CLogFile(CStr _FileName, bool _bAppend)
{
	m_FileName = _FileName;
	m_File.Open(_FileName, CFILE_WRITE | (_bAppend) ? CFILE_APPEND : 0);
}

CLogFile::~CLogFile()
{
	m_File.Close();
}

void CLogFile::Log(const CStr& _s)
{
#ifdef PLATFORM_SHINOBI
	debug_printf((char*)_s);
#else
	m_File.Writeln(_s);
#endif
}

void CLogFile::SetFileName(const CStr& _s, bool _bAppend)
{
	if (_s == m_FileName) return;
	m_File.Close();
	m_FileName = _s;
	m_File.Open(m_FileName, CFILE_WRITE | (_bAppend) ? CFILE_APPEND : 0);
}
