#ifndef __WOBJ_EMITTER_H
#define __WOBJ_EMITTER_H


#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"
#include "../WObj_Messages.h"
#include "../WObj_AI/AI_Auxiliary.h"

enum {
	OBJMSG_EMITTER_EMITWITHVELOCITY = OBJMSGBASE_EMITTER,
	OBJMSG_EMITTER_EMIT,		//Return number of emitted objects

	//Emitter coordinator ogier mesages, do not change these values
	OBJMSG_EMITTERCOOORDINATOR_RESET = 0x5590,	//Clear pending/scheduled emissions.
	OBJMSG_EMITTERCOOORDINATOR_DELAY = 0x5591,	//Change delay

};


//-------------------------------------------------------------------
// Spell Emitter
//-------------------------------------------------------------------

class CWObject_Emitter : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	TArray<CStr> m_lSpawn;
	fp32 m_Freq;
	fp32 m_Velocity;
	fp32 m_Distortion;
	int32 m_MinDuration;

	//These registry keys will be given to any emitted object for evaluation
	TArray<spCRegistry> m_lspEmiteeRegs;

public:
	//Try to emit objects and return the number of emissions
	virtual int Emit(fp32 _Velocity, int _iSender);

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	//Save duration since last emission
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
};




/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:				Emitter that can be temporarily deactivated

Comments:			If given an impulse 0, emitter will not emit
					during a set time delay						
\*____________________________________________________________________*/
class CWObject_Emitter_Suspendable : public CWObject_Emitter
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	//Suspension time
	int m_SuspensionTime;

	//Activation tick
	int m_ActivationTick;	

public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
	virtual int Emit(fp32 _Velocity, int _iSender);
};




/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:				Coordinator of emitters

Comments:			Coordinator will cause the given number of 
					randomly chosen active emitters to emit on an 
					impulse (i.e. impulse 1 -> on emission) after 
					it's set delay. All "emission-requests" are 
					queued until there are available emitters. 
					Note that one emission will emit several 
					objects if the emitter object used has a 
					quantity higher than one.
\*____________________________________________________________________*/
class CWObject_EmitterCoordinator : public CWObject 
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	//Helper class
	class CScheduledEmission
	{
	public:
		int32 m_Tick;
		int8 m_nEmissions;
		CScheduledEmission(int _Tick = 0, int _nEmissions = 0)
		{
			m_Tick = _Tick;
			m_nEmissions = _nEmissions;
		};
		bool operator==(const CScheduledEmission& _Compare)
		{
			return (m_Tick == _Compare.m_Tick) && (m_nEmissions == _Compare.m_nEmissions);
		}
	};

	//Names (to be saved temporarily) and object IDs of coordinated emitters
	TArray<CStr> m_lEmitterNamesFromKey;
	TArray<int16> m_lEmitters;

	//Delay between impulse receival and emission
	int m_EmissionDelay;

	//Tick to emit and number of emissions that are scheduled. Should be a queue really...
	TSimpleDynamicList<CScheduledEmission> m_lScheduledEmissions;

	//Number of emissions that should be performed as soon as possible
	int8 m_nPendingEmissions;

	//When paused, don't perform any emissions but update scheduled emissions as usual
	bool m_bPaused;

	//Perform emission, or fail if no more emissions can be performed right now
	bool PerformEmission();

public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnSpawnWorld();
	virtual aint OnMessage(const CWObject_Message& _Msg);

	//Make scheduled emissions that are due pending emissions and perform as many 
	//pending emissions as possible
	virtual void OnRefresh();

	//Save scheduled emissions etc
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
};


#endif //__WOBJ_EMITTER_H
