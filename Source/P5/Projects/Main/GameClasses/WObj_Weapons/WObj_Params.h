#ifndef WObj_Params_h
#define WObj_Params_h

//-------------------------------------------------------------------

#include "MRTC.h"
#include "MMath.h"
#include "../WRPG/WRPGDef.h"

class CWorld_Server;

//-------------------------------------------------------------------
//- CCameraShake ----------------------------------------------------
//-------------------------------------------------------------------

class CWO_CameraShake
{

public:

	fp32 m_Range;
	fp32 m_Magnitude;
	fp32 m_Duration;
	fp32 m_Speed;
	bool m_bNoRumble;

	void Clear();
	void Parse(const char* _Str);
	bool IsValid();
	void Send(CVec3Dfp32 _Center, int _iSender, CWorld_Server* _pWServer);

	static void Merge(CWO_CameraShake& _Primary, CWO_CameraShake& _Secondary, CWO_CameraShake& _Result);

};

//-------------------------------------------------------------------

#endif /* WObj_Params_h */
