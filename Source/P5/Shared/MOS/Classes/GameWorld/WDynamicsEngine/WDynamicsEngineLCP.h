
#ifndef WDYNAMICSENGINELCP_H
#define WDYNAMICSENGINELCP_H

#include "WDynamicsEngine2.h"
#include "WDynamicsEngineUtil.h"

class CVector
{
public:
	CVector(fp32 *_pVector, int _Len)
	{
		m_pVector = _pVector;
		m_Len = _Len; 
	}

	const fp32 *GetP(int _i) const
	{
		return &m_pVector[_i];
	}

	void Set(int _i, fp32 _x)
	{
		m_pVector[_i] = _x;
	}

	int GetLength() const
	{
		return m_Len;
	}

protected:
	fp32 *m_pVector;
	int m_Len;
};

class CMatrix
{
public:
	CMatrix(fp32 *_pMatrix, int _nRows, int _nColumns)
	{
		m_pMatrix = _pMatrix;
		m_nRows = _nRows;
		m_nColumns = _nColumns;
	}

	int GetRowCount() const
	{
		return m_nRows;
	}

	int GetColumnCount() const
	{
		return m_nColumns;
	}

protected:
	int m_nRows, m_nColumns;
	fp32 *m_pMatrix;
};

class CStateVector
{
public:
	CStateVector(vec128 *_pVector, int _Len)
	{
		m_pVector = _pVector;
		m_Len = _Len; 
	}

	vec128 Get(int _i)
	{
		return m_pVector[_i];
	}

	void Set(int _i, vec128 _v)
	{
		M_VSt(_v, &m_pVector[_i]);
	}

	int GetLength() const
	{
		return m_Len;
	}

protected:
	vec128 *m_pVector;
	int m_Len;
};


template <int BlockSize>
class CBlockVector
{
public:
	CBlockVector(int _Len)
	{
		m_lVector.SetLen(_Len * BlockSize);
		m_pVector = m_lVector;
	}

	fp32 *Get(int _i)
	{
		return &m_pVector[_i * BlockSize];
	}

	int Len()
	{
		return m_pVector.Len() / BlockSize;
	}

	class ConstIterator
	{
	public:
		const fp32 * operator *() 
		{
			return m_pVector;
		}

		ConstIterator& operator ++(int )
		{
			m_pVector += BlockSize;
			return *this;
		}

		ConstIterator& operator --(int) 
		{
			m_pVector -= BlockSize;
			return *this;
		}

		ConstIterator operator +(int _x)
		{
			ConstIterator ret = *this;
			ret.m_pVector += _x * BlockSize;
			return ret;
		}

	protected:
		friend class CBlockVector;

		const fp32 *m_pVector;
	};

	class Iterator
	{
	public:
		fp32 * operator *() 
		{
			return m_pVector;
		}

		Iterator& operator ++(int )
		{
			m_pVector += BlockSize;
			return *this;
		}

		Iterator& operator --(int) 
		{
			m_pVector -= BlockSize;
			return *this;
		}

		Iterator operator +(int _x)
		{
			Iterator ret = *this;
			ret.m_pVector += _x * BlockSize;
			return ret;
		}

	protected:
		friend class CBlockVector;

		fp32 *m_pVector;
	};

	ConstIterator Begin() const
	{
		ConstIterator i;
		i.m_pVector = m_lVector.GetBasePtr();
		return i;
	}

	Iterator Begin() 
	{
		Iterator i;
		i.m_pVector = m_lVector.GetBasePtr();
		return i;
	}

	TThinArray<fp32> & GetVector()
	{
		return m_lVector;
	}


protected:
	//fp32 *m_pVector;
	int m_ScalarLen;
	TThinArray<fp32> m_lVector;
	TAP_RCD<fp32> m_pVector;
};

class CMassVector : public CBlockVector<1>
{
public:
	CMassVector(int _Len) : CBlockVector<1>(_Len) {}

	void Set(int _i, fp32 _x)
	{
		*Get(_i) = _x;
	}
};


class CInertiaTensorVector : public CBlockVector<8>
{
public:
	CInertiaTensorVector(int _Len) : CBlockVector<8>(_Len) {}

	void Set(int _i, const CVec3Dfp32& _V)
	{
		fp32 Ixx = _V[0];
		fp32 Ixy = 0.0f;
		fp32 Ixz = 0.0f;
		fp32 Iyy = _V[1];
		fp32 Iyz = 0.0f;
		fp32 Izz = _V[2];

		fp32 *p = Get(_i);
		(*p++) = Ixx;
		(*p++) = Ixy;
		(*p++) = Ixz;
		(*p++) = 0.0f;
		(*p++) = Iyy;
		(*p++) = Iyz;
		(*p++) = Izz;
		(*p++) = 0.0f;
	}

	void Set(int _i, const CMat4Dfp32& _V)
	{
		fp32 Ixx = _V.k[0][0];
		fp32 Ixy = _V.k[0][1];
		fp32 Ixz = _V.k[0][2];

		//fp32 Iyx = _V.k[1][0];  // The inertia tensor is symmetric
		fp32 Iyy = _V.k[1][1];
		fp32 Iyz = _V.k[1][2];

		//fp32 Izx = _V.k[2][0];  // The inertia tensor is symmetric
		//fp32 Izy = _V.k[2][1];  // The inertia tensor is symmetric
		fp32 Izz = _V.k[2][2];

		fp32 *p = Get(_i);
		(*p++) = Ixx;
		(*p++) = Ixy;
		(*p++) = Ixz;
		(*p++) = 0.0f;
		(*p++) = Iyy;
		(*p++) = Iyz;
		(*p++) = Izz;
		(*p++) = 0.0f;
	}

