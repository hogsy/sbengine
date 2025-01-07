#ifndef _INC_AI_DEVICEHANDLER
#define _INC_AI_DEVICEHANDLER

#include "AICore.h"


//Base device class
class CAI_Device
{
protected:
	CAI_Core * m_pAI;

	//Is device not in use or otherwise locked?
	bool m_bAvailable;
	//Is device locked, but not used?
	bool m_bLocked;
	//Is device paused
	bool m_bPause;

public:
	CAI_Device();
	virtual void SetAI(CAI_Core* _pAI);

	//Devices
	enum {
		LOOK		= 0,
		MOVE,		
		JUMP,			
		SOUND,		
		WEAPON,		
		ITEM,		
		STANCE,		
		ANIMATION,
		MELEE,
		FACIAL,
		DARKNESS,

		NUM_DEVICE,
	};
	// Pauses/unpauses device according to _bPause
	void Pause(bool _bPause);

	//Checks if device isn't in use
	virtual bool IsAvailable();

	//Lock device for usage, without using it
	virtual void Lock(bool _bUse = false);

	//Free device for usage, clearing any current usage values
	virtual void Free();

	//Subclasses individually implement the use method with appropriate arguments

	//Advances the device one frame
	virtual void OnRefresh();
	//Post actions refresh
	virtual void OnPostActionsRefresh();

	//Change AI user
	virtual void ReInit(CAI_Core* _pAI);

	//Get control data. Fails if device haven't been used this frame. Any data is written into the 
	//given pointer adresses.
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);

	//Savegame
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};


//Look device
#define AI_LOOKRATE		0.05f
#define AI_LOOKFRAMES	1
class CAI_Device_Look : public CAI_Device
{
protected:
	//The current look vector
	CVec3Dfp32 m_Look;	
	//The direction the bot will look in after any commands in the control frame has been implemented
	CVec3Dfp32 m_NextLook;

	//This value checks if we should look "softly" or not, i.e. if look should be restricted by movement direction 
	//so that we never deviate look more than a certain threshold from movement direction, which in turn means we
	//never have to strafe or move backwards to maintain look
	bool m_bSoft;

	//Should we always look softly, regardless of use params?
	bool m_bAlwaysSoft;

	//Cosine of angle to movement direction we should never deviate more from when soft looking
	fp32 m_SoftLookCosThreshold;

	//Soft look target (i.e. vector direction we would like to look in eventually)
    CVec3Dfp32 m_TargetDir;
	
public:
	CAI_Device_Look();
	
	//Change look according to the given value or not at all as deafult
	void Use(CVec3Dfp32 _Look, bool _bSoft = false, const CVec3Dfp32& _TargetDir = CVec3Dfp32(_FP32_MAX));
	void Use(bool _bSoft = false, const CVec3Dfp32& _TargetDir = CVec3Dfp32(_FP32_MAX));

	//The direction we'll be looking in after the current look command has been executed
	CVec3Dfp32 GetNextLook();
	void UpdateNextLook();

	//Advances the device one frame
	virtual void OnRefresh();

	//Get control data. Fails if device haven't been used this frame. Any data is written into the 
	//given pointer adresses.
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);

	//Set soft look threshold (given angle in fractions)
	void SetSoftLookThreshold(fp32 _Angle);

	//Should current look target be abandoned given wanted move direction and look restrictions
	bool BadLookTarget(const CVec3Dfp32& _WantedMove);
	bool BadLookDir(const CVec3Dfp32& _Dir);

	//Check if we're currently soft looking. If the optional flag is set we only check if we are always soft looking.
	bool IsSoftLook(bool _bAlwaysOnly = false);

	// Set/reset wether bot should softlook or not
	void SetAlwaysLookSoft(bool _bAlways = true);
};


//Move device
class CAI_Device_Move : public CAI_Device
{
protected:
	//The current movement vector
	CVec3Dfp32 m_Move;
	CVec3Dfp32 m_LastMove;

	//Should we be using animation physics control?
	bool m_bAnimControl;

public:
	CAI_Device_Move();

	//Move according to the given value, or stop as default
	void Use(CVec3Dfp32 _Move = 0, bool _bAnimControl = true);
	//Repeat the last move used
	void UseLastMove(bool _bAnimControl = true);
	CVec3Dfp32 GetLastMove();
	CVec3Dfp32 GetNextMove();

	//Advances the device one frame
	virtual void OnRefresh();

	//Get control data. Fails if device haven't been used this frame. Any data is written into the 
	//given pointer adresses.
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);
};


//Jump device
class CAI_Device_Jump : public CAI_Device
{
protected:
	//Should we jump this frame?
	bool m_bJump;

public:
	CAI_Device_Jump();

	//Jump once
	void Use();

	//Advances the device one frame
	virtual void OnRefresh();
};



//Sound handler; all sounds are emitted from bot's focus position (as defined in AICore.h)
//Note that sounds are used asynchronously, and not through use of the normal control frame.
class CAI_Device_Sound : public CAI_Device 
{
public:
	// Below follws a complex looking thing for handing different types of sounds (IDLE_IDLE,IDLE_THREATEN etc)
	// and the different variants of the same type.
	// Each type is stored in the ms_liDialogueType array inside an CAI_SoundType structure
	// The CAI_SoundType holds the different variants.
	// Usage: To use a specific sound type, say IDLE_THREATEN you call CAI_Device_Sound::UseSoundType(IDLE_THREATEN)
	// A random variant will be picked from CAI_Device_Sound::ms_liDialogueType[IDLE_THREATEN].m_lVariants
#define AI_SOUNDTYPE_DEFAULT_INTERVAL	100
#define AI_SOUNDTYPE_DEFAULT_DURATION	50
#define AI_SOUNDDIALOGUE_MAXRANGE		512

	//Sound types; note that some sounds are handled by character and thus are not handled here
	enum {
		INVALID_SOUND = -1,

