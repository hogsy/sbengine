
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		010209:		Created File
		030505:		Upated Comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MDA3D
#define _INC_MDA3D

#include "MDA.h"
#include "MFile.h"
#include "MMath.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Short_desscription
						
	Comments:			Longer_description_not_mandatory
\*____________________________________________________________________*/

class CHash3DLink
{
public:
	void* pNextH3D;
	void* pPrevH3D;
	CVec3Dfp4 OldPosH3D;

	CHash3DLink()
	{
		pNextH3D = NULL;
		pPrevH3D = NULL;
		OldPosH3D = 0;
	};
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		Short_desscription
						
	Parameters:			
		_param1:		description
						
	Comments:			Longer_description_not_mandatory
\*____________________________________________________________________*/

template<class T>
class THash3D : public CReferenceCount
{
	typedef TList_Vector<T*> QueryBuffer;

	class Size3D
	{
	public:
		int k[3];
	};
	Size3D nBox;
	Size3D nBoxShift;
	Size3D nBoxAnd;
	Size3D BoxSizeShift;
	Size3D BoxSizeAnd;
	Size3D BoxSize;

	T** pHash;

	int GetXBox(fp4 _x) { return (((int) _x) >> BoxSizeShift.k[0]) & nBoxAnd.k[0]; };
	int GetYBox(fp4 _y) { return (((int) _y) >> BoxSizeShift.k[1]) & nBoxAnd.k[1]; };
	int GetZBox(fp4 _z) { return (((int) _z) >> BoxSizeShift.k[2]) & nBoxAnd.k[2]; };

	void GetXBoxRange(fp4 _x0, fp4 _x1, int& x0, int& x1) 
	{ 
		x0 = ((int) _x0) >> BoxSizeShift.k[0];
		x1 = ((int) _x1) >> BoxSizeShift.k[0];
		if ((x1 - x0) >= nBox.k[0])
		{
			x0 = 0; 
			x1 = nBoxAnd.k[0];
		}
		else
		{
			x0 &= nBoxAnd.k[0];
			x1 &= nBoxAnd.k[0];
		};
	};

	void GetYBoxRange(fp4 _y0, fp4 _y1, int& y0, int& y1) 
	{ 
		y0 = ((int) _y0) >> BoxSizeShift.k[1];
		y1 = ((int) _y1) >> BoxSizeShift.k[1];
		if ((y1 - y0) >= nBox.k[1])
		{
			y0 = 0; 
			y1 = nBoxAnd.k[1];
		}
		else
		{
			y0 &= nBoxAnd.k[1];
			y1 &= nBoxAnd.k[1];
		};
	};

	void GetZBoxRange(fp4 _z0, fp4 _z1, int& z0, int& z1) 
	{ 
		z0 = ((int) _z0) >> BoxSizeShift.k[2];
		z1 = ((int) _z1) >> BoxSizeShift.k[2];
		if ((z1 - z0) >= nBox.k[2])
		{
			z0 = 0; 
			z1 = nBoxAnd.k[2];
		}
		else
		{
			z0 &= nBoxAnd.k[2];
			z1 &= nBoxAnd.k[2];
		};
	};

