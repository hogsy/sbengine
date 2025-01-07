/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_CharBlink.h

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2006

	Contents:		CWO_CharBlink

	Comments:		Handles character eye blinks

	History:		
		060330:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharBlink_h__
#define __WObj_CharBlink_h__

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_CharBlink
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_CharBlink
{
	fp32 m_BlinkTime;

public:
	CWO_CharBlink();
	CWO_CharBlink& operator= (const CWO_CharBlink& _x);
	void Update(fp32 _dt);
	void Apply(fp32 _TimeOffset, bool bDead, fp32* _pFaceData) const;
};


#endif // __WObj_CharBlink_h__
