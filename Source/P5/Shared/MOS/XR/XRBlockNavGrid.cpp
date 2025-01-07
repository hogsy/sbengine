
#include "PCH.h"

#include "XRBlockNavGrid.h"
#include "MFloat.h"

MRTC_IMPLEMENT_DYNAMIC(CXR_BlockNav_Grid, CReferenceCount);
MRTC_IMPLEMENT_DYNAMIC(CXR_BlockNav_GridBuilder, CReferenceCount);

//#pragma optimize("", off)
//#pragma inline_depth(0)

// -------------------------------------------------------------------
//  CXR_BlockNav_BitPlane
// -------------------------------------------------------------------
void CXR_BlockNav_BitPlane::Clear(int _Value)
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_Clear, MAUTOSTRIP_VOID);
	int FillVal = (_Value & 1) ? 0xff : 0;
	FillChar(m_lBits, sizeof(m_lBits), FillVal);
}

bool CXR_BlockNav_BitPlane::IsZero() const
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_IsZero, false);
	for(int i = 0; i < sizeof(m_lBits); i++)
		if (m_lBits[i]) return false;
	return true;
}

bool CXR_BlockNav_BitPlane::IsEqual(const CXR_BlockNav_BitPlane& _Plane) const
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_IsEqual, false);
	for(int i = 0; i < sizeof(m_lBits); i++)
		if (m_lBits[i] != _Plane.m_lBits[i]) return false;
	return true;

//	return memcmp(m_lBits, _Plane.m_lBits, sizeof(m_lBits)) == 0;
}

/*void CXR_BlockNav_BitPlane::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_Read, NULL);
	uint16 Ver = 0;
	_pFile->ReadLE(Ver);
	switch(Ver)
	{
	case 0x0100 : 
		{
			_pFile->ReadLE(m_lBits, sizeof(m_lBits));
			break;
		}

	default :
		Error_static("CXR_BlockNav_BitPlane::Read", CStrF("Invalid version: %.4x", Ver));
	}
}

void CXR_BlockNav_BitPlane::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_Write, MAUTOSTRIP_VOID);
	uint16 Ver = XR_NAVBITPLANE_VERSION;
	_pFile->WriteLE(Ver);
	_pFile->WriteLE(m_lBits, sizeof(m_lBits));
}
*/

static uint GetBit(const uint8* _pBits, int _Bit)
{
	MAUTOSTRIP(GetBit, 0);
	return (_pBits[_Bit >> 3] & (1 << (_Bit & 7))) ? 1 : 0;
}

static void SetBit(uint8* _pBits, int _Pos, int _Bit)
{
	MAUTOSTRIP(SetBit, MAUTOSTRIP_VOID);
	if (_Bit & 1)
		_pBits[_Pos >> 3] |= 1 << (_Pos & 7);
	else
		_pBits[_Pos >> 3] &= ~(1 << (_Pos & 7));
}

// -------------------------------------------------------------------
bool CXR_BlockNav_BitPlane::BoxIsValue(const CVec3Dint32& _Origin, const CVec3Dint32& _Size, int _Value) const
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_BoxIsValue, false);
	for(int x = 0; x < _Size[0]; x++)
		for(int y = 0; y < _Size[1]; y++)
			for(int z = 0; z < _Size[2]; z++)
			{
				CVec3Dint32 Pos(_Origin[0] + x, _Origin[1] + y, _Origin[2] + z);
				if (GetAt(Pos) != _Value) return false;
			}

	return true;
}

int CXR_BlockNav_BitPlane::GetBoxValues(const CVec3Dint32& _Origin, const CVec3Dint32& _Size) const
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_GetBoxValues, 0);
	int Mask = 0;
	for(int x = 0; x < _Size[0]; x++)
		for(int y = 0; y < _Size[1]; y++)
			for(int z = 0; z < _Size[2]; z++)
			{
				CVec3Dint32 Pos(_Origin[0] + x, _Origin[1] + y, _Origin[2] + z);
				Mask |= (1 << GetAt(Pos));
			}

	return Mask;
}

int CXR_BlockNav_BitPlane::CompressOcttree_r(uint8* _pCompressBuffer, int _Pos, const CVec3Dint32& _Origin, int _Size) const
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_CompressOcttree_r, 0);
	int Pos = _Pos;
	uint Value = GetAt(_Origin);

	if (_Size == 1)
	{
		SetBit(_pCompressBuffer, Pos++, Value);
	}
	else
	{
		if (BoxIsValue(_Origin, _Size, Value))
		{
			SetBit(_pCompressBuffer, Pos++, 0);
			SetBit(_pCompressBuffer, Pos++, Value);
		}
		else
		{
			if (_Size <= 1) Error_static("CXR_BlockNav_BitPlane::CompressOcttree_r", "Internal error.");

			SetBit(_pCompressBuffer, Pos++, 1);
			_Size >>= 1;

			Pos += CompressOcttree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,0,0),			_Size);
			Pos += CompressOcttree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(_Size,0,0),		_Size);
			Pos += CompressOcttree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,_Size,0),		_Size);
			Pos += CompressOcttree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(_Size,_Size,0),	_Size);
			Pos += CompressOcttree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,0,_Size),		_Size);
			Pos += CompressOcttree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(_Size,0,_Size),	_Size);
			Pos += CompressOcttree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,_Size,_Size),	_Size);
			Pos += CompressOcttree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(_Size,_Size,_Size), _Size);
		}
	}

	return Pos - _Pos;
}

int CXR_BlockNav_BitPlane::CompressBintree_r(uint8* _pCompressBuffer, int _Pos, const CVec3Dint32& _Origin, const CVec3Dint32& _Size) const
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_CompressBintree_r, 0);
	int Pos = _Pos;
	uint Value = GetAt(_Origin);

	if (_Size[0] == 1 && _Size[1] == 1 && _Size[2] == 1)
	{
		SetBit(_pCompressBuffer, Pos++, Value);
	}
	else
	{
		if (BoxIsValue(_Origin, _Size, Value))
		{
			SetBit(_pCompressBuffer, Pos++, 0);
			SetBit(_pCompressBuffer, Pos++, 0);
			SetBit(_pCompressBuffer, Pos++, Value);
		}
		else
		{
			bool bForce = false;

			while(1)
			{
				if (_Size[0] > 1)
				{
					CVec3Dint32 SizeX(_Size[0] >> 1, _Size[1], _Size[2]);
					int Left = GetBoxValues(CVec3Dint32(_Origin[0], _Origin[1], _Origin[2]), SizeX);
					int Right = GetBoxValues(CVec3Dint32(_Origin[0] + SizeX[0], _Origin[1], _Origin[2]), SizeX);

					if (bForce || Left < 3 || Right < 3)
					{
						SetBit(_pCompressBuffer, Pos++, 1);
						SetBit(_pCompressBuffer, Pos++, 0);
						Pos += CompressBintree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,0,0),		SizeX);
						Pos += CompressBintree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(SizeX[0],0,0),	SizeX);
						return Pos - _Pos;
					}
				}

				if (_Size[1] > 1)
				{
					CVec3Dint32 SizeZ(_Size[0], _Size[1] >> 1, _Size[2]);
					int Left = GetBoxValues(CVec3Dint32(_Origin[0], _Origin[1], _Origin[2]), SizeZ);
					int Right = GetBoxValues(CVec3Dint32(_Origin[0], _Origin[1] + SizeZ[1], _Origin[2]), SizeZ);

					if (bForce || Left < 3 || Right < 3)
					{
						SetBit(_pCompressBuffer, Pos++, 0);
						SetBit(_pCompressBuffer, Pos++, 1);
						Pos += CompressBintree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,0,0),		SizeZ);
						Pos += CompressBintree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,SizeZ[1],0),	SizeZ);
						return Pos - _Pos;
					}
				}

				if (_Size[2] > 1)
				{
					CVec3Dint32 SizeZ(_Size[0], _Size[1], _Size[2] >> 1);
					int Left = GetBoxValues(CVec3Dint32(_Origin[0], _Origin[1], _Origin[2]), SizeZ);
					int Right = GetBoxValues(CVec3Dint32(_Origin[0], _Origin[1], _Origin[2] + SizeZ[2]), SizeZ);

					if (bForce || Left < 3 || Right < 3)
					{
						SetBit(_pCompressBuffer, Pos++, 1);
						SetBit(_pCompressBuffer, Pos++, 1);
						Pos += CompressBintree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,0,0),		SizeZ);
						Pos += CompressBintree_r(_pCompressBuffer, Pos, _Origin + CVec3Dint32(0,0,SizeZ[2]),	SizeZ);
						return Pos - _Pos;
					}
				}

				if (bForce) Error_static("CompressBintree_r", "Internal error.");
				bForce = true;
			}
		}
	}

	return Pos - _Pos;
}

