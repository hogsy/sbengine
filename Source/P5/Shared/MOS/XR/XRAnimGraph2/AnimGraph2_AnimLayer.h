#ifndef ANIMGRAPH2_ANIMLAYER_H
#define ANIMGRAPH2_ANIMLAYER_H

//--------------------------------------------------------------------------------

#include "AnimGraph2Defs.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CXRAG2_AnimLayer
{
	public:
	enum
	{
		EIOWholeStruct = 0
	};

	fp32				m_TimeOffset; // Time offset for fixed time offset type.
	fp32				m_TimeScale; // Time scale, applied around time 0.
	fp32				m_Opacity; // Opacity/BlendFactor of this layer (besides blendin/blendout).

	CAG2AnimIndex	m_iAnim; // Index into AnimList/ResourceMap // 2
	CAG2AnimFlags	m_AnimFlags;
	uint8			m_iBaseJoint; //
	CAG2OperatorID	m_iMergeOperator; // Operator with which to apply this layer to the previous layers. // 1
	CAG2PropertyID	m_iTimeControlProperty; // Index of property which controls the anim time of this layer. // 1

public:

	CXRAG2_AnimLayer() { Clear(); }
	void Clear();

	CAG2AnimIndex	GetAnimIndex() const { return m_iAnim; }
	CAG2AnimFlags	GetAnimFlags() const { return m_AnimFlags; }
	uint8			GetBaseJointIndex() const { return m_iBaseJoint; }
	CAG2OperatorID	GetMergeOperator() const { return m_iMergeOperator; }
	CAG2PropertyID	GetTimeControlProperty() const { return m_iTimeControlProperty; }
	fp32				GetTimeOffset() const { return m_TimeOffset; }
	fp32				GetTimeScale() const { return m_TimeScale; }
	fp32				GetOpacity() const { return m_Opacity; }

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

		// Defaults
static CAG2AnimFlags	GetDefaultAnimFlags() { return AG2_ANIMINDEX_NULL; }
static CAG2AnimIndex	GetDefaultAnimIndex() { return AG2_ANIMINDEX_NULL; }
static uint8			GetDefaultBaseJointIndex() { return 0; }
static CAG2OperatorID	GetDefaultMergeOperator() { return AG2_ANIMLAYER_MERGEOPERATOR_BLEND; }
static CAG2PropertyID	GetDefaultTimeControlProperty() { return AG2_PROPERTY_NULL; }
static fp32				GetDefaultTimeOffset() { return 0; }
static fp32				GetDefaultTimeScale() { return 1.0f; }
static fp32				GetDefaultOpacity() { return 1.0f; }

};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

#endif /* AnimGraph_AnimLayer_h */