	int CalcHashIndex(CVec3Dfp4& pos)
	{
		int x = GetXBox(pos.k[0]);
		int y = GetYBox(pos.k[1]);
		int z = GetZBox(pos.k[2]);
		return (x + (y << nBoxShift.k[0]) + (z << (nBoxShift.k[0] + nBoxShift.k[1])));
	};

public:
	THash3D(CVec3Dfp4 _nBoxShift, CVec3Dfp4 _BoxSizeShift)
	{
		nBoxShift.k[0] = _nBoxShift.k[0];
		nBoxShift.k[1] = _nBoxShift.k[1];
		nBoxShift.k[2] = _nBoxShift.k[2];
		BoxSizeShift.k[0] = _BoxSizeShift.k[0];
		BoxSizeShift.k[1] = _BoxSizeShift.k[1];
		BoxSizeShift.k[2] = _BoxSizeShift.k[2];
		pHash = NULL;
		BoxSize.k[0] = 1 << BoxSizeShift.k[0];
		BoxSize.k[1] = 1 << BoxSizeShift.k[1];
		BoxSize.k[2] = 1 << BoxSizeShift.k[2];
		BoxSizeAnd.k[0] = (1 << BoxSizeShift.k[0])-1;
		BoxSizeAnd.k[1] = (1 << BoxSizeShift.k[1])-1;
		BoxSizeAnd.k[2] = (1 << BoxSizeShift.k[2])-1;
		nBox.k[0] = 1 << nBoxShift.k[0];
		nBox.k[1] = 1 << nBoxShift.k[1];
		nBox.k[2] = 1 << nBoxShift.k[2];
		nBoxAnd.k[0] = (1 << nBoxShift.k[0])-1;
		nBoxAnd.k[1] = (1 << nBoxShift.k[1])-1;
		nBoxAnd.k[2] = (1 << nBoxShift.k[2])-1;

		int n = nBox.k[0] * nBox.k[1] * nBox.k[2];
		pHash = DNew(T*) T*[n];
		if (pHash == NULL) MemError("-");
		FillChar(pHash, n*sizeof(T*), 0);
	};

	~THash3D()
	{
		delete[] pHash;
		pHash = NULL;
	};

	void Insert(T* _p, CVec3Dfp4 pos)
	{
		int index = CalcHashIndex(pos);
		if (pHash[index] != NULL)
			pHash[index]->pPrevH3D = _p;
		_p->pNextH3D = pHash[index];
		_p->pPrevH3D = NULL;
		pHash[index] = _p;
		_p->OldPosH3D = pos;
	};

	void Insert(T* _p, const CMat4Dfp4& mpos)
	{
		CVec3Dfp4 pos;
		pos.GetMatrixRow(3, mpos);
		Insert(_p, pos);
	};

#ifndef DEFINE_MAT43_IS_MAT4D
	void Insert(T* _p, const CMat43fp4& mpos)
	{
		CVec3Dfp4 pos;
		pos.GetMatrixRow(3, mpos);
		Insert(_p, pos);
	}
#endif

	void Remove(T* _p)
	{
		int index = CalcHashIndex(_p->OldPosH3D);

		// Fel koll.
		if (((pHash[index] != _p) && (_p->pPrevH3D == NULL)) ||
			((pHash[index] == _p) && (_p->pPrevH3D != NULL)))
		{
			Error("Remove", "Inconsistent linking.");
			_p->pPrevH3D = NULL;
			_p->pNextH3D = NULL;
			return;
		};

		if (_p->pPrevH3D != NULL)
			((T*)_p->pPrevH3D)->pNextH3D = _p->pNextH3D;
		else
			pHash[index] = (T*) _p->pNextH3D;

		if (_p->pNextH3D != NULL)
			((T*)_p->pNextH3D)->pPrevH3D = _p->pPrevH3D;

		_p->pPrevH3D = NULL;
		_p->pNextH3D = NULL;
	};

	void Move(T* _p, CVec3Dfp4 newpos)
	{
		Remove(_p);
		Insert(_p, newpos);
	};

	void Move(T* _p, const CMat4Dfp4& newmpos)
	{
		Remove(_p);
		Insert(_p, newmpos);
	};

#ifndef DEFINE_MAT43_IS_MAT4D
	void Move(T* _p, const CMat43fp4& newmpos)
	{
		Remove(_p);
		Insert(_p, newmpos);
	}
#endif

	void ExtractWithinBox(const CVec3Dfp4& minp, const CVec3Dfp4& maxp, QueryBuffer* pBuffer)
	{
		int x0, x1, y0, y1, z0, z1;
		GetXBoxRange(minp.k[0], maxp.k[0], x0, x1);
		GetYBoxRange(minp.k[1], maxp.k[1], y0, y1);
		GetZBoxRange(minp.k[2], maxp.k[2], z0, z1);

		pBuffer->SetLen(0);
		int xl = -1;
		for (int x = x0; xl != x1; x = (x+1) & nBoxAnd.k[0])
		{
			int yl = -1;
			for (int y = y0; yl != y1; y = (y+1) & nBoxAnd.k[1])
			{
				int zl = -1;
				for (int z = z0; zl != z1; z = (z+1) & nBoxAnd.k[2])
				{
					int index = (x + (y << nBoxShift.k[0]) + (z << (nBoxShift.k[0] + nBoxShift.k[1])));
					T* _p = pHash[index];
					while (_p != NULL)
					{
						pBuffer->Add(_p);
						_p = (T*) _p->pNextH3D;
					};
					zl = z;
				};
				yl = y;
			};
			xl = x;
		};
	};

