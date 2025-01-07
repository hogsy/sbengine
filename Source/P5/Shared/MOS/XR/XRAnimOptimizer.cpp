#include "PCH.h"
#include "XRAnim.h"
#include "XRAnimOptimizer.h"

#ifndef PLATFORM_CONSOLE

#define WORST_ANIM_ERROR	1000000.0f

// Find a similar fitness value for translational keys, but don't use the list of weights
static void QSpline(CQuatfp32& _QuatA0, CQuatfp32& _QuatA1, CQuatfp32& _QuatA2,
	 						 CQuatfp32& _QuatB0, CQuatfp32& _QuatB1, CQuatfp32& _QuatB2, 
							 CQuatfp32& _Dest, fp32 _tFrac, fp32 _tA0, fp32 _tA1, fp32 _tB0, fp32 _tB1)
{
	fp32 tSqr = Sqr(_tFrac);
	fp32 tCube = tSqr * _tFrac;

	fp32 k = 0.5f;
	fp32 tsA0 = k * _tA1 / _tA0;
	fp32 tsA1 = k * _tA1 / _tA1;
	fp32 tsB0 = k * _tA1 / _tB0;
	fp32 tsB1 = k * _tA1 / _tB1;

	// dQuatA
	fp32 dQA[4];
	if (_QuatA0.DotProd(_QuatA1) < 0.0f)
	{
		for(int i = 0; i < 4; i++)
			dQA[i] = -(_QuatA1.k[i] + _QuatA0.k[i]) * tsA0;
	}
	else
	{
		for(int i = 0; i < 4; i++)
			dQA[i] = (_QuatA1.k[i] - _QuatA0.k[i]) * tsA0;
	}

	if (_QuatA2.DotProd(_QuatA1) < 0.0f)
	{
		for(int i = 0; i < 4; i++)
			dQA[i] += -(_QuatA2.k[i] + _QuatA1.k[i]) * tsA1;
	}
	else
	{
		for(int i = 0; i < 4; i++)
			dQA[i] += (_QuatA2.k[i] - _QuatA1.k[i]) * tsA1;
	}

	// dQuatB
	fp32 dQB[4];
	if (_QuatB0.DotProd(_QuatB1) < 0.0f)
	{
		for(int i = 0; i < 4; i++)
			dQB[i] = -(_QuatB1.k[i] + _QuatB0.k[i]) * tsB0;
	}
	else
	{
		for(int i = 0; i < 4; i++)
			dQB[i] = (_QuatB1.k[i] - _QuatB0.k[i]) * tsB0;
	}

	if (_QuatB2.DotProd(_QuatB1) < 0.0f)
	{
		for(int i = 0; i < 4; i++)
			dQB[i] += -(_QuatB2.k[i] + _QuatB1.k[i]) * tsB1;
	}
	else
	{
		for(int i = 0; i < 4; i++)
			dQB[i] += (_QuatB2.k[i] - _QuatB1.k[i]) * tsB1;
	}

	if (_QuatA1.DotProd(_QuatB1) < 0.0f)
	{
		// Spline it, neg
		for(int i = 0; i < 4; i++)
		{
			fp32 v0 = dQA[i];
			fp32 v1 = -dQB[i];
			fp32 p0 = _QuatA1.k[i];
			fp32 p1 = -_QuatB1.k[i];
			fp32 D = p0;
			fp32 C = v0;
			fp32 B = 3.0f*(p1 - D) - (2.0f*v0) - v1;
			fp32 A = -(2.0f * B + v0 - v1) / 3.0f;
			_Dest.k[i] = A*tCube + B*tSqr + C*_tFrac + D;
		}
	}
	else
	{
		// Spline it
		for(int i = 0; i < 4; i++)
		{
			fp32 v0 = dQA[i];
			fp32 v1 = dQB[i];
			fp32 p0 = _QuatA1.k[i];
			fp32 p1 = _QuatB1.k[i];
			fp32 D = p0;
			fp32 C = v0;
			fp32 B = 3.0f*(p1 - D) - (2.0f*v0) - v1;
			fp32 A = -(2.0f * B + v0 - v1) / 3.0f;
			_Dest.k[i] = A*tCube + B*tSqr + C*_tFrac + D;
		}
	}

	_Dest.Normalize();
}

static void VSpline(CVec3Dfp32& _MoveA0, CVec3Dfp32& _MoveA1, CVec3Dfp32& _MoveA2,
						 CVec3Dfp32& _MoveB0, CVec3Dfp32& _MoveB1, CVec3Dfp32& _MoveB2, 
						 CVec3Dfp32& _Dest, fp32 _tFrac, fp32 _tA0, fp32 _tA1, fp32 _tB0, fp32 _tB1)
{
	fp32 tSqr = Sqr(_tFrac);
	fp32 tCube = tSqr * _tFrac;

	fp32 k = 0.5f;
	fp32 tsA0 = k * _tA1 / _tA0;
	fp32 tsA1 = k * _tA1 / _tA1;
	fp32 tsB0 = k * _tA1 / _tB0;
	fp32 tsB1 = k * _tA1 / _tB1;

	// dQuatA
	CVec3Dfp32 dMA = (_MoveA1 - _MoveA0) * tsA0;
	dMA += (_MoveA2 - _MoveA1) * tsA1;

	CVec3Dfp32 dMB = (_MoveB1 - _MoveB0) * tsB0;
	dMB += (_MoveB2 - _MoveB1) * tsB1;


	// Spline it
	for(int i = 0; i < 3; i++)
	{
		fp32 v0 = dMA.k[i];
		fp32 v1 = dMB.k[i];
		fp32 p0 = _MoveA1.k[i];
		fp32 p1 = _MoveB1.k[i];
		fp32 D = p0;
		fp32 C = v0;
		fp32 B = 3.0f*(p1 - D) - (2.0f*v0) - v1;
		fp32 A = -(2.0f * B + v0 - v1) / 3.0f;
		_Dest.k[i] = A*tCube + B*tSqr + C*_tFrac + D;
	}
}

CAnimationOptimizer::CAnimationOptimizer()
{
	m_pAnimSequence = NULL;
	m_plKeys = NULL;
	m_KeyCount= 0;
}

CAnimationOptimizer::~CAnimationOptimizer()
{
	m_lOrgKeys.Clear();

	m_lKeyTimes.Clear();
	m_lKeyTimeDeltas.Clear();
	m_lCullOrder.Clear();
	m_lspCullKeys.Clear();
	m_lErrorWeights.Clear();
	m_lOptimized.Clear();
	m_lErrors.Clear();
}

void CAnimationOptimizer::Setup(CXR_Anim_Sequence *_pSeq)
{
	// Clear out any old data
	m_lOrgKeys.Clear();
	m_lOrgKeys.SetGrow(1000);

	m_lKeyTimes.Clear();
	m_lKeyTimes.SetGrow(1000);

	m_lKeyTimeDeltas.Clear();
	m_lKeyTimeDeltas.SetGrow(1000);

	m_lCullOrder.Clear();
	m_lCullOrder.SetGrow(1000);

	m_lspCullKeys.Clear();
	m_lspCullKeys.SetGrow(1000);

	m_lErrorWeights.Clear();
	m_lErrorWeights.SetGrow(1000);

	m_lOptimized.Clear();
	m_lOptimized.SetGrow(1000);

	m_lErrors.Clear();
	m_lErrors.SetGrow(1000);

	m_pAnimSequence = _pSeq;
	m_plKeys = &(m_pAnimSequence->m_lspKeys);
	// (re) initialize the lists
	m_KeyCount = m_plKeys->Len();
	if (m_KeyCount < 7)
	{
		LogFile("CAnimationOptimizer::Setup was called on a track with less than 7 keys");
		return;
	}

	for (int32 i = 0; i < m_KeyCount; i++)
	{
		m_lOrgKeys.Add((*m_plKeys)[i]);
		m_lKeyTimeDeltas.Add(_pSeq->GetFrameDuration(i));
		// m_lKeyTimeDeltas.Add((*m_plKeys)[i]->m_Data.m_Time);
		m_lKeyTimes.Add((*m_plKeys)[i]->m_AbsTime);
		m_lOptimized.Add(false);
		m_lErrors.Add(0.0f);
	}
	m_KeyCount = m_lOrgKeys.Len();
	m_RotChannels = m_lOrgKeys[0]->m_lRotKeys.Len();
	m_TransChannels = m_lOrgKeys[0]->m_lMoveKeys.Len();
	m_MaxChannels = m_RotChannels;
	if (m_TransChannels > m_MaxChannels)
	{	// However unlikeley it may be
		// (A worm with arthritis juggling balls...)
		m_MaxChannels = m_TransChannels;
	}
}