	static void PrintFixed(fp32 _x)
	{
		char buf[100];

		sprintf(buf, "%1.1f", _x);

		size_t n = 6 - strlen(buf);
		for (int i = 0; i < n; i++)
			M_TRACEALWAYS(" ");

		M_TRACEALWAYS(buf);
	}

	void Print()
	{
		int n = Len();

		for (int i = 0; i < n; i++)
		{
			fp32 *p = Get(i);

			fp32 Ixx = *p++;
			fp32 Ixy = *p++;
			fp32 Ixz = *p++;
			*p++;
			fp32 Iyy = *p++;
			fp32 Iyz = *p++;
			fp32 Izz = *p++;
			*p++;

			fp32 Row1[3] = {Ixx, Ixy, Ixz};
			fp32 Row2[3] = {Ixy, Iyy, Iyz};
			fp32 Row3[3] = {Ixz, Iyz, Izz};
			fp32 *Rows[3] = {Row1, Row2, Row3};

			for (int i = 0; i < 3; i++)
			{
				PrintFixed(Rows[i][0]); M_TRACEALWAYS(" ");
				PrintFixed(Rows[i][1]); M_TRACEALWAYS(" ");
				PrintFixed(Rows[i][2]); M_TRACEALWAYS("\n");
			}
			M_TRACEALWAYS("\n");
		}
	}

};

class CSparseJacobianMatrix
{
public:
	CSparseJacobianMatrix(int _nRows, int _nColumns)
	{
		m_nRows = _nRows;
		m_nColumns = _nColumns;

		m_lMatrix.SetLen(_nRows * 4);
		m_lIndexMap.SetLen(_nRows * 2);

		m_pMatrix = m_lMatrix;
		m_pIndexMap = m_lIndexMap;
	}

	void PostMultiply(const CVector& _V, CVector& _Dest)
	{
		M_ASSERT(_V.GetLength() == 12, "!");
		M_ASSERT(_Dest.GetLength() == m_nRows, "!");

	}

	int GetRowCount() const
	{
		return m_nRows;
	}

	void Multiply(const CVector& _V, CVector& _Dest)
	{
		M_ASSERT(_V.GetLength() == 12, "!");
		M_ASSERT(_Dest.GetLength() == m_nRows, "!");

		int nRows = m_nRows;

		vec128 *pV = m_lMatrix.GetBasePtr();
		uint16 *pI = m_lIndexMap.GetBasePtr();

		for (int i = 0; i < nRows; i++)
		{
			vec128 J1_A, J1_B;
			vec128 J2_A, J2_B;

			int iJ1 = (*pI++);
			int iJ2 = (*pI++);

			J1_A = M_VLdMem(pV++);
			J1_B = M_VLdMem(pV++);
			J2_A = M_VLdMem(pV++);
			J2_B = M_VLdMem(pV++);

			vec128 X1_A = M_VLdMem(_V.GetP(iJ1 * 8));
			vec128 X1_B = M_VLdMem(_V.GetP(iJ1 * 8 + 4));

			vec128 X2_A = M_VLdMem(_V.GetP(iJ2 * 8));
			vec128 X2_B = M_VLdMem(_V.GetP(iJ2 * 8 + 4));

			vec128 T1 = M_VAdd(M_VDp3(J1_A, X1_A), M_VDp3(J1_B, X1_B));
			vec128 T2 = M_VAdd(M_VDp3(J2_A, X2_A), M_VDp3(J2_B, X2_B));
			CVec4Dfp32 Sum = M_VAdd(T1, T2);

			_Dest.Set(i, Sum[0]);
		}
	}

	class ConstIterator
	{
	public:

		ConstIterator& operator ++(int)
		{
			m_pMatrix += 2;
			m_pIndexMap += 1;
			return *this;
		}

		ConstIterator& operator --(int)
		{
			m_pMatrix -= 2;
			m_pIndexMap -= 1;

			return *this;
		}

		ConstIterator operator +(int _x)
		{
			ConstIterator ret = *this;
			ret.m_pMatrix += _x * 2;
			ret.m_pIndexMap += _x;
			return ret;
		}

		vec128 JacA()
		{
			return *m_pMatrix;
		}

		vec128 JacB()
		{
			return *(m_pMatrix+1);
		}

		uint16 Index()
		{
			return *m_pIndexMap;
		}

		friend class CSparseJacobianMatrix;

	protected:
		const vec128 *m_pMatrix;
		const uint16 *m_pIndexMap;
	};

	class Iterator
	{
	public:

		Iterator& operator ++(int)
		{
			m_pMatrix += 2;
			m_pIndexMap += 1;
			return *this;
		}

		Iterator& operator --(int)
		{
			m_pMatrix -= 2;
			m_pIndexMap -= 1;

			return *this;
		}

		Iterator operator +(int _x)
		{
			Iterator ret = *this;
			ret.m_pMatrix += _x * 2;
			ret.m_pIndexMap += _x;
			return ret;
		}