int CXR_BlockNav_BitPlane::GetMaxCompressedSize()
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_GetMaxCompressedSize, 0);
	int nBits = XR_NAVTILE_DIM*XR_NAVTILE_DIM*XR_NAVTILE_DIM * 3;
/*		((XR_NAVTILE_DIM*XR_NAVTILE_DIM*XR_NAVTILE_DIM) >> 3);
		((XR_NAVTILE_DIM*XR_NAVTILE_DIM*XR_NAVTILE_DIM) >> 6);
		((XR_NAVTILE_DIM*XR_NAVTILE_DIM*XR_NAVTILE_DIM) >> 9);*/

	return nBits >> 3;
}

int CXR_BlockNav_BitPlane::CompressOcttree(uint8* _pCompressBuffer) const
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_CompressOcttree, 0);
	FillChar(_pCompressBuffer, GetMaxCompressedSize(), 0);
	int Size = CompressOcttree_r(_pCompressBuffer, 0, CVec3Dint32(0, 0, 0), XR_NAVTILE_DIM);
	int SizeBytes = (Size + 7) >> 3;
	return SizeBytes;
}

int CXR_BlockNav_BitPlane::CompressBintree(uint8* _pCompressBuffer) const
{
	MAUTOSTRIP(CXR_BlockNav_BitPlane_CompressBintree, 0);
	FillChar(_pCompressBuffer, GetMaxCompressedSize(), 0);
	int Size = CompressBintree_r(_pCompressBuffer, 0, CVec3Dint32(0, 0, 0), XR_NAVTILE_DIM);
	int SizeBytes = (Size + 7) >> 3;
	return SizeBytes;
}

int Decompress_r(const uint8* _pBuffer, int _Pos, const CVec3Dint32& _Origin, int _Size, CXR_BlockNav_BitPlane& _BitPlane);
int Decompress_r(const uint8* _pBuffer, int _Pos, const CVec3Dint32& _Origin, int _Size, CXR_BlockNav_BitPlane& _BitPlane)
{
	MAUTOSTRIP(Decompress_r, 0);
	int Pos = _Pos;
	if (_Size == 1)
	{
		_BitPlane.SetAt(_Origin, GetBit(_pBuffer, Pos++));
	}
	else
	{
		if (GetBit(_pBuffer, Pos++))
		{
			// Subdivide
			_Size >>= 1;
			Pos += Decompress_r(_pBuffer, Pos, _Origin + CVec3Dint32(0,0,0),			_Size, _BitPlane);
			Pos += Decompress_r(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,0,0),		_Size, _BitPlane);
			Pos += Decompress_r(_pBuffer, Pos, _Origin + CVec3Dint32(0,_Size,0),		_Size, _BitPlane);
			Pos += Decompress_r(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,_Size,0),	_Size, _BitPlane);
			Pos += Decompress_r(_pBuffer, Pos, _Origin + CVec3Dint32(0,0,_Size),		_Size, _BitPlane);
			Pos += Decompress_r(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,0,_Size),	_Size, _BitPlane);
			Pos += Decompress_r(_pBuffer, Pos, _Origin + CVec3Dint32(0,_Size,_Size),	_Size, _BitPlane);
			Pos += Decompress_r(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,_Size,_Size), _Size, _BitPlane);
		}
		else
		{
			// Fill
			int FillValue = GetBit(_pBuffer, Pos++);
			for(int x = 0; x < _Size; x++)
				for(int y = 0; y < _Size; y++)
					for(int z = 0; z < _Size; z++)
						_BitPlane.SetAt(_Origin[0] + x, _Origin[1] + y, _Origin[2] + z, FillValue);
		}
	}

	return Pos - _Pos;
}

#ifdef XR_NAVGRID_BYTECELL