// Returns the number of keys removed (negative values indicate errors)
int32 CAnimationOptimizer::Optimize(TArray<fp32>& _lWeights)
{
	MSCOPESHORT(CAnimationOptimizer::Optimize);

	int32 i;

	if (m_KeyCount < 7)
	{	// We cannot optimize near the edges of splines for continuity reasons
		LogFile("CAnimationOptimizer::Optimize was called on a track with less than 7 keys");
		return(-1);
	}

	m_lCullOrder.Clear();
	m_lspCullKeys.Clear();
	m_lErrorWeights.Clear();
	m_lOptimized.Clear();
	m_lErrors.Clear();
	int32 keysToOptimize = 0;
	for (i = 0; i < m_KeyCount; i++)
	{
		{
			m_lOptimized.Add(false);
			keysToOptimize++;
		}

		m_lErrors.Add(0.0f);
	}

	if (keysToOptimize < 7)
	{	// No keys can be removed
		// We need 3 left, 3 right and one more
		return(0);
	}

	// Get one key, check the number of channels and fill m_ErrorWeights with that
	int32 channelCount = m_lOrgKeys[0]->m_lRotKeys.Len();
	if (m_lOrgKeys[0]->m_lMoveKeys.Len() > channelCount)
	{
		channelCount = m_lOrgKeys[0]->m_lMoveKeys.Len();
	}
	for (i = 0; i < channelCount; i++)
	{
		if (i < _lWeights.Len())
		{
			m_lErrorWeights.Add(_lWeights[i]);
		}
		else
		{
			m_lErrorWeights.Add(1.0f);
		}
	}

	// Calculate initial error-values
	for(i = 3;i < m_KeyCount-3;i++)
	{
		m_lErrors[i] = Measure(i);
	}

	// OK, all preparations have been done, lets start optimizing
	int optimizedCount = 0;
	bool* pOptimized = m_lOptimized.GetBasePtr();
	fp32* pErrors = m_lErrors.GetBasePtr();
	uint nKeyCount = m_KeyCount;

	do
	{
		//*
		fp32 bestErr = WORST_ANIM_ERROR; 
		int bestIndex = -1;

		// Not very pretty to loop through all keys every iteration...
		// Might be possible to optimize further with a sorted error-value array,
		// Although the constant re-sorting of the items proved to be innefficient when I tried
		for (i = 3; i < nKeyCount-3; i++)
		{
			if (pOptimized[i]) continue;
			// fp32 curErr = Measure(i);
			fp32 curErr = pErrors[i];
			if (curErr < bestErr)
			{
				bestErr = curErr;
				bestIndex = i;
			}
		}
		//*/

		if (bestIndex != -1)
		{	// There was indeed an index to remove
			m_lCullOrder.Add(bestIndex);
			m_lspCullKeys.Add(m_lOrgKeys[bestIndex]);
			pOptimized[bestIndex] = true;
			// m_lErrors[bestIndex] = bestErr;
			optimizedCount++;

			//Recount dependents
			uint nFixed = 0;
			for(i = bestIndex-1;i >= 3;i--)
			{
				if (!pOptimized[i])
				{
					pErrors[i] = Measure(i);
					if (++nFixed >= 2)
						break;
				}
			}
			nFixed = 0;
			for(i = bestIndex+1;i < m_KeyCount-3;i++)
			{
				if (!pOptimized[i])
				{
					pErrors[i] = Measure(i);
					if (++nFixed >= 2)
						break;
				}
			}
		}
		else
		{	// No key was removed so we bail
			break;
		}
	} while(optimizedCount < m_KeyCount-6);

	return(optimizedCount);
}

TArray<int32> g_keysToCheck;

// Returns the max error we get from removing key _index
fp32 CAnimationOptimizer::Measure(int32 _index)
{
	MSCOPESHORT(CAnimationOptimizer::Measure);

	// PSEUDO
	// Retrieve the unoptimized keys to left and right of _index
	// This is done by stepping left/right until an unoptimized key is found (Duh)
	// Calculate t fraction for the missing key
	// Interpolate left1,left2,right1,right2 at t for quat value if key _index is removed
	// Measure angular distance between key and interKey (dot, change sign and redo if negative)
	// Multiply by _weight and 2pi
	// Interpolate similarly for translation and mesure distance, don't weight trans diff
	// Repeat process for all 'channels' in key and returns the highest (worst) value

	int32 i,j;
	
	// Indices to keyframes we use for interpolation
	int32 iA0,iA1,iA2,iB0,iB1,iB2;
	iA0 = iA1 = iA2 = iB0 = iB1 = iB2 = -1;

	bool* pOptimized = m_lOptimized.GetBasePtr();

	g_keysToCheck.QuickSetLen(m_KeyCount);
	int32* keysToCheck = g_keysToCheck.GetBasePtr();
	keysToCheck[0] = _index;
	uint nKeysToCheck = 1;

	for (i = _index - 1; i >= 0; i--)
	{
		if (pOptimized[i] == true)
		{
			keysToCheck[nKeysToCheck++] = i;
			continue;
		}

		// Search left for keys
		if (iA1 == -1)
		{
			iA1 = i;
			continue;
		}
		if (iA0 == -1)
		{
			iA0 = i;
			break;
		}
	}
	for (i = _index + 1; i < m_KeyCount; i++)
	{
		if (pOptimized[i] == true)
		{
			keysToCheck[nKeysToCheck++] = i;
			continue;
		}

		// Search right for keys
		if (iB1 == -1)
		{
			iB1 = i;
			continue;
		}
		if (iB2 == -1)
		{
			iB2 = i;
			break;
		}
	}

	if ((iA0 == -1)||
		(iA1 == -1)||
		(iB1 == -1)||
		(iB2 == -1))
	{	// We return an error so bad we're guaranteed the key won't be culled
		return(WORST_ANIM_ERROR);
	}

	iA2 = iB1;
	iB0 = iA1;

	spCXR_Anim_Keyframe pKeyA0 = m_lOrgKeys[iA0];
	spCXR_Anim_Keyframe pKeyA1 = m_lOrgKeys[iA1];
	spCXR_Anim_Keyframe pKeyA2 = m_lOrgKeys[iA2];
	spCXR_Anim_Keyframe pKeyB0 = m_lOrgKeys[iB0];
	spCXR_Anim_Keyframe pKeyB1 = m_lOrgKeys[iB1];
	spCXR_Anim_Keyframe pKeyB2 = m_lOrgKeys[iB2];

	fp32 tA01 = pKeyA1->m_AbsTime - pKeyA0->m_AbsTime;
	fp32 tA12 = pKeyA2->m_AbsTime - pKeyA1->m_AbsTime;
	fp32 tB01 = pKeyB1->m_AbsTime - pKeyB0->m_AbsTime;
	fp32 tB12 = pKeyB2->m_AbsTime - pKeyB1->m_AbsTime;

	fp32 curRot,curMove,curErr;
	fp32 worstErr = 0.0f;
	int32 worstIndex = -1;

	// Iterate through all keys in the span (not only _index itself)
	// and find the worst error.
	fp32* pKeyTimes = m_lKeyTimes.GetBasePtr();
	for (int32 key = 0; key < nKeysToCheck; key++)
	{	// tNorm is the fractional (0.0 to 1.0) time between the two keys iA1 to iB1
		int32 curKey = keysToCheck[key];
		const CXR_Anim_Keyframe* curKeyToCheck = m_lOrgKeys[curKey];
		fp32 tNorm = (pKeyTimes[curKey] - pKeyTimes[iA1]) / (pKeyTimes[iB1] - pKeyTimes[iA1]);

		CQuatfp32 qA0,qA1,qA2,qB0,qB1,qB2;

		for (j = 0; j < m_MaxChannels; j++)
		{
			CQuatfp32 qRemove,q;
			CVec3Dfp32 vRemove,v;

			// Now that we have the left and right keys, it's time
			// to interpolate the keys for rotation and position
			// No but wait! We must measure every culled or potentially culled key between iA1 and iB1
			curRot = 0.0f;
			if (j < m_RotChannels)
			{
				pKeyA0->m_lRotKeys[j].GetRot(qA0);
				pKeyA1->m_lRotKeys[j].GetRot(qA1);
				pKeyA2->m_lRotKeys[j].GetRot(qA2);
				pKeyB0->m_lRotKeys[j].GetRot(qB0);
				pKeyB1->m_lRotKeys[j].GetRot(qB1);
				pKeyB2->m_lRotKeys[j].GetRot(qB2);
				curKeyToCheck->m_lRotKeys[j].GetRot(qRemove);
				QSpline(qA0,qA1,qA2,qB0,qB1,qB2,q,tNorm,tA01,tA12,tB01,tB12);

				// Determine the error between gRemove and q
				fp32 temp = qRemove.DotProd(q);
				if (temp < 0)
				{	// Wrong hyperarc
					// Why is there no negation operator for quaternions in the engine?
					q.k[0] = -q.k[0];
					q.k[1] = -q.k[1];
					q.k[2] = -q.k[2];
					q.k[3] = -q.k[3];
					temp = qRemove.DotProd(q);
				}
				curRot = (1.0f - temp) * m_lErrorWeights[j] * _PI;
			}
		
			curMove = 0.0f;
			if (j < m_TransChannels)
			{
				vRemove = curKeyToCheck->m_lMoveKeys[j].m_Move;
				VSpline(
					pKeyA0->m_lMoveKeys[j].m_Move,pKeyA1->m_lMoveKeys[j].m_Move,pKeyA2->m_lMoveKeys[j].m_Move, 
					pKeyB0->m_lMoveKeys[j].m_Move,pKeyB1->m_lMoveKeys[j].m_Move,pKeyB2->m_lMoveKeys[j].m_Move,
					v, tNorm, tA01, tA12, tB01, tB12);

				// Determine the error between vRemove and v
				curMove = vRemove.Distance(v);
			}

			if (curRot > curMove)
			{
				curErr = curRot;
			}
			else
			{
				curErr = curMove;
			}

			if (curErr > worstErr)
			{
				worstErr = curErr;
				worstIndex = curKey;
			}
		}
	}	// End of 'for (int32 key = 0; key < keysToCheck.Len(); key++)'

	return(worstErr);
}

