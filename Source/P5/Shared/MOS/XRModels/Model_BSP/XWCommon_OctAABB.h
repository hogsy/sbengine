#ifndef _INC_XWCOMMON_OCTAABB_H
#define _INC_XWCOMMON_OCTAABB_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    File:              XWCommon_OctAABB
					
    Author:            Anders Ekermo
					
    Copyright:         Copyright Starbreeze Studios AB 2006
					
    Contents:          Node class and functions to do with BSP Octree
					
    Comments:          ...
					
    History:
        060719: anek, Created file
\*____________________________________________________________________*/


// TODO
// - Clean out iterator::next of LHS, see if it can be optimized for BSP4
// - Clean out other functions' comments, decide what code to use and what to discard
// - Swap splats in fp32-u16 conversions with Permute if u16 permutes enabled on SSE/SPU
// - Comments according to code standard

#include "../MCC/MCC.h"
#include "../../XR/XRClass.h"
#include "../MCC/MMath_Vec128.h"

#define BSP_OCTAAABBNODE_LEAF		0x80000000
#define BSP_OCTAAABBTREE_VERSION	0x100
#define BSP_OCTAAABBHEADER_VERSION	0x100

#define BSP_OCTAAABBITR_STACKLEN	128
#define BSP_OCTAAABB_LEAFTHRESHOLD	256

#define BSP_USE_OCTAAABB

const vec128 M_V128U16SINGLE[8] = { M_VConst_u16(0xFFFF,0,0,0,0,0,0,0), M_VConst_u16(0,0xFFFF,0,0,0,0,0,0),
	M_VConst_u16(0,0,0xFFFF,0,0,0,0,0),M_VConst_u16(0,0,0,0xFFFF,0,0,0,0),M_VConst_u16(0,0,0,0,0xFFFF,0,0,0),
	M_VConst_u16(0,0,0,0,0,0xFFFF,0,0),M_VConst_u16(0,0,0,0,0,0,0xFFFF,0),M_VConst_u16(0,0,0,0,0,0,0,0xFFFF) };

const vec128 M_V128U16SPLAT[8] = { M_VConst_u8(0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1), 
	M_VConst_u8(2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3),M_VConst_u8(4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5),
	M_VConst_u8(6,7,6,7,6,7,6,7,6,7,6,7,6,7,6,7),M_VConst_u8(8,9,8,9,8,9,8,9,8,9,8,9,8,9,8,9),
	M_VConst_u8(10,11,10,11,10,11,10,11,10,11,10,11,10,11,10,11),
	M_VConst_u8(12,13,12,13,12,13,12,13,12,13,12,13,12,13,12,13),
	M_VConst_u8(14,15,14,15,14,15,14,15,14,15,14,15,14,15,14,15) };

const vec128 M_V128U16ALL = M_VScalar_u16(0xFFFF);

class CBSP_OctaAABBNode
{

public:

	union
	{
		struct  
		{
			vec128	m_MinV[3];
			vec128	m_MaxV[3];
		};
		struct
		{
			uint16	m_Min[24];
			uint16	m_Max[24];
		};
	};

	uint32 m_iChildren;

#ifndef PLATFORM_CONSOLE
	void Write(CDataFile * _pFile)
	{
		_pFile->GetFile()->WriteLE(m_Min,24);
		_pFile->GetFile()->WriteLE(m_Max,24);
		_pFile->GetFile()->WriteLE(m_iChildren);
	}
#endif

	void Read(CCFile * _pFile,int _Version = BSP_OCTAAABBTREE_VERSION)
	{
		_pFile->ReadLE(m_Min,24);
		_pFile->ReadLE(m_Max,24);
		_pFile->ReadLE(m_iChildren);
	}

	void SwapLE()
	{
		for(int i = 0;i < 24;i++)
		{
			::SwapLE(m_Min[i]);
			::SwapLE(m_Max[i]);
		}
		::SwapLE(m_iChildren);
	}
};

typedef TPtr<CBSP_OctaAABBNode>	spCBSP_OctaAABBNode;
typedef TThinArray<CBSP_OctaAABBNode> CBSP_OctaAABBList;


template<typename TModel,typename TPortalLeaf>
class CBSP_OctaAABBIterator
{

private:
	vec128	m_EnumMin[3],m_EnumMax[3];
	vec128	m_HitMask;

	uint32	m_lStack[BSP_OCTAAABBITR_STACKLEN];
	uint32	m_iNext;
	const CBSP_OctaAABBNode * M_RESTRICT m_pNodes;
	const TPortalLeaf * M_RESTRICT m_pLeaves;
	uint16	m_iStackPos;
	uint8	m_iChild;

public:

	uint16	m_iLeaf;

	CBSP_OctaAABBIterator(const CBSP_OctaAABBNode * _pNodes,const TPortalLeaf * _pLeaves,
		const uint16 * _pBox)
	{
		m_pNodes = _pNodes;
		m_pLeaves = _pLeaves;

		m_EnumMin[0] = M_VLdScalar_u16(_pBox[0]);
		m_EnumMin[1] = M_VLdScalar_u16(_pBox[1]);
		m_EnumMin[2] = M_VLdScalar_u16(_pBox[2]);
		m_EnumMax[0] = M_VLdScalar_u16(_pBox[3]);
		m_EnumMax[1] = M_VLdScalar_u16(_pBox[4]);
		m_EnumMax[2] = M_VLdScalar_u16(_pBox[5]);

		m_iNext = (_pLeaves == NULL) ? BSP_OCTAAABBNODE_LEAF : 0;
		m_lStack[0] = m_iNext;
		m_iStackPos = 1;
		m_iChild = 8;
		m_HitMask = M_VScalar(0);
	}