		vec128& JacA()
		{
			return *m_pMatrix;
		}

		vec128& JacB()
		{
			return *(m_pMatrix+1);
		}

		uint16& Index()
		{
			return *m_pIndexMap;
		}

		friend class CSparseJacobianMatrix;

	protected:
		vec128 *m_pMatrix;
		uint16 *m_pIndexMap;
	};

	ConstIterator Begin() const
	{
		ConstIterator i;
		i.m_pMatrix = m_lMatrix.GetBasePtr();
		i.m_pIndexMap = m_lIndexMap.GetBasePtr();
		return i;
	}

	Iterator Begin()
	{
		Iterator i;
		i.m_pMatrix = m_lMatrix.GetBasePtr();
		i.m_pIndexMap = m_lIndexMap.GetBasePtr();
		return i;
	}

	void Print() const
	{
		ConstIterator i = Begin();
		const int W = 4;

		M_TRACEALWAYS("{\n");	
		for (int iRow = 0; iRow < m_nRows; iRow++)
		{
			M_TRACEALWAYS("{ ");	
			vec128 J1A = i.JacA();
			vec128 J1B = i.JacB();
			uint16 iJ1 = i.Index();
			i++;
			vec128 J2A = i.JacA();
			vec128 J2B = i.JacB();
			uint16 iJ2 = i.Index();
			i++;

			for (int j = 0; j < iJ1; j++)
			{
				for (int k = 0; k < 6; k++)
				{
					PrintFixed(0.0f, W);
					M_TRACEALWAYS(",");
				}
			}

			PrintFixed(J1A, W);
			M_TRACEALWAYS(", ");
			PrintFixed(J1B, W);
			M_TRACEALWAYS(", ");

			for (int j = iJ1+1; j < iJ2; j++)
			{
				for (int k = 0; k < 6; k++)
				{
					PrintFixed(0.0f, W);
					M_TRACEALWAYS(", ");
				}
			}

			PrintFixed(J2A, W);
			M_TRACEALWAYS(", ");
			PrintFixed(J2B, W); 

			if (iRow != m_nRows - 1)
				M_TRACEALWAYS("}, \n");
			else
				M_TRACEALWAYS("} \n");

		}
		M_TRACEALWAYS("}\n");	
	}

	/*

	TODO: Är inte testade...
	int GetJacobianIndex(uint16 _iRow, uint16 _iColumn) const
	{
	int i = m_pIndexMap[_iRow * m_nRealRows + _iColumn];
	return i;
	}

	void  GetJacobian(uint16 _iRow, uint16 _iColumn, vec128& _J1, vec128& _J2) const
	{
	_J1 = M_VLdMem(&m_pMatrix[m_pIndexMap[_iRow * m_nRealRows + _iColumn]]);
	_J2 = M_VLdMem(&m_pMatrix[m_pIndexMap[_iRow * m_nRealRows + _iColumn]] + 4);
	}

	void  GetJacobianFromIndex(uint16 _Index, vec128& _J1, vec128& _J2) const
	{
	_J1 = M_VLdMem(&m_pMatrix[_Index * 8]);
	_J2 = M_VLdMem(&m_pMatrix[_Index * 8] + 4);
	}
	*/

	/*
	const fp32 *GetJacobian(uint16 _iRow, uint16 _iColumn) const
	{
	int i = m_pMap[_iRow * m_nRealRows + _iColumn];
	return &m_pMatrix[i];
	}
	*/

protected:
	int m_nRows, m_nColumns;
	TThinArray<vec128> m_lMatrix;
	TThinArray<uint16> m_lIndexMap;

	TAP_RCD<vec128> m_pMatrix;
	TAP_RCD<uint16> m_pIndexMap;
};


template <typename T, int TStride = sizeof(T)>
class TSparseMatrixIterator
{
public:

	TSparseMatrixIterator(TThinArray<T>& _v, TThinArray<uint16>& _iv)
	{
		m_pMatrix = _v.GetBasePtr();
		m_pIndexMap = _iv.GetBasePtr();

#ifdef WDYNAMICSENGINE_DEBUG
		m_pMinMatrix = _v.GetBasePtr();
		m_pMaxMatrix = _v.GetBasePtr() + _v.Len();
		m_pMaxMatrix = (T *) (((char *) m_pMaxMatrix) - TStride);

		m_pMinIndexMap = _iv.GetBasePtr();
		m_pMaxIndexMap = _iv.GetBasePtr() + (_v.Len() - 1);
#endif
	}

	TSparseMatrixIterator& operator ++(int)
	{
		m_pMatrix = (T *) (((char *) m_pMatrix) + TStride);
		m_pIndexMap++;
		return *this;
	}

	TSparseMatrixIterator& operator --(int)
	{
		m_pMatrix = (T *) (((char *) m_pMatrix) - TStride);
		m_pIndexMap++;
		return *this;
	}

	TSparseMatrixIterator operator + (int _x)
	{
		TSparseMatrixIterator i = *this;
		i.m_pMatrix = (T *) (((char *) i.m_pMatrix) + TStride * _x);
		i.m_pIndexMap += _x;
		return i;
	}

	TSparseMatrixIterator operator - (int _x)
	{
		TSparseMatrixIterator i = *this;
		i.m_pMatrix = (T *) (((char *) i.m_pMatrix) - TStride * _x);
		i.m_pIndexMap -= _x;
		return i;
	}