fp32 CAnimationOptimizer::Cull(fp32 _maxError,fp32 _cullFactor)
{
	int32 i;
	fp32 lastErr = 0.0f;
	int32 orgKeyCount = m_lOrgKeys.Len();
	
	if (m_lCullOrder.Len() == 0)
	{
		LogFile("CAnimationOptimizer::Cull was called with no keys to remove");
		return(0.0f);
	}

	// First we copy all keys to m_lspKeys
	// and the nwe start culling them.
	m_plKeys->Clear();
	for (i = 0; i < orgKeyCount; i++)
	{
		m_plKeys->Add(m_lOrgKeys[i]);
	}

	for (i = 0; i < m_lCullOrder.Len(); i++)
	{
		int32 startIndex = m_lCullOrder[i];
		fp32 Err = m_lErrors[startIndex];
		if (/*(_maxError > 0)&&*/(Err > _maxError))
		{
			break;
		}
		if (m_plKeys->Len() <= (1.0f - _cullFactor) * fp32(orgKeyCount))
		{
			break;
		}

		if (startIndex >= m_plKeys->Len())
		{
			startIndex = m_plKeys->Len() - 1;
		}
		for (int32 j = startIndex; j >= 1; j--)
		{
			if ((*m_plKeys)[j] == m_lspCullKeys[i])
			{	
				// Found the key!
				(*m_plKeys).Del(j);
				lastErr = Err;
				break;
			}
		}
	}

	return(lastErr);
}

// =====================
// CAnim_TracksOptimizer
// =====================

// Comparison function used to qsort errors
int CAnim_TracksOptimizer::CompareError(const void* _FirstElem,const void* _SecondElem)
{
	// Assume we got ptrs to CAnim_Error
	CAnim_KeyValue* pFirst = (CAnim_KeyValue*)_FirstElem;
	CAnim_KeyValue* pSecond = (CAnim_KeyValue*)_SecondElem;

	if (pFirst->m_Error >= pSecond->m_Error)
	{
		return(1);
	}
	else
	{
		return(-1);
	}
}

// Comparison function used to qsort time
int CAnim_TracksOptimizer::CompareTime(const void* _FirstElem,const void* _SecondElem)
{
	// Assume we got ptrs to CAnim_Error
	CAnim_KeyValue* pFirst = (CAnim_KeyValue*)_FirstElem;
	CAnim_KeyValue* pSecond = (CAnim_KeyValue*)_SecondElem;

	if (pFirst->m_Time >= pSecond->m_Time)
	{
		return(1);
	}
	else
	{
		return(-1);
	}
}

void CAnim_TracksOptimizer::QSortByError(TArray<CAnim_KeyValue>& _lKeys)
{
	int N = _lKeys.Len();
	CAnim_KeyValue* pKeys = _lKeys.GetBasePtr();

	qsort(pKeys,N,sizeof(CAnim_KeyValue),&CompareError);
}

void CAnim_TracksOptimizer::QSortByTime(TArray<CAnim_KeyValue>& _lKeys)
{
	int N = _lKeys.Len();
	CAnim_KeyValue* pKeys = _lKeys.GetBasePtr();

	qsort(pKeys,N,sizeof(CAnim_KeyValue),&CompareTime);
}

CAnim_TracksOptimizer::CAnim_TracksOptimizer()
{
}

CAnim_TracksOptimizer::~CAnim_TracksOptimizer()
{
	Clear();
}

void CAnim_TracksOptimizer::Clear()
{
	m_lKeys.Clear();
	m_lRemainingKeys.Clear();
}

void CAnim_TracksOptimizer::Setup(spCXR_Anim_SequenceTracks _spSeq,bool _LogFlag)
{
	Clear();
	m_spOrgAnimSequence = _spSeq;
	m_LogFlag = _LogFlag;
}