		IDLE_TALK,						// Talk that do not need a response
		IDLE_CALL,						// Idle chat
		IDLE_CALL_RESPONSE,				// Item for item matching response to IDLE_CALL
		IDLE_AFFIRMATIVE,				// Peacetime affirmative of order
		IDLE_TO_WARY,					// Bot goes to wary
		
		COMBAT_AFFIRMATIVE,				// Combat affirmative of order
		SUSPICIOUS_SOUND,				// What the fuck was that and < HOSTILE character might say
		DANGEROUS_SOUND,				// What the fuck was that and < ENEMY character might say
		SPOT_PLAYER_ODD,				// Only civs say this, badguys use EscalateOdd voices
		SPOT_PLAYER_GUN,				// Only civs say this, badguys use EscalateOdd voices
		ENEMY_SPOT_PLAYER,
		AFRAID_PLEAD_FOR_LIFE,
		AFRAID_PANIC,

		ENEMY_RETREAT,
		SPOT_PLAYER_DARKNESS,
		
		IDLE_WARNING,				// Warn player to stay (the fuck) away
		WARY_THREATEN,				// Punch player and threaten some more, each punch raises the hostility
		
		ESCALATE_THREAT_HOSTILE0,
		ESCALATE_THREAT_HOSTILE1,
		ESCALATE_THREAT_HOSTILE2,
		ESCALATE_THREAT_STOP,

		ESCALATE_ODD_HOSTILE0,
		ESCALATE_ODD_HOSTILE1,
		ESCALATE_ODD_HOSTILE2,
		ESCALATE_ODD_STOP,

		SEARCH_START,
		SEARCH_CONTINUE,
		SEARCH_STOP,
		SEARCH_STOP_RESPONSE,

		COMBAT_DETECTED,
		COMBAT_SPOTTED,
		COMBAT_ALONE,

		CHECKDEAD_IDLE_SPOT_CORPSE,
		CHECKDEAD_DEATHCAUSE_GENERIC,			// He is dead

		LEADER_COMBAT_FWD,
		LEADER_COMBAT_FLANK,
		LEADER_COMBAT_REAR,
		LEADER_COMBAT_COVER,

		COVER_TAUNT,						// Your mother was a hamster!

		// Darkling specific voices
		CHECKDEAD_IDLE_PISS_CORPSE,			// Pissing on a corpse
		COMBAT_ATTACKJUMP,					//DarklingJumpClose
		COMBAT_KILLJOY,
		IDLE_ATTENTION,						//Hey I noticed something
		IDLE_SHORTJUMP,						//Short Idle jump
		IDLE_LONGJUMP,						//Longer idle jump
		IDLE_AVOIDJUMP,						//Jump to avoid player
		IDLE_BOREDOM,						//Booooorrrrriiiiinnnnnggggg!!!!!
		IDLE_NOWAY,							//Refusal to redirect etc
		IDLE_TO_DEADBODY,					// Verbally desecrate corpse
		IDLE_TO_RIVAL,						// Gunner and Kamikaze loudmouths

