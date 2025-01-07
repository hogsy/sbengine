#ifndef _INC_AICORE_BIGALIEN
#define _INC_AICORE_BIGALIEN

#include "../AICore.h"


class CAI_Core_Darkling : public CAI_Core
{
	MRTC_DECLARE;
public:
	//Interface

	enum
	{
		DK_VERY_LOW = 0,
		DK_LOW,
		DK_AVERAGE,
		DK_HIGH,
		DK_VERY_HIGH,
	};

	bool m_bMeatFace;					// Meatface?

	int m_DK_LightResistance;
	int m_DK_Toughness;
	int m_DK_MaxBoredomTicks;			// Max nbr of continuous ticks of boredom, -1 means infinite
	int m_CurBoredomTicks;				// Accumulated boredom

	// int m_DK_Bravery;				// Uses CAI_Core::m_Bravery instead
	fp32 m_DKLightDamageThreshold;		// Minimum light to have any damage effect
	fp32 m_AccumulatedLightDamage;		// Accumulated light - m_LightDamageThreshold
	bool m_bTookLightDamageLastTick;	// True when light damage
	bool m_bPlayerFOVTargets;			// Only accept targets within playerFOV when true
	int32 m_iDKHeadAttack;				// Behaviour to run when performing a head attack

	CVec3Dfp32	m_DarkSpot;				// Last spot where we were stationary and didn't take light damage
	fp32		m_DarkSpotLight;		// How bright is the darkspot?

	int32		m_DK_Special;
	enum
	{
		DK_SPECIAL_NONE			= 0,
		DK_SPECIAL_INFORMER		= 1,
		DK_SPECIAL_BERSERKER	= 2,
		DK_SPECIAL_LIGHTKILLER	= 3,
		DK_SPECIAL_KAMIKAZE		= 4,
		DK_SPECIAL_GUNNER		= 5,
	};

	// ***TBD*** Should we allow so many check or should we allow only ONE per xxx ticks?
#define AI_SP_LIGHTMETER_MAXCOUNT	2
	int m_SPLightMeasureCount;

	//Copy constructor	
	CAI_Core_Darkling(CAI_Core * _pAI_Core);

	//Initializer
	virtual void Init(CWObject * _pCharacter, CWorld_Server * _pServer);

	virtual int GetType();
	virtual void OnPreScriptAction(int _iAction, bool _bTake);

	// Determines the most suitable target (darklings determine best target with other params than regular AIs)
	virtual bool IsValidTarget(int _iTarget);
	
	// Verifies _pSP for light etc 
	virtual bool CheckScenepoint(CWO_ScenePoint* _pSP);

	//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
	//but information is gathered.
	virtual void OnRefresh(bool _bPassive = false);

	virtual void OnTakeAction();

	//Handles incoming messages
	virtual aint OnMessage(const CWObject_Message& _Msg);

	//Handles any registry keys of a character that are AI-related
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	// Adds all used behaviours to _liBehaviours, returns the nbr added
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	virtual int GetUsedGestures(TArray<int16>& _liGestures) const;

	virtual void OnBumped(bool _bWeAreBumping,int _iObj,CVec3Dfp32& CollPos);

	//Load/save
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);

};


#endif
