#ifndef ANIMGRAPH2DEFS_H
#define ANIMGRAPH2DEFS_H

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// !!!! EVERYTHING CHANGED HERE IMPLIES A NEW XAG VERSION (See AnimGraph.h) !!!!
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

// AnimGraph versions and history.
#define XR_ANIMGRAPH2_VERSION3 0x0003 // Created
#define XR_ANIMGRAPH2_VERSION 0x0004 // Created

#define M_AGINLINE M_INLINE
//--------------------------------------------------------------------------------

#define AG2_DUMPFLAGS_ALL					(~0)
#define AG2_DUMPFLAGS_STATES				0x00000100
#define AG2_DUMPFLAGS_STATE_PROPERTIES		0x00000001
#define AG2_DUMPFLAGS_STATE_STATECONSTANTS	0x00000002
#define AG2_DUMPFLAGS_STATE_ANIMLAYERS		0x00000004
#define AG2_DUMPFLAGS_STATE_NODETREES		0x00000008
#define AG2_DUMPFLAGS_STATE_NODETREEEFFECTS	0x00000010

#define AG2_DUMPFLAGS_ACTIONS				0x00001000
#define AG2_DUMPFLAGS_CONDITIONNODES		0x00002000
#define AG2_DUMPFLAGS_ANIMLAYERS			0x00004000
#define AG2_DUMPFLAGS_EFFECTINSTANCES		0x00008000
#define AG2_DUMPFLAGS_CALLBACKPARAMS		0x00010000

#define AG2_DUMPFLAGS_EXPORTEDACTIONS		0x01000000
#define AG2_DUMPFLAGS_EXPORTEDSTATES		0x02000000
#define AG2_DUMPFLAGS_EXPORTEDPROPERTIES	0x04000000
#define AG2_DUMPFLAGS_EXPORTEDOPERATORS		0x08000000
#define AG2_DUMPFLAGS_EXPORTEDEFFECTS		0x10000000
#define AG2_DUMPFLAGS_EXPORTEDSTATECONSTANTS	0x20000000
#define AG2_DUMPFLAGS_EXPORTEDIMPULSETYPES	0x40000000

//--------------------------------------------------------------------------------

typedef int8	CAG2AnimGraphID;
typedef	int8	CAG2TokenID;
typedef	int16	CAG2StateIndex;
typedef	int16	CAG2GraphBlockIndex;
typedef	int16	CAG2NodeIndex;
typedef	int16	CAG2ActionIndex;
typedef	int16	CAG2ReactionIndex;
typedef	int16	CAG2AnimLayerIndex;
typedef	uint8	CAG2PropertyID;
typedef	uint8	CAG2OperatorID;
typedef	uint8	CAG2EffectID;
typedef	uint8	CAG2StateConstantID;
typedef	int16	CAG2AnimIndex;
typedef	uint32	CAG2AnimFlags;
typedef	int16	CAG2EffectInstanceIndex;
typedef	int16	CAG2MoveTokenIndex;
typedef	int16	CAG2MoveAnimgraphIndex;
typedef	int16	CAG2CallbackParamIndex;
typedef	int16	CAG2StateConstantIndex;
typedef	uint32	CAG2StateFlags;
typedef int16	CAG2ImpulseType;
typedef int16	CAG2ImpulseValue;
typedef int32	CAG2AnigraphNameHash;
typedef int16	CAG2ActionValIndex;

//--------------------------------------------------------------------------------

#define AG2_TARGETSTATE_TERMINATEMASK	(1<<15)
#define AG2_TARGETSTATE_INDEXMASK		(~AG2_TARGETSTATE_TERMINATEMASK)

//--------------------------------------------------------------------------------
#define AG2_TOKENID_NULL					((CAG2TokenID)(-1))
#define AG2_TOKENID_DEFAULT					((CAG2TokenID)0)

#define AG2_STATEINDEX_NULL					((CAG2StateIndex)(-1))
#define AG2_STATEINDEX_STARTAG				((CAG2StateIndex)(-2))
#define AG2_GRAPHBLOCKINDEX_STARTAG			((CAG2GraphBlockIndex)(-2))
#define AG2_GRAPHBLOCKINDEX_NULL			((CAG2GraphBlockIndex)(-1))
#define AG2_STATEINDEX_TERMINATE			((CAG2StateIndex)AG2_TARGETSTATE_TERMINATEMASK)