		MAX_SOUNDS,
	};

static const char* ms_lTranslateSpeech[];

#define IDLE_TALK_1							250
#define IDLE_TALK_2 						251
#define IDLE_TALK_3 						252
#define IDLE_TALK_4 						253
#define IDLE_TALK_5 						254
#define IDLE_TALK_6 						255
#define IDLE_TALK_7 						256
#define IDLE_TALK_8 						257
#define IDLE_TALK_9 						258
#define IDLE_TALK_10 						259
#define IDLE_TALK_11 						260
#define IDLE_TALK_12 						261
#define IDLE_TALK_13 						262
#define IDLE_TALK_14 						263
#define IDLE_TALK_15 						264
#define IDLE_TALK_16 						265
#define IDLE_TALK_17 						266
#define IDLE_TALK_18 						267
#define IDLE_TALK_19 						268
#define IDLE_TALK_20 						269

#define IDLE_CALL_1							710
#define IDLE_CALL_2							711
#define IDLE_CALL_3							712
#define IDLE_CALL_4							713
#define IDLE_CALL_5							714
#define IDLE_CALL_6							715
#define IDLE_CALL_7							716
#define IDLE_CALL_8							717
#define IDLE_CALL_9							718
#define IDLE_CALL_10						719

#define IDLE_CALL_RESPONSE_1				720
#define IDLE_CALL_RESPONSE_2				721
#define IDLE_CALL_RESPONSE_3				722
#define IDLE_CALL_RESPONSE_4				723
#define IDLE_CALL_RESPONSE_5				724
#define IDLE_CALL_RESPONSE_6				725
#define IDLE_CALL_RESPONSE_7				726
#define IDLE_CALL_RESPONSE_8				727
#define IDLE_CALL_RESPONSE_9				728
#define IDLE_CALL_RESPONSE_10				729

#define IDLE_AFFIRMATIVE_1					200
#define IDLE_AFFIRMATIVE_2					201
#define IDLE_AFFIRMATIVE_3					202
#define IDLE_AFFIRMATIVE_4					203
#define IDLE_AFFIRMATIVE_5					204

#define IDLE_TO_WARY_1						220
#define IDLE_TO_WARY_2						221
#define IDLE_TO_WARY_3						222
#define IDLE_TO_WARY_4						223
#define IDLE_TO_WARY_5						224

#define COMBAT_AFFIRMATIVE_1				210
#define COMBAT_AFFIRMATIVE_2				211
#define COMBAT_AFFIRMATIVE_3				212
#define COMBAT_AFFIRMATIVE_4				213
#define COMBAT_AFFIRMATIVE_5				214

#define SUSPICIOUS_SOUND_1					240
#define SUSPICIOUS_SOUND_2					241
#define SUSPICIOUS_SOUND_3					242
#define SUSPICIOUS_SOUND_4					243
#define SUSPICIOUS_SOUND_5					244
#define SUSPICIOUS_SOUND_6					245
#define SUSPICIOUS_SOUND_7					246
#define SUSPICIOUS_SOUND_8					247
#define SUSPICIOUS_SOUND_9					248
#define SUSPICIOUS_SOUND_10					249

// Dummy indices
#define DANGEROUS_SOUND_1					999
#define DANGEROUS_SOUND_2					999
#define DANGEROUS_SOUND_3					999

#define ENEMY_SPOT_PLAYER_1					330
#define ENEMY_SPOT_PLAYER_2					331
#define ENEMY_SPOT_PLAYER_3					332
#define ENEMY_SPOT_PLAYER_4					333
#define ENEMY_SPOT_PLAYER_5					334
#define ENEMY_SPOT_PLAYER_6					335
#define ENEMY_SPOT_PLAYER_7					336
#define ENEMY_SPOT_PLAYER_8					337
#define ENEMY_SPOT_PLAYER_9					338
#define ENEMY_SPOT_PLAYER_10				339
#define ENEMY_SPOT_PLAYER_11				340
#define ENEMY_SPOT_PLAYER_12				341
#define ENEMY_SPOT_PLAYER_13				342
#define ENEMY_SPOT_PLAYER_14				343
#define ENEMY_SPOT_PLAYER_15				344
#define ENEMY_SPOT_PLAYER_16				345
#define ENEMY_SPOT_PLAYER_17				346
#define ENEMY_SPOT_PLAYER_18				347
#define ENEMY_SPOT_PLAYER_19				348
#define ENEMY_SPOT_PLAYER_20				349

#define ENEMY_RETREAT_1						700
#define ENEMY_RETREAT_2						701
#define ENEMY_RETREAT_3						702
#define ENEMY_RETREAT_4						703
#define ENEMY_RETREAT_5						704
#define ENEMY_RETREAT_6						705
#define ENEMY_RETREAT_7						706
#define ENEMY_RETREAT_8						707
#define ENEMY_RETREAT_9						708
#define ENEMY_RETREAT_10					709

// Dummy indices
#define SPOT_PLAYER_ODD_1					360
#define SPOT_PLAYER_ODD_2					361
#define SPOT_PLAYER_ODD_3					362
#define SPOT_PLAYER_GUN_1					370
#define SPOT_PLAYER_GUN_2					371
#define SPOT_PLAYER_GUN_3					372
#define AFRAID_PLEAD_FOR_LIFE_1				400
#define AFRAID_PLEAD_FOR_LIFE_2				401
#define AFRAID_PLEAD_FOR_LIFE_3				402
#define AFRAID_PANIC_1						400
#define AFRAID_PANIC_2						401
#define AFRAID_PANIC_3						402

#define SPOT_PLAYER_DARKNESS_1				410
#define SPOT_PLAYER_DARKNESS_2				411
#define SPOT_PLAYER_DARKNESS_3				412
#define SPOT_PLAYER_DARKNESS_4				413
#define SPOT_PLAYER_DARKNESS_5				414
#define SPOT_PLAYER_DARKNESS_6				415
#define SPOT_PLAYER_DARKNESS_7				416
#define SPOT_PLAYER_DARKNESS_8				417
#define SPOT_PLAYER_DARKNESS_9				418
#define SPOT_PLAYER_DARKNESS_10				419

#define IDLE_WARNING_1						730
#define IDLE_WARNING_2						731
#define IDLE_WARNING_3						732
#define IDLE_WARNING_4						733
#define IDLE_WARNING_5						734
#define WARY_THREATEN_1						740
#define WARY_THREATEN_2						741
#define WARY_THREATEN_3						742
#define WARY_THREATEN_4						743
#define WARY_THREATEN_5						744

#define ESCALATE_THREAT_HOSTILE0_1			420
#define ESCALATE_THREAT_HOSTILE0_2			421
#define ESCALATE_THREAT_HOSTILE0_3			422
#define ESCALATE_THREAT_HOSTILE0_4			423
#define ESCALATE_THREAT_HOSTILE0_5			424
#define ESCALATE_THREAT_HOSTILE0_6			425
#define ESCALATE_THREAT_HOSTILE0_7			426
#define ESCALATE_THREAT_HOSTILE0_8			427
#define ESCALATE_THREAT_HOSTILE0_9			428
#define ESCALATE_THREAT_HOSTILE0_10			429

#define ESCALATE_THREAT_HOSTILE1_1			430
#define ESCALATE_THREAT_HOSTILE1_2			431
#define ESCALATE_THREAT_HOSTILE1_3			432
#define ESCALATE_THREAT_HOSTILE1_4			433
#define ESCALATE_THREAT_HOSTILE1_5			434

#define ESCALATE_THREAT_HOSTILE2_1			440
#define ESCALATE_THREAT_HOSTILE2_2			441
#define ESCALATE_THREAT_HOSTILE2_3			442
#define ESCALATE_THREAT_HOSTILE2_4			443
#define ESCALATE_THREAT_HOSTILE2_5			444

#define ESCALATE_THREAT_STOP_1				450
#define ESCALATE_THREAT_STOP_2				451
#define ESCALATE_THREAT_STOP_3				452
#define ESCALATE_THREAT_STOP_4				453
#define ESCALATE_THREAT_STOP_5				454


#define ESCALATE_ODD_HOSTILE0_1				460
#define ESCALATE_ODD_HOSTILE0_2				461
#define ESCALATE_ODD_HOSTILE0_3				462
#define ESCALATE_ODD_HOSTILE0_4				463
#define ESCALATE_ODD_HOSTILE0_5				464

#define ESCALATE_ODD_HOSTILE1_1				470
#define ESCALATE_ODD_HOSTILE1_2				471
#define ESCALATE_ODD_HOSTILE1_3				472
#define ESCALATE_ODD_HOSTILE1_4				473
#define ESCALATE_ODD_HOSTILE1_5				474

#define ESCALATE_ODD_HOSTILE2_1				480
#define ESCALATE_ODD_HOSTILE2_2				481
#define ESCALATE_ODD_HOSTILE2_3				482
#define ESCALATE_ODD_HOSTILE2_4				483
#define ESCALATE_ODD_HOSTILE2_5				484

#define ESCALATE_ODD_STOP_1					490
#define ESCALATE_ODD_STOP_2					491
#define ESCALATE_ODD_STOP_3					492
#define ESCALATE_ODD_STOP_4					493
#define ESCALATE_ODD_STOP_5					494

#define SEARCH_START_1						500
#define SEARCH_START_2						501
#define SEARCH_START_3						502
#define SEARCH_START_4						503
#define SEARCH_START_5						504

#define SEARCH_CONTINUE_1					510
#define SEARCH_CONTINUE_2					511
#define SEARCH_CONTINUE_3					512
#define SEARCH_CONTINUE_4					513
#define SEARCH_CONTINUE_5					514

#define SEARCH_STOP_1						520
#define SEARCH_STOP_2						521
#define SEARCH_STOP_3						522
#define SEARCH_STOP_4						523
#define SEARCH_STOP_5						524

#define SEARCH_STOP_RESPONSE_1				530
#define SEARCH_STOP_RESPONSE_2				531
#define SEARCH_STOP_RESPONSE_3				532
#define SEARCH_STOP_RESPONSE_4				533
#define SEARCH_STOP_RESPONSE_5				534

#define	COMBAT_ALONE_1						560
#define COMBAT_ALONE_2						561
#define COMBAT_ALONE_3						562
#define COMBAT_ALONE_4						563
#define COMBAT_ALONE_5						564
#define COMBAT_ALONE_6						565
#define COMBAT_ALONE_7						566
#define COMBAT_ALONE_8						567
#define COMBAT_ALONE_9						568
#define COMBAT_ALONE_10						569
#define COMBAT_ALONE_11						570
#define COMBAT_ALONE_12						571
#define COMBAT_ALONE_13						572
#define COMBAT_ALONE_14						573
#define COMBAT_ALONE_15						574	
#define COMBAT_ALONE_16						575
#define COMBAT_ALONE_17						576
#define COMBAT_ALONE_18						577
#define COMBAT_ALONE_19						578
#define COMBAT_ALONE_20						579

#define COMBAT_DETECTED_1					580
#define COMBAT_DETECTED_2					581
#define COMBAT_DETECTED_3					582
#define COMBAT_DETECTED_4					583
#define COMBAT_DETECTED_5					584
#define COMBAT_DETECTED_6					585
#define COMBAT_DETECTED_7					586
#define COMBAT_DETECTED_8					587
#define COMBAT_DETECTED_9					588
#define COMBAT_DETECTED_10					589
#define COMBAT_DETECTED_11					590
#define COMBAT_DETECTED_12					591
#define COMBAT_DETECTED_13					592
#define COMBAT_DETECTED_14					593
#define COMBAT_DETECTED_15					594
#define COMBAT_DETECTED_16					595
#define COMBAT_DETECTED_17					596
#define COMBAT_DETECTED_18					597
#define COMBAT_DETECTED_19					598
#define COMBAT_DETECTED_20					599

#define COMBAT_SPOTTED_1					600
#define COMBAT_SPOTTED_2					601
#define COMBAT_SPOTTED_3					602
#define COMBAT_SPOTTED_4					603
#define COMBAT_SPOTTED_5					604
#define COMBAT_SPOTTED_6					605
#define COMBAT_SPOTTED_7					606
#define COMBAT_SPOTTED_8					607
#define COMBAT_SPOTTED_9					608
#define COMBAT_SPOTTED_10					609
#define COMBAT_SPOTTED_11					610
#define COMBAT_SPOTTED_12					611
#define COMBAT_SPOTTED_13					612
#define COMBAT_SPOTTED_14					613
#define COMBAT_SPOTTED_15					614
#define COMBAT_SPOTTED_16					615
#define COMBAT_SPOTTED_17					616
#define COMBAT_SPOTTED_18					617
#define COMBAT_SPOTTED_19					618
#define COMBAT_SPOTTED_20					619

#define CHECKDEAD_IDLE_SPOT_CORPSE_1		620
#define CHECKDEAD_IDLE_SPOT_CORPSE_2		621
#define CHECKDEAD_IDLE_SPOT_CORPSE_3		622
#define CHECKDEAD_IDLE_SPOT_CORPSE_4		623
#define CHECKDEAD_IDLE_SPOT_CORPSE_5		624
#define CHECKDEAD_IDLE_SPOT_CORPSE_6		625
#define CHECKDEAD_IDLE_SPOT_CORPSE_7		626
#define CHECKDEAD_IDLE_SPOT_CORPSE_8		627
#define CHECKDEAD_IDLE_SPOT_CORPSE_9		628
#define CHECKDEAD_IDLE_SPOT_CORPSE_10		629

#define CHECKDEAD_DEATHCAUSE_GENERIC_1		630
#define CHECKDEAD_DEATHCAUSE_GENERIC_2		631
#define CHECKDEAD_DEATHCAUSE_GENERIC_3		632
#define CHECKDEAD_DEATHCAUSE_GENERIC_4		633
#define CHECKDEAD_DEATHCAUSE_GENERIC_5		634
#define CHECKDEAD_DEATHCAUSE_GENERIC_6		635
#define CHECKDEAD_DEATHCAUSE_GENERIC_7		636
#define CHECKDEAD_DEATHCAUSE_GENERIC_8		637
#define CHECKDEAD_DEATHCAUSE_GENERIC_9		638
#define CHECKDEAD_DEATHCAUSE_GENERIC_10		639

#define LEADER_COMBAT_FWD_1					650
#define LEADER_COMBAT_FWD_2					651
#define LEADER_COMBAT_FWD_3					652
#define LEADER_COMBAT_FWD_4					653
#define LEADER_COMBAT_FWD_5					654

#define LEADER_COMBAT_FLANK_1				660
#define LEADER_COMBAT_FLANK_2				661
#define LEADER_COMBAT_FLANK_3				662
#define LEADER_COMBAT_FLANK_4				663
#define LEADER_COMBAT_FLANK_5				664

#define LEADER_COMBAT_REAR_1				670
#define LEADER_COMBAT_REAR_2				671
#define LEADER_COMBAT_REAR_3				672
#define LEADER_COMBAT_REAR_4				673
#define LEADER_COMBAT_REAR_5				674

#define LEADER_COMBAT_COVER_1				680
#define LEADER_COMBAT_COVER_2				681
#define LEADER_COMBAT_COVER_3				682
#define LEADER_COMBAT_COVER_4				683
#define LEADER_COMBAT_COVER_5				684

#define COVER_TAUNT_1						750
#define COVER_TAUNT_2						751
#define COVER_TAUNT_3						752
#define COVER_TAUNT_4						753
#define COVER_TAUNT_5						754
#define COVER_TAUNT_6						755
#define COVER_TAUNT_7						756
#define COVER_TAUNT_8						757
#define COVER_TAUNT_9						758
#define COVER_TAUNT_10						759
#define COVER_TAUNT_11						760
#define COVER_TAUNT_12						761
#define COVER_TAUNT_13						762
#define COVER_TAUNT_14						763
#define COVER_TAUNT_15						764
#define COVER_TAUNT_16						765
#define COVER_TAUNT_17						766
#define COVER_TAUNT_18						767
#define COVER_TAUNT_19						768
#define COVER_TAUNT_20						769

#define CHECKDEAD_IDLE_PISS_CORPSE_1		100
#define CHECKDEAD_IDLE_PISS_CORPSE_2		101
#define CHECKDEAD_IDLE_PISS_CORPSE_3		102
#define CHECKDEAD_IDLE_PISS_CORPSE_4		103
#define CHECKDEAD_IDLE_PISS_CORPSE_5		104
#define CHECKDEAD_IDLE_PISS_CORPSE_6		105
#define CHECKDEAD_IDLE_PISS_CORPSE_7		106
#define CHECKDEAD_IDLE_PISS_CORPSE_8		107
#define CHECKDEAD_IDLE_PISS_CORPSE_9		108
#define CHECKDEAD_IDLE_PISS_CORPSE_10		109

#define COMBAT_ATTACKJUMP_1					130
#define COMBAT_ATTACKJUMP_2					131
#define COMBAT_ATTACKJUMP_3					132
#define COMBAT_ATTACKJUMP_4					133
#define COMBAT_ATTACKJUMP_5					134
#define COMBAT_ATTACKJUMP_6					135
#define COMBAT_ATTACKJUMP_7					136
#define COMBAT_ATTACKJUMP_8					137
#define COMBAT_ATTACKJUMP_9					138
#define COMBAT_ATTACKJUMP_10				139

#define COMBAT_KILLJOY_1					140
#define COMBAT_KILLJOY_2					141
#define COMBAT_KILLJOY_3					142
#define COMBAT_KILLJOY_4					143
#define COMBAT_KILLJOY_5					144
#define COMBAT_KILLJOY_6					145
#define COMBAT_KILLJOY_7					146
#define COMBAT_KILLJOY_8					147
#define COMBAT_KILLJOY_9					148
#define COMBAT_KILLJOY_10					149

#define IDLE_ATTENTION_1					190
#define IDLE_ATTENTION_2					191
#define IDLE_ATTENTION_3					192
#define IDLE_ATTENTION_4					193
#define IDLE_ATTENTION_5					194
#define IDLE_ATTENTION_6					195
#define IDLE_ATTENTION_7					196
#define IDLE_ATTENTION_8					197
#define IDLE_ATTENTION_9					198
#define IDLE_ATTENTION_10					199

#define IDLE_SHORTJUMP_1					900
#define IDLE_SHORTJUMP_2					901
#define IDLE_SHORTJUMP_3					902
#define IDLE_SHORTJUMP_4					903
#define IDLE_SHORTJUMP_5					904
#define IDLE_SHORTJUMP_6					905
#define IDLE_SHORTJUMP_7					906
#define IDLE_SHORTJUMP_8					907
#define IDLE_SHORTJUMP_9					908
#define IDLE_SHORTJUMP_10					909

#define IDLE_LONGJUMP_1						910
#define IDLE_LONGJUMP_2						911
#define IDLE_LONGJUMP_3						912
#define IDLE_LONGJUMP_4						913
#define IDLE_LONGJUMP_5						914
#define IDLE_LONGJUMP_6						915
#define IDLE_LONGJUMP_7						916
#define IDLE_LONGJUMP_8						917
#define IDLE_LONGJUMP_9						918
#define IDLE_LONGJUMP_10					919

#define IDLE_AVOIDJUMP_1					220
#define IDLE_AVOIDJUMP_2					221
#define IDLE_AVOIDJUMP_3					222
#define IDLE_AVOIDJUMP_4					223
#define IDLE_AVOIDJUMP_5					224
#define IDLE_AVOIDJUMP_6					225
#define IDLE_AVOIDJUMP_7					226
#define IDLE_AVOIDJUMP_8					227
#define IDLE_AVOIDJUMP_9					228
#define IDLE_AVOIDJUMP_10					229

#define IDLE_BOREDOM_1						230
#define IDLE_BOREDOM_2						231
#define IDLE_BOREDOM_3						232
#define IDLE_BOREDOM_4						233
#define IDLE_BOREDOM_5						234
#define IDLE_BOREDOM_6						235
#define IDLE_BOREDOM_7						236
#define IDLE_BOREDOM_8						237
#define IDLE_BOREDOM_9						238
#define IDLE_BOREDOM_10						239

#define IDLE_NOWAY_1						180
#define IDLE_NOWAY_2						181
#define IDLE_NOWAY_3						182
#define IDLE_NOWAY_4						183
#define IDLE_NOWAY_5						184

#define EXTRAS_TO_DEADBODY_1				150
#define EXTRAS_TO_DEADBODY_2				151
#define EXTRAS_TO_DEADBODY_3				152
#define EXTRAS_TO_DEADBODY_4				153
#define EXTRAS_TO_DEADBODY_5				154
#define EXTRAS_TO_DEADBODY_6				155
#define EXTRAS_TO_DEADBODY_7				156
#define EXTRAS_TO_DEADBODY_8				157
#define EXTRAS_TO_DEADBODY_9				158
#define EXTRAS_TO_DEADBODY_10				159
#define EXTRAS_TO_DEADBODY_11				160
#define EXTRAS_TO_DEADBODY_12				161
#define EXTRAS_TO_DEADBODY_13				162
#define EXTRAS_TO_DEADBODY_14				163
#define EXTRAS_TO_DEADBODY_15				164

#define EXTRAS_TO_RIVAL_1					170
#define EXTRAS_TO_RIVAL_2					171
#define EXTRAS_TO_RIVAL_3					172
#define EXTRAS_TO_RIVAL_4					173
#define EXTRAS_TO_RIVAL_5					174
#define EXTRAS_TO_RIVAL_6					175
#define EXTRAS_TO_RIVAL_7					176
#define EXTRAS_TO_RIVAL_8					177
#define EXTRAS_TO_RIVAL_9					178
#define EXTRAS_TO_RIVAL_10					179

protected:
	class CAI_SoundType : public CReferenceCount
	{
		public:
		CAI_SoundType();
		virtual void operator= (const CAI_SoundType& _Sound);
		void Clear();
		int16 GetRandom();
		int16 Get(int _Index);