int Decompress2_r(const uint8* _pBuffer, int _Pos, const CVec3Dint32& _Origin, uint _Level, uint _Size, CXR_BlockNav_Grid::CDecompressedTile& _Tile, CVec128Access& _BitValue)
{
	MAUTOSTRIP(Decompress2_r, 0);
	int Pos = _Pos;
	if (_Size == 1)
	{
		uint FillValue = GetBit(_pBuffer, Pos++);
		if (FillValue)
			_Tile.OrAt(_Origin, _BitValue.ku8[0]);
//		_BitPlane.SetAt(_Origin, );
	}
	else
	{
		if (GetBit(_pBuffer, Pos++))
		{
			// Subdivide
			_Size >>= 1;
			Pos += Decompress2_r(_pBuffer, Pos, _Origin + CVec3Dint32(0,0,0),			_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_r(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,0,0),		_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_r(_pBuffer, Pos, _Origin + CVec3Dint32(0,_Size,0),		_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_r(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,_Size,0),	_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_r(_pBuffer, Pos, _Origin + CVec3Dint32(0,0,_Size),		_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_r(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,0,_Size),	_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_r(_pBuffer, Pos, _Origin + CVec3Dint32(0,_Size,_Size),	_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_r(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,_Size,_Size), _Level+1, _Size, _Tile, _BitValue);
		}
		else
		{
			// Fill
			uint FillValue = GetBit(_pBuffer, Pos++);
			if (FillValue)
			{
				vec128 FillAnd = CXR_BlockNav_Grid::CDecompressedTile::ms_lFillAnd[_Level*16 + _Origin[0]];
				vec128 OrVal = M_VAnd(FillAnd, _BitValue);

				for(uint y = 0; y < _Size; y++)
					for(uint z = 0; z < _Size; z++)
						_Tile.OrRow(_Origin[1] + y, _Origin[2] + z, OrVal);
			}

//						_BitPlane.SetAt(_Origin[0] + x, _Origin[1] + y, _Origin[2] + z, FillValue);

/*				for(int x = 0; x < _Size; x++)
					for(int y = 0; y < _Size; y++)
						for(int z = 0; z < _Size; z++)
							_BitPlane.SetAt(_Origin[0] + x, _Origin[1] + y, _Origin[2] + z, FillValue);*/
		}
	}

	return Pos - _Pos;
}
/*
int Decompress2_rTest(const uint8* _pBuffer, int _Pos, const CVec3Dint32& _Origin, uint _Level, uint _Size, CXR_BlockNav_Grid::CDecompressedTile& _Tile, CVec128Access& _BitValue)
{
	MAUTOSTRIP(Decompress2_r, 0);
	int Pos = _Pos;
	if (_Size == 1)
	{
		uint FillValue = GetBit(_pBuffer, Pos++);
		if (FillValue)
			if ((_Tile.GetAt(_Origin) & (_BitValue.ku8[0]))!=_BitValue.ku8[0])
				M_BREAKPOINT;
	}
	else
	{
		if (GetBit(_pBuffer, Pos++))
		{
			// Subdivide
			_Size >>= 1;
			Pos += Decompress2_rTest(_pBuffer, Pos, _Origin + CVec3Dint32(0,0,0),			_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_rTest(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,0,0),		_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_rTest(_pBuffer, Pos, _Origin + CVec3Dint32(0,_Size,0),		_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_rTest(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,_Size,0),	_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_rTest(_pBuffer, Pos, _Origin + CVec3Dint32(0,0,_Size),		_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_rTest(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,0,_Size),	_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_rTest(_pBuffer, Pos, _Origin + CVec3Dint32(0,_Size,_Size),	_Level+1, _Size, _Tile, _BitValue);
			Pos += Decompress2_rTest(_pBuffer, Pos, _Origin + CVec3Dint32(_Size,_Size,_Size), _Level+1, _Size, _Tile, _BitValue);
		}
		else
		{
			// Fill
			uint FillValue = GetBit(_pBuffer, Pos++);
			if (FillValue)
			{
				vec128 FillAnd = CXR_BlockNav_Grid::CDecompressedTile::ms_lFillAnd[_Level*16 + _Origin[0]];
				vec128 OrVal = M_VAnd(FillAnd, _BitValue);

				for(uint y = 0; y < _Size; y++)
					for(uint z = 0; z < _Size; z++)
						if (!M_VCmpAllEq(M_VAnd(_Tile.GetRow(_Origin[1] + y, _Origin[2] + z),OrVal),OrVal))
							M_BREAKPOINT;
			}
		}
	}

	return Pos - _Pos;
}
*/

int Decompress2_x(const uint8* _pBuffer, int _Pos, const CVec3Dint32& _Origin, uint _Level, uint _Size, CXR_BlockNav_Grid::CDecompressedTile& _Tile, CVec128Access& _BitValue)
{
	MAUTOSTRIP(Decompress2_x, 0);

	enum { StackSize=512 };
	struct M_ALIGN(16) StackData
	{
		void Set(const CVec3Dint32& _Origin,uint16 _Size, uint16 _Level)
		{
			m_Origin = _Origin;
			m_Size = _Size;
			m_Level = _Level;
		}
		CVec3Dint32 m_Origin;
		uint16 m_Level;
		uint16 m_Size;
	};
	StackData WorkingStack[StackSize];
	uint stackpnt=0;
	WorkingStack[stackpnt].Set(_Origin,_Size,_Level);
	stackpnt++;
	int Pos = _Pos;
	while (stackpnt)
	{
		stackpnt--;
		uint16 Size = WorkingStack[stackpnt].m_Size;
		uint16 Level = WorkingStack[stackpnt].m_Level;
		CVec3Dint32 Origin = WorkingStack[stackpnt].m_Origin;
		if (Size == 1)
		{
			uint FillValue = GetBit(_pBuffer, Pos++);
			if (FillValue)
				_Tile.OrAt(Origin, _BitValue.ku8[0]);
			//		_BitPlane.SetAt(_Origin, );
		}
		else
		{
			if (GetBit(_pBuffer, Pos++))
			{
				// Subdivide
				Size >>= 1;
				if (stackpnt+8>=StackSize)
					M_BREAKPOINT;
				Level++;
#if 1
				CVec3Dint32 PlusSize=Origin+CVec3Dint32(Size,Size,Size);
				WorkingStack[stackpnt+0].Set(PlusSize,Size,Level);
				WorkingStack[stackpnt+1].Set(CVec3Dint32(  Origin.k[0],PlusSize.k[1],PlusSize.k[2]),Size,Level);
				WorkingStack[stackpnt+2].Set(CVec3Dint32(PlusSize.k[0],  Origin.k[1],PlusSize.k[2]),Size,Level);
				WorkingStack[stackpnt+3].Set(CVec3Dint32(  Origin.k[0],  Origin.k[1],PlusSize.k[2]),Size,Level);
				WorkingStack[stackpnt+4].Set(CVec3Dint32(PlusSize.k[0],PlusSize.k[1],  Origin.k[2]),Size,Level);
				WorkingStack[stackpnt+5].Set(CVec3Dint32(  Origin.k[0],PlusSize.k[1],  Origin.k[2]),Size,Level);
				WorkingStack[stackpnt+6].Set(CVec3Dint32(PlusSize.k[0],  Origin.k[1],  Origin.k[2]),Size,Level);
				WorkingStack[stackpnt+7].Set(Origin,Size,Level);
#else
				vec128 * M_RESTRICT Vecs = (vec128 * M_RESTRICT) &WorkingStack[stackpnt];

				vec128 Originv = M_VLdU(&Origin);
				vec128 PlusSize = M_VAdd_u32(Originv,M_VLdScalar_u32(Size));
				uint tmp= Size | (Level<<16);
				vec128 SizeLevelv = M_VLdScalar_u32(tmp);
				Originv = M_VSelMsk(M_VConstMsk(1,1,1,0),Originv,SizeLevelv);
				PlusSize = M_VSelMsk(M_VConstMsk(1,1,1,0),PlusSize,SizeLevelv);
				Vecs[0]=(PlusSize);
				Vecs[1]=(M_VSelMsk(M_VConstMsk(1,0,0,1),Originv,PlusSize));
				Vecs[2]=(M_VSelMsk(M_VConstMsk(0,1,0,1),Originv,PlusSize));
				Vecs[3]=(M_VSelMsk(M_VConstMsk(1,1,0,1),Originv,PlusSize));
				Vecs[4]=(M_VSelMsk(M_VConstMsk(0,0,1,1),Originv,PlusSize));
				Vecs[5]=(M_VSelMsk(M_VConstMsk(1,0,1,1),Originv,PlusSize));
				Vecs[6]=(M_VSelMsk(M_VConstMsk(0,1,1,1),Originv,PlusSize));
				Vecs[7]=(Originv);
#endif
				stackpnt+=8;
			}
			else
			{
				// Fill
				uint FillValue = GetBit(_pBuffer, Pos++);
				if (FillValue)
				{
					vec128 FillAnd = CXR_BlockNav_Grid::CDecompressedTile::ms_lFillAnd[Level*16 + Origin[0]];
					vec128 OrVal = M_VAnd(FillAnd, _BitValue);

					for(uint y = 0; y < Size; y++)
						for(uint z = 0; z < Size; z++)
							_Tile.OrRow(Origin[1] + y, Origin[2] + z, OrVal);
				}

				//						_BitPlane.SetAt(_Origin[0] + x, _Origin[1] + y, _Origin[2] + z, FillValue);

				/*				for(int x = 0; x < _Size; x++)
				for(int y = 0; y < _Size; y++)
				for(int z = 0; z < _Size; z++)
				_BitPlane.SetAt(_Origin[0] + x, _Origin[1] + y, _Origin[2] + z, FillValue);*/
			}
		}
	}
	return Pos - _Pos;
}



vec128 CXR_BlockNav_Grid::CDecompressedTile::ms_lFillAnd[4*16];

void CXR_BlockNav_Grid::CDecompressedTile::CreateFillAndTable()
{
	for(int l = 0; l < 4; l++)
	{
		for(uint x = 0; x < 16; x++)
		{
			uint size = M_BitD(4-l);
			CVec128Access v;
			v.v = M_VScalar_u32(0);
			size = Min(size, 16-x);
			for(int i = 0; i < size; i++)
				v.ku8[x + i] = 0xff;
			ms_lFillAnd[l*16 + x] = v.v;
		}
	}
}

void CXR_BlockNav_Grid::CDecompressedTile::Decompress(const uint8* _pBuffer, uint _Value)
{
	CVec128Access Value;
	uint32 v32 = _Value + (_Value << 8) + (_Value << 16) + (_Value << 24);
	Value.ku32[0] = v32;
	Value.ku32[1] = v32;
	Value.ku32[2] = v32;
	Value.ku32[3] = v32;
	Decompress2_x(_pBuffer, 0, CVec3Dint32(0,0,0), 0, XR_NAVTILE_DIM, *this, Value);
}

#endif

void CXR_BlockNav_BitPlane::Decompress(const uint8* _pBuffer)
{
	Decompress_r(_pBuffer, 0, CVec3Dint32(0,0,0), XR_NAVTILE_DIM, *this);
}

// -------------------------------------------------------------------
//  CXR_BlockNav_BitPlaneContainer
// -------------------------------------------------------------------

void CXR_BlockNav_BitPlaneContainer::AddBitPlane(const CXR_BlockNav_BitPlane& _BitPlane)
{
	MAUTOSTRIP(CXR_BlockNav_BitPlaneContainer_AddBitPlane, MAUTOSTRIP_VOID);
	m_lData.SetGrow(Max(65536, m_lData.Len() >> 1));

	uint8 Buffer[4096];
	if (sizeof(Buffer) < CXR_BlockNav_BitPlane::GetMaxCompressedSize()) Error("AddBitPlane", "Internal error.");

	int Size = _BitPlane.CompressOcttree(&Buffer[0]);

	m_liBitPlaneData.Add(m_lData.Len());
	for(int i = 0; i < Size; i++)
		m_lData.Add(Buffer[i]);

//	m_lData.SetLen(m_lData.Len() + Size);
//	memcpy(&m_lData[m_lData.Len() - Size], Buffer, Size);
}

void CXR_BlockNav_BitPlaneContainer::GetBitPlane(int _iBitPlane, CXR_BlockNav_BitPlane& _BitPlane)
{
	MAUTOSTRIP(CXR_BlockNav_BitPlaneContainer_GetBitPlane, MAUTOSTRIP_VOID);
	int iData = m_liBitPlaneData[_iBitPlane];
	_BitPlane.Decompress(&m_lData[iData]);
}

void CXR_BlockNav_BitPlaneContainer::Read(CCFile* _pFile, int _Ver)
{
	MAUTOSTRIP(CXR_BlockNav_BitPlaneContainer_Read, MAUTOSTRIP_VOID);
	switch(_Ver)
	{
	case 0x0200 :
		{
			uint32 nData = 0;
			uint32 nBitPlanes = 0;
			_pFile->ReadLE(nData);
			_pFile->ReadLE(nBitPlanes);
			m_lData.SetLen(nData);
			m_liBitPlaneData.SetLen(nBitPlanes);
			_pFile->ReadLE(m_lData.GetBasePtr(), nData);
			_pFile->ReadLE(m_liBitPlaneData.GetBasePtr(), m_liBitPlaneData.Len());
		};
		break;

	default :
		Error("CXR_BlockNav_BitPlaneContainer::Read", CStrF("Invalid version: %.4x", _Ver));
	}
}

void CXR_BlockNav_BitPlaneContainer::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_BlockNav_BitPlaneContainer_Write, MAUTOSTRIP_VOID);
	uint32 nData = m_lData.Len();
	uint32 nBitPlanes = m_liBitPlaneData.Len();
	_pFile->WriteLE(nData);
	_pFile->WriteLE(nBitPlanes);
	_pFile->WriteLE(m_lData.GetBasePtr(), m_lData.Len());
	_pFile->WriteLE(m_liBitPlaneData.GetBasePtr(), m_liBitPlaneData.Len());
}

// -------------------------------------------------------------------
//  CXR_BlockNav_Tile
// -------------------------------------------------------------------
CXR_BlockNav_Tile::CXR_BlockNav_Tile()
{
	MAUTOSTRIP(CXR_BlockNav_Tile_ctor, MAUTOSTRIP_VOID);
	m_nBitPlanes = 0;
	m_iiBitPlanes = 0;
}

bool CXR_BlockNav_Tile::IsEqual(const CXR_BlockNav_Tile& _Tile) const
{
	MAUTOSTRIP(CXR_BlockNav_Tile_IsEqual, false);
	return
		(m_nBitPlanes == _Tile.m_nBitPlanes) &&
		(m_iiBitPlanes == _Tile.m_iiBitPlanes);
}

void CXR_BlockNav_Tile::Read(CCFile* _pFile, int _Ver)
{
	MAUTOSTRIP(CXR_BlockNav_Tile_Read, MAUTOSTRIP_VOID);
	switch(_Ver)
	{
	case 0x0200 : 
		{
			uint8 temp;
			_pFile->ReadLE(temp);
			m_nBitPlanes = temp;
			_pFile->ReadLE(m_iiBitPlanes);
			break;
		}

	default :
		Error_static("CXR_BlockNav_Tile::Read", CStrF("Invalid version: %.4x", _Ver));
	}


//	LogFile(CStrF("        nBP %d, iBP %d, Flags %d", m_nBitPlanes,  m_iBitPlanes, m_Flags));
}

void CXR_BlockNav_Tile::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_BlockNav_Tile_Write, MAUTOSTRIP_VOID);
	uint8 temp = m_nBitPlanes;
	_pFile->WriteLE(temp);
	_pFile->WriteLE(m_iiBitPlanes);
}


