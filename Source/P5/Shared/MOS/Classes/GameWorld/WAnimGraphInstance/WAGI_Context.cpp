#include "PCH.h"

//--------------------------------------------------------------------------------

#include "WAGI_Context.h"
#include "../WPhysState.h"

//--------------------------------------------------------------------------------

CWAGI_Context::CWAGI_Context()
{
	m_pObj = NULL;
	m_pWPhysState = NULL;
	m_GameTime;
	m_TimeSpan = 0;
	m_bGameTimeIncluded = true;
}

//--------------------------------------------------------------------------------

CWAGI_Context::CWAGI_Context(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CMTime _GameTime, fp32 _TimeSpan, bool _GameTimeIncluded)
{
	m_pObj = _pObj;
	m_pWPhysState = _pWPhysState;
	m_GameTime = _GameTime;
	m_TimeSpan = _TimeSpan;
	m_bGameTimeIncluded = _GameTimeIncluded;

	if (m_TimeSpan < 0.0f)
		m_TimeSpan = _pWPhysState->GetGameTickTime();
}

//--------------------------------------------------------------------------------

CWAGI_Context* CWAGI_Context::SplitAtFraction(fp32 _SplitTimeFraction, bool _bGameTimeIncluded, CWAGI_Context* _pSplitContext) const
{
	// Use copies in case (_pSplitContext == this).
	CMTime GameTime = m_GameTime;
	fp32 TimeSpan = m_TimeSpan;
	_pSplitContext->m_GameTime = GameTime + CMTime::CreateFromSeconds(_SplitTimeFraction * TimeSpan);
	_pSplitContext->m_TimeSpan = TimeSpan - (_pSplitContext->m_GameTime - GameTime).GetTime(); // More exact than *=(1.0f - _TimeFraction), I think =).
	_pSplitContext->m_pObj = m_pObj;
	_pSplitContext->m_pWPhysState = m_pWPhysState;
	_pSplitContext->m_bGameTimeIncluded = _bGameTimeIncluded;

	return _pSplitContext;
}

//--------------------------------------------------------------------------------

CWAGI_Context* CWAGI_Context::SplitAtTime(CMTime _SplitTime, bool _bGameTimeIncluded, CWAGI_Context* _pSplitContext) const
{
	if ((_SplitTime.Compare(m_GameTime) > 0) && (_SplitTime.Compare(m_GameTime + CMTime::CreateFromSeconds(m_TimeSpan)) < 0))
	{
		// Use copies in case (_pSplitContext == this).
		CMTime GameTime = m_GameTime;
		fp32 TimeSpan = m_TimeSpan;
		_pSplitContext->m_GameTime = _SplitTime;
		_pSplitContext->m_TimeSpan = TimeSpan - (_pSplitContext->m_GameTime - GameTime).GetTime();
	}
	else
	{
		_pSplitContext->m_GameTime = m_GameTime;
		_pSplitContext->m_TimeSpan = m_TimeSpan;
	}

	_pSplitContext->m_pObj = m_pObj;
	_pSplitContext->m_pWPhysState = m_pWPhysState;
	_pSplitContext->m_bGameTimeIncluded = _bGameTimeIncluded;

	return _pSplitContext;
}

//--------------------------------------------------------------------------------

CWAGI_Context* CWAGI_Context::Extend(CMTime _ExtendGameTime, CWAGI_Context* _pExtendedContext) const
{
	// Use copies in case (_pExtendedContext == this).
	CMTime GameTime = m_GameTime;
	fp32 TimeSpan = m_TimeSpan;
	if (_ExtendGameTime.Compare(GameTime) <= 0)
	{
		_pExtendedContext->m_GameTime = _ExtendGameTime;
		_pExtendedContext->m_TimeSpan = TimeSpan + (GameTime - _ExtendGameTime).GetTime();
		_pExtendedContext->m_pObj = m_pObj;
		_pExtendedContext->m_pWPhysState = m_pWPhysState;
		_pExtendedContext->m_bGameTimeIncluded = true;
	}
	else if (_ExtendGameTime.Compare(GameTime + CMTime::CreateFromSeconds(TimeSpan)) > 0)
	{
		// Unusual case, since one should really not extend into the future.
		_pExtendedContext->m_GameTime = GameTime;
		_pExtendedContext->m_TimeSpan = (_ExtendGameTime - GameTime).GetTime();
		_pExtendedContext->m_pObj = m_pObj;
		_pExtendedContext->m_pWPhysState = m_pWPhysState;
		_pExtendedContext->m_bGameTimeIncluded = m_bGameTimeIncluded;
	}
	else
	{
		// _ExtendGameTime already included.
		*_pExtendedContext = *this;
	}

	return _pExtendedContext;
}

//--------------------------------------------------------------------------------

CWAGI_Context* CWAGI_Context::StartAtTime(CMTime _StartTime, CWAGI_Context* _pNewContext) const
{
	if (_StartTime.Compare(m_GameTime) < 0)
		return Extend(_StartTime, _pNewContext);
	else
		return SplitAtTime(_StartTime, true, _pNewContext);
}

//--------------------------------------------------------------------------------

CWAGI_Context* CWAGI_Context::Duplicate(CWAGI_Context* _pTargetContext) const
{
	_pTargetContext->m_GameTime = m_GameTime;
	_pTargetContext->m_TimeSpan = m_TimeSpan;
	_pTargetContext->m_pObj = m_pObj;
	_pTargetContext->m_pWPhysState = m_pWPhysState;
	_pTargetContext->m_pMapData = m_pMapData;
	_pTargetContext->m_bGameTimeIncluded = m_bGameTimeIncluded;
	return _pTargetContext;
}

//--------------------------------------------------------------------------------
