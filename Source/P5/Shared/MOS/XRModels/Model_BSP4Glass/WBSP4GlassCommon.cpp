#include "PCH.h"
#include "MFloat.h"
#include "../../Classes/Render/MWireContainer.h"

#include "WBSP4GlassCommon.h"


#if GLASS_OPTIMIZE_OFF
#pragma xrMsg("optimize off!")
#pragma optimize("", off)
#pragma inline_depth(0)
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Index
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_Index::CBSP4Glass_Index()
	: m_iInstance(0)
	, m_nInstance(0)
	, m_NameHash(0)
	, m_BoundingBox(0,0)
	, m_Center(0)
	, m_Radius(0)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Index::CBSP4Glass_Index, XR_BSP4GLASS);
}


CBSP4Glass_Index::~CBSP4Glass_Index()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Index::~CBSP4Glass_Index, XR_BSP4GLASS);
}


void CBSP4Glass_Index::Write(CCFile* _pFile)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Index::Write, XR_BSP4GLASS);

	_pFile->WriteLE(m_iInstance);
	_pFile->WriteLE(m_nInstance);
	m_Center.Write(_pFile);
	_pFile->WriteLE(m_Radius);
	m_BoundingBox.Write(_pFile);
}


void CBSP4Glass_Index::Read(CCFile* _pFile)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Index::Read, XR_BSP4GLASS);

	_pFile->ReadLE(m_iInstance);
	_pFile->ReadLE(m_nInstance);
	m_Center.Read(_pFile);
	_pFile->ReadLE(m_Radius);
	m_BoundingBox.Read(_pFile);
}


CBSP4Glass_Index& CBSP4Glass_Index::operator = (const CBSP4Glass_IndexLoad& _Index)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Index::operator=, XR_BSP4GLASS);
	m_iInstance = _Index.m_iInstance;
	m_nInstance = _Index.m_nInstance;
	m_NameHash = StringToHash(_Index.m_Name);
	m_BoundingBox = _Index.m_BoundingBox;
	m_Center = _Index.m_Center;
	m_Radius = _Index.m_Radius;

	return *this;
}


CBSP4Glass_IndexLoad::CBSP4Glass_IndexLoad()
	: CBSP4Glass_Index()
	, m_Name("")
{
	GLASS_MSCOPE_ALL(CBSP4Glass_IndexLoad::CBSP4Glass_IndexLoad, XR_BSP4GLASS);
}


CBSP4Glass_IndexLoad::~CBSP4Glass_IndexLoad()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_IndexLoad::~CBSP4Glass_IndexLoad, XR_BSP4GLASS);
}


void CBSP4Glass_IndexLoad::Write(CCFile* _pFile)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_IndexLoad::Write, XR_BSP4GLASS);

	_pFile->WriteLE(m_iInstance);
	_pFile->WriteLE(m_nInstance);
	m_Name.Write(_pFile);
	m_Center.Write(_pFile);
	_pFile->WriteLE(m_Radius);
	m_BoundingBox.Write(_pFile);
}


void CBSP4Glass_IndexLoad::Read(CCFile* _pFile)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_IndexLoad::Read, XR_BSP4GLASS);

	_pFile->ReadLE(m_iInstance);
	_pFile->ReadLE(m_nInstance);
	m_Name.Read(_pFile);
	m_Center.Read(_pFile);
	_pFile->ReadLE(m_Radius);
	m_BoundingBox.Read(_pFile);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CGlassAttrib
|__________________________________________________________________________________________________
\*************************************************************************************************/
CGlassAttrib::CGlassAttrib()
	: m_Flags(0)
{
	GLASS_MSCOPE_ALL(CGlassAttrib::CGlassAttrib, XR_BSP4GLASS);
}


CGlassAttrib::~CGlassAttrib()
{
	GLASS_MSCOPE_ALL(CGlassAttrib::~CGlassAttrib, XR_BSP4GLASS);
}


void CGlassAttrib::OnDeltaLoad(CCFile* _pFile)
{
	// Load attributes
	_pFile->ReadLE(m_Flags);
	_pFile->ReadLE(m_Durability);
}


uint32 CGlassAttrib::OnDeltaSave(CCFile* _pFile)
{
	// Save attributes
	_pFile->WriteLE(m_Flags);
	_pFile->WriteLE(m_Durability);

	uint32 TotalSize = sizeof(m_Flags) + sizeof(m_Durability);
	M_TRACEALWAYS(CStrF("   - GlassAttributes (%d) bytes\n", TotalSize));
	return TotalSize;
}


void CGlassAttrib::Attrib_SetPhys(const bool _bHasPhys)
{
	GLASS_MSCOPE_ALL(CGlassAttrib::Attrib_SetPhys, XR_BSP4GLASS);
	if(_bHasPhys) m_Flags &= ~ATTRIB_FLAGS_NOPHYS;
	else m_Flags |= ATTRIB_FLAGS_NOPHYS;
}


void CGlassAttrib::Attrib_SetBaseRender(const bool _bBaseRender)
{
	GLASS_MSCOPE_ALL(CGlassAttrib::Attrib_SetPhys, XR_BSP4GLASS);
	if(_bBaseRender) m_Flags &= ~ATTRIB_FLAGS_NOBASERENDER;
	else m_Flags |= ATTRIB_FLAGS_NOBASERENDER;
}


