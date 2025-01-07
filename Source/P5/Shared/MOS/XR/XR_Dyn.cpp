
#include "mda.h"

#include "MRTC.h"

class CRegisterXR
{
public:
	CRegisterXR()
	{
		MRTC_REFERENCE(CXR_Anim_Keyframe);
		MRTC_REFERENCE(CXR_Anim_Sequence);
		MRTC_REFERENCE(CXR_Anim_SequenceTracks);
		MRTC_REFERENCE(CXR_Anim_Base);
		MRTC_REFERENCE(CXR_SkeletonInstance);
		MRTC_REFERENCE(CXR_Skeleton);
		MRTC_REFERENCE(CXR_Model_TriangleMesh);
		MRTC_REFERENCE(CXR_TriangleMeshDecalContainer);

		#ifndef PLATFORM_XBOX
		MRTC_REFERENCE(CXR_Model_BSP);
		#endif

		MRTC_REFERENCE(CXR_Model_BSP2);

		#ifndef PLATFORM_XBOX
		MRTC_REFERENCE(CXR_Model_BSP3);
		#endif

		MRTC_REFERENCE(CXR_Model_BSP4);

		MRTC_REFERENCE(CXR_Model_BSP4Glass);

		MRTC_REFERENCE(CXR_Model_Flare);
		MRTC_REFERENCE(CXR_Model_Sprite);
		MRTC_REFERENCE(CXR_Model_SpotLightVolume);
		MRTC_REFERENCE(CXR_Model_Sky);
		MRTC_REFERENCE(CXR_Model_MultiTriMesh);

		#ifndef PLATFORM_XBOX
		MRTC_REFERENCE(CXR_Model_Multi);
		#endif

		MRTC_REFERENCE(CTextureContainer_Render);
		MRTC_REFERENCE(CTextureContainer_EnginePortals);
		#if 1
//		MRTC_REFERENCE(CTextureContainer_ShadowDecals);
		MRTC_REFERENCE(CTextureContainer_Screen);
		MRTC_REFERENCE(CTextureContainer_ScreenCopy);
		MRTC_REFERENCE(CTextureContainer_RenderCallback);
		#endif
		MRTC_REFERENCE(CXR_EngineImpl);

		MRTC_REFERENCE(CXR_Model_FogVolume);

		MRTC_REFERENCE(CXR_ParticleContainer);

		MRTC_REFERENCE(CXW_SurfaceKeyFrame);
		MRTC_REFERENCE(CXW_SurfaceSequence);
		MRTC_REFERENCE(CXW_Surface);

		MRTC_REFERENCE(CXR_SurfaceContext);

		MRTC_REFERENCE(CXR_VBContext);

		MRTC_REFERENCE(CXR_VBManager);
		MRTC_REFERENCE(CXR_VBMContainer);

		MRTC_REFERENCE(CXR_VBOperator_TextureAnim);
		MRTC_REFERENCE(CXR_VBOperator_Offset);
		MRTC_REFERENCE(CXR_VBOperator_Scroll);
		MRTC_REFERENCE(CXR_VBOperator_Scale);
		MRTC_REFERENCE(CXR_VBOperator_Rotate);
		MRTC_REFERENCE(CXR_VBOperator_GenReflection);
		MRTC_REFERENCE(CXR_VBOperator_GenEnv);
		MRTC_REFERENCE(CXR_VBOperator_GenEnv2);
		MRTC_REFERENCE(CXR_VBOperator_Wave);
		#if 0
		MRTC_REFERENCE(CXR_VBOperator_GenColorViewN_Util);
		MRTC_REFERENCE(CXR_VBOperator_GenAlphaLava_Util);
		#endif
		MRTC_REFERENCE(CXR_VBOperator_Particles);
		MRTC_REFERENCE(CXR_VBOperator_Debug_Normals);
		MRTC_REFERENCE(CXR_VBOperator_Debug_Tangents);
		MRTC_REFERENCE(CXR_VBOperator_MulDecalBlend);
		MRTC_REFERENCE(CXR_VBOperator_UseShader);
		MRTC_REFERENCE(CXR_VBOperator_DepthOffset);
		MRTC_REFERENCE(CXR_VBOperator_DrawOrder);
		MRTC_REFERENCE(CXR_VBOperator_NormalTransform);
		MRTC_REFERENCE(CXR_VBOperator_FP20_Water);
		MRTC_REFERENCE(CXR_VBOperator_TexEnvProj1);

		#if 1
		MRTC_REFERENCE(CXR_VBOperator_NV20_GenEnv);
		MRTC_REFERENCE(CXR_VBOperator_NV20_RMBM2D);
		MRTC_REFERENCE(CXR_VBOperator_MultiTxt);
		MRTC_REFERENCE(CXR_VBOperator_NV20_Fresnel);
		MRTC_REFERENCE(CXR_VBOperator_NV20_YUV2RGB);
		#endif

		#if defined(PLATFORM_DOLPHIN) 
			MRTC_REFERENCE(CXR_VBOperator_GC_Bump_Util);
		#endif

		MRTC_REFERENCE(CXR_BlockNav_Grid);
		MRTC_REFERENCE(CXR_BlockNav_GridBuilder);
		MRTC_REFERENCE(CXR_NavGraph);
		MRTC_REFERENCE(CXR_NavGraph_Writer);
		MRTC_REFERENCE(CXR_NavGraph_Builder);

		MRTC_REFERENCE(CXR_BlockNav);

		#if defined(PLATFORM_DOLPHIN) || defined(PLATFORM_PS2)
		MRTC_REFERENCE(CXR_Shader);
		#endif

	}
};

CRegisterXR g_XRDyn;