bool CAnim_TracksOptimizer::RotCullError(uint16 _iKey,TArray<CAnim_KeyValue*>& _lpKeys,fp32* _pError)
{
	if ((_iKey < 2)||(_iKey >= _lpKeys.Len()-2))
	{
		return(false);
	}

	fp32 Fraction;
	CQuatfp32 QA0,QA1,QA2,QCur,QB0,QB1,QB2;
	QA0 = _lpKeys[_iKey-2]->m_Rot;
	QA1 = _lpKeys[_iKey-1]->m_Rot;
	QA2 = _lpKeys[_iKey+1]->m_Rot;
	QCur = _lpKeys[_iKey]->m_Rot;
	QB0 = _lpKeys[_iKey-1]->m_Rot;
	QB1 = _lpKeys[_iKey+1]->m_Rot;
	QB2 = _lpKeys[_iKey+2]->m_Rot;

	fp32 Duration01 = _lpKeys[_iKey-1]->m_Time - _lpKeys[_iKey-2]->m_Time;
	fp32 Duration12 = _lpKeys[_iKey+1]->m_Time - _lpKeys[_iKey-1]->m_Time;
	fp32 Duration23 = _lpKeys[_iKey+2]->m_Time - _lpKeys[_iKey+1]->m_Time;
	Fraction = (_lpKeys[_iKey]->m_Time - _lpKeys[_iKey-1]->m_Time) / Duration12;

	QSpline(QA0,QA1,QA2,QB0,QB1,QB2,QCur,
		Fraction,Duration01,Duration12,Duration12,Duration23);
	fp32 temp = QCur.DotProd(_lpKeys[_iKey]->m_Rot);
	if (temp < 0)
	{	// Wrong hyperarc
		// Why is there no negation operator for quaternions in the engine?
		QCur.k[0] = -QCur.k[0];
		QCur.k[1] = -QCur.k[1];
		QCur.k[2] = -QCur.k[2];
		QCur.k[3] = -QCur.k[3];
		temp = QCur.DotProd(_lpKeys[_iKey]->m_Rot);
	}
	if (temp > 1.0f)
	{
		temp = 1.0f;
	}
	*_pError = (1.0f - temp) * _PI;

	return(true);
}

bool CAnim_TracksOptimizer::RotCullError(uint16 _iKey,TArray<CAnim_KeyValue>& _lpKeys,fp32* _pError)
{
	if ((_iKey < 2)||(_iKey >= _lpKeys.Len()-2))
	{
		return(false);
	}

	fp32 Fraction;
	CQuatfp32 QA0,QA1,QA2,QCur,QB0,QB1,QB2;
	QA0 = _lpKeys[_iKey-2].m_Rot;
	QA1 = _lpKeys[_iKey-1].m_Rot;
	QA2 = _lpKeys[_iKey+1].m_Rot;
	QCur = _lpKeys[_iKey].m_Rot;
	QB0 = _lpKeys[_iKey-1].m_Rot;
	QB1 = _lpKeys[_iKey+1].m_Rot;
	QB2 = _lpKeys[_iKey+2].m_Rot;

	fp32 Duration01 = _lpKeys[_iKey-1].m_Time - _lpKeys[_iKey-2].m_Time;
	fp32 Duration12 = _lpKeys[_iKey+1].m_Time - _lpKeys[_iKey-1].m_Time;
	fp32 Duration23 = _lpKeys[_iKey+2].m_Time - _lpKeys[_iKey+1].m_Time;
	Fraction = (_lpKeys[_iKey].m_Time - _lpKeys[_iKey-1].m_Time) / Duration12;

	QSpline(QA0,QA1,QA2,QB0,QB1,QB2,QCur,Fraction, 
			Duration01,Duration12,Duration12,Duration23);
	fp32 temp = QCur.DotProd(_lpKeys[_iKey].m_Rot);
	if (temp < 0)
	{	// Wrong hyperarc
		// Why is there no negation operator for quaternions in the engine?
		QCur.k[0] = -QCur.k[0];
		QCur.k[1] = -QCur.k[1];
		QCur.k[2] = -QCur.k[2];
		QCur.k[3] = -QCur.k[3];
		temp = QCur.DotProd(_lpKeys[_iKey].m_Rot);
	}
	if (temp > 1.0f)
	{
		temp = 1.0f;
	}
	*_pError = (1.0f - temp) * _PI;

	return(true);
}

bool CAnim_TracksOptimizer::MoveCullError(uint16 _iKey,TArray<CAnim_KeyValue*>& _lpKeys,fp32* _pError)
{
	// Step through lUnCulled (sorted by error)
	if ((_iKey < 2)||(_iKey >= _lpKeys.Len()-2))
	{
		return(false);
	}

	fp32 Fraction;
	CVec3Dfp32 VA0,VA1,VA2,VCur,VB0,VB1,VB2;
	VA0 = _lpKeys[_iKey-2]->m_Move;
	VA1 = _lpKeys[_iKey-1]->m_Move;
	VA2 = _lpKeys[_iKey+1]->m_Move;
	VCur = _lpKeys[_iKey]->m_Move;
	VB0 = _lpKeys[_iKey-1]->m_Move;
	VB1 = _lpKeys[_iKey+1]->m_Move;
	VB2 = _lpKeys[_iKey+2]->m_Move;

	fp32 Duration01 = _lpKeys[_iKey-1]->m_Time - _lpKeys[_iKey-2]->m_Time;
	fp32 Duration12 = _lpKeys[_iKey+1]->m_Time - _lpKeys[_iKey-1]->m_Time;
	fp32 Duration23 = _lpKeys[_iKey+2]->m_Time - _lpKeys[_iKey+1]->m_Time;
	Fraction = (_lpKeys[_iKey]->m_Time - _lpKeys[_iKey-1]->m_Time) / Duration12;
	
	VSpline(VA0,VA1,VA2,VB0,VB1,VB2,VCur,
		Fraction,Duration01,Duration12,Duration12,Duration23);
	*_pError = VCur.Distance(_lpKeys[_iKey]->m_Move);

	return(true);
}

bool CAnim_TracksOptimizer::MoveCullError(uint16 _iKey,TArray<CAnim_KeyValue>& _lpKeys,fp32* _pError)
{
	// Step through lUnCulled (sorted by error)
	if ((_iKey < 2)||(_iKey >= _lpKeys.Len()-2))
	{
		return(false);
	}

	fp32 Fraction;
	CVec3Dfp32 VA0,VA1,VA2,VCur,VB0,VB1,VB2;
	VA0 = _lpKeys[_iKey-2].m_Move;
	VA1 = _lpKeys[_iKey-1].m_Move;
	VA2 = _lpKeys[_iKey+1].m_Move;
	VCur = _lpKeys[_iKey].m_Move;
	VB0 = _lpKeys[_iKey-1].m_Move;
	VB1 = _lpKeys[_iKey+1].m_Move;
	VB2 = _lpKeys[_iKey+2].m_Move;

	fp32 Duration01 = _lpKeys[_iKey-1].m_Time - _lpKeys[_iKey-2].m_Time;
	fp32 Duration12 = _lpKeys[_iKey+1].m_Time - _lpKeys[_iKey-1].m_Time;
	fp32 Duration23 = _lpKeys[_iKey+2].m_Time - _lpKeys[_iKey+1].m_Time;
	Fraction = (_lpKeys[_iKey].m_Time - _lpKeys[_iKey-1].m_Time) / Duration12;
	
	VSpline(VA0,VA1,VA2,VB0,VB1,VB2,VCur,Fraction, 
			Duration01,Duration12,Duration12,Duration23);
	*_pError = VCur.Distance(_lpKeys[_iKey].m_Move);

	return(true);
}

