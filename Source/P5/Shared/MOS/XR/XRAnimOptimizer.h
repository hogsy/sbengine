#ifndef __XRANIMOPTIMIZER_H
#define __XRANIMOPTIMIZER_H
#ifndef PLATFORM_CONSOLE

#include "XRAnim.h"
#include "XRSkeleton.h"

// CAnimationOptimizer
// Manages optimization of animation data
class CAnimationOptimizer
{
public:
	CAnimationOptimizer();
	~CAnimationOptimizer();
	void Setup(class CXR_Anim_Sequence* _pSeq);
	int32 Optimize(TArray<fp32>& _lWeights);
	fp32 Measure(int32 _index);
	fp32 Cull(fp32 _maxError,fp32 _cullFactor = 1.0);
	
private:
	TPtr<class CXR_Anim_Sequence>					m_pAnimSequence;
	TArray<TPtr<class CXR_Anim_Keyframe> >*	m_plKeys;
	TArray<TPtr<class CXR_Anim_Keyframe> >	m_lOrgKeys;
	int32											m_RotChannels;
	int32											m_TransChannels;
	int32											m_MaxChannels;
	
	int32											m_KeyCount;
	TArray<fp32>										m_lKeyTimes;
	TArray<fp32>										m_lKeyTimeDeltas;
	TArray<int32>									m_lCullOrder;
	TArray<TPtr<CXR_Anim_Keyframe> >				m_lspCullKeys;
	TArray<fp32>										m_lErrorWeights;
	TArray<bool>									m_lOptimized;
	TArray<fp32>										m_lErrors;
};


// =========================
// CAnimationTracksOptimizer
// =========================
// Optimizes animations by removing keys that contribute little to the animation
// Usage:
// Call Setup() and supply an spCXR_Anim_SequenceTracks
// Call CalcErrors() (time consuming) to measure and rank keys for removal.
// Call CullMaxError() and supply it with a max error that determines the maximum
// distance each skeleton node may err from its original position
// or call CullFactor() with a supplied factor to cull a certain percentage of keys.
//
// Implementation
// ==============
// for every rottrack
//		Measure every quat and store in lOrgRots
//		for every key of current rottrack
//			Measure quat with current key removed
//			Calculate error (angle diff * weight) and store in lPreliminaryErrors
//		end
//		Sort lPreliminaryErrors by error
//		for every key of current rottrack in lPreliminaryErrors order
//			Measure culled quat taking previously culled quats into account
//			Calculate new error and add to m_lErrors
//		end
//		Clear lPreliminaryErrors
// end
// for every movetrack
//		Measure every pos and store in lOrgMoves
//		for every key of current movetrack
//			Measure pos with current key removed
//			Calculate error (pos diff * weight) and store in lPreliminaryErrors
//		end
//		Sort lPreliminaryErrors by error
//		for every key of current movetrack in lPreliminaryErrors order
//			Measure culled pos taking previously culled pos into account
//			Calculate new error and add to m_lErrors
//		end
//		Clear lPreliminaryErrors
// end
// Sort m_lErrors by error
// Now m_lErrors should shouls hold a list of in what order keys should be
// culled from the animation.

class CAnim_TracksOptimizer
{
public:
	CAnim_TracksOptimizer();
	~CAnim_TracksOptimizer();

	void Clear();
	void Setup(spCXR_Anim_SequenceTracks _spSeq,bool _LogFlag = false);
	// Calculate error values for the keys
	// _lRotWeights is a list of weights for rottracks (0 or less indicate keys cannot be culled)
	// _lMoveWeights is a list of weights for movetracks (0 or less indicate keys cannot be culled)
	// If no keys differ more than _RotMinDiff or _MoveMinDiff in a track, all but one will be culled
	void CalcErrors(TArray<fp32>& _lRotWeights,TArray<fp32>& _lMoveWeights,fp32 _RotMinDiff,fp32 _MoveMinDiff);
	spCXR_Anim_SequenceTracks CullMaxError(fp32 _maxError);
	spCXR_Anim_SequenceTracks RecreateTracks();

private:
	class CAnim_KeyValue;
	typedef TPtr<CAnim_KeyValue> spCAnim_KeyValue;
	class CAnim_KeyValue
	{
	public:
		CAnim_KeyValue()
		{
			m_Error = -1.0f;	// Error not yet determined
			m_iTrack = 0;
			m_iKey = 0;
			m_Time = 0.0f;
			m_iTime= -1;
			m_RotType = true;
		};

		fp32	m_Error;
		int32 m_iTrack;
		int32 m_iKey;
		fp32 m_Time;
		int32 m_iTime;

		bool m_RotType;	// Move type when false
		CQuatfp32		m_Rot;	// Valid only when m_RotType == true
		CVec3Dfp32		m_Move;	// Valid only when m_RotType != true
	};

	
	static int M_CDECL CompareError(const void* _FirstElem,const void* _SecondElem);
	static int M_CDECL CompareTime(const void* _FirstElem,const void* _SecondElem);
	// Sort by error (low to high)
	void QSortByError(TArray<CAnim_KeyValue>& _lKeys);
	// Sort by time (low to high)
	void QSortByTime(TArray<CAnim_KeyValue>& _lKeys);

	bool RotCullError(uint16 _iKey,TArray<CAnim_KeyValue*>& _lpKeys,fp32* _pError);
	bool RotCullError(uint16 _iKey,TArray<CAnim_KeyValue>& _lpKeys,fp32* _pError);
	bool MoveCullError(uint16 _iKey,TArray<CAnim_KeyValue*>& _lpKeys,fp32* _pError);
	bool MoveCullError(uint16 _iKey,TArray<CAnim_KeyValue>& _lpKeys,fp32* _pError);

	bool MeasureRot(int _iKey,TArray<CAnim_KeyValue>& _lKeys,TArray<bool>& _lbUnCulled,fp32* _pError);
	bool MeasureMove(int _iKey,TArray<CAnim_KeyValue>& _lKeys,TArray<bool>& _lbUnCulled,fp32* _pError);

	spCXR_Anim_SequenceTracks						m_spOrgAnimSequence;
	TArray<CAnim_KeyValue>							m_lKeys;
	TArray<CAnim_KeyValue>							m_lRemainingKeys;

public:
	bool											m_LogFlag;
};

#endif
#endif