	M_FORCEINLINE CBSP_OctaAABBIterator(const TModel * _pModel, const TPortalLeaf * _pLeaves,
		const CBox3Dfp32 &_EnumBox)
	{
		//Modifier to uint16-space
		{
			vec128 v_zero = M_VScalar(0),v_maxu16 = M_VScalar(65535.0f),v_one = M_VScalar(1.0f);
			vec128 v_min,v_max;

			v_min = M_VLd_V3_Slow(&_EnumBox.m_Min);
			v_max = M_VLd_V3_Slow(&_EnumBox.m_Max);
			
			v_min = M_VSub(v_min,_pModel->m_OctaAABBTranslate);
			v_max = M_VSub(v_max,_pModel->m_OctaAABBTranslate);
			v_min = M_VMul(v_min,_pModel->m_OctaAABBScale);
			v_max = M_VAdd( M_VMul(v_max,_pModel->m_OctaAABBScale), v_one );

			v_min = M_VCnv_f32_i32(v_min);
			v_max = M_VCnv_f32_i32(v_max);

			//Splat
			m_EnumMin[0] = M_VSplatX(v_min);
			m_EnumMin[0] = M_VCnvS_i32_u16(m_EnumMin[0],m_EnumMin[0]);
			m_EnumMin[1] = M_VSplatY(v_min);
			m_EnumMin[1] = M_VCnvS_i32_u16(m_EnumMin[1],m_EnumMin[1]);
			m_EnumMin[2] = M_VSplatZ(v_min);
			m_EnumMin[2] = M_VCnvS_i32_u16(m_EnumMin[2],m_EnumMin[2]);
			m_EnumMax[0] = M_VSplatX(v_max);
			m_EnumMax[0] = M_VCnvS_i32_u16(m_EnumMax[0],m_EnumMax[0]);
			m_EnumMax[1] = M_VSplatY(v_max);
			m_EnumMax[1] = M_VCnvS_i32_u16(m_EnumMax[1],m_EnumMax[1]);
			m_EnumMax[2] = M_VSplatZ(v_max);
			m_EnumMax[2] = M_VCnvS_i32_u16(m_EnumMax[2],m_EnumMax[2]);
		}

		m_iNext = (_pLeaves == NULL) ? BSP_OCTAAABBNODE_LEAF : 0;
		m_lStack[0] = m_iNext;
		m_iStackPos = 1;
		m_iChild = 8;

		m_pNodes = _pModel->m_lOctaAABBNodes.GetBasePtr();
		m_pLeaves = _pLeaves;
		m_HitMask = M_VScalar(0);
	}


	//enum face
	M_FORCEINLINE uint32 Next()
	{
		while( m_iChild < 8 )
		{ 
			vec128 Test = M_VAnd(m_HitMask,M_V128U16SINGLE[m_iChild]);
			if( M_VCmpAnyEq_u16(M_V128U16ALL,Test) )
			{
				uint32 Ret = (m_pNodes[m_lStack[m_iStackPos] & 0x7FFFFFFF].m_iChildren & ~BSP_OCTAAABBNODE_LEAF) + m_iChild;
				m_iChild++;
				return Ret;
			}
			m_iChild++;
		}

		while( m_iStackPos )
		{
			uint32 iCurrent = (m_iNext == 0xFFFFFFFF) ? m_lStack[m_iStackPos-1] : m_iNext;
			uint32 NodeMask = iCurrent & 0x80000000;

			if( iCurrent & 0x40000000 )
			{
				m_iLeaf = iCurrent & 0x80000000;
				iCurrent = m_pLeaves[m_iLeaf].m_iOctaAABBNode;
			}

			const CBSP_OctaAABBNode &Nd = m_pNodes[iCurrent & 0x3FFFFFFF];
			m_iStackPos--;
			m_iNext = 0xFFFFFFFF;

			//Check
			vec128 MinTest[3],MaxTest[3],Res;
			MinTest[0] = M_VCmpLTMsk_u16(m_EnumMin[0],Nd.m_MaxV[0]);
			MinTest[1] = M_VCmpLTMsk_u16(m_EnumMin[1],Nd.m_MaxV[1]);
			MinTest[2] = M_VCmpLTMsk_u16(m_EnumMin[2],Nd.m_MaxV[2]);
			MaxTest[0] = M_VCmpGTMsk_u16(m_EnumMax[0],Nd.m_MinV[0]);
			MaxTest[1] = M_VCmpGTMsk_u16(m_EnumMax[1],Nd.m_MinV[1]);
			MaxTest[2] = M_VCmpGTMsk_u16(m_EnumMax[2],Nd.m_MinV[2]);

			Res = M_VAnd(M_VAnd(M_VAnd(MinTest[0],MinTest[1]),M_VAnd(MinTest[2],MaxTest[0])),M_VAnd(MaxTest[1],MaxTest[2]));

			if( Nd.m_iChildren & BSP_OCTAAABBNODE_LEAF )
			{
				// Process faces
				if( NodeMask )
				{
					int i;
					for(i = 0;i < 8;i++)
					{
						vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
						if( M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) break;
					}

					if( i >= 8 ) continue;
					
					m_iChild = i+1;
					m_HitMask = Res;
					return (Nd.m_iChildren & ~BSP_OCTAAABBNODE_LEAF) + i;
				}

				//Process portal leaves
				else
				{
					for(int i = 0;i < 8;i++)
					{
						vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
						if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;

						uint32 iLeaf = (Nd.m_iChildren & ~BSP_OCTAAABBNODE_LEAF) + i;
						m_lStack[m_iStackPos] = iLeaf | 0xC0000000;
						m_iStackPos++;
#ifndef M_RTM
						if( m_iStackPos >= BSP_OCTAAABBITR_STACKLEN )
							Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");
#endif
					}
				}
			}
			else
			{
				//Loop over children
				for(int i = 0;i < 8;i++)
				{
					vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
					if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;

					m_iNext = (Nd.m_iChildren + i) | NodeMask;
					m_lStack[m_iStackPos] = m_iNext;
					m_iStackPos++;
#ifndef M_RTM
					if( m_iStackPos >= 256 )
						Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");
#endif
				}
			}
		}

		//End of tree
		return 0xFFFFFFFF;
	}

};

