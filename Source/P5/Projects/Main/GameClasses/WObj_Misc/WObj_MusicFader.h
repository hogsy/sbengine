/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Class that plays a 2D sound and fades it after distance.

Author:			Roger Mattsson

Copyright:		2006 Starbreeze Studios AB

Contents:		CWObject_MusicFader
				CWObject_MusicTrackFader

Comments:

History:		
060727:			Created file
\*____________________________________________________________________________________________*/

#ifndef __WObj_MusicFader_h__
#define __WObj_MusicFader_h__

class CWObject_MusicFader : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_MusicFader();

	virtual void	OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void	OnCreate();
	virtual aint	OnMessage(const CWObject_Message& _Msg);
	virtual void	OnRefresh(void);

protected:
	bool		m_bPlaying;
	int			m_iSound;
	fp32		m_Range;
	fp32		m_InnerRange;
};

class CWObject_MusicTrackFader : public CWObject_MusicFader
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_MusicTrackFader();

	virtual void	OnCreate();
	virtual void	OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void	OnRefresh(void);

private:
	int			m_iTrack;
};

#endif

