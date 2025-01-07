#ifndef WAGI_Context_h
#define WAGI_Context_h

//--------------------------------------------------------------------------------

class CWObject_CoreData;
class CWorld_PhysState;
class CMapData;

class CWAGI_Context
{
	public:

		CWObject_CoreData* m_pObj;
		CWorld_PhysState* m_pWPhysState;
		CMapData *m_pMapData;
		CMTime m_GameTime;
		fp32 m_TimeSpan;
		bool m_bGameTimeIncluded;

	public:
/*
		CWObject_CoreData* GetObj() const { return m_pObj; }
		CWorld_PhysState* GetPhysState() const { return m_pWPhysState; }
		fp32 GetGameTime() const { return m_GameTime; }
		fp32 GetTimeSpan() const { return m_TimeSpan; }
		bool IsGameTimeIncluded() const { return m_bGameTimeIncluded; }
*/
		CWAGI_Context();
		CWAGI_Context(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CMTime _GameTime, fp32 _TimeSpan = -1.0f, bool _GameTimeIncluded = true);
		CWAGI_Context(const CWAGI_Context& _Context) { _Context.Duplicate(this); };

		// Create a splited context from this context. Target context ptr may be 'this'.
		CWAGI_Context* SplitAtFraction(fp32 _SplitTimeFraction, bool _bGameTimeIncluded, CWAGI_Context* _pSplitContext) const;
		CWAGI_Context* SplitAtTime(CMTime _SplitTime, bool _bGameTimeIncluded, CWAGI_Context* _pSplitContext) const;

		// Create an extended context from this context. Target context ptr may be 'this'.
		CWAGI_Context* Extend(CMTime _ExtendGameTime, CWAGI_Context* _pExtendedContext) const;

		// Create an split OR extended context from this context. Target context ptr may be 'this'.
		CWAGI_Context* StartAtTime(CMTime _StartTime, CWAGI_Context* _pNewContext) const;

		// Create a duplicate of this context. Target context ptr may be 'this'.
		CWAGI_Context* Duplicate(CWAGI_Context* _pTargetContext) const;
};

//--------------------------------------------------------------------------------

#endif /* WAGI_Context_h */
