
#ifndef __MDA_HASH2D_INCLUDED_
#define __MDA_HASH2D_INCLUDED_

#include "MDA_Hash.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| THash2D
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CHash2DNull
{
public:
	int m_Hirr;
	CHash2DNull() {};
	~CHash2DNull() {};
};

class MCCDLLEXPORT CHash2D : public THash<int32, CHash2DNull>
{
protected:
	int m_BoxSize;
	int m_BoxShiftSize;
	int m_BoxAndSize;
	int m_nBoxAndX;
	int m_nBoxAndY;
	int m_nBoxesX;
	int m_nBoxesY;
	int m_nBoxes;
	int m_HashAndSizeX;
	int m_HashAndSizeY;

public:
	CHash2D();
	~CHash2D();
	void Create(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge);
	
	void Insert(int _ID, const CVec3Dfp4& _Min, const CVec3Dfp4& _Max);
	int EnumerateBox(const CVec3Dfp4& _Min, const CVec3Dfp4& _Max, int32* _pEnumRetIDs, int _MaxEnumIDs);
};

#endif // __MDA_HASH2D_INCLUDED_