bool CAnim_TracksOptimizer::MeasureRot(int _iKey,TArray<CAnim_KeyValue>& _lKeys,TArray<bool>& _lbUnCulled,fp32* _pError)
{
	if ((_iKey < 2)||(_iKey >= _lKeys.Len()-2))
	{
		return(false);
	}

	// Find the left right neighbour indices
	int32 nKeys = _lKeys.Len();
	int32 iKeyLeft,iKeyLeftLeft,iKeyRight,iKeyRightRight;
	iKeyLeft = iKeyLeftLeft = iKeyRight = iKeyRightRight = -1;
	for (int iLeft = _iKey-1; iLeft >= 1; iLeft--)
	{
		if (_lbUnCulled[iLeft])
		{
			iKeyLeft = iLeft;
			for (int iLeftLeft = iLeft-1; iLeftLeft >= 0; iLeftLeft--)
			{
				if (_lbUnCulled[iLeftLeft])
				{
					iKeyLeftLeft = iLeftLeft;
					break;
				}
			}
			break;
		}
	}
	for (int iRight = _iKey+1; iRight < nKeys-1; iRight++)
	{
		if (_lbUnCulled[iRight])
		{
			iKeyRight = iRight;
			for (int iRightRight = iRight+1; iRightRight < nKeys; iRightRight++)
			{
				if (_lbUnCulled[iRightRight])
				{
					iKeyRightRight = iRightRight;
					break;
				}
			}
			break;
		}
	}

	if ((iKeyLeftLeft == -1)||(iKeyLeft == -1)||(iKeyRight == -1)||(iKeyRightRight == -1))
	{
		return(false);
	}

	fp32 Fraction;
	CQuatfp32 QA0,QA1,QA2,QOrg,QB0,QB1,QB2;
	QA0 = _lKeys[iKeyLeftLeft].m_Rot;
	QA1 = _lKeys[iKeyLeft].m_Rot;
	QA2 = _lKeys[iKeyRight].m_Rot;
	QOrg = _lKeys[_iKey].m_Rot;
	QB0 = _lKeys[iKeyLeft].m_Rot;
	QB1 = _lKeys[iKeyRight].m_Rot;
	QB2 = _lKeys[iKeyRightRight].m_Rot;

	fp32 Duration01 = _lKeys[iKeyLeft].m_Time - _lKeys[iKeyLeftLeft].m_Time;
	fp32 Duration12 = _lKeys[iKeyRight].m_Time - _lKeys[iKeyLeft].m_Time;
	fp32 Duration23 = _lKeys[iKeyRightRight].m_Time - _lKeys[iKeyRight].m_Time;
	Fraction = (_lKeys[_iKey].m_Time - _lKeys[iKeyLeft].m_Time) / Duration12;

	CQuatfp32 QCulled;
	QSpline(QA0,QA1,QA2,QB0,QB1,QB2,QCulled,Fraction, 
			Duration01,Duration12,Duration12,Duration23);
	fp32 temp = QCulled.DotProd(QOrg);
	if (temp < 0)
	{	// Wrong hyperarc
		// Why is there no negation operator for quaternions in the engine?
		QCulled.k[0] = -QCulled.k[0];
		QCulled.k[1] = -QCulled.k[1];
		QCulled.k[2] = -QCulled.k[2];
		QCulled.k[3] = -QCulled.k[3];
		temp = QCulled.DotProd(QOrg);
	}
	if (temp > 1.0f)
	{
		temp = 1.0f;
	}
	*_pError = (1.0f - temp) * _PI;

	return(true);
}

bool CAnim_TracksOptimizer::MeasureMove(int _iKey,TArray<CAnim_KeyValue>& _lKeys,TArray<bool>& _lbUnCulled,fp32* _pError)
{
	if ((_iKey < 2)||(_iKey >= _lKeys.Len()-2))
	{
		return(false);
	}

	// Find the left right neighbour indices
	int32 nKeys = _lKeys.Len();
	int32 iKeyLeft,iKeyLeftLeft,iKeyRight,iKeyRightRight;
	iKeyLeft = iKeyLeftLeft = iKeyRight = iKeyRightRight = -1;
	for (int iLeft = _iKey-1; iLeft >= 1; iLeft--)
	{
		if (_lbUnCulled[iLeft])
		{
			iKeyLeft = iLeft;
			for (int iLeftLeft = iLeft-1; iLeftLeft >= 0; iLeftLeft--)
			{
				if (_lbUnCulled[iLeftLeft])
				{
					iKeyLeftLeft = iLeftLeft;
					break;
				}
			}
			break;
		}
	}
	for (int iRight = _iKey+1; iRight < nKeys-1; iRight++)
	{
		if (_lbUnCulled[iRight])
		{
			iKeyRight = iRight;
			for (int iRightRight = iRight+1; iRightRight < nKeys; iRightRight++)
			{
				if (_lbUnCulled[iRightRight])
				{
					iKeyRightRight = iRightRight;
					break;
				}
			}
			break;
		}
	}

	if ((iKeyLeftLeft == -1)||(iKeyLeft == -1)||(iKeyRight == -1)||(iKeyRightRight == -1))
	{
		return(false);
	}

	fp32 Fraction;
	CVec3Dfp32 VA0,VA1,VA2,VOrg,VB0,VB1,VB2;
	VA0 = _lKeys[iKeyLeftLeft].m_Move;
	VA1 = _lKeys[iKeyLeft].m_Move;
	VA2 = _lKeys[iKeyRight].m_Move;
	VOrg = _lKeys[_iKey].m_Move;
	VB0 = _lKeys[iKeyLeft].m_Move;
	VB1 = _lKeys[iKeyRight].m_Move;
	VB2 = _lKeys[iKeyRightRight].m_Move;

	fp32 Duration01 = _lKeys[iKeyLeft].m_Time - _lKeys[iKeyLeftLeft].m_Time;
	fp32 Duration12 = _lKeys[iKeyRight].m_Time - _lKeys[iKeyLeft].m_Time;
	fp32 Duration23 = _lKeys[iKeyRightRight].m_Time - _lKeys[iKeyRight].m_Time;
	Fraction = (_lKeys[_iKey].m_Time - _lKeys[iKeyLeft].m_Time) / Duration12;
	
	CVec3Dfp32 VCulled;
	VSpline(VA0,VA1,VA2,VB0,VB1,VB2,VCulled,Fraction, 
			Duration01,Duration12,Duration12,Duration23);
	*_pError = VCulled.Distance(VOrg);

	return(true);
}

