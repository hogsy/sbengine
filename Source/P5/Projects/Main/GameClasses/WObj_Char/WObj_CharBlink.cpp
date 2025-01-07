/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_CharBlink.cpp

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2006

	Contents:		CWO_CharBlink

	Comments:		Handles character eye blinks

	History:		
		060330:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharBlink.h"
#include "../WObj_Char.h"

// This file should be moved to Shared

#define CHAR_BLINK_DURATION 0.2f		// time it takes to perform a blink
#define CHAR_BLINK_MINGAP 1.0f			// minimum time between blinks
#define CHAR_BLINK_MAXGAP 6.0f			// maximum        - " - 


CWO_CharBlink::CWO_CharBlink()
	: m_BlinkTime(1000.0f)
{ 
}


CWO_CharBlink& CWO_CharBlink::operator= (const CWO_CharBlink& _x)
{
	m_BlinkTime = _x.m_BlinkTime;
	return *this;
}


void CWO_CharBlink::Update(fp32 _dt)
{
	m_BlinkTime += _dt;

	if (m_BlinkTime > CHAR_BLINK_DURATION)
	{
		fp32 Gap = CHAR_BLINK_MINGAP + Random * (CHAR_BLINK_MAXGAP - CHAR_BLINK_MINGAP);
		m_BlinkTime = -Gap;
		//M_TRACE("Gap: %.3f\n", Gap);
	}
}


void CWO_CharBlink::Apply(fp32 _TimeOffset, bool bDead, fp32* _pFaceData) const
{
	fp32 t = (m_BlinkTime + _TimeOffset) * (1.0f / CHAR_BLINK_DURATION);
	fp32 Value = 0.0f;
	if (bDead)
	{
		// close eyes completely when dead
		Value = 1.0f;
	}
	else if (t >= 0.0f && t <= 1.0f)
	{
		// animate: 0.0 -> 1.0 -> 0.0
		Value = 1.0f - 2.0f * M_Fabs(t - 0.5f); 
	}
	else 
		return; // do nothing

	// Set eyelid muscle group data
	_pFaceData[PLAYER_FACEDATA_REYELID_UP_CLOSE] = Value;
	_pFaceData[PLAYER_FACEDATA_LEYELID_UP_CLOSE] = Value;
	_pFaceData[PLAYER_FACEDATA_REYELID_DOWN_CLOSE] = Value;
	_pFaceData[PLAYER_FACEDATA_LEYELID_DOWN_CLOSE] = Value;

	// kill all "open" muscles
	_pFaceData[PLAYER_FACEDATA_REYELID_UP_OPEN] *= (1.0f - Value);
	_pFaceData[PLAYER_FACEDATA_LEYELID_UP_OPEN] *= (1.0f - Value);
	_pFaceData[PLAYER_FACEDATA_REYELID_DOWN_OPEN] *= (1.0f - Value);
	_pFaceData[PLAYER_FACEDATA_LEYELID_DOWN_OPEN] *= (1.0f - Value);
}