#define AG2_NODEINDEX_NULL					((CAG2NodeIndex)(-1))

#define AG2_EXPORTEDACTIONINDEX_NULL		((CAG2ActionIndex)(-1))

#define AG2_ACTIONINDEX_NULL				((CAG2ActionIndex)(-1))
#define AG2_ACTIONINDEX_START				((CAG2ActionIndex)0)

#define AG2_REACTIONINDEX_NULL				((CAG2ReactionIndex)(-1))

#define AG2_IMPULSETYPE_UNDEFINED			((CAG2ImpulseType)-1)
#define AG2_IMPULSEVALUE_UNDEFINED			((CAG2ImpulseValue)-1)

#define AG2_ANIMLAYERINDEX_NULL				((CAG2AnimLayerIndex)(-1))
#define AG2_STATECONSTANTINDEX_NULL			((CAG2StateConstantIndex)(-1))
#define AG2_EFFECTINSTANCEINDEX_NULL		((CAG2EffectInstanceIndex)(-1))
#define AG2_CALLBACKPARAMINDEX_NULL			((CAG2CallbackParamIndex)(-1))
#define AG2_MOVETOKENINDEX_NULL				((CAG2MoveTokenIndex)(-1))
#define AG2_MOVEANIMGRAPHINDEX_NULL			((CAG2MoveAnimgraphIndex)(-1))
#define AG2_ANIMGRAPHNAMEHASH_NULL			((CAG2AnigraphNameHash)(-1))

#define AG2_PROPERTY_NULL				((CAG2PropertyID)(-1))
#define AG2_PROPERTYTYPE_UNDEFINED		(-1)
#define AG2_OPERATOR_NULL				((CAG2OperatorID)(-1))
#define AG2_EFFECTID_NULL				((CAG2EffectID)(-1))

#define AG2_ANIMINDEX_NULL				((CAG2AnimIndex)(-1))
#define AG2_ANIMFLAGS_NULL				((CAG2AnimFlags)(0))

//--------------------------------------------------------------------------------

enum
{
	AG2_ANIMLAYER_MERGEOPERATOR_BLEND		= 0x1,
	AG2_ANIMLAYER_MERGEOPERATOR_ADD			= 0x2,
};

enum
{
	AG2_PROPERTYTYPE_FLOAT					= 0,
	AG2_PROPERTYTYPE_INT					= 1,
	AG2_PROPERTYTYPE_BOOL					= 2,
	AG2_PROPERTYTYPE_CONDITION				= 3,		// Float
	AG2_PROPERTYTYPE_FUNCTION				= 4,		// Float

	AG2_PROPERTYTYPE_END = AG2_PROPERTYTYPE_FUNCTION,

	AG2_STATETYPE_NORMAL					= 0,
	AG2_STATETYPE_SWITCH					= 1,
};

//--------------------------------------------------------------------------------

enum
{
	AG2_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT		= 0x1,
	AG2_ANIMLAYER_TIMEOFFSETTYPE_INHERIT			= 0x2,
	AG2_ANIMLAYER_TIMEOFFSETTYPE_INHERITEQ		= 0x3,
	AG2_ANIMLAYER_TIMEOFFSETTYPE_ROLLBACKBLEND	= 0x4,
};

//--------------------------------------------------------------------------------

#define AG2_NODEACTION_BITS			7

#define AG2_NODEACTION_TYPEMASK		(1 << (AG2_NODEACTION_BITS - 1))
#define AG2_NODEACTION_ACTIONMASK	((1 << (AG2_NODEACTION_BITS - 1)) - 1)
#define AG2_NODEACTION_NODEMASK		((1 << (AG2_NODEACTION_BITS - 1)) - 1)

#define AG2_NODEACTION_ENDPARSE		AG2_NODEACTION_TYPEMASK
#define AG2_NODEACTION_PARSENODE		0
#define AG2_NODEACTION_FAILPARSE		(AG2_NODEACTION_ENDPARSE | ((-1) & AG2_NODEACTION_ACTIONMASK))

//--------------------------------------------------------------------------------

#endif /* AnimGraphDefs_h */
