#ifndef DInc_WFrontEndInfo_h
#define DInc_WFrontEndInfo_h

class CWFrontEnd_Mod;
class CMWnd_FrontEndInfo
{
public:
	CWFrontEnd_Mod* m_pFrontEnd;

	CMWnd_FrontEndInfo()
	{
		m_pFrontEnd = NULL;
	}

	virtual void SetClientGame(CWFrontEnd_Mod* _pCG)
	{
		m_pFrontEnd = _pCG;
	}

	virtual void OnPostSetClientGame()
	{
	}
};

#endif // DInc_WFrontEndInfo_h