void CGlassAttrib::Attrib_SetInactive(const bool _bInactive)
{
	GLASS_MSCOPE_ALL(CGlassAttrib::Attrib_SetInactive, XR_BSP4GLASS);
	if(_bInactive) m_Flags |= ATTRIB_FLAGS_INACTIVE;
	else m_Flags &= ~ATTRIB_FLAGS_INACTIVE;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_TangentSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_TangentSetup::CBSP4Glass_TangentSetup(const CBSP4Glass_MappingSetup& _MappingSetup, const CVec3Dfp32& _PlaneN)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_TangentSetup::CBSP4Glass_TangentSetup, XR_BSP4GLASS);

	const CVec3Dfp32& UVec = _MappingSetup.GetUVec();
	const CVec3Dfp32& VVec = _MappingSetup.GetVVec();
	const fp32 ULengthRecp = _MappingSetup.GetUVecLengthRcp();
	const fp32 VLengthRecp = _MappingSetup.GetVVecLengthRcp();

	// Tangent setups
	UVec.Scale(ULengthRecp, m_TangU[0]);
	VVec.Scale(VLengthRecp, m_TangV[0]);
	m_TangU[1] = m_TangU[0];
	m_TangV[1] = m_TangV[0];

	m_TangU[0].Combine(_PlaneN, -(_PlaneN * m_TangU[0]), m_TangU[0]);
	m_TangV[0].Combine(_PlaneN, -(_PlaneN * m_TangV[0]), m_TangV[0]);
	m_TangU[1].Combine(-(_PlaneN), _PlaneN * m_TangU[1], m_TangU[1]);
	m_TangV[1].Combine(-(_PlaneN), _PlaneN * m_TangV[1], m_TangV[1]);

	// Normalize
	m_TangU[0].Normalize();
	m_TangV[0].Normalize();
	m_TangU[1].Normalize();
	m_TangV[1].Normalize();

	m_NTangU[0] = -( _PlaneN * m_TangU[0]);
	m_NTangV[0] = -( _PlaneN * m_TangV[0]);
	m_NTangU[1] = -(-_PlaneN * m_TangU[1]);
	m_NTangV[1] = -(-_PlaneN * m_TangV[1]);

	m_TangU[0].Combine( _PlaneN, m_NTangU[0], m_TangU[0]);
	m_TangV[0].Combine( _PlaneN, m_NTangV[0], m_TangV[0]);
	m_TangU[1].Combine(-_PlaneN, m_NTangU[1], m_TangU[1]);
	m_TangV[1].Combine(-_PlaneN, m_NTangV[1], m_TangV[1]);

	// Normalize
	m_TangU[0].Normalize();
	m_TangV[0].Normalize();
	m_TangU[1].Normalize();
	m_TangV[1].Normalize();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_MappingSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_MappingSetup::CBSP4Glass_MappingSetup(const CXR_PlaneMapping& _Mapping, const CGlassAttrib& _Attrib)
	: m_TProjMin(_FP32_MAX)
	, m_TProjMax(-_FP32_MAX)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_MappingSetup::CBSP4Glass_MappingSetup, XR_BSP4GLASS);

	m_UVec = _Mapping.m_U;
	m_VVec = _Mapping.m_V;
		
	m_UVecLenSqrInv = 1.0f / (m_UVec * m_UVec);
	m_VVecLenSqrInv = 1.0f / (m_VVec * m_VVec);
	m_UVecLenRcp = 1.0f / m_UVec.Length();
	m_VVecLenRcp = 1.0f / m_VVec.Length();
		
	m_UOffset = _Mapping.m_UOffset;
	m_VOffset = _Mapping.m_VOffset;

	m_TxtWidthInv = _Attrib.m_TxtWidthInv;
	m_TxtHeightInv = _Attrib.m_TxtHeightInv;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CGridData (Dynamic sized bit array)
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_Grid::CGridData::CGridData()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::CGridData, XR_BSP4GLASS);
	Clear();
}


CBSP4Glass_Grid::CGridData::~CGridData()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::~CGridData, XR_BSP4GLASS);
	Clear();
}


void CBSP4Glass_Grid::CGridData::operator = (const CBSP4Glass_Grid::CGridData& _GridData)
{
	m_lBits.Clear();
	m_lBits.Add(_GridData.m_lBits);
	m_pBits = m_lBits;
	m_nElem = _GridData.m_nElem;
	m_nBits = _GridData.m_nBits;
}


void CBSP4Glass_Grid::CGridData::OnDeltaLoad(CCFile* _pFile)
{
	uint8 Load = 0;
	_pFile->ReadLE(m_nBits);
	_pFile->ReadLE(m_nElem);
	_pFile->ReadLE(Load);

	// Set array and start reading all bits
	SetLen(m_nElem, m_nBits);

	// Check if we need to load data, or just set it
	if (!Load)
	{
		SetAll(false);
		return;
	}

	uint32 nLeft = m_pBits.Len();
	uint32 nLen = m_pBits.Len();
	while (nLeft > 0)
	{
		uint16 nRead = 0;
		uint8 Val = 0;
		_pFile->ReadLE(nRead);
		_pFile->ReadLE(Val);
		
		FillChar(&m_pBits[nLen - nLeft], nRead, Val);

		nLeft -= nRead;
	}
}


uint32 CBSP4Glass_Grid::CGridData::OnDeltaSave(CCFile* _pFile)
{
	// Examine if we need to store this grid at all
	uint8 Save = 0;
	for (uint i = 0; i < m_pBits.Len(); i++)
	{
		if (m_pBits[i] != 0)
		{
			Save = 1;
			break;
		}
	}

#ifndef M_RTM
	uint WriteEntrySize = sizeof(uint8) + sizeof(uint8);
	uint TotalSize = sizeof(m_nBits) + sizeof(m_nElem);
#endif

	_pFile->WriteLE(m_nBits);
	_pFile->WriteLE(m_nElem);
	_pFile->WriteLE(Save);

	// Check if we need to store grid data
	if (!Save)
	{
#ifndef M_RTM
		M_TRACEALWAYS(CStrF("   - GlassGrid Data (%d) byte\n", TotalSize + sizeof(uint8)));
		return TotalSize + sizeof(uint8);
#else
		return 0;
#endif
	}

	uint32 iPos = 0;
	uint32 nLeft = m_pBits.Len();
	uint32 nLen = m_pBits.Len();
	while (nLeft > 0)
	{
		uint8 Val = m_pBits[iPos];
		uint8 i = iPos + 1;
		for (i; i < nLen; i++)
		{
			if (m_pBits[i] != m_pBits[iPos])
				break;
		}

		_pFile->WriteLE(uint8(i - iPos));
		_pFile->WriteLE(Val);

		nLeft -= (i - iPos);
		iPos = i;

#ifndef M_RTM
		TotalSize += WriteEntrySize;
#endif
	}

#ifndef M_RTM
	M_TRACEALWAYS(CStrF("   - GlassGrid Data (%d) bytes - %.2f kB - GridSize(%d bytes / %.2f kB)\n", TotalSize, (fp32(TotalSize) / 1024.0f), sizeof(uint8)*nLen, fp32(sizeof(uint8)*nLen)/1024.0f));
#endif

#ifndef M_RTM
	return TotalSize;
#else
	return 0;
#endif
}


void CBSP4Glass_Grid::CGridData::SetAll(const bool _bSet)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::SetAll, XR_BSP4GLASS);
	if(_bSet)
	{
		FillChar(m_lBits.GetBasePtr(), m_lBits.Len(), 0xFF);
	}
	else
	{
		FillChar(m_lBits.GetBasePtr(), m_lBits.Len(), 0x00);
	}
}


void CBSP4Glass_Grid::CGridData::SetAll(const uint8 _iBit, const bool _bSet)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::SetAll, XR_BSP4GLASS);
	SetAllRange(_bSet, m_nElem * _iBit, m_nElem);
}


void CBSP4Glass_Grid::CGridData::SetAllRange(const bool _bSet, const uint32 _Start, const uint32 _Count)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::SetAllRange, XR_BSP4GLASS);
	M_ASSERT(m_lBits.Len() - (_Start + _Count) < 0, "CBSP4Glass_Grid::CGridData::SetAllRange: Out of range!");

	if(_bSet)
	{
		FillChar(m_lBits.GetBasePtr() + _Start, _Count, 0xFF);
	}
	else
	{
		FillChar(m_lBits.GetBasePtr() + _Start, _Count, 0x00);
	}
}


void CBSP4Glass_Grid::CGridData::SetLen(const uint32 _nElem, const uint8 _nBits)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::SetLen, XR_BSP4GLASS);
	m_nElem = (_nElem >> 3) + ((_nElem % 8) ? 1 : 0);
	m_nBits = _nBits;
	m_lBits.SetLen(m_nElem * _nBits);
	m_pBits = m_lBits;
}