template<typename TModel>
static uint32 BSPOctaAABB_EnumPL(const TModel * M_RESTRICT _pModel,uint16 * M_RESTRICT _pPL,uint32 _nMax,
								 CBox3Dfp32 &_Box,const uint8 * M_RESTRICT _pPVS)
{
//	uint16 RetMinX = 0xFFFF, RetMinY = 0xFFFF, RetMinZ = 0xFFFF;
//	uint16 RetMaxX = 0, RetMaxY = 0, RetMaxZ = 0;
//	uint16 EnumMinX,EnumMinY,EnumMinZ,EnumMaxX,EnumMaxY,EnumMaxZ;
//	vec128 EnumMin,EnumMax,RetMin = M_VScalar_u16(0xFFFF),RetMax = M_VScalar_u16(0);
	vec128 BoxMinX,BoxMinY,BoxMinZ,BoxMaxX,BoxMaxY,BoxMaxZ;

	uint32 nPL = 0;

	//Prepare min-max
	{	
		vec128 v_zero = M_VScalar(0),v_maxu16 = M_VScalar(65535.0f),v_one = M_VScalar(1.0f);
		// CVec3Dfp32 Mn,Mx;
		vec128 v_min,v_max;
		v_min = M_VLd_V3_Slow(&_Box.m_Min);
		v_max = M_VLd_V3_Slow(&_Box.m_Max);

		v_min = M_VSub(v_min,_pModel->m_OctaAABBTranslate);
		v_max = M_VSub(v_max,_pModel->m_OctaAABBTranslate);
		v_min = M_VMul(v_min,_pModel->m_OctaAABBScale);
		v_max = M_VAdd( M_VMul(v_max,_pModel->m_OctaAABBScale), v_one );

		//_Box.m_Min.MultiplyMatrix(_pModel->m_OctaAABBToU16,Mn);
		//_Box.m_Max.MultiplyMatrix(_pModel->m_OctaAABBToU16,Mx);
/*
		uint16 EnumBox[6];
		EnumBox[0] = (uint16)(Max(Mn.k[0],0.0f));
		EnumBox[1] = (uint16)(Max(Mn.k[1],0.0f));
		EnumBox[2] = (uint16)(Max(Mn.k[2],0.0f));
		EnumBox[3] = (uint16)(Min(Mx.k[0]+1.0f,65535.0f));
		EnumBox[4] = (uint16)(Min(Mx.k[1]+1.0f,65535.0f));
		EnumBox[5] = (uint16)(Min(Mx.k[2]+1.0f,65535.0f));
		*/
		v_min = M_VCnv_f32_i32(v_min);
		v_max = M_VCnv_f32_i32(v_max);
//		EnumMin = M_VCnvS_i32_u16(v_min,v_maxu16);
//		EnumMax = M_VCnvS_i32_u16(v_max,v_zero);

		//Splat
		BoxMinX = M_VSplatX(v_min);
		BoxMinX = M_VCnvS_i32_u16(BoxMinX,BoxMinX);
		BoxMinY = M_VSplatY(v_min);
		BoxMinY = M_VCnvS_i32_u16(BoxMinY,BoxMinY);
		BoxMinZ = M_VSplatZ(v_min);
		BoxMinZ = M_VCnvS_i32_u16(BoxMinZ,BoxMinZ);
		BoxMaxX = M_VSplatX(v_max);
		BoxMaxX = M_VCnvS_i32_u16(BoxMaxX,BoxMaxX);
		BoxMaxY = M_VSplatY(v_max);
		BoxMaxY = M_VCnvS_i32_u16(BoxMaxY,BoxMaxY);
		BoxMaxZ = M_VSplatZ(v_max);
		BoxMaxZ = M_VCnvS_i32_u16(BoxMaxZ,BoxMaxZ);
/*
		BoxMinX = M_VLdScalar_u16(EnumBox[0]);
		BoxMinY = M_VLdScalar_u16(EnumBox[1]);
		BoxMinZ = M_VLdScalar_u16(EnumBox[2]);
		BoxMaxX = M_VLdScalar_u16(EnumBox[3]);
		BoxMaxY = M_VLdScalar_u16(EnumBox[4]);
		BoxMaxZ = M_VLdScalar_u16(EnumBox[5]);
*/
		//Create box
		/*
		EnumMin = M_VAdd_u16( M_VAnd(BoxMinX,M_V128U16SINGLE[0]), M_VAnd(BoxMinY,M_V128U16SINGLE[1]) );
		EnumMin = M_VAdd_u16( M_VAnd(BoxMinZ,M_V128U16SINGLE[2]), EnumMin );
		EnumMax = M_VAdd_u16( M_VAnd(BoxMaxX,M_V128U16SINGLE[0]), M_VAnd(BoxMaxY,M_V128U16SINGLE[1]) );
		EnumMax = M_VAdd_u16( M_VAnd(BoxMaxZ,M_V128U16SINGLE[2]), EnumMax );
		*/
/*
		CVec128Access acc;
		acc = BoxMinX; EnumMinX = acc.ku16[0];
		acc = BoxMinY; EnumMinY = acc.ku16[0];
		acc = BoxMinZ; EnumMinZ = acc.ku16[0];
		acc = BoxMaxX; EnumMaxX = acc.ku16[0];
		acc = BoxMaxY; EnumMaxY = acc.ku16[0];
		acc = BoxMaxZ; EnumMaxZ = acc.ku16[0];
*/
	}

	//Declare stack
	uint32 iStackPos = 1;
	uint32 iNext = 0;
	uint32 liNodes[BSP_OCTAAABBITR_STACKLEN];
	liNodes[0] = 0;

	TAP<const CBSP_OctaAABBNode> pNodes = _pModel->m_lOctaAABBNodes;
	//const CBSP_OctaAABBNode * M_RESTRICT pNodes = _pModel->m_lOctaAABBNodes.GetBasePtr();

	//Loop through
	while( iStackPos )
	{
		//Extract node
		const CBSP_OctaAABBNode * M_RESTRICT pNd;
		if( iNext == 0xFFFFFFFF )
		{
			pNd = &pNodes[liNodes[iStackPos-1]];
		}
		else
		{
			pNd = &pNodes[iNext];
			iNext = 0xFFFFFFFF;
		}
		iStackPos--;
		
		//Check
		vec128 MinTestX,MinTestY,MinTestZ;
		vec128 MaxTestX,MaxTestY,MaxTestZ,Res;
		MinTestX = M_VCmpLTMsk_u16(BoxMinX,pNd->m_MaxV[0]);
		MinTestY = M_VCmpLTMsk_u16(BoxMinY,pNd->m_MaxV[1]);
		MinTestZ = M_VCmpLTMsk_u16(BoxMinZ,pNd->m_MaxV[2]);
		MaxTestX = M_VCmpGTMsk_u16(BoxMaxX,pNd->m_MinV[0]);
		MaxTestY = M_VCmpGTMsk_u16(BoxMaxY,pNd->m_MinV[1]);
		MaxTestZ = M_VCmpGTMsk_u16(BoxMaxZ,pNd->m_MinV[2]);

		Res = M_VAnd(M_VAnd(M_VAnd(MinTestX,MinTestY),M_VAnd(MinTestZ,MaxTestX)),M_VAnd(MaxTestY,MaxTestZ));

		//Found leaf, expand boundingbox
		if( pNd->m_iChildren & BSP_OCTAAABBNODE_LEAF )
		{
			//We need to address the portal leaves only
			for(int i = 0;i < 8;i++)
			{
				vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
				if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;

				//Check against PVS
				uint32 iLeaf = (pNd->m_iChildren & ~BSP_OCTAAABBNODE_LEAF) + i;
				if( !(_pPVS[iLeaf >> 3] & M_BitD(iLeaf & 7)) ) continue;

				_pPL[nPL] = iLeaf;
				nPL++;

#ifndef M_RTM
				if(nPL >= _nMax)
				{
					M_TRACEALWAYS("WARNING! BSPOctaAABB_EnumPL - Buffer overflow!");
					return nPL;
				}
#endif

				//Create vec128 box
				/*
				vec128 CurMin,CurMax;
				CurMin = M_VAnd( M_V128U16SINGLE[0], M_VPerm_Helper(pNd->m_MinV[0],M_VScalar_u16(0),M_V128U16SPLAT[i]));
				CurMin = M_VAdd_u16( CurMin, M_VAnd( M_V128U16SINGLE[1], M_VPerm_Helper(pNd->m_MinV[1],CurMin,M_V128U16SPLAT[i])));
				CurMin = M_VAdd_u16( CurMin, M_VAnd( M_V128U16SINGLE[2], M_VPerm_Helper(pNd->m_MinV[2],CurMin,M_V128U16SPLAT[i])));
				CurMax = M_VAnd( M_V128U16SINGLE[0], M_VPerm_Helper(pNd->m_MaxV[0],M_VScalar_u16(0),M_V128U16SPLAT[i]));
				CurMax = M_VAdd_u16( CurMax, M_VAnd( M_V128U16SINGLE[1], M_VPerm_Helper(pNd->m_MaxV[1],CurMax,M_V128U16SPLAT[i])));
				CurMax = M_VAdd_u16( CurMax, M_VAnd( M_V128U16SINGLE[2], M_VPerm_Helper(pNd->m_MaxV[2],CurMax,M_V128U16SPLAT[i])));
				
				RetMax = M_VMax_u16( RetMax, M_VMin_u16(EnumMax,CurMax) );
				RetMin = M_VMin_u16( RetMin, M_VMax_u16(EnumMin,CurMin) );
				*/
				/*
				RetMaxX = Max<uint16>( RetMaxX, Min<uint16>(EnumMaxX,pNd->m_Max[i]));
				RetMaxY = Max<uint16>( RetMaxY, Min<uint16>(EnumMaxY,pNd->m_Max[i+8]));
				RetMaxZ = Max<uint16>( RetMaxZ, Min<uint16>(EnumMaxZ,pNd->m_Max[i+16]));
				RetMinX = Min<uint16>( RetMinX, Max<uint16>(EnumMinX,pNd->m_Min[i]));
				RetMinY = Min<uint16>( RetMinY, Max<uint16>(EnumMinY,pNd->m_Min[i+8]));
				RetMinZ = Min<uint16>( RetMinZ, Max<uint16>(EnumMinZ,pNd->m_Min[i+16]));
				*/
			}
		}

		//Keep searching...
		else
		{
			//Loop over children
			for(int i = 0;i < 8;i++)
			{
				vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
				if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;
	
				iNext = pNd->m_iChildren + i;
				liNodes[iStackPos] = iNext;
				iStackPos++;
#ifndef M_RTM
				if( iStackPos >= BSP_OCTAAABBITR_STACKLEN )
					Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");
#endif
			}
		}
	}

	//Finalize
	{
	//	CVec128Access AccMin = RetMin,AccMax = RetMax;

	//	CVec3Dfp32 Mn,Mx;
		/*
		Mn.k[0] = (fp32)(AccMin.ku16[0]);
		Mn.k[1] = (fp32)(AccMin.ku16[1]);
		Mn.k[2] = (fp32)(AccMin.ku16[2]);
		Mx.k[0] = (fp32)(AccMax.ku16[0]);
		Mx.k[1] = (fp32)(AccMax.ku16[1]);
		Mx.k[2] = (fp32)(AccMax.ku16[2]);
		*/
		/*
		Mn.k[0] = (fp32)(RetMinX);
		Mn.k[1] = (fp32)(RetMinY);
		Mn.k[2] = (fp32)(RetMinZ);
		Mx.k[0] = (fp32)(RetMaxX);
		Mx.k[1] = (fp32)(RetMaxY);
		Mx.k[2] = (fp32)(RetMaxZ);
		*/
/*
		vec128 v_min = M_VCnv_i32_f32( M_VCnvL_u16_u32( RetMin ) );
		vec128 v_max = M_VCnv_i32_f32( M_VCnvL_u16_u32( RetMax ) );

		v_min = M_VAdd( M_VMul( v_min, _pModel->m_OctaAABBInvScale ), _pModel->m_OctaAABBTranslate );
		v_max = M_VAdd( M_VMul( v_max, _pModel->m_OctaAABBInvScale ), _pModel->m_OctaAABBTranslate );

		_Box.m_Min = M_VGetV3_Slow(v_min);
		_Box.m_Max = M_VGetV3_Slow(v_max);
*/
		
		//Mn.MultiplyMatrix(_pModel->m_OctaAABBFromU16,_Box.m_Min);
		//Mx.MultiplyMatrix(_pModel->m_OctaAABBFromU16,_Box.m_Max);
	}

	return nPL;
}


