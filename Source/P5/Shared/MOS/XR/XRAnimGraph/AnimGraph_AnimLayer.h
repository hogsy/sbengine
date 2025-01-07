#ifndef AnimGraph_AnimLayer_h
#define AnimGraph_AnimLayer_h

//--------------------------------------------------------------------------------

#include "AnimGraphDefs.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CXRAG_AnimLayer
{
	public:

		//Interface
virtual CAGAnimIndex	GetAnimIndex() const { return GetDefaultAnimIndex(); }
virtual uint8			GetBaseJointIndex() const { return GetDefaultBaseJointIndex(); }
virtual CAGOperatorID	GetMergeOperator() const { return GetDefaultMergeOperator(); }
virtual CAGPropertyID	GetTimeControlProperty() const { return GetDefaultTimeControlProperty(); }
virtual fp32				GetTimeOffset() const { return GetDefaultTimeOffset(); }
virtual fp32				GetTimeScale() const { return GetDefaultTimeScale(); }
virtual fp32				GetOpacity() const { return GetDefaultOpacity(); }

		// Defaults
static CAGAnimIndex		GetDefaultAnimIndex() { return AG_ANIMINDEX_NULL; }
static uint8			GetDefaultBaseJointIndex() { return 0; }
static CAGOperatorID	GetDefaultMergeOperator() { return AG_ANIMLAYER_MERGEOPERATOR_BLEND; }
static CAGPropertyID	GetDefaultTimeControlProperty() { return AG_PROPERTY_NULL; }
static fp32				GetDefaultTimeOffset() { return 0; }
static fp32				GetDefaultTimeScale() { return 1.0f; }
static fp32				GetDefaultOpacity() { return 1.0f; }

};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CXRAG_AnimLayer_Small : public CXRAG_AnimLayer
{
	public:
	enum
	{
		EIOWholeStruct = 0
	};
		
		CAGAnimIndex	m_iAnim; // Index into AnimList/ResourceMap
		uint8			m_iBaseJoint; //

	public:

		CXRAG_AnimLayer_Small() { Clear(); }
		void Clear();

		CAGAnimIndex	GetAnimIndex() const { return m_iAnim; }
		uint8			GetBaseJointIndex() const { return m_iBaseJoint; }

		void Read(CCFile* _pFile, int _Ver);
		void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
		void SwapLE();
#endif

};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CXRAG_AnimLayer_Full : public CXRAG_AnimLayer
{
	public:
	enum
	{
		EIOWholeStruct = 0
	};
		
		fp32				m_TimeOffset; // Time offset for fixed time offset type.
		fp32				m_TimeScale; // Time scale, applied around time 0.
		fp32				m_Opacity; // Opacity/BlendFactor of this layer (besides blendin/blendout).

		CAGAnimIndex	m_iAnim; // Index into AnimList/ResourceMap // 2
		uint8			m_iBaseJoint; //
		CAGOperatorID	m_iMergeOperator; // Operator with which to apply this layer to the previous layers. // 1
		CAGPropertyID	m_iTimeControlProperty; // Index of property which controls the anim time of this layer. // 1

	public:

		CXRAG_AnimLayer_Full() { Clear(); }
		void Clear();

		CAGAnimIndex	GetAnimIndex() const { return m_iAnim; }
		uint8			GetBaseJointIndex() const { return m_iBaseJoint; }
		CAGOperatorID	GetMergeOperator() const { return m_iMergeOperator; }
		CAGPropertyID	GetTimeControlProperty() const { return m_iTimeControlProperty; }
		fp32				GetTimeOffset() const { return m_TimeOffset; }
		fp32				GetTimeScale() const { return m_TimeScale; }
		fp32				GetOpacity() const { return m_Opacity; }

		void Read(CCFile* _pFile, int _Ver);
		void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
		void SwapLE();
#endif
};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

#endif /* AnimGraph_AnimLayer_h */