	uint16 Index() const
	{
#ifdef WDYNAMICSENGINE_DEBUG
		M_ASSERT(m_pIndexMap >= m_pMinIndexMap, "!");
		M_ASSERT(m_pIndexMap <= m_pMaxIndexMap, "!");
#endif

		return *m_pIndex;
	}

	uint16& Index()
	{
#ifdef WDYNAMICSENGINE_DEBUG
		M_ASSERT(m_pIndexMap >= m_pMinIndexMap, "!");
		M_ASSERT(m_pIndexMap <= m_pMaxIndexMap, "!");
#endif

		return *m_pIndexMap;
	}

	T& operator *()
	{
#ifdef WDYNAMICSENGINE_DEBUG
		M_ASSERT(m_pMatrix >= m_pMinMatrix, "!");
		M_ASSERT(m_pMatrix <= m_pMaxMatrix, "!");
#endif

		return *m_pMatrix;
	}

	const T operator *() const
	{
#ifdef WDYNAMICSENGINE_DEBUG
		M_ASSERT(m_pMatrix >= m_pMinMatrix, "!");
		M_ASSERT(m_pMatrix <= m_pMaxMatrix, "!");
#endif

		return *m_pMatrix;
	}

protected:
	T *m_pMatrix;
	uint16 *m_pIndexMap;

#ifdef WDYNAMICSENGINE_DEBUG
	T *m_pMinMatrix, *m_pMaxMatrix;
	uint16 *m_pMinIndexMap, *m_pMaxIndexMap;
#endif

};


template <typename T, int TStride = sizeof(T)>
class TVectorIterator
{
public:
	TVectorIterator(TThinArray<T>& _v)
	{
		m_pV = _v.GetBasePtr();
#ifdef WDYNAMICSENGINE_DEBUG
		m_pMinV = _v.GetBasePtr();
		m_pMaxV = _v.GetBasePtr() + _v.Len();
		m_pMaxV = (T *) (((char *) m_pMaxV) - TStride);
#endif
	}

	TVectorIterator(T *_p)
	{
		m_pV = _p;
	}

	TVectorIterator& operator++(int)
	{
		m_pV = (T *) (((char *) m_pV) + TStride);
		return *this;
	}

	TVectorIterator& operator--(int)
	{
		m_pV = (T *) (((char *) m_pV) - TStride);
		return *this;
	}

	TVectorIterator operator+ (int _x)
	{
		TVectorIterator i = *this;
		i.m_pV = (T *) (((char *) i.m_pV) + TStride * _x);
		return i;
	}

	T& operator *()
	{
#ifdef WDYNAMICSENGINE_DEBUG
		M_ASSERT(m_pV >= m_pMinV, "!");
		M_ASSERT(m_pV <= m_pMaxV, "!");
#endif
		return *m_pV;
	}

	const T operator *() const
	{
#ifdef WDYNAMICSENGINE_DEBUG
		M_ASSERT(m_pV >= m_pMinV, "!");
		M_ASSERT(m_pV <= m_pMaxV, "!");
#endif
		return *m_pV;
	}

protected:
	T *m_pV;

#ifdef WDYNAMICSENGINE_DEBUG
	T *m_pMinV, *m_pMaxV;
#endif

};


/*
struct vec256
{
vec256() {}

vec256(vec128 _a, vec128 _b)
{
a = _a;
b = _b;
}

vec128 a, b;
};
*/

class CLCPMath
{
public:

	static void Multiply(const CVector& _V, const CMatrix& _M, CMatrix& _Result)
	{
		M_ASSERT(_V.GetLength() == _M.GetRowCount(), "!");
		M_ASSERT(_Result.GetRowCount() == 1, "!");
		M_ASSERT(_Result.GetColumnCount() == _M.GetColumnCount(), "!");

		//int 

		//	for ()
	}

	static void Multiply(const CSparseJacobianMatrix& _J, const CBlockVector<8>& _V, CBlockVector<1>& _R)
	{
		//		M_ASSERT(_V.GetLength() == 12, "!");
		//		M_ASSERT(_Dest.GetLength() == m_nRows, "!");

		CSparseJacobianMatrix::ConstIterator iJ = _J.Begin();
		CBlockVector<8>::ConstIterator iV = _V.Begin();
		CBlockVector<1>::Iterator iR = _R.Begin();

		int nRows = _J.GetRowCount();
		for (int i = 0; i < nRows; i++)
		{
			vec128 J1_A = iJ.JacA();
			vec128 J1_B = iJ.JacB();
			uint16 iJ1 = iJ.Index();
			iJ++;
			vec128 J2_A = iJ.JacA();
			vec128 J2_B = iJ.JacB();
			uint16 iJ2 = iJ.Index();
			iJ++;

			vec128 X1_A = M_VLdMem(*(iV + iJ1));
			vec128 X1_B = M_VLdMem((*(iV + iJ1)) + 4);

			vec128 X2_A = M_VLdMem(*(iV + iJ2));
			vec128 X2_B = M_VLdMem((*(iV + iJ2)) + 4);

			vec128 T1 = M_VAdd(M_VDp3(J1_A, X1_A), M_VDp3(J1_B, X1_B));
			vec128 T2 = M_VAdd(M_VDp3(J2_A, X2_A), M_VDp3(J2_B, X2_B));
			CVec4Dfp32 Sum = M_VAdd(T1, T2);

			**iR = Sum[0];
			iR++;
		}
	}