void CBSP4Glass_Grid::CGridData::Clear()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Clear, XR_BSP4GLASS);
	m_nElem = 0;
	m_lBits.Clear();
	m_pBits = m_lBits;
	m_nBits = 0;
}


void CBSP4Glass_Grid::CGridData::Set0(const uint32 _iElem, const uint8 _iBit)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Set0, XR_BSP4GLASS);
	const uint32 iArray = (_iElem >> 3);
	const uint32 iArrayPos = iArray + (_iBit * m_nElem);
	const uint32 iArrayBit = _iElem - (iArray << 3);

	M_ASSERT(m_lBits.Len() >= iArrayPos, "CBSP4Glass_Grid::CGridData::Set0: Out of range!");
	m_pBits[iArrayPos] &= ~M_BitD(iArrayBit);
}


bool CBSP4Glass_Grid::CGridData::Set0(const uint32 _iElem)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Set0, XR_BSP4GLASS);
	
	const uint32 iArray = (_iElem >> 3);
	const uint32 iArrayBit = _iElem - (iArray << 3);

	const uint32 iArrayLastPos = iArray + ((m_nBits-1) * m_nElem);
	M_ASSERT(m_lBits.Len() >= iArrayLastPos, "CBSP4Glass_Grid::CGridData::Set0: Out of range!");
	if(m_pBits[iArrayLastPos] & M_BitD(iArrayBit))
	{
		for(uint8 i = 0; i < m_nBits; i++)
		{
			const uint32 iArrayPos = iArray + (i * m_nElem);
			M_ASSERT(m_lBits.Len() >= iArrayPos, "CBSP4Glass_Grid::CGridData::Set0: Out of range!");
			if(m_pBits[iArrayPos] & M_BitD(iArrayBit))
			{
				m_pBits[iArrayPos] &= ~M_BitD(iArrayBit);

				// Setting last bit
				if(i+1 == m_nBits)
					return true;

				break;
			}
		}
	}

	return false;
}


void CBSP4Glass_Grid::CGridData::Set0All(const uint32 _iElem)
{
	const uint32 iArray = (_iElem >> 3);
	const uint32 iArrayBit = _iElem - (iArray << 3);
	
	for(uint8 i = 0; i < m_nBits; i++)
	{
		const uint32 iArrayPos = iArray + (i * m_nElem);

		M_ASSERT(m_lBits.Len() >= iArrayPos, "CBSP4Glass_Grid::CGridData::Set0All: Out of range!");
		m_pBits[iArrayPos] &= ~M_BitD(iArrayBit);
	}
}


void CBSP4Glass_Grid::CGridData::Set1(const uint32 _iElem, const uint8 _iBit)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Set1, XR_BSP4GLASS);
	const uint32 iArray = (_iElem >> 3);
	const uint32 iArrayPos = iArray + (_iBit * m_nElem);
	const uint32 iArrayBit = _iElem - (iArray << 3);

	M_ASSERT(m_lBits.Len() >= iArrayPos, "CBSP4Glass_Grid::CGridData::Set1: Out of range!");
	m_pBits[iArrayPos] |= M_BitD(iArrayBit);
}


bool CBSP4Glass_Grid::CGridData::Set1(const uint32 _iElem)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Set1, XR_BSP4GLASS);

	const uint32 iArray = (_iElem >> 3);
	const uint32 iArrayBit = _iElem - (iArray << 3);

	if(m_pBits[iArray + ((m_nBits - 1) * m_nElem)] & M_BitD(iArrayBit))
	{
		for(uint8 i = 0; i < m_nBits; i++)
		{
			const uint32 iArrayPos = iArray + (i * m_nElem);
			M_ASSERT(m_lBits.Len() >= iArrayPos, "CBSP4Glass_Grid::CGridData::Set0: Out of range!");
			if(!(m_pBits[iArrayPos] & M_BitD(iArrayBit)))
			{
				m_pBits[iArrayPos] |= M_BitD(iArrayBit);

				// Last bit
				if(i+1 == m_nBits)
					return true;

				break;
			}
		}
	}

	return false;
}


void CBSP4Glass_Grid::CGridData::Set1All(const uint32 _iElem)
{
	const uint32 iArray = (_iElem >> 3);
	const uint32 iArrayBit = _iElem - (iArray << 3);

	for(uint8 i = 0; i < m_nBits; i++)
	{
		const uint32 iArrayPos = iArray + (i * m_nElem);

		M_ASSERT(m_lBits.Len() >= iArrayPos, "CBSP4Glass_Grid::CGridData::Set1All: Out of range!");
		m_pBits[iArrayPos] |= M_BitD(iArrayBit);
	}
}


void CBSP4Glass_Grid::CGridData::Set(const uint32 _iElem, const uint8 _iBit, const bool _bSet)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Set, XR_BSP4GLASS);
	if(_bSet)
		Set1(_iElem, _iBit);
	else
		Set0(_iElem, _iBit);
}


bool CBSP4Glass_Grid::CGridData::Set(const uint32 _iElem, const bool _bSet)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Set, XR_BSP4GLASS);
	if(_bSet)
		return Set1(_iElem);
	else
		return Set0(_iElem);
}


bool CBSP4Glass_Grid::CGridData::Get(const uint32 _iElem, const uint8 _iBit) const
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Get, XR_BSP4GLASS);

	const uint32 iArray = (_iElem >> 3);
	const uint32 iArrayPos = iArray + (_iBit * m_nElem);
	const uint32 iArrayBit = _iElem - (iArray << 3);

	M_ASSERT(m_lBits.Len() >= iArrayPos, "CBSP4Glass_Grid::CGridData::Get: Out of range!");
	return ((m_pBits[iArrayPos] & M_BitD(iArrayBit)) != 0);
}


uint CBSP4Glass_Grid::CGridData::Get01(const uint32 _iElem, const uint8 _iBit) const
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::Get01, XR_BSP4GLASS);
	return (Get(_iElem, _iBit) ? 1 : 0);
}


uint8* CBSP4Glass_Grid::CGridData::GetArray()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CGridData::GetArray, XR_BSP4GLASS);
	return m_lBits.GetBasePtr();
}