// quick leaf find
template <typename TModel>
static uint32 BSPOctaAABB_EnumLeaf(const TModel * M_RESTRICT _pModel,uint32 * M_RESTRICT _pL,uint _nMaxRet,
								 const CBox3Dfp32 &_Box)
{
	vec128 BoxMinX,BoxMinY,BoxMinZ,BoxMaxX,BoxMaxY,BoxMaxZ;
	uint32 nL = 0;

	//Prepare min-max
	{	
		vec128 v_zero = M_VScalar(0),v_maxu16 = M_VScalar(65535.0f),v_one = M_VScalar(1.0f);
		vec128 v_min,v_max;
		v_min = M_VLd_V3_Slow(&_Box.m_Min);
		v_max = M_VLd_V3_Slow(&_Box.m_Max);

		v_min = M_VSub(v_min,_pModel->m_OctaAABBTranslate);
		v_max = M_VSub(v_max,_pModel->m_OctaAABBTranslate);
		v_min = M_VMul(v_min,_pModel->m_OctaAABBScale);
		v_max = M_VAdd( M_VMul(v_max,_pModel->m_OctaAABBScale), v_one );

		v_min = M_VCnv_f32_i32(v_min);
		v_max = M_VCnv_f32_i32(v_max);

		//Splat
		BoxMinX = M_VSplatX(v_min);
		BoxMinX = M_VCnvS_i32_u16(BoxMinX,BoxMinX);
		BoxMinY = M_VSplatY(v_min);
		BoxMinY = M_VCnvS_i32_u16(BoxMinY,BoxMinY);
		BoxMinZ = M_VSplatZ(v_min);
		BoxMinZ = M_VCnvS_i32_u16(BoxMinZ,BoxMinZ);
		BoxMaxX = M_VSplatX(v_max);
		BoxMaxX = M_VCnvS_i32_u16(BoxMaxX,BoxMaxX);
		BoxMaxY = M_VSplatY(v_max);
		BoxMaxY = M_VCnvS_i32_u16(BoxMaxY,BoxMaxY);
		BoxMaxZ = M_VSplatZ(v_max);
		BoxMaxZ = M_VCnvS_i32_u16(BoxMaxZ,BoxMaxZ);
	}

	//Declare stack
	uint32 iStackPos = 1;
	uint32 iNext = 0;
	uint32 liNodes[BSP_OCTAAABBITR_STACKLEN];
	liNodes[0] = 0;

	TAP<const CBSP_OctaAABBNode> pNodes = _pModel->m_lOctaAABBNodes;
	//const CBSP_OctaAABBNode * M_RESTRICT pNodes = _pModel->m_lOctaAABBNodes.GetBasePtr();

	//Loop through
	while( iStackPos )
	{
		//Extract node
		const CBSP_OctaAABBNode * M_RESTRICT pNd;
		if( iNext == 0xFFFFFFFF )
		{
			pNd = &pNodes[liNodes[iStackPos-1]];
		}
		else
		{
			pNd = &pNodes[iNext];
			iNext = 0xFFFFFFFF;
		}
		iStackPos--;
		
		//Check
		vec128 MinTestX,MinTestY,MinTestZ;
		vec128 MaxTestX,MaxTestY,MaxTestZ,Res;
		MinTestX = M_VCmpLTMsk_u16(BoxMinX,pNd->m_MaxV[0]);
		MinTestY = M_VCmpLTMsk_u16(BoxMinY,pNd->m_MaxV[1]);
		MinTestZ = M_VCmpLTMsk_u16(BoxMinZ,pNd->m_MaxV[2]);
		MaxTestX = M_VCmpGTMsk_u16(BoxMaxX,pNd->m_MinV[0]);
		MaxTestY = M_VCmpGTMsk_u16(BoxMaxY,pNd->m_MinV[1]);
		MaxTestZ = M_VCmpGTMsk_u16(BoxMaxZ,pNd->m_MinV[2]);

		Res = M_VAnd(M_VAnd(M_VAnd(MinTestX,MinTestY),M_VAnd(MinTestZ,MaxTestX)),M_VAnd(MaxTestY,MaxTestZ));

		//Found leaf, expand boundingbox
		if( pNd->m_iChildren & BSP_OCTAAABBNODE_LEAF )
		{
			//We need to address the portal leaves only
			for(int i = 0;i < 8;i++)
			{
				vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
				if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;

				_pL[nL] = (pNd->m_iChildren & ~BSP_OCTAAABBNODE_LEAF) + i;
				nL++;

				if (nL >= _nMaxRet)
					return _nMaxRet;
			}
		}

		//Keep searching...
		else
		{
			//Loop over children
			for(int i = 0;i < 8;i++)
			{
				vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
				if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;
	
				iNext = pNd->m_iChildren + i;
				liNodes[iStackPos] = iNext;
				iStackPos++;
#ifndef M_RTM
				if( iStackPos >= BSP_OCTAAABBITR_STACKLEN )
					Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");
#endif
			}
		}
	}

	return nL;
}