	M_INLINE static void UnpackInertiaTensor(vec128 _v1, vec128 _v2, vec128& _row1, vec128& _row2, vec128& _row3)
	{
		_row1 = _v1;
		_row2 = M_VOr(M_VShuf(_v2, M_VSHUF(3,0,1,3)), M_VShuf(_v1, M_VSHUF(1,3,3,3)));
		_row3 = M_VOr(M_VShuf(_v2, M_VSHUF(3,1,2,3)), M_VShuf(_v1, M_VSHUF(2,3,3,3)));
	}

	M_INLINE static void PackInertiaTensor(const CVec3Dfp32& _V, CVec4Dfp32& _V1, CVec4Dfp32& _V2)
	{
		fp32 Ixx = _V[0];
		fp32 Ixy = 0.0f;
		fp32 Ixz = 0.0f;
		fp32 Iyy = _V[1];
		fp32 Iyz = 0.0f;
		fp32 Izz = _V[2];

		_V1[0] = Ixx;
		_V1[1] = Ixy;
		_V1[2] = Ixz;
		_V1[3] = 0.0f;
		_V2[0] = Iyy;
		_V2[1] = Iyz;
		_V2[2] = Izz;
		_V2[3] = 0.0f;
	}

	M_INLINE static CVector4Pair PackInertiaTensor(const CVec4Dfp32& _V)
	{
		CVec4Dfp32 T1, T2;
		PackInertiaTensor(CVec3Dfp32(_V[0], _V[1], _V[2]), T1, T2);
		return CVector4Pair(T1, T2);
	}

	M_INLINE static void PackInertiaTensor(const CMat4Dfp32& _M, CVec4Dfp32& _V1, CVec4Dfp32& _V2)
	{
		fp32 Ixx = _M.k[0][0];
		fp32 Ixy = _M.k[0][1];
		fp32 Ixz = _M.k[0][2];

		//fp32 Iyx = _V.k[1][0];  // The inertia tensor is symmetric
		fp32 Iyy = _M.k[1][1];
		fp32 Iyz = _M.k[1][2];

		//fp32 Izx = _V.k[2][0];  // The inertia tensor is symmetric
		//fp32 Izy = _V.k[2][1];  // The inertia tensor is symmetric
		fp32 Izz = _M.k[2][2];

		_V1[0] = Ixx;
		_V1[1] = Ixy;
		_V1[2] = Ixz;
		_V1[3] = 0.0f;
		_V2[0] = Iyy;
		_V2[1] = Iyz;
		_V2[2] = Izz;
		_V2[3] = 0.0f;
	}


	/*
	template <int MStride, int IStride>
	static void MassMatrix_Times_Vector(TVectorIterator<fp32, MStride> _Minv, TVectorIterator<vec128, IStride> _IInv)
	{
	fp32 x = 0.0f;
	for (int i = 0; i < 10; i++)
	{
	x += *_Minv;
	_Minv++;
	}

	M_TRACEALWAYS("%f",x);

	}

	*/

	template <int MStride, int IStride, int VStride, int RStride>
		static void Mi_Times_V(int _n, TVectorIterator<fp32, MStride> _Minv,  TVectorIterator<vec128, IStride> _Iinv, TVectorIterator<fp32, VStride> _V, TVectorIterator<fp32, RStride> _Result)
	{
		/*M_ASSERT(_Minv.Len() == _Iinv.Len(), "!");
		M_ASSERT(2 * _Minv.Len() == _Iinv.Len(), "!");
		M_ASSERT(_V.Len() == _Result.Len(), "!");
		M_ASSERT(_V.Len() == _n * 6, "!");
		*/


		for (int i = 0; i < _n; i++)
		{
			vec128 Iinv_Row1, Iinv_Row2, Iinv_Row3;
			vec128 T1 = *_Iinv; _Iinv++;
			vec128 T2 = *_Iinv; _Iinv++;
			UnpackInertiaTensor(T1, T2, Iinv_Row1, Iinv_Row2, Iinv_Row3);

			fp32 Mass = *_Minv; _Minv++;

			fp32 x1 = *_V; _V++;
			fp32 x2 = *_V; _V++;
			fp32 x3 = *_V; _V++;
			fp32 x4 = *_V; _V++;
			fp32 x5 = *_V; _V++;
			fp32 x6 = *_V; _V++;

			*_Result = x1 * Mass; _Result++;
			*_Result = x2 * Mass; _Result++;
			*_Result = x3 * Mass; _Result++;

			vec128 v = M_VLd(x4, x5, x6, 0.0f);
			CVec4Dfp32 T = M_VDp4x4(v, Iinv_Row1, v, Iinv_Row2, v, Iinv_Row3, M_VZero(), M_VZero());

			*_Result = T[0]; _Result++;
			*_Result = T[1]; _Result++;
			*_Result = T[2]; _Result++;
		}
	}