// -------------------------------------------------------------------
//  CXR_BlockNav_TileTile
// -------------------------------------------------------------------
CXR_BlockNav_TileTile::CXR_BlockNav_TileTile()
{
	MAUTOSTRIP(CXR_BlockNav_TileTile_ctor, MAUTOSTRIP_VOID);
}

bool CXR_BlockNav_TileTile::IsEqual(const CXR_BlockNav_TileTile& _TileTile) const
{
	MAUTOSTRIP(CXR_BlockNav_TileTile_IsEqual, false);
	return memcmp(m_liTiles, _TileTile.m_liTiles, sizeof(m_liTiles)) == 0;
}

void CXR_BlockNav_TileTile::Read(CCFile* _pFile, int _Ver)
{
	MAUTOSTRIP(CXR_BlockNav_TileTile_Read, MAUTOSTRIP_VOID);
	switch(_Ver)
	{
	case 0x0200 :
		_pFile->ReadLE(m_liTiles, sizeof(m_liTiles) / sizeof(m_liTiles[0]));
		break;

	default :
		Error_static("CXR_BlockNav_TileTile::Read", CStrF("Invalid version: %.4x", _Ver));
	}
}

void CXR_BlockNav_TileTile::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_BlockNav_TileTile_Write, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_liTiles, sizeof(m_liTiles) / sizeof(m_liTiles[0]));
}