uint8 CBSP4Glass_Grid::CGridData::GetLastBit()
{
	return (m_nBits - 1);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Wallmark
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_Wallmark& CBSP4Glass_Wallmark::operator = (const CXR_WallmarkDesc& _WM)
{
	CMTime m_SpawnTime;
	uint32 m_GUID;

	m_SurfaceID = _WM.m_SurfaceID;
	m_Flags = _WM.m_Flags;
	m_iNode = _WM.m_iNode;
	m_Size = _WM.m_Size;
	m_SpawnTime = _WM.m_SpawnTime;
	m_GUID = _WM.m_GUID;

	for(int i = 0; i < XR_WALLMARK_MAXTEXTUREPARAMS; i++)
		m_SurfaceParam_TextureID[i] = _WM.m_SurfaceParam_TextureID[i];

	return (*this);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_WallmarkContext
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_WallmarkContext::CBSP4Glass_WallmarkContext()
{
	m_nV = 0;
	m_pSC = NULL;
	m_spQWM = NULL;
	m_spLink = NULL;
}


CBSP4Glass_WallmarkContext::~CBSP4Glass_WallmarkContext()
{
	m_pSC = NULL;
	m_spQWM = NULL;
	m_spLink = NULL;
}


void CBSP4Glass_WallmarkContext::Create(CXR_Model_BSP2* _pModel, int _nV)
{
	MAUTOSTRIP(CBSP4Glass_WallmarkContext::Create, MAUTOSTRIP_VOID);
	GLASS_MSCOPE(CBSP4Glass_WallmarkContext::Create, XR_BSP4GLASS);

	m_lV.SetLen(_nV);
	m_lN.SetLen(_nV);
	m_lTV.SetLen(_nV);
	m_lTV2.SetLen(_nV);
	m_lCol.SetLen(_nV);
	m_lTangU.SetLen(_nV);
	m_lTangV.SetLen(_nV);
	m_nV = _nV;
	m_iVTail = 0;
	m_iVHead = 0;
	
	m_spQWM = MNew1(CBSP4Glass_QWM, _nV >> 2);
	if(!m_spQWM)
		MemError("CBSP4Glass_WallmarkContext::Create(m_spQWM)");

	m_spLink = MNew(CBSP4Glass_LinkContext);
	if (!m_spLink)
		MemError("CBSP4Glass_WallmarkContext::Create(m_spLink)");
	m_spLink->Create(_pModel, _nV >> 2, _nV / 3, _pModel->m_nStructureLeaves, false);

	// Fetch surface context
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if(!pSC)
		Error("CBSP4Glass_WallmarkContext::Create", "No surface-context available!");
	
	m_pSC = pSC;
}


void CBSP4Glass_WallmarkContext::Clear()
{
	MAUTOSTRIP(CBSP4Glass_WallmarkContext::Clear, MAUTOSTRIP_VOID);
	m_iVHead = 0;
	m_iVTail = 0;
	if(!m_spQWM)
		return;

	while(!m_spQWM->Empty())
	{
		m_spLink->Remove(m_spQWM->GetTailIndex());
		m_spQWM->Get();
	}
}


void CBSP4Glass_WallmarkContext::FreeWallmark()
{
	MAUTOSTRIP(CBSP4Glass_WallmarkContext::FreeWallmark, MAUTOSTRIP_VOID);
	
	if(!m_spQWM)
		return;

	int iTail = m_spQWM->GetTailIndex();
	CBSP4Glass_Wallmark WM = m_spQWM->Get();
	CBSP4Glass_Wallmark* pWM = m_spQWM->GetTail();
	m_spLink->Remove(iTail);

	if(!pWM)
	{
		m_iVTail = 0;
		m_iVHead = 0;
		return;
	}

	int iVLast = pWM->m_iV;
	while(m_iVTail != iVLast)
	{
		m_iVTail++;
		if(m_iVTail >= m_nV)
			m_iVTail = 0;
	}
}


int CBSP4Glass_WallmarkContext::AddWallmark(const CBSP4Glass_Wallmark& _WM)
{
	MAUTOSTRIP(CBSP4Glass_WallmarkContext::AddWallmark, 0);
	if(!m_spQWM)
		return 0;

	if(!m_spQWM->MaxPut())
		FreeWallmark();

	int iHead = m_spQWM->GetHeadIndex();
	m_spQWM->Put(_WM);
	return iHead;
}


int CBSP4Glass_WallmarkContext::MaxPut() const
{
	MAUTOSTRIP(CBSP4Glass_WallmarkContext::MaxPut, 0);
	if (m_iVHead == m_iVTail)
		return m_nV-1;
	else
		return Max(0, ((2*m_nV - (m_iVHead - m_iVTail)) % m_nV) -1);
}


int CBSP4Glass_WallmarkContext::AddVertices(int _nV)
{
	MAUTOSTRIP(CBSP4Glass_WallmarkContext::AddVertices, 0);
	
	if ((m_iVHead + _nV > m_nV))
	{
		while(m_iVTail > m_iVHead)
			FreeWallmark();

		if (m_iVTail == 0)
			FreeWallmark();

		m_iVHead = 0;
	}

	while(MaxPut() < _nV+1)
		FreeWallmark();

	int iv = m_iVHead;
	m_iVHead += _nV;
	if (m_iVHead >= m_nV)
		m_iVHead = 0;

	if (iv + _nV > m_nV)
		Error("CBSP4Glass_WallmarkContext::AddVertices", CStrF("Internal error _nV %d, iV %d, nV %d, H %d, T %d", _nV, iv, m_nV, m_iVHead, m_iVTail));

	return iv;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_Grid::CBSP4Glass_Grid()
	: m_bCreated(0)
	, m_iSound(0)
	, m_Seed(0)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CBSP4Glass_Grid, XR_BSP4GLASS);
}


void CBSP4Glass_Grid::OnDeltaLoad(CCFile* _pFile)
{
	uint8 LoadData = 0;

	_pFile->ReadLE(LoadData);
	_pFile->ReadLE(m_Seed);

	// Set from load data
	m_bCreated = (LoadData >> 0) & 0x1;
	m_bFullBreak = (LoadData >> 1) & 0x1;
	m_bTimeBreak = (LoadData >> 2) & 0x1;
	m_iSound = (LoadData >> 3) & 0x3;

	// Load grid data if needed
	if (!m_bFullBreak && m_bCreated)
		m_GridData.OnDeltaLoad(_pFile);
}


uint32 CBSP4Glass_Grid::OnDeltaSave(CCFile* _pFile)
{
	// Build save data 
	uint8 SaveData = 0;
	SaveData |= (m_bCreated << 0);
	SaveData |= (m_bFullBreak << 1);
	SaveData |= (m_bTimeBreak << 2);
	SaveData |= (m_iSound << 3);

	_pFile->WriteLE(SaveData);
	_pFile->WriteLE(m_Seed);

	uint32 TotalSize = sizeof(SaveData) + sizeof(m_Seed);
	M_TRACEALWAYS(CStrF("   - GlassGrid (%d) bytes\n", TotalSize));

	// Save grid data if needed
	if (!m_bFullBreak && m_bCreated)
		TotalSize += m_GridData.OnDeltaSave(_pFile);

	return TotalSize;
}


uint32 CBSP4Glass_Grid::GetQuadsX()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::GetQuadsX, XR_BSP4GLASS);
	return m_nQuadsX;
}


uint32 CBSP4Glass_Grid::GetQuadsY()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::GetQuadsY, XR_BSP4GLASS);
	return m_nQuadsY;
}


uint32 CBSP4Glass_Grid::GetSeed()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::GetSeed, XR_BSP4GLASS);
	return m_Seed;
}


//CVec3Dfp32 CBSP4Glass_Grid::GetWPlaneN()
//{
//	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::GetWPlaneN, XR_BSP4GLASS);
//	return m_WPlaneN;
//}


CVec3Dfp32 CBSP4Glass_Grid::GetPlaneN()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::GetPlaneN, XR_BSP4GLASS);
	return m_PlaneN;
}


CVec3Dfp32 CBSP4Glass_Grid::GetFrontTangU()
{
	return m_FrontTangU;
}


