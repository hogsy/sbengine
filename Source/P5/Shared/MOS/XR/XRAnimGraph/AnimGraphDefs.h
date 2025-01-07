#ifndef AnimGraphDef_h
#define AnimGraphDef_h

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// !!!! EVERYTHING CHANGED HERE IMPLIES A NEW XAG VERSION (See AnimGraph.h) !!!!
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

// AnimGraph versions and history.
#define XR_ANIMGRAPH_VERSION 0x0101 // Created

//--------------------------------------------------------------------------------

#define AG_DUMPFLAGS_ALL					(~0)
#define AG_DUMPFLAGS_STATES					0x00000100
#define AG_DUMPFLAGS_STATE_PROPERTIES		0x00000001
#define AG_DUMPFLAGS_STATE_STATECONSTANTS	0x00000002
#define AG_DUMPFLAGS_STATE_ANIMLAYERS		0x00000004
#define AG_DUMPFLAGS_STATE_NODETREES		0x00000008
#define AG_DUMPFLAGS_STATE_NODETREEEFFECTS	0x00000010

#define AG_DUMPFLAGS_ACTIONS				0x00001000
#define AG_DUMPFLAGS_CONDITIONNODES			0x00002000
#define AG_DUMPFLAGS_ANIMLAYERS				0x00004000
#define AG_DUMPFLAGS_EFFECTINSTANCES		0x00008000
#define AG_DUMPFLAGS_CALLBACKPARAMS			0x00010000

#define AG_DUMPFLAGS_EXPORTEDACTIONS		0x01000000
#define AG_DUMPFLAGS_EXPORTEDSTATES			0x02000000
#define AG_DUMPFLAGS_EXPORTEDPROPERTIES		0x04000000
#define AG_DUMPFLAGS_EXPORTEDOPERATORS		0x08000000
#define AG_DUMPFLAGS_EXPORTEDEFFECTS		0x10000000
#define AG_DUMPFLAGS_EXPORTEDSTATECONSTANTS	0x20000000

//--------------------------------------------------------------------------------

typedef	int8	CAGTokenID;
typedef	int16	CAGStateIndex;
typedef	int16	CAGNodeIndex;
typedef	int16	CAGActionIndex;
typedef	int16	CAGAnimLayerIndex;
typedef	uint8	CAGPropertyID;
typedef	uint8	CAGOperatorID;
typedef	uint8	CAGEffectID;
typedef	uint8	CAGStateConstantID;
typedef	int16	CAGAnimIndex;
typedef	int16	CAGEffectInstanceIndex;
typedef	int16	CAGCallbackParamIndex;
typedef	int16	CAGStateConstantIndex;
typedef	uint32	CAGStateFlags;

//--------------------------------------------------------------------------------

#define AG_TARGETSTATE_TERMINATEMASK	(1<<15)
#define AG_TARGETSTATE_INDEXMASK		(~AG_TARGETSTATE_TERMINATEMASK)

//--------------------------------------------------------------------------------

#define AG_TOKENID_NULL					((CAGTokenID)(-1))
#define AG_TOKENID_DEFAULT				((CAGTokenID)0)

#define AG_STATEINDEX_NULL				((CAGStateIndex)(-1))
#define AG_STATEINDEX_TERMINATE			((CAGStateIndex)AG_TARGETSTATE_TERMINATEMASK)

#define AG_NODEINDEX_NULL				((CAGNodeIndex)(-1))

#define AG_EXPORTEDACTIONINDEX_NULL		((CAGActionIndex)(-1))

#define AG_ACTIONINDEX_NULL				((CAGActionIndex)(-1))
#define AG_ACTIONINDEX_START			((CAGActionIndex)0)

#define AG_ANIMLAYERINDEX_NULL			((CAGAnimLayerIndex)(-1))
#define AG_STATECONSTANTINDEX_NULL		((CAGStateConstantIndex)(-1))
#define AG_EFFECTINSTANCEINDEX_NULL		((CAGEffectInstanceIndex)(-1))
#define AG_CALLBACKPARAMINDEX_NULL		((CAGCallbackParamIndex)(-1))

#define AG_PROPERTY_NULL				((CAGPropertyID)(-1))
#define AG_OPERATOR_NULL				((CAGOperatorID)(-1))
#define AG_EFFECTID_NULL				((CAGEffectID)(-1))

#define AG_ANIMINDEX_NULL				((CAGAnimIndex)(-1))

//--------------------------------------------------------------------------------

enum
{
	AG_ANIMLAYER_MERGEOPERATOR_BLEND		= 0x1,
	AG_ANIMLAYER_MERGEOPERATOR_ADD			= 0x2,
};

//--------------------------------------------------------------------------------

enum
{
	AG_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT		= 0x1,
	AG_ANIMLAYER_TIMEOFFSETTYPE_INHERIT			= 0x2,
	AG_ANIMLAYER_TIMEOFFSETTYPE_INHERITEQ		= 0x3,
	AG_ANIMLAYER_TIMEOFFSETTYPE_ROLLBACKBLEND	= 0x4,
};

//--------------------------------------------------------------------------------

#define AG_NODEACTION_BITS			7

#define AG_NODEACTION_TYPEMASK		(1 << (AG_NODEACTION_BITS - 1))
#define AG_NODEACTION_ACTIONMASK	((1 << (AG_NODEACTION_BITS - 1)) - 1)
#define AG_NODEACTION_NODEMASK		((1 << (AG_NODEACTION_BITS - 1)) - 1)

#define AG_NODEACTION_ENDPARSE		AG_NODEACTION_TYPEMASK
#define AG_NODEACTION_PARSENODE		0
#define AG_NODEACTION_FAILPARSE		(AG_NODEACTION_ENDPARSE | ((-1) & AG_NODEACTION_ACTIONMASK))

//--------------------------------------------------------------------------------

#endif /* AnimGraphDefs_h */