// -------------------------------------------------------------------
//  CXR_BlockNav_GridLayout
// -------------------------------------------------------------------
/*
CXR_BlockNav_GridLayout::CXR_BlockNav_GridLayout()
{
	MAUTOSTRIP(CXR_BlockNav_GridLayout_ctor, MAUTOSTRIP_VOID);
	m_TileTileGridDim = 0;
	m_CellGridDim = 0;
	m_Origin = 0;
	m_UnitsPerCell = 0;
	m_UnitsPerTile = 0;
	m_UnitsPerTileTile = 0;
	m_CellsPerUnit = 0;
	m_TilesPerUnit = 0;
	m_TileTilesPerUnit = 0;
}

void CXR_BlockNav_GridLayout::Create(const CBox3Dfp32& _BoundBox, fp32 _UnitsPerCell)
{
	MAUTOSTRIP(CXR_BlockNav_GridLayout_Create, MAUTOSTRIP_VOID);
	m_UnitsPerCell = _UnitsPerCell;
	m_UnitsPerTile = _UnitsPerCell * XR_NAVTILE_DIM;
	m_UnitsPerTileTile = _UnitsPerCell * XR_NAVTILE_DIM * XR_NAVTILETILE_DIM;
	m_CellsPerUnit = 1.0f / _UnitsPerCell;
	m_TilesPerUnit = 1.0f / m_UnitsPerTile;
	m_TileTilesPerUnit = 1.0f / m_UnitsPerTileTile;

	m_Origin[0] = M_Floor(_BoundBox.m_Min[0] * m_TileTilesPerUnit);
	m_Origin[1] = M_Floor(_BoundBox.m_Min[1] * m_TileTilesPerUnit);
	m_Origin[2] = M_Floor(_BoundBox.m_Min[2] * m_TileTilesPerUnit);
	m_TileTileGridDim[0] = int32(fp32(M_Floor(_BoundBox.m_Max[0] * m_TileTilesPerUnit)) -  fp32(m_Origin[0]));
	m_TileTileGridDim[1] = int32(fp32(M_Floor(_BoundBox.m_Max[1] * m_TileTilesPerUnit)) -  fp32(m_Origin[1]));
	m_TileTileGridDim[2] = int32(fp32(M_Floor(_BoundBox.m_Max[2] * m_TileTilesPerUnit)) -  fp32(m_Origin[2]));
	m_Origin *= m_UnitsPerTileTile;
	m_TileTileGridDim += 1;

	m_CellGridDim = m_TileTileGridDim;
	m_CellGridDim *= XR_NAVTILE_DIM * XR_NAVTILETILE_DIM;

	LogFile("Creating navgrid.");
	LogFile(CStr("        Bound: ") + _BoundBox.GetString());
	LogFile(CStrF("        UnitsPerCell:     %f", m_UnitsPerCell));
	LogFile(CStrF("        UnitsPerTile:     %f", m_UnitsPerTile));
	LogFile(CStrF("        UnitsPerTileTile: %f", m_UnitsPerTileTile));
	LogFile(CStrF("        CellsPerUnit:     %f", m_CellsPerUnit));
	LogFile(CStrF("        TilesPerUnit:     %f", m_TilesPerUnit));
	LogFile(CStrF("        TileTilesPerUnit: %f", m_TileTilesPerUnit));
	LogFile(CStrF("        TileTileGridDim:  %d, %d, %d", m_TileTileGridDim[0], m_TileTileGridDim[1], m_TileTileGridDim[2] ));
	LogFile(CStrF("        CellGridDim:      %d, %d, %d", m_CellGridDim[0], m_CellGridDim[1], m_CellGridDim[2] ));
	LogFile(CStr("        Origin:           ") + m_Origin.GetString());
}

void CXR_BlockNav_GridLayout::Create(const CXR_BlockNav_GridLayout& _Grid)
{
	MAUTOSTRIP(CXR_BlockNav_GridLayout_Create_2, MAUTOSTRIP_VOID);
	m_TileTileGridDim = _Grid.m_TileTileGridDim;
	m_CellGridDim = _Grid.m_CellGridDim;
	m_Origin = _Grid.m_Origin;
	m_UnitsPerCell = _Grid.m_UnitsPerCell;
	m_UnitsPerTile = _Grid.m_UnitsPerTile;
	m_UnitsPerTileTile = _Grid.m_UnitsPerTileTile;
	m_CellsPerUnit = _Grid.m_CellsPerUnit;
	m_TilesPerUnit = _Grid.m_TilesPerUnit;
	m_TileTilesPerUnit = _Grid.m_TileTilesPerUnit;
}*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_BlockNav_Grid
|__________________________________________________________________________________________________
\*************************************************************************************************/

CXR_BlockNav_Grid::CXR_BlockNav_Grid()
{
	MAUTOSTRIP(CXR_BlockNav_Grid_ctor, MAUTOSTRIP_VOID);
	m_AccessCount = 0;
	CDecompressedTile::CreateFillAndTable();
}

CXR_BlockNav_Grid::~CXR_BlockNav_Grid()
{
	MAUTOSTRIP(CXR_BlockNav_Grid_dtor, MAUTOSTRIP_VOID);
}

void CXR_BlockNav_Grid::Create(int _nCachedTiles)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_Create, MAUTOSTRIP_VOID);
	Hash_Clear();
	m_lDecompressedTiles.Clear();
	m_lDecompressedTiles.SetLen(_nCachedTiles);
}

void CXR_BlockNav_Grid::OnPostDecompressTile(CDecompressedTile& _Tile)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_OnPostDecompressTile, MAUTOSTRIP_VOID);
}

int CXR_BlockNav_Grid::GetTileTile(const CVec3Dint32& _TileTilePos) const
{
	MAUTOSTRIP(CXR_BlockNav_Grid_GetTileTile, 0);
	return m_liTileTiles[_TileTilePos[0] + ((_TileTilePos[1] + _TileTilePos[2]*m_TileTileGridDim[1])*m_TileTileGridDim[0])];
}

void CXR_BlockNav_Grid::SetTileTile(const CVec3Dint32& _TileTilePos, int _iTileTile)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_SetTileTile, MAUTOSTRIP_VOID);
	m_liTileTiles[_TileTilePos[0] + ((_TileTilePos[1] + _TileTilePos[2]*m_TileTileGridDim[1])*m_TileTileGridDim[0])] = _iTileTile;
}

int g_GRID_DEBUG = 0;

int CXR_BlockNav_Grid::DecompressTile(int _iTile, const CVec3Dint32& _TilePos)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_DecompressTile, 0);
	MSCOPESHORT(CXR_BlockNav_Grid::DecompressTile);

//	CMTime TimeStart, TimePost;
//	TimeStart.Start();

	// "DT" = "Decompressed Tile"

	int nDT = m_lDecompressedTiles.Len();
	CDecompressedTile* pDT = m_lDecompressedTiles.GetBasePtr();

	// Find the oldest accessed tile in the cache
	int MinAccessCount = 0x7fffffff;
	int iMinAccessCount = -1;

	{
		for(int i = 0; i < nDT; i++)
		{
			if (pDT[i].m_LastAccessCount < MinAccessCount)
			{
				MinAccessCount = pDT[i].m_LastAccessCount;
				iMinAccessCount = i;
			}
		}
	}

	if (iMinAccessCount < 0)
		Error("DecompressTile", "Internal error.");

	int iDT = iMinAccessCount;
	CDecompressedTile& DT = pDT[iDT];

/*ConOut(CStrF("Decompressing tile %d,%d,%d over %d,%d,%d", 
	_TilePos[0], _TilePos[1], _TilePos[2], 
	DT.m_TilePos[0], DT.m_TilePos[1], DT.m_TilePos[2]));*/

	// Remove current cached tile from hash table
	Hash_RemoveDT(iDT);

	// Set new position and reinsert into hash table.
	DT.m_TilePos = _TilePos;
	Hash_InsertDT(iDT);

	// Decompress the tile
#ifdef XR_NAVGRID_BYTECELL
	{
		const CXR_BlockNav_Tile& Tile = m_lTiles[_iTile];
		DT.Clear();
		uint Value = 1;
		for(int i = 0; i < Tile.m_nBitPlanes; i++)
		{
			int iBP = m_liBitPlanes[Tile.m_iiBitPlanes + i];
			DT.Decompress(m_BitPlaneContainer.GetBitPlaneCompressedData(iBP), Value);
			Value += Value;
		}
	}
#else
	{
		const CXR_BlockNav_Tile& Tile = m_lTiles[_iTile];
		DT.m_nBitPlanes = Tile.m_nBitPlanes;
		for(int i = 0; i < Tile.m_nBitPlanes; i++)
		{
			int iBP = m_liBitPlanes[Tile.m_iiBitPlanes + i];
			m_BitPlaneContainer.GetBitPlane(iBP, DT.m_lBitPlanes[i]);

			/*
			if(g_GRID_DEBUG)
			{
				M_TRACEALWAYS("M%d: \r\n", i);
				for(int k = 0; k < 512; k++)
					M_TRACEALWAYS("%02x", DT.m_lBitPlanes[i].m_lBits[k]);
				M_TRACEALWAYS("\r\n");
			}*/
		}
	}
#endif
//	TimePost.Start();
	OnPostDecompressTile(DT);
//	TimePost.Stop();
//	TimeStart.Stop();

//	M_TRACEALWAYS("(CXR_BlockNav_Grid::DecompressTile) Time %f us, PostDecompress %f us\n", TimeStart.GetTime() * 1000000.0f, TimePost.GetTime() * 1000000.0f);

	return iDT;
}