CVec3Dfp32 CBSP4Glass_Grid::GetFrontTangV()
{
	return m_FrontTangV;
}


CVec3Dfp32 CBSP4Glass_Grid::GetBackTangU()
{
	return m_BackTangU;
}


CVec3Dfp32 CBSP4Glass_Grid::GetBackTangV()
{
	return m_BackTangV;
}


/*
CMat4Dfp32 CBSP4Glass_Grid::GetPositionMatrix()
{
	return m_WMat;
}


void CBSP4Glass_Grid::GetInverseOrthogonalWMat(CMat4Dfp32& _WMatInv)
{
	m_WMat.InverseOrthogonal(_WMatInv);
}
*/


void CBSP4Glass_Grid::SetFrontTang(const CVec3Dfp32& _TangU, const CVec3Dfp32& _TangV)
{
	m_FrontTangU = _TangU;
	m_FrontTangV = _TangV;
}


void CBSP4Glass_Grid::SetBackTang(const CVec3Dfp32& _TangU, const CVec3Dfp32& _TangV)
{
	m_BackTangU = _TangU;
	m_BackTangV = _TangV;
}


void CBSP4Glass_Grid::CleanGrid(const bool _bKeepCreated)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CleanGrid, XR_BSP4GLASS);
	m_lGridPoints.Clear();
	m_GridData.Clear();
	m_nQuadsX = 0;
	m_nQuadsY = 0;
	
	// Clean EVERYTHING!
	if(!_bKeepCreated)
	{
		m_bCreated = 0;
		m_Seed = 0;
	}
}


void CBSP4Glass_Grid::operator = (const CBSP4Glass_Grid& _Grid)
{
	m_lGridPoints.Clear();

	m_lGridPoints.Add(_Grid.m_lGridPoints);
	m_bCreated = _Grid.m_bCreated;
	m_bFullBreak = _Grid.m_bFullBreak;
	m_bTimeBreak = _Grid.m_bTimeBreak;
	m_Seed = _Grid.m_Seed;
	m_nQuadsX = _Grid.m_nQuadsX;
	m_nQuadsY = _Grid.m_nQuadsY;
	m_FrontTangU = _Grid.m_FrontTangU;
	m_FrontTangV = _Grid.m_FrontTangV;
	m_BackTangU = _Grid.m_BackTangU;
	m_BackTangV = _Grid.m_BackTangV;
//	m_WMat = _Grid.m_WMat;
	m_PlaneN = _Grid.m_PlaneN;
	m_PlaneD = _Grid.m_PlaneD;
	m_GridData = _Grid.m_GridData;
}


void CBSP4Glass_Grid::CreateGridMap(const CGlassAttrib& _Attrib, const CVec3Dfp32& _Origin, CVec3Dfp32 _Vec0, CVec3Dfp32 _Vec1, const CMat4Dfp32& _WMat, uint8 _CrushFlags)
{
	GLASS_MSCOPE(CBSP4Glass_Grid::CreateGridMap, XR_BSP4GLASS);

//	m_WMat.Unit();
	m_PlaneN = _Attrib.m_Plane.n;
	m_PlaneD = _Attrib.m_Plane.d;
	m_bFullBreak = (_Attrib.Attrib_FullBreak() || (_CrushFlags & GLASS_CRUSHFLAGS_FULLBREAK)) ? 1 : 0;
	m_bTimeBreak = (!m_bFullBreak && _Attrib.Attrib_TimeBreaker()) ? 1 : 0;
	
	// Quad size (~6cm)
	//const fp32 QuadDim = 0.32f * 6.0f;

	// Calculate number of points needed
	const uint32 nPointsX = 9;
	const uint32 nPointsY = 9;
	const uint32 nPoints = nPointsX * nPointsY;

	// Set number of quads in grid
	m_nQuadsX = nPointsX - 1;
	m_nQuadsY = nPointsY - 1;
	const uint32 nQuads = m_nQuadsX * m_nQuadsY;

	// Calculate 0->1 mapping sizes
	const fp32 SizeX = 1.0f / m_nQuadsX;
	const fp32 SizeY = 1.0f / m_nQuadsY;

	// Allocate space for grid points
	m_lGridPoints.SetLen(nPoints);
	TAP_RCD<CVec3Dfp32> lGridPoints = m_lGridPoints;

	// Setup mapping from 0->1 space (corners
	lGridPoints[0]						= _Origin;
	lGridPoints[m_nQuadsX]				= (_Origin + _Vec0);
	lGridPoints[m_nQuadsY * nPointsX]	= (_Origin + _Vec1);
	lGridPoints[nPoints-1]				= (_Origin + _Vec0 + _Vec1);

	// Calculate world space normal
	{
		//CVec3Dfp32 Temp0 = pGridPoints[m_nQuadsX] - pGridPoints[0];
		//CVec3Dfp32 Temp1 = pGridPoints[m_nQuadsY * nPointsX] - pGridPoints[0];
		//Temp0.Normalize();
		//Temp1.Normalize();
		//m_WPlaneN = Temp0 / Temp1;
	}

	// Setup mapping from 0->1 space (top/bottom points)
	uint32 Rand = m_Seed;
	for(uint32 x0 = 1, x1 = (m_nQuadsY * nPointsX) + 1; x0 < m_nQuadsX; x0++, x1++)
	{
		CVec3Dfp32& GridPoint0 = lGridPoints[x0];
		CVec3Dfp32& GridPoint1 = lGridPoints[x1];
		GridPoint0 = (_Origin +			(_Vec0 * (SizeX * ((fp32)x0 + (MFloat_GetRand(Rand++) - 0.5f) * 0.8f))));
		GridPoint1 = (_Origin + _Vec1 + (_Vec0 * (SizeX * ((fp32)x0 + (MFloat_GetRand(Rand++) - 0.5f) * 0.8f))));
	}

	// Setup mapping from 0->1 space (left/right points)
	for(uint32 y0 = nPointsX, y1 = nPointsX + m_nQuadsX, y2 = 1; y1 < nPoints-1; y0 += nPointsX, y1 += nPointsX, y2++)
	{
		CVec3Dfp32& GridPoint0 = lGridPoints[y0];
		CVec3Dfp32& GridPoint1 = lGridPoints[y1];
		GridPoint0 = (_Origin +			(_Vec1 * (SizeY * ((fp32)y2 + (MFloat_GetRand(Rand++) - 0.5f) * 0.8f))));
		GridPoint1 = (_Origin + _Vec0 + (_Vec1 * (SizeY * ((fp32)y2 + (MFloat_GetRand(Rand++) - 0.5f) * 0.8f))));
	}

	// Setup mapping from 0->1 space (middle points)
	for(uint32 y = 1; y < m_nQuadsY; y++)
	{
		uint32 i = (y * nPointsX) + 1;
		for(uint32 x = 1; x < m_nQuadsX; x++)
		{
			CVec3Dfp32& GridPoint = lGridPoints[i++];
			GridPoint = (_Origin + (_Vec0 * (SizeX * ((fp32)x + ((MFloat_GetRand(Rand+0) - 0.5f) * 0.8f)))) +
								   (_Vec1 * (SizeY * ((fp32)y + ((MFloat_GetRand(Rand+1) - 0.5f) * 0.8f)))));
			Rand += 2;
		}
	}

	// Setup grid data to all quads valid or invalid if full break enable
	m_liRefresh.Clear();
	m_lActiveTime.Clear();

	uint8 nBits = 1;
	if(m_bTimeBreak)
	{
		nBits = 4;
		m_liRefresh.SetGrow(32);
		m_lActiveTime.SetLen(nQuads);
		FillChar(m_lActiveTime.GetBasePtr(), sizeof(int32) * m_lActiveTime.Len(), 0x0);
		m_iSound = 0;
	}
	else
		m_iSound = 1;

	m_GridData.SetLen(nQuads, nBits);
	m_GridData.SetAll(true);
}


