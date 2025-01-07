#include "PCH.h"

//--------------------------------------------------------------------------------

#include "WAG2I_Context.h"
#include "../WPhysState.h"

//--------------------------------------------------------------------------------

CWAG2I_Context::CWAG2I_Context()
{
	m_pObj = NULL;
	m_pWPhysState = NULL;
	m_pWorldData = NULL;
//	m_GameTime;
	m_TimeSpan = 0;
}

//--------------------------------------------------------------------------------

CWAG2I_Context::CWAG2I_Context(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CMTime _GameTime, fp32 _TimeSpan)
{
	m_pObj = _pObj;
	m_pWPhysState = _pWPhysState;
	m_GameTime = _GameTime;
	m_TimeSpan = _TimeSpan;

	if (_pWPhysState)
	{
		m_pWorldData = m_pWPhysState->GetWorldData();
		if (m_TimeSpan <= 0.0f)
			m_TimeSpan = _pWPhysState->GetGameTickTime();
	}
}

//--------------------------------------------------------------------------------

CWAG2I_Context* CWAG2I_Context::SplitAtFraction(fp32 _SplitTimeFraction, bool _bGameTimeIncluded, CWAG2I_Context* _pSplitContext) const
{
	// Use copies in case (_pSplitContext == this).
	CMTime GameTime = m_GameTime;
	fp32 TimeSpan = m_TimeSpan;
	_pSplitContext->m_GameTime = GameTime + CMTime::CreateFromSeconds(_SplitTimeFraction * TimeSpan);
	_pSplitContext->m_TimeSpan = TimeSpan - (_pSplitContext->m_GameTime - GameTime).GetTime(); // More exact than *=(1.0f - _TimeFraction), I think =).
	_pSplitContext->m_pObj = m_pObj;
	_pSplitContext->m_pWPhysState = m_pWPhysState;
	_pSplitContext->m_pWorldData = m_pWorldData;

	return _pSplitContext;
}

//--------------------------------------------------------------------------------

CWAG2I_Context* CWAG2I_Context::SplitAtTime(CMTime _SplitTime, bool _bGameTimeIncluded, CWAG2I_Context* _pSplitContext) const
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
	_pSplitContext->m_pWorldData = m_pWorldData;

	return _pSplitContext;
}

//--------------------------------------------------------------------------------

CWAG2I_Context* CWAG2I_Context::Extend(CMTime _ExtendGameTime, CWAG2I_Context* _pExtendedContext) const
{
	// Use copies in case (_pExtendedContext == this).
	CMTime GameTime = m_GameTime;
	fp32 TimeSpan = m_TimeSpan;
	_pExtendedContext->m_pObj = m_pObj;
	_pExtendedContext->m_pWPhysState = m_pWPhysState;
	_pExtendedContext->m_pWorldData = m_pWorldData;
	if (_ExtendGameTime.Compare(GameTime) <= 0)
	{
		_pExtendedContext->m_GameTime = _ExtendGameTime;
		_pExtendedContext->m_TimeSpan = TimeSpan + (GameTime - _ExtendGameTime).GetTime();
	}
	else if (_ExtendGameTime.Compare(GameTime + CMTime::CreateFromSeconds(TimeSpan)) > 0)
	{
		// Unusual case, since one should really not extend into the future.
		_pExtendedContext->m_GameTime = GameTime;
		_pExtendedContext->m_TimeSpan = (_ExtendGameTime - GameTime).GetTime();
	}
	else
	{
		// _ExtendGameTime already included.
		*_pExtendedContext = *this;
	}

	return _pExtendedContext;
}

//--------------------------------------------------------------------------------

CWAG2I_Context* CWAG2I_Context::StartAtTime(CMTime _StartTime, CWAG2I_Context* _pNewContext) const
{
	if (_StartTime.Compare(m_GameTime) < 0)
		return Extend(_StartTime, _pNewContext);
	else
		return SplitAtTime(_StartTime, true, _pNewContext);
}

//--------------------------------------------------------------------------------

CWAG2I_Context* CWAG2I_Context::Duplicate(CWAG2I_Context* _pTargetContext) const
{
	_pTargetContext->m_GameTime = m_GameTime;
	_pTargetContext->m_TimeSpan = m_TimeSpan;
	_pTargetContext->m_pObj = m_pObj;
	_pTargetContext->m_pWPhysState = m_pWPhysState;
	_pTargetContext->m_pWorldData = m_pWorldData;
	return _pTargetContext;
}

//--------------------------------------------------------------------------------