void CXR_BlockNav_Grid::RewindAccessCounters()
{
	MAUTOSTRIP(CXR_BlockNav_Grid_RewindAccessCounters, MAUTOSTRIP_VOID);
	// Rewind all access counters by 0x10000000, clamp to zero.
	// This will happen once every 1073676288th access. (0x3fff0000)

	if (m_AccessCount > 0x40000000)
		m_AccessCount -= 0x40000000;
	else
		m_AccessCount = 0;

	for(int i = 0; i < m_lDecompressedTiles.Len(); i++)
	{
		if (m_lDecompressedTiles[i].m_LastAccessCount > 0x40000000)
			m_lDecompressedTiles[i].m_LastAccessCount -= 0x40000000;
		else
			m_lDecompressedTiles[i].m_LastAccessCount = 0;
	}
}

uint CXR_BlockNav_Grid::GetAt(const CVec3Dint32& _Pos)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_GetAt, 0);
	MSCOPESHORT(CXR_BlockNav_Grid::GetAt);
 
	CVec3Dint32 Pos(_Pos);
	if (Pos[0] < 0) Pos[0] = 0;
	if (Pos[1] < 0) Pos[1] = 0;
	if (Pos[2] < 0) Pos[2] = 0;
	if (Pos[0] >= m_CellGridDim[0]) Pos[0] = m_CellGridDim[0]-1;
	if (Pos[1] >= m_CellGridDim[1]) Pos[1] = m_CellGridDim[1]-1;
	if (Pos[2] >= m_CellGridDim[2]) Pos[2] = m_CellGridDim[2]-1;
	CVec3Dint32 TilePos(
		Pos[0] >> XR_NAVTILE_DIMSHIFT, 
		Pos[1] >> XR_NAVTILE_DIMSHIFT, 
		Pos[2] >> XR_NAVTILE_DIMSHIFT);

	int iDT = Hash_GetDT(TilePos);
	if (iDT < 0 || g_GRID_DEBUG)
	{
		const int TileTileShift = XR_NAVTILETILE_DIMSHIFT + XR_NAVTILE_DIMSHIFT;
		CVec3Dint32 TileTilePos(Pos[0] >> TileTileShift, Pos[1] >> TileTileShift, Pos[2] >> TileTileShift);
		CVec3Dint32 TilePosWrapped(
			(Pos[0] >> XR_NAVTILE_DIMSHIFT) & XR_NAVTILETILE_DIMAND, 
			(Pos[1] >> XR_NAVTILE_DIMSHIFT) & XR_NAVTILETILE_DIMAND, 
			(Pos[2] >> XR_NAVTILE_DIMSHIFT) & XR_NAVTILETILE_DIMAND);

		int iTileTile = GetTileTile(TileTilePos);
		const CXR_BlockNav_TileTile& TileTile = m_lTileTiles[iTileTile];
		int iTile = TileTile.GetTile(TilePosWrapped);
		iDT = DecompressTile(iTile, TilePos);
	}

	CDecompressedTile& DT = m_lDecompressedTiles[iDT];
	DT.m_LastAccessCount = m_AccessCount++;
	if (m_AccessCount > 0x7fff0000)
		RewindAccessCounters();
#ifdef XR_NAVGRID_BYTECELL
	uint ret = DT.GetAt(Pos[0] & XR_NAVTILE_DIMAND, Pos[1] & XR_NAVTILE_DIMAND, Pos[2] & XR_NAVTILE_DIMAND);

#else
	CVec3Dint32 Cell(Pos[0] & XR_NAVTILE_DIMAND, Pos[1] & XR_NAVTILE_DIMAND, Pos[2] & XR_NAVTILE_DIMAND);
	uint ret = CXR_BlockNav_BitPlane::GetAt(Cell, DT.m_lBitPlanes, DT.m_nBitPlanes);

#endif
	/*if(g_GRID_DEBUG)
	{
		g_GRID_DEBUG++;
		M_TRACEALWAYS("MPU_GetAt: %05d: (%d,%d,%d) = %d\r\n", g_GRID_DEBUG-1, _Pos[0], _Pos[1], _Pos[2], ret);
	}*/
	return ret;
}

/*
CVec3Dfp32 CXR_BlockNav_Grid::GetCellPosition(const CVec3Dint32& _Pos)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_GetCellPosition, CVec3Dfp32());
	return CVec3Dfp32(
		fp32(_Pos[0]) * m_UnitsPerCell,
		fp32(_Pos[1]) * m_UnitsPerCell,
		fp32(_Pos[2]) * m_UnitsPerCell);
}*/

int CXR_BlockNav_Grid::ReadStruct(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_ReadStruct, 0);
	uint16 Ver = 0;
	_pFile->ReadLE(Ver);
	switch(Ver)
	{
	case 0x0200 : 
		{
			m_TileTileGridDim.Read(_pFile);
			m_CellGridDim.Read(_pFile);
			m_Origin.Read(_pFile);
			_pFile->ReadLE(m_UnitsPerCell);
			_pFile->ReadLE(m_UnitsPerTile);
			_pFile->ReadLE(m_UnitsPerTileTile);
			_pFile->ReadLE(m_CellsPerUnit);
			_pFile->ReadLE(m_TilesPerUnit);
			_pFile->ReadLE(m_TileTilesPerUnit);
			break;
		}

	default :
		Error("CXR_BlockNav_Grid::Read", CStrF("Invalid version: %.4x", Ver));
	}

	return Ver;
}

void CXR_BlockNav_Grid::WriteStruct(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_WriteStruct, MAUTOSTRIP_VOID);
	uint16 Ver = XR_NAVGRID_VERSION;
	_pFile->WriteLE(Ver);

	m_TileTileGridDim.Write(_pFile);
	m_CellGridDim.Write(_pFile);
	m_Origin.Write(_pFile);
	_pFile->WriteLE(m_UnitsPerCell);
	_pFile->WriteLE(m_UnitsPerTile);
	_pFile->WriteLE(m_UnitsPerTileTile);
	_pFile->WriteLE(m_CellsPerUnit);
	_pFile->WriteLE(m_TilesPerUnit);
	_pFile->WriteLE(m_TileTilesPerUnit);
}

void CXR_BlockNav_Grid::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_Read, MAUTOSTRIP_VOID);
	if (!_pDFile->GetNext("GRIDDESC")) Error("Read", "No GRIDDESC entry.");
	ReadStruct(_pDFile->GetFile());

	// CODEBOOK
	{
		if (!_pDFile->GetNext("CODEBOOK")) Error("Read", "No CODEBOOK entry.");
		m_liTileTiles.SetLen(_pDFile->GetUserData()); 
		if (_pDFile->GetUserData2() == 0x0200)
			_pDFile->GetFile()->ReadLE(m_liTileTiles.GetBasePtr(), m_liTileTiles.Len());
		else	
			Error("Read", CStrF("Unsupported codebook version %.4x", _pDFile->GetUserData2()));
	}

	// COMPRESSEDBITPLANES
	{
		if (!_pDFile->GetNext("COMPRESSEDBITPLANES")) Error("Read", "No BITPLANES entry.");
		m_BitPlaneContainer.Read(_pDFile->GetFile(), _pDFile->GetUserData2());
	}

	// BITPLANEINDICES
	{
		if (!_pDFile->GetNext("BITPLANEINDICES")) Error("Read", "No BITPLANEINDICES entry.");
		m_liBitPlanes.SetLen(_pDFile->GetUserData());
		if (_pDFile->GetUserData2() == 0x0200)
			_pDFile->GetFile()->ReadLE(m_liBitPlanes.GetBasePtr(), m_liBitPlanes.Len());
		else
			Error("Read", CStrF("Unsupported bitplane index version %.4x", _pDFile->GetUserData2()));
	}

	// TILES
	{
		if (!_pDFile->GetNext("TILES")) Error("Read", "No TILES entry.");

		m_lTiles.SetLen(_pDFile->GetUserData()); 
		for(int i = 0; i < m_lTiles.Len(); i++)
			m_lTiles[i].Read(_pDFile->GetFile(), _pDFile->GetUserData2());
	}

	// TILETILES
	{
		if (!_pDFile->GetNext("TILETILES")) Error("Read", "No TILETILES entry.");

		m_lTileTiles.SetLen(_pDFile->GetUserData()); 
		for(int i = 0; i < m_lTileTiles.Len(); i++)
			m_lTileTiles[i].Read(_pDFile->GetFile(), _pDFile->GetUserData2());
	}
}