	void ExtractWithinSphere(const CVec3Dfp4& Pos, fp4 Radius, QueryBuffer* pBuffer)
	{
		CVec3Dfp4 minp(Pos);
		CVec3Dfp4 maxp(Pos);
		minp.k[0] -= Radius;
		minp.k[1] -= Radius;
		minp.k[2] -= Radius;
		maxp.k[0] += Radius;
		maxp.k[1] += Radius;
		maxp.k[2] += Radius;
		ExtractWithinBox(minp, maxp, pBuffer);
	};
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Short_desscription
						
	Comments:			Longer_description_not_mandatory
\*____________________________________________________________________*/

class CSE_IDInfo
{
public:
	int m_IsInserted;
	CBox3Dint m_Box;

	CSE_IDInfo()
	{
		m_IsInserted = 0;
	}
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Short_desscription
						
	Comments:			Longer_description_not_mandatory
\*____________________________________________________________________*/

class CSE_IDLink
{
public:
	int16 m_ID;
	int16 m_iPrevID;
	int16 m_iNextID;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Short_desscription
						
	Comments:			Longer_description_not_mandatory
\*____________________________________________________________________*/

class CSE_Node
{
public:
	int16 m_iFirstID;
	int16 m_iParent;
	int32 m_iChildMask;
	int16 m_iChildren[8];

	CSE_Node()
	{
		m_iFirstID = -1;
		m_iParent = -1;
		m_iChildMask = 0;
		for(int i = 0; i < 8; m_iChildren[i++] = -1) {};
	}
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Short_desscription
						
	Comments:			Longer_description_not_mandatory
\*____________________________________________________________________*/

class MCCDLLEXPORT CSpaceEnum_OctTree : public CReferenceCount
{
	TList_Vector<CSE_Node> m_lNodes;
	TList_Vector<CSE_IDInfo> m_lIDInfo;
	TList_Vector<CSE_IDLink> m_lIDLinks;

	CSE_Node* m_pNodes;
	CSE_IDLink* m_pIDLinks;

	int m_MaxSize;
	int m_MinSize;
	int m_iRoot;
	int m_MaxLevel;

	TPtr<CIDHeap> m_spNodeHeap;
	TPtr<CIDHeap> m_spIDLinkHeap;
	CBox3Dint m_RootBox;

	void CreateSubBox(int _Pos, const CBox3Dint& _Box, CBox3Dint& _SubBox);
	void CreateSubNode(int _iNode, int _iChild);
	void RemoveEmptyChildren(CSE_Node* _pNode);
	int InsertAtNode(int _ID, int _iNode, const CBox3Dint& _Box, const CBox3Dint& _NodeBox, int _Level);
	int ReInsertAtNode(int _ID, int _iNode, const CBox3Dint& _Box, const CBox3Dint& _OldBox, const CBox3Dint& _NodeBox, int _Level);
	void RemoveAtNode(int _ID, int _iNode, const CBox3Dint& _Box, const CBox3Dint& _NodeBox, int _Level);

public:
	CSpaceEnum_OctTree(int _BoxSizeShift, int _MinBoxSizeShift, int _nMaxNodes, int _nMaxIDLinks, int _nIDs);

	int Insert(int _ID, const CVec3Dfp4& _Min, const CVec3Dfp4& _Max);
	void Remove(int _ID);
	void EnumerateBox(CVec3Dfp4 _Min, CVec3Dfp4 _Max, int(*pfnEnumObjectsCallback)(int _ID));

	// IO for rendering the OctTree.
	const CSE_Node* GetRootPtr() const;
	CBox3Dint GetRootBox() const;
};

#endif // _INC_MDA3D