		TThinArray<int32>	m_lVariants;	// List of variant indices
		int32				m_LastUseTick;	// Tick of last use
		int32				m_MinInterval;	// Min ticks between uses (includes actual duration)
		int16				m_LastUsed1;
		int16				m_LastUsed2;
	};

	static bool ms_bDialogSetup;
	static bool ms_bUseticksReset;
	// AI_GLOBAL_VOICE_SEPARATION_FACTOR how long each sound must be separated:
	// 1.0: No separation
	// 0.5: 50% overlap
	// 1.5: 50% silence between soudns etc
#define AI_GLOBAL_VOICE_SEPARATION_FACTOR 0.7f
	static int ms_NextUseTick;	// Global tick for next voice use
	static CAI_SoundType ms_liDialogueTypes[MAX_SOUNDS];
public:
	void SetupDialogueIndices();
	void ResetDialogueStartTicks();
protected:
	//Count how many frames the device is unavailable (i.e. how long current sounds will be playing)
	int m_iWait;
	
	int m_iDelayTicks;
	int m_iDelaySound;
	int m_iDelayPrio;
public:
	CAI_Device_Sound();

	//Change AI user
	virtual void ReInit(CAI_Core* _pAI);

	// Returns an legit variant that can be later spoken by UseRandom()
	// Returns -1 if none was found
	int GetRandomToUse(int _iType);
	// Returns an actual dialogue index or 0
	int GetRandomDialogue(int _iType);
	// Emit random sound from given type if device is available.
	// Returns the variant used if a sound was actually played and -1 if none was played
	int UseRandom(int _iType,int _Prio);
	int UseRandom(int _iType,int _Prio,int _iVariant);
	// Emit a random variant of _iType, delayed _Delay, ai ticks
	void UseRandomDelayed(int _iType,int _Prio,int _DelayTicks);