void CXR_BlockNav_Grid::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_Write, MAUTOSTRIP_VOID);
	_pDFile->BeginEntry("GRIDDESC");
	WriteStruct(_pDFile->GetFile());
	_pDFile->EndEntry(XR_NAVGRID_VERSION);

	// CODEBOOK
	{
		_pDFile->BeginEntry("CODEBOOK");
		_pDFile->GetFile()->WriteLE(m_liTileTiles.GetBasePtr(), m_liTileTiles.Len());
		_pDFile->EndEntry(m_liTileTiles.Len(), XR_NAVCODEBOOK_VERSION);
	}

	// COMPRESSEDBITPLANES
	{
		_pDFile->BeginEntry("COMPRESSEDBITPLANES");
		m_BitPlaneContainer.Write(_pDFile->GetFile());
		_pDFile->EndEntry(m_BitPlaneContainer.m_liBitPlaneData.Len(), XR_NAVBITPLANE_VERSION);
	}

	// BITPLANEINDICES
	{
		_pDFile->BeginEntry("BITPLANEINDICES");
		_pDFile->GetFile()->WriteLE(m_liBitPlanes.GetBasePtr(), m_liBitPlanes.Len());
		_pDFile->EndEntry(m_liBitPlanes.Len(), XR_NAVBITPLANEINDEX_VERSION);
	}

	// TILES
	{
		_pDFile->BeginEntry("TILES");
		for(int i = 0; i < m_lTiles.Len(); i++)
			m_lTiles[i].Write(_pDFile->GetFile());
		_pDFile->EndEntry(m_lTiles.Len(), XR_NAVTILE_VERSION);
	}

	// TILES
	{
		_pDFile->BeginEntry("TILETILES");
		for(int i = 0; i < m_lTileTiles.Len(); i++)
			m_lTileTiles[i].Write(_pDFile->GetFile());
		_pDFile->EndEntry(m_lTileTiles.Len(), XR_NAVTILETILE_VERSION);
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_BlockNav_GridBuilder
|__________________________________________________________________________________________________
\*************************************************************************************************/

CXR_BlockNav_GridBuilder::CXR_BlockNav_GridBuilder()
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_ctor, MAUTOSTRIP_VOID);
}

void CXR_BlockNav_GridBuilder::InitFromGridLayout()
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_InitFromGridLayout, MAUTOSTRIP_VOID);
	m_TileGridDim = m_TileTileGridDim;
	m_TileGridDim *= XR_NAVTILETILE_DIM;

	m_liTiles.SetLen(m_TileGridDim[0] * m_TileGridDim[1] * m_TileGridDim[2]);
	FillChar(m_liTiles.GetBasePtr(), m_liTiles.ListSize(), 0);

	m_lBitPlanes.Clear();
	m_lTiles.Clear();

	m_lBitPlanes.SetGrow(4096);
	m_liBitPlanes.SetGrow(16384);
	m_lTiles.SetGrow(4096);
	m_lTileTiles.SetGrow(4096);

	// Create zero and one bitplanes.
	CXR_BlockNav_BitPlane BitPlane;
	BitPlane.Clear(0);
	AddBitPlane(BitPlane);
	BitPlane.Clear(1);
	AddBitPlane(BitPlane);

	// Create zero and one tiles
	CXR_BlockNav_Tile TileZero;
	TileZero.m_nBitPlanes = 1;
	TileZero.m_iiBitPlanes = m_liBitPlanes.Add((uint16)0);
	AddTile(TileZero);

	CXR_BlockNav_Tile TileOne;
	TileOne.m_nBitPlanes = 1;
	TileOne.m_iiBitPlanes = m_liBitPlanes.Add(1);
	AddTile(TileOne);

	CXR_BlockNav_Tile TileDie;
	TileDie.m_nBitPlanes = 4;
	TileDie.m_iiBitPlanes = m_liBitPlanes.Len();
	m_liBitPlanes.Add(1);
	m_liBitPlanes.Add((uint16)0);
	m_liBitPlanes.Add((uint16)0);
	m_liBitPlanes.Add(1);
	AddTile(TileDie);
}

void CXR_BlockNav_GridBuilder::Create(const CBox3Dfp32& _BoundBox, fp32 _UnitsPerCell)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_Create, MAUTOSTRIP_VOID);

	CXR_BlockNav_GridLayout::Create(_BoundBox, _UnitsPerCell);
	InitFromGridLayout();
}

void CXR_BlockNav_GridBuilder::Create(const CXR_BlockNav_GridLayout& _Grid)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_Create_2, MAUTOSTRIP_VOID);
	CXR_BlockNav_GridLayout::Create(_Grid);
	InitFromGridLayout();
}

int CXR_BlockNav_GridBuilder::AddTileTile(const CXR_BlockNav_TileTile& _TT)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_AddTileTile, 0);
	for(int iTT = 0; iTT < m_lTileTiles.Len(); iTT++)
		if (m_lTileTiles[iTT].IsEqual(_TT))
			return iTT;

	return m_lTileTiles.Add(_TT);
}

void CXR_BlockNav_GridBuilder::CompileTileTiles()
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_CompileTileTiles, MAUTOSTRIP_VOID);
	m_liTileTiles.SetLen(m_TileTileGridDim[0] * m_TileTileGridDim[1] * m_TileTileGridDim[2]);

	for(int x = 0; x < m_TileTileGridDim[0]; x++)
		for(int y = 0; y < m_TileTileGridDim[1]; y++)
			for(int z = 0; z < m_TileTileGridDim[2]; z++)
			{
				CXR_BlockNav_TileTile TT;
				for(int xt = 0; xt < XR_NAVTILETILE_DIM; xt++)
					for(int yt = 0; yt < XR_NAVTILETILE_DIM; yt++)
						for(int zt = 0; zt < XR_NAVTILETILE_DIM; zt++)
						{
							CVec3Dint32 TilePos(
								x*XR_NAVTILETILE_DIM + xt,
								y*XR_NAVTILETILE_DIM + yt,
								z*XR_NAVTILETILE_DIM + zt);

							TT.SetTile(CVec3Dint32(xt,yt,zt), GetTile(TilePos));
						}

				int iTileTile = AddTileTile(TT);
				SetTileTile(CVec3Dint32(x,y,z), iTileTile);
			}

	m_BitPlaneContainer.m_lData.Clear();
	m_BitPlaneContainer.m_liBitPlaneData.Clear();

	m_BitPlaneContainer.m_lData.SetGrow(2*1024*1024);
	m_BitPlaneContainer.m_liBitPlaneData.SetGrow(1024*256);

	for(int i = 0; i < m_lBitPlanes.Len(); i++)
		m_BitPlaneContainer.AddBitPlane(m_lBitPlanes[i]);
}

int CXR_BlockNav_GridBuilder::AddBitPlane(const CXR_BlockNav_BitPlane& _BitPlane)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_AddBitPlane, 0);
	for(int i = 0; i < m_lBitPlanes.Len(); i++)
		if (_BitPlane.IsEqual(m_lBitPlanes[i]))
			return i;

	return m_lBitPlanes.Add(_BitPlane);
}

int CXR_BlockNav_GridBuilder::AddTile(const CXR_BlockNav_Tile& _Tile)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_AddTile, 0);
	for(int i = 0; i < m_lTiles.Len(); i++)
		if (_Tile.IsEqual(m_lTiles[i])) return i;

	return m_lTiles.Add(_Tile);
}

int CXR_BlockNav_GridBuilder::GetTile(const CVec3Dint32& _TilePos)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_GetTile, 0);
	return m_liTiles[_TilePos[0] + ((_TilePos[1] + _TilePos[2]*m_TileGridDim[1])*m_TileGridDim[0])];
}