	template <int MStride, int IStride, int VStride, int RStride>
		static void Mi_Times_V(int _n, TVectorIterator<fp32, MStride> _Minv,  TVectorIterator<CVector4Pair, IStride> _Iinv, TVectorIterator<fp32, VStride> _V, TVectorIterator<fp32, RStride> _Result)
	{
		for (int i = 0; i < _n; i++)
		{
			const CVector4Pair& I = *_Iinv;
			vec128 Iinv_Row1, Iinv_Row2, Iinv_Row3;
			vec128 T1 = I.a;
			vec128 T2 = I.b;
			_Iinv++;
			UnpackInertiaTensor(T1, T2, Iinv_Row1, Iinv_Row2, Iinv_Row3);

			fp32 Mass = *_Minv; _Minv++;

			fp32 x1 = *_V; _V++;
			fp32 x2 = *_V; _V++;
			fp32 x3 = *_V; _V++;
			fp32 x4 = *_V; _V++;
			fp32 x5 = *_V; _V++;
			fp32 x6 = *_V; _V++;

			*_Result = x1 * Mass; _Result++;
			*_Result = x2 * Mass; _Result++;
			*_Result = x3 * Mass; _Result++;

			vec128 v = M_VLd(x4, x5, x6, 0.0f);
			CVec4Dfp32 T = M_VDp4x4(v, Iinv_Row1, v, Iinv_Row2, v, Iinv_Row3, M_VZero(), M_VZero());

			*_Result = T[0]; _Result++;
			*_Result = T[1]; _Result++;
			*_Result = T[2]; _Result++;
		}
	}

	template <int MStride, int IStride, int VStride, int RStride>
		static void Mi_Times_V(int _n, 
		TVectorIterator<fp32, MStride> _Minv,  
		TVectorIterator<CVector4Pair, IStride> _Iinv, 
		TVectorIterator<CVector4Pair, VStride> _V, 
		TVectorIterator<CVector4Pair, RStride> _Result)
	{
		for (int i = 0; i < _n; i++)
		{
			const CVector4Pair& I = *_Iinv;
			vec128 Iinv_Row1, Iinv_Row2, Iinv_Row3;
			vec128 T1 = I.a;
			vec128 T2 = I.b;
			_Iinv++;
			UnpackInertiaTensor(T1, T2, Iinv_Row1, Iinv_Row2, Iinv_Row3);

			vec128 Mass = M_VLdScalar(*_Minv);
			_Minv++;

			const CVector4Pair& V = *_V;
			vec128 Va = V.a;
			vec128 Vb = V.b;
			_V++;

			vec128 R1 = M_VMul(Va, Mass);
			CVec4Dfp32 R2 = M_VDp4x4(Vb, Iinv_Row1, Vb, Iinv_Row2, Vb, Iinv_Row3, M_VZero(), M_VZero());

			CVector4Pair *pR = &(*_Result);
			pR->a = R1;
			pR->b = R2;

			_Result++;
		}
	}

	template <int VStride, int DStride>
		static void Divide(int _n, TSparseMatrixIterator<CVector4Pair> _J, TVectorIterator<fp32, VStride> _V, TVectorIterator<fp32, DStride> _D)
	{
		for (int i = 0; i < _n; i++)
		{
			fp32 DrecipScalar = 1.0f / *_D;
			vec128 Drecip = M_VLdScalar(DrecipScalar);
			_D++;

			CVector4Pair *J1 = &(*_J);
			_J++;
			CVector4Pair *J2 = &(*_J);
			_J++;

			vec128 J1_A = J1->a;
			vec128 J1_B = J1->b;
			J1->a = M_VMul(J1_A, Drecip);
			J1->b = M_VMul(J1_B, Drecip);

			vec128 J2_A = J2->a;
			vec128 J2_B = J2->b;
			J2->a = M_VMul(J2_A, Drecip);
			J2->b = M_VMul(J2_B, Drecip);

			*_V *= DrecipScalar;

			_V++;
		}
	}

	/*
	_R MUST BE ZERO!!!
	*/

	template <int VStride, int RStride>
		static void Sparse_Times_Vector(int _n, TSparseMatrixIterator<CVector4Pair> _J, TVectorIterator<fp32, VStride> _V, TVectorIterator<CVector4Pair, RStride> _R)
	{
		for (int i = 0; i < _n; i++)
		{
			const CVector4Pair& J1 = *_J;
			uint16 iJ1 = _J.Index();
			vec128 J1_A = J1.a;
			vec128 J1_B = J1.b;
			_J++;

			const CVector4Pair& J2 = *_J;
			uint16 iJ2 = _J.Index();
			vec128 J2_A = J2.a;
			vec128 J2_B = J2.b;
			_J++;

			vec128 v = M_VLdScalar(*_V);
			_V++;

			vec128 T1 = M_VMul(J1_A, v);
			vec128 T2 = M_VMul(J1_B, v);
			vec128 T3 = M_VMul(J2_A, v);
			vec128 T4 = M_VMul(J2_B, v);

			CVector4Pair *pR1 = &(*(_R + iJ1));
			CVector4Pair *pR2 = &(*(_R + iJ2));

			vec128 R1a = pR1->a;
			vec128 R1b = pR1->b;
			vec128 R2a = pR2->a;
			vec128 R2b = pR2->b;

			pR1->a = M_VAdd(T1, R1a);
			pR1->b = M_VAdd(T2, R1b);
			pR2->a = M_VAdd(T3, R2a);
			pR2->b = M_VAdd(T4, R2b);

			/*
			Ger dååååååååålig kod på pz!

			const CVector4Pair& R1 = *(_R + iJ1);
			const CVector4Pair& R2 = *(_R + iJ2);

			*(_R + iJ1) = CVector4Pair(M_VAdd(T1, R1.a), M_VAdd(T2, R1.b));
			*(_R + iJ2) = CVector4Pair(M_VAdd(T3, R2.a), M_VAdd(T4, R2.b));
			*/
		}
	}