template<typename TModel,typename TEnumContext,typename TPortalLeaf,typename TFace>
static void BSPOctaAABB_EnumFaces(const TModel * _pModel, CXR_PhysicsContext * _pPhysContext, 
								  const TPortalLeaf * _pLeaves,TEnumContext * _pEnumContext,
								  const bool _bUseSphere, uint32 _Mask = 0 )
{
	vec128 BoxMin[3],BoxMax[3];

	//Prepare min-max
	{	
		/*
		uint16 lBox[6];
		CVec3Dfp32 Mn,Mx;

		if( _bUseSphere )
		{
			CVec3Dfp32 Rad(_pEnumContext->m_EnumSphereR,_pEnumContext->m_EnumSphereR,_pEnumContext->m_EnumSphereR);
			(_pEnumContext->m_EnumSphere - Rad).MultiplyMatrix(_pModel->m_OctaAABBToU16,Mn);
			(_pEnumContext->m_EnumSphere + Rad).MultiplyMatrix(_pModel->m_OctaAABBToU16,Mx);
		}
		else
		{
			_pEnumContext->m_EnumBox.m_Min.MultiplyMatrix(_pModel->m_OctaAABBToU16,Mn);
			_pEnumContext->m_EnumBox.m_Max.MultiplyMatrix(_pModel->m_OctaAABBToU16,Mx);
		}
		
		lBox[0] = (uint16)(Max(Mn.k[0],0.0f));
		lBox[1] = (uint16)(Max(Mn.k[1],0.0f));
		lBox[2] = (uint16)(Max(Mn.k[2],0.0f));
		lBox[3] = (uint16)(Min(Mx.k[0]+1.0f,65535.0f));
		lBox[4] = (uint16)(Min(Mx.k[1]+1.0f,65535.0f));
		lBox[5] = (uint16)(Min(Mx.k[2]+1.0f,65535.0f));

		for(int i = 0;i < 3;i++)
		{
			BoxMin[i] = M_VLdScalar_u16(lBox[i]);
			BoxMax[i] = M_VLdScalar_u16(lBox[i+3]);
		}
		*/

		vec128 v_zero = M_VScalar(0),v_maxu16 = M_VScalar(65535.0f),v_one = M_VScalar(1.0f);
		vec128 v_min,v_max;

		if( _bUseSphere )
		{
			vec128 v_rad = M_VLdScalar(_pEnumContext->m_EnumSphereR);
			v_min = M_VLd_V3_Slow(&_pEnumContext->m_EnumSphere);
			v_max = M_VAdd(v_min,v_rad);
			v_min = M_VSub(v_min,v_rad);
		}
		else
		{
			v_min = M_VLd_V3_Slow(&_pEnumContext->m_EnumBox.m_Min);
			v_max = M_VLd_V3_Slow(&_pEnumContext->m_EnumBox.m_Max);
		}

		v_min = M_VSub(v_min,_pModel->m_OctaAABBTranslate);
		v_max = M_VSub(v_max,_pModel->m_OctaAABBTranslate);
		v_min = M_VMul(v_min,_pModel->m_OctaAABBScale);
		v_max = M_VAdd( M_VMul(v_max,_pModel->m_OctaAABBScale), v_one );

		v_min = M_VCnv_f32_i32(v_min);
		v_max = M_VCnv_f32_i32(v_max);

		//Splat
		BoxMin[0] = M_VSplatX(v_min);
		BoxMin[0] = M_VCnvS_i32_u16(BoxMin[0],BoxMin[0]);
		BoxMin[1] = M_VSplatY(v_min);
		BoxMin[1] = M_VCnvS_i32_u16(BoxMin[1],BoxMin[1]);
		BoxMin[2] = M_VSplatZ(v_min);
		BoxMin[2] = M_VCnvS_i32_u16(BoxMin[2],BoxMin[2]);
		BoxMax[0] = M_VSplatX(v_max);
		BoxMax[0] = M_VCnvS_i32_u16(BoxMax[0],BoxMax[0]);
		BoxMax[1] = M_VSplatY(v_max);
		BoxMax[1] = M_VCnvS_i32_u16(BoxMax[1],BoxMax[1]);
		BoxMax[2] = M_VSplatZ(v_max);
		BoxMax[2] = M_VCnvS_i32_u16(BoxMax[2],BoxMax[2]);
	}

	//Stack
	uint32 liNodes[256];
	liNodes[0] = _Mask;
	uint32 iNext = _Mask;
	uint16 iStackPos = 1;

	//Extra
	uint8* pFTag = _pEnumContext->m_pFTag;
	uint32* piFUntag = _pEnumContext->m_piFUntag;

	//Iterate
	TAP<const CBSP_OctaAABBNode> pNodes = _pModel->m_lOctaAABBNodes;
	while( iStackPos )
	{
		uint32 iNode = (iNext == 0xFFFFFFFF) ? liNodes[iStackPos-1] : iNext;
		iNext = 0xFFFFFFFF;

		uint32 NodeMask = iNode & 0x80000000;
		
		if( iNode & 0x40000000 )
		{
			uint32 iLeaf = iNode & 0x3FFFFFFF;
			_pEnumContext->m_EnumPL = iLeaf;
			iNode = _pLeaves[iLeaf].m_iOctaAABBNode;
		}

		const CBSP_OctaAABBNode &Nd = pNodes[iNode & 0x3FFFFFFF];
		iStackPos--;
		
		//Check
		vec128 MinTest[3],MaxTest[3],Res;
		MinTest[0] = M_VCmpLTMsk_u16(BoxMin[0],Nd.m_MaxV[0]);
		MinTest[1] = M_VCmpLTMsk_u16(BoxMin[1],Nd.m_MaxV[1]);
		MinTest[2] = M_VCmpLTMsk_u16(BoxMin[2],Nd.m_MaxV[2]);
		MaxTest[0] = M_VCmpGTMsk_u16(BoxMax[0],Nd.m_MinV[0]);
		MaxTest[1] = M_VCmpGTMsk_u16(BoxMax[1],Nd.m_MinV[1]);
		MaxTest[2] = M_VCmpGTMsk_u16(BoxMax[2],Nd.m_MinV[2]);

		Res = M_VAnd(M_VAnd(M_VAnd(MinTest[0],MinTest[1]),M_VAnd(MinTest[2],MaxTest[0])),M_VAnd(MaxTest[1],MaxTest[2]));

		if( Nd.m_iChildren & BSP_OCTAAABBNODE_LEAF )
		{
			if( NodeMask )
			{
				//Loop over faces
				for(int i = 0;i < 8;i++)
				{
					vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
					if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;

					uint32 iFace = (Nd.m_iChildren & ~BSP_OCTAAABBNODE_LEAF) + i;
					uint32 iFaceIdx = iFace >> 3;
					uint8 iFaceMask = 1 << (iFace & 7);

					//Tagged already (this *might* not be necessary)
					if( pFTag[iFaceIdx] & iFaceMask) continue;

					//Unused tag
					if( !pFTag[iFaceIdx] )
					{
						if(_pEnumContext->m_nUntagEnum >= _pEnumContext->m_MaxUntagEnum)
							M_ASSERT(0,"UntagEnum overflow");
						piFUntag[_pEnumContext->m_nUntagEnum++] = iFaceIdx;
					}
					pFTag[iFaceIdx] |= iFaceMask;

	//				if (_pPhysContext->m_pWC) 
	//					_pModel->__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff100020);

					//Plane testing (might not be necessary either)
					const TFace* pF = &_pModel->m_pFaces[iFace];
					int iPlane = pF->m_iPlane;
					fp32 MinDist;
					
					if( _bUseSphere )
						MinDist = Abs( _pModel->m_pPlanes[iPlane].Distance(_pEnumContext->m_EnumSphere) ) - _pEnumContext->m_EnumSphereR;
					else 
						MinDist = _pModel->m_pPlanes[iPlane].GetBoxMinDistance(_pEnumContext->m_EnumBox.m_Min, _pEnumContext->m_EnumBox.m_Max);
					
					if (MinDist > 0.001f) continue;

					//Faceflag, medium
					if (_pEnumContext->m_EnumQuality & TModel::ENUM_FACEFLAGS && !(_pEnumContext->m_EnumFaceFlags & pF->m_Flags)) continue;
					if (_pEnumContext->m_EnumQuality & TModel::ENUM_MEDIUMFLAGS && !(_pModel->m_lMediums[pF->m_iBackMedium].m_MediumFlags & _pEnumContext->m_EnumMedium)) continue;

					//We be done
					if( _pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
					{
						_pEnumContext->m_piEnum[_pEnumContext->m_nEnum] = iFace;
						if (_pEnumContext->m_piEnumPL) _pEnumContext->m_piEnumPL[_pEnumContext->m_nEnum] = _pEnumContext->m_EnumPL;
						_pEnumContext->m_nEnum++;
					}
					else
					{
						_pEnumContext->m_bEnumError = true;
						return;
					}
				}
			}
			else
			{
				//We need to address the portal leaves
				for(int i = 0;i < 8;i++)
				{
					vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
					if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;

					uint32 iLeaf = (Nd.m_iChildren & ~BSP_OCTAAABBNODE_LEAF) + i;
					iNext = iLeaf | 0xC0000000;
					liNodes[iStackPos] = iNext;
					iStackPos++;

#ifndef M_RTM
					if( iStackPos >= 256 )
						Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");
#endif
				}
			}
		}
		else
		{
			//Loop over children
			for(int i = 0;i < 8;i++)
			{
				vec128 Test = M_VAnd(Res,M_V128U16SINGLE[i]);
				if( !M_VCmpAnyEq_u16(M_V128U16ALL,Test) ) continue;
				
				iNext = (Nd.m_iChildren + i) | NodeMask;
				liNodes[iStackPos] = iNext;
				iStackPos++;
#ifndef M_RTM
				if( iStackPos >= 256 )
					Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");
#endif
			}
		}
	}
}


#ifdef M_Profile

uint32 BSPOctaAABB_CountChildren(const CBSP_OctaAABBNode * _pNodes,uint32 _iNode, bool _bLeaves = false);

void BSPOctaAABB_DrawTree(const CBSP_OctaAABBNode * _pNodes, uint32 _iNode, const CPixel32 * _pColors, uint8 _nColors,
						  const CMat4Dfp32 &_FromU16,
						  CVec3Dfp32 * _pVtx,uint16 * _piV,CPixel32 * _pClr,
						  bool _bLeaves = false, const CPixel32 *_pLeafColor = NULL);

template<typename TPortalLeaf>
bool BSPOctaAABB_ValidateVolume_r(const CBSP_OctaAABBNode * _pNodes,uint32 _iNode,
								  const TPortalLeaf * _pLeaves, TBox<uint16> *_pBox = NULL)
{
	const CBSP_OctaAABBNode &Nd = _pNodes[_iNode & 0x7FFFFFFF];

	for(int i = 0;(i < 8) && (Nd.m_Max[i] > Nd.m_Min[i]);i++)
	{
		TBox<uint16> NewBox;
		NewBox.m_Min.k[0] = Nd.m_Min[i];
		NewBox.m_Min.k[1] = Nd.m_Min[8+i];
		NewBox.m_Min.k[2] = Nd.m_Min[16+i];
		NewBox.m_Max.k[0] = Nd.m_Max[i];
		NewBox.m_Max.k[1] = Nd.m_Max[8+i];
		NewBox.m_Max.k[2] = Nd.m_Max[16+i];
		
		if( _pBox && (!NewBox.IsCovering(*_pBox)) ) return false;

		if( Nd.m_iChildren & BSP_OCTAAABBNODE_LEAF )
		{
			if( !(_iNode & BSP_OCTAAABBNODE_LEAF) && _pLeaves )
			{
				const TPortalLeaf &PL = _pLeaves[(Nd.m_iChildren & 0x7FFFFFF)+i];
				if( !BSPOctaAABB_ValidateVolume_r(_pNodes,PL.m_iOctaAABBNode | BSP_OCTAAABBNODE_LEAF,_pLeaves,&NewBox) ) return false;
			}
		}
		else
		{
			if( !BSPOctaAABB_ValidateVolume_r(_pNodes,(Nd.m_iChildren+i) | (_iNode & BSP_OCTAAABBNODE_LEAF),_pLeaves,&NewBox) ) return false;
		}
	}

	return true;
}

template<typename TPortalLeaf>
void BSPOctaAABB_DrawTree_r(const CBSP_OctaAABBNode * _pNodes, uint32 _iNode, const TPortalLeaf * _pLeaves, 
							CXR_VBManager* _pVBM,const CMat4Dfp32 &_Transform,
							const CMat4Dfp32 &_FromU16, const CPixel32 * _pColors, uint8 _nColors, bool _bNodes, 
							bool _bLeaves = true, const CPixel32 *_pLeafColor = NULL,
							uint32 _iGeneration = 0, bool _bFaces = false)
{
	const CBSP_OctaAABBNode &Nd = _pNodes[_iNode];
	CPixel32 Color;
	if( (Nd.m_iChildren & BSP_OCTAAABBNODE_LEAF) && _bFaces )
	{
		if( !_bLeaves ) return;
		Color = *_pLeafColor;
	}
	else if( Nd.m_iChildren & BSP_OCTAAABBNODE_LEAF )
	{
		//Recurse
		for(int i = 0;(i < 8) && (Nd.m_Max[i] > Nd.m_Min[i]);i++)
		{
			BSPOctaAABB_DrawTree_r(_pNodes,_pLeaves[Nd.m_iChildren + i].m_iOctaAABBNode,_pLeaves,_pVBM,_Transform,_FromU16,_pColors,
				_nColors,_bNodes,_bLeaves,_pLeafColor,_iGeneration+1,true);
		}

		if( !_bNodes ) return;

		Color = _pColors[Min<uint32>(_nColors-1,_iGeneration)];
	}
	else
	{
		//Recurse
		for(int i = 0;(i < 8) && (Nd.m_Max[i] > Nd.m_Min[i]);i++)
		{
			BSPOctaAABB_DrawTree_r(_pNodes,Nd.m_iChildren + i,_pLeaves,_pVBM,_Transform,_FromU16,_pColors,
				_nColors,_bNodes,_bLeaves,_pLeafColor,_iGeneration+1,_bFaces);
		}

		if( !_bNodes ) return;

		Color = _pColors[Min<uint32>(_nColors-1,_iGeneration)];
	}

	//Draw children. *NOT* pretty, but it gets the job done.
	for(int i = 0;(i < 8) && (Nd.m_Max[i] > Nd.m_Min[i]);i++)
	{
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

		CVec3Dfp32 Vtx[8];
		DBox.GetVertices(Vtx);

		CVec3Dfp32 lPts[24];
		lPts[0] = Vtx[0];
		lPts[1] = Vtx[1];
		lPts[2] = Vtx[0];
		lPts[3] = Vtx[2];
		lPts[4] = Vtx[1];
		lPts[5] = Vtx[3];
		lPts[6] = Vtx[2];
		lPts[7] = Vtx[3];

		lPts[8] = Vtx[4];
		lPts[9] = Vtx[5];
		lPts[10] = Vtx[4];
		lPts[11] = Vtx[6];
		lPts[12] = Vtx[5];
		lPts[13] = Vtx[7];
		lPts[14] = Vtx[6];
		lPts[15] = Vtx[7];

		lPts[16] = Vtx[0];
		lPts[17] = Vtx[4];
		lPts[18] = Vtx[2];
		lPts[19] = Vtx[6];
		lPts[20] = Vtx[1];
		lPts[21] = Vtx[5];
		lPts[22] = Vtx[3];
		lPts[23] = Vtx[7];

		_pVBM->RenderWires(_Transform, lPts, g_IndexRamp32, 24, Color, false);
	}
}


template<typename TPortalLeaf>
static uint32 BSPOctaAABB_EnumFaces_Test(CBSP_OctaAABBNode * _pNodes, const TPortalLeaf * _pLeaves,
										 uint32 _nNodes,const CBox3Dfp32 &_BB,const CBox3Dfp32 &_Bx,
										 uint32 &_nResNodes, uint32 _Mask = 0)
{
	vec128 BoxMin[3],BoxMax[3];

	//Prepare min-max
	{
		//Modifier to uint16-space
		// This needs to be created manually
		CMat4Dfp32 ToU16;
		ToU16.Unit();
		ToU16.k[0][0] = 65535.0f / (_BB.m_Max[0] - _BB.m_Min[0]);
		ToU16.k[1][1] = 65535.0f / (_BB.m_Max[1] - _BB.m_Min[1]);
		ToU16.k[2][2] = 65535.0f / (_BB.m_Max[2] - _BB.m_Min[2]);
		ToU16.k[3][0] = -_BB.m_Min[0] * ToU16.k[0][0];
		ToU16.k[3][1] = -_BB.m_Min[1] * ToU16.k[1][1];
		ToU16.k[3][2] = -_BB.m_Min[2] * ToU16.k[2][2];

		uint16 lBox[6];
		CVec3Dfp32 Mn,Mx;

		_Bx.m_Min.MultiplyMatrix(ToU16,Mn);
		_Bx.m_Max.MultiplyMatrix(ToU16,Mx);

		lBox[0] = (uint16)(Max(Mn.k[0],0.0f));
		lBox[1] = (uint16)(Max(Mn.k[1],0.0f));
		lBox[2] = (uint16)(Max(Mn.k[2],0.0f));
		lBox[3] = (uint16)(Min(Mx.k[0]+1.0f,65535.0f));
		lBox[4] = (uint16)(Min(Mx.k[1]+1.0f,65535.0f));
		lBox[5] = (uint16)(Min(Mx.k[2]+1.0f,65535.0f));

		for(int i = 0;i < 3;i++)
		{
			BoxMin[i] = M_VLdScalar_u16(lBox[i]);
			BoxMax[i] = M_VLdScalar_u16(lBox[i+3]);
		}
	}

	//Stack
	uint32 liNodes[256];
	liNodes[0] = _Mask;
	uint16 iStackPos = 1;

	//Return value
	uint32 nCount = 0;
	uint32 nFinal = 0;

	//Iterate
	while( iStackPos )
	{
		const CBSP_OctaAABBNode &Nd = _pNodes[liNodes[iStackPos-1] & 0x7FFFFFFF];
		uint32 NodeMask = liNodes[iStackPos-1] & 0x80000000;
		iStackPos--;

		//Check
		vec128 MinTest[3],MaxTest[3];
		CVec128Access Res;
		MinTest[0] = M_VCmpLTMsk_u16(BoxMin[0],Nd.m_MaxV[0]);
		MinTest[1] = M_VCmpLTMsk_u16(BoxMin[1],Nd.m_MaxV[1]);
		MinTest[2] = M_VCmpLTMsk_u16(BoxMin[2],Nd.m_MaxV[2]);
		MaxTest[0] = M_VCmpGTMsk_u16(BoxMax[0],Nd.m_MinV[0]);
		MaxTest[1] = M_VCmpGTMsk_u16(BoxMax[1],Nd.m_MinV[1]);
		MaxTest[2] = M_VCmpGTMsk_u16(BoxMax[2],Nd.m_MinV[2]);

		Res.v = M_VAnd(M_VAnd(M_VAnd(MinTest[0],MinTest[1]),M_VAnd(MinTest[2],MaxTest[0])),M_VAnd(MaxTest[1],MaxTest[2]));

		if( Nd.m_iChildren & BSP_OCTAAABBNODE_LEAF )
		{
			if( NodeMask )
			{
				//Loop over faces
				for(int i = 0;i < 8;i++)
				{
					if( !Res.ku16[i] ) continue;
					nFinal++;
				}
			}
			else
			{
				//We need to address the portal leaves
				for(int i = 0;i < 8;i++)
				{
					if( !Res.ku16[i] ) continue;

					uint32 iLeaf = (Nd.m_iChildren & ~BSP_OCTAAABBNODE_LEAF) + i;
					liNodes[iStackPos] = _pLeaves[iLeaf].m_iOctaAABBNode | 0x80000000;
					iStackPos++;
					nCount++;

#ifndef M_RTM
					if( iStackPos >= 256 )
						Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");

					if( (liNodes[iStackPos-1] & 0x7FFFFFFF) >= _nNodes )
						Error_static("BSPOctaAABB_EnumFaces_Box","Node overflow- should've been culled");
#endif
				}
			}
		}
		else
		{
			//Loop over children
			for(int i = 0;i < 8;i++)
			{
				if( Res.ku16[i] )
				{
					liNodes[iStackPos] = (Nd.m_iChildren + i) | NodeMask;
					iStackPos++;
					nCount++;

#ifndef M_RTM
					if( iStackPos >= 256 )
						Error_static("BSPOctaAABB_EnumFaces_Box","Stack overflow!");

					if( (liNodes[iStackPos-1] & 0x7FFFFFFF) >= _nNodes )
						Error_static("BSPOctaAABB_EnumFaces_Box","Node overflow- should've been culled");
#endif
				}
			}
		}
	}

	_nResNodes = nCount;
	return nFinal;
}

#endif


#endif // Inclusion guard
