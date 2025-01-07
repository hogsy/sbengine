#ifndef WAG2I_Context_h
#define WAG2I_Context_h

//--------------------------------------------------------------------------------

class CWObject_CoreData;
class CWorld_PhysState;
class CMapData;

class CWAG2I_Context
{
	public:

		CMTime m_GameTime;
		CWObject_CoreData* m_pObj;
		CWorld_PhysState* m_pWPhysState;
		CWorldData* m_pWorldData;
		fp32 m_TimeSpan;

	public:
		CWAG2I_Context();
		CWAG2I_Context(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CMTime _GameTime, fp32 _TimeSpan = -1.0f);
		CWAG2I_Context(const CWAG2I_Context& _Context) { _Context.Duplicate(this); };

		// Create a split context from this context. Target context ptr may be 'this'.
		CWAG2I_Context* SplitAtFraction(fp32 _SplitTimeFraction, bool _bGameTimeIncluded, CWAG2I_Context* _pSplitContext) const;
		CWAG2I_Context* SplitAtTime(CMTime _SplitTime, bool _bGameTimeIncluded, CWAG2I_Context* _pSplitContext) const;

		// Create an extended context from this context. Target context ptr may be 'this'.
		CWAG2I_Context* Extend(CMTime _ExtendGameTime, CWAG2I_Context* _pExtendedContext) const;

		// Create an split OR extended context from this context. Target context ptr may be 'this'.
		CWAG2I_Context* StartAtTime(CMTime _StartTime, CWAG2I_Context* _pNewContext) const;

		// Create a duplicate of this context. Target context ptr may be 'this'.
		CWAG2I_Context* Duplicate(CWAG2I_Context* _pTargetContext) const;
};

//--------------------------------------------------------------------------------

#endif /* WAG2I_Context_h */
