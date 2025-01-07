#include "PCH.h"

#include  "XRVBOperators_GC.h"
#include  "XRUtil.h"

MRTC_IMPLEMENT_DYNAMIC(CXR_VBOperator_GC_Bump, CXR_VBOperator_Util);

/*
  OnOperate().

*/
bool  CXR_VBOperator_GC_Bump::OnOperate(  CXR_VBOperatorContext &_Context, const class CXW_LayerOperation &_Oper, class CXR_VertexBuffer  *_pVB )
{
  MAUTOSTRIP( CXR_VBOperator_GC_Bump_OnOperate, false );

#ifdef  PLATFORM_DOLPHIN
	if( _Context.m_pEngine && _Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_GAMECUBE )
	{
		if( _pVB->m_pAttrib )
		{
      OSReport( "GC BumpCubeMap on channel %d\n", _Context.m_iTexChannel );

      if( _Context.m_VBChainIndex )
        return( true );

			_pVB->m_pAttrib->Attrib_TexGen( _Context.m_iTexChannel, CRC_TEXGENMODE_BUMPCUBEENV );

      //int tmpID = _pVB->m_pAttrib->m_TextureID[0];
			//_pVB->m_pAttrib->Attrib_TextureID( 0, _pVB->m_pAttrib->m_TextureID[1] );
			//_pVB->m_pAttrib->Attrib_TextureID( 1, tmpID );

      MACRO_GetRegisterObject( CTextureContext , pTC, "SYSTEM.TEXTURECONTEXT" );
      if( !pTC )
        return(0);

      OSReport( "Using textures <%s> and <%s>\n", (char *)pTC->GetName( _pVB->m_pAttrib->m_TextureID[0] ), (char *)pTC->GetName( _pVB->m_pAttrib->m_TextureID[1] ) );

			//  Get world-space transform.
			if( _Context.m_pWorld2View )
			{
				CMat4Dfp32 *pW2VInv = _Context.m_pVBM->Alloc_M4();
				if( !pW2VInv )
				  return(false);

				_Context.m_pWorld2View->InverseOrthogonal( *pW2VInv );
				CVec3Dfp32::GetRow( *pW2VInv, 3 ) = 0;
				_pVB->Matrix_Set( pW2VInv, CRC_MATRIX_TEXTURE0 + _Context.m_iTexChannel );
			}
			else
			OSReport("(CXR_VBOperator_GC_Bump::OnOperate) Software fallback not implemented. (This only works on GameCube, lowlife)");
		}
		else
  		OSReport("(CXR_VBOperator_GC_Bump::OnOperate) No attributes.");
  }
#endif
  return( true );
}

/*
  OnTestHWAccelerated().

*/
bool  CXR_VBOperator_GC_Bump::OnTestHWAccelerated( CXR_VBOperatorContext &_Context, class CRC_Attributes *_pAttrib, const class CXW_LayerOperation &_Oper )
{
  MAUTOSTRIP( CXR_VBOperator_GC_Bump_OnTestHWAccelerated, false );

#ifdef  PLATFORM_DOLPHIN
	if( _Context.m_pEngine && _Context.m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_GAMECUBE )
		return true;
	else
		ConOutD("§cf80WARNING: (CXR_VBOperator_GC_Bump::OnTestHWAccelerated) Software fallback not implemented. (This only works on GameCube, lowlife)");
#endif	// PLATFORM_DOLPHIN

  return( false );
}

/*
  OnInitSurface().

*/
void  CXR_VBOperator_GC_Bump::OnInitSurface( class CXW_Surface *_pSurf, const class CXW_LayerOperation &_Oper )
{
  MAUTOSTRIP( CXR_VBOperator_GC_Bump_OnInitSurface, MAUTOSTRIP_VOID );

#ifdef  PLATFORM_DOLPHIN
//  OSReport( "CXR_VBOperator_GC_Bump::OnInitSurface\n" );

	_pSurf->m_Flags |= XW_SURFFLAGS_NEEDNORMALS;
	//_pSurf->m_Flags |= XW_SURFFLAGS_NEEDTANGENTS;

	_pSurf->m_Requirements |= XW_SURFREQ_GAMECUBE;
#endif	// PLATFORM_DOLPHIN
}
