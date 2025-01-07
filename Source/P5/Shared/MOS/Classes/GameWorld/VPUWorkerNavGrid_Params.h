enum
{
	VPU_NAVGRID_PARAM_TILETILEINDEX,
	VPU_NAVGRID_PARAM_TILETILES,
	VPU_NAVGRID_PARAM_TILES,
	VPU_NAVGRID_PARAM_BITPLANES,
	VPU_NAVGRID_PARAM_BITPLANESDATA,
	VPU_NAVGRID_PARAM_COMPRESSEDDATA,
	
	VPU_NAVGRID_PARAM_MODELS,

	VPU_NAVGRID_PARAM_HASH_SETTINGS,

	VPU_NAVGRID_PARAM_HASH1_LINKS,
	VPU_NAVGRID_PARAM_HASH1_ELEMENTS,
	VPU_NAVGRID_PARAM_HASH1_IDS,

	VPU_NAVGRID_PARAM_HASH2_LINKS,
	VPU_NAVGRID_PARAM_HASH2_ELEMENTS,
	VPU_NAVGRID_PARAM_HASH2_IDS,

	VPU_NAVGRID_PARAM_GRIDLAYOUT,
	VPU_NAVGRID_PARAM_SEARCHPARAMS,

	VPU_NAVGRID_PARAM_RESULTNODES,
	VPU_NAVGRID_PARAM_RESULT,

	VPU_NAVGRID_PARAM_DEBUG,
	VPU_NAVGRID_PARAM_DEBUG_OUTPUT,
};


////////////////////////////////////////////////////////////////////////
struct SHashSettings
{
	int m_nBoxesSmall;
	int m_BoxShiftSizeSmall;
	int m_nBoxesLarge;
	int m_BoxShiftSizeLarge;
	int m_nObjects;
	bool m_bIsServerCWObject;
};
