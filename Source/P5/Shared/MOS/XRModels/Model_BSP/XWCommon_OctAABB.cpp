#include "PCH.h"

#include "XWCommon.h"
#include "XWCommon_OctAABB.h"


uint32 BSPOctaAABB_CountChildren(const CBSP_OctaAABBNode * _pNodes,uint32 _iNode, bool _bLeaves)
{
	//Stack
	uint32 liNodes[256];
	liNodes[0] = _iNode;
	uint16 iStackPos = 1;

	//Return value
	uint32 nCount = 0;

	//Iterate
	while( iStackPos )
	{
		const CBSP_OctaAABBNode &Nd = _pNodes[liNodes[iStackPos-1] & 0x7FFFFFFF];
		uint32 NodeMask = liNodes[iStackPos-1] & 0x80000000;
		iStackPos--;

		//Loop over children
		if( NodeMask )
		{
			if( _bLeaves )
			{
				for(int i = 0;(i < 8) && (Nd.m_Max[i] > Nd.m_Min[i]);i++)
				{
					nCount++;
				}
			}
		}
		else
		{
			for(int i = 0;(i < 8) && (Nd.m_Max[i] > Nd.m_Min[i]);i++)
			{
				nCount++;
				liNodes[iStackPos] = (Nd.m_iChildren + i) | NodeMask;
				iStackPos++;
#ifndef M_RTM
				if( iStackPos >= 256 )
					Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");
#endif
			}
		}
	}

	return nCount;
}


void BSPOctaAABB_DrawTree(const CBSP_OctaAABBNode * _pNodes, uint32 _iNode, const CPixel32 * _pColors, uint8 _nColors,
						  const CMat4Dfp32 &_FromU16,
						  CVec3Dfp32 * _pVtx,uint16 * _piV,CPixel32 * _pClr,
						  bool _bLeaves, const CPixel32 *_pLeafColor)
{
	//Stack
	uint32 liNodes[256];
	uint8 lGen[256];
	lGen[0] = 0;
	liNodes[0] = _iNode;
	uint16 iStackPos = 1;
	uint16 iVtx = 0;

	//Iterate
	while( iStackPos )
	{
		const CBSP_OctaAABBNode &Nd = _pNodes[liNodes[iStackPos-1] & 0x7FFFFFFF];
		uint32 NodeMask = liNodes[iStackPos-1] & 0x80000000;
		uint8 Gen = lGen[iStackPos-1];
		iStackPos--;

		//Loop over children
		if( (!NodeMask) || _bLeaves )
		{
			for(int i = 0;(i < 8) && (Nd.m_Max[i] > Nd.m_Min[i]);i++)
			{
				if( !NodeMask )
				{
					liNodes[iStackPos] = (Nd.m_iChildren + i) | NodeMask;
					lGen[iStackPos] = Gen+1;
					iStackPos++;
#ifndef M_RTM
					if( iStackPos >= 256 )
						Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");
#endif
					for( int j = 0;j < 8;j++ ) _pClr[j] = _pColors[Gen];
					_pClr += 8;
				}
				else
				{
					for( int j = 0;j < 8;j++ ) _pClr[j] = *_pLeafColor;
					_pClr += 8;
				}

				//Extract vertices
				CBox3Dfp32 DBox;
				CVec3Dfp32 Mn,Mx;
				Mn.k[0] = Nd.m_Min[i + 0];
				Mn.k[1] = Nd.m_Min[i + 8];
				Mn.k[2] = Nd.m_Min[i + 16];
				Mx.k[0] = Nd.m_Max[i + 0];
				Mx.k[1] = Nd.m_Max[i + 8];
				Mx.k[2] = Nd.m_Max[i + 16];
				Mn.MultiplyMatrix(_FromU16,DBox.m_Min);
				Mx.MultiplyMatrix(_FromU16,DBox.m_Max);

				DBox.GetVertices(_pVtx);
				_pVtx += 8;
				iVtx += 8;

				_piV[0] = 0 + iVtx;
				_piV[1] = 1 + iVtx;
				_piV[2] = 0 + iVtx;
				_piV[3] = 2 + iVtx;
				_piV[4] = 1 + iVtx;
				_piV[5] = 3 + iVtx;
				_piV[6] = 2 + iVtx;
				_piV[7] = 3 + iVtx;

				_piV[8] = 4 + iVtx;
				_piV[9] = 5 + iVtx;
				_piV[10] = 4 + iVtx;
				_piV[11] = 6 + iVtx;
				_piV[12] = 5 + iVtx;
				_piV[13] = 7 + iVtx;
				_piV[14] = 6 + iVtx;
				_piV[15] = 7 + iVtx;

				_piV[16] = 0 + iVtx;
				_piV[17] = 4 + iVtx;
				_piV[18] = 2 + iVtx;
				_piV[19] = 6 + iVtx;
				_piV[20] = 1 + iVtx;
				_piV[21] = 5 + iVtx;
				_piV[22] = 3 + iVtx;
				_piV[23] = 7 + iVtx;

				_piV += 24;
			}
		}
	}
}
