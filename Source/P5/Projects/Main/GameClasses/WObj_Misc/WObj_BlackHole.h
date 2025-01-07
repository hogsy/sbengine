/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_BlackHole.h

	Author:			Anton Ragnarsson, Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_BlackHole

	Comments:		

	History:		
		050929:		(anton)  Created File
		051009:		(willbo) Added visual effect, moved m_iState to m_Data[0].
		060130:		(anton)  Added line-check, tweaked forces, added ragdoll support.
		060614:		(willbo) Removed OnRefresh/OnClientRender when inactive. Added script support.
\*____________________________________________________________________________________________*/
#ifndef __WObj_BlackHole_h__
#define __WObj_BlackHole_h__



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_BlackHole
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_BlackHole : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject parent;

	enum
	{
		BLACKHOLE_DATA_STATE			= 0,
		BLACKHOLE_DATA_OWNER,
		BLACKHOLE_DATA_ACTIVATETICK,
		BLACKHOLE_DATA_LASTDAMAGETICK,
		BLACKHOLE_DATA_FLAGS,
		BLACKHOLE_DATA_LVLMODEL,

		BLACKHOLE_STATE_INACTIVE		= 0,	// idle
		BLACKHOLE_STATE_INIT,					// building up
		BLACKHOLE_STATE_MAIN,					// sucking in objects
		BLACKHOLE_STATE_EXPLODE,				// hole collapse with explosion
		BLACKHOLE_STATE_COLLAPSE,				// hole simply collapse
		BLACKHOLE_STATE_POST,					// Stuff to do after hole collapsed

		BLACKHOLE_SOUND_3D_START		= 0,
		BLACKHOLE_SOUND_3D_END,
		BLACKHOLE_SOUND_3D_LOOP,
		BLACKHOLE_SOUND_2D_START,
		BLACKHOLE_SOUND_2D_END,
		BLACKHOLE_SOUND_2D_LOOP,
		BLACKHOLE_SOUND_NUM,

		BLACKHOLE_IMPULSE_ACTIVATE				= 1,	// Activate blackhole through impule
		BLACKHOLE_IMPULSE_DEACTIVATE_EXPLODE	= 2,	// Deactivate blackhole with explosion
		BLACKHOLE_IMPULSE_DEACTIVATE_COLLAPSE	= 3,	// Deactivate blackhole without explosion

		BLACKHOLE_FLAG_TRACETEST		= M_Bit(0),
		BLACKHOLE_FLAG_BILLBOARD		= M_Bit(1),
		BLACKHOLE_FLAG_XROTATION		= M_Bit(2),
		BLACKHOLE_FLAG_YROTATION		= M_Bit(3),
		BLACKHOLE_FLAG_EXPLODE			= M_Bit(4),

		BLACKHOLE_EXMODELS_NUM			= 5,			// Level models for blackhole
	};

	fp32 m_Force;
	fp32 m_MaxForce;

	fp32 m_ActiveForce;
	int32 m_ObjectFlags;

	uint m_nObjects;
	CMap16 m_liObjects;				// Objects that are being affected by the black hole
	bool m_bQueuedDeactivation;

	int m_liSounds[BLACKHOLE_SOUND_NUM];

	uint16 m_liExModels[BLACKHOLE_EXMODELS_NUM];

	void SetOwnerDarkness(const uint16 _DarknessType);
	void SetPostState();

	M_FORCEINLINE uint16& iExModel(uint _iSlot)		{ M_ASSERT(_iSlot < BLACKHOLE_EXMODELS_NUM, CFStrF("CWObject_BlackHole::iExModel: Out of range %d/%d", _iSlot, BLACKHOLE_EXMODELS_NUM-1)); return m_liExModels[_iSlot]; }

public:
	void Activate(const CVec3Dfp32& _Position, uint _iLevel);
	void Deactivate();
	void SetExplode(const bool _bExplode);
	void AffectObjects(fp32 _Radius, fp32 _Force, uint _nDamage, bool _bExplode);
	void DamageTriggers(CWO_DamageMsg& _Dmg, fp32 _Radius);
	void FindObjects(fp32 _Radius);

	virtual void OnCreate();
	virtual void OnRefresh();
	bool IsActive();
	bool CanDeactivate();
	// Add force amount, returns fraction of force added
	fp32 AddForce();
	fp32 AddForce(fp32 _Amount);
	int GetDarkness(int _MaxDarkness, int _GameTick);
	fp32 GetActiveForce();

	virtual void Model_Set(const uint8 _iModel, const CStr& _Model);
	virtual void ModelEx_Set(const uint8 _iSlot, const CStr& _Model);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual aint OnMessage(const CWObject_Message &_Msg);

	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);

	static fp32   GetActivateRenderTime(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void  OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	static void  OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void  OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};






#endif // __WObj_BlackHole_h__