bool CBSP4Glass_Grid::IsValid()
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::IsValid, XR_BSP4GLASS);
	return (m_bCreated != 0);
}


void CBSP4Glass_Grid::CreateGrid(const CGlassAttrib& _Attrib, const CVec3Dfp32* _pV, const uint32 _nV, const CMat4Dfp32& _WMat, aint _ShardModel, uint8 _CrushFlags)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::CreateGrid, XR_BSP4GLASS);
	CVec3Dfp32 HitPos;

	if(m_bCreated || _nV != 4)
	{
		if (_nV != 4)
		{
			M_TRACEALWAYS("Invalid number of vertices in glass model!!\n");
			M_BREAKPOINT;
		}
		return;
	}

	// Create grid points and flag quads as valid pieces
    CreateGridMap(_Attrib, _pV[0], _pV[3] - _pV[0], _pV[1] - _pV[0], _WMat, _CrushFlags);

	m_bCreated = 1;
	m_ShardModel = _ShardModel;
}


void CBSP4Glass_Grid::Phys_SetupIntersectGrid(const CVec3Dfp32& _LocalPos, const fp32* _pRadius, const CMat4Dfp32& _WMat, CBSP4Glass_Grid_PhysInfo& _PhysInfo)
{
	GLASS_MSCOPE(CBSP4Glass_Grid::Phys_SetupIntersectGrid, XR_BSP4GLASS);
	const TAP_RCD<CVec3Dfp32> lGridPoints = m_lGridPoints;

	const CVec3Dfp32 GridPoint0 = lGridPoints[0] * _WMat;
	const CVec3Dfp32 GridPointX = lGridPoints[m_nQuadsX] * _WMat;
	const CVec3Dfp32 GridPointXY = lGridPoints[(m_nQuadsX+1) * m_nQuadsY] * _WMat;
	const CVec3Dfp32 VecP = (_LocalPos * _WMat) - GridPoint0;
	const CVec3Dfp32 Vec0 = GridPointX  - GridPoint0;
	const CVec3Dfp32 Vec1 = GridPointXY - GridPoint0;

	CVec3Dfp32 VecP0;
	CVec3Dfp32 VecP1;
	VecP.Project(Vec0, VecP0);
	VecP.Project(Vec1, VecP1);

	const fp32 Size0 = 1.0f / (Vec0.Length() / m_nQuadsX);
	const fp32 Size1 = 1.0f / (Vec1.Length() / m_nQuadsY);

	// Calculate which quad we actually might be in (according to grid)
	if (_pRadius)
	{
		const fp32 VecP0Length = VecP0.Length();
		const fp32 VecP1Length = VecP1.Length();
		_PhysInfo.m_Start.k[0] = (int32)MaxMT(0, TruncToInt((VecP0Length - *_pRadius) * Size0) - 1);
		_PhysInfo.m_Start.k[1] = (int32)MaxMT(0, TruncToInt((VecP1Length - *_pRadius) * Size1) - 1);
		_PhysInfo.m_End.k[0]   = (int32)MinMT(m_nQuadsX, TruncToInt((VecP0Length + *_pRadius) * Size0) + 2);
		_PhysInfo.m_End.k[1]   = (int32)MinMT(m_nQuadsY, TruncToInt((VecP1Length + *_pRadius) * Size1) + 2);
	}
	else
	{
		const fp32 VecP0Length = VecP0.Length();
		const fp32 VecP1Length = VecP1.Length();
		_PhysInfo.m_Start.k[0] = (int32)MaxMT(0, TruncToInt(VecP0.Length() * Size0) - 1);
		_PhysInfo.m_Start.k[1] = (int32)MaxMT(0, TruncToInt(VecP1.Length() * Size1) - 1);
		_PhysInfo.m_End.k[0]   = (int32)MinMT(m_nQuadsX, _PhysInfo.m_Start.k[0] + 3);
		_PhysInfo.m_End.k[1]   = (int32)MinMT(m_nQuadsY, _PhysInfo.m_Start.k[1] + 3);
	}
}


void CBSP4Glass_Grid::InvalidateGrid(const CBSP4Glass_Grid_PhysInfo& _PhysInfo, int32 _GameTick, CVec3Dfp32* _pSrcPos, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE(CBSP4Glass_Grid::InvalidateGrid_PhysInfo, XR_BSP4GLASS);

	if (_PhysInfo.IsValid(m_nQuadsX, m_nQuadsY))
	{
		const CVec3Dfp32& WMatPos = _WMat.GetRow(3);
		if (m_bFullBreak)
		{
			m_GridData.SetAll(false);
			if (_pSrcPos)
			{
				uint nSrcPos = (m_nQuadsY * m_nQuadsY) << 1;
				for (uint i = 0; i < nSrcPos; i++)
					_pSrcPos[i] += WMatPos;
			}
		}
		else if (m_bTimeBreak)
		{
			// Setup random generator
			GLASS_RAND_CREATE1(m_Seed);
			uint iLastBit = m_GridData.GetLastBit();
			int32 iFirstX = _PhysInfo.m_Start.k[0];
			int32 iFirstY = _PhysInfo.m_Start.k[1];
			int32 iEndX = _PhysInfo.m_End.k[0];
			int32 iEndY = _PhysInfo.m_End.k[1];
			if (_pSrcPos)
			{
				for (int32 y = iFirstY; y < iEndY; y++)
				{
					int32 iGrid = ((y * m_nQuadsX) + iFirstX) << 1;
					for (int32 x = iFirstX; x < iEndX; x++, iGrid+=2)
					{
						_pSrcPos[iGrid] += WMatPos;
						_pSrcPos[iGrid+1] += WMatPos;
					}
				}
			}

			for (int32 y = iFirstY; y < iEndY; y++)
			{
				int32 iGrid = (y * m_nQuadsX) + iFirstX;
				for (int32 x = iFirstX; x < iEndX; x++, iGrid++)
				{
					m_GridData.Set0All(iGrid);
					m_lActiveTime[iGrid] = _GameTick;
				}
			}

			// Tag around
			iFirstX = MaxMT(0, iFirstX - 3);
			iFirstY = MaxMT(0, iFirstY - 3);
			iEndX = MinMT(m_nQuadsX, iEndX + 3);
			iEndY = MinMT(m_nQuadsY, iEndY + 3);
			for (int32 y = iFirstY; y < iEndY; y++)
			{
				int32 iGrid = (y * m_nQuadsX) + iFirstX;
				for (int32 x = iFirstX; x < iEndX; x++, iGrid++)
				{
					bool bLastMarked = false;
					
					uint8 nSet = ((GLASS_RAND % 3) + 1) * m_GridData.Get01(iGrid, iLastBit);
					for (uint8 i = 0; i < nSet; i++)
						bLastMarked |= m_GridData.Set0(iGrid);

					if (bLastMarked)
					{
						m_liRefresh.Add(MakeEntry(x, y));
						m_lActiveTime[iGrid] = _GameTick;
						if (_pSrcPos)
						{
							uint iSrcPos = iGrid << 1;
							_pSrcPos[iSrcPos] += WMatPos;
							_pSrcPos[iSrcPos+1] += WMatPos;
						}
					}
				}
			}
		}
		else
		{
			ConOutL("§cf80WARNING: No glass crush type set! Trying complete break!");
			m_GridData.SetAll(false);
		}
	}
}