	// Returns the duration of _iVariant of _iType in ticks, returns -1 on error (wrong index etc)
	int GetDuration(int _iType,int _iVariant);
	// PlayDialogue: Plays the specified dialogue
	void PlayDialogue(int _iDialogue, int _Prio = CAI_Action::PRIO_IDLE, int _Wait = AI_SOUNDTYPE_DEFAULT_DURATION);
	int GetDialogueLength(int _iDialogue);
	void PauseType(int _iType,fp32 _Duration = -1.0f);
	// Check if the particular soundtype is available
	// Check if a particular soundtype is available AND the device too
	bool SoundAvailable(int _iType);
	// A highly setting specific call to play a dialogue and queu up two more
	// _iDialogue will play right away, _iNext1 will play after _iNext1Delay and _iNext2 will play _iNext2Delay delay
	// void SetupRadioDialogue(int _iDialogue,int16 _iNext1 = 0,int16 _iNext1Delay = 0,int16 _iNext2 = 0,int16 _iNext2Delay = 0);

	//Free device
	virtual void Free();
	
	//The currently playing sound will lock device for a number of frames as set by the 
	//duration of the sound, or until device is explicitly freed. 
	//Note that the current sound functionality does not support moving a sound source, 
	//so all sounds will be emitted from the position where they where originally emitted.
	virtual void OnRefresh();