	/*
	D = Diagonal(J * MJT_T);
	J and MJT_T MUST OBEY THE SAME SPARSITY PATTERN!!!
	*/

	template <int DStride>
		static void Sparse_Times_Sparse_Diagonal(int _n, TSparseMatrixIterator<CVector4Pair> _J, TSparseMatrixIterator<CVector4Pair> _MJT_T, TVectorIterator<fp32, DStride> _D)
	{
		for (int i = 0; i < _n; i++)
		{

#ifdef WDYNAMICSENGINE_DEBUG
			M_ASSERT(_J.Index() == _MJT_T.Index(), "Invalid sparsity pattern!");
#endif
			const CVector4Pair& J1 = *_J;
			_J++;
			vec128 J1_A = J1.a;
			vec128 J1_B = J1.b;

			const CVector4Pair& MJT_T1 = *_MJT_T;
			_MJT_T++;
			vec128 MJT_T1_A = MJT_T1.a;
			vec128 MJT_T1_B = MJT_T1.b;

#ifdef WDYNAMICSENGINE_DEBUG
			M_ASSERT(_J.Index() == _MJT_T.Index(), "Invalid sparsity pattern!");
#endif
			const CVector4Pair& J2 = *_J;
			_J++;
			vec128 J2_A = J2.a;
			vec128 J2_B = J2.b;

			const CVector4Pair& MJT_T2 = *_MJT_T;
			_MJT_T++;
			vec128 MJT_T2_A = MJT_T2.a;
			vec128 MJT_T2_B = MJT_T2.b;

			vec128 T = M_VDp4x4(J1_A, MJT_T1_A, 
				J1_B, MJT_T1_B, 
				J2_A, MJT_T2_A, 
				J2_B, MJT_T2_B);

			// TODO: Load-hit-store?
			CVec4Dfp32 tmp = M_VAdd(M_VAdd(M_VAdd(T, M_VSplat(T, 1)), M_VSplat(T, 2)), M_VSplat(T, 3));
			*_D = tmp[0];
			_D++;
		}
	}

	template <int MStride, int IStride>
		static void Mi_Times_Jt_t(int _n,
		TVectorIterator<fp32, MStride> _Minv, 
		TVectorIterator<CVector4Pair, IStride> _Iinv, 
		TSparseMatrixIterator<CVector4Pair> _J,
		TSparseMatrixIterator<CVector4Pair> _MJT_T)
	{
		vec128 Zero = M_VZero();

		int nRows = _n;
		for (int i = 0; i < nRows; i++)
		{
			const CVector4Pair &J1 = *_J;
			vec128 J1_A = J1.a;
			vec128 J1_B = J1.b;
			uint16 iJ1 = _J.Index();
			_J++;

			const CVector4Pair &J2 = *_J;
			vec128 J2_A = J2.a;
			vec128 J2_B = J2.b;
			uint16 iJ2 = _J.Index();
			_J++;

			vec128 m1inv = M_VLdScalar(*(_Minv + iJ1));
			vec128 m2inv = M_VLdScalar(*(_Minv + iJ2));

			vec128 I1inv_Row1, I1inv_Row2, I1inv_Row3;
			const CVector4Pair& Tmp1 = *(_Iinv + iJ1);
			UnpackInertiaTensor(Tmp1.a, Tmp1.b, I1inv_Row1, I1inv_Row2, I1inv_Row3);

			vec128 I2inv_Row1, I2inv_Row2, I2inv_Row3;
			const CVector4Pair& Tmp2 = *(_Iinv + iJ2);
			UnpackInertiaTensor(Tmp2.a, Tmp2.b, I2inv_Row1, I2inv_Row2, I2inv_Row3);

			vec128 T1 = M_VMul(J1_A, m1inv);
			vec128 T2 = M_VDp4x4(J1_B, I1inv_Row1, J1_B, I1inv_Row2, J1_B, I1inv_Row3, Zero, Zero);		

			vec128 T3 = M_VMul(J2_A, m2inv);
			vec128 T4 = M_VDp4x4(J2_B, I2inv_Row1, J2_B, I2inv_Row2, J2_B, I2inv_Row3, Zero, Zero);

			CVector4Pair *pMJT1 = &(*_MJT_T);
			pMJT1->a = T1;
			pMJT1->b = T2;
			_MJT_T.Index() = iJ1;
			_MJT_T++;

			CVector4Pair *pMJT2 = &(*_MJT_T);
			pMJT2->a = T3;
			pMJT2->b = T4;
			_MJT_T.Index() = iJ2;
			_MJT_T++;
		}
	}