void CBSP4Glass_Grid::InvalidateGrid(uint32 _nGrids, uint32* _pGrids, int32 _GameTick, CVec3Dfp32* _pSrcPos, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE(CBSP4Glass_Grid::InvalidateGrid_GridIndices, XR_BSP4GLASS);
	if (_nGrids > 0)
	{
		const CVec3Dfp32& WMatPos = _WMat.GetRow(3);

		if (m_bTimeBreak)
		{
			CBSP4Glass_Grid_CollisionInfo GCInfo;
			uint8 iLastBit = m_GridData.GetLastBit();
			int32 nQuadsX = m_nQuadsX;
			int32 nQuadsY = m_nQuadsY;

			CGridData Data;
			Data.SetLen(nQuadsX * nQuadsY, 1);
			Data.SetAll(true);

			for (int32 i = 0; i < _nGrids; i++)
			{
				int32 iInside = _pGrids[i];
				if (m_GridData.Get(iInside, iLastBit))
				{
					m_GridData.Set0All(iInside);
					m_lActiveTime[iInside] = _GameTick;
					
					if (_pSrcPos)
					{
						uint iSrcPos = iInside << 1;
						_pSrcPos[iSrcPos] += WMatPos;
						_pSrcPos[iSrcPos+1] += WMatPos;
					}

					int32 Seed = m_Seed * iInside;
					GLASS_RAND_CREATE1(Seed);

					// Mark grid point around
					int32 iHitY = iInside / nQuadsX;
					int32 iHitX = iInside - (iHitY * nQuadsX);
					int32 iFirstX = MaxMT(0, iHitX - 3);
					int32 iFirstY = MaxMT(0, iHitY - 3);
					int32 iEndX = MinMT(nQuadsX, iHitX + 3);
					int32 iEndY = MinMT(nQuadsY, iHitY + 3);
					for(int32 y = iFirstY; y < iEndY; y++)
					{
						int32 iGrid = (y * m_nQuadsX) + iFirstX;
						for(int32 x = iFirstX; x < iEndX; x++, iGrid++)
						{
							// Last bit marked?
							uint8 nSet = ((GLASS_RAND % 3) + 1) * m_GridData.Get01(iGrid, iLastBit);
							bool bLastMarked = false;
							if (Data.Get(iGrid, 0))
							{
								Data.Set0(iGrid);
								for(uint8 i = 0; i < nSet; i++)
									bLastMarked |= m_GridData.Set0(iGrid);
							}

							if (bLastMarked)
							{
								m_liRefresh.Add(MakeEntry(x, y));
								m_lActiveTime[iGrid] = _GameTick;

								if (_pSrcPos)
								{
									uint iSrcPos = iGrid << 1;
									_pSrcPos[iSrcPos] += WMatPos;
									_pSrcPos[iSrcPos+1] += WMatPos;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			if (!m_bFullBreak)
				ConOutL("§cf80WARNING: No glass crush type set! Trying complete break!");

			m_GridData.SetAll(false);
			if (_pSrcPos)
			{
				uint nSrcPos = ((m_nQuadsX * m_nQuadsY) << 1);
				for (uint i = 0; i < nSrcPos; i++)
					_pSrcPos[i] += WMatPos;
			}
		}
	}
}


void CBSP4Glass_Grid::InvalidateGrid(const CBSP4Glass_Grid_CollisionInfo& _CInfo, int32 _GameTick, CVec3Dfp32* _pSrcPos, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE(CBSP4Glass_Grid::InvalidateGrid_CollisionInfo, XR_BSP4GLASS);

	// We actually hit something
	if(_CInfo.IsValid())
	{
		// Ivalidate either whole glass or part of it
		const CVec3Dfp32& WMatPos = _WMat.GetRow(3);
		int32 iInside = _CInfo.m_iInside;
		if(m_bFullBreak)
		{
			m_GridData.SetAll(false);
			if (_pSrcPos)
			{
				uint nSrcPos = ((m_nQuadsX * m_nQuadsY) << 1);
				for (uint i = 0; i < nSrcPos; i++)
					_pSrcPos[i] += WMatPos;
			}
		}
		else if(m_bTimeBreak)
		{
			int32 iHitX = _CInfo.m_iHitX;
			int32 iHitY = _CInfo.m_iHitY;
			int32 Seed = m_Seed * iInside;

			// If piece is still here, just mark it more
			uint8 iLastBit = m_GridData.GetLastBit();
			if(m_GridData.Get(iInside, iLastBit))
			{
				GLASS_RAND_CREATE1(Seed);

				m_GridData.Set0All(iInside);
				m_lActiveTime[iInside] = _GameTick;

				if (_pSrcPos)
				{
					uint iSrcPos = iInside << 1;
					_pSrcPos[iSrcPos] += WMatPos;
					_pSrcPos[iSrcPos+1] += WMatPos;
				}

				// Mark grid point around
				int32 iFirstX = MaxMT(0, iHitX - 3);
				int32 iFirstY = MaxMT(0, iHitY - 3);
				int32 iEndX = MinMT(m_nQuadsX, iHitX + 3);
				int32 iEndY = MinMT(m_nQuadsY, iHitY + 3);
				for(int32 y = iFirstY; y < iEndY; y++)
				{
					int32 iGrid = (y * m_nQuadsX) + iFirstX;
					for(int32 x = iFirstX; x < iEndX; x++, iGrid++)
					{
						// Last bit marked?
						uint8 nSet = ((GLASS_RAND % 3) + 1) * m_GridData.Get01(iGrid, iLastBit);
						bool bLastMarked = false;
						for(uint8 i = 0; i < nSet; i++)
							bLastMarked |= m_GridData.Set0(iGrid);

						if(bLastMarked)
						{
							m_liRefresh.Add(MakeEntry(x, y));
							m_lActiveTime[iGrid] = _GameTick;

							if (_pSrcPos)
							{
								uint iSrcPos = iGrid << 1;
								_pSrcPos[iSrcPos] += WMatPos;
								_pSrcPos[iSrcPos+1] += WMatPos;
							}
						}
					}
				}

				#ifndef M_RTM
					//Debug_DumpGrid();
				#endif
			}
		}
		else
		{
			ConOutL("§cf80WARNING: No glass crush type set! Trying complete break!");
			m_GridData.SetAll(false);
		}
	}
}


int32 CBSP4Glass_Grid::Refresh(CVec4Dfp32* _pVel, const int32 _GameTick, CVec3Dfp32* _pSrcPos, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::Refresh, XR_BSP4GLASS);

	TAP_RCD<uint32> liRefresh = m_liRefresh;
	uint Len = liRefresh.Len();

	if(Len <= 0)
		return -1;

	int32 iGridX = (int32)GetEntryX(liRefresh[0]);
	int32 iGridY = (int32)GetEntryY(liRefresh[0]);
	int32 iGridXY = (iGridY * m_nQuadsX) + iGridX;

	// If piece is still here, just mark it more

	// Mark grid point around
	int32 iFirstX = MaxMT(0, iGridX - 1);
	int32 iFirstY = MaxMT(0, iGridY - 1);
	int32 iEndX = MinMT(m_nQuadsX, iGridX + 2);
	int32 iEndY = MinMT(m_nQuadsY, iGridY + 2);
	for(int32 y = iFirstY; y < iEndY; y++)
	{
		int32 iGrid = (y * m_nQuadsX) + iFirstX;
		for(int32 x = iFirstX; x < iEndX; x++, iGrid++)
		{
			// Last bit marked?
			if(m_GridData.Set0(iGrid))
			{
				m_liRefresh.Add(MakeEntry(x, y));
				m_lActiveTime[iGrid] = _GameTick;
				
				if (_pSrcPos)
				{
					uint iSrcPos = iGrid << 1;
					_pSrcPos[iSrcPos] += _WMat.GetRow(3);
					_pSrcPos[iSrcPos+1] += _WMat.GetRow(3);
				}
				//CalcNewVelocity(_pVel, iGrid+x);
			}
		}
	}

	if(Len > 0)
	{
		#ifndef M_RTM
			//Debug_DumpGrid();
		#endif
		m_liRefresh.Delx(0, 1);
	}

	return iGridXY;
}


void CBSP4Glass_Grid::SetNewRefreshVelocity(TAP_RCD<CVec4Dfp32>& _lVelocity, const int32 _nRefresh, const CVec3Dfp32& _Force)
{
	TAP_RCD<uint32> liRefresh = m_liRefresh;
	int32 Len = liRefresh.Len();

	for(int32 i = 0; i < _nRefresh; i++)
	{
		uint32 iRefresh = liRefresh[Len - i - 1];
		uint32 iX = GetEntryX(iRefresh);
		uint32 iY = GetEntryY(iRefresh);
		SetNewVelocity(_lVelocity, (iY * m_nQuadsX) + iX, _Force);
	}
}


void CBSP4Glass_Grid::SetNewVelocity(TAP_RCD<CVec4Dfp32>& _lVelocity, const int32 _iHitIndex, const CVec3Dfp32& _Force)
{
	GLASS_RAND_CREATE1(m_Seed * _iHitIndex + 0xfab5);
	_lVelocity[(_iHitIndex << 1)+0] = _Force * ((MFloat_GetRand(GLASS_RAND) + 0.5f) * 4.0f);
	_lVelocity[(_iHitIndex << 1)+1] = _Force * ((MFloat_GetRand(GLASS_RAND) + 0.5f) * 4.0f);
}


void CBSP4Glass_Grid::CalcNewVelocity(CVec4Dfp32* _pVel, int32 _iXY)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::::CalcNewVelocity, XR_BSP4GLASS);

	CVec4Dfp32& Velocity0 = _pVel[_iXY << 1];
	CVec4Dfp32& Velocity1 = _pVel[(_iXY << 1)+1];

	CVec4Dfp32 Vel(0.0f);

	const int32 nQuadsX = m_nQuadsX;
	const int32 nQuadsY = m_nQuadsY;
	const int32 nQuadsXY = nQuadsX * nQuadsY;
	const int32 iQuadXBorder0 = (_iXY / nQuadsX) * nQuadsX;
	const int32 iQuadXBorder1 = iQuadXBorder0 + nQuadsX;

	const int32 i1 = _iXY - nQuadsX;
	const int32 i2 = _iXY + nQuadsX;
	const int32 i3 = _iXY - 1;
	const int32 i4 = _iXY + 1;

	if(i1 >= 0)
		Vel += _pVel[i1];

	if(i2 < nQuadsXY)
		Vel += _pVel[i2];

	if(i3 >= iQuadXBorder0)
		Vel += _pVel[i3];

	if(i4 < iQuadXBorder1)
		Vel += _pVel[i4];

	Velocity0 = Vel * 0.125f;
	Velocity1 = Vel * 0.125f;
}


#ifndef M_RTM
void CBSP4Glass_Grid::Debug_RenderGrid(CWireContainer* _pWC, CPixel32 _Color, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE_ALL(CBSP4Glass_Grid::Debug_RenderGrid, XR_BSP4GLASS);
	if(m_bCreated && _pWC)
	{
		const TAP_RCD<CVec3Dfp32> lGridPoints = m_lGridPoints;
		
		// Render only valid grid quads
		const uint32 nQuadsX = m_nQuadsX;
		const uint32 nQuadsY = m_nQuadsY;
		for(uint32 y = 0; y < nQuadsY; y++)
		{
			uint32 i = (nQuadsX * y);
			for(uint32 x = 0; x < nQuadsX; x++, i++)
			{
				if (m_GridData.Get(i, m_GridData.GetLastBit()))
				{
					uint iPoint0 = (y * (nQuadsX+1)) + x;
					uint iPoint1 = iPoint0 + (nQuadsX+1);
					_pWC->RenderWire(lGridPoints[iPoint0] * _WMat, lGridPoints[iPoint0+1] * _WMat, _Color, 0.0f, false);
					_pWC->RenderWire(lGridPoints[iPoint1] * _WMat, lGridPoints[iPoint1+1] * _WMat, _Color, 0.0f, false);
					_pWC->RenderWire(lGridPoints[iPoint0] * _WMat, lGridPoints[iPoint1] * _WMat, _Color, 0.0f, false);
					_pWC->RenderWire(lGridPoints[iPoint0+1] * _WMat, lGridPoints[iPoint1+1] * _WMat, _Color, 0.0f, false);
				}
			}
		}
	}
}

void CBSP4Glass_Grid::Debug_DumpGrid()
{
	if(m_bTimeBreak)
	{
		uint8 nBits = m_GridData.GetLastBit()+1;

		M_TRACEALWAYS("\n\n\nGlassGrid:\n");
		for(uint32 y = 0; y < m_nQuadsY; y++)
		{
			CStr Row("GridData: ");
			int iGrid = y * m_nQuadsX;
			for(uint32 x = 0; x < m_nQuadsX; x++)
			{
				uint8 Health = 0;
				for(uint i = 0; i < nBits; i++)
					Health += (m_GridData.Get(iGrid+x, i) ? 1 : 0);

				Row = CStrF("%s %d", Row.GetStr(), Health);
			}

			M_TRACEALWAYS("   %s\n", Row.GetStr());
		}
	}
}
#endif