	//This always fails, since device doesn't use the normal control frame routine
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);

	//Savegame
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};


//Item/weapon device
class CAI_Device_Item : public CAI_Device
{
protected:
	//Usage mode. NONE when not in use
	int m_iUse;

	//Delayed usage mode, index and frame delay
	int m_iDelayUse;
	int m_iDelay;

	//Continuous useage timer
	int m_ContinuousUse;

public:
	CAI_Device_Item();

	//Usage modes
	enum 
	{
		NONE = 0,
		USE,
		SWITCH_NEXT,
		SWITCH_PREVIOUS,
		SWITCH,
	};

	//Use item. Optional name parameter only used if we're switching item by index
	void Use(int _Mode = USE, CStr _Name = "");

	//Use item after the given delay. Only one delayeduse can be active at one time, and 
	//device will be locked until use
	void UseDelayed(int _iDelay, int _iMode = USE);

	//Use item (not switch) for the given number of frames before releasing it
	void UseContinuous(int _UseTime);

	//Advances the device one frame
	virtual void OnRefresh();

	//Get control data. Fails if device haven't been used this frame. Any data is written into the 
	//given pointer adresses.
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);

	//Savegame
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};


#define AI_WEAPON_SHOT_INTERVAL	10
class CAI_Device_Weapon : public CAI_Device_Item
{
protected:
	int m_Period;			// Average nbr of ticks between uses
	int m_PressDuration;	// Duration of Use when firing periodically
	int m_NextUseTimer;		// NExt m_pAI->m_Timer we may be used on
	
public:
	CAI_Device_Weapon();
	void SetPeriod(int _Period);			// How long between presses when used periodically
	void SetPressDuration(int _Duration);	// How long should trigger be pressed when used periodically, default = 0
	void UsePeriodic();
	//Advances the device one frame
	virtual void OnPostActionsRefresh();
	//Savegame
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};