	/*	template <int MStride, int IStride, int VStride, int RStride>
	static void MassMatrix_Times_Vector(TVectorIterator<fp32, MStride> _Minv,  TVectorIterator<fp32, IStride> _Iinv, TVectorIterator<vec128, VStride> _V, TVectorIterator<vec128, RStride> _Result)
	{
	int n = _Minv.Len();

	M_ASSERT(_Minv.Len() == _Iinv.Len(), "!");
	M_ASSERT(2 * _Minv.Len() == _Iinv.Len(), "!");
	M_ASSERT(_V.Len() == _Result.Len(), "!");
	M_ASSERT(_V.Len() == n * 6, "!");

	for (int i = 0; i < n; i++)
	{
	vec128 Iinv_Row1, Iinv_Row2, Iinv_Row3;
	vec128 T1 = *_Iinv++;
	vec128 T2 = *_Iinv++;
	UnpackInertiaTensor(T1, T2, Iinv_Row1, Iinv_Row2, Iinv_Row3);

	fp32 Mass = *_Minv++;

	vec128 v1 = *_V++;
	vec128 v2 = *_V++;

	*_Result++ = M_VMul(v1, Mass);

	CVec4Dfp32 T = M_VDp4x4(v2, Iinv_Row1, v2, Iinv_Row2, v2, Iinv_Row3, M_VZero(), M_VZero());
	*_Result++ = T;
	}
	}
	*/
	static void Mi_Times_Jt_t(const CBlockVector<1> & _Minv, const CBlockVector<8> & _Iinv, const CSparseJacobianMatrix& _J, CSparseJacobianMatrix& _MJT_T)
	{
		CBlockVector<1>::ConstIterator iMinv = _Minv.Begin();
		CBlockVector<8>::ConstIterator iInv = _Iinv.Begin();
		CSparseJacobianMatrix::ConstIterator iJ = _J.Begin();
		CSparseJacobianMatrix::Iterator iMJT_T = _MJT_T.Begin();

		vec128 Zero = M_VZero();

		int nRows = _J.GetRowCount();
		for (int i = 0; i < nRows; i++)
		{
			vec128 J1_A = iJ.JacA();
			vec128 J1_B = iJ.JacB();
			uint16 iJ1 = iJ.Index();
			iJ++;
			vec128 J2_A = iJ.JacA();
			vec128 J2_B = iJ.JacB();
			uint16 iJ2 = iJ.Index();
			iJ++;

			vec128 m1inv = M_VLdScalar(**(iMinv + iJ1));
			vec128 m2inv = M_VLdScalar(**(iMinv + iJ2));

			vec128 I1inv_Row1 = M_VLdMem(*(iInv + iJ1));
			vec128 I1inv_Other = M_VLdMem((*(iInv + iJ1)) + 4);
			vec128 I1inv_Row2 = M_VOr(M_VShuf(I1inv_Other, M_VSHUF(3,0,1,3)), M_VShuf(I1inv_Row1, M_VSHUF(1,3,3,3)));
			vec128 I1inv_Row3 = M_VOr(M_VShuf(I1inv_Other, M_VSHUF(3,1,2,3)), M_VShuf(I1inv_Row1, M_VSHUF(2,3,3,3)));

			vec128 I2inv_Row1 = M_VLdMem(*(iInv + iJ2));
			vec128 I2inv_Other = M_VLdMem((*(iInv + iJ2)) + 4);
			vec128 I2inv_Row2 = M_VOr(M_VShuf(I2inv_Other, M_VSHUF(3,0,1,3)), M_VShuf(I2inv_Row1, M_VSHUF(1,3,3,3)));
			vec128 I2inv_Row3 = M_VOr(M_VShuf(I2inv_Other, M_VSHUF(3,1,2,3)), M_VShuf(I2inv_Row1, M_VSHUF(2,3,3,3)));

			vec128 T1 = M_VMul(J1_A, m1inv);
			vec128 T2 = M_VDp4x4(J1_B, I1inv_Row1, J1_B, I1inv_Row2, J1_B, I1inv_Row3, Zero, Zero);		

			vec128 T3 = M_VMul(J2_A, m2inv);
			vec128 T4 = M_VDp4x4(J2_B, I2inv_Row1, J2_B, I1inv_Row2, J2_B, I2inv_Row3, Zero, Zero);

			iMJT_T.JacA() = T1;
			iMJT_T.JacB() = T2;
			iMJT_T.Index() = iJ1;
			iMJT_T++;
			iMJT_T.JacA() = T3;
			iMJT_T.JacB() = T4;
			iMJT_T.Index() = iJ2;
			iMJT_T++;
		}
	}

protected:

};

class CWD_LCPSolver : public CWD_DynamicsSolver
{
public:
	virtual void Solve(const CWD_Island& _Island);

protected:

	// "Work" vectors
	TThinArray<CVector4Pair> m_lFext;
	TThinArray<fp32> m_lMassInv;
	TThinArray<CVector4Pair> m_lInertiaTensorInv;
	TThinArray<CVector4Pair> m_lJacobian;
	TThinArray<uint16> m_lJacobianIndices;

	// Temporary result vectors
	TThinArray<CVector4Pair> m_lMiJt_t;
	TThinArray<uint16> m_lMiJt_t_Indices;
	TThinArray<CVector4Pair> m_lMi_Fext;
};


/*
class CProjectedGaussSeidelSolver
{
public:
void Solve(const TThinArray<fp32>& _Mrecip, const TThinArray<fp32>& _Irecip, const CSparseJacobianMatrix& _J);
};

void CProjectedGaussSeidelSolver::Solve(const TThinArray<fp32>& _Mrecip, const TThinArray<fp32>& _Irecip, const CSparseJacobianMatrix& _J)
{
//CSparseJacobianMatrix _Mrecip_Jtrans;



}
*/


#endif