void CAnim_TracksOptimizer::CalcErrors(TArray<fp32>& _lRotWeights,TArray<fp32>& _lMoveWeights,fp32 _RotMinDiff,fp32 _MoveMinDiff)
{
	m_lKeys.Clear();
	m_lKeys.SetGrow(m_spOrgAnimSequence->m_lRotTracks.Len());

	// Fill out the lists
	if (_lRotWeights.Len() < m_spOrgAnimSequence->m_lRotTracks.Len())
	{
		for (int i = _lRotWeights.Len(); i < m_spOrgAnimSequence->m_lRotTracks.Len(); i++)
		{
			_lRotWeights.Add(1.0f);
		}
	}
	if (_lMoveWeights.Len() < m_spOrgAnimSequence->m_lMoveTracks.Len())
	{
		for (int i = _lMoveWeights.Len(); i < m_spOrgAnimSequence->m_lMoveTracks.Len(); i++)
		{
			_lMoveWeights.Add(1.0f);
		}
	}
 
	for (int iRotTrack = 0; iRotTrack < m_spOrgAnimSequence->m_lRotTracks.Len(); iRotTrack++)
	{
		const CXR_Anim_RotTrack* pTrack = &m_spOrgAnimSequence->m_lRotTracks[iRotTrack];
		// ===========
		// Pre pass
		// ===========
		// First we check if all keys but one should be removed because they are too similar
		if (_lRotWeights[iRotTrack] > 0.0f)
		{
			CQuatfp32 QFirst;
			pTrack->m_lData[0].GetRot(QFirst);
			bool allEqual = true;
			for (int iKey = 1; iKey < pTrack->m_lData.Len(); iKey++)
			{
				CQuatfp32 QCur;
				pTrack->m_lData[iKey].GetRot(QCur);
				fp32 temp = QFirst.DotProd(QCur);
				if (temp < 0)
				{	// Wrong hyperarc
					// Why is there no negation operator for quaternions in the engine?
					QFirst.k[0] = -QFirst.k[0];
					QFirst.k[1] = -QFirst.k[1];
					QFirst.k[2] = -QFirst.k[2];
					QFirst.k[3] = -QFirst.k[3];
					temp = QFirst.DotProd(QCur);
				}
				if (temp > 1.0f)
				{
					temp = 1.0f;
				}
				fp32 error = (1.0f - temp) * _PI;
				if (error > _RotMinDiff)
				{
					allEqual = false;
					break;
				}
			}
			if (allEqual)
			{
				CAnim_KeyValue KeyValue;
				KeyValue.m_Error = -1;
				KeyValue.m_iTrack = iRotTrack;
				KeyValue.m_iKey = 0;	// Original key index
				KeyValue.m_Time = m_spOrgAnimSequence->m_lTimes[pTrack->m_liTimes[0]];
				KeyValue.m_RotType = true;
				pTrack->m_lData[0].GetRot(KeyValue.m_Rot);
				m_lKeys.Add(KeyValue);

				continue;	// Next track
			}
		}

		// Q: Why not just add the CAnim_KeyValue to m_lKeys instead of the intermediate lRotKeys?
		// A: Because we need it for the second pass as detailed below
		TArray<CAnim_KeyValue>	lRotKeys;
		lRotKeys.SetGrow(pTrack->m_lData.Len());

		for (int iKey = 0; iKey < pTrack->m_lData.Len(); iKey++)
		{
			CAnim_KeyValue KeyValue;
			KeyValue.m_Error = -1;
			KeyValue.m_iTrack = iRotTrack;
			KeyValue.m_iKey = iKey;	// Original key index
			KeyValue.m_Time = m_spOrgAnimSequence->m_lTimes[pTrack->m_liTimes[iKey]];
			KeyValue.m_RotType = true;
			pTrack->m_lData[iKey].GetRot(KeyValue.m_Rot);
			lRotKeys.Add(KeyValue);
		}

		if (_lRotWeights[iRotTrack] > 0.0f)
		{
			// ===========
			// First pass
			// ===========
			// Sort the tracks keys by time and measure the error from removing each key and
			// keeping the rest. This will be our first aproximation error values.
			// The reason this is a first aproximation only is because that we only measure the error
			// when one key is removed. In reality, when keys are removed they will affect the error values
			// of their neighbours thus making removal of neighbouring keys less likely.
			// The trick is to use the order of lRotKeys to actually remove them one by one and reevaluate the errors

			QSortByTime(lRotKeys);
			int32 nUculled = lRotKeys.Len();
			TArray<bool> lbUnCulled;
			lbUnCulled.SetLen(nUculled);
			for (int iKey = 0; iKey < nUculled; iKey++)
			{
				lbUnCulled[iKey] = true;
			}
			for (int iKey = 0; iKey < lRotKeys.Len(); iKey++)
			{
				fp32 KeyError;
				if (MeasureRot(iKey,lRotKeys,lbUnCulled,&KeyError))
				{
					lRotKeys[iKey].m_Error = KeyError * _lRotWeights[iRotTrack];
				}
			}

			// ===========
			// Second pass
			// ===========
			// We will now find the current largest error key, (re)measure every key between its
			// unculled leftleft and unculled rightright and cull it.
			// This will be repeated until only 4 keys are left unculled
			/*
			while(nUculled > 4)
			{
				// Find the key with largest unculled error value (skip the first and last 2
				fp32 worstError = 0.0f;
				int iWorst = -1;
				for (int iKey = 2; iKey < lRotKeys.Len()-2; iKey++)
				{
					if ((lbUnCulled[iKey]) && (lRotKeys[iKey].m_Error > worstError))
					{
						worstError = lRotKeys[iKey].m_Error;
						iWorst = iKey;
					}
				}
				if (iWorst != -1)
				{
					fp32 KeyError;
					if (MeasureRot(iWorst,lRotKeys,lbUnCulled,&KeyError))
					{
						KeyError *= _lRotWeights[iRotTrack];
						if (lRotKeys[iWorst].m_Error < KeyError)
						{
							lRotKeys[iWorst].m_Error = KeyError;
						}
						lbUnCulled[iWorst] = false;
						// Find the leftleft and rightright unculled neighbours,
						// step from leftleft to rightright including culled keys and measure errors.
						// We up the keys errors if higher and up iWorst error if the worst neighbour error is higher
						int32 iKeyLeftLeft,iKeyRightRight;
						iKeyLeftLeft = iKeyRightRight = -1;
						for (int iKey = iWorst-1; iKey >= 0; iKey--)
						{
							if (lbUnCulled[iKey])
							{
								if (iKeyLeftLeft != -1)
								{
									iKeyLeftLeft = iKey;
									break;
								}
								else
								{
									iKeyLeftLeft = iKey;
								}
							}
						}
						for (int iKey = iWorst+1; iKey < lbUnCulled.Len(); iKey++)
						{
							if (lbUnCulled[iKey])
							{
								if (iKeyRightRight != -1)
								{
									iKeyRightRight = iKey;
									break;
								}
								else
								{
									iKeyRightRight = iKey;
								}
							}
						}
						if ((iKeyLeftLeft != -1) && (iKeyRightRight != -1))
						{
							for (int iKey = iKeyLeftLeft; iKey <= iKeyRightRight; iKey++)
							{
								fp32 KeyError;
								if (iKey == iWorst)
								{
									continue;
								}

								worstError = 0.0f;	// It's safe to reuse worstError here albeit a tad hard to follow
								if (MeasureRot(iKey,lRotKeys,lbUnCulled,&KeyError))
								{
									KeyError *= _lRotWeights[iRotTrack];
									if ((lbUnCulled[iKey] == false) && (KeyError > lRotKeys[iKey].m_Error))
									{
										lRotKeys[iKey].m_Error = KeyError;
									}
									if (KeyError > worstError)
									{
										worstError = KeyError;
									}
								}
							}
							if (worstError > lRotKeys[iWorst].m_Error)
							{
								lRotKeys[iWorst].m_Error = worstError;
							}
						}
					}
				}
				else
				{
					break;
				}
			}
			*/
		}

		m_lKeys.Add(&lRotKeys);
		lRotKeys.Clear();
	}

	for (int iMoveTrack = 0; iMoveTrack < m_spOrgAnimSequence->m_lMoveTracks.Len(); iMoveTrack++)
	{
		const CXR_Anim_MoveTrack* pTrack = &m_spOrgAnimSequence->m_lMoveTracks[iMoveTrack];
		// ===========
		// Pre pass
		// ===========
		// First we check if all keys but one should be removed because they are too similar
		if (_lMoveWeights[iMoveTrack] > 0.0f)
		{
			CVec3Dfp32 VFirst;
			pTrack->m_lData[0].GetMove(VFirst);
			bool allEqual = true;
			for (int iKey = 1; iKey < pTrack->m_lData.Len(); iKey++)
			{
				CVec3Dfp32 VCur;
				pTrack->m_lData[iKey].GetMove(VCur);
				fp32 error = VFirst.Distance(VCur) * _lMoveWeights[iMoveTrack] ;
				if (error > _MoveMinDiff)
				{
					allEqual = false;
					break;
				}
			}
			if (allEqual)
			{
				CAnim_KeyValue KeyValue;
				KeyValue.m_Error = -1;
				KeyValue.m_iTrack = iMoveTrack;
				KeyValue.m_iKey = 0;	// Original key index
				KeyValue.m_Time = m_spOrgAnimSequence->m_lTimes[pTrack->m_liTimes[0]];
				KeyValue.m_RotType = false;
				pTrack->m_lData[0].GetMove(KeyValue.m_Move);
				m_lKeys.Add(KeyValue);

				continue;	// Next track
			}
		}

		// Q: Why not just add the CAnim_KeyValue to m_lKeys instead of the intermediate lMoveKeys?
		// A: Because we need it for the second pass as detailed below
		TArray<CAnim_KeyValue>	lMoveKeys;
		lMoveKeys.SetGrow(pTrack->m_lData.Len());

		// ===========
		// First pass
		// ===========
		for (int iKey = 0; iKey < pTrack->m_lData.Len(); iKey++)
		{	// Add a keyvalue
			CAnim_KeyValue KeyValue;
			KeyValue.m_Error = -1;
			KeyValue.m_iTrack = iMoveTrack;
			KeyValue.m_Time = m_spOrgAnimSequence->m_lTimes[pTrack->m_liTimes[iKey]];
			KeyValue.m_RotType = false;
			pTrack->m_lData[iKey].GetMove(KeyValue.m_Move);
			lMoveKeys.Add(KeyValue);
		}
	
		if (_lMoveWeights[iMoveTrack] > 0.0f)
		{
			// ===========
			// First pass
			// ===========
			// Sort the tracks keys by time and measure the error from removing each key and
			// keeping the rest. This will be our first aproximation error values.
			// The reason this is a first aproximation only is because that we only measure the error
			// when one key is removed. In reality, when keys are removed they will affect the error values
			// of their neighbours thus making removal of neighbouring keys less likely.
			// The trick is to use the order of lRotKeys to actually remove them one by one and reevaluate the errors

			QSortByTime(lMoveKeys);
			int32 nUculled = lMoveKeys.Len();
			TArray<bool> lbUnCulled;
			lbUnCulled.SetLen(nUculled);
			for (int iKey = 0; iKey < nUculled; iKey++)
			{
				lbUnCulled[iKey] = true;
			}
			for (int iKey = 0; iKey < lMoveKeys.Len(); iKey++)
			{
				fp32 KeyError;
				if (MeasureMove(iKey,lMoveKeys,lbUnCulled,&KeyError))
				{
					lMoveKeys[iKey].m_Error = KeyError * _lMoveWeights[iMoveTrack];
				}
			}

			// ===========
			// Second pass
			// ===========
			// We will now find the current largest error key, (re)measure every key between its
			// unculled leftleft and unculled rightright and cull it.
			// This will be repeated until only 4 keys are left unculled
			/*
			while(nUculled > 4)
			{
				// Find the key with largest unculled error value (skip the first and last 2
				fp32 worstError = 0.0f;
				int iWorst = -1;
				for (int iKey = 2; iKey < lMoveKeys.Len()-2; iKey++)
				{
					if ((lbUnCulled[iKey]) && (lMoveKeys[iKey].m_Error > worstError))
					{
						worstError = lMoveKeys[iKey].m_Error;
						iWorst = iKey;
					}
				}
				if (iWorst != -1)
				{
					fp32 KeyError;
					if (MeasureMove(iWorst,lMoveKeys,lbUnCulled,&KeyError))
					{
						KeyError *= _lMoveWeights[iMoveTrack];
						if (lMoveKeys[iWorst].m_Error < KeyError)
						{
							lMoveKeys[iWorst].m_Error = KeyError;
						}
						lbUnCulled[iWorst] = false;
						// Find the leftleft and rightright unculled neighbours,
						// step from leftleft to rightright including culled keys and measure errors.
						// We up the keys errors if higher and up iWorst error if the worst neighbour error is higher
						int32 iKeyLeftLeft,iKeyRightRight;
						iKeyLeftLeft = iKeyRightRight = -1;
						for (int iKey = iWorst-1; iKey >= 0; iKey--)
						{
							if (lbUnCulled[iKey])
							{
								if (iKeyLeftLeft != -1)
								{
									iKeyLeftLeft = iKey;
									break;
								}
								else
								{
									iKeyLeftLeft = iKey;
								}
							}
						}
						for (int iKey = iWorst+1; iKey < lbUnCulled.Len(); iKey++)
						{
							if (lbUnCulled[iKey])
							{
								if (iKeyRightRight != -1)
								{
									iKeyRightRight = iKey;
									break;
								}
								else
								{
									iKeyRightRight = iKey;
								}
							}
						}
						if ((iKeyLeftLeft != -1) && (iKeyRightRight != -1))
						{
							for (int iKey = iKeyLeftLeft; iKey <= iKeyRightRight; iKey++)
							{
								fp32 KeyError;
								if (iKey == iWorst)
								{
									continue;
								}

								worstError = 0.0f;	// It's safe to reuse worstError here albeit a tad hard to follow
								if (MeasureMove(iKey,lMoveKeys,lbUnCulled,&KeyError))
								{
									KeyError *= _lMoveWeights[iMoveTrack];
									if ((lbUnCulled[iKey] == false) && (KeyError > lMoveKeys[iKey].m_Error))
									{
										lMoveKeys[iKey].m_Error = KeyError;
									}
									if (KeyError > worstError)
									{
										worstError = KeyError;
									}
								}
							}
							if (worstError > lMoveKeys[iWorst].m_Error)
							{
								lMoveKeys[iWorst].m_Error = worstError;
							}
						}
					}
				}
				else
				{
					break;
				}
			}
			*/
		}

		m_lKeys.Add(&lMoveKeys);
		lMoveKeys.Clear();
	}
}