void CXR_BlockNav_GridBuilder::SetTile(const CVec3Dint32& _TilePos, int _iTile)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_SetTile, MAUTOSTRIP_VOID);
	m_liTiles[_TilePos[0] + ((_TilePos[1] + _TilePos[2]*m_TileGridDim[1])*m_TileGridDim[0])] = _iTile;
}

int CXR_BlockNav_GridBuilder::SetTile(const CVec3Dint32& _TilePos, const CXR_BlockNav_BitPlane* _pBitPlanes, int _nBitPlanes)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_SetTile_2, 0);
	// Get number of used bitplanes
	int iPlane = _nBitPlanes-1;
	while(iPlane > 1 && _pBitPlanes[iPlane].IsZero()) iPlane--;
	int nPlanes = iPlane+1;

	// Look for identical tile.
	for(int t = 0; t < m_lTiles.Len(); t++)
	{
		const CXR_BlockNav_Tile& Tile = m_lTiles[t];
		if (Tile.m_nBitPlanes == nPlanes)
		{
			int i = 0;
			for(; i < nPlanes; i++)
				if (!m_lBitPlanes[m_liBitPlanes[Tile.m_iiBitPlanes + i]].IsEqual(_pBitPlanes[i]))
					break;

			// Found match
			if (i == nPlanes)
			{
				SetTile(_TilePos, t);
				return t;
			}
		}
	}

	// Add bitplanes & tile to new grid
	CXR_BlockNav_Tile Tile;
	Tile.m_iiBitPlanes = m_liBitPlanes.Len();
	Tile.m_nBitPlanes = nPlanes;
	for(int p = 0; p < nPlanes; p++)
		m_liBitPlanes.Add(AddBitPlane(_pBitPlanes[p]));

	int iTile = AddTile(Tile);
	SetTile(_TilePos, iTile);
	return iTile;
}


int CXR_BlockNav_GridBuilder::GetTile(const CVec3Dint32& _TilePos, CXR_BlockNav_Tile& _Tile, CXR_BlockNav_BitPlane* _pBitPlanes, int _MaxBitPlanes)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_GetTile_2, 0);
	CVec3Dint32 TilePos(
		MinMT(MaxMT(0, _TilePos[0]), m_TileGridDim[0]-1),
		MinMT(MaxMT(0, _TilePos[1]), m_TileGridDim[1]-1),
		MinMT(MaxMT(0, _TilePos[2]), m_TileGridDim[2]-1));

	const CXR_BlockNav_Tile& Tile = m_lTiles[GetTile(TilePos)];
	_Tile = Tile;
	int nBitPlanes = MinMT(Tile.m_nBitPlanes, _MaxBitPlanes);
	for(int i = 0; i < nBitPlanes; i++)
		_pBitPlanes[i] = m_lBitPlanes[m_liBitPlanes[Tile.m_iiBitPlanes + i]];

	return nBitPlanes;
}

CBox3Dint CXR_BlockNav_GridBuilder::GetTileDomain(const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_GetTileDomain, CBox3Dint());
	CBox3Dint Box;

	CVec3Dfp32 VMin(_Box.m_Min);
	CVec3Dfp32 VMax(_Box.m_Max);
	VMin -= m_Origin;
	VMax -= m_Origin;

	Box.m_Min[0] = int(M_Floor(VMin[0] * m_TilesPerUnit));
	Box.m_Min[1] = int(M_Floor(VMin[1] * m_TilesPerUnit));
	Box.m_Min[2] = int(M_Floor(VMin[2] * m_TilesPerUnit));
	Box.m_Max[0] = int(M_Floor(VMax[0] * m_TilesPerUnit));
	Box.m_Max[1] = int(M_Floor(VMax[1] * m_TilesPerUnit));
	Box.m_Max[2] = int(M_Floor(VMax[2] * m_TilesPerUnit));
	
	Box.m_Min[0] = MaxMT(0, Box.m_Min[0]);
	Box.m_Min[1] = MaxMT(0, Box.m_Min[1]);
	Box.m_Min[2] = MaxMT(0, Box.m_Min[2]);
	Box.m_Max[0] = int(MinMT(m_TileGridDim[0] - 1.0f, Box.m_Max[0]));
	Box.m_Max[1] = int(MinMT(m_TileGridDim[1] - 1.0f, Box.m_Max[1]));
	Box.m_Max[2] = int(MinMT(m_TileGridDim[2] - 1.0f, Box.m_Max[2]));

	return Box;
} 
 
CBox3Dfp32 CXR_BlockNav_GridBuilder::GetTileExtents(const CVec3Dint32& _Pos)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_GetTileExtents, CBox3Dfp32());
	CBox3Dfp32 Box;
	Box.m_Min[0] = fp32(_Pos[0]) * m_UnitsPerTile + m_Origin[0];
	Box.m_Min[1] = fp32(_Pos[1]) * m_UnitsPerTile + m_Origin[1];
	Box.m_Min[2] = fp32(_Pos[2]) * m_UnitsPerTile + m_Origin[2];
	Box.m_Max[0] = fp32(_Pos[0]+1) * m_UnitsPerTile + m_Origin[0];
	Box.m_Max[1] = fp32(_Pos[1]+1) * m_UnitsPerTile + m_Origin[1];
	Box.m_Max[2] = fp32(_Pos[2]+1) * m_UnitsPerTile + m_Origin[2];
	return Box;
}

CVec3Dint32 CXR_BlockNav_GridBuilder::GetTileCellBase(const CVec3Dint32& _Pos)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_GetTileCellBase, CVec3Dint32(0,0,0));
	return CVec3Dint32(_Pos[0] * XR_NAVTILE_DIM, _Pos[1] * XR_NAVTILE_DIM, _Pos[2] * XR_NAVTILE_DIM);
}

CVec3Dfp32 CXR_BlockNav_GridBuilder::GetCellExtents()
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_GetCellExtents, CVec3Dfp32());
	return CVec3Dfp32(m_UnitsPerCell);
}

void CXR_BlockNav_GridBuilder::GetPosTileCellClamped(const CVec3Dint32& _Pos, CVec3Dint32& _Tile, CVec3Dint32& _Cell)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_GetPosTileCellClamped, MAUTOSTRIP_VOID);
	CVec3Dint32 Pos(_Pos);
	if (Pos[0] < 0) Pos[0] = 0;
	if (Pos[1] < 0) Pos[1] = 0;
	if (Pos[2] < 0) Pos[2] = 0;
	if (Pos[0] >= m_CellGridDim[0]) Pos[0] = m_CellGridDim[0]-1;
	if (Pos[1] >= m_CellGridDim[1]) Pos[1] = m_CellGridDim[1]-1;
	if (Pos[2] >= m_CellGridDim[2]) Pos[2] = m_CellGridDim[2]-1;
	CVec3Dint32 Tile(Pos[0] >> XR_NAVTILE_DIMSHIFT, Pos[1] >> XR_NAVTILE_DIMSHIFT, Pos[2] >> XR_NAVTILE_DIMSHIFT);
	CVec3Dint32 Cell(Pos[0] & XR_NAVTILE_DIMAND, Pos[1] & XR_NAVTILE_DIMAND, Pos[2] & XR_NAVTILE_DIMAND);

	_Tile = Tile;
	_Cell = Cell;
}


uint CXR_BlockNav_GridBuilder::GetAt(const CVec3Dint32& _Pos)
{
	MAUTOSTRIP(CXR_BlockNav_GridBuilder_GetAt, 0);
	CVec3Dint32 TilePos, CellPos;
	GetPosTileCellClamped(_Pos, TilePos, CellPos);
	const CXR_BlockNav_Tile& Tile = m_lTiles[GetTile(TilePos)];

	return CXR_BlockNav_BitPlane::GetAt(CellPos, m_lBitPlanes.GetBasePtr(), Tile.m_nBitPlanes, &m_liBitPlanes[Tile.m_iiBitPlanes]);
}

