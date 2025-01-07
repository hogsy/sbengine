#ifndef WRPGSpells_h
#define WRPGSpells_h

#include "WRPGCore.h"
#include "WRPGSummon.h"

//#include "../Models/WModels_Misc.h"
//#include "../WObj_AutoVar_AttachModel.h"

// Här ligger numer bara gammalt skräpt som skall bort så fort det inte används längre =).

//-------------------------------------------------------------------
// CSummonInit
//-------------------------------------------------------------------

class CSummonInit
{
public:
	CRPG_Object_Item *m_pSpell;
	CRPG_Object *m_pCaster;
	CCollisionInfo *m_pCollisionInfo;
	int m_iCaster;
	int m_iEffect;
	fp32 m_Param0;
	fp32 m_Param1; // Added by Mondelore.
};

// -------------------------------------------------------------------
//  CRPG_Object_RaySummon
// -------------------------------------------------------------------
class CRPG_Object_RaySummon : public CRPG_Object_Summon
{
	MRTC_DECLARE;

public:
	int m_Range;
	fp32 m_MoveBack;
	int8 m_SpawnType;

	void OnCreate();
	bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
};


// -------------------------------------------------------------------
//  CRPG_Object_MultiRaySummon
// -------------------------------------------------------------------

class CRPG_Object_MultiRaySummon : public CRPG_Object_Summon
{
	MRTC_DECLARE;

public:
	int m_Range;
	fp32 m_MoveBack;
	int8 m_SpawnType;
	int m_nRays;
	int m_nChunks;
	int m_iChunk;
	int m_ChunkTime;
	int m_ChunkDuration;
	int m_ChunkOwner;
	fp32 m_Scatter;
	fp32 m_ImpactForce;
	fp32 m_EffectRate;

	int m_iSound_Chunk;
	int m_iSound_LRP;		// LRP = Listener ray projection
	int m_iSound_HitWorld;
	int m_iSound_HitChar;

	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual void OnCreate();
	bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void FireChunk(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual bool Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject);
};

// -------------------------------------------------------------------

#endif /* WRPGSpells_h */