//Stance device
class CAI_Device_Stance : public CAI_Device
{
protected:
	//Current stance
	int m_iStance;
	int m_iMinStance;
	int m_iMaxStance;
	// Flagfield?
	bool m_bCrouching;
	bool m_bCrouchAvailable;
	bool m_bStanceAvailable;
#define TARGET_IN_FOV_TICKS		10
	int m_TargetInFOVCounter;
	int m_CrouchCounter;
	
public:
	CAI_Device_Stance();

	//Idle stances
	enum {
		IDLESTANCE_IDLE			= 0,	// Idling, chatting
		IDLESTANCE_HOSTILE		= 1,	// Searching, hostile, wary
		IDLESTANCE_COMBAT		= 2,	// Combat, panic
	};

	// static int StrToStance(CStr _Str); Obsolete
	void SetTargetInFOV(bool _State);
	//Set stance. If not set every frame, stance reverts to standing, unless the optional peristent
	//flag is set to true, in which case stance is kept until it's explicitly changed by another use.
	virtual void Use(int _iStance);

	//Advances the device one frame
	virtual void OnRefresh();
	virtual void OnPostActionsRefresh();

	//Get control data. Fails if device haven't been used this frame. Any data is written into the 
	//given pointer adresses.
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);

	//Change idle stance of character. Can be performed even when device is in use.
	void SetMinStance(int _Stance, bool _bRefresh = true);
	void SetMaxStance(int _Stance, bool _bRefresh = true);
	int GetMinStance();
	int GetMaxStance();
	//bool SetMinIdleStance(int _Stance);
	bool SetIdleStance(int _Stance, int _iVariant = 0);
	void Crouch(bool _State, int _MinCrouchDuration = 40);	// Why can't I use CAI_Core::AI_TICKS_PER_SECOND?
	bool IsCrouching();
	int GetIdleStance(bool _bFromAG = false);

	//Savegame
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};


//Animation handler; used for explicitly trigger character animations. 
//Note that animations are used asynchronously, and not through use of the normal control frame.
class CAI_Device_Animation : public CAI_Device
{
public:
	//Animation types (AI only of course).
	enum {
		INVALID_ANIM = -1,

		TAUNT_SHORT = 0,
		TAUNT_LONG,
		IDLE,
		THREATEN,
	
		MAX_ANIMS,
	};

protected:
	//The animation resource indices
	enum {
		ANIM_THREATEN			= 246,

		//Taunt slots undefined for now (use threaten for simplicity)
		ANIM_TAUNT_SHORT1		= ANIM_THREATEN,
		ANIM_TAUNT_SHORT2		= ANIM_THREATEN,
		ANIM_TAUNT_SHORT3		= ANIM_THREATEN,
		ANIM_TAUNT_LONG1		= ANIM_THREATEN,
		ANIM_TAUNT_LONG2		= ANIM_THREATEN,
	};

	//Count how many frames the device is unavailable (i.e. how long current sounds will be playing)
	int m_iWait;

	//Is the current anim a torso anim?
	bool m_bTorsoAnim;

	//Current animation
	int m_iAnim;

	//Stop current animation if it's still playing
	void Stop();
	
public:
	CAI_Device_Animation();

	//This always fails, since device doesn't use the normal control frame routine
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);

	//Reduce wait and stop animation if it times out
	virtual void OnRefresh();
	
	//Set animation, or stop current animation if no anim was given. The device will be locked for
	//the playing time of the animation, plus the optionally specified settle time in frames.
	//Animation will be torso anim if so specified. If the bForce argument i true anim is 
	//always played, even if device is unavailable.
	void Use(int _iAnim = 0, int _SettleTime = 0, bool _bTorsoAnim = false, bool _bForce = false);

	//Use random animation of the given type, with given settle time, forcing it to be played if so specified.
	void UseRandom(int _iType, int _SettleTime = 0, bool _bForce = false);

	//Savegame
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};


//Special device for melee actions.
class CAI_Device_Melee : public CAI_Device
{
protected:
	//Current actions
	int m_Actions;

public:
	CAI_Device_Melee();

	//Take some actions
	enum {
		NONE = 0,
		INITIATE		= 0x1,
		BLOCK			= 0x2,
		BREAK_OFF		= 0x4,
		ATTACK_PUNCH	= 0x8,
		ATTACK_KICK		= 0x10,
		MOVE_FWD		= 0x20,
		MOVE_BWD		= 0x40,
		MOVE_LEFT		= 0x80,
		MOVE_RIGHT		= 0x100,
	};
	void Use(int _Actions);
	
	//Advances the device one frame
	virtual void OnRefresh();

	//Get control data. Fails if device haven't been used this frame. Any data is written into the 
	//given pointer adresses.
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);
};

class CAI_Device_Facial : public CAI_Device
{
protected:
	//Current facial
	int16 m_FacialGroup;
	int16 m_iFacial;
	int16 m_iTempFacial;
	int32 m_TempFacialCounter;
	bool m_bChanged;

public:
	CAI_Device_Facial();