spCXR_Anim_SequenceTracks CAnim_TracksOptimizer::CullMaxError(fp32 _maxError)
{
	// Sort all keys by error
	// QSortByError(m_lKeys);
	m_lRemainingKeys.Clear();
	m_lRemainingKeys.SetGrow(m_lKeys.Len());
	for (int iKey = 0; iKey < m_lKeys.Len(); iKey++)
	{
		CAnim_KeyValue Err = m_lKeys[iKey];
		if ((_maxError < 0.0f)||(m_lKeys[iKey].m_Error < 0.0f)||
			(m_lKeys[iKey].m_Error > _maxError))
		{
			m_lRemainingKeys.Add(m_lKeys[iKey]);
		}
		else
		{
			CAnim_KeyValue CulledKey = m_lKeys[iKey];
			bool debug = true;	// Key was culled!
		}
	}

	spCXR_Anim_SequenceTracks rSeq = RecreateTracks();

	return(rSeq);
}

// Recreate the tracks from m_lKeys
spCXR_Anim_SequenceTracks CAnim_TracksOptimizer::RecreateTracks()
{
	QSortByTime(m_lRemainingKeys);

	// Q: Why don't I just use the m_spOrgAnimSequence instead of rSeq?
	// A: When/if I want to change the code to work on a copy I just change the assignment here.
	spCXR_Anim_SequenceTracks rSeq = m_spOrgAnimSequence;

	TArray<uint16>	lRotKeys;
	TArray<uint16>	lMoveKeys;
	int oldNumKeys;
	if (m_LogFlag)
	{
		oldNumKeys = rSeq->m_lTimes.Len();
		lRotKeys.SetLen(rSeq->m_lRotTracks.Len());
		lMoveKeys.SetLen(rSeq->m_lMoveTracks.Len());
		for (int iRotTrack = 0; iRotTrack < rSeq->m_lRotTracks.Len(); iRotTrack++)
		{
			lRotKeys[iRotTrack] = rSeq->m_lRotTracks[iRotTrack].m_lData.Len();
		}
		for (int iMoveTrack = 0; iMoveTrack < rSeq->m_lMoveTracks.Len(); iMoveTrack++)
		{
			lMoveKeys[iMoveTrack] = rSeq->m_lMoveTracks[iMoveTrack].m_lData.Len();
		}
	}

	// Clear each item of m_lRotTracks and m_lMoveTracks
	rSeq->m_lTimes.Clear();
	for (int iRotTrack = 0; iRotTrack < rSeq->m_lRotTracks.Len(); iRotTrack++)
	{
		rSeq->m_lRotTracks[iRotTrack].m_lData.Clear();
		rSeq->m_lRotTracks[iRotTrack].m_liTimes.Clear();
		rSeq->m_lRotTracks[iRotTrack].m_liKeys.Clear();
	}
	for (int iMoveTrack = 0; iMoveTrack < rSeq->m_lMoveTracks.Len(); iMoveTrack++)
	{
		rSeq->m_lMoveTracks[iMoveTrack].m_lData.Clear();
		rSeq->m_lMoveTracks[iMoveTrack].m_liTimes.Clear();
		rSeq->m_lMoveTracks[iMoveTrack].m_liKeys.Clear();
	}

	// Loop through each key and find the unique time values
	for (int iKey = 0; iKey < m_lRemainingKeys.Len(); iKey++)
	{
		// Add the time if neccessary
		int32 nTimes = rSeq->m_lTimes.Len();
		if ((nTimes == 0)||(rSeq->m_lTimes[nTimes-1] != m_lRemainingKeys[iKey].m_Time))
		{
			rSeq->m_lTimes.Add(m_lRemainingKeys[iKey].m_Time);
			nTimes++;
		}
		m_lRemainingKeys[iKey].m_iTime = nTimes-1;
	}

	// Now that we have all the time values we must reassign the m_iTime values for the keys
	// as we have new indices for time values. Oh, did I mention that it will be slooooooow?
	for (int iKey = 0; iKey < m_lRemainingKeys.Len(); iKey++)
	{
		for (int iTime = 0; iTime < rSeq->m_lTimes.Len(); iTime++)
		{
			if (m_lRemainingKeys[iKey].m_Time == rSeq->m_lTimes[iTime])
			{
				m_lRemainingKeys[iKey].m_iTime = iTime;
				break;
			}
		}
	}

	int iTimeLastRot = -1;
	int iTimeLastMove = -1;
	for (int iKey = 0; iKey < m_lRemainingKeys.Len(); iKey++)
	{
		int iRotTrack,iMoveTrack;
		if (m_lRemainingKeys[iKey].m_RotType)
		{
			iRotTrack = m_lRemainingKeys[iKey].m_iTrack;
			CXR_Anim_RotTrack* pTrack = &rSeq->m_lRotTracks[iRotTrack];
			// Add data and time index
			CXR_Anim_RotKey Data;
			Data.SetRot(m_lRemainingKeys[iKey].m_Rot);
			pTrack->m_lData.Add(Data);
			pTrack->m_liTimes.Add(m_lRemainingKeys[iKey].m_iTime);
			if (m_lRemainingKeys[iKey].m_Time != rSeq->m_lTimes[m_lRemainingKeys[iKey].m_iTime])
			{
				LogFile(CStrF("Wrong time index %d Track, %d Key, %d Index, %f Value",
					m_lRemainingKeys[iKey].m_iTrack,
					pTrack->m_liTimes.Len()-1,
					m_lRemainingKeys[iKey].m_iTime,
					rSeq->m_lTimes[m_lRemainingKeys[iKey].m_iTime]));
			}
		}
		else
		{
			iMoveTrack = m_lRemainingKeys[iKey].m_iTrack;
			CXR_Anim_MoveTrack* pTrack = &rSeq->m_lMoveTracks[iMoveTrack];
			// Add data and time index
			CXR_Anim_MoveKey Data;
			Data.SetMove(m_lRemainingKeys[iKey].m_Move);
			pTrack->m_lData.Add(Data);
			pTrack->m_liTimes.Add(m_lRemainingKeys[iKey].m_iTime);
			if (m_lRemainingKeys[iKey].m_Time != rSeq->m_lTimes[m_lRemainingKeys[iKey].m_iTime])
			{
				LogFile(CStrF("Wrong time index %d Track, %d Key, %d Index, %f Value",
					m_lRemainingKeys[iKey].m_iTrack,
					pTrack->m_liTimes.Len()-1,
					m_lRemainingKeys[iKey].m_iTime,
					rSeq->m_lTimes[m_lRemainingKeys[iKey].m_iTime]));
			}
		}
	}

	// Ok, everything is in now, we just need to set the pTrack->m_liKeys values
	for (int iRotTrack = 0; iRotTrack < rSeq->m_lRotTracks.Len(); iRotTrack++)
	{
		CXR_Anim_RotTrack* pTrack = &rSeq->m_lRotTracks[iRotTrack];
		// We don't need to add any m_liKeys values if we only got one data sample
		// (Saves us 2 bytes per culled track)
		if (pTrack->m_liTimes.Len() > 1)
		{
			for (int iKey = 1; iKey < pTrack->m_liTimes.Len(); iKey++)
			{
				for (int i = pTrack->m_liTimes[iKey-1]; i < pTrack->m_liTimes[iKey]; i++)
				{
					pTrack->m_liKeys.Add(iKey-1);
				}
			}

			// Add the last key too
			if (pTrack->m_lData.Len() > 0)
			{
				pTrack->m_liKeys.Add(pTrack->m_liTimes.Len()-1);
			}
		}
	}

	for (int iMoveTrack = 0; iMoveTrack < rSeq->m_lMoveTracks.Len(); iMoveTrack++)
	{
		CXR_Anim_MoveTrack* pTrack = &rSeq->m_lMoveTracks[iMoveTrack];
		if (pTrack->m_liTimes.Len() > 1)
		{
			for (int iKey = 1; iKey < pTrack->m_liTimes.Len(); iKey++)
			{
				for (int i = pTrack->m_liTimes[iKey-1]; i < pTrack->m_liTimes[iKey]; i++)
				{
					pTrack->m_liKeys.Add(iKey-1);
				}
			}

			// Add the last key too
			if (pTrack->m_lData.Len() > 0)
			{
				pTrack->m_liKeys.Add(pTrack->m_liTimes.Len()-1);
			}
		}
	}

	if (m_LogFlag)
	{
		int newNumKeys = rSeq->m_lTimes.Len();
		CStr Line = rSeq->m_Name;
		Line += CStrF(": %d original keys, culled %d",oldNumKeys,oldNumKeys - newNumKeys);
		ConOut(Line);
		LogFile(Line);

		// *** Code to verify the integrity of data ***
		for (int iRotTrack = 0; iRotTrack < rSeq->m_lRotTracks.Len(); iRotTrack++)
		{
			CXR_Anim_RotTrack* pTrack = &rSeq->m_lRotTracks[iRotTrack];
			if (pTrack->m_liTimes.Len() > 1)
			{
				uint16 iTimeLast = 0;
				for (int iiKey = 0; iiKey < pTrack->m_liKeys.Len(); iiKey++)
				{
					uint16 iKey = pTrack->m_liKeys[iiKey];
					if (iKey >= rSeq->m_lTimes.Len())
					{
						LogFile(CStrF("Rottrack %d key %d is out of range: %iKey, nTimes: %d",iRotTrack,iiKey,iKey,rSeq->m_lTimes.Len()));
					}
				}
			}
		}
		for (int iMoveTrack = 0; iMoveTrack < rSeq->m_lMoveTracks.Len(); iMoveTrack++)
		{
			CXR_Anim_MoveTrack* pTrack = &rSeq->m_lMoveTracks[iMoveTrack];
			if (pTrack->m_liTimes.Len() > 1)
			{
				uint16 iTimeLast = 0;
				for (int iiKey = 0; iiKey < pTrack->m_liKeys.Len(); iiKey++)
				{
					uint16 iKey = pTrack->m_liKeys[iiKey];
					if (iKey >= rSeq->m_lTimes.Len())
					{
						LogFile(CStrF("Movetrack %d key %d is out of range: %iKey, nTimes: %d",iMoveTrack,iiKey,iKey,rSeq->m_lTimes.Len()));
					}
				}
			}
		}
		// ***

		for (int iRotTrack = 0; iRotTrack < rSeq->m_lRotTracks.Len(); iRotTrack++)
		{
			int oldKeys = lRotKeys[iRotTrack];
			int newKeys = rSeq->m_lRotTracks[iRotTrack].m_lData.Len();
			if (oldKeys != newKeys)
			{
				if (newKeys > 1)
				{
					Line = CStrF("Rot %d culled %d",iRotTrack,oldKeys - newKeys);

				}
				else
				{
					Line = CStrF("Rot %d culled track",iRotTrack);
				}
				LogFile(Line);
			}
		}
		for (int iMoveTrack = 0; iMoveTrack < rSeq->m_lMoveTracks.Len(); iMoveTrack++)
		{
			int oldKeys = lRotKeys[iMoveTrack];
			int newKeys = rSeq->m_lRotTracks[iMoveTrack].m_lData.Len();
			if (oldKeys != newKeys)
			{
				if (newKeys > 1)
				{
					Line = CStrF("Move %d culled %d",iMoveTrack,oldKeys - newKeys);
				}
				else
				{
					Line = CStrF("Move %d culled track",iMoveTrack);
				}
				LogFile(Line);
			}
		}
		
	}

	return(rSeq);
}
#endif