	// F_XPRESS_IDLE_BS_01			Idle (Neutral)	   
	// F_XPRESS_ASLEEP_BS_01		Asleep	   
	// F_XPRESS_SLEEPY_BS_01		Sleepy	   
	// F_XPRESS_DRUNK_BS_01			Drunk	   
	// F_XPRESS_SAD_BS_01			Sad	   
	// F_XPRESS_ATTENTIVE_BS_01		Attentive (listen for suspicious sound)	   
	// F_XPRESS_DISTRACTED_BS_01	Distracted	   
	// F_XPRESS_IRRITATED_BS_01		Irritated	   
	// F_XPRESS_CONFUSED_BS_01		Confused	   
	// F_XPRESS_SECRET_BS_01		Secret	   
	// F_XPRESS_SURPRISED_BS_01		Surprised	   
	// F_XPRESS_HEARTY_BS_01		Hearty	   
	// F_XPRESS_JOKE_BS_01			Joke	   
	// F_XPRESS_WARY_BS_01			Wary (Nervous)	   
	// F_XPRESS_SCARED_BS_01		Scared	   
	// F_XPRESS_PANIC_BS_01			Panic	   
	// F_XPRESS_STRAINED_BS_01		Strained	   
	// F_XPRESS_HURT_BS_01			Hurt (Agony)	   
	// F_XPRESS_INPAIN_BS_01		In pain	   
	// F_XPRESS_DEAD_BS_01			Dead	   
	// F_XPRESS_HOSTILE_BS_01		Hostile (Threatening)	   
	// F_XPRESS_COMBAT_BS_01		Combat (War face)	   
	// F_XPRESS_AIMGUN_BS_01		Aiming Gun

	enum
	{
		FACIALGROUP_INVALID		= -1,
		FACIALGROUP_HUNTER		= 0,
		FACIALGROUP_BUTCHER		= 1,
		FACIALGROUP_JENNY		= 2,
		FACIALGROUP_SARAH		= 3,
		FACIALGROUP_CIVMALE3	= 4,
		FACIALGROUP_EXTREME		= 5,
		// To be continued...
	};

	// Facials
	enum {
		FACIAL_INVALID			= -1,
		FACIAL_IDLE				= 0,	// AG2_STANCETYPE_IDLE
		FACIAL_AIMGUN			= 1,	// Fire weaponattackranged
		FACIAL_ASLEEP			= 2,
		FACIAL_ATTENTIVE		= 3,	// Suspicious sound
		FACIAL_COMBAT			= 4,	// AG2_STANCETYPE_COMBAT
		FACIAL_CONFUSED			= 5,
		FACIAL_DEAD				= 6,	// Used when character is dead or undead stunned
		FACIAL_DISTRACTED		= 7,
		FACIAL_DRUNK			= 8,
		FACIAL_HEARTY			= 9,
		FACIAL_HOSTILE			= 10,	// AG2_STANCETYPE_HOSTILE
		FACIAL_HURT				= 11,	// When slightly hurt
		FACIAL_INPAIN			= 12,	// When seriously hurt
		FACIAL_JOKE				= 13,
		FACIAL_PANIC			= 14,	// Civ: AG2_STANCETYPE_PANIC
		FACIAL_SAD				= 15,
		FACIAL_SCARED			= 16,	// Used when attacked by blackhole or scared by darkness	 
		FACIAL_SECRET			= 17,
		FACIAL_SLEEPY			= 18,
		FACIAL_STRAINED			= 19,	// Used when surviving blackhole
		FACIAL_SURPRISED		= 20,	// When hearing gunshot while not in combat
		FACIAL_WARY				= 21,	// Civ: AG2_STANCETYPE_WARY
		FACIAL_IRRITATED		= 22,	// When alarmed or reloading
	};


	void SetFacialGroup(int16 _FacialGroup);
	bool Ag2SetFacial(int16 _FacialGroup, int16 _iFacial) const;

	// _Duration > 0: Sets a temp facial for _Duration ticks
	// _Duration == 0: Sets a permanent facial
	virtual void Use(int _iFacial, int32 _Duration = 0);

	//Advances the device one frame
	virtual void OnRefresh();

	int16 GetFacial();

	//Savegame
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};

//Darkness device, used to control darkness powers (dark jackie)
class CAI_Device_Darkness : public CAI_Device
{
private:
	int8 m_Usage;
	int8 m_Power;
	CVec3Dfp32 m_Move;

public:
	CAI_Device_Darkness();

	// Usage
	enum {
		USAGE_NONE = 0,
		USAGE_DRAIN		= 0x1,
		USAGE_ACTIVATE	= 0x2,
		USAGE_SELECT	= 0x4,
	};

	// Powers that can be selected and activated 
	enum {
		POWER_INVALID = -1,
		POWER_CREEPING_DARK = 0,
		POWER_DEMON_ARM,
		POWER_BLACK_HOLE
	};

	//Check if given darkness power is available
	bool IsPowerAvailable(int _Power);

	// Get the currently selected darkness power
	int GetSelectedPower();

	// Use darkness. You must use this device every tick to keep a power going continuously.
	// If a power is given without the USAGE_ACTIVATE flag, the power will be prepared for usage.
	void Use(int8 _Usage, int8 _Power = POWER_INVALID, const CVec3Dfp32& _Move = CVec3Dfp32(_FP32_MAX));

	//Advances the device one frame
	virtual void OnRefresh();

	//Get control data. Fails if device haven't been used this frame. Any data is written into the 
	//given pointer addresses.
	virtual bool GetData(int* _piData, CVec3Dfp32* _pVecData);

	// Control handler specific flags.
	enum {
		USEMOVE		= 0x100,
	};

	// Force drain darkness juice, bypassing any normal lighting restrictions etc. If _bInstant is true we fill up to maximum darkness in one go.
	void ForceDrainDarkness(bool _bInstant = false);

	// Force remove all darkness juice.
	void RemoveAllDarknessJuice();

	// The darkness power that is currently being used
	int GetActivePower();

private:
	void GatherPower(int _Amount);
};

#endif
