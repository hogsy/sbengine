#ifndef _INC_MOS_Math
#define _INC_MOS_Math

#ifdef	PLATFORM_PS2
#include <eekernel.h>
extern "C" void FooBreak();
#endif

/*
------------------------------------------------------------------------------------------------
NAME:		MOS_Math.cpp/h
PURPOSE:	Math library
CREATION:	960901
AUTHOR:		Magnus Högdahl
COPYRIGHT:	Magnus Högdahl

CONTENTS:
template class		Template_Matrix4			960820	OK					typedef: CMat4Dint8 .. CMat4Dfp10
template class		Template_Matrix2			9610??	OK					typedef: CMat2Dint8 .. CMat2Dfp10
template class		Template_Vector3			960820	OK					typedef: CVec3Dint8 .. CVec3Dfp10
template class		Template_Vector4			9610??	OK					typedef: CVec4Dint8 .. CVec4Dfp10
class				CVec4D8S					960820	Not tested			RGBA operations (Deleted ?)
template class		Template_Complex			960820	Not tested			typedef: cpf4 .. cfp10
template class		Template_Roots				960828	Not tested			Root container, roots_cp4.. (Deleted 990101)
and more stuff...

*.*					*.*							990101						Polishing and indexing of all classes.
template class		TAxisRot					990101	OK					Axis-rotation class.
template class		TVector4					990608	OK					Implemented most functionally from Vec3.
template class		TOBB						000516	OK					Oriented bounding box template.


------------------------------------------------------------------------------------------------
 TMATRIX2
------------------------------------------------------------------------------------------------
class TMatrix2
{
public:
	T k[2][2];  // row, column

   	// --------------------------------
	// !!!NOT INDEXED!!!
	// --------------------------------
}

------------------------------------------------------------------------------------------------
 TMATRIX4
------------------------------------------------------------------------------------------------
class TMatrix4
{
public:
	T k[4][4];  // row, column

	// --------------------------------
	// Construction/creation
	// --------------------------------
	TMatrix4()
	TMatrix4(T k00, T k01, T k02, T k03, ...)			// Construct from components
	void Unit()
	void Unit3x3()										// Load identity into 3x3.
	void UnitNot3x3()									// Load identity into row & column 4. (!3x3)

	void SetXRotation(T v)								// Load euler rotations.
	void SetYRotation(T v)
	void SetZRotation(T v)
	void SetXRotation3x3(T v)							// Load euler rotations, leaving affine part unchanged.
	void SetYRotation3x3(T v)
	void SetZRotation3x3(T v)

	// --------------------------------
	// Operations
	// --------------------------------
	void M_x_RotX(T v)									// Multiply by euler rotations
	void M_x_RotY(T v)
	void M_x_RotZ(T v)
	void RotX_x_M(T v)									// Backwards multiply by euler rotations
	void RotY_x_M(T v)
	void RotZ_x_M(T v)

	void Transpose()									// Transpose 4x4
	void Transpose3x3()									// Transpose 3x3
	void Transpose(M& DestMat) const					// Transpose 4x4 with destination
	void Transpose3x3(M& DestMat) const					// Transpose 3x3 with destination
	bool Inverse(M& DestMat) const						// Calculate 4x4 inverse with destination
	bool Inverse3x3(M& DestMat) const					// Calculate 3x3 inverse with destination, clearing affine part.
	
	void InverseOrthogonal(M& DestMat) const			// Calculate 4x4 inverse of ortogonal matrix.
	bool IsMirrored() const								// Check if 3x3 part is mirrored.

	void Multiply3x3(T s)								// Scale 3x3
	void Multiply(T s)									// Scale 4x4
	void Multiply(const M& m, M& DestMat) const			// Multiply 4x4 with destination
	void Multiply3x3(const M& m, M& DestMat) const		// Multiply 3x3 with destination, clearing affine part
	
	void Normalize3x3()									// Normalize 3x3
	void RecreateMatrix(int _Priority0, int _Priority1)	// Renormalize and make it orthogonal

	// --------------------------------
	// Helpers
	// --------------------------------
	CStr GetStringRow(int row) const					// Get output string from row
	CStr GetString()									// Get string for output.

  	CStr GetParseString() const
	void ParseString(CStr _s)
	void Read(CCFile* _pFile)
	void Write(CCFile* _pFile) const
	void SwapLE()
}

------------------------------------------------------------------------------------------------
 TVECTOR3
------------------------------------------------------------------------------------------------
class TVector3
{
public:
	T k[3];

	// --------------------------------
	// Construction
	// --------------------------------
	TVector3()
	TVector3(T val)										// Construct and fill with val.
	TVector3(T x, T y, T z)								// Construct from components
	TVector3(const V& a)								// Construct from vector

	// --------------------------------
	// Convertion
	// --------------------------------
	TVector3<fp32> Getfp32()								// Get float(fp32) vector
	TVector3<fp64> Getfp64()								// Get double(fp64) vector
	void Assignfp32(TVector3<fp32>& _Dst)					// Assign to float vector
	void Assignfp64(TVector3<fp64>& _Dst)					// Assign to double vector

	// --------------------------------
	// Operators
	// --------------------------------
	T& operator[] (int _k)								// Component index-operator
	const T& operator[] (int _k) const					// Component index-operator
	bool operator== (const V& _v) const					// Equal operator
	bool operator!= (const V& _v) const					// Not equal operator
	bool AlmostEqual(const V& _v, T _Epsilon) const						// Compare with epsilon
	V operator+ (const V& a) const						// Vector add
	V operator- (const V& a) const						// Vector subtract
	V operator- () const								// Negate
	void operator+= (const V& a)						// Assignment add
	void operator-= (const V& a)						// Assignment subtract
	T operator* (const V& a) const						// Dotproduct
	V operator* (T scalar) const						// Scale
	void operator*= (T scalar)							// Assignment scale
	V operator* (const M& m) const						// Multiply with matrix
	void operator*= (const M& m)						// Assignment matrix-multiply
	V operator/ (const V& a) const						// Crossproduct
	V operator/ (T a) const								// Scale with 1 / a

	// --------------------------------
	// Operations
	// --------------------------------
	void Scale(T scalar, V& dest) const					// Scale with destination.
	void CrossProd(const V& a, V& dest) const			// Crossproduct with destination.
	void CompMul(const V& a, V& dest) const				// Multiplies components
	void Add(const V& a, V& dest) const					// Add with destination.
	void Sub(const V& a, V& dest) const					// Subtract with destination.
	void Combine(const V& a, T t, V& dest) const		// Combine (dest = this + a*t) with destination.
	void Lerp(const V& a, T t, V& dest) const			// Interpolates between this and a with t.
	void MultiplyMatrix(const M& m, V& dest) const		// Multiply matrix with destination.
	T Distance(const V& a) const						// Distance between this and a
	T DistanceSqr(const V& a) const						// Square-distance between this and a
	T Length() const									// Length of vector. (Magnitude)
	T LengthSqr() const									// Square-length of vector.
	V& Normalize()										// Normalize this, returning reference

	// --------------------------------
	// Matrix load/store/multiply
	// --------------------------------
	void AddMatrixRow(M& m, int row) const				// Add matrix-row
	void AddMatrixColumn(M& m, int column) const		// Add matrix-column

	static V& GetMatrixRow(M& _Mat, int _Row)			// Get vector-reference from matrix-row
	static const V& GetMatrixRow(const M& _Mat, int _Row) // Get vector-reference from matrix-row
	void SetMatrixRow(M& _Mat, int _Row) const			// Set matrix-row

	static V& GetRow(M& _Mat, int _Row)			// Get vector-reference from matrix-row
	static const V& GetRow(const M& _Mat, int _Row) // Get vector-reference from matrix-row
	void SetRow(M& _Mat, int _Row) const			// Set matrix-row

	static void MultiplyMatrix(const V* src, V* dest, const M& m, int n)	// Multiply vector-array by matrix
	void MultiplyMatrix(const M& m)						// Affine matrix multiply
	void MultiplyMatrix3x3(const M& m)					// Matrix multiply
	void GetDirectionX(const M& mat)					// Load matrix column 0
	void GetDirectionY(const M& mat)					// Load matrix column 1
	void GetDirectionZ(const M& mat)					// Load matrix column 2

	// --------------------------------
	// Miscellaneous
	// --------------------------------
	static void GetMinBoundSphere(const V* src, V& p0, T& radius, int n)// Get bounding sphere
	static void GetMinBoundBox(const V* src, V& _min, V& _max, int n)	// Get bounding box
	T Point2LineDistance(const V& p0, const V& l0, const V& l1)			// Point distance to closed line.
	void Project(const V& b, V& r) const								// Project this on b and place result-vector in r.
	static T GetPolygonArea(const V* _pV, int _nV)						// Calculates area of convex polygon
	static T GetIndexedPolygonArea(const V* _pV, uint32* _piV, int _nV)	// Calculates area of convex polygon

	static void Spline(													// 3:rd degree spline defined with 3 points for start and ending.
		V *pMoveA0, V *pMoveA1, V *pMoveA2,
		V *pMoveB0, V *pMoveB1, V *pMoveB2, 
		V* _pDest, T _tFrac, T _tA0, T _tA1, T _tB0, T _tB1, int _nV)


	// --------------------------------
	// Plane-functions (Use CPlane3Dfpx instead)
	// --------------------------------
	void ProjectPlane(const V& n, V& p) const							// Project this into the plane defined by n and 0^ (origo) and put the result in p
	void Reflect(const V& n, V& r) const								// Reflect this on the plane defined by n and 0^ (origo) and put the result in r
	static T IntersectPlane(const V& p0, const V& v0, const V& n)		// Return distance in |v0| units to intersection by the plane defined by n and 0^ (origo)
	int PointOnPlaneSide(const V& n, const V& pi) const					// -1 behind, 0 on plane, 1 front
	int PointOnPlaneSide_Epsilon(const V& n, const V& pi, T _Epsilon) const // -1 behind, 0 on plane, 1 front
	T PlaneDistance(const V& n, const V& pi)							// Distance to plane defined by n and pi.

	// --------------------------------
	// Intersection-functions (Unreliable, don't use without testing!)
	// --------------------------------
	static bool IntersectSphere(const V& p0, T radius, const V& l0, const V& v0)
	static bool IntersectSphere(const V& p0, T radius, const V& l0, const V& v0, T& t)
	static bool IntersectTriangle(const V& p, const V& v, const V& p0, const V& p1, const V& p2)
	static bool IntersectTriangle(const V& p, const V& v, const V& p0, const V& p1, const V& p2, T& s)

	// --------------------------------
	// Rotation convertion
	// --------------------------------
	void CreateAxisRotateMatrix(fp32 _Angle, CMat4Dfp32& _Mat) const		// Create matrix from axis-angle.
	void CreateMatrixFromAngles(int AnglePriority, M& DestMat) const	// Create matrix from euler-angles.
	static T AngleFromVector(T x, T z)									// Get angle from 2D-vector
	static void CreateAngleZYXFromMatrix(V& v, ... )					// Helper-function for CreateAnglesFromMatrix()
	void CreateAnglesFromMatrix(int AnglePriority, const M& m)			// Get euler-angles from matrix

	// --------------------------------
	// Helpers
	// --------------------------------
	CStr GetString()									// Get string for output.
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	void ParseString(CStr _s)							// Parse vector from string with digits separated by anything.
	void ParseColor(CStr& _s, bool _bHexUnit = false)	// Parse color in hex or float notation.
}

------------------------------------------------------------------------------------------------
 TPLANE3
------------------------------------------------------------------------------------------------
class TPlane3
{
public:
	V n;	// Normal
	T d;	// Distance

	// --------------------------------
	// Construction
	// --------------------------------
	void Create(const V& _v, T _a)						// Create from axis-angle
	void Create(const M& _Mat)							// Create from matrix
	TPlane3()
	TPlane3(const TPlane3& _Plane)						// Construct from plane
	TPlane3(const V& _n, T _d)							// Construct from normal + distance
	TPlane3(const V& _n, const V& _p)					// Construct from normal + point
	TPlane3(const V& _p0, const V& _p1, const V& _p2)	// Construct from 3 points
	void Create(const V& _n, const V& _p)				// Create from normal + point
	void Create(const V& _p0, const V& _p1, const V& _p2) // Create from 3 points
	void CreateInverse(const TPlane3 _Plane)			// Create from inverse plane

	void Inverse()										// Inverse
	V GetPointInPlane() const							// Get a point on the plane
	T Distance(const V& v) const						// Point-2-plane distance

	// --------------------------------
	// Point plane-side classification
	// --------------------------------
	int GetPlaneSide(const V& _v) const
	int GetPlaneSide_Epsilon(const V& _v, T _Epsilon) const
	int GetPlaneSideMask_Epsilon(const V& _v, T _Epsilon) const
	int GetArrayPlaneSideMask(const V* _pV, int _nv) const
	int GetArrayPlaneSideMask_Epsilon(const V* _pV, int _nv, T _Epsilon) const

	// --------------------------------
	// Box functions
	// --------------------------------
	int GetBoxPlaneSideMask(const V& _VMin, const V& _VMax) const	// Box plane-side classification
	T GetBoxMinDistance(const V& _VMin, const V& _VMax) const
	T GetBoxMaxDistance(const V& _VMin, const V& _VMax) const

	void GetIntersectionPoint(const V& _p0, const V& _p1, V& _RetV) const
	void Translate(const V& _dV)						// Translate plane by dV
	void Transform(const M& _Mat)						// Affine transform by matrix.

	// --------------------------------
	// Helpers
	// --------------------------------
	CStr GetString()									// Get string for output.
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
}

------------------------------------------------------------------------------------------------
 TVECTOR4
------------------------------------------------------------------------------------------------
class TVector4
{
public:
	T k[4];

	// --------------------------------
	// Construction
	// --------------------------------
	TVector4()
	TVector4(T val)										// Construct and fill with val.
	TVector4(T x, T y, T z, T w)						// Construct from components
	TVector4(const V& a)								// Construct from vector

	// --------------------------------
	// Operators
	// --------------------------------
	T& operator[] (int _k)								// Component index-operator
	const T& operator[] (int _k) const					// Component index-operator
	bool operator== (const V& _v) const					// Equal operator
	bool operator!= (const V& _v) const					// Not equal operator
	V operator+ (const V& a) const						// Vector add
	V operator- (const V& a) const						// Vector subtract
	V operator- () const								// Negate
	void operator+= (const V& a)						// Assignment add
	void operator-= (const V& a)						// Assignment subtract
	T operator* (const V& a) const						// Dotproduct
	V operator* (T scalar) const						// Scale
	void operator*= (T scalar)							// Assignment scale
	V operator* (const M& m) const						// Multiply with matrix
	void operator*= (const M& m)						// Assignment matrix-multiply

	// --------------------------------
	// Operations
	// --------------------------------
	void Scale(T scalar, V& dest) const					// Scale with destination.
	void CompMul(const V& a, V& dest) const				// Multiplies components
	void Add(const V& a, V& dest) const					// Add with destination.
	void Sub(const V& a, V& dest) const					// Subtract with destination.
	void Combine(const V& a, T t, V& dest) const		// Combine (dest = this + a*t) with destination.
	void Lerp(const V& a, T t, V& dest) const			// Interpolates between this and a with t.
	void MultiplyMatrix(const M& m, V& dest) const		// Multiply matrix with destination.
	T Distance(const V& a) const						// Distance between this and a
	T DistanceSqr(const V& a) const						// Square-distance between this and a
	T Length() const									// Length of vector. (Magnitude)
	T LengthSqr() const									// Square-length of vector.
	V& Normalize()										// Normalize this, returning reference

	static void MultiplyMatrix(const V* src, V* dest, const M& m, int n)	// Array matrix transform
	static void MultiplyMatrix(const V3* src, V* dest, const M& m, int n)	// Array matrix transform of v3 to v4
	void operator= (const V3& _v)											// V3 assignment
  
	// --------------------------------
	// Helpers
	// --------------------------------
	CStr GetString()									// Get string for output.
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	void ParseString(CStr _s)							// Parse vector from string with digits separated by anything.
	void ParseColor(CStr& _s, bool _bHexUnit = false)	// Parse color in hex or float notation.
}

------------------------------------------------------------------------------------------------
 TVECTOR2
------------------------------------------------------------------------------------------------
class TVector2
{

public:
	T k[2];

  	// --------------------------------
	// !!!NOT INDEXED!!!
	// --------------------------------
}

------------------------------------------------------------------------------------------------
 TVIEWVECTOR
------------------------------------------------------------------------------------------------
class TViewVector
{
public:
	V4 v;
	ScrV sv;

  	// --------------------------------
	// !!!NOT INDEXED!!!
	// --------------------------------
}

------------------------------------------------------------------------------------------------
 TCOMPLEX
------------------------------------------------------------------------------------------------
class TComplex
{
public:
	T re;
	T im;

	// --------------------------------
	// !!!NOT INDEXED!!!
	// --------------------------------
}

------------------------------------------------------------------------------------------------
 TQUATERNION
------------------------------------------------------------------------------------------------
class TQuaternion
{
public:
	T k[4];

	// --------------------------------
	// Construction
	// --------------------------------
	void Create(const V& _v, T _a)						// Create from axis-angle
	void Create(const M& _Mat)							// Create from matrix
	TQuaternion()
	TQuaternion(const V& _v, T _a)						// Construct from axis-angle
	void Unit()

	// --------------------------------
	// Operations
	// --------------------------------
	void Normalize()
	void Inverse()										// Inverse-rotation
	T DotProd(const TQuaternion& _Q) const				// Dot product
	void Interpolate(const TQuaternion& _Other, TQuaternion& _Dest, fp32 _t) const
	void Multiply(const TQuaternion& _Quat2, TQuaternion& _QDest) const
	void operator*= (const TQuaternion& _Quat2)
	TQuaternion operator* (const TQuaternion& _Quat2) const

	// --------------------------------
	// Convertion
	// --------------------------------
	T CreateAxisAngle(V& _v) const						// Create axis-angle from quaternion
	void CreateMatrix3x3(M& _Mat) const					// Create matrix from quaterion, changing only the 3x3 part.
	void CreateMatrix(M& _Mat) const					// Create matrix from quaterion, setting translation to zero.

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	CStr GetString() const								// Create string for output.
}

------------------------------------------------------------------------------------------------
 TAXISROT
------------------------------------------------------------------------------------------------
class TAxisRot
{
public:
	V m_Axis;
	T m_Angle;

	// --------------------------------
	// Construction
	// --------------------------------
	TAxisRot()
	TAxisRot(const V& _Axis, T _Angle)					// Construct from axis-angle.
	TAxisRot(const Q& _Quat)							// Construct from quaternion.
	TAxisRot(const M& _Mat)								// Construct from matrix.
	void Unit()
	void Create(const Q& _Quat)							// Create from quaternion.
	void Create(const M& _Mat)							// Create from matrix. (via quaternion)
	void Create(T x, T y, T z)							// Create from euler angles. (via quaternions)
	void Create(const V& _Euler)						// Create from euler angles is vector-form. (via quaternions)

	// --------------------------------
	// Operations
	// --------------------------------
	void Normalize()									// Normalize axis.
	void CreateQuaternion(Q& _Quat) const				// Create quaternion from axis-angle.
	void CreateMatrix3x3(M& _Mat) const					// Create matrix from axis-angle.
	void CreateMatrix(M& _Mat) const					// Create matrix from axis-angle.
	void Multiply(const TAxisRot& _Rot)					// Multiply axis-angle rotations. (via quaternions)
	void Multiply(const Q& _Rot)						// Multiply axis-angle with quaterion rotation. (via quaternions)

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	CStr GetString() const								// Get string for output.
}

------------------------------------------------------------------------------------------------
 TBOX, (AABB, Axis-aligned bounding box)
------------------------------------------------------------------------------------------------
class TBox
{
public:
	T m_Min;
	T m_Max;

	// --------------------------------
	// Miscellaneous
	// --------------------------------
	TBox()
	TBox(const T& _Min, const T& _Max)					// Construct from min and max vertices.
	bool IsInside(const TBox &_Box) const				// Box inside another box?
	bool IsCovering(const TBox &_Box) const				// Box covering another box?
	void GetCenter(T& _Center) const					// Get center point.

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	CStr GetString() const								// Get string for output.
}

// -------------------------------------------------------------------
//  TOBB, Oriented bounding box
// -------------------------------------------------------------------
class TOBB
{
public:
	V m_A[3];	// Axis
	V m_E;		// Extents
	V m_C;		// Center

	// -------------------------------------------------------------------
	//  Construction
	// -------------------------------------------------------------------
	void Create(const V& _E, const V& _C)				// Construct from extents (_E) and center origin (_C)
	void Create(const V& _E, const V& _C, const M& _Mat)// Construct from extents (_E), center origin (_C), and orientation (_Mat)
	void Create(const B& _Box)							// Construct from AABB (Axis-aligned bounding box)
	TOBB()
	TOBB(const TOBB& _OBB)								// Copy constructor
	TOBB(const B& _Box)									// Copy constructor from AABB
	void SetDimensions(const V& _E)						// Set extents
	void SetPosition(const M& _Pos)						// Set position and orientation

	// -------------------------------------------------------------------
	//  Operations
	// -------------------------------------------------------------------
	void operator+= (const V& _v)						// Translate
	void operator*= (const M& _Mat)						// Transform
	void Transform(const M& _Mat, TOBB& _Dest) const	// Transform
	void GetVertices(V* _pV) const						// Get the 8 corner vertices for the box.
	void GetBoundBox(B& _Box) const						// Get bounding AABB.
	void GetLocalBox(B& _Box) const						// Get local space AABB
	void TransformToBoxSpace(const V& _v, V& _Dst) const// Transform vertex to box-space
	void TransformFromBoxSpace(const V& _v, V& _Dst) const// Transform vertex from box-space
	bool LocalPointInBox(const CVec3Dfp32 _p0) const		// Box-space point intersection
	bool LocalIntersectLine(const V& _p0, const V& _p1, V& _HitPos) const	// Box-space intersect line
	T LocalMinSqrDistance(const V& _v) const			// Calc minimum square distance from point to box in box-space.
	T LocalMaxSqrDistance(const V& _v) const			// Calc maximum square distance from point to box in box-space.
	T GetMinDistance(const P& _Plane) const				// Calc minimum distance from box to plane. Negative if box is partially or wholly on the back side of the plane.
	T GetMaxDistance(const P& _Plane) const				// Calc maximum distance from box to plane. Positive if box is partially or wholly on the front side of the plane.

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read OBB from file
	void Write(CCFile* _pFile) const					// Write OBB to file
	void SwapLE()
	CStr GetString() const								// Get formatted information string for printing.
}

------------------------------------------------------------------------------------------------
*/
// -------------------------------------------------------------------
#define MOS_MATH_MAXROTS 16

#include "MCCInc.h"
#include "MFile.h"
#include "MFloat.h"

#ifdef PLATFORM_SHINOBI
	#include "DC/CW_Math.h"
	#include "machine.h"
	#include <math.h>
#endif


// -------------------------------------------------------------------
//  Miscellaneous helper macros
// -------------------------------------------------------------------

// OBS! Funkar bara om det finns en class T definerad!
#define Macro_Matrix_SetRow(row, k0, k1, k2, k3)	\
{	k[row][0]=k0; k[row][1]=k1; k[row][2]=k2; k[row][3]=k3; };

// OBS! Funkar bara om det finns en class T definerad!
#define Macro_Matrix_SetRow3x3(row, k0, k1, k2)	\
{	k[row][0]=k0; k[row][1]=k1; k[row][2]=k2; };

// OBS! Funkar bara om det finns en class T definerad!
#define Macro_Matrix_SetRow_Dest(mat, row, k0, k1, k2, k3)	\
{	mat.k[row][0]=k0; mat.k[row][1]=k1; mat.k[row][2]=k2; mat.k[row][3]=k3; };

// OBS! Funkar bara om det finns en class T definerad!
#define Macro_Matrix_SetRow_Dest3x3(mat, row, k0, k1, k2)	\
{	mat.k[row][0]=k0; mat.k[row][1]=k1; mat.k[row][2]=k2; };

// -------------------------------------------------------------------
//  Equation solvers
// -------------------------------------------------------------------
bool SolveP2(const fp32 a, const fp32 b, const fp32 c, fp32& _s0, fp32& _s1);

template <class Real>
int SolveP3(Real fC0, Real fC1, Real fC2, Real fC3, Real& root1, Real& root2, Real& root3);

// -------------------------------------------------------------------
//  Miscellaneous helper templates
// -------------------------------------------------------------------
template<class T>
inline void RotateElements(T& e0, T& e1, T COS, T SIN)
{
	T tmp = e0*COS + e1*SIN;
	e1 = (-e0*SIN) + e1*COS;
	e0 = tmp;
};

template<class T>
inline void RotateElements(T& e0, T& e1, T v)
{
	T s = (T) M_Sin(v*2*_PI);
	T c = (T) M_Cos(v*2*_PI);
	T tmp = e0*c + e1*s;
	e1 = (-e0*s) + e1*c;
	e0 = tmp;
};

template<class T>
inline void RotateElementsDest(T e0, T e1, T COS, T SIN, T& d0, T& d1)
{
	d0 = e0*COS + e1*SIN;
	d1 = (-e0*SIN) + e1*COS;
};

// -------------------------------------------------------------------
template<class T>
T Determinant2x2(T a, T b, T c, T d) { return ((a*d) - (b*c)); };

template<class T>
T Determinant3x3(T a, T b, T c, T d, T e, T f, T g, T h, T i) 
{
	return ((a*Determinant2x2(e,f,h,i)) -
		(b*Determinant2x2(d,f,g,i)) +
		(c*Determinant2x2(d,e,g,h)));
};

template<class T>
T Determinant4x4(T a, T b, T c, T d, T e, T f, T g, T h, T i, T j, T k, T l, T m, T n, T o, T p) 
{
	return 	((a*Determinant3x3(f,g,h,j,k,l,n,o,p)) -
		(b*Determinant3x3(e,g,h,i,k,l,m,o,p)) +
		(c*Determinant3x3(e,f,h,i,j,l,m,n,p)) -
		(d*Determinant3x3(e,f,g,i,j,k,m,n,o)));
};

#define MACRO_DET2x2(V, r0, r1, k0, k1)	\
	Determinant2x2(V[r0][k0], V[r1][k0], V[r0][k1], V[r1][k1])

#define MACRO_DET3x3(V, r0, r1, r2, k0, k1, k2) \
	((V[r0][k0]*Determinant2x2(V[r1][k1], V[r2][k1], V[r1][k2], V[r2][k2])) -	\
	 (V[r1][k0]*Determinant2x2(V[r0][k1], V[r2][k1], V[r0][k2], V[r2][k2])) +	\
	 (V[r2][k0]*Determinant2x2(V[r0][k1], V[r1][k1], V[r0][k2], V[r1][k2])))




// -------------------------------------------------------------------
// Forward-declare

template<class T> class TMatrix43;
template<class T> class TVector3;
template<class T> class TVector3Aggr;
template<class T> class TVector4;
template<class T> class TPlane3;
template<class T> class TQuaternion;

// -------------------------------------------------------------------
// Template-fiffel(tm)

template<class T>
class TMathTemplateProperties
{
public:
	typedef struct { T k[4]; } TMatrix4RowIntrinsic;
	typedef struct { T k[4]; } TVector4Intrinsic;
};


template<>
class TMathTemplateProperties<fp32>
{
public:
	typedef vec128 TMatrix4RowIntrinsic;
	typedef vec128 TVector4Intrinsic;
};

template<>
class TMathTemplateProperties<int32>
{
public:
	typedef vec128 TMatrix4RowIntrinsic;
	typedef vec128 TVector4Intrinsic;
};

template<>
class TMathTemplateProperties<uint32>
{
public:
	typedef vec128 TMatrix4RowIntrinsic;
	typedef vec128 TVector4Intrinsic;
};

// -------------------------------------------------------------------
//  TMatrix2
// -------------------------------------------------------------------
template <class T>
class TMatrix2
{
	typedef TMatrix2 M;

public:
	T k[2][2];  // row, column

	TMatrix2()
	{
	};
	
	TMatrix2(T k00, T k01, T k10, T k11)
	{
		k[0][0] = k00; k[0][1] = k01;
		k[1][0] = k10; k[1][1] = k11;
	};

	void Unit()
	{
		k[0][0] = 1; k[0][1] = 0;
		k[1][0] = 0; k[1][1] = 1;
	};

	void Inverse(M& Dest)
	{
		T d = k[0][0]*k[1][1] - k[1][0]*k[0][1];
		if (d == (T)0.0) Error("Inverse", "Determinant == 0");

		T dinv = T(1.0)/d;
		Dest.k[0][0] = k[0][0]*dinv;
		Dest.k[0][1] = k[1][0]*dinv;
		Dest.k[1][0] = k[0][1]*dinv;
		Dest.k[1][1] = k[1][1]*dinv;
	};
};

typedef TMatrix2<fp32> CMat2Dfp32;
typedef TMatrix2<fp64> CMat2Dfp64;
typedef TMatrix2<int8> CMat2Dint8;
typedef TMatrix2<uint8> CMat2Duint8;
typedef TMatrix2<int16> CMat2Dint16;
typedef TMatrix2<uint16> CMat2Duint16;
typedef TMatrix2<int32> CMat2Dint32;
typedef TMatrix2<uint32> CMat2Duint32;
typedef TMatrix2<int> CMat2Dint;


// -------------------------------------------------------------------
//  TMatrix4
// -------------------------------------------------------------------
template <class T>
class TMatrix4
{
public:
	typedef TMatrix4 M;
	typedef TMatrix43<T> M43;
	typedef TVector3<T> V;

	typedef typename TMathTemplateProperties<T>::TMatrix4RowIntrinsic TRowIntrinsic;

	union
	{
		TRowIntrinsic r[4];
		T k[4][4];  // row, column
	};

	/*
	// --------------------------------
	// Construction/creation
	// --------------------------------
	TMatrix4()
	TMatrix4(T k00, T k01, T k02, T k03, ...)			// Construct from components
	void Unit()
	void Unit3x3()										// Load identity into 3x3.
	void UnitNot3x3()									// Load identity into row & column 4. (!3x3)

	void SetXRotation(T v)								// Load euler rotations.
	void SetYRotation(T v)
	void SetZRotation(T v)
	void SetXRotation3x3(T v)							// Load euler rotations, leaving affine part unchanged.
	void SetYRotation3x3(T v)
	void SetZRotation3x3(T v)

	// --------------------------------
	// Operations
	// --------------------------------
	void M_x_RotX(T v)									// Multiply by euler rotations
	void M_x_RotY(T v)
	void M_x_RotZ(T v)
	void RotX_x_M(T v)									// Backwards multiply by euler rotations
	void RotY_x_M(T v)
	void RotZ_x_M(T v)

	void Transpose()									// Transpose 4x4
	void Transpose3x3()									// Transpose 3x3
	void Transpose(M& DestMat) const					// Transpose 4x4 with destination
	void Transpose3x3(M& DestMat) const					// Transpose 3x3 with destination
	bool Inverse(M& DestMat) const						// Calculate 4x4 inverse with destination
	bool Inverse3x3(M& DestMat) const					// Calculate 3x3 inverse with destination, clearing affine part.
	
	void InverseOrthogonal(M& DestMat) const			// Calculate 4x4 inverse of ortogonal matrix.
	bool IsMirrored() const								// Check if 3x3 part is mirrored.

	void Multiply3x3(T s)								// Scale 3x3
	void Multiply(T s)									// Scale 4x4
	void Multiply(const M& m, M& DestMat) const			// Multiply 4x4 with destination
	void Multiply3x3(const M& m, M& DestMat) const		// Multiply 3x3 with destination, clearing affine part

	void Normalize3x3()									// Normalize 3x3
	void RecreateMatrix<_PrioRow0, _PrioRow1>()			// Renormalize and make it orthogonal (vec128 version)
	void RecreateMatrix(int _Priority0, int _Priority1);// Renormalize and make it orthogonal

	// --------------------------------
	// Helpers
	// --------------------------------
	CStr GetStringRow(int row) const					// Get output string from row
	CStr GetString()									// Get string for output.
  	CStr GetParseString() const
	void ParseString(CStr _s)
	void Read(CCFile* _pFile)
	void Write(CCFile* _pFile) const
	void SwapLE()

	*/

	TMatrix4()
	{
	};
	
	TMatrix4(
		T k00, T k01, T k02, T k03,
		T k10, T k11, T k12, T k13,
		T k20, T k21, T k22, T k23,
		T k30, T k31, T k32, T k33)
	{
		Macro_Matrix_SetRow(0, k00, k01, k02, k03);
		Macro_Matrix_SetRow(1, k10, k11, k12, k13);
		Macro_Matrix_SetRow(2, k20, k21, k22, k23);
		Macro_Matrix_SetRow(3, k30, k31, k32, k33);
	};

	void CreateFrom(const M43& _Src)
	{
		V::GetMatrixRow(_Src, 0).SetMatrixRow(*this, 0);
		V::GetMatrixRow(_Src, 1).SetMatrixRow(*this, 1);
		V::GetMatrixRow(_Src, 2).SetMatrixRow(*this, 2);
		V::GetMatrixRow(_Src, 3).SetMatrixRow(*this, 3);
		k[0][3] = 0;
		k[1][3] = 0;
		k[2][3] = 0;
		k[3][3] = 1.0f;
	}

	void Create(const TQuaternion<T>& _Quaternion, const TVector4<T>& _Translation)
	{
		_Quaternion.CreateMatrix3x3(*this);
		k[0][3] = 0;
		k[1][3] = 0;
		k[2][3] = 0;
		r[3]=_Translation;
		k[3][3]=1.0f;
	}


	void CreateTranslation(const TVector3Aggr<T>& _Translation)
	{
		Macro_Matrix_SetRow(0, 1, 0, 0, 0);
		Macro_Matrix_SetRow(1, 0, 1, 0, 0);
		Macro_Matrix_SetRow(2, 0, 0, 1, 0);
		Macro_Matrix_SetRow(3, _Translation.x, _Translation.y, _Translation.z, 1);
	}

	void CreateTranslation(const TVector4<T>& _Translation)
	{
		Macro_Matrix_SetRow(0, 1, 0, 0, 0);
		Macro_Matrix_SetRow(1, 0, 1, 0, 0);
		Macro_Matrix_SetRow(2, 0, 0, 1, 0);
		r[3]=_Translation;
		k[3][3]=1.0f;
	}

	void Unit()
	{
		Macro_Matrix_SetRow(0, 1, 0, 0, 0);
		Macro_Matrix_SetRow(1, 0, 1, 0, 0);
		Macro_Matrix_SetRow(2, 0, 0, 1, 0);
		Macro_Matrix_SetRow(3, 0, 0, 0, 1);
	};

	void Unit3x3()
	{
		// Load identity into 3x3.
		k[0][0] = (T) 1; k[1][0] = (T) 0; k[2][0] = (T) 0;
		k[0][1] = (T) 0; k[1][1] = (T) 1; k[2][1] = (T) 0;
		k[0][2] = (T) 0; k[1][2] = (T) 0; k[2][2] = (T) 1;
	};

	void UnitNot3x3()
	{
		// Load identity into row & column 4. (!3x3)
		k[0][3] = (T) 0;
		k[1][3] = (T) 0;
		k[2][3] = (T) 0;
		k[3][3] = (T) 1;
		k[3][2] = (T) 0;
		k[3][1] = (T) 0;
		k[3][0] = (T) 0;
	};

	TMatrix4<fp32> Getfp32() const
	{
		return TMatrix4<fp32>(k[0][0], k[0][1], k[0][2], k[0][3], 
											k[1][0], k[1][1], k[1][2], k[1][3], 
											k[2][0], k[2][1], k[2][2], k[2][3],
											k[3][0], k[3][1], k[3][2], k[3][3]);
	}
	
	TMatrix4<fp64> Getfp64() const
	{
		return TMatrix4<fp64>(k[0][0], k[0][1], k[0][2], k[0][3], 
											k[1][0], k[1][1], k[1][2], k[1][3], 
											k[2][0], k[2][1], k[2][2], k[2][3],
											k[3][0], k[3][1], k[3][2], k[3][3]);
	}

	void operator= (const TMatrix4<T>& _M) 
	{
		r[0] = _M.r[0];
		r[1] = _M.r[1];
		r[2] = _M.r[2];
		r[3] = _M.r[3];
	}

	void Assignfp32(TMatrix4<fp32>& _Dst) const
	{
		_Dst.k[0][0] = k[0][0];
		_Dst.k[0][1] = k[0][1];
		_Dst.k[0][2] = k[0][2];
		_Dst.k[0][3] = k[0][3];
		_Dst.k[1][0] = k[1][0];
		_Dst.k[1][1] = k[1][1];
		_Dst.k[1][2] = k[1][2];
		_Dst.k[1][3] = k[1][3];
		_Dst.k[2][0] = k[2][0];
		_Dst.k[2][1] = k[2][1];
		_Dst.k[2][2] = k[2][2];
		_Dst.k[2][3] = k[2][3];
		_Dst.k[3][0] = k[3][0];
		_Dst.k[3][1] = k[3][1];
		_Dst.k[3][2] = k[3][2];
		_Dst.k[3][3] = k[3][3];
	}

	void Assignfp64(TMatrix4<fp64>& _Dst) const
	{
		_Dst.k[0][0] = k[0][0];
		_Dst.k[0][1] = k[0][1];
		_Dst.k[0][2] = k[0][2];
		_Dst.k[0][3] = k[0][3];
		_Dst.k[1][0] = k[1][0];
		_Dst.k[1][1] = k[1][1];
		_Dst.k[1][2] = k[1][2];
		_Dst.k[1][3] = k[1][3];
		_Dst.k[2][0] = k[2][0];
		_Dst.k[2][1] = k[2][1];
		_Dst.k[2][2] = k[2][2];
		_Dst.k[2][3] = k[2][3];
		_Dst.k[3][0] = k[3][0];
		_Dst.k[3][1] = k[3][1];
		_Dst.k[3][2] = k[3][2];
		_Dst.k[3][3] = k[3][3];
	}

	bool AlmostEqual(const M& _m, T _Epsilon) const
	{
		if (M_Fabs(k[0][0]-_m.k[0][0]) > _Epsilon)
			return false;
		if (M_Fabs(k[3][0]-_m.k[3][0]) > _Epsilon)
			return false;

		T ErrSum = 
			/*M_Fabs(_m.k[0][0] - k[0][0]) +*/ M_Fabs(_m.k[0][1] - k[0][1]) + M_Fabs(_m.k[0][2] - k[0][2]) + M_Fabs(_m.k[0][3] - k[0][3]) + 
			M_Fabs(_m.k[1][0] - k[1][0]) + M_Fabs(_m.k[1][1] - k[1][1]) + M_Fabs(_m.k[1][2] - k[1][2]) + M_Fabs(_m.k[1][3] - k[1][3]) + 
			M_Fabs(_m.k[2][0] - k[2][0]) + M_Fabs(_m.k[2][1] - k[2][1]) + M_Fabs(_m.k[2][2] - k[2][2]) + M_Fabs(_m.k[2][3] - k[2][3]) + 
			/*M_Fabs(_m.k[3][0] - k[3][0]) +*/ M_Fabs(_m.k[3][1] - k[3][1]) + M_Fabs(_m.k[3][2] - k[3][2]) + M_Fabs(_m.k[3][3] - k[3][3]);

		return (ErrSum < _Epsilon * T(14.0));
	}

	bool AlmostUnit(T _Epsilon) const
	{
		T ErrSum = 
			M_Fabs(T(1.0) - k[0][0]) + M_Fabs(k[0][1]) + M_Fabs(k[0][2]) + M_Fabs(k[0][3]) + 
			M_Fabs(k[1][0]) + M_Fabs(T(1.0) - k[1][1]) + M_Fabs(k[1][2]) + M_Fabs(k[1][3]) + 
			M_Fabs(k[2][0]) + M_Fabs(k[2][1]) + M_Fabs(T(1.0) - k[2][2]) + M_Fabs(k[2][3]) + 
			M_Fabs(k[3][0]) + M_Fabs(k[3][1]) + M_Fabs(k[3][2]) + M_Fabs(T(1.0) - k[3][3]);
		return (ErrSum < _Epsilon * T(16.0));
	}

	bool AlmostEqual4x3(const M& _m, T _Epsilon) const
	{
		if (M_Fabs(k[0][0]-_m.k[0][0]) > _Epsilon)
			return false;
		if (M_Fabs(k[3][0]-_m.k[3][0]) > _Epsilon)
			return false;

		T ErrSum = 
			/*M_Fabs(_m.k[0][0] - k[0][0]) +*/ M_Fabs(_m.k[0][1] - k[0][1]) + M_Fabs(_m.k[0][2] - k[0][2]) +
			M_Fabs(_m.k[1][0] - k[1][0]) + M_Fabs(_m.k[1][1] - k[1][1]) + M_Fabs(_m.k[1][2] - k[1][2]) +
			M_Fabs(_m.k[2][0] - k[2][0]) + M_Fabs(_m.k[2][1] - k[2][1]) + M_Fabs(_m.k[2][2] - k[2][2]) +
			/*M_Fabs(_m.k[3][0] - k[3][0]) +*/ M_Fabs(_m.k[3][1] - k[3][1]) + M_Fabs(_m.k[3][2] - k[3][2]);

		return (ErrSum < _Epsilon * T(14.0));
	}

	bool AlmostUnit4x3(T _Epsilon) const
	{
		T ErrSum = 
			M_Fabs(T(1.0) - k[0][0]) + M_Fabs(k[0][1]) + M_Fabs(k[0][2]) +
			M_Fabs(k[1][0]) + M_Fabs(T(1.0) - k[1][1]) + M_Fabs(k[1][2]) +
			M_Fabs(k[2][0]) + M_Fabs(k[2][1]) + M_Fabs(T(1.0) - k[2][2]) +
			M_Fabs(k[3][0]) + M_Fabs(k[3][1]) + M_Fabs(k[3][2]);
		return (ErrSum < _Epsilon * T(16.0));
	}

	//---------------------------------------------------
	// 4X4 matrix rotations
	//---------------------------------------------------
	void SetXRotation(T v)
	{
//		v %= 1;
		T s = (T) M_Sin(v*_PI2);
		T c = (T) M_Cos(v*_PI2);

		Macro_Matrix_SetRow(0, 1, 0, 0, 0);
		Macro_Matrix_SetRow(1, 0, c, s, 0);
		Macro_Matrix_SetRow(2, 0,-s, c, 0);
		Macro_Matrix_SetRow(3, 0, 0, 0, 1);
	};

	void SetYRotation(T v)
	{
//		v %= 1;
		T s = (T) M_Sin(v*_PI2);
		T c = (T) M_Cos(v*_PI2);

		Macro_Matrix_SetRow(0, c, 0,-s, 0);
		Macro_Matrix_SetRow(1, 0, 1, 0, 0);
		Macro_Matrix_SetRow(2, s, 0, c, 0);
		Macro_Matrix_SetRow(3, 0, 0, 0, 1);
	};

	void SetZRotation(T v)
	{
//		v %= 1;
		T s = (T) M_Sin(v*_PI2);
		T c = (T) M_Cos(v*_PI2);

		Macro_Matrix_SetRow(0, c, s, 0, 0);
		Macro_Matrix_SetRow(1,-s, c, 0, 0);
		Macro_Matrix_SetRow(2, 0, 0, 1, 0);
		Macro_Matrix_SetRow(3, 0, 0, 0, 1);
	};

	//---------------------------------------------------
	// 3X3 matrix rotations
	//---------------------------------------------------
	void SetXRotation3x3(T v)
	{
//		v %= 1;
		T s = (T) M_Sin(v*_PI2);
		T c = (T) M_Cos(v*_PI2);

		Macro_Matrix_SetRow3x3(0, 1, 0, 0);
		Macro_Matrix_SetRow3x3(1, 0, c, s);
		Macro_Matrix_SetRow3x3(2, 0,-s, c);
	};

	void SetYRotation3x3(T v)
	{
//		v %= 1;
		T s = (T) M_Sin(v*_PI2);
		T c = (T) M_Cos(v*_PI2);

		Macro_Matrix_SetRow3x3(0, c, 0,-s);
		Macro_Matrix_SetRow3x3(1, 0, 1, 0);
		Macro_Matrix_SetRow3x3(2, s, 0, c);
	};

	void SetZRotation3x3(T v)
	{
//		v %= 1;
		T s = (T) M_Sin(v*_PI2);
		T c = (T) M_Cos(v*_PI2);

		Macro_Matrix_SetRow3x3(0, c, s, 0);
		Macro_Matrix_SetRow3x3(1,-s, c, 0);
		Macro_Matrix_SetRow3x3(2, 0, 0, 1);
	};

	//---------------------------------------------------
	// Matrix rotations
	//---------------------------------------------------
	void M_x_RotX(T v)
	{
		// 17 mul, 16 add/sub., 2 trig.
		T v2pi = v*_PI2;
		T s = (T) M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][1], k[0][2], c, s);
		RotateElements(k[1][1], k[1][2], c, s);
		RotateElements(k[2][1], k[2][2], c, s);
	};

	void RotX_x_M(T v)
	{
		// 17 mul, 16 add/sub, 2 trig.
		T v2pi = v*_PI2;
		T s = (T) M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[1][0], k[2][0], c, s);
		RotateElements(k[1][1], k[2][1], c, s);
		RotateElements(k[1][2], k[2][2], c, s);
	};

	void M_x_RotY(T v)
	{
		// 17 mul, 16 add/sub., 2 trig.
		T v2pi = v*_PI2;
		T s = (T) -M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][0], k[0][2], c, s);
		RotateElements(k[1][0], k[1][2], c, s);
		RotateElements(k[2][0], k[2][2], c, s);
	};

	void RotY_x_M(T v)
	{
		// 17 mul, 16 add/sub, 2 trig.
		T v2pi = v*_PI2;
		T s = (T) -M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][0], k[2][0], c, s);
		RotateElements(k[0][1], k[2][1], c, s);
		RotateElements(k[0][2], k[2][2], c, s);
	};

	void M_x_RotZ(T v)
	{
		// 17 mul, 16 add/sub., 2 trig.
		T v2pi = v*_PI2;
		T s = (T) M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][0], k[0][1], c, s);
		RotateElements(k[1][0], k[1][1], c, s);
		RotateElements(k[2][0], k[2][1], c, s);
	};

	void RotZ_x_M(T v)
	{
		// 17 mul, 16 add/sub, 2 trig.
		T v2pi = v*_PI2;
		T s = (T) M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][0], k[1][0], c, s);
		RotateElements(k[0][1], k[1][1], c, s);
		RotateElements(k[0][2], k[1][2], c, s);
	};

	//---------------------------------------------------
	void Transpose()
	{
		Swap(k[0][1], k[1][0]); Swap(k[0][2], k[2][0]); Swap(k[0][3], k[3][0]);
		Swap(k[1][2], k[2][1]); Swap(k[1][3], k[3][1]);
		Swap(k[2][3], k[3][2]);
	};

	void Transpose3x3()
	{
		Swap(k[0][1], k[1][0]); Swap(k[0][2], k[2][0]);
		Swap(k[1][2], k[2][1]);
	};

	void Transpose(M& DestMat) const
	{
		Macro_Matrix_SetRow_Dest(DestMat, 0, (k[0][0]), (k[1][0]), (k[2][0]), (k[3][0]));
		Macro_Matrix_SetRow_Dest(DestMat, 1, (k[0][1]), (k[1][1]), (k[2][1]), (k[3][1]));
		Macro_Matrix_SetRow_Dest(DestMat, 2, (k[0][2]), (k[1][2]), (k[2][2]), (k[3][2]));
		Macro_Matrix_SetRow_Dest(DestMat, 3, (k[0][3]), (k[1][3]), (k[2][3]), (k[3][3]));
	};

	void Transpose3x3(M& DestMat) const
	{
		T a = k[0][0];
		T b = k[1][1];
		T c = k[2][2];
		T d = k[1][0];
		T e = k[2][0];
		T f = k[2][1];
		T g = k[0][1];
		T h = k[0][2];
		T i = k[1][2];

		DestMat.k[0][0] = a;
		DestMat.k[1][1] = b;
		DestMat.k[2][2] = c;

		DestMat.k[0][1] = d;
		DestMat.k[0][2] = e;
		DestMat.k[1][2] = f;

		DestMat.k[1][0] = g;
		DestMat.k[2][0] = h;
		DestMat.k[2][1] = i;
	};

	//---------------------------------------------------
	bool Inverse(M& DestMat) const
	{
		// Determinant:	24/13 mul + 23 add/sub.
		// Inv:			1 div.
		// Transpose:	16 mul.

		// Total:		50/39 mul, 1 div, 23 sub/add.
		// Memory:		17 writes minimum.

		T d = Determinant4x4(
			(k[0][0]), (k[0][1]), (k[0][2]), (k[0][3]),
			(k[1][0]), (k[1][1]), (k[1][2]), (k[1][3]),
			(k[2][0]), (k[2][1]), (k[2][2]), (k[2][3]),
			(k[3][0]), (k[3][1]), (k[3][2]), (k[3][3]));

		if (d == T(0.0))
		{
			DestMat.Unit();
			return false;
		};

		T dInv = T(1.0)/d;
		DestMat.k[0][0] =  dInv*MACRO_DET3x3(k, 1, 2, 3, 1, 2, 3);
		DestMat.k[0][1] = -dInv*MACRO_DET3x3(k, 0, 2, 3, 1, 2, 3);
		DestMat.k[0][2] =  dInv*MACRO_DET3x3(k, 0, 1, 3, 1, 2, 3);
		DestMat.k[0][3] = -dInv*MACRO_DET3x3(k, 0, 1, 2, 1, 2, 3);

		DestMat.k[1][0] = -dInv*MACRO_DET3x3(k, 1, 2, 3, 0, 2, 3);
		DestMat.k[1][1] =  dInv*MACRO_DET3x3(k, 0, 2, 3, 0, 2, 3);
		DestMat.k[1][2] = -dInv*MACRO_DET3x3(k, 0, 1, 3, 0, 2, 3);
		DestMat.k[1][3] =  dInv*MACRO_DET3x3(k, 0, 1, 2, 0, 2, 3);

		DestMat.k[2][0] =  dInv*MACRO_DET3x3(k, 1, 2, 3, 0, 1, 3);
		DestMat.k[2][1] = -dInv*MACRO_DET3x3(k, 0, 2, 3, 0, 1, 3);
		DestMat.k[2][2] =  dInv*MACRO_DET3x3(k, 0, 1, 3, 0, 1, 3);
		DestMat.k[2][3] = -dInv*MACRO_DET3x3(k, 0, 1, 2, 0, 1, 3);

		DestMat.k[3][0] = -dInv*MACRO_DET3x3(k, 1, 2, 3, 0, 1, 2);
		DestMat.k[3][1] =  dInv*MACRO_DET3x3(k, 0, 2, 3, 0, 1, 2);
		DestMat.k[3][2] = -dInv*MACRO_DET3x3(k, 0, 1, 3, 0, 1, 2);
		DestMat.k[3][3] =  dInv*MACRO_DET3x3(k, 0, 1, 2, 0, 1, 2);
		return true;
	};

	bool Inverse3x3(M& DestMat) const
	{
		T d = Determinant3x3((k[0][0]), (k[0][1]), (k[0][2]), 
							(k[1][0]), (k[1][1]), (k[1][2]), 
							(k[2][0]), (k[2][1]), (k[2][2]));
		if (d == T(0.0))
		{
			DestMat.Unit();
			return false;
		};

		T dInv = T(1.0)/d;
		DestMat.k[0][0] =  dInv*MACRO_DET2x2(k, 1, 2, 1, 2);
		DestMat.k[0][1] = -dInv*MACRO_DET2x2(k, 0, 2, 1, 2);
		DestMat.k[0][2] =  dInv*MACRO_DET2x2(k, 0, 1, 1, 2);

		DestMat.k[1][0] = -dInv*MACRO_DET2x2(k, 1, 2, 0, 2);
		DestMat.k[1][1] =  dInv*MACRO_DET2x2(k, 0, 2, 0, 2);
		DestMat.k[1][2] = -dInv*MACRO_DET2x2(k, 0, 1, 0, 2);

		DestMat.k[2][0] =  dInv*MACRO_DET2x2(k, 1, 2, 0, 1);
		DestMat.k[2][1] = -dInv*MACRO_DET2x2(k, 0, 2, 0, 1);
		DestMat.k[2][2] =  dInv*MACRO_DET2x2(k, 0, 1, 0, 1);

		DestMat.UnitNot3x3();
		return true;
	};
	
	void InverseOrthogonal(M& DestMat) const
	{
		T k00 = k[0][0];
		T k11 = k[1][1];
		T k22 = k[2][2];
		T k10 = k[1][0];
		T k20 = k[2][0];
		T k21 = k[2][1];
		T k01 = k[0][1];
		T k02 = k[0][2];
		T k12 = k[1][2];
		T k30 = k[3][0];
		T k31 = k[3][1];
		T k32 = k[3][2];

		DestMat.k[0][0] = k00;
		DestMat.k[1][1] = k11;
		DestMat.k[2][2] = k22;

		DestMat.k[0][1] = k10;
		DestMat.k[0][2] = k20;
		DestMat.k[1][2] = k21;

		DestMat.k[1][0] = k01;
		DestMat.k[2][0] = k02;
		DestMat.k[2][1] = k12;
		DestMat.k[3][0] = -(k30 * k00 + k31 * k01 + k32 * k02);
		DestMat.k[3][1] = -(k30 * k10 + k31 * k11 + k32 * k12);
		DestMat.k[3][2] = -(k30 * k20 + k31 * k21 + k32 * k22);

		DestMat.k[3][3] = 1;
		DestMat.k[0][3] = 0;
		DestMat.k[1][3] = 0;
		DestMat.k[2][3] = 0;
	};

	bool IsMirrored() const
	{
		T x = k[0][1] * k[1][2] - k[0][2] * k[1][1];
		T y = - k[0][0] * k[1][2] + k[0][2] * k[1][0];
		T z = k[0][0] * k[1][1] - k[0][1] * k[1][0];
		return (x*k[2][0] + y*k[2][1] + z*k[2][2] < T(0.0));
	}

	//---------------------------------------------------
	void Multiply3x3(T s)
	{
#ifdef	PLATFORM_PS2
		for (int row=0; row<3; row++)
		{
			k[row][0] *= s;
			k[row][1] *= s;
			k[row][2] *= s;
		}
#else
		for (int row=0; row<3; row++)
			for (int kol=0; kol<3; kol++)
				k[row][kol] *= s;
#endif
	};

	void Multiply(T s)
	{
#ifdef	PLATFORM_PS2
		for (int row=0; row<4; row++)
		{
			k[row][0] *= s;
			k[row][1] *= s;
			k[row][2] *= s;
			k[row][3] *= s;
		}
#else
		for (int row=0; row<4; row++)
			for (int kol=0; kol<4; kol++)
				k[row][kol] *= s;
#endif
	};

	void Multiply(const M& m, M& DestMat) const
	{
		// 64 mul, 48 add, 16 write
		for (int row=0; row<4; row++)
			for (int kol=0; kol<4; kol++)
				DestMat.k[row][kol] = 
					(k[row][0]*m.k[0][kol]) + 
					(k[row][1]*m.k[1][kol]) + 
					(k[row][2]*m.k[2][kol]) + 
					(k[row][3]*m.k[3][kol]);
	};

	void Multiply(const M43& m, M& DestMat) const
	{
		// 64 mul, 48 add, 16 write
		for (int row=0; row<4; row++)
			for (int kol=0; kol<3; kol++)
				DestMat.k[row][kol] = 
					(k[row][0]*m.k[0][kol]) + 
					(k[row][1]*m.k[1][kol]) + 
					(k[row][2]*m.k[2][kol]) + 
					(k[row][3]*m.k[3][kol]);

		for (int row=0; row<4; row++)
				DestMat.k[row][3] = k[row][3];

	};

	void Multiply(const M& m, M43& DestMat) const
	{
		// 64 mul, 48 add, 16 write
		for (int row=0; row<4; row++)
			for (int kol=0; kol<3; kol++)
				DestMat.k[row][kol] = 
					(k[row][0]*m.k[0][kol]) + 
					(k[row][1]*m.k[1][kol]) + 
					(k[row][2]*m.k[2][kol]) + 
					(k[row][3]*m.k[3][kol]);
	};

	void Multiply(const M43& m, M43& DestMat) const
	{
		// 64 mul, 48 add, 16 write
		for (int row=0; row<4; row++)
			for (int kol=0; kol<3; kol++)
				DestMat.k[row][kol] = 
					(k[row][0]*m.k[0][kol]) + 
					(k[row][1]*m.k[1][kol]) + 
					(k[row][2]*m.k[2][kol]) + 
					(k[row][3]*m.k[3][kol]);
	};

	void Multiply3x3(const M& m, M& DestMat) const
	{
		// 27 mul, 18 add, 16 write
		for (int row=0; row<3; row++)
			for (int kol=0; kol<3; kol++)
				DestMat.k[row][kol] = (k[row][0]*m.k[0][kol]) + (k[row][1]*m.k[1][kol]) + (k[row][2]*m.k[2][kol]);

		DestMat.UnitNot3x3();
	};

	void Multiply3x3(const M43& m, M& DestMat) const
	{
		// 27 mul, 18 add, 16 write
		for (int row=0; row<3; row++)
			for (int kol=0; kol<3; kol++)
				DestMat.k[row][kol] = (k[row][0]*m.k[0][kol]) + (k[row][1]*m.k[1][kol]) + (k[row][2]*m.k[2][kol]);

		DestMat.UnitNot3x3();
	}

	//---------------------------------------------------
	void Normalize3x3()
	{
		// Normalizerar "rotations-delen".
		T dividend;
		T inverse;

		dividend = 
			Max(Max(Length3(k[0][0], k[1][0], k[2][0]),
					Length3(k[0][1], k[1][1], k[2][1])),
					Length3(k[0][2], k[1][2], k[2][2]));

		if (dividend != (T)0)
		{
			inverse = T(1.0) / dividend;
			for (int i=0; i<3; i++)
				for (int j=0; j<3; j++)
					k[i][j] = k[i][j]*inverse;
		}
		else
		{
			Unit();
		}
	}

	void NormalizeRows3x3()
	{
		// Normalizerar "rotations-delen".
		T dividend;
		T inverse;

		for(int i = 0; i < 3; i++)
		{
			dividend = (T(Sqr(k[i][0]) + Sqr(k[i][1]) + Sqr(k[i][2])));
			if (dividend != (T)0)
			{
				inverse = M_InvSqrt(dividend);
				k[i][0] = k[i][0]*inverse;
				k[i][1] = k[i][1]*inverse;
				k[i][2] = k[i][2]*inverse;
			}
			else
			{
				k[i][0] = k[i][1] = k[i][2] = 0;
				k[i][i] = (T)1;
			}
		}
	}

	template<int _PrioRow0, int _PrioRow1>
	void RecreateMatrix();
	void RecreateMatrix(int _Priority0, int _Priority1);

	//---------------------------------------------------
	CStr GetStringRow(int row) const
	{
		CStr tmp("(");
		if ((row < 0) || (row > 3)) Error_static("TMatrix4::GetStringRow", "Invalid row.");
		for (int j = 0; j < 4; j++)
			tmp += CStrF("%.3f,", (fp64) k[row][j]);
		tmp = tmp.Del(tmp.Len()-1, 1) + ")";
		return tmp;
	};

	CStr GetString() const
	{
		CStr tmp("(");
		for (int i = 0; i < 4; i++)
			tmp += GetStringRow(i);
		tmp += ")";
		return tmp;
	};

	CStr GetParseString() const
	{
		return CStrF("%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
			k[0][0], k[0][1], k[0][2], k[0][3], 
			k[1][0], k[1][1], k[1][2], k[1][3], 
			k[2][0], k[2][1], k[2][2], k[2][3], 
			k[3][0], k[3][1], k[3][2], k[3][3]);
	}

	M& ParseString(const CStr& _s)
	{
		const char* pStr = (const char*) _s;
		if (!pStr) { /*this = 0;*/ return *this; }
		int pos = 0;
		int len = _s.Len();
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
			{
				pos = CStr::GoToDigit(pStr, pos, len);
				k[i][j] = M_AToF(&pStr[pos]);
				pos = CStr::SkipADigit(pStr, pos, len);
			}
		return *this;
	}

	void Read(CCFile* _pFile)
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				_pFile->ReadLE(k[i][j]);
	}

	void Write(CCFile* _pFile) const
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				_pFile->WriteLE(k[i][j]);
	}
	
#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				::SwapLE(k[i][j]);
	}
#endif
	
	// These functions use the TVector3-class, and are implemented after the TVector3-class
	const V& GetRow(uint _iRow) const;
	      V& GetRow(uint _iRow);
};

typedef TMatrix4<int8> CMat4Dint8;
typedef TMatrix4<int16> CMat4Dint16;
typedef TMatrix4<int32> CMat4Dint32;
typedef TMatrix4<int> CMat4Dint;	// Undefined size integer matrix (ie. >= 32bit)
typedef TMatrix4<fp32> CMat4Dfp32;
typedef TMatrix4<fp64> CMat4Dfp64;
typedef TMatrix43<fp32> CMat43fp32;
//typedef TMatrix43<fp64> CMat43fp64;


#define MACRO_DET3x3_43(V, r0, r1, r2, k0, k1) \
	((V[r0][k0]*Determinant2x2(V[r1][k1], V[r2][k1], 0.0f, 0.0f)) -	\
	 (V[r1][k0]*Determinant2x2(V[r0][k1], V[r2][k1], 0.0f, 0.0f)) +	\
	 (V[r2][k0]*Determinant2x2(V[r0][k1], V[r1][k1], 0.0f, 0.0f)))


// -------------------------------------------------------------------
//  TMatrix43
// -------------------------------------------------------------------
template <class T>
class TMatrix43
{
	static TMatrix43<T> _RotXWorkMat;
	static TMatrix43<T> _RotYWorkMat;
	static TMatrix43<T> _RotZWorkMat;
	
	typedef TMatrix4<T> M44;
	typedef TMatrix43 M;
	typedef TVector3<T> V;

public:
	typedef typename TMathTemplateProperties<T>::TMatrix4RowIntrinsic TRowIntrinsic;

	union
	{
		TRowIntrinsic v[3];
		T k[4][3];  // row, column
	};


	TMatrix43()
	{
	}

	TMatrix43(
		T k00, T k01, T k02,
		T k10, T k11, T k12,
		T k20, T k21, T k22,
		T k30, T k31, T k32)
	{
		Macro_Matrix_SetRow3x3(0, k00, k01, k02);
		Macro_Matrix_SetRow3x3(1, k10, k11, k12);
		Macro_Matrix_SetRow3x3(2, k20, k21, k22);
		Macro_Matrix_SetRow3x3(3, k30, k31, k32);
	}

	void Unit()
	{
		Macro_Matrix_SetRow3x3(0, T(1), T(0), T(0));
		Macro_Matrix_SetRow3x3(1, T(0), T(1), T(0));
		Macro_Matrix_SetRow3x3(2, T(0), T(0), T(1));
		Macro_Matrix_SetRow3x3(3, T(0), T(0), T(0));
	}

	void Unit3x3()
	{	// Load identity into 3x3.
		Macro_Matrix_SetRow3x3(0, T(1), T(0), T(0));
		Macro_Matrix_SetRow3x3(1, T(0), T(1), T(0));
		Macro_Matrix_SetRow3x3(2, T(0), T(0), T(1));
	}

	void UnitNot3x3()
	{	// Load identity into row 4. (!3x3)
		Macro_Matrix_SetRow3x3(3, T(0), T(0), T(0));
	}

	TMatrix43<fp32> Getfp32() const
	{
		return TMatrix43<fp32>(k[0][0], k[0][1], k[0][2],
		                                     k[1][0], k[1][1], k[1][2],
		                                     k[2][0], k[2][1], k[2][2],
		                                     k[3][0], k[3][1], k[3][2]);
	}
	
	TMatrix43<fp64> Getfp64() const
	{
		return TMatrix43<fp64>(k[0][0], k[0][1], k[0][2],
		                                     k[1][0], k[1][1], k[1][2],
		                                     k[2][0], k[2][1], k[2][2],
		                                     k[3][0], k[3][1], k[3][2]);
	}

	void Assignfp32(TMatrix43<fp32>& _Dst) const
	{
		Macro_Matrix_SetRow_Dest3x3(_Dst, 0, k[0][0], k[0][1], k[0][2]);
		Macro_Matrix_SetRow_Dest3x3(_Dst, 1, k[1][0], k[1][1], k[1][2]);
		Macro_Matrix_SetRow_Dest3x3(_Dst, 2, k[2][0], k[2][1], k[2][2]);
		Macro_Matrix_SetRow_Dest3x3(_Dst, 3, k[3][0], k[3][1], k[3][2]);
	}

	void Assignfp64(TMatrix43<fp64>& _Dst) const
	{
		Macro_Matrix_SetRow_Dest3x3(_Dst, 0, k[0][0], k[0][1], k[0][2]);
		Macro_Matrix_SetRow_Dest3x3(_Dst, 1, k[1][0], k[1][1], k[1][2]);
		Macro_Matrix_SetRow_Dest3x3(_Dst, 2, k[2][0], k[2][1], k[2][2]);
		Macro_Matrix_SetRow_Dest3x3(_Dst, 3, k[3][0], k[3][1], k[3][2]);
	}

	void Assign4x4(TMatrix4<T>& _Dst) const
	{
		Macro_Matrix_SetRow_Dest(_Dst, 0, k[0][0], k[0][1], k[0][2], T(0));
		Macro_Matrix_SetRow_Dest(_Dst, 1, k[1][0], k[1][1], k[1][2], T(0));
		Macro_Matrix_SetRow_Dest(_Dst, 2, k[2][0], k[2][1], k[2][2], T(0));
		Macro_Matrix_SetRow_Dest(_Dst, 3, k[3][0], k[3][1], k[3][2], T(1));
	}

	TMatrix4<T> Get4x4() const
	{
		TMatrix4<T> tmp;
		Assign4x4(tmp);
		return tmp;
	}

	void CreateFrom(const TMatrix4<T>& _Src)
	{
		V::GetMatrixRow(_Src, 0).SetMatrixRow(*this, 0);
		V::GetMatrixRow(_Src, 1).SetMatrixRow(*this, 1);
		V::GetMatrixRow(_Src, 2).SetMatrixRow(*this, 2);
		V::GetMatrixRow(_Src, 3).SetMatrixRow(*this, 3);
	}

	bool AlmostEqual(const M& _m, T _Epsilon) const
	{
		if (M_Fabs(k[0][0]-_m.k[0][0]) > _Epsilon)
			return false;
		if (M_Fabs(k[3][0]-_m.k[3][0]) > _Epsilon)
			return false;

		T ErrSum = 
			                               M_Fabs(_m.k[0][1] - k[0][1]) + M_Fabs(_m.k[0][2] - k[0][2]) + 
			M_Fabs(_m.k[1][0] - k[1][0]) + M_Fabs(_m.k[1][1] - k[1][1]) + M_Fabs(_m.k[1][2] - k[1][2]) + 
			M_Fabs(_m.k[2][0] - k[2][0]) + M_Fabs(_m.k[2][1] - k[2][1]) + M_Fabs(_m.k[2][2] - k[2][2]) + 
			                               M_Fabs(_m.k[3][1] - k[3][1]) + M_Fabs(_m.k[3][2] - k[3][2]);

		return (ErrSum < _Epsilon * T(10));
	}

	bool AlmostUnit(T _Epsilon) const
	{
		T ErrSum = 
			M_Fabs(T(1) - k[0][0]) + M_Fabs(k[0][1]) + M_Fabs(k[0][2]) + 
			M_Fabs(k[1][0]) + M_Fabs(T(1) - k[1][1]) + M_Fabs(k[1][2]) + 
			M_Fabs(k[2][0]) + M_Fabs(k[2][1]) + M_Fabs(T(1) - k[2][2]) + 
			M_Fabs(k[3][0]) + M_Fabs(k[3][1]) + M_Fabs(k[3][2]);
		return (ErrSum < _Epsilon * T(16));
	}

	//---------------------------------------------------
	// 4X3 matrix rotations
	//---------------------------------------------------
	void SetXRotation(T v)
	{
		T s = (T) M_Sin(v*T(2)*T(_PI));
		T c = (T) M_Cos(v*T(2)*T(_PI));

		Macro_Matrix_SetRow3x3(0, 1, 0, 0);
		Macro_Matrix_SetRow3x3(1, 0, c, s);
		Macro_Matrix_SetRow3x3(2, 0,-s, c);
		Macro_Matrix_SetRow3x3(3, 0, 0, 0);
	}

	void SetYRotation(T v)
	{
		T s = (T) M_Sin(v*T(2)*T(_PI));
		T c = (T) M_Cos(v*T(2)*T(_PI));

		Macro_Matrix_SetRow3x3(0, c, 0,-s);
		Macro_Matrix_SetRow3x3(1, 0, 1, 0);
		Macro_Matrix_SetRow3x3(2, s, 0, c);
		Macro_Matrix_SetRow3x3(3, 0, 0, 0);
	}

	void SetZRotation(T v)
	{
		T s = (T) M_Sin(v*T(2)*T(_PI));
		T c = (T) M_Cos(v*T(2)*T(_PI));

		Macro_Matrix_SetRow3x3(0, c, s, 0);
		Macro_Matrix_SetRow3x3(1,-s, c, 0);
		Macro_Matrix_SetRow3x3(2, 0, 0, 1);
		Macro_Matrix_SetRow3x3(3, 0, 0, 0);
	}

	//---------------------------------------------------
	// 3X3 matrix rotations
	//---------------------------------------------------
	void SetXRotation3x3(T v)
	{
		T s = (T) M_Sin(v*2*_PI);
		T c = (T) M_Cos(v*2*_PI);

		Macro_Matrix_SetRow3x3(0, 1, 0, 0);
		Macro_Matrix_SetRow3x3(1, 0, c, s);
		Macro_Matrix_SetRow3x3(2, 0,-s, c);
	}

	void SetYRotation3x3(T v)
	{
		T s = (T) M_Sin(v*2*_PI);
		T c = (T) M_Cos(v*2*_PI);

		Macro_Matrix_SetRow3x3(0, c, 0,-s);
		Macro_Matrix_SetRow3x3(1, 0, 1, 0);
		Macro_Matrix_SetRow3x3(2, s, 0, c);
	}

	void SetZRotation3x3(T v)
	{
		T s = (T) M_Sin(v*2*_PI);
		T c = (T) M_Cos(v*2*_PI);

		Macro_Matrix_SetRow3x3(0, c, s, 0);
		Macro_Matrix_SetRow3x3(1,-s, c, 0);
		Macro_Matrix_SetRow3x3(2, 0, 0, 1);
	}

	//---------------------------------------------------
	// Matrix rotations
	//---------------------------------------------------
	void M_x_RotX(T v)
	{
		// 17 mul, 16 add/sub., 2 trig.
		T v2pi = v*2* (T) _PI;
		T s = (T) M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][1], k[0][2], c, s);
		RotateElements(k[1][1], k[1][2], c, s);
		RotateElements(k[2][1], k[2][2], c, s);
	}

	void RotX_x_M(T v)
	{
		// 17 mul, 16 add/sub, 2 trig.
		T v2pi = v*2* (T) _PI;
		T s = (T) M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[1][0], k[2][0], c, s);
		RotateElements(k[1][1], k[2][1], c, s);
		RotateElements(k[1][2], k[2][2], c, s);
	}

	void M_x_RotY(T v)
	{
		// 17 mul, 16 add/sub., 2 trig.
		T v2pi = v*2* (T) _PI;
		T s = (T) -M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][0], k[0][2], c, s);
		RotateElements(k[1][0], k[1][2], c, s);
		RotateElements(k[2][0], k[2][2], c, s);
	}

	void RotY_x_M(T v)
	{
		// 17 mul, 16 add/sub, 2 trig.
		T v2pi = v*2* (T) _PI;
		T s = (T) -M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][0], k[2][0], c, s);
		RotateElements(k[0][1], k[2][1], c, s);
		RotateElements(k[0][2], k[2][2], c, s);
	}

	void M_x_RotZ(T v)
	{
		// 17 mul, 16 add/sub., 2 trig.
		T v2pi = v*2* (T) _PI;
		T s = (T) M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][0], k[0][1], c, s);
		RotateElements(k[1][0], k[1][1], c, s);
		RotateElements(k[2][0], k[2][1], c, s);
	}

	void RotZ_x_M(T v)
	{
		// 17 mul, 16 add/sub, 2 trig.
		T v2pi = v*2* (T) _PI;
		T s = (T) M_Sin(v2pi); T c = (T) M_Cos(v2pi);
		RotateElements(k[0][0], k[1][0], c, s);
		RotateElements(k[0][1], k[1][1], c, s);
		RotateElements(k[0][2], k[1][2], c, s);
	}

	//---------------------------------------------------
	void Transpose3x3()
	{
		Swap(k[0][1], k[1][0]); Swap(k[0][2], k[2][0]);
		Swap(k[1][2], k[2][1]);
	}

	void Transpose3x3(M& DestMat) const
	{
		DestMat.k[0][0] = k[0][0];
		DestMat.k[1][1] = k[1][1];
		DestMat.k[2][2] = k[2][2];

		DestMat.k[0][1] = k[1][0];
		DestMat.k[0][2] = k[2][0];
		DestMat.k[1][2] = k[2][1];
		DestMat.k[1][0] = k[0][1];
		DestMat.k[2][0] = k[0][2];
		DestMat.k[2][1] = k[1][2];
	}

	void Transpose3x3(M44& DestMat) const
	{
		DestMat.k[0][0] = k[0][0];
		DestMat.k[1][1] = k[1][1];
		DestMat.k[2][2] = k[2][2];

		DestMat.k[0][1] = k[1][0];
		DestMat.k[0][2] = k[2][0];
		DestMat.k[1][2] = k[2][1];
		DestMat.k[1][0] = k[0][1];
		DestMat.k[2][0] = k[0][2];
		DestMat.k[2][1] = k[1][2];
	}

	bool Inverse(M& DestMat) const
	{
		// Determinant:	24/13 mul + 23 add/sub.
		// Inv:			1 div.
		// Transpose:	16 mul.

		// Total:		50/39 mul, 1 div, 23 sub/add.
		// Memory:		17 writes minimum.

		T d = Determinant4x4(
			(k[0][0]), (k[0][1]), (k[0][2]), 0.0f,
			(k[1][0]), (k[1][1]), (k[1][2]), 0.0f,
			(k[2][0]), (k[2][1]), (k[2][2]), 0.0f,
			(k[3][0]), (k[3][1]), (k[3][2]), 1.0f);

		if (d == T(0.0))
		{
			DestMat.Unit();
			return false;
		};

		T dInv = T(1.0)/d;
		DestMat.k[0][0] =  dInv*MACRO_DET3x3_43(k, 1, 2, 3, 1, 2);
		DestMat.k[0][1] = -dInv*MACRO_DET3x3_43(k, 0, 2, 3, 1, 2);
		DestMat.k[0][2] =  dInv*MACRO_DET3x3_43(k, 0, 1, 3, 1, 2);

		DestMat.k[1][0] = -dInv*MACRO_DET3x3_43(k, 1, 2, 3, 0, 2);
		DestMat.k[1][1] =  dInv*MACRO_DET3x3_43(k, 0, 2, 3, 0, 2);
		DestMat.k[1][2] = -dInv*MACRO_DET3x3_43(k, 0, 1, 3, 0, 2);

		DestMat.k[2][0] =  dInv*MACRO_DET3x3_43(k, 1, 2, 3, 0, 1);
		DestMat.k[2][1] = -dInv*MACRO_DET3x3_43(k, 0, 2, 3, 0, 1);
		DestMat.k[2][2] =  dInv*MACRO_DET3x3_43(k, 0, 1, 3, 0, 1);

		DestMat.k[3][0] = -dInv*MACRO_DET3x3(k, 1, 2, 3, 0, 1, 2);
		DestMat.k[3][1] =  dInv*MACRO_DET3x3(k, 0, 2, 3, 0, 1, 2);
		DestMat.k[3][2] = -dInv*MACRO_DET3x3(k, 0, 1, 3, 0, 1, 2);
		return true;
	};

	//---------------------------------------------------
	bool Inverse3x3(M& DestMat) const
	{
		T d = Determinant3x3((k[0][0]), (k[0][1]), (k[0][2]), 
		                     (k[1][0]), (k[1][1]), (k[1][2]), 
		                     (k[2][0]), (k[2][1]), (k[2][2]));
		if (d == T(0))
		{
			DestMat.Unit();
			return false;
		}

		T dInv = T(1)/d;
		DestMat.k[0][0] =  dInv*MACRO_DET2x2(k, 1, 2, 1, 2);
		DestMat.k[0][1] = -dInv*MACRO_DET2x2(k, 0, 2, 1, 2);
		DestMat.k[0][2] =  dInv*MACRO_DET2x2(k, 0, 1, 1, 2);

		DestMat.k[1][0] = -dInv*MACRO_DET2x2(k, 1, 2, 0, 2);
		DestMat.k[1][1] =  dInv*MACRO_DET2x2(k, 0, 2, 0, 2);
		DestMat.k[1][2] = -dInv*MACRO_DET2x2(k, 0, 1, 0, 2);

		DestMat.k[2][0] =  dInv*MACRO_DET2x2(k, 1, 2, 0, 1);
		DestMat.k[2][1] = -dInv*MACRO_DET2x2(k, 0, 2, 0, 1);
		DestMat.k[2][2] =  dInv*MACRO_DET2x2(k, 0, 1, 0, 1);

		DestMat.UnitNot3x3();
		return true;
	}

	void InverseOrthogonal(M& DestMat) const
	{
		Transpose3x3(DestMat);
		for (int j = 0; j < 3; j++)
			DestMat.k[3][j] = - (k[3][0]*k[j][0] + k[3][1]*k[j][1] + k[3][2]*k[j][2]);
	}

	void InverseOrthogonal(M44& DestMat) const
	{
		Transpose3x3(DestMat);
		for (int j = 0; j < 3; j++)
			DestMat.k[3][j] = - (k[3][0]*k[j][0] + k[3][1]*k[j][1] + k[3][2]*k[j][2]);

		DestMat.k[3][3] = 1;
		DestMat.k[0][3] = 0;
		DestMat.k[1][3] = 0;
		DestMat.k[2][3] = 0;
	}

	bool IsMirrored() const
	{
		T x =   k[0][1] * k[1][2] - k[0][2] * k[1][1];
		T y = - k[0][0] * k[1][2] + k[0][2] * k[1][0];
		T z =   k[0][0] * k[1][1] - k[0][1] * k[1][0];
		return (x*k[2][0] + y*k[2][1] + z*k[2][2] < T(0));
	}

	//---------------------------------------------------
	void Multiply3x3(T s)
	{
		for (int row=0; row<3; row++)
			for (int kol=0; kol<3; kol++)
				k[row][kol] *= s;
	}

	void Multiply(T s)
	{
		for (int row=0; row<4; row++)
			for (int kol=0; kol<3; kol++)
				k[row][kol] *= s;
	}

	void Multiply3x3(const M& m, M& DestMat) const
	{
		// 27 mul, 18 add, 12 write
		for (int row=0; row<3; row++)
			for (int kol=0; kol<3; kol++)
				DestMat.k[row][kol] = (k[row][0]*m.k[0][kol]) + (k[row][1]*m.k[1][kol]) + (k[row][2]*m.k[2][kol]);

		DestMat.UnitNot3x3();
	}

	void Multiply(const M& m, M& DestMat) const
	{
		// 37 mul, 27 add, 12 write
		int kol;
		for (int row=0; row<3; row++)
			for (kol=0; kol<3; kol++)
				DestMat.k[row][kol] = (k[row][0]*m.k[0][kol]) + (k[row][1]*m.k[1][kol]) + (k[row][2]*m.k[2][kol]);

			for (kol=0; kol<3; kol++)
				DestMat.k[3][kol] = k[3][0]*m.k[0][kol] + k[3][1]*m.k[1][kol] + k[3][2]*m.k[2][kol] + m.k[3][kol];
	}

	void Multiply(const M44& m, M& DestMat) const
	{
		// 37 mul, 27 add, 12 write
		int kol;
		for (int row=0; row<3; row++)
			for (kol=0; kol<3; kol++)
				DestMat.k[row][kol] = (k[row][0]*m.k[0][kol]) + (k[row][1]*m.k[1][kol]) + (k[row][2]*m.k[2][kol]);

			for (kol=0; kol<3; kol++)
				DestMat.k[3][kol] = k[3][0]*m.k[0][kol] + k[3][1]*m.k[1][kol] + k[3][2]*m.k[2][kol] + m.k[3][kol];
	}

	void Multiply(const M& m, M44& DestMat) const
	{
		// 37 mul, 27 add, 12 write
		int kol;
		for (int row=0; row<3; row++)
			for (kol=0; kol<3; kol++)
				DestMat.k[row][kol] = (k[row][0]*m.k[0][kol]) + (k[row][1]*m.k[1][kol]) + (k[row][2]*m.k[2][kol]);

			for (kol=0; kol<3; kol++)
				DestMat.k[3][kol] = k[3][0]*m.k[0][kol] + k[3][1]*m.k[1][kol] + k[3][2]*m.k[2][kol] + m.k[3][kol];

		DestMat.k[0][3] = T(0);
		DestMat.k[1][3] = T(0);
		DestMat.k[2][3] = T(0);
		DestMat.k[3][3] = T(1);
	}

	void Multiply(const M44& m, M44& DestMat) const
	{
		// 48 mul, 36 add
		int kol;
		for (int row = 0; row < 3; row++)
		{
			for (kol = 0; kol < 3; kol++)
				DestMat.k[row][kol] = (k[row][0]*m.k[0][kol]) + (k[row][1]*m.k[1][kol]) + (k[row][2]*m.k[2][kol]);

			DestMat.k[row][3] = (k[row][0]*m.k[0][3]) + (k[row][1]*m.k[1][3]) + (k[row][2]*m.k[2][3]);
		}

			for (kol = 0; kol < 3; kol++)
				DestMat.k[3][kol] = k[3][0]*m.k[0][kol] + k[3][1]*m.k[1][kol] + k[3][2]*m.k[2][kol] + m.k[3][kol];

			DestMat.k[3][3] = (k[3][0]*m.k[0][3]) + (k[3][1]*m.k[1][3]) + (k[3][2]*m.k[2][3]) + m.k[3][3];
	}

	//---------------------------------------------------
	void Normalize3x3()
	{
		// Normalizerar "rotations-delen".
		T dividend;
		T inverse;

		dividend = 
			Max(Max(Length3(k[0][0], k[1][0], k[2][0]),
					Length3(k[0][1], k[1][1], k[2][1])),
					Length3(k[0][2], k[1][2], k[2][2]));

		if (dividend != (T)0)
		{
			inverse = T(1.0) / dividend;
			for (int i=0; i<3; i++)
				for (int j=0; j<3; k[i][j] = k[i][j++]*inverse);
		}
		else
		{
			Unit();
		}
	}

	void NormalizeRows3x3()
	{
		// Normalizerar "rotations-delen".
		T dividend;
		T inverse;

		for(int i = 0; i < 3; i++)
		{
			dividend = (T(Sqr(k[i][0]) + Sqr(k[i][1]) + Sqr(k[i][2])));
			if (dividend != (T)0)
			{
				inverse = M_InvSqrt(dividend);
				k[i][0] = k[i][0]*inverse;
				k[i][1] = k[i][1]*inverse;
				k[i][2] = k[i][2]*inverse;
			}
			else
			{
				k[i][0] = k[i][1] = k[i][2] = 0;
				k[i][i] = (T)1;
			}
		}
	}
	
	void RecreateMatrix(int _Priority0, int _Priority1);
	

	//---------------------------------------------------
	CStr GetStringRow(int row) const
	{
		CStr tmp("(");
		if ((row < 0) || (row > 3)) Error_static("TMatrix43::GetStringRow", "Invalid row.");
		for (int j = 0; j < 3; j++)
			tmp += CStrF("%.3f,", (fp64) k[row][j]);
		tmp = tmp.Del(tmp.Len()-1, 1) + ")";
		return tmp;
	}

	CStr GetString() const
	{
		CStr tmp("(");
		for (int i = 0; i < 4; i++)
			tmp += GetStringRow(i);
		tmp += ")";
		return tmp;
	}

	CStr GetParseString() const
	{
		return CStrF("%f, %f, %f, 0.0, %f, %f, %f, 0.0, %f, %f, %f, 0.0, %f, %f, %f, 1.0",
			k[0][0], k[0][1], k[0][2], 
			k[1][0], k[1][1], k[1][2], 
			k[2][0], k[2][1], k[2][2],
			k[3][0], k[3][1], k[3][2]);
	}

	M& ParseString(const CStr& _s)
	{
		const char* pStr = (const char*) _s;
		if (!pStr) { /*this = 0;*/ return *this; }
		int pos = 0;
		int len = _s.Len();
		for(int i = 0; i < 4; i++)
		{
			for(int j = 0; j < 3; j++)
			{
				pos = CStr::GoToDigit(pStr, pos, len);
				k[i][j] = M_AToF(&pStr[pos]);
				pos = CStr::SkipADigit(pStr, pos, len);
			}
			pos = CStr::GoToDigit(pStr, pos, len);
			pos = CStr::SkipADigit(pStr, pos, len);
		}
		return *this;
	}

	void Read(CCFile* _pFile)
	{
		for(int i = 0; i < 4; i++)
		{
			for(int j = 0; j < 3; j++)
				_pFile->ReadLE(k[i][j]);

			T tmp;
			_pFile->ReadLE(tmp);
		}
	}

	void Write(CCFile* _pFile) const
	{
		for(int i = 0; i < 4; i++)
		{
			for(int j = 0; j < 3; j++)
				_pFile->WriteLE(k[i][j]);

			if (i<3)
				_pFile->WriteLE(T(0));
			else
				_pFile->WriteLE(T(1));
		}
	}
	
#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 3; j++)
				::SwapLE(k[i][j]);
	}
#endif

	// These functions use the TVector3-class, and are implemented after the TVector3-class
	const V& GetRow(uint _iRow) const;
	      V& GetRow(uint _iRow);
};

// typedef TMatrix4<fp10> CMat4Dfp10;

// -------------------------------------------------------------------
//  TVector3
// -------------------------------------------------------------------
template <class T>
class TVector3Aggr
{
public:
	T k[3];
	typedef TMatrix4<T> M;
	typedef TVector3Aggr V;
//	typename TVector3<T>;
	typedef TVector3<T> V3;
	typedef T CDataType;
	typedef TMatrix43<T> M43;

	/*
	// --------------------------------
	// Construction
	// --------------------------------
	void Create(const V& _v, T _a)						// Create from axis-angle
	void Create(const M& _Mat)							// Create from matrix

	TVector3()
	TVector3(T val)										// Construct and fill with val.
	TVector3(T x, T y, T z)								// Construct from components
	TVector3(const V& a)								// Construct from vector

	// --------------------------------
	// Convertion
	// --------------------------------
	TVector3<fp32> Getfp32()								// Get float(fp32) vector
	TVector3<fp64> Getfp64()								// Get double(fp64) vector
	void Assignfp32(TVector3<fp32>& _Dst)					// Assign to float vector
	void Assignfp64(TVector3<fp64>& _Dst)					// Assign to double vector

	// --------------------------------
	// Operators
	// --------------------------------
	T& operator[] (int _k)								// Component index-operator
	const T& operator[] (int _k) const					// Component index-operator
	bool operator== (const V& _v) const					// Equal operator
	bool operator!= (const V& _v) const					// Not equal operator
	bool AlmostEqual(const V& _v, T _Epsilon) const		// Compare with epsilon
	V operator+ (const V& a) const						// Vector add
	V operator- (const V& a) const						// Vector subtract
	V operator- () const								// Negate
	void operator+= (const V& a)						// Assignment add
	void operator-= (const V& a)						// Assignment subtract
	T operator* (const V& a) const						// Dotproduct
	V operator* (T scalar) const						// Scale
	void operator*= (T scalar)							// Assignment scale
	V operator* (const M& m) const						// Multiply with matrix
	void operator*= (const M& m)						// Assignment matrix-multiply
	V operator/ (const V& a) const						// Crossproduct

	// --------------------------------
	// Operations
	// --------------------------------
	void Scale(T scalar, V& dest) const					// Scale with destination.
	void CrossProd(const V& a, V& dest) const			// Crossproduct with destination.
	void Add(const V& a, V& dest) const					// Add with destination.
	void Sub(const V& a, V& dest) const					// Subtract with destination.
	void Combine(const V& a, T t, V& dest) const		// Combine (dest = this + a*t) with destination.
	void MultiplyMatrix(const M& m, V& dest) const		// Multiply matrix with destination.
	T Length() const									// Length of vector. (Magnitude)
	T LengthSqr() const									// Square-length of vector.
	V& Normalize()										// Normalize this, returning reference

	// --------------------------------
	// Matrix load/store/multiply
	// --------------------------------
	void AddMatrixRow(M& m, int row) const				// Add matrix-row
	void AddMatrixColumn(M& m, int column) const		// Add matrix-column
	static V& GetMatrixRow(M& _Mat, int _Row)			// Get vector-reference from matrix-row
	static const V& GetMatrixRow(const M& _Mat, int _Row) // Get vector-reference from matrix-row
	void SetMatrixRow(M& _Mat, int _Row) const			// Set matrix-row

	static void MultiplyMatrix(const V* src, V* dest, const M& m, int n)	// Multiply vector-array by matrix
	void MultiplyMatrix(const M& m)						// Affine matrix multiply
	void MultiplyMatrix3x3(const M& m)					// Matrix multiply
	void GetDirectionX(const M& mat)					// Load matrix column 0
	void GetDirectionY(const M& mat)					// Load matrix column 1
	void GetDirectionZ(const M& mat)					// Load matrix column 2

	// --------------------------------
	// Miscellaneous
	// --------------------------------
	static void GetMinBoundSphere(const V* src, V& p0, T& radius, int n)// Get bounding sphere
	static void GetMinBoundBox(const V* src, V& _min, V& _max, int n)	// Get bounding box
	T Point2LineDistance(const V& p0, const V& l0, const V& l1)			// Point distance to closed line.
	void Project(const V& b, V& r) const								// Project this on b and place result-vector in r.
	static T GetPolygonArea(const V* _pV, int _nV)						// Calculates area of convex polygon
	static T GetIndexedPolygonArea(const V* _pV, uint32* _piV, int _nV)	// Calculates area of convex polygon

	static void Spline(													// 3:rd degree spline defined with 3 points for start and ending.
		V *pMoveA0, V *pMoveA1, V *pMoveA2,
		V *pMoveB0, V *pMoveB1, V *pMoveB2, 
		V* _pDest, T _tFrac, T _tA0, T _tA1, T _tB0, T _tB1, int _nV)


	// --------------------------------
	// Plane-functions (Use CPlane3Dfpx instead)
	// --------------------------------
	void ProjectPlane(const V& n, V& p) const							// Project this into the plane defined by n and 0^ (origo) and put the result in p
	void Reflect(const V& n, V& r) const								// Reflect this on the plane defined by n and 0^ (origo) and put the result in r
	static T IntersectPlane(const V& p0, const V& v0, const V& n)		// Return distance in |v0| units to intersection by the plane defined by n and 0^ (origo)
	int PointOnPlaneSide(const V& n, const V& pi) const					// -1 behind, 0 on plane, 1 front
	int PointOnPlaneSide_Epsilon(const V& n, const V& pi, T _Epsilon) const // -1 behind, 0 on plane, 1 front
	T PlaneDistance(const V& n, const V& pi)							// Distance to plane defined by n and pi.

	// --------------------------------
	// Rotation convertion
	// --------------------------------
	void CreateAxisRotateMatrix(fp32 _Angle, CMat4Dfp32& _Mat) const		// Create matrix from axis-angle.
	void CreateMatrixFromAngles(int AnglePriority, M& DestMat) const	// Create matrix from euler-angles.
	static T AngleFromVector(T x, T z)									// Get angle from 2D-vector
	static void CreateAngleZYXFromMatrix(V& v, ... )					// Helper-function for CreateAnglesFromMatrix()
	void CreateAnglesFromMatrix(int AnglePriority, const M& m)			// Get euler-angles from matrix

	// --------------------------------
	// Helpers
	// --------------------------------
	CStr GetString()									// Get string for output.
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	void ParseString(CStr _s)							// Parse vector from string with digits separated by anything.

	*/



	// kma: Warning, if you use 1.0 as max you can get sign errors, use 1.01f
	V& Unpack32(uint32 _V, fp32 _Max)
	{
		const fp32 Max2 = _Max + _Max;
		k[0] = ((fp32((_V & 0xffe00000) >> 21) * (1.0f/ 2048.0f)) - 0.5f) * Max2;
		k[1] = ((fp32((_V & 0x001ffc00) >> 10) * (1.0f/ 2048.0f)) - 0.5f) * Max2;
		k[2] = ((fp32(_V & 0x000003ff) * (1.0f/ 1024.0f)) - 0.5f) * Max2;
		return *this;
	}

	// kma: Warning, if you use 1.0 as max you can get sign errors, use 1.01f
	uint32 Pack32(fp32 _Max) const
	{
		const fp32 Max2 = _Max + _Max;
		return Clamp(uint32((k[0] * (1.0f/ Max2) + 0.5f) * 2048.0f),0,2047) << 21 |
			   Clamp(uint32((k[1] * (1.0f/ Max2) + 0.5f) * 2048.0f),0,2047) << 10 |
			   Clamp(uint32((k[2] * (1.0f/ Max2) + 0.5f) * 1024.0f),0,1023);
	}

	// Added by Mondelore {START}.
	V& Unpack24(uint32 _V, T _Max)
	{
		const fp32 Max2 = _Max + _Max;
		k[0] = ((T(_V & 0x00ff0000) / ((T)(128 * 256 * 256))) - (T)0.5) * Max2;
		k[1] = ((T(_V & 0x0000ff00) / ((T)(128 * 256))) - (T)0.5) * Max2;
		k[2] = ((T(_V & 0x000000ff) / ((T)(128))) - (T)0.5) * Max2;
		return *this;
	}

	uint32 Pack24(T _Max) const
	{
		return uint32((k[0] / (_Max * 2) + (T)0.5) * (T)(128 * 256 * 256)) & 0x00ff0000 |
			   uint32((k[1] / (_Max * 2) + (T)0.5) * (T)(128 * 256)) & 0x0000ff00 |
			   uint32((k[2] / (_Max * 2) + (T)0.5) * (T)(128)) & 0x000000ff;
	}
	// Added by Mondelore {END}.

	V& Unpack16(uint16 _V, fp32 _Max)
	{
		const fp32 Max2 = _Max + _Max;
		k[0] = ((fp32(_V & 0xfc00) * (1.0f/ (32.0f * 32 * 64))) - 0.5f) * Max2;
		k[1] = ((fp32(_V & 0x03e0) * (1.0f/ (32.0f * 32))) - 0.5f) * Max2;
		k[2] = ((fp32(_V & 0x001f) * (1.0f/ (32.0f))) - 0.5f) * Max2;
		return *this;
	}

	uint16 Pack16(fp32 _Max) const
	{
		const fp32 Max2 = _Max + _Max;
		return uint16((k[0] * (1.0f/ Max2) + 0.5f) * (32.0f * 32 * 64)) & 0xfc00 |
			   uint16((k[1] * (1.0f/ Max2) + 0.5f) * (32.0f * 32)) & 0x03e0 |
			   uint16((k[2] * (1.0f/ Max2) + 0.5f) * 32.0f) & 0x001f;
	}

	template<typename T2>
	TVector3<T2> Get() const
	{
		return TVector3<T2>(T2(k[0]), T2(k[1]), T2(k[2]));
	}

	template<typename T2>
	void Assign(TVector3<T2>& _Dst) const
	{
		_Dst.k[0] = T2(k[0]);
		_Dst.k[1] = T2(k[1]);
		_Dst.k[2] = T2(k[2]);
	}

	void Set(T _x, T _y, T _z)
	{
		k[0] = _x;
		k[1] = _y;
		k[2] = _z;
	}

	void SetScalar(T _v)
	{
		k[0] = _v;
		k[1] = _v;
		k[2] = _v;
	}


	TVector3Aggr<fp32> Getfp32() const { return Get<fp32>(); }
	TVector3Aggr<fp64> Getfp64() const { return Get<fp64>(); }

	void Assignfp32(TVector3<fp32>& _Dst) const { Assign<fp32>(_Dst); }
	void Assignfp64(TVector3<fp64>& _Dst) const { Assign<fp64>(_Dst); }

	T& operator[] (int _k)
	{
		return k[_k];
	}

	const T& operator[] (int _k) const
	{
		return k[_k];
	}

	bool operator== (const V& _v) const
	{
		return ((k[0] == _v.k[0]) && (k[1] == _v.k[1]) && (k[2] == _v.k[2]));
	};

	bool operator!= (const V& _v) const
	{
		return ((k[0] != _v.k[0]) || (k[1] != _v.k[1]) || (k[2] != _v.k[2]));
	};

	bool AlmostEqual(const V& _v, T _Epsilon) const
	{
		if (Abs(k[0] - _v.k[0]) > _Epsilon) return false;
		if (Abs(k[1] - _v.k[1]) > _Epsilon) return false;
		if (Abs(k[2] - _v.k[2]) > _Epsilon) return false;
		return true;
	}

	V3 operator+ (const V& a) const
	{
		return V3(k[0]+a.k[0], k[1]+a.k[1], k[2]+a.k[2]);
	}
	
	V3 operator- (const V& a) const
	{
		return V3(k[0]-a.k[0],k[1]-a.k[1],k[2]-a.k[2]);
	};

	V3 operator- () const
	{
		return V3(-k[0],-k[1],-k[2]);
	};

	void operator+= (const V& a)
	{
		k[0] += a.k[0];
		k[1] += a.k[1];
		k[2] += a.k[2];
	};
	
	void operator-= (const V& a)
	{
		k[0] -= a.k[0];
		k[1] -= a.k[1];
		k[2] -= a.k[2];
	};
	
	void operator+= (T a)
	{
		k[0] += a;
		k[1] += a;
		k[2] += a;
	};
	
	void operator-= (T a)
	{
		k[0] -= a;
		k[1] -= a;
		k[2] -= a;
	};
	
	T operator* (const V& a) const
	{
		return (k[0]*a.k[0] + k[1]*a.k[1] + k[2]*a.k[2]);
	};

	V3 operator* (T scalar) const
	{
		return V3(k[0]*scalar, k[1]*scalar, k[2]*scalar);
	};

	void operator*= (T scalar) 
	{
		k[0] *= scalar;
		k[1] *= scalar;
		k[2] *= scalar;
	};

	/* Move to global operator due to compiler bug (create cylinder in Ogier) - JA 060228
	
	V operator* (const M& m) const
	{
		V r;
		for (int i=0; i<3; i++)
			r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
		return r;
	};*/

	void operator*= (const M& m)
	{
		V r;
		for (int i=0; i<3; i++)
			r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
		k[0] = r.k[0];
		k[1] = r.k[1];
		k[2] = r.k[2];
	};

	V operator/ (const V& a) const
	{
		V Vec;
		CrossProd(a, Vec);
		return Vec;
//		return V(k[1]*a.k[2] - k[2]*a.k[1], -k[0]*a.k[2] + k[2]*a.k[0], k[0]*a.k[1] - k[1]*a.k[0]);
	}

	V3 operator/ (T a) const
	{
		T arecp = (T)1 / a;
		return V3(k[0] * arecp, k[1] * arecp, k[2] * arecp);
	}

	void Scale(T scalar, V& dest) const
	{
		dest.k[0] = k[0]*scalar;
		dest.k[1] = k[1]*scalar;
		dest.k[2] = k[2]*scalar;
	}

	void CrossProd(const V& a, V& dest) const
	{
		dest.k[0] = k[1]*a.k[2] - k[2]*a.k[1];
		dest.k[1] = -k[0]*a.k[2] + k[2]*a.k[0];
		dest.k[2] = k[0]*a.k[1] - k[1]*a.k[0];
	}

	void CompMul(const V& a, V& dest) const
	{
		dest.k[0] = k[0] * a.k[0];
		dest.k[1] = k[1] * a.k[1];
		dest.k[2] = k[2] * a.k[2];
	}

	void Add(const V& a, V& dest) const
	{
		dest.k[0] = k[0] + a.k[0];
		dest.k[1] = k[1] + a.k[1];
		dest.k[2] = k[2] + a.k[2];
	}

	void Sub(const V& a, V& dest) const
	{
		dest.k[0] = k[0] - a.k[0];
		dest.k[1] = k[1] - a.k[1];
		dest.k[2] = k[2] - a.k[2];
	}

	void Combine(const V& a, T t, V& dest) const
	{
		dest.k[0] = k[0] + t*a.k[0];
		dest.k[1] = k[1] + t*a.k[1];
		dest.k[2] = k[2] + t*a.k[2];
	}

	void Lerp(const V& _Other, T t, V& dest) const
	{
		T invt = 1.0f - t;

		T k0 = k[0];
		T k1 = k[1];
		T k2 = k[2];
		T o0 = _Other.k[0];
		T o1 = _Other.k[1];
		T o2 = _Other.k[2];

		dest.k[0] = k0 * invt + t * o0;
		dest.k[1] = k1 * invt + t * o1;
		dest.k[2] = k2 * invt + t * o2;
	}
	
	void Moderate(const V& newq, V& qprim, int a)
	{
		const T Scale = T(512.0);
		for(int i = 0; i < 3; i++)
		{
			int x = newq.k[i]*Scale;
			int xprim = qprim.k[i]*Scale;
			Moderate(x, newq.k[i]*Scale, xprim, a);
			newq.k[i] = T(x) / Scale;
			qprim.k[i] = T(xprim) / Scale;
		}
	}

	void MultiplyMatrix(const M& m, V& dest) const
	{
		for (int i = 0; i < 3; i++)
			dest.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
	}

	M_INLINE T Distance(const V& a) const
	{
		return (T)M_Sqrt(Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]) + Sqr(k[2] - a.k[2]));
	}

	M_INLINE T DistanceInv(const V& a) const
	{
		return (T)M_InvSqrt(Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]) + Sqr(k[2] - a.k[2]));
	}

	M_INLINE T DistanceSqr(const V& a) const
	{
		return Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]) + Sqr(k[2] - a.k[2]);
	}

	M_INLINE T LengthInv() const
	{
		return (T(M_InvSqrt(Sqr(k[0]) + Sqr(k[1]) + Sqr(k[2]))));
	}

	M_INLINE T Length() const
	{
		return (T(M_Sqrt(Sqr(k[0]) + Sqr(k[1]) + Sqr(k[2]))));
	}

	M_INLINE T LengthSqr() const
	{
		return Sqr(k[0]) + Sqr(k[1]) + Sqr(k[2]);
	}

#if defined(PLATFORM_PS2) || defined(PLATFORM_DOLPHIN)
	M_INLINE V& Normalize()
	{
		T inverse = LengthInv();
//		M_ASSERT( dividend != T(0), "Null-vector is _bad_ input.");

		k[0] = k[0]*inverse;
		k[1] = k[1]*inverse;
		k[2] = k[2]*inverse;

		return *this;
	}
	
	M_INLINE V& SetLength( const T& _Length )
	{
		T scale = _Length * LengthInv();
//		M_ASSERT( dividend != T(0), "Null-vector is _bad_ input.");

		k[0] = k[0]*scale;
		k[1] = k[1]*scale;
		k[2] = k[2]*scale;

		return *this;
	}
#else
	M_INLINE V& Normalize()
	{
		T s = LengthSqr();
	//	M_ASSERT(s != T(0), "WARNING: Don't normalize null vectors! (consider using SafeNormalize()?)");

		s = M_FSel(-s, T(1), s);	// protect against 0 vector
		s = M_InvSqrt(s);

		T x = k[0] * s;
		T y = k[1] * s;
		T z = k[2] * s;
		k[0] = x;
		k[1] = y;
		k[2] = z;
		return *this;
	}

	M_INLINE V& SafeNormalize()
	{
		T s = LengthSqr();
		s = M_FSel(-s, T(1), s);	// protect against 0 vector
		s = M_InvSqrt(s);

		T x = k[0] * s;
		T y = k[1] * s;
		T z = k[2] * s;
		k[0] = x;
		k[1] = y;
		k[2] = z;
		return *this;
	}

	M_INLINE V& SetLength(T _Length)
	{
		T s = LengthSqr();
	//	M_ASSERT(s != T(0), "WARNING: Don't normalize null vectors!");

		s = M_FSel(-s, T(1), s);	// protect against 0 vector
		s = _Length * M_InvSqrt(s);

		T x = k[0] * s;
		T y = k[1] * s;
		T z = k[2] * s;
		k[0] = x;
		k[1] = y;
		k[2] = z;
		return *this;
	}

	M_INLINE V& SetMaxLength(T _MaxLen)
	{
		T curr2 = LengthSqr();
		T new2 = Min(curr2, Sqr(_MaxLen));			// don't go above MaxLen
		curr2 = M_FSel(-curr2, T(1), curr2);		// protect against 0 vector
		T s = M_Sqrt(new2) * M_InvSqrt(curr2);		// v *= sqrt(newlen^2 / currlen^2)
		T x = k[0] * s;
		T y = k[1] * s;
		T z = k[2] * s;
		k[0] = x;
		k[1] = y;
		k[2] = z;
		return *this;
	}

#endif	

	//---------------------------------------------------
	void AddMatrixRow(M& m, int row) const
	{
		m.k[row][0] += k[0];
		m.k[row][1] += k[1];
		m.k[row][2] += k[2];
	}

	void AddMatrixColumn(M& m, int column) const
	{
		m.k[0][column] += k[0];
		m.k[1][column] += k[1];
		m.k[2][column] += k[2];
	}

	static V& GetMatrixRow(M& _Mat, int _Row)
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static const V& GetMatrixRow(const M& _Mat, int _Row)
	{
		const V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	void SetMatrixRow(M& _Mat, int _Row) const
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		*pV = *this;
	}

	// Same functions, shorter names.
	static V& GetRow(M& _Mat, int _Row)
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static const V& GetRow(const M& _Mat, int _Row)
	{
		const V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	void SetRow(M& _Mat, int _Row) const
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		*pV = *this;
	}

	void SetCol(M& _Mat, int _iCol) const
	{
		_Mat.k[0][_iCol] = k[0];
		_Mat.k[1][_iCol] = k[1];
		_Mat.k[2][_iCol] = k[2];
	}

	//---------------------------------------------------
	static void MultiplyMatrix(const V* src, V* dest, const M& m, int n)
	{
		for (int vnr = 0; vnr<n; vnr++) {
			for (int i=0; i<3; i++)
				dest[vnr].k[i] = m.k[0][i]*src[vnr].k[0] + m.k[1][i]*src[vnr].k[1] + m.k[2][i]*src[vnr].k[2] + m.k[3][i];
		}
	}

	void MultiplyMatrix(const M& m)
	{
		T x = k[0];
		T y = k[1];
		T z = k[2];
		for (int i = 0; i < 3; i++)
			k[i] = m.k[0][i]*x + m.k[1][i]*y + m.k[2][i]*z + m.k[3][i];
	}

	void MultiplyMatrix3x3(const M& m)
	{
		T x = k[0];
		T y = k[1];
		T z = k[2];
		for (int i = 0; i < 3; i++)
			k[i] = m.k[0][i]*x + m.k[1][i]*y + m.k[2][i]*z;
	}

	void MultiplyMatrix3x3(const M& m, V &_d) const
	{
		T x = k[0];
		T y = k[1];
		T z = k[2];
		for (int i = 0; i < 3; i++)
			_d.k[i] = m.k[0][i]*x + m.k[1][i]*y + m.k[2][i]*z;
	}

	void GetDirectionX(const M& mat)
	{
		k[0] = mat.k[0][0];
		k[1] = mat.k[1][0];
		k[2] = mat.k[2][0];
	}

	void GetDirectionY(const M& mat)
	{
		k[0] = mat.k[0][1];
		k[1] = mat.k[1][1];
		k[2] = mat.k[2][1];
	}

	void GetDirectionZ(const M& mat)
	{
		k[0] = mat.k[0][2];
		k[1] = mat.k[1][2];
		k[2] = mat.k[2][2];
	}

	//---------------------------------------------------
	// Miscellaneous
	//---------------------------------------------------
	static void GetMinBoundSphere(const V* src, V& p0, T& radius, int n)
	{
		if (n == 0) { p0.SetScalar(0); radius = 0; return; };

		// Find approx center.
		V c(0);
		{ for (int v = 0; v < n; v++) c+= src[v]; };
		c *= 1/n;

		// Find max sqr-radius.
		T maxrsqr = 0;
		{
			for (int v = 0; v < n; v++)
			{
				T rsqr = Sqr(src[v].k[0]-c.k[0]) + Sqr(src[v].k[1]-c.k[1]) + Sqr(src[v].k[2]-c.k[2]);
				maxrsqr = Max(maxrsqr, rsqr);
			};
		};
		p0 = c;
		radius = M_Sqrt(maxrsqr);
	};

	static void GetMinBoundBox(const V* src, V& _min, V& _max, int n)
	{
		if (n == 0) { _min.SetScalar(0); _max.SetScalar(0); return; };

		V min = src[0];
		V max = src[0];
		for (int v = 1; v < n; v++)
		{
			min.k[0] = Min(src[v].k[0], min.k[0]);
			max.k[0] = Max(src[v].k[0], max.k[0]);
			min.k[1] = Min(src[v].k[1], min.k[1]);
			max.k[1] = Max(src[v].k[1], max.k[1]);
			min.k[2] = Min(src[v].k[2], min.k[2]);
			max.k[2] = Max(src[v].k[2], max.k[2]);
		};
		_min = min;
		_max = max;
	};

	static T Point2LineDistance(const V& p0, const V& l0, const V& l1)
	{
		V v0(p0-l0);
		V a(l1-l0);

		if ((v0*a) <= T(0.0))
			return v0.Length();
		else 
		{
			V v1(p0-l1);
			if ((v1*a) >= T(0.0))
				return v1.Length();
			else
				return ((a/v1).Length()) / a.Length();
		}
	};

	void Project(const V& b, V& r) const
	{
		// projicerar this i b och lägger det i r
		T blen2 = (b*b);
		if (blen2 == T(0.0)) { r.SetScalar(0); return; };
		T s = (*this)*b / (b*b);
		b.Scale(s, r);
	};

	void ProjectPlane(const V& n, V& p) const
	{
		// projicerar this planet def av n och lägger det i p
		T blen2 = (n*n);
		if (blen2 == T(0.0)) { p.SetScalar(0); return; };
		T s = (*this)*n / (n*n);
		V np;
		n.Scale(s, np);
		p = (*this);
		p -= np;
	};

	void Reflect(const V& n, V& r) const
	{
		// this pekar "ner" i planet n, r pekar up ifrån planet.
		V np;
		Project(n, np);
		np.Scale(-2, r);
		r += (*this);
	};

	static T GetPolygonArea(const V* _pV, int _nV)
	{
		if (_nV < 3) return 0;

		T Area = 0;
		V v0;
		_pV[1].Sub(_pV[0], v0);
		for(int v = 2; v < _nV; v++)
		{
			V v1,v2;
			_pV[v].Sub(_pV[0], v1);
			v0.CrossProd(v1, v2);
			Area += v2.Length()*0.5f;
			v0 = v1;
		}
		return Area;
	}

	static T GetIndexedPolygonArea(const V* _pV, uint32* _piV, int _nV)
	{
		if (_nV < 3) return 0;

		T Area = 0;
		V v0;
		_pV[_piV[1]].Sub(_pV[_piV[0]], v0);
		for(int v = 2; v < _nV; v++)
		{
			V v1,v2;
			_pV[_piV[v]].Sub(_pV[_piV[0]], v1);
			v0.CrossProd(v1, v2);
			Area += v2.Length()*0.5f;
			v0 = v1;
		}
		return Area;
	}

	static void Spline(
		const V *pMoveA0, const V *pMoveA1, const V *pMoveA2,
		const V *pMoveB0, const V *pMoveB1, const V *pMoveB2, 
		V* _pDest, T _tFrac, T _tA0, T _tA1, T _tB0, T _tB1, int _nV)
	{
		T tSqr = Sqr(_tFrac);
		T tCube = tSqr * _tFrac;

		T k = T(0.5);
		T tsA0 = k * _tA1 / _tA0;
		T tsA1 = k * _tA1 / _tA1;
		T tsB0 = k * _tA1 / _tB0;
		T tsB1 = k * _tA1 / _tB1;

		for(int iV = 0; iV < _nV; iV++)
		{
			// dQuatA
			V dMA = (pMoveA1[iV] - pMoveA0[iV]) * tsA0;
			dMA += (pMoveA2[iV] - pMoveA1[iV]) * tsA1;

			V dMB = (pMoveB1[iV] - pMoveB0[iV]) * tsB0;
			dMB += (pMoveB2[iV] - pMoveB1[iV]) * tsB1;


			// Spline it
			for(int i = 0; i < 3; i++)
			{
				T v0 = dMA.k[i];
				T v1 = dMB.k[i];
				T p0 = pMoveA1[iV].k[i];
				T p1 = pMoveB1[iV].k[i];
				T D = p0;
				T C = v0;
				T B = T(3.0)*(p1 - D) - (T(2.0)*v0) - v1;
				T A = -(T(2.0) * B + v0 - v1) / T(3.0);
				_pDest[iV].k[i] = A*tCube + B*tSqr + C*_tFrac + D;

			}
		}
	}

	// Mondelore: short, simple and it bloody works!
	static void Spline2(const V& _v0, const V& _v1, const V& _v2, const V& _v3, T _t, V& _dest)
	{
		T t = _t;
		T t2 = t * t;
		T t3 = t2 * t;

		V p, q, r, s;

		p = (_v3 - _v2) - (_v0 - _v1);
		q = (_v0 - _v1) - p;
		r = _v2 - _v0;
		s = _v1;

		_dest = p * t3 + q * t2 + r * t + s;
	}

	static void Spline3(const V& _v0, const V& _v1, const V& _v2, const V& _v3, T _t, V& _dest)
	{
		T t, t2, t3;
		t = _t;
		t2 = t * t;
		t3 = t2 * t;

		T m0, m1, m2, m3;
		m0 = -t3 + 2 * t2 - t;
		m1 = t3 - 2 * t2  + 1;
		m2 = -t3 + t2 + t;
		m3 = t3 - t2;
		_dest = _v0 * m0 + _v1 * m1 + _v2 * m2 + _v3 * m3;
	}

	// Hermite spline where tangents are calculated from neighbouring points
	// If tension is set to 0.5f we get a Catmull-Clarke spline
	static void SplineCardinal(const T _a,const V *pP0,const V *pP1,const V *pP2,const V *pPt3, 
		V* _pDest,const T _tFrac,const T _t01,const T _t12,const T _t23,int _nV)
	{
		T tSqr = Sqr(_tFrac);
		T tCube = tSqr * _tFrac;

		T h1 = 2.0f * tCube - 3.0f * tSqr + 1.0f;
		T h2 = -2.0f * tCube + 3.0f * tSqr;
		T h3 = tCube - 2.0f * tSqr + _tFrac;
		T h4 = tCube - tSqr;

		// Account for various timedeltas
		T ts2 = 2.0f * _t12 /(_t12 + _t23);
		T ts1 = 2.0f * _t12 /(_t01 + _t12);

		for(int iV = 0; iV < _nV; iV++)
		{
			// Cardinal splines uses variable tension (a)
			// Catmull-Clarke splines uses a fixed 0.5 (a)
			V T1 = (pP2[iV] - pP0[iV]) * _a * ts1;
			V T2 = (pPt3[iV] - pP1[iV]) * _a * ts2;

			V p = pP1[iV] * h1;
			p += pP2[iV] * h2;
			p += T1 * h3;
			p += T2 * h4;

			_pDest[iV] = p;
		}
	}

	// --------------------------------
	// Plane-functions (Use CPlane3Dfpx instead)
	// --------------------------------
	int PointOnPlaneSide(const V& n, const V& pi) const
	{
		// n = plane normal
		// pi = point in plane.

		// retur: 1 = normal sidan.
		//		 -1 = baksidan
		//        0 = i planet.

		T s = (k[0]-pi.k[0]) * n.k[0] + 
			(k[1]-pi.k[1]) * n.k[1] + 
			(k[2]-pi.k[2]) * n.k[2];
		if (s < T(0.0)) return -1;
		if (s > T(0.0)) return 1;
		return 0;
	};

	int PointOnPlaneSide_Epsilon(const V& n, const V& pi, T _Epsilon) const
	{
		// n = plane normal
		// pi = point in plane.

		// retur: 1 = normal sidan.
		//		 -1 = baksidan
		//        0 = i planet.

		T s = (k[0]-pi.k[0]) * n.k[0] + 
			(k[1]-pi.k[1]) * n.k[1] + 
			(k[2]-pi.k[2]) * n.k[2];
		if (s < -_Epsilon) return -1;
		if (s > _Epsilon) return 1;
		return 0;
	};

	T PlaneDistance(const V& n, const V& pi)
	{
		// n = plane normal
		// pi = point in plane.

		return (k[0]-pi.k[0]) * n.k[0] + 
			(k[1]-pi.k[1]) * n.k[1] + 
			(k[2]-pi.k[2]) * n.k[2];
	};

	//---------------------------------------------------
	// Matrix and angle functions...
	//---------------------------------------------------
	void CreateAxisRotateMatrix(T _Angle, M& _Mat) const
	{
		// *this == Rotation-axis, Must be normalized!
		// _Angle = degrees / 360 (ie. 0..1)

		T x = k[0];
		T y = k[1];
		T z = k[2];

		T a = Sqr(x);
		T b = Sqr(y);
		T c = Sqr(z);
		T C = M_Cos(_Angle*T(2.0)*_PI);
		T S = M_Sin(_Angle*T(2.0)*_PI);
		T zS = z*S;
		T xS = x*S;
		T yS = y*S;
		T xz_ = x*z*(T(1.0)-C);
		T xy_ = x*y*(T(1.0)-C);
		T yz_ = y*z*(T(1.0)-C);

		_Mat.k[0][0] = a + C*(T(1.0)-a);
		_Mat.k[0][1] = xy_ - zS;
		_Mat.k[0][2] = yS + xz_;
		_Mat.k[1][0] = zS + xy_;
		_Mat.k[1][1] = b + C*(T(1.0)-b);
		_Mat.k[1][2] = yz_ - xS;
		_Mat.k[2][0] = xz_ - yS;
		_Mat.k[2][1] = xS + yz_;
		_Mat.k[2][2] = c + C*(T(1.0)-c);

		_Mat.UnitNot3x3();
	}

	void CreateAxisRotateMatrix(fp32 _Angle, CMat43fp32& _Mat) const
	{
		// *this == Rotation-axis, Must be normalized!
		// _Angle = degrees / 360 (ie. 0..1)

		fp32 x = k[0];
		fp32 y = k[1];
		fp32 z = k[2];

		fp32 a = Sqr(x);
		fp32 b = Sqr(y);
		fp32 c = Sqr(z);
		fp32 C = M_Cos(_Angle*T(2.0)*_PI);
		fp32 S = M_Sin(_Angle*T(2.0)*_PI);
		fp32 zS = z*S;
		fp32 xS = x*S;
		fp32 yS = y*S;
		fp32 xz_ = x*z*(T(1.0)-C);
		fp32 xy_ = x*y*(T(1.0)-C);
		fp32 yz_ = y*z*(T(1.0)-C);

		_Mat.k[0][0] = a + C*(T(1.0)-a);
		_Mat.k[0][1] = xy_ - zS;
		_Mat.k[0][2] = yS + xz_;
		_Mat.k[1][0] = zS + xy_;
		_Mat.k[1][1] = b + C*(T(1.0)-b);
		_Mat.k[1][2] = yz_ - xS;
		_Mat.k[2][0] = xz_ - yS;
		_Mat.k[2][1] = xS + yz_;
		_Mat.k[2][2] = c + C*(T(1.0)-c);

		_Mat.UnitNot3x3();
	}

	void CreateMatrixFromAngles(int AnglePriority, M& DestMat) const
	{
		/*
		Rotation priorities:

		  0: [mat] = [z]*[y]*[x]
		  1: [mat] = [z]*[x]*[y]
		  2: [mat] = [y]*[z]*[x]
		  3: [mat] = [y]*[x]*[z]
		  4: [mat] = [x]*[z]*[y]
		  5: [mat] = [x]*[y]*[z]


		Unit		16 writes
		Rotate		3 * (17 mul, 16 add/sub, 2 trig, 16 writes)

		Total		51 mul, 36 add/sub, 6 trig, 64 write.
		*/

		DestMat.Unit();
		switch (AnglePriority) {
		case 5 : 
			{
				DestMat.M_x_RotZ(k[2]);
				DestMat.M_x_RotY(k[1]);
				DestMat.M_x_RotX(k[0]);
				break;
			};
		case 3 : 
			{
				DestMat.M_x_RotZ(k[2]);
				DestMat.M_x_RotX(k[0]);
				DestMat.M_x_RotY(k[1]);
				break;
			};
		case 4 : 
			{
				DestMat.M_x_RotY(k[1]);
				DestMat.M_x_RotZ(k[2]);
				DestMat.M_x_RotX(k[0]);
				break;
			};
		case 1 : 
			{
				DestMat.M_x_RotY(k[1]);
				DestMat.M_x_RotX(k[0]);
				DestMat.M_x_RotZ(k[2]);
				break;
			};
		case 2 : 
			{
				DestMat.M_x_RotX(k[0]);
				DestMat.M_x_RotZ(k[2]);
				DestMat.M_x_RotY(k[1]);
				break;
			};
		case 0 : 
			{
				DestMat.M_x_RotX(k[0]);
				DestMat.M_x_RotY(k[1]);
				DestMat.M_x_RotZ(k[2]);
				break;
			};
		default :
			break;
		};

	}

	static T AngleFromVector(T x, T z)
	{
		//  Ut: Vinkel i [0..1]
		//		x = A*cos(v), A > 0
		//		z = A*sin(v)

		T absx = Abs(x);
		T absz = Abs(z);

		if (absx > absz)
		{
			T v = M_ATan(absz/absx) * ((T(1.0f) / _PI) * 0.5f);
			if (x > T(0.0f))
			{
				if (z < T(0.0f)) return -v;
				return v;
			}
			else
			{
				if (z < T(0.0f)) return v+T(0.5f);
				return -v+T(0.5f);
			};
		}
		else
		{
			T v = M_ATan(absx/absz) * ((T(1.0f) / _PI) * 0.5f);
			if (z > T(0.0f))
			{
				if (x < T(0.0f)) return v+T(0.25f);
				return -v+T(0.25f);
			}
			else
			{
				if (x < T(0.0f)) return -v+T(0.75f);
				return v+T(0.75f);
			}
		}
	}

	static void CreateAngleZYXFromMatrix(V& v, 
			T e00, T e01, T e02,
			T e10, T e11, T e12,
			T e20, T e21, T e22)
	{
		T sb = -e20;
		T q1 = Abs(e00) + Abs(e10);
		T q2 = Abs(e21) + Abs(e22);
		if ((q1<T(0.00001f)) || (q2<T(0.00001f)))
		{
			if (sb <= T(0.0f))
				v.k[1] = T(0.25f);
			else
				v.k[1] = -T(0.25f);
			v.k[2] = -AngleFromVector(e11, -e01);
			v.k[0] = 0;
		}
		else
		{
			T tmp = M_Sqrt(Abs(T(1.0) - Sqr(sb)));
			v.k[1] = -AngleFromVector(tmp, sb);
			v.k[2] = -AngleFromVector(e00, e10);
			v.k[0] = AngleFromVector(e22, -e21);
		};
	};

	void CreateAnglesFromMatrix(int AnglePriority, const M& m)
	{
		switch (AnglePriority)
		{
		case 5 : 
			{
				CreateAngleZYXFromMatrix(*this, 
					m.k[0][0], m.k[0][1], m.k[0][2],
					m.k[1][1], m.k[1][1], m.k[1][2],
					m.k[2][2], m.k[2][1], m.k[2][2]);
				break;
			};
		case 3 :
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[1][1], m.k[1][0], m.k[1][2],
					m.k[0][1], m.k[0][0], m.k[0][2],
					m.k[2][1], m.k[2][0], m.k[2][2]);
				k[0] = -a.k[1]; k[1] = -a.k[0]; k[2] = -a.k[2];
				break;
			};
		case 4 : 
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[0][0], m.k[0][2], m.k[0][1],
					m.k[2][0], m.k[2][2], m.k[2][1],
					m.k[1][0], m.k[1][2], m.k[1][1]);
				k[0] = -a.k[0]; k[1] = -a.k[2]; k[2] = -a.k[1];
				break;
			};
		case 1 :
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[0][0], m.k[0][1], m.k[0][2],
					m.k[1][0], m.k[1][1], m.k[1][2],
					m.k[2][0], m.k[2][1], m.k[2][2]);
				k[0] = a.k[1]; k[1] = a.k[2]; k[2] = a.k[0];
				break;
			};
		case 2 :
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[1][1], m.k[1][2], m.k[1][0],
					m.k[2][1], m.k[2][2], m.k[2][0],
					m.k[0][1], m.k[0][2], m.k[0][0]);
				k[2] = a.k[1]; k[1] = a.k[0]; k[0] = a.k[2];
				break;
			};
 		case 0 :
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[2][2], m.k[2][1], m.k[2][0],
					m.k[1][2], m.k[1][1], m.k[1][0],
					m.k[0][2], m.k[0][1], m.k[0][0]);
				k[1] = -a.k[1]; k[2] = -a.k[0]; k[0] = -a.k[2];
				break;
			};
		default :
			Error_static("TVector3::CreateAnglesFromMatrix", "Invalid angle-priority.");
		};
		k[0] = -k[0];
		k[1] = -k[1];
		k[2] = -k[2];
	};

	//---------------------------------------------------
	void Snap(T _Grid, T _SnapTresh)
	{
		for(int i = 0; i < 3; i++)
			k[i] = SnapFloat(k[i], _Grid, _SnapTresh);
	}

	V GetSnapped(T _Grid, T _SnapTresh) const
	{
		V s;
		for(int i = 0; i < 3; i++)
			s.k[i] = SnapFloat(k[i], _Grid, _SnapTresh);
		return s;
	}

	//---------------------------------------------------
	void Read(CCFile* _pFile)
	{
		_pFile->ReadLE(k[0]);
		_pFile->ReadLE(k[1]);
		_pFile->ReadLE(k[2]);
	}

	void Write(CCFile* _pFile) const
	{
		_pFile->WriteLE(k[0]);
		_pFile->WriteLE(k[1]);
		_pFile->WriteLE(k[2]);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(k[0]);
		::SwapLE(k[1]);
		::SwapLE(k[2]);
	}
#endif
	//---------------------------------------------------
	CStr GetString() const
	{
		return CStrF("(%.3f,%.3f,%.3f)", k[0], k[1], k[2]);
	};

	CFStr GetFilteredString(int _iType = 0) const
	{
		switch(_iType)
		{
		case 0: return CFStrF("%s,%s,%s", (const char *)CFStr::GetFilteredString(k[0]), (const char *)CFStr::GetFilteredString(k[1]), (const char *)CFStr::GetFilteredString(k[2]));
		case 1: return CFStrF("%s %s %s", (const char *)CFStr::GetFilteredString(k[0]), (const char *)CFStr::GetFilteredString(k[1]), (const char *)CFStr::GetFilteredString(k[2]));
		}
		return CFStr();
	};

	V& ParseString(const CStr& _s)
	{
		const char* pStr = (const char*) _s;
		if (!pStr) { SetScalar((T)0); return *this; }
		int pos = 0;
		int len = _s.Len();
#ifdef CPU_SUPPORT_FP64
		fp64 last = T(0.0);
#else
		fp32 last = T(0.0);
#endif
		for(int i = 0; i < 3; i++)
		{
			pos = CStr::GoToDigit(pStr, pos, len);
			if (pStr[pos] != 0)
				last = M_AToF(&pStr[pos]);
			k[i] = (T)last;
			pos = CStr::SkipADigit(pStr, pos, len);
		}
		return *this;
	}

	void ParseColor(const CStr& _s, bool _bHexUnit = false)
	{
		if (_s.CompareSubStr("0x") == 0)
		{
			int c = _s.Val_int();
			k[0] = (c >> 16) & 0xff;
			k[1] = (c >> 8) & 0xff;
			k[2] = (c >> 0) & 0xff;
			if (_bHexUnit)
				*this *= T(1.0f) / T(255.0f);
		}
		else
		{
			const char* pStr = (const char*) _s;
			if (!pStr) { SetScalar((T)0);; return; }
			int pos = 0;
			int len = CStr::StrLen(pStr);
			pos = CStr::GoToDigit(pStr, pos, len);
			int i = 0;
			for(; i < 3; i++)
			{
				k[i] = M_AToF(&pStr[pos]);
				pos = CStr::SkipADigit(pStr, pos, len);
				pos = CStr::GoToDigit(pStr, pos, len);
				if (pos == len) break;
			}

			if (i == 0) 
				SetScalar(k[0]);
		}
	}

	V operator* (const M43& m) const
	{
		V r;
		for (int i=0; i<3; i++)
			r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
		return r;
	}

	void operator*= (const M43& m)
	{
		V r;
		for (int i=0; i<3; i++)
			r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
		k[0] = r.k[0];
		k[1] = r.k[1];
		k[2] = r.k[2];
	}

	void MultiplyMatrix(const M43& m, V& dest) const
	{
		for (int i = 0; i < 3; i++)
			dest.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
	}

	void AddMatrixRow(M43& m, int row) const
	{
		m.k[row][0] += k[0];
		m.k[row][1] += k[1];
		m.k[row][2] += k[2];
	}

	void AddMatrixColumn(M43& m, int column) const
	{
		M_ASSERT(column >= 0 && column <= 2, "!");
		m.k[0][column] += k[0];
		m.k[1][column] += k[1];
		m.k[2][column] += k[2];
	}

	static V& GetMatrixRow(M43& _Mat, int _Row)
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static const V& GetMatrixRow(const M43& _Mat, int _Row)
	{
		const V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	void SetMatrixRow(M43& _Mat, int _Row) const
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		*pV = *this;
	}

	static V& GetRow(M43& _Mat, int _Row)
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static const V& GetRow(const M43& _Mat, int _Row)
	{
		const V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	void SetRow(M43& _Mat, int _Row) const
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		*pV = *this;
	}

	void SetCol(M43& _Mat, int _iCol) const
	{
		_Mat.k[0][_iCol] = k[0];
		_Mat.k[1][_iCol] = k[1];
		_Mat.k[2][_iCol] = k[2];
	}

	static void MultiplyMatrix(const V* src, V* dest, const M43& m, int n)
	{
		for (int vnr = 0; vnr<n; vnr++) {
			for (int i=0; i<3; i++)
				dest[vnr].k[i] = m.k[0][i]*src[vnr].k[0] + m.k[1][i]*src[vnr].k[1] + m.k[2][i]*src[vnr].k[2] + m.k[3][i];
		}
	}

	void MultiplyMatrix(const M43& m)
	{
		T x = k[0];
		T y = k[1];
		T z = k[2];
		for (int i = 0; i < 3; i++)
			k[i] = m.k[0][i]*x + m.k[1][i]*y + m.k[2][i]*z + m.k[3][i];
	}

	void MultiplyMatrix3x3(const M43& m)
	{
		T x = k[0];
		T y = k[1];
		T z = k[2];
		for (int i = 0; i < 3; i++)
			k[i] = m.k[0][i]*x + m.k[1][i]*y + m.k[2][i]*z;
	}

	void GetDirectionX(const M43& mat)
	{
		k[0] = mat.k[0][0];
		k[1] = mat.k[1][0];
		k[2] = mat.k[2][0];
	}

	void GetDirectionY(const M43& mat)
	{
		k[0] = mat.k[0][1];
		k[1] = mat.k[1][1];
		k[2] = mat.k[2][1];
	}

	void GetDirectionZ(const M43& mat)
	{
		k[0] = mat.k[0][2];
		k[1] = mat.k[1][2];
		k[2] = mat.k[2][2];
	}

	void CreateMatrixFromAngles(int AnglePriority, M43& DestMat) const
	{
		/*
		Rotation priorities:

		  0: [mat] = [z]*[y]*[x]
		  1: [mat] = [z]*[x]*[y]
		  2: [mat] = [y]*[z]*[x]
		  3: [mat] = [y]*[x]*[z]
		  4: [mat] = [x]*[z]*[y]
		  5: [mat] = [x]*[y]*[z]


		Unit		16 writes
		Rotate		3 * (17 mul, 16 add/sub, 2 trig, 16 writes)

		Total		51 mul, 36 add/sub, 6 trig, 64 write.
		*/

		DestMat.Unit();
		switch (AnglePriority) {
		case 5 : 
			{
				DestMat.M_x_RotZ(k[2]);
				DestMat.M_x_RotY(k[1]);
				DestMat.M_x_RotX(k[0]);
				break;
			};
		case 3 : 
			{
				DestMat.M_x_RotZ(k[2]);
				DestMat.M_x_RotX(k[0]);
				DestMat.M_x_RotY(k[1]);
				break;
			};
		case 4 : 
			{
				DestMat.M_x_RotY(k[1]);
				DestMat.M_x_RotZ(k[2]);
				DestMat.M_x_RotX(k[0]);
				break;
			};
		case 1 : 
			{
				DestMat.M_x_RotY(k[1]);
				DestMat.M_x_RotX(k[0]);
				DestMat.M_x_RotZ(k[2]);
				break;
			};
		case 2 : 
			{
				DestMat.M_x_RotX(k[0]);
				DestMat.M_x_RotZ(k[2]);
				DestMat.M_x_RotY(k[1]);
				break;
			};
		case 0 : 
			{
				DestMat.M_x_RotX(k[0]);
				DestMat.M_x_RotY(k[1]);
				DestMat.M_x_RotZ(k[2]);
				break;
			};
		default :
			break;
		};
	}

	void CreateAnglesFromMatrix(int AnglePriority, const M43& m)
	{
		switch (AnglePriority)
		{
		case 5 : 
			{
				CreateAngleZYXFromMatrix(*this, 
					m.k[0][0], m.k[0][1], m.k[0][2],
					m.k[1][1], m.k[1][1], m.k[1][2],
					m.k[2][2], m.k[2][1], m.k[2][2]);
				break;
			};
		case 3 :
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[1][1], m.k[1][0], m.k[1][2],
					m.k[0][1], m.k[0][0], m.k[0][2],
					m.k[2][1], m.k[2][0], m.k[2][2]);
				k[0] = -a.k[1]; k[1] = -a.k[0]; k[2] = -a.k[2];
				break;
			};
		case 4 : 
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[0][0], m.k[0][2], m.k[0][1],
					m.k[2][0], m.k[2][2], m.k[2][1],
					m.k[1][0], m.k[1][2], m.k[1][1]);
				k[0] = -a.k[0]; k[1] = -a.k[2]; k[2] = -a.k[1];
				break;
			};
		case 1 :
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[0][0], m.k[0][1], m.k[0][2],
					m.k[1][0], m.k[1][1], m.k[1][2],
					m.k[2][0], m.k[2][1], m.k[2][2]);
				k[0] = a.k[1]; k[1] = a.k[2]; k[2] = a.k[0];
				break;
			};
		case 2 :
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[1][1], m.k[1][2], m.k[1][0],
					m.k[2][1], m.k[2][2], m.k[2][0],
					m.k[0][1], m.k[0][2], m.k[0][0]);
				k[2] = a.k[1]; k[1] = a.k[0]; k[0] = a.k[2];
				break;
			};
 		case 0 :
			{
				V a;
				CreateAngleZYXFromMatrix(a, 
					m.k[2][2], m.k[2][1], m.k[2][0],
					m.k[1][2], m.k[1][1], m.k[1][0],
					m.k[0][2], m.k[0][1], m.k[0][0]);
				k[1] = -a.k[1]; k[2] = -a.k[0]; k[0] = -a.k[2];
				break;
			};
		default :
			Error_static("TVector3::CreateAnglesFromMatrix", "Invalid angle-priority.");
		};
		k[0] = -k[0];
		k[1] = -k[1];
		k[2] = -k[2];
	}
//#endif
};


template<class T>
class TVector3 : public TVector3Aggr<T>
{
public:
	typedef TVector3Aggr<T> VBase;
	typedef TVector3 V;
	typedef TMatrix4<T> M;
	typedef TMatrix43<T> M43;

	TVector3()
	{
	}

	TVector3(T val) 
	{
		VBase::k[0] = val;
		VBase::k[1] = val;
		VBase::k[2] = val;
	}

	TVector3(T x, T y, T z)
	{
		VBase::k[0] = x;
		VBase::k[1] = y;
		VBase::k[2] = z;
	}

	TVector3(const VBase& _v)
	{
		T x = _v.k[0];
		T y = _v.k[1];
		T z = _v.k[2];
		VBase::k[0] = x;
		VBase::k[1] = y;
		VBase::k[2] = z;
	};

/*	void operator= (const VBase& _v)
	{
		T x = _v.k[0];
		T y = _v.k[1];
		T z = _v.k[2];
		VBase::k[0] = x;
		VBase::k[1] = y;
		VBase::k[2] = z;	
	}*/

	static V& GetRow(M& _Mat, int _Row)
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static const V& GetRow(const M& _Mat, int _Row)
	{
		const V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static V& GetRow(M43& _Mat, int _Row)
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static const V& GetRow(const M43& _Mat, int _Row)
	{
		const V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static V& GetMatrixRow(M& _Mat, int _Row)
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static const V& GetMatrixRow(const M& _Mat, int _Row)
	{
		const V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static V& GetMatrixRow(M43& _Mat, int _Row)
	{
		V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

	static const V& GetMatrixRow(const M43& _Mat, int _Row)
	{
		const V* pV = (V*) &_Mat.k[_Row][0];
		return *pV;
	}

};

typedef TVector3<int8> CVec3Dint8;
typedef TVector3<uint8> CVec3Duint8;
typedef TVector3<int16> CVec3Dint16;
typedef TVector3<uint16> CVec3Duint16;
typedef TVector3<int32> CVec3Dint32;	
typedef TVector3<int> CVec3Dint;		// Undefined size integer vector (ie. >= 32bit)
typedef TVector3<fp32> CVec3Dfp32;
typedef TVector3Aggr<fp32> CVec3Dfp32Aggr;
typedef TVector3<fp64> CVec3Dfp64;



template <class T>
M_INLINE const TVector3<T>& TMatrix4<T>::GetRow(uint _iRow) const
{
	return *((const V*)&k[_iRow][0]);
}

template <class T>
M_INLINE TVector3<T>& TMatrix4<T>::GetRow(uint _iRow)
{
	return *((V*)&k[_iRow][0]);
}

template <class T>
M_INLINE const TVector3<T>& TMatrix43<T>::GetRow(uint _iRow) const
{
	return *((const V*)&k[_iRow][0]);
}

template <class T>
M_INLINE TVector3<T>& TMatrix43<T>::GetRow(uint _iRow)
{
	return *((V*)&k[_iRow][0]);
}

template<class T>
TVector3Aggr<T> operator *(const TVector3Aggr<T> &k, const TMatrix4<T>& m)
{
	TVector3Aggr<T> r;
	for (int i=0; i<3; i++)
		r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i];
	return r;
}

// -------------------------------------------------------------------
//  TPlane3
// -------------------------------------------------------------------
template<class T>
class TPlane3Aggr
{
public:
	typedef TVector3Aggr<T> V;
	typedef TVector3<T> V3;
	typedef TMatrix4<T> M;
	typedef TMatrix43<T> M43;
	typedef typename TMathTemplateProperties<T>::TVector4Intrinsic TIntrinsic;
	typedef TPlane3Aggr P;
	typedef TPlane3<T> P3;


public:
	union
	{
		TIntrinsic v;
		struct
		{
			V n;	// Normal
			T d;	// Distance
		};
	};

	/*
	// --------------------------------
	// Construction
	// --------------------------------
	void Create(const V& _v, T _a)						// Create from axis-angle
	void Create(const M& _Mat)							// Create from matrix
	TPlane3()
	TPlane3(const TPlane3& _Plane)						// Construct from plane
	TPlane3(const V& _n, T _d)							// Construct from normal + distance
	TPlane3(const V& _n, const V& _p)					// Construct from normal + point
	TPlane3(const V& _p0, const V& _p1, const V& _p2)	// Construct from 3 points
	void Create(const V& _n, const V& _p)				// Create from normal + point
	void Create(const V& _p0, const V& _p1, const V& _p2) // Create from 3 points
	void CreateInverse(const TPlane3 _Plane)			// Create from inverse plane

	void Inverse()										// Inverse
	V GetPointInPlane() const							// Get a point on the plane
	T Distance(const V& v) const						// Point-2-plane distance

	// --------------------------------
	// Point plane-side classification
	// --------------------------------
	int GetPlaneSide(const V& _v) const
	int GetPlaneSide_Epsilon(const V& _v, T _Epsilon) const
	int GetPlaneSideMask_Epsilon(const V& _v, T _Epsilon) const
	int GetArrayPlaneSideMask(const V* _pV, int _nv) const
	int GetArrayPlaneSideMask_Epsilon(const V* _pV, int _nv, T _Epsilon) const

	// --------------------------------
	// Box functions
	// --------------------------------
	int GetBoxPlaneSideMask(const V& _VMin, const V& _VMax) const	// Box plane-side classification
	T GetBoxMinDistance(const V& _VMin, const V& _VMax) const
	T GetBoxMaxDistance(const V& _VMin, const V& _VMax) const

	void GetIntersectionPoint(const V& _p0, const V& _p1, V& _RetV) const
	void Translate(const V& _dV)						// Translate plane by dV
	void Transform(const M& _Mat)						// Affine transform by matrix.

	// --------------------------------
	// Helper functions
	// --------------------------------
	CStr GetString()									// Get string for output.
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()

	*/

	M_FORCEINLINE void operator= (const P& _p)
	{
		v = _p.v;
	}

	M_INLINE void CreateND(const V3& _n, T _d)
	{
		n = _n;
		d = _d;
	}

	M_INLINE void CreateNV(const V3& _n, const V3& _p)
	{
		n = _n;
		d = -(_n*_p);
	}

	void Create(const V3& _p0, const V3& _p1, const V3& _p2)
	{
		V3 v0 = _p1 - _p0;
		V3 v1 = _p2 - _p0;
		n = (v0 / v1).Normalize();
		d = -(n * _p0);
//		p = _p0;
	}

	M_INLINE void CreateInverse(const P& _Plane)
	{
		n = -_Plane.n;
		d = -_Plane.d;
	}

	M_INLINE void Inverse()
	{
		n = -n;
		d = -d;
	}

	V3 GetPointInPlane() const
	{
		return n * (-d);
	}

	M_INLINE T Distance(const V3& v) const
	{
		return (v.k[0]*n.k[0] + v.k[1]*n.k[1] + v.k[2]*n.k[2] + d);
	}

	bool AlmostEqual(const TPlane3<T>& _p1, T _EpsilonD = 0.0001, T _EpsilonN = 0.0001) const
	{
		if ((Abs(Distance(_p1.GetPointInPlane())) < _EpsilonD) &&
			(Abs((T)1.0 - n*_p1.n) < _EpsilonN))
			return true;
		return false;
	}

	int GetPlaneSide(const V3& _v) const
	{
		// retur: 1 = normal sidan.
		//		 -1 = baksidan
		//        0 = i planet.

		T dist = Distance(_v);
		if (dist < (T)0) return -1;
		else if (dist > (T)0) return 1;
		return 0;
	}

	int GetPlaneSide_Epsilon(const V3& _v, T _Epsilon) const
	{
		// retur: 1 = normal sidan.
		//		 -1 = baksidan
		//        0 = i planet.

		T dist = Distance(_v);
		if (dist < -_Epsilon) return -1;
		else if (dist > _Epsilon) return 1;
		return 0;
	}

	int GetPlaneSideMask_Epsilon(const V3& _v, T _Epsilon) const
	{
		// retur: 1 = normal sidan.
		//		  2 = baksidan
		//        4 = i planet.
		T dist = Distance(_v);
		if (dist < (T)-_Epsilon) return 2;
		else if (dist > (T)_Epsilon) return 1;
		return 4;
	}

	int GetArrayPlaneSideMask(const V3* _pV, int _nv) const
	{
		// retur: 1 = normal sidan.
		//		  2 = baksidan

		int SideMask = 0;
		for(int i = 0; i < _nv; i++)
		{
			if (Distance(_pV[i]) >= (T) 0)
				SideMask |= 1;
			else
				SideMask |= 2;
			if (SideMask == 3) return SideMask;
		}
		return SideMask;
	}

	int GetArrayPlaneSideMask_Epsilon(const V3* _pV, int _nv, T _Epsilon) const
	{
		// retur: 1 = normal sidan.
		//		  2 = baksidan
		//        4 = i planet.
		//
		// Hoppar ur om (mask & 3) == 3

		int SideMask = 0;
		for(int i = 0; i < _nv; i++)
		{
			SideMask |= GetPlaneSideMask_Epsilon(_pV[i], _Epsilon);
			if ((SideMask & 3) == 3) return SideMask;
		}
		return SideMask;
	}

	int GetBoxPlaneSideMask(const V3& _VMin, const V3& _VMax) const
	{
		// retur: 1 = normal sidan.
		//		  2 = baksidan

		T xMin, xMax, yMin, yMax, zMin, zMax;
//		if (n.k[0] > T(0.0)) 
//			{ xMin = _VMin.k[0]*n.k[0]; xMax = _VMax.k[0]*n.k[0]; }
//		else
//			{ xMin = _VMax.k[0]*n.k[0]; xMax = _VMin.k[0]*n.k[0]; }
		xMin = M_FSel(n.k[0], _VMin.k[0], _VMax.k[0]) * n.k[0];
		xMax = M_FSel(n.k[0], _VMax.k[0], _VMin.k[0]) * n.k[0];

//		if (n.k[1] > T(0.0)) 
//			{ yMin = _VMin.k[1]*n.k[1]; yMax = _VMax.k[1]*n.k[1]; }
//		else
//			{ yMin = _VMax.k[1]*n.k[1]; yMax = _VMin.k[1]*n.k[1]; }
		yMin = M_FSel(n.k[1], _VMin.k[1], _VMax.k[1]) * n.k[1];
		yMax = M_FSel(n.k[1], _VMax.k[1], _VMin.k[1]) * n.k[1];

//		if (n.k[2] > T(0.0)) 
//			{ zMin = _VMin.k[2]*n.k[2]; zMax = _VMax.k[2]*n.k[2]; }
//		else
//			{ zMin = _VMax.k[2]*n.k[2]; zMax = _VMin.k[2]*n.k[2]; }
		zMin = M_FSel(n.k[2], _VMin.k[2], _VMax.k[2]) * n.k[2];
		zMax = M_FSel(n.k[2], _VMax.k[2], _VMin.k[2]) * n.k[2];

		if (xMin + yMin + zMin + d > T(0.0)) return 1;
		if (xMax + yMax + zMax + d > T(0.0)) return 3;
		return 2;
	}

	M_INLINE T GetBoxMinDistance(const V3& _VMin, const V3& _VMax) const
	{
/*		fp32 Result = d;
		if (FloatIsNeg(n[0]))
			Result += _VMax.k[0]*n.k[0];
		else
			Result += _VMin.k[0]*n.k[0];

		if (FloatIsNeg(n[1]))
			Result += _VMax.k[1]*n.k[1];
		else
			Result += _VMin.k[1]*n.k[1];

		if (FloatIsNeg(n[2]))
			Result += _VMax.k[2]*n.k[2];
		else
			Result += _VMin.k[2]*n.k[2];

		return Result;
*/
/*		fp32 xMin, yMin, zMin;
		xMin = (FloatIsNeg(n.k[0])) ? _VMax.k[0]*n.k[0] : _VMin.k[0]*n.k[0];
		yMin = (FloatIsNeg(n.k[1])) ? _VMax.k[1]*n.k[1] : _VMin.k[1]*n.k[1];
		zMin = (FloatIsNeg(n.k[2])) ? _VMax.k[2]*n.k[2] : _VMin.k[2]*n.k[2];
		return xMin + yMin + zMin + d;*/

		T xMin, yMin, zMin;
//		xMin = (n.k[0] > T(0.0)) ? _VMin.k[0]*n.k[0] : _VMax.k[0]*n.k[0];
//		yMin = (n.k[1] > T(0.0)) ? _VMin.k[1]*n.k[1] : _VMax.k[1]*n.k[1];
//		zMin = (n.k[2] > T(0.0)) ? _VMin.k[2]*n.k[2] : _VMax.k[2]*n.k[2];
		xMin = M_FSel(n.k[0], _VMin.k[0], _VMax.k[0]) * n.k[0];
		yMin = M_FSel(n.k[1], _VMin.k[1], _VMax.k[1]) * n.k[1];
		zMin = M_FSel(n.k[2], _VMin.k[2], _VMax.k[2]) * n.k[2];
		return xMin + yMin + zMin + d;
	}

	M_INLINE T GetBoxMaxDistance(const V3& _VMin, const V3& _VMax) const
	{
/*		fp32 Result = d;
		if (FloatIsNeg(n[0]))
			Result += _VMin.k[0]*n.k[0];
		else
			Result += _VMax.k[0]*n.k[0];

		if (FloatIsNeg(n[1]))
			Result += _VMin.k[1]*n.k[1];
		else
			Result += _VMax.k[1]*n.k[1];

		if (FloatIsNeg(n[2]))
			Result += _VMin.k[2]*n.k[2];
		else
			Result += _VMax.k[2]*n.k[2];

		return Result;*/

/*		fp32 xMax, yMax, zMax;
		xMax = (FloatIsNeg(n[0])) ? _VMin.k[0]*n.k[0] : _VMax.k[0]*n.k[0];
		yMax = (FloatIsNeg(n[1])) ? _VMin.k[1]*n.k[1] : _VMax.k[1]*n.k[1];
		zMax = (FloatIsNeg(n[2])) ? _VMin.k[2]*n.k[2] : _VMax.k[2]*n.k[2];
		return xMax + yMax + zMax + d;*/

		T xMax, yMax, zMax;
//		xMax = (n.k[0] < T(0.0)) ? _VMin.k[0]*n.k[0] : _VMax.k[0]*n.k[0];
//		yMax = (n.k[1] < T(0.0)) ? _VMin.k[1]*n.k[1] : _VMax.k[1]*n.k[1];
//		zMax = (n.k[2] < T(0.0)) ? _VMin.k[2]*n.k[2] : _VMax.k[2]*n.k[2];
		xMax = M_FSel(n.k[0], _VMax.k[0], _VMin.k[0]) * n.k[0];
		yMax = M_FSel(n.k[1], _VMax.k[1], _VMin.k[1]) * n.k[1];
		zMax = M_FSel(n.k[2], _VMax.k[2], _VMin.k[2]) * n.k[2];
		return xMax + yMax + zMax + d;
	}

	M_INLINE void GetBoxMinAndMaxDistance(const V3& _VMin, const V3& _VMax, T& _MinDistance, T& _MaxDistance) const
	{
/*		fp32 MinDist = d;
		fp32 MaxDist = d;

		if (FloatIsNeg(n[0]))
		{
			MinDist += _VMax.k[0]*n.k[0];
			MaxDist += _VMin.k[0]*n.k[0];
		}
		else
		{
			MinDist += _VMin.k[0]*n.k[0];
			MaxDist += _VMax.k[0]*n.k[0];
		}

		if (FloatIsNeg(n[1]))
		{
			MinDist += _VMax.k[1]*n.k[1];
			MaxDist += _VMin.k[1]*n.k[1];
		}
		else
		{
			MinDist += _VMin.k[1]*n.k[1];
			MaxDist += _VMax.k[1]*n.k[1];
		}

		if (FloatIsNeg(n[2]))
		{
			MinDist += _VMax.k[2]*n.k[2];
			MaxDist += _VMin.k[2]*n.k[2];
		}
		else
		{
			MinDist += _VMin.k[2]*n.k[2];
			MaxDist += _VMax.k[2]*n.k[2];
		}

		_MinDistance = MinDist;
		_MaxDistance = MaxDist;
*/

		T xMin, yMin, zMin, xMax, yMax, zMax;

//		if (n.k[0] < 0.0f)
//		{
//			xMin = _VMax.k[0]*n.k[0];
//			xMax = _VMin.k[0]*n.k[0];
//		}
//		else
//		{
//			xMin = _VMin.k[0]*n.k[0];
//			xMax = _VMax.k[0]*n.k[0];
//		}
		xMin = M_FSel(n.k[0], _VMin.k[0], _VMax.k[0]) * n.k[0];
		xMax = M_FSel(n.k[0], _VMax.k[0], _VMin.k[0]) * n.k[0];

//		if (n.k[1] < 0.0f)
//		{
//			yMin = _VMax.k[1]*n.k[1];
//			yMax = _VMin.k[1]*n.k[1];
//		}
//		else
//		{
//			yMin = _VMin.k[1]*n.k[1];
//			yMax = _VMax.k[1]*n.k[1];
//		}
		yMin = M_FSel(n.k[1], _VMin.k[1], _VMax.k[1]) * n.k[1];
		yMax = M_FSel(n.k[1], _VMax.k[1], _VMin.k[1]) * n.k[1];

//		if (n.k[2] < 0.0f)
//		{
//			zMin = _VMax.k[2]*n.k[2];
//			zMax = _VMin.k[2]*n.k[2];
//		}
//		else
//		{
//			zMin = _VMin.k[2]*n.k[2];
//			zMax = _VMax.k[2]*n.k[2];
//		}
		zMin = M_FSel(n.k[2], _VMin.k[2], _VMax.k[2]) * n.k[2];
		zMax = M_FSel(n.k[2], _VMax.k[2], _VMin.k[2]) * n.k[2];

		_MinDistance = xMin + yMin + zMin + d;
		_MaxDistance = xMax + yMax + zMax + d;

/*		fp32 xMin, yMin, zMin;
		xMin = (n.k[0] > 0.0f) ? _VMin.k[0]*n.k[0] : _VMax.k[0]*n.k[0];
		yMin = (n.k[1] > 0.0f) ? _VMin.k[1]*n.k[1] : _VMax.k[1]*n.k[1];
		zMin = (n.k[2] > 0.0f) ? _VMin.k[2]*n.k[2] : _VMax.k[2]*n.k[2];
		return xMin + yMin + zMin + d;*/
	}

	bool IntersectsPlane(const V3& _p0, const V3& _p1, V3& _RetV) const
	{
		T dvx = (_p1.k[0] - _p0.k[0]);
		T dvy = (_p1.k[1] - _p0.k[1]);
		T dvz = (_p1.k[2] - _p0.k[2]);
		
		T s = dvx*n.k[0] + dvy*n.k[1] + dvz*n.k[2];
		
		if (s == (T)0)
			return false;
		else if(s > 0 && GetPlaneSide(_p0) > 0)
			return false;
		else if(s < 0 && GetPlaneSide(_p0) < 0)
			return false;
		
		T sp = _p0*n + d;
		T t = -sp/s;
		
		_RetV.k[0] = _p0.k[0] + dvx * t;
		_RetV.k[1] = _p0.k[1] + dvy * t;
		_RetV.k[2] = _p0.k[2] + dvz * t;
		return true;
	}

	
	bool IntersectLineSegment(const V3& _p0, const V3& _p1, V3& _RetV) const
	{
		T dvx = (_p1.k[0] - _p0.k[0]);
		T dvy = (_p1.k[1] - _p0.k[1]);
		T dvz = (_p1.k[2] - _p0.k[2]);

		T s = dvx*n.k[0] + dvy*n.k[1] + dvz*n.k[2];

		if (s == (T)0)
			return false;
		else if(s > 0 && GetPlaneSide(_p0) > 0)
			return false;
		else if(s < 0 && GetPlaneSide(_p0) < 0)
			return false;

		T sp = _p0*n + d;
		T t = -sp/s;

		if (t >= 0.0 && t<=1.0)
		{
			_RetV.k[0] = _p0.k[0] + dvx * t;
			_RetV.k[1] = _p0.k[1] + dvy * t;
			_RetV.k[2] = _p0.k[2] + dvz * t;
			return true;
		}
		return false;
	}
	

	void GetIntersectionPoint(const V3& _p0, const V3& _p1, V3& _RetV) const
	{
		T dvx = (_p1.k[0] - _p0.k[0]);
		T dvy = (_p1.k[1] - _p0.k[1]);
		T dvz = (_p1.k[2] - _p0.k[2]);

		T s = dvx*n.k[0] + dvy*n.k[1] + dvz*n.k[2];

		if (s == (T)0)
		{
			_RetV = _p0;
			return;
		}
		T sp = _p0*n + d;
		T t = -sp/s;

		_RetV.k[0] = _p0.k[0] + dvx * t;
		_RetV.k[1] = _p0.k[1] + dvy * t;
		_RetV.k[2] = _p0.k[2] + dvz * t;
	}

	void Translate(const V3& _dV)
	{
		d -= (n * _dV);
	}

	void Transform(const M& _Mat)
	{
		n.MultiplyMatrix3x3(_Mat);
		Translate(V::GetMatrixRow(_Mat, 3));
	}

	//---------------------------------------------------
	void ReflectMatrix3x3(const M& _Mat, M& _RefMat) const
	{
		// Reflects _Mat in _Plane and puts the result in _RefMat
		_RefMat.UnitNot3x3();
		V::GetRow(_Mat, 0).Reflect(n, V::GetRow(_RefMat, 0));
		V::GetRow(_Mat, 1).Reflect(n, V::GetRow(_RefMat, 1));
		V::GetRow(_Mat, 2).Reflect(n, V::GetRow(_RefMat, 2));
	}

	void ReflectMatrix(const M& _Mat, M& _RefMat) const
	{
		// Reflects _Mat in _Plane and puts the result in _RefMat
		_RefMat.UnitNot3x3();
		V::GetRow(_Mat, 0).Reflect(n, V::GetRow(_RefMat, 0));
		V::GetRow(_Mat, 1).Reflect(n, V::GetRow(_RefMat, 1));
		V::GetRow(_Mat, 2).Reflect(n, V::GetRow(_RefMat, 2));
		V::GetRow(_Mat, 3).Combine(n, -T(2.0) * Distance(V::GetRow(_Mat, 3)), V::GetRow(_RefMat, 3));
	}

	//---------------------------------------------------
	CStr GetString() const
	{
		return CStrF("n:(%f, %f, %f) d:%f", n.k[0], n.k[1], n.k[2], d);
	}

	void Read(CCFile* _pFile)
	{
		n.Read(_pFile);
		_pFile->ReadLE(d);
	}

	void Write(CCFile* _pFile) const
	{
		n.Write(_pFile);
		_pFile->WriteLE(d);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		n.SwapLE();
		::SwapLE(d);
	}
#endif

#ifndef DEFINE_MAT43_IS_MAT44
	void ReflectMatrix3x3(const M43& _Mat, M43& _RefMat) const
	{
		// Reflects _Mat in _Plane and puts the result in _RefMat
		_RefMat.UnitNot3x3();
		V::GetRow(_Mat, 0).Reflect(n, V::GetRow(_RefMat, 0));
		V::GetRow(_Mat, 1).Reflect(n, V::GetRow(_RefMat, 1));
		V::GetRow(_Mat, 2).Reflect(n, V::GetRow(_RefMat, 2));
	}

	void ReflectMatrix(const M43& _Mat, M43& _RefMat) const
	{
		// Reflects _Mat in _Plane and puts the result in _RefMat
		_RefMat.UnitNot3x3();
		V::GetRow(_Mat, 0).Reflect(n, V::GetRow(_RefMat, 0));
		V::GetRow(_Mat, 1).Reflect(n, V::GetRow(_RefMat, 1));
		V::GetRow(_Mat, 2).Reflect(n, V::GetRow(_RefMat, 2));
		V::GetRow(_Mat, 3).Combine(n, -T(2.0) * Distance(V::GetRow(_Mat, 3)), V::GetRow(_RefMat, 3));
	}

	void Transform(const M43& _Mat)
	{
		n.MultiplyMatrix3x3(_Mat);
		Translate(V::GetMatrixRow(_Mat, 3));
	}
#endif	
};

template<class T>
class TPlane3 : public TPlane3Aggr<T>
{
	typedef TPlane3Aggr<T> PBase;
	typedef TVector3<T> V;
public:
	M_INLINE TPlane3()
	{
	}

	M_INLINE TPlane3(const TPlane3& _Plane)
	{
		PBase::v=_Plane.v;
	}

	M_INLINE TPlane3(const V& _n, T _d)
	{
		PBase::n = _n;
		PBase::d = _d;
	}

	M_INLINE TPlane3(const V& _n, const V& _p)
	{
		CreateNV(_n, _p);
	}

	M_INLINE TPlane3(const V& _p0, const V& _p1, const V& _p2)
	{
		Create(_p0, _p1, _p2);
	}

	M_FORCEINLINE void operator= (const PBase& _p)
	{
		TPlane3Aggr<T>::v = _p.v;
	}
};



typedef TPlane3<fp32> CPlane3Dfp32;
typedef TPlane3<fp64> CPlane3Dfp64;

// -------------------------------------------------------------------
//  TVector4
// -------------------------------------------------------------------
template<class T>
class TVector4
{
	typedef TVector3<T> V3;
	typedef TMatrix4<T> M;
	typedef TVector4 V;

public:
	typedef T CDataType;

	typedef typename TMathTemplateProperties<T>::TVector4Intrinsic TIntrinsic;

	union
	{
		TIntrinsic v;
		T k[4];
	};

	/*
	// --------------------------------
	// Construction
	// --------------------------------
	TVector4()
	TVector4(T val)										// Construct and fill with val.
	TVector4(T x, T y, T z, T w)						// Construct from components
	TVector4(const V& a)								// Construct from vector

	// --------------------------------
	// Operators
	// --------------------------------
	T& operator[] (int _k)								// Component index-operator
	const T& operator[] (int _k) const					// Component index-operator
	bool operator== (const V& _v) const					// Equal operator
	bool operator!= (const V& _v) const					// Not equal operator
	V operator+ (const V& a) const						// Vector add
	V operator- (const V& a) const						// Vector subtract
	V operator- () const								// Negate
	void operator+= (const V& a)						// Assignment add
	void operator-= (const V& a)						// Assignment subtract
	T operator* (const V& a) const						// Dotproduct
	V operator* (T scalar) const						// Scale
	void operator*= (T scalar)							// Assignment scale
	V operator* (const M& m) const						// Multiply with matrix
	void operator*= (const M& m)						// Assignment matrix-multiply

	// --------------------------------
	// Operations
	// --------------------------------
	void Scale(T scalar, V& dest) const					// Scale with destination.
	void CompMul(const V& a, V& dest) const				// Multiplies components
	void Add(const V& a, V& dest) const					// Add with destination.
	void Sub(const V& a, V& dest) const					// Subtract with destination.
	void Combine(const V& a, T t, V& dest) const		// Combine (dest = this + a*t) with destination.
	void Lerp(const V& a, T t, V& dest) const			// Interpolates between this and a with t.
	void MultiplyMatrix(const M& m, V& dest) const		// Multiply matrix with destination.
	T Distance(const V& a) const						// Distance between this and a
	T DistanceSqr(const V& a) const						// Square-distance between this and a
	T Length() const									// Length of vector. (Magnitude)
	T LengthSqr() const									// Square-length of vector.
	V& Normalize()										// Normalize this, returning reference

	static void MultiplyMatrix(const V* src, V* dest, const M& m, int n)	// Array matrix transform
	static void MultiplyMatrix(const V3* src, V* dest, const M& m, int n)	// Array matrix transform of v3 to v4
	void operator= (const V3& _v)											// V3 assignment
  
	// --------------------------------
	// Helpers
	// --------------------------------
	CStr GetString()									// Get string for output.
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	void ParseString(CStr _s)							// Parse vector from string with digits separated by anything.
	void ParseColor(CStr& _s)							// Parse color in hex or float notation.
	*/

	void Set(T _x, T _y, T _z, T _w)
	{
		k[0] = _x;
		k[1] = _y;
		k[2] = _z;
		k[3] = _w;
	}

	void SetScalar(T _v)
	{
		k[0] = _v;
		k[1] = _v;
		k[2] = _v;
		k[3] = _v;
	}

	TVector4()
	{
	}
	
	TVector4(T val) 
	{
		k[0] = val;
		k[1] = val;
		k[2] = val;
		k[3] = val;
	}

	TVector4(T x, T y, T z, T w)
	{
		k[0] = x;
		k[1] = y;
		k[2] = z;
		k[3] = w;
	}

	TVector4(const V& a)
	{
		v = a.v;
	};

	TVector4(TIntrinsic _v) : v(_v) {};

	void operator= (const TVector4& a)
	{
		v = a.v;
	}

	void operator= (const V3& _v)
	{
		k[0] = _v.k[0];
		k[1] = _v.k[1];
		k[2] = _v.k[2];
		k[3] = 1;
	};

	void operator= (T a)
	{
		k[0] = a;
		k[1] = a;
		k[2] = a;
		k[3] = a;
	}

	void operator= (TIntrinsic _v)
	{
		v = _v;
	}

	operator TIntrinsic() const
	{
		return v;
	}

	T& operator[] (int _k)
	{
		return k[_k];
	}

	const T& operator[] (int _k) const
	{
		return k[_k];
	}

	bool operator== (const V& _v) const
	{
		return ((k[0] == _v.k[0]) && (k[1] == _v.k[1]) && (k[2] == _v.k[2]) && (k[3] == _v.k[3]));
	};

	bool operator!= (const V& _v) const
	{
		return ((k[0] != _v.k[0]) || (k[1] != _v.k[1]) || (k[2] != _v.k[2]) || (k[3] != _v.k[3]));
	};

	V operator+ (const V& a) const
	{
		return V(k[0]+a.k[0], k[1]+a.k[1], k[2]+a.k[2], k[3]+a.k[3]);
	}
	
	V operator- (const V& a) const
	{
		return V(k[0]-a.k[0], k[1]-a.k[1], k[2]-a.k[2], k[3]-a.k[3]);
	};

	V operator- () const
	{
		return V(-k[0], -k[1], -k[2], -k[3]);
	};

	void operator+= (const V& a)
	{
		k[0] += a.k[0];
		k[1] += a.k[1];
		k[2] += a.k[2];
		k[3] += a.k[3];
	};
	
	void operator-= (const V& a)
	{
		k[0] -= a.k[0];
		k[1] -= a.k[1];
		k[2] -= a.k[2];
		k[3] -= a.k[3];
	};
	
	T operator* (const V& a) const
	{
		return (k[0]*a.k[0] + k[1]*a.k[1] + k[2]*a.k[2] + k[3]*a.k[3]);
	};

	V operator* (T scalar) const
	{
		return V(k[0]*scalar, k[1]*scalar, k[2]*scalar, k[3]*scalar);
	};

	void operator*= (T scalar) 
	{
		k[0] *= scalar;
		k[1] *= scalar;
		k[2] *= scalar;
		k[3] *= scalar;
	};

	V operator* (const M& m) const
	{
		V r;
#ifdef PLATFORM_SHINOBI
		mtrxload(m.k);
		ftrv(k, r.k);
#else
		for (int i=0; i<4; i++)
			r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i]*k[3];
#endif
		return r;
	};

	void operator*= (const M& m)
	{
		V r;
#ifdef PLATFORM_SHINOBI
		mtrxload(m.k);
		ftrv(k, r.k);
#else
		for (int i=0; i<4; i++)
			r.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i]*k[3];
#endif
		k[0] = r.k[0];
		k[1] = r.k[1];
		k[2] = r.k[2];
		k[3] = r.k[3];
	};

	void Scale(T scalar, V& dest) const
	{
		dest.k[0] = k[0]*scalar;
		dest.k[1] = k[1]*scalar;
		dest.k[2] = k[2]*scalar;
		dest.k[3] = k[3]*scalar;
	}

	void CompMul(const V& a, V& dest) const
	{
		dest.k[0] = k[0] * a.k[0];
		dest.k[1] = k[1] * a.k[1];
		dest.k[2] = k[2] * a.k[2];
		dest.k[3] = k[3] * a.k[3];
	}

	void Add(const V& a, V& dest) const
	{
		dest.k[0] = k[0] + a.k[0];
		dest.k[1] = k[1] + a.k[1];
		dest.k[2] = k[2] + a.k[2];
		dest.k[3] = k[3] + a.k[3];
	}

	void Sub(const V& a, V& dest) const
	{
		dest.k[0] = k[0] - a.k[0];
		dest.k[1] = k[1] - a.k[1];
		dest.k[2] = k[2] - a.k[2];
		dest.k[3] = k[3] - a.k[3];
	}

	void Combine(const V& a, T t, V& dest) const
	{
		dest.k[0] = k[0] + t*a.k[0];
		dest.k[1] = k[1] + t*a.k[1];
		dest.k[2] = k[2] + t*a.k[2];
		dest.k[3] = k[3] + t*a.k[3];
	}

	void Lerp(const V& a, T t, V& dest) const
	{
		dest.k[0] = k[0] + t*(a.k[0] - k[0]);
		dest.k[1] = k[1] + t*(a.k[1] - k[1]);
		dest.k[2] = k[2] + t*(a.k[2] - k[2]);
		dest.k[3] = k[3] + t*(a.k[3] - k[3]);
	}

	void Moderate(const V& newq, V& qprim, int a)
	{
		const T Scale = T(512.0);
		for(int i = 0; i < 4; i++)
		{
			int x = newq.k[i]*Scale;
			int xprim = qprim.k[i]*Scale;
			Moderate(x, newq.k[i]*Scale, xprim, a);
			newq.k[i] = T(x) / Scale;
			qprim.k[i] = T(xprim) / Scale;
		}
	}

	void MultiplyMatrix(const M& m, V& dest) const
	{
#ifdef PLATFORM_SHINOBI
		mtrxload(m.k);
		mtrx4mul(k, dest.k);
#else
		for (int i = 0; i < 4; i++)
			dest.k[i] = m.k[0][i]*k[0] + m.k[1][i]*k[1] + m.k[2][i]*k[2] + m.k[3][i]*k[3];
#endif
	};

	T Distance(const V& a) const
	{
		return (T)M_Sqrt(Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]) + Sqr(k[2] - a.k[2]) + Sqr(k[3] - a.k[3]));
	}

	T DistanceInv(const V& a) const
	{
		return (T)M_InvSqrt(Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]) + Sqr(k[2] - a.k[2]) + Sqr(k[3] - a.k[3]));
	}

	T DistanceSqr(const V& a) const
	{
		return Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]) + Sqr(k[2] - a.k[2]) + Sqr(k[3] - a.k[3]);
	}

	T LengthInv() const
	{
		return (T(M_InvSqrt(Sqr(k[0]) + Sqr(k[1]) + Sqr(k[2]) + Sqr(k[3]))));
	}

	T Length() const
	{
		return (T(M_Sqrt(Sqr(k[0]) + Sqr(k[1]) + Sqr(k[2]) + Sqr(k[3]))));
	}

	T LengthSqr() const
	{
		return Sqr(k[0]) + Sqr(k[1]) + Sqr(k[2]) + Sqr(k[3]);
	}

#ifdef PLATFORM_PS2
	V& Normalize()
	{
		T inverse = LengthInv();
//		M_ASSERT( dividend != T(0), "Null-vector is _bad_ input.");

		k[0] = k[0]*inverse;
		k[1] = k[1]*inverse;
		k[2] = k[2]*inverse;
		k[3] = k[3]*inverse;

		return *this;
	}
#else
	V& Normalize()
	{
		T dividend;
		T inverse;

		dividend = Length();
		if (dividend != (T)0)
		{
			inverse = (T)1.0 / dividend;
			k[0] = k[0]*inverse;
			k[1] = k[1]*inverse;
			k[2] = k[2]*inverse;
			k[3] = k[3]*inverse;
		}
		else
		{
			k[0] = k[1] = k[2] = k[3] = 0;
		}
		return *this;
	}
#endif	

	// V4*M4 -> V4
	static void MultiplyMatrix(const V* src, V* dest, const M& m, int n)
	{
#ifdef PLATFORM_SHINOBI
		mtrxload(m.k);
		for (int vnr = 0; vnr < n; vnr++)
			ftrv(src[vnr].k, dest[vnr].k);
#else
		for (int vnr = 0; vnr < n; vnr++)
		{
			for (int i = 0; i < 4; i++)
				dest[vnr].k[i] = m.k[0][i]*src[vnr].k[0] + m.k[1][i]*src[vnr].k[1] + m.k[2][i]*src[vnr].k[2] + m.k[3][i];
		};
#endif
	};

	// V3*M4 -> V4
	static void MultiplyMatrix(const V3* src, V* dest, const M& m, int n)
	{
		for (int vnr = 0; vnr < n; vnr++)
		{
			for (int i = 0; i < 4; i++)
				dest[vnr].k[i] = m.k[0][i]*src[vnr].k[0] + m.k[1][i]*src[vnr].k[1] + m.k[2][i]*src[vnr].k[2] + m.k[3][i];
		};
/*		for (int vnr = 0; vnr < n; vnr++) {
			for (int i = 0; i < 3; i++)
				dest[vnr].k[i] = m.k[i][0]*src[vnr].k[0] + m.k[i][1]*src[vnr].k[1] + m.k[i][2]*src[vnr].k[2] + m.k[i][3];
			dest[vnr].k[3] = m.k[3][0] + m.k[3][1] + m.k[3][2] + m.k[3][3];
		};*/
	};


	//---------------------------------------------------
	void Read(CCFile* _pFile)
	{
		_pFile->ReadLE(k[0]);
		_pFile->ReadLE(k[1]);
		_pFile->ReadLE(k[2]);
		_pFile->ReadLE(k[3]);
	}

	void Write(CCFile* _pFile) const
	{
		_pFile->WriteLE(k[0]);
		_pFile->WriteLE(k[1]);
		_pFile->WriteLE(k[2]);
		_pFile->WriteLE(k[3]);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(k[0]);
		::SwapLE(k[1]);
		::SwapLE(k[2]);
		::SwapLE(k[3]);
	}
#endif
	//---------------------------------------------------
	CStr GetString() const
	{
		return CStrF("(%.3f,%.3f,%.3f,%.3f)", k[0], k[1], k[2], k[3]);
	};

	CFStr GetFilteredString(int _iType = 0) const
	{
		switch(_iType)
		{
		case 0: return CFStrF("%s,%s,%s,%s", (const char *)CFStr::GetFilteredString(k[0]), (const char *)CFStr::GetFilteredString(k[1]), (const char *)CFStr::GetFilteredString(k[2]), (const char *)CFStr::GetFilteredString(k[3]));
		case 1: return CFStrF("%s %s %s %s", (const char *)CFStr::GetFilteredString(k[0]), (const char *)CFStr::GetFilteredString(k[1]), (const char *)CFStr::GetFilteredString(k[2]), (const char *)CFStr::GetFilteredString(k[3]));
		}
		return CFStr();
	};

	V& ParseString(const CStr& _s)
	{
		const char* pStr = (const char*) _s;
		if (!pStr) { *this = 0; return *this; }
		int pos = 0;
		int len = _s.Len();
		for(int i = 0; i < 4; i++)
		{
			pos = CStr::GoToDigit(pStr, pos, len);
			k[i] = M_AToF(&pStr[pos]);
			pos = CStr::SkipADigit(pStr, pos, len);
		}
		return *this;
	}

	V& ParseColor(const CStr& _s, bool _bHexUnit = false)
	{
		if (_s.CompareSubStr("0x") == 0)
		{
			int c = _s.Val_int();
			k[0] = (c >> 16) & 0xff;
			k[1] = (c >> 8) & 0xff;
			k[2] = (c >> 0) & 0xff;
			k[3] = (c >> 24) & 0xff;
			if (_bHexUnit)
				*this *= T(1.0) / T(255.0);
		}
		else
		{
			const char* pStr = (const char*) _s;
			if (!pStr) { *this = TVector4(T(0.0)); return *this; }
			int len = CStr::StrLen(pStr);
			int pos = 0;
			pos = CStr::GoToDigit(pStr, pos, len);
			int i = 0;
			for(; i < 4; i++)
			{
				k[i] = M_AToF(&pStr[pos]);
				pos = CStr::SkipADigit(pStr, pos, len);
				pos = CStr::GoToDigit(pStr, pos, len);
				if (pos == len) break;
			}

			if (i == 0) 
				*this = TVector4(k[0]);
			else if (i == 2)
				k[3] = _bHexUnit ? T(1.0) : T(255.0);
		}
		return *this;
	}
};

typedef TVector4<fp32> CVec4Dfp32;
typedef TVector4<fp64> CVec4Dfp64;
typedef TVector4<uint32> CVec4Duint32;
typedef TVector4<int32> CVec4Dint32;

// -------------------------------------------------------------------
//  TVector2
// -------------------------------------------------------------------
template <class T>
class TVector2Aggr
{
	/*
	Summary of operators:

		+	Add
		-	Sub
		-	Neg
		+=	Increment
		-=	Decrement
		*	Multiply
		*	Scalar product
		/	Cross product
	
	*/

public:
	T k[2];
	typedef TMatrix2<T> M;
	typedef TVector2Aggr<T> V;
	typedef T CDataType;

/*	M_FORCEINLINE void operator= (const V& _v)
	{
		T x = _v.k[0];
		T y = _v.k[1];
		k[0] = x;
		k[1] = y;
	}*/
	
	M_FORCEINLINE void Set(T _x, T _y)
	{
		k[0] = _x;
		k[1] = _y;
	}

	M_FORCEINLINE void SetScalar(T _Scalar)
	{
		k[0] = _Scalar;
		k[1] = _Scalar;
	}

	M_FORCEINLINE const T& operator[] (int _k) const
	{
		return k[_k];
	}

	M_FORCEINLINE T& operator[] (int _k)
	{
		return k[_k];
	}

	bool operator== (const V& _v) const
	{
		return ((k[0] == _v.k[0]) && (k[1] == _v.k[1]));
	};

	bool operator!= (const V& _v) const
	{
		return ((k[0] != _v.k[0]) || (k[1] != _v.k[1]));
	};

	V operator+ (const V& a) const
	{
		V Ret;
		Ret[0] = k[0] + a.k[0];
		Ret[1] = k[1] + a.k[1];
		return Ret;
	}
	
	V operator- (const V& a) const
	{
		V Ret;
		Ret[0] = k[0] - a.k[0];
		Ret[1] = k[1] - a.k[1];
		return Ret;
	};

	V operator- () 	
	{
		V Ret;
		Ret[0] = -k[0];
		Ret[1] = -k[1];
		return Ret;
	};

	void operator+= (const V& a)
	{
		k[0] += a.k[0];
		k[1] += a.k[1];
	};
	
	void operator-= (const V& a)
	{
		k[0] -= a.k[0];
		k[1] -= a.k[1];
	};
	
	T operator* (const V& a) const
	{
		return (k[0]*a.k[0] + k[1]*a.k[1]);
	};

	V operator* (T scalar) const
	{
		V Ret;
		Ret[0] = k[0]*scalar;
		Ret[1] = k[1]*scalar;
		return Ret;
	};

//	V operator* (int scalar) const
//	{
//		return V(k[0]*(T)scalar, k[1]*(T)scalar);
//	};

	void operator*= (T scalar) 
	{
		k[0] *= scalar;
		k[1] *= scalar;
	};

	T operator/ (const V& a) const
	{
		return k[0]*a.k[1] - k[1]*a.k[0];
	}

	void Scale(T scalar, V& dest) const
	{
		dest.k[0] = k[0]*scalar;
		dest.k[1] = k[1]*scalar;
	}

	T CrossProd(const V& a) const
	{
		return k[0]*a.k[1] - k[1]*a.k[0];
	}

	void CompMul(const V& a, V& dest) const
	{
		dest.k[0] = k[0] * a.k[0];
		dest.k[1] = k[1] * a.k[1];
	}

	void Add(const V& a, V& dest) const
	{
		dest.k[0] = k[0] + a.k[0];
		dest.k[1] = k[1] + a.k[1];
	}

	void Sub(const V& a, V& dest) const
	{
		dest.k[0] = k[0] - a.k[0];
		dest.k[1] = k[1] - a.k[1];
	}

	void Combine(const V& a, T t, V& dest) const
	{
		dest.k[0] = k[0] + t*a.k[0];
		dest.k[1] = k[1] + t*a.k[1];
	}

	void Lerp(const V& a, T t, V& dest) const
	{
		dest.k[0] = k[0] + t*(a.k[0] - k[0]);
		dest.k[1] = k[1] + t*(a.k[1] - k[1]);
	}

	void Moderate(const V& newq, V& qprim, int a)
	{
		const T Scale = T(512.0);
		for(int i = 0; i < 2; i++)
		{
			int x = newq.k[i]*Scale;
			int xprim = qprim.k[i]*Scale;
			Moderate(x, newq.k[i]*Scale, xprim, a);
			newq.k[i] = T(x) / Scale;
			qprim.k[i] = T(xprim) / Scale;
		}
	}

	T Distance(const V& a) const
	{
		return (T)M_Sqrt(Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]));
	}

	T DistanceInv(const V& a) const
	{
		return (T)M_InvSqrt(Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]));
	}

	T DistanceSqr(const V& a) const
	{
		return Sqr(k[0] - a.k[0]) + Sqr(k[1] - a.k[1]);
	}

	T LengthInv() const
	{
		return (T(M_InvSqrt(Sqr(k[0]) + Sqr(k[1]))));
	}

	T Length() const
	{
		return (T(M_Sqrt(Sqr(k[0]) + Sqr(k[1]))));
	}

	T LengthSqr() const
	{
		return Sqr(k[0]) + Sqr(k[1]);
	}

#if	defined(PLATFORM_PS2)	|| defined(PLATFORM_DOLPHIN)
	void Normalize()
	{
		T inverse = LengthInv();
//		M_ASSERT( dividend != T(0), "Null-vector is _bad_ input.");

		k[0] = k[0]*inverse;
		k[1] = k[1]*inverse;
	}
	
	void SetLength( const T& _Length )
	{
		T scale = _Length * LengthInv();
		
		k[0]	*= scale;
		k[1]	*= scale;
	}
#else
	void Normalize()
	{
		T dividend;
		T inverse;

		dividend = Length();
		if (dividend != T(0.0))
		{
			inverse = T(1.0) / dividend;
			k[0] = k[0]*inverse;
			k[1] = k[1]*inverse;
		}
		else
		{
			k[0] = k[1] = 0;
		}
	}

	void SetLength( const T& _Length )
	{
		T dividend = Length();
		if (dividend != T(0.0))
		{
			T scale = _Length / dividend;
			k[0] = k[0]*scale;
			k[1] = k[1]*scale;
		}
		else
		{
			k[0] = k[1] = 0;
		}
	}
#endif	

	T GetAngle()
	{
		/*
			(0, 0) => 0
			(0, 1) => 0
			(1, 0) => 0.25
			(0, -1) => 0.5
			(-1, 0) => 0.75
		*/
		T x = k[0];
		T y = k[1];
		T FullAngle = 2.0f * _PI;
		T Angle;
		if (x != 0)
		{
			Angle = M_ATan(y / x) / FullAngle;
			if (x < 0)
				Angle += 0.5f;
			else if (y < 0) 
				Angle += 1.0f;	
		}
		else
		{
			if (y < 0)
				Angle = 0.75f; 
			else if (y > 0)
				Angle = 0.25f; 
			else
				Angle = 0; 
		}
		return Angle;
	}

	//---------------------------------------------------

	uint32 Pack32(fp32 _Max)
	{
		fp32 xf = (k[0] / (_Max * 2) + 0.5f);
		fp32 yf = (k[1] / (_Max * 2) + 0.5f);
		uint32 xi = uint32(xf * (fp32)0xFFFF) & 0xFFFF;
		uint32 yi = uint32(yf * (fp32)0xFFFF) & 0xFFFF;
		return ((xi << 16) | yi);
	}

	void Unpack32(uint32 _V, fp32 _Max)
	{
		uint32 xi = (_V >> 16) & 0xFFFF;
		uint32 yi = _V & 0xFFFF;
		fp32 xf = xi / (fp32)(0xFFFF);
		fp32 yf = yi / (fp32)(0xFFFF);
		k[0] = (xf - 0.5f) * _Max * 2;
		k[1] = (yf - 0.5f) * _Max * 2;
	}

	//---------------------------------------------------

	uint32 Pack16(fp32 _Max)
	{
		fp32 xf = (k[0] / (_Max * 2) + 0.5f);
		fp32 yf = (k[1] / (_Max * 2) + 0.5f);
		uint32 xi = uint32(xf * (fp32)0xFF) & 0xFF;
		uint32 yi = uint32(yf * (fp32)0xFF) & 0xFF;
		return ((xi << 8) | yi);
	}

	void Unpack16(uint32 _V, fp32 _Max)
	{
		uint32 xi = (_V >> 8) & 0xFF;
		uint32 yi = _V & 0xFF;
		fp32 xf = xi / (fp32)(0xFF);
		fp32 yf = yi / (fp32)(0xFF);
		k[0] = (xf - 0.5f) * _Max * 2;
		k[1] = (yf - 0.5f) * _Max * 2;
	}

	static void SplineCardinal(const T _a,const V *pP0,const V *pP1,const V *pP2,const V *pPt3, 
		V* _pDest,const T _tFrac,const T _t01,const T _t12,const T _t23,int _nV)
	{
		T tSqr = Sqr(_tFrac);
		T tCube = tSqr * _tFrac;

		T h1 = 2.0f * tCube - 3.0f * tSqr + 1.0f;
		T h2 = -2.0f * tCube + 3.0f * tSqr;
		T h3 = tCube - 2.0f * tSqr + _tFrac;
		T h4 = tCube - tSqr;

		// Account for various timedeltas
		T ts2 = 2.0f * _t12 /(_t12 + _t23);
		T ts1 = 2.0f * _t12 /(_t01 + _t12);

		for(int iV = 0; iV < _nV; iV++)
		{
			// Cardinal splines uses variable tension (a)
			// Catmull-Clarke splines uses a fixed 0.5 (a)
			V T1 = (pP2[iV] - pP0[iV]) * _a * ts1;
			V T2 = (pPt3[iV] - pP1[iV]) * _a * ts2;

			V p = pP1[iV] * h1;
			p += pP2[iV] * h2;
			p += T1 * h3;
			p += T2 * h4;

			_pDest[iV] = p;
		}
	}

	// Mondelore: short, simple and it bloody works!
	static void Spline2(const V& _v0, const V& _v1, const V& _v2, const V& _v3, T _t, V& _dest)
	{
		T t = _t;
		T t2 = t * t;
		T t3 = t2 * t;

		V p, q, r, s;

		p = (_v3 - _v2) - (_v0 - _v1);
		q = (_v0 - _v1) - p;
		r = _v2 - _v0;
		s = _v1;

		_dest = p * t3 + q * t2 + r * t + s;
	}

	static void Spline3(const V& _v0, const V& _v1, const V& _v2, const V& _v3, T _t, V& _dest)
	{
		T t, t2, t3;
		t = _t;
		t2 = t * t;
		t3 = t2 * t;

		T m0, m1, m2, m3;
		m0 = -t3 + 2 * t2 - t;
		m1 = t3 - 2 * t2  + 1;
		m2 = -t3 + t2 + t;
		m3 = t3 - t2;
		_dest = _v0 * m0 + _v1 * m1 + _v2 * m2 + _v3 * m3;
	}

	//---------------------------------------------------
	// Bounding
	//---------------------------------------------------
	static void GetMinBoundCircle(const V* src, V& p0, T& radius, int n)
	{
		if (n == 0) { p0 = 0; radius = 0; return; };

		// Find approx center.
		V c(0);
		{ for (int v = 0; v < n; v++) c+= src[v]; };
		c *= 1/n;

		// Find max sqr-radius.
		T maxrsqr = 0;
		{
			for (int v = 0; v < n; v++)
			{
				T rsqr = Sqr(src[v].k[0]-c.k[0]) + Sqr(src[v].k[1]-c.k[1]);
				if (rsqr > maxrsqr) maxrsqr = rsqr;
			};
		};
		p0 = c;
		radius = M_Sqrt(maxrsqr);
	};

	static void GetMinBoundRect(const V* src, V& _min, V& _max, int n)
	{
		if (n == 0)
		{ 
			_min.SetScalar(0); 
			_max.SetScalar(0);
			return;
		};

		V min = src[0];
		V max = src[0];
		for (int v = 1; v < n; v++)
		{
			min.k[0] = Min(src[v].k[0], min.k[0]);
			max.k[0] = Max(src[v].k[0], max.k[0]);
			min.k[1] = Min(src[v].k[1], min.k[1]);
			max.k[1] = Max(src[v].k[1], max.k[1]);
		};
		_min = min;
		_max = max;
	};

	bool AlmostEqual(const V& _v, T _Margin) const
	{
		if (Abs(k[0] - _v.k[0]) > _Margin) return false;
		if (Abs(k[1] - _v.k[1]) > _Margin) return false;
		return true;
	}

	//---------------------------------------------------
	// Fetare funktioner:
	//---------------------------------------------------
	T Point2LineDistance(const V& p0, const V& l0, const V& l1)
	{
		V v0(p0-l0);
		V a(l1-l0);

		if ((v0*a) <= 0)
			return v0.Length();
		else 
		{
			V v1(p0-l1);
			if ((v1*a) >= 0)
				return v1.Length();
			else
				return ((a/v1).Length()) / a.Length();
		}
	};

	void Project(const V& b, V& r) const
	{
		// projicerar this i b och lägger det i r
		T blen2 = (b*b);
		if (blen2 == 0) { r = 0; return; };
		T s = (*this)*b / (b*b);
		b.Multiply(s, r);
	};

	void Reflect(const V& n, V& r) const
	{
		// this pekar "ner" i planet n, r pekar up ifrån planet.
		V np;
		Project(n, np);
		np.Multiply(-2, r);
		r += (*this);
	};

	int PointOnLineSide(const V& n, const V& pi)
	{
		// n = line normal
		// pi = point on line.

		// retur: 1 = normal sidan.
		//		 -1 = baksidan
		//        0 = i planet.

		T s = (k[0]-pi.k[0]) * n.k[0] + 
			(k[1]-pi.k[1]) * n.k[1];
		if (s < 0) return -1;
		if (s > 0) return 1;
		return 0;
	};

	//---------------------------------------------------
	void Snap(T _Grid, T _SnapTresh)
	{
		for(int i = 0; i < 2; i++)
			k[i] = SnapFloat(k[i], _Grid, _SnapTresh);
	}

	V GetSnapped(T _Grid, T _SnapTresh) const
	{
		V s;
		for(int i = 0; i < 2; i++)
			s.k[i] = SnapFloat(k[i], _Grid, _SnapTresh);
		return s;
	}

	//---------------------------------------------------
	CStr GetString() const
	{
		return CStrF("(%.3f,%.3f)", k[0], k[1]);
	};

	V& ParseString(const CStr& _s)
	{
		const char* pStr = (const char*) _s;
		if (!pStr) { SetScalar(0); return *this; }
		int pos = 0;
		int len = _s.Len();
		fp32 last = 0.0f;
		for(int i = 0; i < 2; i++)
		{
			pos = CStr::GoToDigit(pStr, pos, len);
			if (pStr[pos] != 0)
				k[i] = last = M_AToF(&pStr[pos]);
			else
				k[i] = last;
			pos = CStr::SkipADigit(pStr, pos, len);
		}
		return *this;
	}

	CStr GetFilteredString(int _iType = 0) const
	{
		switch(_iType)
		{
		case 0: return CStrF("%s,%s", (const char *)CStr::GetFilteredString(k[0]), (const char *)CStr::GetFilteredString(k[1]));
		case 1: return CStrF("%s %s", (const char *)CStr::GetFilteredString(k[0]), (const char *)CStr::GetFilteredString(k[1]));
		}
		return CStr();
	};

	void Read(CCFile* _pFile)
	{
		_pFile->ReadLE(k[0]);
		_pFile->ReadLE(k[1]);
	}

	void Write(CCFile* _pFile) const
	{
		_pFile->WriteLE(k[0]);
		_pFile->WriteLE(k[1]);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(k[0]);
		::SwapLE(k[1]);
	}
#endif
};

template<typename T>
class TVector2 : public TVector2Aggr<T>
{
public:
	typedef TVector2Aggr<T> V;

	TVector2()
	{
	}

	TVector2(T val) 
	{
		V::k[0] = val;
		V::k[1] = val;
	}

	TVector2(T x, T y)
	{
		V::k[0] = x;
		V::k[1] = y;
	}

	TVector2(const V& a)
	{
		V::k[0] = a.k[0];
		V::k[1] = a.k[1];
	};

	M_FORCEINLINE void operator= (const V& _v)
	{
		T x = _v.k[0];
		T y = _v.k[1];
		V::k[0] = x;
		V::k[1] = y;
	}
};

typedef TVector2<fp32> CVec2Dfp32;
typedef TVector2<fp64> CVec2Dfp64;

typedef TVector2<uint8> CVec2Duint8;
typedef TVector2<uint16> CVec2Duint16;
typedef TVector2<uint32> CVec2Duint32;
typedef TVector2<int8> CVec2Dint8;
typedef TVector2<int16> CVec2Dint16;
typedef TVector2<int32> CVec2Dint32;
typedef TVector2<int> CVec2Dint;

// -------------------------------------------------------------------
//  TComplex
// -------------------------------------------------------------------
template <class T>
class TComplex
{
public:
	T re;
	T im;

	typedef TComplex type;

	TComplex()
	{
	}
	
	TComplex(T _re)
	{
		re = _re;
		im = 0;
	}

	TComplex(T _re, T _im)
	{
		re = _re;
		im = _im;
	}

	inline type operator- ()
	{
		return type(-re, -im);
	}
	
	inline type operator+ (type a)
	{
		return type(re + a.re, im + a.im);
	}

	inline type operator- (type a)
	{
		return type(re - a.re, im - a.im);
	}

	inline type& operator+= (type a)
	{
		re += a.re;
		im += a.im;
		return(this);
	}

	inline type& operator-= (type a)
	{
		re -= a.re;
		im -= a.im;
		return(this);
	}

	inline type operator* (type a)
	{
		return type(re*a.re - im*a.im, re*a.im + im*a.re);
	}

	inline type operator* (T a)
	{
		return type(re*a, im*a);
	}
	
	type operator/ (type a)
	{
		T dividend;
		dividend = Sqr(a.re) + Sqr(a.im);
		if (dividend==0)
		{
			return type(0,0);
		}
		else
		{
			return type((re*a.re + im*a.im)/dividend, 
				        (im*a.re - re*a.im)/dividend);
		}
	}

/*	inline complex operator T (T a)
	{
		return complex(a);
	};*/

	inline T len()
	{
		return length2(re, im);
	};

	inline int iscomplex()
	{
		return(im != (T)0);
	};

	inline int isreal()
	{
		return(im == (T)0);
	};
};

typedef TComplex<fp32> cfp32;
typedef TComplex<fp64> cfp64;
//typedef TComplex<fp10> cfp10;

// -------------------------------------------------------------------
//  TQuaternion
// -------------------------------------------------------------------
template<class T>
class TQuaternion
{
	// sqrt(sqr(x)+sqr(y)+sqr(z)+sqr(w)) == 1
	// cos(2v) = w
	typedef TQuaternion<T> Q;
	typedef TMatrix4<T> M;
	typedef TVector3Aggr<T> V;
	typedef TMatrix43<T> M43;
	typedef typename TMathTemplateProperties<T>::TVector4Intrinsic TIntrinsic;

public:
	union
	{
		TIntrinsic v;
		T k[4];
	};

	/*
	// --------------------------------
	// Construction
	// --------------------------------
	void Create(const V& _v, T _a)						// Create from axis-angle
	void Create(const M& _Mat)							// Create from matrix
	TQuaternion()
	TQuaternion(const V& _v, T _a)						// Construct from axis-angle
	void Unit()

	// --------------------------------
	// Operations
	// --------------------------------
	void Normalize()
	void Inverse()										// Inverse-rotation
	T DotProd(const TQuaternion& _Q) const				// Dot product
	void Interpolate(const TQuaternion& _Other, TQuaternion& _Dest, fp32 _t) const
	void Multiply(const TQuaternion& _Quat2)			// same as *= operator
	void Multiply(const TQuaternion& _Quat2, TQuaternion& _QDest) const

	// --------------------------------
	// Convertion
	// --------------------------------
	T CreateAxisAngle(V& _v) const						// Create axis-angle from quaternion
	void CreateMatrix3x3(M& _Mat) const					// Create matrix from quaterion, changing only the 3x3 part.
	void CreateMatrix(M& _Mat) const					// Create matrix from quaterion, setting translation to zero.

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	CStr GetString() const								// Create string for output.
	*/

	// Create from axis-angle
	M_FORCEINLINE void operator= (const Q& _q)
	{
		v = _q.v;
	}

	void Create(const V& _v, T _Angle)
	{
		T x = _v.k[0];
		T y = _v.k[1];
		T z = _v.k[2];
		T lensqr = Sqr(x) + Sqr(y) + Sqr(z);
		
//		if (Abs(lensqr - T(1.0f)) > T(0.0001f))
		if (Abs(lensqr - T(1.0f)) > 0)
		{
			const T length = (T) T(1.0) * M_InvSqrt(lensqr);
			x = x * length;
			y = y * length;
			z = z * length;
		}
		
#if defined(PLATFORM_PS2) && !defined( M_RTM )
		if( _Angle != 0.0f && M_Fabs( _Angle ) < 0.001f )
		{
//			FooBreak();
			_Angle = 0.0f;
			scePrintf( "Angle is too small for M_Cos/M_Sin to handle properly!!!\n" );
		}
#endif

		T a = _Angle * ( T(1.0f) / T(2.0f) * _PI * T(2.0f) );
		k[3] = M_Cos(a);
		const T sn = M_Sin(a);
		k[0] = x * sn;
		k[1] = y * sn;
		k[2] = z * sn;

	};
	
	TQuaternion& ParseString(const CStr& _s)
	{
		const char* pStr = (const char*) _s;
		if (!pStr) { return *this; }
		int pos = 0;
		int len = _s.Len();
		for(int i = 0; i < 4; i++)
		{
			pos = CStr::GoToDigit(pStr, pos, len);
			k[i] = M_AToF(&pStr[pos]);
			pos = CStr::SkipADigit(pStr, pos, len);
		}
		return *this;
	}

	// Create from matrix
	void Create(const M& _Mat)
	{
		// 7 Add
		// 1 Sqrt
		// 4 Mul
		// 1 Div
		T Trace = (T)0;
		for (int i = 0; i < 3; i++)
			Trace += _Mat.k[i][i];

		if (Trace > (T)0)
		{
			T s = M_Sqrt(Trace + (T)1.0f);
			k[3] = s*T(0.5f);
			s = T(0.5f) / s;

			for (int i = 0; i < 3; i++)
			{
				int j = (i == 2) ? 0 : (i + 1);
				int m = (j == 2) ? 0 : (j + 1);
				k[i] = (_Mat.k[m][j]-_Mat.k[j][m]) * s;
			}
		}
		else
		{
			int i = 0;
			if (_Mat.k[1][1] > _Mat.k[0][0]) i = 1;
			if (_Mat.k[2][2] > _Mat.k[i][i]) i = 2;
			int j = (i == 2) ? 0 : (i + 1);
			int m = (j == 2) ? 0 : (j + 1);

			T s = M_Sqrt((_Mat.k[i][i] - (_Mat.k[j][j] + _Mat.k[m][m])) + (T)1.0f);
			k[i] = s * T(0.5f);
			s = T(0.5f) / s;
			k[3] = (_Mat.k[m][j] - _Mat.k[j][m])*s;
			k[j] = (_Mat.k[j][i] + _Mat.k[i][j])*s;
			k[m] = (_Mat.k[m][i] + _Mat.k[i][m])*s;
		}
	}

	TQuaternion()
	{
	};

	TQuaternion(const V& _v, T _a)
	{
		Create(_v, _a);
	};

	void Unit()
	{
		k[0] = (T) 0.0f;
		k[1] = (T) 0.0f;
		k[2] = (T) 0.0f;
		k[3] = (T) 1.0f;
	};
	
	M_INLINE void Normalize()
	{
		T k0 = k[0];
		T k1 = k[1];
		T k2 = k[2];
		T k3 = k[3];
		T InvLen = M_InvSqrt(Sqr(k0) + Sqr(k1) + Sqr(k2) + Sqr(k3));
		if (InvLen == (T) 0) return;
		k[0] = k0 * InvLen;
		k[1] = k1 * InvLen;
		k[2] = k2 * InvLen;
		k[3] = k3 * InvLen;
	}

	M_INLINE void Inverse()
	{
		k[0] = -k[0];
		k[1] = -k[1];
		k[2] = -k[2];
//		k[3] = k[3];
	}

	M_INLINE T DotProd(const TQuaternion& _Q) const
	{
		T k0 = k[0];
		T k1 = k[1];
		T k2 = k[2];
		T k3 = k[3];
		T qk0 = _Q.k[0];
		T qk1 = _Q.k[1];
		T qk2 = _Q.k[2];
		T qk3 = _Q.k[3];
		return k0*qk0 + k1*qk1 + k2*qk2 + k3*qk3;
	}

	M_INLINE void Lerp(const TQuaternion& _Other, T _t, TQuaternion& _Dest) const
	{
		T a0 = k[0];
		T a1 = k[1];
		T a2 = k[2];
		T a3 = k[3];

		T dot = (a0 * _Other.k[0] + a1 * _Other.k[1] + a2 * _Other.k[2] + a3 * _Other.k[3]);
		T u = M_FSel(dot, (T)1.0 - _t, _t - (T)1.0);

		T d0 = a0 * u + _Other.k[0] * _t;
		T d1 = a1 * u + _Other.k[1] * _t;
		T d2 = a2 * u + _Other.k[2] * _t;
		T d3 = a3 * u + _Other.k[3] * _t;
		T InvLen = M_InvSqrt(Sqr(d0) + Sqr(d1) + Sqr(d2) + Sqr(d3));
#ifndef M_RTM
		if (InvLen == (T)0)
			return;
#endif
		_Dest.k[0] = d0 * InvLen;
		_Dest.k[1] = d1 * InvLen;
		_Dest.k[2] = d2 * InvLen;
		_Dest.k[3] = d3 * InvLen;
	}
	
	static void Spline(const TQuaternion* pQuatA0, const TQuaternion* pQuatA1, const TQuaternion* pQuatA2,
		const TQuaternion* pQuatB0, const TQuaternion* pQuatB1, const TQuaternion* pQuatB2, 
		TQuaternion* _pDest, T _tFrac, T _tA0, T _tA1, T _tB0, T _tB1, int _nQ)
	{
		T tSqr = Sqr(_tFrac);
		T tCube = tSqr * _tFrac;

		const T k = T(0.5f);
		register T tsA0 = (k * _tA1) / _tA0;
		register T tsA1 = (k * _tA1) / _tA1;
		register T tsB0 = (k * _tA1) / _tB0;
		register T tsB1 = (k * _tA1) / _tB1;

		for(int iQ = 0; iQ < _nQ; iQ++)
		{
			// dQuatA
			T dQA[4];
			if (pQuatA0[iQ].DotProd(pQuatA1[iQ]) < T(0.0f))
			{
				dQA[0] = -(pQuatA1[iQ].k[0] + pQuatA0[iQ].k[0]) * tsA0;
				dQA[1] = -(pQuatA1[iQ].k[1] + pQuatA0[iQ].k[1]) * tsA0;
				dQA[2] = -(pQuatA1[iQ].k[2] + pQuatA0[iQ].k[2]) * tsA0;
				dQA[3] = -(pQuatA1[iQ].k[3] + pQuatA0[iQ].k[3]) * tsA0;
			}
			else
			{
				dQA[0] = (pQuatA1[iQ].k[0] - pQuatA0[iQ].k[0]) * tsA0;
				dQA[1] = (pQuatA1[iQ].k[1] - pQuatA0[iQ].k[1]) * tsA0;
				dQA[2] = (pQuatA1[iQ].k[2] - pQuatA0[iQ].k[2]) * tsA0;
				dQA[3] = (pQuatA1[iQ].k[3] - pQuatA0[iQ].k[3]) * tsA0;
			}

			if (pQuatA2[iQ].DotProd(pQuatA1[iQ]) < T(0.0))
			{
				dQA[0] += -(pQuatA2[iQ].k[0] + pQuatA1[iQ].k[0]) * tsA1;
				dQA[1] += -(pQuatA2[iQ].k[1] + pQuatA1[iQ].k[1]) * tsA1;
				dQA[2] += -(pQuatA2[iQ].k[2] + pQuatA1[iQ].k[2]) * tsA1;
				dQA[3] += -(pQuatA2[iQ].k[3] + pQuatA1[iQ].k[3]) * tsA1;
			}
			else
			{
				dQA[0] += (pQuatA2[iQ].k[0] - pQuatA1[iQ].k[0]) * tsA1;
				dQA[1] += (pQuatA2[iQ].k[1] - pQuatA1[iQ].k[1]) * tsA1;
				dQA[2] += (pQuatA2[iQ].k[2] - pQuatA1[iQ].k[2]) * tsA1;
				dQA[3] += (pQuatA2[iQ].k[3] - pQuatA1[iQ].k[3]) * tsA1;
			}

			// dQuatB
			T dQB[4];
			if (pQuatB0[iQ].DotProd(pQuatB1[iQ]) < T(0.0))
			{
				dQB[0] = -(pQuatB1[iQ].k[0] + pQuatB0[iQ].k[0]) * tsB0;
				dQB[1] = -(pQuatB1[iQ].k[1] + pQuatB0[iQ].k[1]) * tsB0;
				dQB[2] = -(pQuatB1[iQ].k[2] + pQuatB0[iQ].k[2]) * tsB0;
				dQB[3] = -(pQuatB1[iQ].k[3] + pQuatB0[iQ].k[3]) * tsB0;
			}
			else
			{
				dQB[0] = (pQuatB1[iQ].k[0] - pQuatB0[iQ].k[0]) * tsB0;
				dQB[1] = (pQuatB1[iQ].k[1] - pQuatB0[iQ].k[1]) * tsB0;
				dQB[2] = (pQuatB1[iQ].k[2] - pQuatB0[iQ].k[2]) * tsB0;
				dQB[3] = (pQuatB1[iQ].k[3] - pQuatB0[iQ].k[3]) * tsB0;
			}

			if (pQuatB2[iQ].DotProd(pQuatB1[iQ]) < T(0.0))
			{
				dQB[0] += -(pQuatB2[iQ].k[0] + pQuatB1[iQ].k[0]) * tsB1;
				dQB[1] += -(pQuatB2[iQ].k[1] + pQuatB1[iQ].k[1]) * tsB1;
				dQB[2] += -(pQuatB2[iQ].k[2] + pQuatB1[iQ].k[2]) * tsB1;
				dQB[3] += -(pQuatB2[iQ].k[3] + pQuatB1[iQ].k[3]) * tsB1;
			}
			else
			{
				dQB[0] += (pQuatB2[iQ].k[0] - pQuatB1[iQ].k[0]) * tsB1;
				dQB[1] += (pQuatB2[iQ].k[1] - pQuatB1[iQ].k[1]) * tsB1;
				dQB[2] += (pQuatB2[iQ].k[2] - pQuatB1[iQ].k[2]) * tsB1;
				dQB[3] += (pQuatB2[iQ].k[3] - pQuatB1[iQ].k[3]) * tsB1;
			}

			if (pQuatA1[iQ].DotProd(pQuatB1[iQ]) < T(0.0f))
			{
				// Spline it, neg
				for(int i = 0; i < 4; i++)
				{
					T v0 = dQA[i];
					T v1 = -dQB[i];
					T p0 = pQuatA1[iQ].k[i];
					T p1 = -pQuatB1[iQ].k[i];
					T D = p0;
					T C = v0;
					T B = T(3.0f)*(p1 - D) - (T(2.0f)*v0) - v1;
					T A = -(T(2.0f) * B + v0 - v1) * ( T(1.0f) / T(3.0f) );
					_pDest[iQ].k[i] = A*tCube + B*tSqr + C*_tFrac + D;
				}
			}
			else
			{
				// Spline it
				for(int i = 0; i < 4; i++)
				{
					T v0 = dQA[i];
					T v1 = dQB[i];
					T p0 = pQuatA1[iQ].k[i];
					T p1 = pQuatB1[iQ].k[i];
					T D = p0;
					T C = v0;
					T B = T(3.0f)*(p1 - D) - (T(2.0f)*v0) - v1;
					T A = -(T(2.0f) * B + v0 - v1) * ( T(1.0f) / T(3.0f) );
					_pDest[iQ].k[i] = A*tCube + B*tSqr + C*_tFrac + D;
				}
			}

			_pDest[iQ].Normalize();
		}
	}

	void Multiply(const TQuaternion& _Quat2, TQuaternion& _QDest) const
	{
		T a0 = k[0];
		T a1 = k[1];
		T a2 = k[2];
		T a3 = k[3];
		T b0 = _Quat2.k[0];
		T b1 = _Quat2.k[1];
		T b2 = _Quat2.k[2];
		T b3 = _Quat2.k[3];
		_QDest.k[0] = a3*b0 + a0*b3 + a1*b2 - a2*b1;
		_QDest.k[1] = a3*b1 + a1*b3 + a2*b0 - a0*b2;
		_QDest.k[2] = a3*b2 + a2*b3 + a0*b1 - a1*b0;
		_QDest.k[3] = a3*b3 - a0*b0 - a1*b1 - a2*b2;
	}

	void Multiply(const TQuaternion& _Quat2)
	{
		T a0 = k[0];
		T a1 = k[1];
		T a2 = k[2];
		T a3 = k[3];
		T b0 = _Quat2.k[0];
		T b1 = _Quat2.k[1];
		T b2 = _Quat2.k[2];
		T b3 = _Quat2.k[3];
		k[0] = a3*b0 + a0*b3 + a1*b2 - a2*b1;
		k[1] = a3*b1 + a1*b3 + a2*b0 - a0*b2;
		k[2] = a3*b2 + a2*b3 + a0*b1 - a1*b0;
		k[3] = a3*b3 - a0*b0 - a1*b1 - a2*b2;
	}

	void operator*= (const TQuaternion& _Quat2)
	{
		Multiply( _Quat2 );
	}

	TQuaternion operator* (const TQuaternion& _Quat2) const
	{
		TQuaternion Temp;
		Multiply( _Quat2, Temp );
		return Temp;
	}

	T CreateAxisAngle(V& _v) const
	{
		M_ASSERT( ( ( k[3] >= -T(1.0f) ) || ( k[3] <= T(1.0f) ) ), "Quaternion needs normalizing!" );

		T LenSqr = Sqr(k[0]) + Sqr(k[1]) + Sqr(k[2]);
		if ( LenSqr > T(0.000001f) )
		{
			T invlen = M_InvSqrt(LenSqr);
			_v.k[0] = k[0]*invlen;
			_v.k[1] = k[1]*invlen;
			_v.k[2] = k[2]*invlen;
			return T(2.0f)*M_ACos(k[3]) / _PI2;	// Angle
		}
		else
		{
			// angle is 0 (mod 2*pi), so any axis will do
			_v.k[0] = T(1.0f);
			_v.k[1] = T(0.0f);
			_v.k[2] = T(0.0f);
			return 0;						// Angle
		}
	}

	void CreateMatrix3x3(M& _Mat) const
	{
		// 12 Mul + 12 Add/Sub;

		T xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;
		T s = T(2.0f);

		xs = k[0]*s;  ys = k[1]*s;  zs = k[2]*s;
		wx = k[3]*xs; wy = k[3]*ys; wz = k[3]*zs;
		xx = k[0]*xs; xy = k[0]*ys; xz = k[0]*zs;
		yy = k[1]*ys; yz = k[1]*zs; zz = k[2]*zs;

		_Mat.k[0][0] = (T(1.0f) - (yy+zz));
		_Mat.k[0][1] = (xy-wz);
		_Mat.k[0][2] = (xz+wy);

		_Mat.k[1][0] = (xy+wz);
		_Mat.k[1][1] = (T(1.0f) - (xx+zz));
		_Mat.k[1][2] = (yz-wx);

		_Mat.k[2][0] = (xz-wy);
		_Mat.k[2][1] = (yz+wx);
		_Mat.k[2][2] = (T(1.0f) - (xx+yy));
	}

//#ifndef DEFINE_MAT43_IS_MAT44
	// Create from 4x3 matrix
	void Create(const M43& _Mat)
	{
		// 7 Add
		// 1 Sqrt
		// 4 Mul
		// 1 Div
		T Trace = (T)0;
		for (int i = 0; i < 3; i++)
			Trace += _Mat.k[i][i];

		if (Trace > (T)0)
		{
			T s = M_Sqrt(Trace + (T)1.0f);
			k[3] = s*0.5f;
			s = 0.5f / s;

			for (int i = 0; i < 3; i++)
			{
				int j = (i == 2) ? 0 : (i + 1);
				int m = (j == 2) ? 0 : (j + 1);
				k[i] = (_Mat.k[m][j]-_Mat.k[j][m]) * s;
			}
		}
		else
		{
			int i = 0;
			if (_Mat.k[1][1] > _Mat.k[0][0]) i = 1;
			if (_Mat.k[2][2] > _Mat.k[i][i]) i = 2;
			int j = (i == 2) ? 0 : (i + 1);
			int m = (j == 2) ? 0 : (j + 1);

			T s = M_Sqrt((_Mat.k[i][i] - (_Mat.k[j][j] + _Mat.k[m][m])) + (T)1.0f);
			k[i] = s * 0.5f;
			s = 0.5f / s;
			k[3] = (_Mat.k[m][j] - _Mat.k[j][m])*s;
			k[j] = (_Mat.k[j][i] + _Mat.k[i][j])*s;
			k[m] = (_Mat.k[m][i] + _Mat.k[i][m])*s;
		}
	}

	void CreateMatrix3x3(M43& _Mat) const
	{
		// 12 Mul + 12 Add/Sub;

		T xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;
		T s = T(2.0);

		xs = k[0]*s;  ys = k[1]*s;  zs = k[2]*s;
		wx = k[3]*xs; wy = k[3]*ys; wz = k[3]*zs;
		xx = k[0]*xs; xy = k[0]*ys; xz = k[0]*zs;
		yy = k[1]*ys; yz = k[1]*zs; zz = k[2]*zs;

		_Mat.k[0][0] = (T(1.0f) - (yy+zz));
		_Mat.k[0][1] = (xy-wz);
		_Mat.k[0][2] = (xz+wy);

		_Mat.k[1][0] = (xy+wz);
		_Mat.k[1][1] = (T(1.0f) - (xx+zz));
		_Mat.k[1][2] = (yz-wx);

		_Mat.k[2][0] = (xz-wy);
		_Mat.k[2][1] = (yz+wx);
		_Mat.k[2][2] = (T(1.0f) - (xx+yy));
	}

	void CreateMatrix(M43& _Mat) const
	{
		// 4+6 = 10 Mul;
		CreateMatrix3x3(_Mat);
		_Mat.UnitNot3x3();
	}

	void GetMatrix(const M43& _Mat)
	{
		Create(_Mat);
	}

	void SetMatrix3x3(M43& _Mat) const
	{
		CreateMatrix3x3(_Mat);
	}

	void SetMatrix(M43& _Mat) const
	{
		// 4+6 = 10 Mul;
		SetMatrix3x3(_Mat);
		_Mat.UnitNot3x3();
	}
//#endif

	void CreateMatrix(M& _Mat) const
	{
		// 4+6 = 10 Mul;
		CreateMatrix3x3(_Mat);
		_Mat.UnitNot3x3();
	}

	// These are here just for backwards-compatibility.
	void GetMatrix(const M& _Mat)
	{
		Create(_Mat);
	}

	void SetMatrix3x3(M& _Mat) const
	{
		CreateMatrix3x3(_Mat);
	}

	void SetMatrix(M& _Mat) const
	{
		// 4+6 = 10 Mul;
		SetMatrix3x3(_Mat);
		_Mat.UnitNot3x3();
	}

	void Read(CCFile* _pFile)
	{
		for(int i = 0; i < 4; i++)
			_pFile->ReadLE(k[i]);
	}

	void Write(CCFile* _pFile) const
	{
		for(int i = 0; i < 4; i++)
			_pFile->WriteLE(k[i]);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(k[0]);
		::SwapLE(k[1]);
		::SwapLE(k[2]);
		::SwapLE(k[3]);
	}
#endif

	CStr GetString() const
	{
		return CStrF("%f, %f, %f, %f", k[0], k[1], k[2], k[3]);
	}
	
};

typedef TQuaternion<fp32> CQuatfp32;
typedef TQuaternion<fp64> CQuatfp64;

// -------------------------------------------------------------------
template<class T>
class TAxisRot
{
	typedef TMatrix43<T> M43;
	typedef TQuaternion<T> Q;
	typedef TVector3<T> V;
	typedef TMatrix4<T> M;

	typedef typename TMathTemplateProperties<T>::TVector4Intrinsic TIntrinsic;

public:
	union
	{
		TIntrinsic v;

		struct
		{
			TVector3Aggr<T> m_Axis;
			T m_Angle;
		};
	};

	/*
	// --------------------------------
	// Construction
	// --------------------------------
	TAxisRot()
	TAxisRot(const V& _Axis, T _Angle)					// Construct from axis-angle.
	TAxisRot(const Q& _Quat)							// Construct from quaternion.
	TAxisRot(const M& _Mat)								// Construct from matrix.
	void Unit()
	void Create(const Q& _Quat)							// Create from quaternion.
	void Create(const M& _Mat)							// Create from matrix. (via quaternion)
	void Create(T x, T y, T z)							// Create from euler angles. (via quaternions)
	void Create(const V& _Euler)						// Create from euler angles is vector-form. (via quaternions)

	// --------------------------------
	// Operations
	// --------------------------------
	void Normalize()									// Normalize axis.
	void CreateQuaternion(Q& _Quat) const				// Create quaternion from axis-angle.
	void CreateMatrix3x3(M& _Mat) const					// Create matrix from axis-angle.
	void CreateMatrix(M& _Mat) const					// Create matrix from axis-angle.
	void Multiply(const TAxisRot& _Rot)					// Multiply axis-angle rotations. (via quaternions)
	void Multiply(const Q& _Rot)						// Multiply axis-angle with quaterion rotation. (via quaternions)

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	CStr GetString() const								// Get string for output.

	*/

	M_FORCEINLINE void operator= (const TAxisRot<T>& _AxisRot)
	{
		v = _AxisRot.v;
	}

	TAxisRot()
	{
	}

	TAxisRot(const V& _Axis, T _Angle)
	{
		m_Axis = _Axis;
		m_Angle = _Angle;
	}

	TAxisRot(const Q& _Quat)
	{
		Create(_Quat);
	}

	TAxisRot(const M& _Mat)
	{
		Create(_Mat);
	}

	void Unit()
	{
		m_Axis.k[0] = 1;
		m_Axis.k[1] = 0;
		m_Axis.k[2] = 0;
		m_Angle = 0;
	}

	// Create from quaternion
	void Create(const Q& _Quat)
	{
		m_Angle = _Quat.CreateAxisAngle(m_Axis);
	}

	// Create from matrix
	void Create(const M& _Mat)
	{
		Q Quat;
		Quat.GetMatrix(_Mat);
		m_Angle = Quat.CreateAxisAngle(m_Axis);
	}

	// Create from euler angles
	void Create(T x, T y, T z)
	{
		// convert to quaternions
///		const T pitchAn = EulerAngles[0];
//		const T yawAn = EulerAngles[1];
//		const T rollAn = EulerAngles[2];

		V rollAx( 0, 0, T(1.0) );
		V pitchAx(T(1.0), 0, 0);
		V yawAx(0, T(1.0), 0);

//		Q roll(z, rollAx);
//		Q pitch(x, pitchAx);
//		Q yaw(y, yawAx);
		Q roll(rollAx, z);
		Q pitch(pitchAx, x);
		Q yaw(yawAx, y);

		// multiply the quaternions
		roll.Multiply(pitch);
		roll.Multiply(yaw);
		Create(roll);
	}

	// Create from euler angles
	void Create(const V& _Euler)
	{
		Create(_Euler.k[0], _Euler.k[1], _Euler.k[2]);
	}

	void Normalize()
	{
		m_Axis.Normalize();
	}

	void CreateQuaternion(Q& _Quat) const
	{
		_Quat.Create(m_Axis, m_Angle);
	}

	void CreateMatrix3x3(M& _Mat) const
	{
		// *this == Rotation-axis, Must be normalized!
		// _Angle = degrees / 360 (ie. 0..1)

		T x = m_Axis.k[0];
		T y = m_Axis.k[1];
		T z = m_Axis.k[2];

		T a = Sqr(x);
		T b = Sqr(y);
		T c = Sqr(z);
		T C = M_Cos(m_Angle*_PI2);
		T S = M_Sin(m_Angle*_PI2);
		T zS = z*S;
		T xS = x*S;
		T yS = y*S;
		T xz_ = x*z*(T(1.0)-C);
		T xy_ = x*y*(T(1.0)-C);
		T yz_ = y*z*(T(1.0)-C);

		_Mat.k[0][0] = a + C*(T(1.0)-a);
		_Mat.k[0][1] = xy_ - zS;
		_Mat.k[0][2] = yS + xz_;
		_Mat.k[1][0] = zS + xy_;
		_Mat.k[1][1] = b + C*(T(1.0)-b);
		_Mat.k[1][2] = yz_ - xS;
		_Mat.k[2][0] = xz_ - yS;
		_Mat.k[2][1] = xS + yz_;
		_Mat.k[2][2] = c + C*(T(1.0)-c);
	}
	void CreateMatrix(M& _Mat) const
	{
		CreateMatrix3x3(_Mat);
		_Mat.UnitNot3x3();
	}

	void Multiply(const TAxisRot& _Rot)
	{
		// Check if rotation-axis is parallell to the current axis
/*		T dotp = _Rot.m_Axis * m_Axis;
		if (Abs(dotp) > T(1.000)-T(0.0001))
		{
			// Yes, do simple angle-addition.
			if (dotp > T(0.0))
				m_Angle += _Rot.m_Angle;
			else
				m_Angle -= _Rot.m_Angle;
		}
		else*/
		{
			// No, convert to quaternions, multiply, and then convert back again.
			Q Quat;
			Q Rot;
			Q Dest;

			this->CreateQuaternion(Quat);
			_Rot.CreateQuaternion(Rot);
			Quat.Multiply(Rot, Dest);
			Create(Dest);
		}
	}

	void Multiply(const Q& _Rot)
	{
		Q Quat;
		Q Dest;

		CreateQuaternion(Quat);
		_Rot.Multiply(Quat, Dest);
		Create(Dest);
	}

	void Read(CCFile* _pFile)
	{
		m_Axis.Read(_pFile);
		_pFile->ReadLE(m_Angle);
	}

	void Write(CCFile* _pFile) const
	{
		m_Axis.Write(_pFile);
		_pFile->WriteLE(m_Angle);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		m_Axis.SwapLE();
		::SwapLE(m_Angle);
	}
#endif

	CStr GetString() const
	{
		return CStrF("Axis (%.3f,%.3f,%.3f), Angle %.3f", m_Axis.k[0], m_Axis.k[1], m_Axis.k[2], m_Angle);
	}


	TAxisRot(const M43& _Mat)
	{
		Create(_Mat);
	}

	void Create(const M43& _Mat)
	{
		Q Quat;
		Quat.GetMatrix(_Mat);
		m_Angle = Quat.CreateAxisAngle(m_Axis);
	}

	void CreateMatrix3x3(M43& _Mat) const
	{
		// *this == Rotation-axis, Must be normalized!
		// _Angle = degrees / 360 (ie. 0..1)

		T x = m_Axis.k[0];
		T y = m_Axis.k[1];
		T z = m_Axis.k[2];

		T a = Sqr(x);
		T b = Sqr(y);
		T c = Sqr(z);
		T C = M_Cos(m_Angle*_PI2);
		T S = M_Sin(m_Angle*_PI2);
		T zS = z*S;
		T xS = x*S;
		T yS = y*S;
		T xz_ = x*z*(T(1.0)-C);
		T xy_ = x*y*(T(1.0)-C);
		T yz_ = y*z*(T(1.0)-C);

		_Mat.k[0][0] = a + C*(T(1.0)-a);
		_Mat.k[0][1] = xy_ - zS;
		_Mat.k[0][2] = yS + xz_;
		_Mat.k[1][0] = zS + xy_;
		_Mat.k[1][1] = b + C*(T(1.0)-b);
		_Mat.k[1][2] = yz_ - xS;
		_Mat.k[2][0] = xz_ - yS;
		_Mat.k[2][1] = xS + yz_;
		_Mat.k[2][2] = c + C*(T(1.0)-c);
	}

	void CreateMatrix(M43& _Mat) const
	{
		CreateMatrix3x3(_Mat);
		_Mat.UnitNot3x3();
	}
};

typedef TAxisRot<fp32> CAxisRotfp32;
typedef TAxisRot<fp64> CAxisRotfp64;

// -------------------------------------------------------------------
template<class T> 
class TRect
{
	typedef TMatrix2<T> M;
	typedef TVector2Aggr<T> V;
	typedef typename TMathTemplateProperties<T>::TVector4Intrinsic TIntrinsic;

public:
	union
	{
		struct
		{
			V m_Min;
			V m_Max;
		};
		TIntrinsic v;
	};

	/*
	// --------------------------------
	// Miscellaneous
	// --------------------------------
	TRect()
	TRect(const T& _Min, const T& _Max)					// Construct from min and max vertices.
	bool IsInside(const TRect &_Box) const				// Box inside another box?
	bool IsCovering(const TRect &_Box) const				// Box covering another box?
	void GetCenter(T& _Center) const					// Get center point.

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	CStr GetString() const								// Get string for output.
	*/

	TRect()
	{
	}

	TRect(const V& _Min, const V& _Max)
	{
		m_Min = _Min;
		m_Max = _Max;
	}

	TRect(T _minx, T _miny, T _maxx, T _maxy)
	{
		m_Min[0] = _minx;
		m_Min[1] = _miny;
		m_Max[0] = _maxx;
		m_Max[1] = _maxy;
	}

	void Set(T _minx, T _miny, T _maxx, T _maxy)
	{
		m_Min[0] = _minx;
		m_Min[1] = _miny;
		m_Max[0] = _maxx;
		m_Max[1] = _maxy;
	}

	void Set(const V& _Min, const V& _Max)
	{
		m_Min = _Min;
		m_Max = _Max;
	}

	M_INLINE void And(const TRect& _Box)
	{
		m_Min.k[0]	= Max( m_Min.k[0], _Box.m_Min.k[0] );
		m_Min.k[1]	= Max( m_Min.k[1], _Box.m_Min.k[1] );

		m_Max.k[0]	= Min( m_Max.k[0], _Box.m_Max.k[0] );
		m_Max.k[1]	= Min( m_Max.k[1], _Box.m_Max.k[1] );
	}

	M_INLINE void Expand(const TRect& _Box)
	{
		m_Min.k[0]	= Min( m_Min.k[0], _Box.m_Min.k[0] );
		m_Min.k[1]	= Min( m_Min.k[1], _Box.m_Min.k[1] );

		m_Max.k[0]	= Max( m_Max.k[0], _Box.m_Max.k[0] );
		m_Max.k[1]	= Max( m_Max.k[1], _Box.m_Max.k[1] );
	}

	M_INLINE void Expand(const V& _v)
	{
		m_Min.k[0]	= Min( m_Min.k[0], _v.k[0] );
		m_Max.k[0]	= Max( m_Max.k[0], _v.k[0] );

		m_Min.k[1]	= Min( m_Min.k[1], _v.k[1] );
		m_Max.k[1]	= Max( m_Max.k[1], _v.k[1] );
	}

	void Grow(T _v)
	{
		m_Min -= _v;
		m_Max += _v;
	}

	void Grow(const V& _v)
	{
		if (_v[0] < (T)0) m_Min[0] += _v[0]; else m_Max[0] += _v[0];
		if (_v[1] < (T)0) m_Min[1] += _v[1]; else m_Max[1] += _v[1];
	}

	void Transform(const M& _Mat, TRect& _Dest)
	{
		// Transforms and expands box so that it still is a valid bounding box for the original space.
		V E;
		V C;
		m_Max.Lerp(m_Min, 0.5f, C);
		m_Max.Sub(C, E);

		for(int k = 0; k < 2; k++)
			_Dest.m_Max.k[k] = M_Fabs(_Mat.k[0][k]*E[0]) + M_Fabs(_Mat.k[1][k]*E[1]);

		_Dest.m_Min = -_Dest.m_Max;
		C *= _Mat;
		_Dest.m_Min += C;
		_Dest.m_Max += C;
	}

	bool IsInside(const TRect& _Box) const
	{
		// True if _Box intersects this box.
		if ((_Box.m_Max.k[0] <= m_Min.k[0]) ||
			(_Box.m_Max.k[1] <= m_Min.k[1])) return false;

		if ((_Box.m_Min.k[0] > m_Max.k[0]) ||
			(_Box.m_Min.k[1] > m_Max.k[1])) return false;

		return true;
	}

	bool IsCovering(const TRect& _Box) const
	{
		// True if this box fit inside _Box
		if ((_Box.m_Max.k[0] < m_Max.k[0]) ||
			(_Box.m_Max.k[1] < m_Max.k[1])) return false;

		if ((_Box.m_Min.k[0] > m_Min.k[0]) ||
			(_Box.m_Min.k[1] > m_Min.k[1])) return false;

		return true;
	}

	void GetCenter(V& _Center) const
	{
		_Center.k[0] = (m_Max.k[0] + m_Min.k[0]) * 0.5f;
		_Center.k[1] = (m_Max.k[1] + m_Min.k[1]) * 0.5f;
	}

	void GetVertices(V* _pV) const
	{
		register T a, b, c, d;
		a	= m_Min.k[0];
		b	= m_Min.k[1];
		c	= m_Max.k[0];
		d	= m_Max.k[1];
		
		_pV[0].k[0] = a;		_pV[0].k[1] = b;
		_pV[1].k[0] = c;		_pV[1].k[1] = b;
		_pV[2].k[0] = a;		_pV[2].k[1] = d;
		_pV[3].k[0] = c;		_pV[3].k[1] = d;
	}

	T GetMinSqrDistance(const V& _v) const
	{
		V dV;
		for(int i = 0; i < 2; i++)
		{
			if (_v.k[i] > m_Min.k[i])
			{
				if (_v.k[i] > m_Max.k[i])
					dV.k[i] = _v.k[i] - m_Max.k[i];
				else
					dV.k[i] = 0;
			}
			else
				dV.k[i] = _v.k[i] - m_Min.k[i];
		}

		return dV.LengthSqr();
	}

	T GetMaxSqrDistance(const V& _v) const
	{
		V dV;
		for(int i = 0; i < 2; i++)
		{
			T Mid = (m_Min.k[i] + m_Max.k[i]) * 0.5f;
			if (_v.k[i] > Mid)
				dV.k[i] = m_Min.k[i] - _v.k[i];
			else
				dV.k[i] = m_Max.k[i] - _v.k[i];
		}

		return dV.LengthSqr();
	}

	T GetMinDistance(const V& _v) const
	{
		return M_Sqrt(GetMinSqrDistance(_v));
	}

	T GetMaxDistance(const V& _v) const
	{
		return M_Sqrt(GetMaxSqrDistance(_v));
	}

	void Read(CCFile* _pFile)
	{
		m_Min.Read(_pFile);
		m_Max.Read(_pFile);
	}

	void Write(CCFile* _pFile) const
	{
		m_Min.Write(_pFile);
		m_Max.Write(_pFile);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		m_Min.SwapLE();
		m_Max.SwapLE();
	}
#endif

	CStr GetString() const
	{
		return CStrF("(%.3f,%.3f), (%.3f,%.3f)", m_Min.k[0], m_Min.k[1], m_Max.k[0], m_Max.k[1]);
	}
};

typedef TRect<uint16> CRect2Duint16;
typedef TRect<uint8> CRect2Duint8;
typedef TRect<int16> CRect2Dint16;
typedef TRect<int8> CRect2Dint8;
typedef TRect<int> CRect2Dint;
typedef TRect<fp32> CRect2Dfp32;
typedef TRect<fp64> CRect2Dfp64;

// -------------------------------------------------------------------
template<class T> 
class TBox
{
	typedef TMatrix4<T> M;
	typedef TMatrix43<T> M43;
	typedef TVector3<T> V;
	typedef TVector4<T> V4;
	typedef TPlane3<T> P;

public:
	V m_Min;
	V m_Max;

	/*
	// --------------------------------
	// Miscellaneous
	// --------------------------------
	TBox()
	TBox(const T& _Min, const T& _Max)					// Construct from min and max vertices.
	bool IsInside(const TBox &_Box) const				// Box inside another box?
	bool IsCovering(const TBox &_Box) const				// Box covering another box?
	void GetCenter(T& _Center) const					// Get center point.

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read from file.
	void Write(CCFile* _pFile) const					// Write to file.
	void SwapLE()
	CStr GetString() const								// Get string for output.
	*/

	TBox()
	{
	}

	TBox(const V& _Min, const V& _Max)
	{
		m_Min = _Min;
		m_Max = _Max;
	}

	M_INLINE void And(const TBox& _Box)
	{
		m_Min.k[0]	= Max( m_Min.k[0], _Box.m_Min.k[0] );
		m_Min.k[1]	= Max( m_Min.k[1], _Box.m_Min.k[1] );
		m_Min.k[2]	= Max( m_Min.k[2], _Box.m_Min.k[2] );

		m_Max.k[0]	= Min( m_Max.k[0], _Box.m_Max.k[0] );
		m_Max.k[1]	= Min( m_Max.k[1], _Box.m_Max.k[1] );
		m_Max.k[2]	= Min( m_Max.k[2], _Box.m_Max.k[2] );
	}

	M_INLINE void And(const TBox& _Box, TBox& _Dst) const
	{
		_Dst.m_Min.k[0]	= Max( m_Min.k[0], _Box.m_Min.k[0] );
		_Dst.m_Min.k[1]	= Max( m_Min.k[1], _Box.m_Min.k[1] );
		_Dst.m_Min.k[2]	= Max( m_Min.k[2], _Box.m_Min.k[2] );

		_Dst.m_Max.k[0]	= Min( m_Max.k[0], _Box.m_Max.k[0] );
		_Dst.m_Max.k[1]	= Min( m_Max.k[1], _Box.m_Max.k[1] );
		_Dst.m_Max.k[2]	= Min( m_Max.k[2], _Box.m_Max.k[2] );
	}

	M_INLINE void Expand(const TBox& _Box)
	{
		m_Min.k[0] = Min( m_Min.k[0], _Box.m_Min.k[0] );
		m_Min.k[1] = Min( m_Min.k[1], _Box.m_Min.k[1] );
		m_Min.k[2] = Min( m_Min.k[2], _Box.m_Min.k[2] );
		
		m_Max.k[0] = Max( m_Max.k[0], _Box.m_Max.k[0] );
		m_Max.k[1] = Max( m_Max.k[1], _Box.m_Max.k[1] );
		m_Max.k[2] = Max( m_Max.k[2], _Box.m_Max.k[2] );
	}

	M_INLINE void Expand(const V& _v)
	{
		m_Min.k[0]	= Min( m_Min.k[0], _v.k[0] );
		m_Max.k[0]	= Max( m_Max.k[0], _v.k[0] );

		m_Min.k[1]	= Min( m_Min.k[1], _v.k[1] );
		m_Max.k[1]	= Max( m_Max.k[1], _v.k[1] );

		m_Min.k[2]	= Min( m_Min.k[2], _v.k[2] );
		m_Max.k[2]	= Max( m_Max.k[2], _v.k[2] );
	}

	void Grow(T _v)
	{
		m_Min -= _v;
		m_Max += _v;
	}

	void Grow(const V& _v)
	{
		if (_v[0] < (T)0) m_Min[0] += _v[0]; else m_Max[0] += _v[0];
		if (_v[1] < (T)0) m_Min[1] += _v[1]; else m_Max[1] += _v[1];
		if (_v[2] < (T)0) m_Min[2] += _v[2]; else m_Max[2] += _v[2];
	}

	void Transform(const M& _Mat, TBox& _Dest) const
	{
		// Transforms and expands box so that it still is a valid bounding box for the original space.
		V E;
		V C;
		m_Max.Lerp(m_Min, T(0.5), C);
		m_Max.Sub(C, E);

		for(int k = 0; k < 3; k++)
			_Dest.m_Max.k[k] = M_Fabs(_Mat.k[0][k]*E[0]) + M_Fabs(_Mat.k[1][k]*E[1]) + M_Fabs(_Mat.k[2][k]*E[2]);

		_Dest.m_Min = -_Dest.m_Max;
		C *= _Mat;
		_Dest.m_Min += C;
		_Dest.m_Max += C;
	}

	bool IsInside(const TBox& _Box) const
	{
		// True if _Box intersects this box.
		if ((_Box.m_Max.k[0] < m_Min.k[0]) ||
			(_Box.m_Max.k[1] < m_Min.k[1]) ||
			(_Box.m_Max.k[2] < m_Min.k[2])) return false;

		if ((_Box.m_Min.k[0] > m_Max.k[0]) ||
			(_Box.m_Min.k[1] > m_Max.k[1]) ||
			(_Box.m_Min.k[2] > m_Max.k[2])) return false;

		return true;
	}

	bool IsCovering(const TBox& _Box) const
	{
		// True if this box fit inside _Box
		if ((_Box.m_Max.k[0] < m_Max.k[0]) ||
			(_Box.m_Max.k[1] < m_Max.k[1]) ||
			(_Box.m_Max.k[2] < m_Max.k[2])) return false;

		if ((_Box.m_Min.k[0] > m_Min.k[0]) ||
			(_Box.m_Min.k[1] > m_Min.k[1]) ||
			(_Box.m_Min.k[2] > m_Min.k[2])) return false;

		return true;
	}

	void GetCenter(V& _Center) const
	{
		_Center.k[0] = (m_Max.k[0] + m_Min.k[0]) / 2;
		_Center.k[1] = (m_Max.k[1] + m_Min.k[1]) / 2;
		_Center.k[2] = (m_Max.k[2] + m_Min.k[2]) / 2;
	}

	void GetVertices(V* _pV) const
	{
		const T a = m_Min.k[0];
		const T b = m_Min.k[1];
		const T c = m_Min.k[2];
		const T d = m_Max.k[0];
		const T e = m_Max.k[1];
		const T f = m_Max.k[2];

		_pV[0].k[0] = a;		_pV[0].k[1] = b;		_pV[0].k[2] = c;
		_pV[1].k[0] = d;		_pV[1].k[1] = b;		_pV[1].k[2] = c;
		_pV[2].k[0] = a;		_pV[2].k[1] = e;		_pV[2].k[2] = c;
		_pV[3].k[0] = d;		_pV[3].k[1] = e;		_pV[3].k[2] = c;
		_pV[4].k[0] = a;		_pV[4].k[1] = b;		_pV[4].k[2] = f;
		_pV[5].k[0] = d;		_pV[5].k[1] = b;		_pV[5].k[2] = f;
		_pV[6].k[0] = a;		_pV[6].k[1] = e;		_pV[6].k[2] = f;
		_pV[7].k[0] = d;		_pV[7].k[1] = e;		_pV[7].k[2] = f;
	}

	void GetVerticesV4(V4* _pV) const
	{
		const T a = m_Min.k[0];
		const T b = m_Min.k[1];
		const T c = m_Min.k[2];
		const T d = m_Max.k[0];
		const T e = m_Max.k[1];
		const T f = m_Max.k[2];

		_pV[0].k[0] = a;		_pV[0].k[1] = b;		_pV[0].k[2] = c;		_pV[0].k[3] = 1.0f;
		_pV[1].k[0] = d;		_pV[1].k[1] = b;		_pV[1].k[2] = c;		_pV[1].k[3] = 1.0f;
		_pV[2].k[0] = a;		_pV[2].k[1] = e;		_pV[2].k[2] = c;		_pV[2].k[3] = 1.0f;
		_pV[3].k[0] = d;		_pV[3].k[1] = e;		_pV[3].k[2] = c;		_pV[3].k[3] = 1.0f;
		_pV[4].k[0] = a;		_pV[4].k[1] = b;		_pV[4].k[2] = f;		_pV[4].k[3] = 1.0f;
		_pV[5].k[0] = d;		_pV[5].k[1] = b;		_pV[5].k[2] = f;		_pV[5].k[3] = 1.0f;
		_pV[6].k[0] = a;		_pV[6].k[1] = e;		_pV[6].k[2] = f;		_pV[6].k[3] = 1.0f;
		_pV[7].k[0] = d;		_pV[7].k[1] = e;		_pV[7].k[2] = f;		_pV[7].k[3] = 1.0f;
	}

	bool InVolume(const P* _pP, int _nP) const
	{
		V C;
		V E;
		m_Max.Lerp(m_Min, (T)0.5, C);
		m_Max.Sub(C, E);

		for(int ip = 0; ip < _nP; ip++)
		{
			const P* pP = &_pP[ip];
			T d = C*pP->n + pP->d - (M_Fabs(E[0]*pP->n[0]) + M_Fabs(E[1]*pP->n[1]) + M_Fabs(E[2]*pP->n[2]));
			if (d > (T)0.0) return false;
		}
		return true;
	}

	T GetMinSqrDistance(const V& _v) const
	{
		V dV;
		for(int i = 0; i < 3; i++)
		{
			if (_v.k[i] > m_Min.k[i])
			{
				if (_v.k[i] > m_Max.k[i])
					dV.k[i] = _v.k[i] - m_Max.k[i];
				else
					dV.k[i] = 0;
			}
			else
				dV.k[i] = _v.k[i] - m_Min.k[i];
		}

		return dV.LengthSqr();
	}

	T GetMaxSqrDistance(const V& _v) const
	{
		V dV;
		for(int i = 0; i < 3; i++)
		{
			T Mid = (m_Min.k[i] + m_Max.k[i]) * 0.5f;
			if (_v.k[i] > Mid)
				dV.k[i] = m_Min.k[i] - _v.k[i];
			else
				dV.k[i] = m_Max.k[i] - _v.k[i];
		}

		return dV.LengthSqr();
	}

	T GetMinDistance(const V& _v) const
	{
		return M_Sqrt(GetMinSqrDistance(_v));
	}

	T GetMaxDistance(const V& _v) const
	{
		return M_Sqrt(GetMaxSqrDistance(_v));
	}

	bool IntersectLine(const V& _p0, const V& _p1, V& _RetHitPos) const
	{
		V p0(_p0);
		V p1(_p1);

		if (p0.k[0] < p1.k[0])
		{
			if (p0.k[0] > m_Max.k[0]) return false;
			if (p1.k[0] < m_Min.k[0]) return false;

			if (p0.k[0] < m_Min.k[0])
			{
				T t = (m_Min.k[0] - p0.k[0]) / (p1.k[0] - p0.k[0]);
				p0.k[0] = m_Min.k[0];
				p0.k[1] += t*(p1.k[1] - p0.k[1]);
				p0.k[2] += t*(p1.k[2] - p0.k[2]);
			}
			if (p1.k[0] > m_Max.k[0])
			{
				T t = -(p1.k[0] - m_Max.k[0]) / (p1.k[0] - p0.k[0]);
				p1.k[0] = m_Max.k[0];
				p1.k[1] += t*(p1.k[1] - p0.k[1]);
				p1.k[2] += t*(p1.k[2] - p0.k[2]);
			}
		}
		else
		{
			if (p1.k[0] > m_Max.k[0]) return false;
			if (p0.k[0] < m_Min.k[0]) return false;

			if (p1.k[0] < m_Min.k[0])
			{
				T t = (m_Min.k[0] - p1.k[0]) / (p0.k[0] - p1.k[0]);
				p1.k[0] = m_Min.k[0];
				p1.k[1] += t*(p0.k[1] - p1.k[1]);
				p1.k[2] += t*(p0.k[2] - p1.k[2]);
			}
			if (p0.k[0] > m_Max.k[0])
			{
				T t = -(p0.k[0] - m_Max.k[0]) / (p0.k[0] - p1.k[0]);
				p0.k[0] = m_Max.k[0];
				p0.k[1] += t*(p0.k[1] - p1.k[1]);
				p0.k[2] += t*(p0.k[2] - p1.k[2]);
			}
		}

		if (p0.k[1] < p1.k[1])
		{
			if (p0.k[1] > m_Max.k[1]) return false;
			if (p1.k[1] < m_Min.k[1]) return false;

			if (p0.k[1] < m_Min.k[1])
			{
				T t = (m_Min.k[1] - p0.k[1]) / (p1.k[1] - p0.k[1]);
				p0.k[0] += t*(p1.k[0] - p0.k[0]);
				p0.k[1] = m_Min.k[1];
				p0.k[2] += t*(p1.k[2] - p0.k[2]);
			}
			if (p1.k[1] > m_Max.k[1])
			{
				T t = -(p1.k[1] - m_Max.k[1]) / (p1.k[1] - p0.k[1]);
				p1.k[0] += t*(p1.k[0] - p0.k[0]);
				p1.k[1] = m_Max.k[1];
				p1.k[2] += t*(p1.k[2] - p0.k[2]);
			}
		}
		else
		{
			if (p1.k[1] > m_Max.k[1]) return false;
			if (p0.k[1] < m_Min.k[1]) return false;

			if (p1.k[1] < m_Min.k[1])
			{
				T t = (m_Min.k[1] - p1.k[1]) / (p0.k[1] - p1.k[1]);
				p1.k[0] += t*(p0.k[0] - p1.k[0]);
				p1.k[1] = m_Min.k[1];
				p1.k[2] += t*(p0.k[2] - p1.k[2]);
			}
			if (p0.k[1] > m_Max.k[1])
			{
				T t = -(p0.k[1] - m_Max.k[1]) / (p0.k[1] - p1.k[1]);
				p0.k[0] += t*(p0.k[0] - p1.k[0]);
				p0.k[1] = m_Max.k[1];
				p0.k[2] += t*(p0.k[2] - p1.k[2]);
			}
		}

		if (p0.k[2] < p1.k[2])
		{
			if (p0.k[2] > m_Max.k[2]) return false;
			if (p1.k[2] < m_Min.k[2]) return false;

			if (p0.k[2] < m_Min.k[2])
			{
				T t = (m_Min.k[2] - p0.k[2]) / (p1.k[2] - p0.k[2]);
				p0.k[0] += t*(p1.k[0] - p0.k[0]);
				p0.k[1] += t*(p1.k[1] - p0.k[1]);
				p0.k[2] = m_Min.k[2];
			}
			if (p1.k[2] > m_Max.k[2])
			{
				T t = -(p1.k[2] - m_Max.k[2]) / (p1.k[2] - p0.k[2]);
				p1.k[0] += t*(p1.k[0] - p0.k[0]);
				p1.k[1] += t*(p1.k[1] - p0.k[1]);
				p1.k[2] = m_Max.k[2];
			}
		}
		else
		{
			if (p1.k[2] > m_Max.k[2]) return false;
			if (p0.k[2] < m_Min.k[2]) return false;

			if (p1.k[2] < m_Min.k[2])
			{
				T t = (m_Min.k[2] - p1.k[2]) / (p0.k[2] - p1.k[2]);
				p1.k[0] += t*(p0.k[0] - p1.k[0]);
				p1.k[1] += t*(p0.k[1] - p1.k[1]);
				p1.k[2] = m_Min.k[2];
			}
			if (p0.k[2] > m_Max.k[2])
			{
				T t = -(p0.k[2] - m_Max.k[2]) / (p0.k[2] - p1.k[2]);
				p0.k[0] += t*(p0.k[0] - p1.k[0]);
				p0.k[1] += t*(p0.k[1] - p1.k[1]);
				p0.k[2] = m_Max.k[2];
			}
		}

	//ConOut("After: " + p0.GetString() + p1.GetString());
		_RetHitPos = p0;

		return true;
	}

	void Read(CCFile* _pFile)
	{
		m_Min.Read(_pFile);
		m_Max.Read(_pFile);
	}

	void Write(CCFile* _pFile) const
	{
		m_Min.Write(_pFile);
		m_Max.Write(_pFile);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		m_Min.SwapLE();
		m_Max.SwapLE();
	}
#endif

	CStr GetString() const
	{
		return CStrF("(%.3f,%.3f,%.3f), (%.3f,%.3f,%.3f)", m_Min.k[0], m_Min.k[1], m_Min.k[2], m_Max.k[0], m_Max.k[1], m_Max.k[2]);
	}

	void Transform(const M43& _Mat, TBox& _Dest)
	{
		// Transforms and expands box so that it still is a valid bounding box for the original space.
		V E;
		V C;
		m_Max.Lerp(m_Min, T(0.5), C);
		m_Max.Sub(C, E);

		for(int k = 0; k < 3; k++)
			_Dest.m_Max.k[k] = M_Fabs(_Mat.k[0][k]*E[0]) + M_Fabs(_Mat.k[1][k]*E[1]) + M_Fabs(_Mat.k[2][k]*E[2]);

		_Dest.m_Min = -_Dest.m_Max;
		C *= _Mat;
		_Dest.m_Min += C;
		_Dest.m_Max += C;
	}
};

typedef TBox<int> CBox3Dint;
typedef TBox<fp32> CBox3Dfp32;
typedef TBox<fp64> CBox3Dfp64;

// -------------------------------------------------------------------
//  TOBB, Oriented bounding box
// -------------------------------------------------------------------
template<class T>
class TOBB
{
public:
	typedef TMatrix4<T> M;
	typedef TVector3<T> V;
	typedef TBox<T> B;
	typedef TPlane3<T> P;

	V m_A[3];	// Axis
	V m_E;		// Extents
	V m_C;		// Center

	/*
	// -------------------------------------------------------------------
	//  Construction
	// -------------------------------------------------------------------
	void Create(const V& _E, const V& _C)				// Construct from extents (_E) and center origin (_C)
	void Create(const V& _E, const V& _C, const M& _Mat)// Construct from extents (_E), center origin (_C), and orientation (_Mat)
	void Create(const B& _Box)							// Construct from AABB (Axis-aligned bounding box)
	TOBB()
	TOBB(const TOBB& _OBB)								// Copy constructor
	TOBB(const B& _Box)									// Copy constructor from AABB
	void SetDimensions(const V& _E)						// Set extents
	void SetPosition(const M& _Pos)						// Set position and orientation

	// -------------------------------------------------------------------
	//  Operations
	// -------------------------------------------------------------------
	void operator+= (const V& _v)						// Translate
	void operator*= (const M& _Mat)						// Transform
	void Transform(const M& _Mat, TOBB& _Dest) const	// Transform
	void GetVertices(V* _pV) const						// Get the 8 corner vertices for the box.
	void GetBoundBox(B& _Box) const						// Get bounding AABB.
	void GetLocalBox(B& _Box) const						// Get local space AABB
	void TransformToBoxSpace(const V& _v, V& _Dst) const// Transform vertex to box-space
	void TransformFromBoxSpace(const V& _v, V& _Dst) const// Transform vertex from box-space
	bool LocalPointInBox(const CVec3Dfp32 _p0) const		// Box-space point intersection
	bool LocalIntersectLine(const V& _p0, const V& _p1, V& _HitPos) const	// Box-space intersect line
	T LocalMinSqrDistance(const V& _v) const			// Calc minimum square distance from point to box in box-space.
	T LocalMaxSqrDistance(const V& _v) const			// Calc maximum square distance from point to box in box-space.
	T GetMinDistance(const P& _Plane) const				// Calc minimum distance from box to plane. Negative if box is partially or wholly on the back side of the plane.
	T GetMaxDistance(const P& _Plane) const				// Calc maximum distance from box to plane. Positive if box is partially or wholly on the front side of the plane.

	// --------------------------------
	// Helpers
	// --------------------------------
	void Read(CCFile* _pFile)							// Read OBB from file
	void Write(CCFile* _pFile) const					// Write OBB to file
	void SwapLE()
	CStr GetString() const								// Get formatted information string for printing.

	*/

	void Create(const V& _E, const V& _C)
	{
		m_E = _E;
		m_C = _C;
		m_A[0] = V(1,0,0);
		m_A[1] = V(0,1,0);
		m_A[2] = V(0,0,1);
	}

	void Create(const V& _E, const V& _C, const M& _Mat)
	{
		m_E = _E;
		m_C = _C;
		m_A[0] = V::GetMatrixRow(_Mat, 0);
		m_A[1] = V::GetMatrixRow(_Mat, 1);
		m_A[2] = V::GetMatrixRow(_Mat, 2);
	}

	void Create(const B& _Box)
	{
		_Box.m_Max.Lerp(_Box.m_Min, T(0.5), m_C);
		_Box.m_Max.Sub(m_C, m_E);
		m_A[0] = V(1,0,0);
		m_A[1] = V(0,1,0);
		m_A[2] = V(0,0,1);
	}

	TOBB()
	{
	}

	TOBB(const TOBB& _OBB)
	{
		m_A[0] = _OBB.m_A[0];
		m_A[1] = _OBB.m_A[1];
		m_A[2] = _OBB.m_A[2];
		m_E = _OBB.m_E;
		m_C = _OBB.m_C;
	}

	TOBB(const B& _Box)
	{
		Create(_Box);
	}

	void SetDimensions(const V& _E)
	{
		m_E = _E;
	}

	void SetPosition(const M& _Pos)
	{
		m_C = V::GetRow(_Pos, 3);
		m_A[0] = V::GetRow(_Pos, 0);
		m_A[1] = V::GetRow(_Pos, 1);
		m_A[2] = V::GetRow(_Pos, 2);
	}

	void operator+= (const V& _v)
	{
		m_C += _v;
	}

	void operator*= (const M& _Mat)
	{
		m_A[0].MultiplyMatrix3x3(_Mat);
		m_A[1].MultiplyMatrix3x3(_Mat);
		m_A[2].MultiplyMatrix3x3(_Mat);
		m_C *= _Mat;
	}

	void Transform(const M& _Mat, TOBB& _Dest) const
	{
		_Dest.m_A[0] = m_A[0];
		_Dest.m_A[0].MultiplyMatrix3x3(_Mat);
		_Dest.m_A[1] = m_A[1];
		_Dest.m_A[1].MultiplyMatrix3x3(_Mat);
		_Dest.m_A[2] = m_A[2];
		_Dest.m_A[2].MultiplyMatrix3x3(_Mat);
		_Dest.m_E = m_E;
		_Dest.m_C = m_C;
		_Dest.m_C *= _Mat;
	}

	void GetVertices(V* _pV) const
	{
		for(int k = 0; k < 3; k++)
		{
			_pV[0][k] = m_C[k] - (m_E[0]*m_A[0][k]) - (m_E[1]*m_A[1][k]) - (m_E[2]*m_A[2][k]);
			_pV[1][k] = m_C[k] + (m_E[0]*m_A[0][k]) - (m_E[1]*m_A[1][k]) - (m_E[2]*m_A[2][k]);
			_pV[2][k] = m_C[k] + (m_E[0]*m_A[0][k]) + (m_E[1]*m_A[1][k]) - (m_E[2]*m_A[2][k]);
			_pV[3][k] = m_C[k] - (m_E[0]*m_A[0][k]) + (m_E[1]*m_A[1][k]) - (m_E[2]*m_A[2][k]);
			_pV[4][k] = m_C[k] - (m_E[0]*m_A[0][k]) - (m_E[1]*m_A[1][k]) + (m_E[2]*m_A[2][k]);
			_pV[5][k] = m_C[k] + (m_E[0]*m_A[0][k]) - (m_E[1]*m_A[1][k]) + (m_E[2]*m_A[2][k]);
			_pV[6][k] = m_C[k] + (m_E[0]*m_A[0][k]) + (m_E[1]*m_A[1][k]) + (m_E[2]*m_A[2][k]);
			_pV[7][k] = m_C[k] - (m_E[0]*m_A[0][k]) + (m_E[1]*m_A[1][k]) + (m_E[2]*m_A[2][k]);
		}
	}

	void GetBoundBox(B& _Box) const
	{
		for(int k = 0; k < 3; k++)
			_Box.m_Max.k[k] = M_Fabs(m_A[0][k]*m_E[0]) + M_Fabs(m_A[1][k]*m_E[1]) + M_Fabs(m_A[2][k]*m_E[2]);

		_Box.m_Min = -_Box.m_Max;
		_Box.m_Min += m_C;
		_Box.m_Max += m_C;
	}

	void GetLocalBox(B& _Box) const
	{
		_Box.m_Min = -m_E;
		_Box.m_Max = m_E;
	}

	void TransformToBoxSpace(const V& _v, V& _Dst) const
	{
		T vx = _v[0] - m_C[0];
		T vy = _v[1] - m_C[1];
		T vz = _v[2] - m_C[2];
		T dstx = vx*m_A[0][0] + vy*m_A[0][1] + vz*m_A[0][2];
		T dsty = vx*m_A[1][0] + vy*m_A[1][1] + vz*m_A[1][2];
		T dstz = vx*m_A[2][0] + vy*m_A[2][1] + vz*m_A[2][2];
		_Dst.k[0] = dstx;
		_Dst.k[1] = dsty;
		_Dst.k[2] = dstz;
	}

	void TransformFromBoxSpace(const V& _v, V& _Dst) const
	{
		T dstx = _v.k[0]*m_A[0][0] + _v.k[1]*m_A[1][0] + _v.k[2]*m_A[2][0] + m_C[0];
		T dsty = _v.k[0]*m_A[0][1] + _v.k[1]*m_A[1][1] + _v.k[2]*m_A[2][1] + m_C[1];
		T dstz = _v.k[0]*m_A[0][2] + _v.k[1]*m_A[1][2] + _v.k[2]*m_A[2][2] + m_C[2];
		_Dst.k[0] = dstx;
		_Dst.k[1] = dsty;
		_Dst.k[2] = dstz;
	}

	void TransformToBoxSpace_Vector(const V& _v, V& _Dst) const
	{
		T vx = _v[0];
		T vy = _v[1];
		T vz = _v[2];
		T dstx = vx*m_A[0][0] + vy*m_A[0][1] + vz*m_A[0][2];
		T dsty = vx*m_A[1][0] + vy*m_A[1][1] + vz*m_A[1][2];
		T dstz = vx*m_A[2][0] + vy*m_A[2][1] + vz*m_A[2][2];
		_Dst.k[0] = dstx;
		_Dst.k[1] = dsty;
		_Dst.k[2] = dstz;
	}

	void TransformFromBoxSpace_Vector(const V& _v, V& _Dst) const
	{
		T dstx = _v.k[0]*m_A[0][0] + _v.k[1]*m_A[1][0] + _v.k[2]*m_A[2][0];
		T dsty = _v.k[0]*m_A[0][1] + _v.k[1]*m_A[1][1] + _v.k[2]*m_A[2][1];
		T dstz = _v.k[0]*m_A[0][2] + _v.k[1]*m_A[1][2] + _v.k[2]*m_A[2][2];
		_Dst.k[0] = dstx;
		_Dst.k[1] = dsty;
		_Dst.k[2] = dstz;
	}

	bool LocalPointInBox(const V& _p0) const
	{
		for(int i = 0; i < 3; i++)
		{
			if (_p0.k[i] > m_E[i]) return false;
			if (_p0.k[i] < -m_E[i]) return false;
		}
		return true;
	}

	bool LocalIntersectLine(const V& _p0, const V& _p1, V& _HitPos) const
	{
		// _p0, _p1 == Line in box-coordinates.
		// Internal processing is in the box' coordinate-space.

		V p0(_p0);
		V p1(_p1);

		if (p0.k[0] < p1.k[0])
		{
			if (p0.k[0] > m_E.k[0]) return false;
			if (p1.k[0] < -m_E.k[0]) return false;

			if (p0.k[0] < -m_E.k[0])
			{
				T t = (-m_E.k[0] - p0.k[0]) / (p1.k[0] - p0.k[0]);
				p0.k[0] = -m_E.k[0];
				p0.k[1] += t*(p1.k[1] - p0.k[1]);
				p0.k[2] += t*(p1.k[2] - p0.k[2]);
			}
			if (p1.k[0] > m_E.k[0])
			{
				T t = -(p1.k[0] - m_E.k[0]) / (p1.k[0] - p0.k[0]);
				p1.k[0] = m_E.k[0];
				p1.k[1] += t*(p1.k[1] - p0.k[1]);
				p1.k[2] += t*(p1.k[2] - p0.k[2]);
			}
		}
		else
		{
			if (p1.k[0] > m_E.k[0]) return false;
			if (p0.k[0] < -m_E.k[0]) return false;

			if (p1.k[0] < -m_E.k[0])
			{
				T t = (-m_E.k[0] - p1.k[0]) / (p0.k[0] - p1.k[0]);
				p1.k[0] = -m_E.k[0];
				p1.k[1] += t*(p0.k[1] - p1.k[1]);
				p1.k[2] += t*(p0.k[2] - p1.k[2]);
			}
			if (p0.k[0] > m_E.k[0])
			{
				T t = -(p0.k[0] - m_E.k[0]) / (p0.k[0] - p1.k[0]);
				p0.k[0] = m_E.k[0];
				p0.k[1] += t*(p0.k[1] - p1.k[1]);
				p0.k[2] += t*(p0.k[2] - p1.k[2]);
			}
		}

		if (p0.k[1] < p1.k[1])
		{
			if (p0.k[1] > m_E.k[1]) return false;
			if (p1.k[1] < -m_E.k[1]) return false;

			if (p0.k[1] < -m_E.k[1])
			{
				T t = (-m_E.k[1] - p0.k[1]) / (p1.k[1] - p0.k[1]);
				p0.k[0] += t*(p1.k[0] - p0.k[0]);
				p0.k[1] = -m_E.k[1];
				p0.k[2] += t*(p1.k[2] - p0.k[2]);
			}
			if (p1.k[1] > m_E.k[1])
			{
				T t = -(p1.k[1] - m_E.k[1]) / (p1.k[1] - p0.k[1]);
				p1.k[0] += t*(p1.k[0] - p0.k[0]);
				p1.k[1] = m_E.k[1];
				p1.k[2] += t*(p1.k[2] - p0.k[2]);
			}
		}
		else
		{
			if (p1.k[1] > m_E.k[1]) return false;
			if (p0.k[1] < -m_E.k[1]) return false;

			if (p1.k[1] < -m_E.k[1])
			{
				T t = (-m_E.k[1] - p1.k[1]) / (p0.k[1] - p1.k[1]);
				p1.k[0] += t*(p0.k[0] - p1.k[0]);
				p1.k[1] = -m_E.k[1];
				p1.k[2] += t*(p0.k[2] - p1.k[2]);
			}
			if (p0.k[1] > m_E.k[1])
			{
				T t = -(p0.k[1] - m_E.k[1]) / (p0.k[1] - p1.k[1]);
				p0.k[0] += t*(p0.k[0] - p1.k[0]);
				p0.k[1] = m_E.k[1];
				p0.k[2] += t*(p0.k[2] - p1.k[2]);
			}
		}

		if (p0.k[2] < p1.k[2])
		{
			if (p0.k[2] > m_E.k[2]) return false;
			if (p1.k[2] < -m_E.k[2]) return false;

			if (p0.k[2] < -m_E.k[2])
			{
				T t = (-m_E.k[2] - p0.k[2]) / (p1.k[2] - p0.k[2]);
				p0.k[0] += t*(p1.k[0] - p0.k[0]);
				p0.k[1] += t*(p1.k[1] - p0.k[1]);
				p0.k[2] = -m_E.k[2];
			}
			if (p1.k[2] > m_E.k[2])
			{
				T t = -(p1.k[2] - m_E.k[2]) / (p1.k[2] - p0.k[2]);
				p1.k[0] += t*(p1.k[0] - p0.k[0]);
				p1.k[1] += t*(p1.k[1] - p0.k[1]);
				p1.k[2] = m_E.k[2];
			}
		}
		else
		{
			if (p1.k[2] > m_E.k[2]) return false;
			if (p0.k[2] < -m_E.k[2]) return false;

			if (p1.k[2] < -m_E.k[2])
			{
				T t = (-m_E.k[2] - p1.k[2]) / (p0.k[2] - p1.k[2]);
				p1.k[0] += t*(p0.k[0] - p1.k[0]);
				p1.k[1] += t*(p0.k[1] - p1.k[1]);
				p1.k[2] = -m_E.k[2];
			}
			if (p0.k[2] > m_E.k[2])
			{
				T t = -(p0.k[2] - m_E.k[2]) / (p0.k[2] - p1.k[2]);
				p0.k[0] += t*(p0.k[0] - p1.k[0]);
				p0.k[1] += t*(p0.k[1] - p1.k[1]);
				p0.k[2] = m_E.k[2];
			}
		}

		_HitPos = p0;
		return true;
	}

	T LocalMinSqrDistance(const V& _v) const
	{
		// _v is in box-space
		V dV;
		for(int i = 0; i < 3; i++)
		{
			if (_v.k[i] > -m_E.k[i])
			{
				if (_v.k[i] > m_E.k[i])
					dV.k[i] = _v.k[i] - m_E.k[i];
				else
					dV.k[i] = 0;
			}
			else
				dV.k[i] = _v.k[i]-(m_E.k[i]);
		}

		return dV.LengthSqr();
	}

	T LocalMaxSqrDistance(const V& _v) const
	{
		// _v is in box-space
		V dV;
		for(int i = 0; i < 3; i++)
		{
			if (_v.k[i] > 0)
				dV.k[i] = -m_E.k[i] - _v.k[i];
			else
				dV.k[i] = m_E.k[i] - _v.k[i];
		}

		return dV.LengthSqr();
	}

	// ---------------------------------------------------------------------------
	T GetMinDistance(const P& _Plane) const
	{
		T d = _Plane.Distance(m_C);
		for(int k = 0; k < 3; k++)
			d -= M_Fabs(_Plane.n * m_A[k]) * m_E[k];
		return d;
	}

	T GetMaxDistance(const P& _Plane) const
	{
		T d = _Plane.Distance(m_C);
		for(int k = 0; k < 3; k++)
			d += M_Fabs(_Plane.n * m_A[k]) * m_E[k];
		return d;
	}

	// ---------------------------------------------------------------------------
	void Read(CCFile* _pFile)
	{
		m_A[0].Read(_pFile);
		m_A[1].Read(_pFile);
		m_A[2].Read(_pFile);
		m_E.Read(_pFile);
		m_C.Read(_pFile);
	}

	void Write(CCFile* _pFile) const
	{
		m_A[0].Write(_pFile);
		m_A[1].Write(_pFile);
		m_A[2].Write(_pFile);
		m_E.Write(_pFile);
		m_C.Write(_pFile);
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		m_A[0].SwapLE();
		m_A[1].SwapLE();
		m_A[2].SwapLE();
		m_E.SwapLE();
		m_C.SwapLE();
	}
#endif

	CStr GetString() const
	{
		return CStrF("C:(%.3f,%.3f,%.3f) E:(%.3f,%.3f,%.3f) A0:(%.3f,%.3f,%.3f) A1:(%.3f,%.3f,%.3f) A2:(%.3f,%.3f,%.3f)", 
			m_C[0], m_C[1], m_C[2], 
			m_E[0], m_E[1], m_E[2], 
			m_A[0][0], m_A[0][1], m_A[0][2], 
			m_A[1][0], m_A[1][1], m_A[1][2], 
			m_A[2][0], m_A[2][1], m_A[2][2]);
	}
};

typedef TOBB<fp32> COBBfp32;
typedef TOBB<fp64> COBBfp64;

// -------------------------------------------------------------------
//  TCapsule
// -------------------------------------------------------------------
#define CAPSULE_VERSION 1
enum eCapsuleFlags {
	CAPSULE_INVERTED=1
};

template<class T>
class TCapsule
{
public:
	typedef TMatrix4<T> M;
	typedef typename TMathTemplateProperties<T>::TVector4Intrinsic TIntrinsic;
	union
	{
		struct  
		{
			TVector3Aggr<T> m_Point1;
			T m_w1;
		};
		TIntrinsic m_Pointv1;
	};
	union
	{
		struct  
		{
			TVector3Aggr<T> m_Point2;
			T m_w2;
		};
		TIntrinsic m_Pointv2;
	};
	union 
	{	
		struct  
		{
			T m_Radius;
			T m_InvSqrDistance; // 1 / Sqr(m_Point2 - m_Point1)
			int32 m_UserValue;
			uint32 m_Flags;
		};
		TIntrinsic m_Misc;
	};

	TCapsule() {}

	TCapsule(const TCapsule<T>& _capsule)
	{
		m_Pointv1=_capsule.m_Pointv1;
		m_Pointv2=_capsule.m_Pointv2;
		m_Misc=_capsule.m_Misc;
	}

	bool IsInverted() const { return (m_Flags & CAPSULE_INVERTED)!=0; }

	void Read(CCFile* _pFile,int _Version)
	{
		_pFile->ReadLE(m_Radius);
		m_Point1.Read(_pFile);
		m_Point2.Read(_pFile);
		_pFile->ReadLE(m_UserValue);
		if (_Version==0)
			m_Flags=0;
		else if (_Version==1)
			_pFile->ReadLE(m_Flags);
		else
		{
			M_ASSERT(0,"Unknown capsule version.");
		}
		UpdateInternal();
	}

	void Write(CCFile* _pFile) const
	{
		_pFile->WriteLE(m_Radius);
		m_Point1.Write(_pFile);
		m_Point2.Write(_pFile);
		_pFile->WriteLE(m_UserValue);
		_pFile->WriteLE(m_Flags);
	}

	void UpdateInternal()
	{
		TVector3<T> ab = m_Point2 - m_Point1;
		m_InvSqrDistance = 1.0f / (ab * ab);
	}
};

// -------------------------------------------------------------------
//  TIndexedEdge
// -------------------------------------------------------------------
typedef TCapsule<fp32> CCapsulefp32;
typedef TCapsule<fp64> CCapsulefp64;

template<class T>
class TIndexedEdge
{
public:
	T m_liV[2];

	TIndexedEdge() {};

	TIndexedEdge(int _iv0, int _iv1)
	{
		m_liV[0] = _iv0;
		m_liV[1] = _iv1;
	}

	void Read(CCFile* _pFile)
	{
		_pFile->ReadLE(m_liV, 2);
	}
	void Write(CCFile* _pFile) const
	{
		_pFile->WriteLE(m_liV, 2);
	}
};

typedef TIndexedEdge<uint16> CIndexedEdge16;
typedef TIndexedEdge<uint32> CIndexedEdge32;

// -------------------------------------------------------------------
//  TIndexedTriangle
// -------------------------------------------------------------------
template<class T>
class TIndexedTriangle
{
public:	
	T m_liV[3];

	TIndexedTriangle()
	{
	}

	TIndexedTriangle(int _iV0, int _iV1, int _iV2)
	{
		m_liV[0] = _iV0;
		m_liV[1] = _iV1;
		m_liV[2] = _iV2;
	}

	TIndexedTriangle(const T* _piV)
	{
		m_liV[0] = _piV[0];
		m_liV[1] = _piV[1];
		m_liV[2] = _piV[2];
	}

	void Read(CCFile* _pFile)
	{
		_pFile->ReadLE(m_liV, 3);
	}
	void Write(CCFile* _pFile) const
	{
		_pFile->WriteLE(m_liV, 3);
	}
};

typedef TIndexedTriangle<uint16> CIndexedTriangle16;
typedef TIndexedTriangle<uint32> CIndexedTriangle32;


template <class T>
void M_INLINE TMatrix4<T>::RecreateMatrix(int _Priority0, int _Priority1)
{
	// The matrix normalizes the _Priority0 row, then uses the _Priority1 row to create an orthogonal matrix.
	// The information in the third row (not _Priority0 and not _Priority1) is completely ignored.

	int Missing = 3 - (_Priority0 + _Priority1);
	if(_Priority0 < 0 || _Priority1 > 2)
		Error_static("RecreateMatrix", "Invalid parameters");

	int iType = _Priority0  - _Priority1;
	if(iType == -1 || iType == 2)
	{
		GetRow(_Priority0).Normalize();
		GetRow(Missing) = -(GetRow(_Priority1) / GetRow(_Priority0)).Normalize();
		GetRow(_Priority1) = GetRow(Missing) / GetRow(_Priority0);
	}
	else
	{
		GetRow(_Priority0).Normalize();
		GetRow(Missing) = (GetRow(_Priority1) / GetRow(_Priority0)).Normalize();
		GetRow(_Priority1) = GetRow(_Priority0) / GetRow(Missing);
	}
}


template <class T>
void M_INLINE TMatrix43<T>::RecreateMatrix(int _Priority0, int _Priority1)
{
	// The matrix normalizes the _Priority0 row, then uses the _Priority1 row to create an orthogonal matrix.
	// The information in the third row (not _Priority0 and not _Priority1) is completely ignored.

	int Missing = 3 - (_Priority0 + _Priority1);
	if(_Priority0 < 0 || _Priority1 > 2)
		Error_static("RecreateMatrix", "Invalid parameters");

	int iType = _Priority0  - _Priority1;
	if(iType == -1 || iType == 2)
	{
		GetRow(_Priority0).Normalize();
		GetRow(Missing) = -(GetRow(_Priority1) / GetRow(_Priority0)).Normalize();
		GetRow(_Priority1) = GetRow(Missing) / GetRow(_Priority0);
	}
	else
	{
		GetRow(_Priority0).Normalize();
		GetRow(Missing) = (GetRow(_Priority1) / GetRow(_Priority0)).Normalize();
		GetRow(_Priority1) = GetRow(_Priority0) / GetRow(Missing);
	}
}

// -------------------------------------------------------------------
//  Some pretty useful functions...
// -------------------------------------------------------------------
int MCCDLLEXPORT CutFence(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, int& _bClip);
#ifndef CPU_SOFTWARE_FP64
int MCCDLLEXPORT CutFence(CVec3Dfp64* _pVerts, int _nv, const CPlane3Dfp64* _pPlanes, int _np, int& _bClip);
#endif
// Use (_bInvertPlanes = true) to yield the same result as CutFence.
int MCCDLLEXPORT CutFence(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, 
	bool _bInvertPlanes, CVec2Dfp32* _pTVerts1 = NULL, CVec2Dfp32* _pTVerts2 = NULL, CVec2Dfp32* _pTVerts3 = NULL);

// -------------------------------------------------------------------

// Added by Mondelore.
//void MCCDLLEXPORT MatrixLerp(const CMat4Dfp32 &_M0, const CMat4Dfp32 &_M1, fp32 _fpTime, CMat4Dfp32 &_Rese);
void MCCDLLEXPORT MatrixLerp(const CMat4Dfp32 &_M0o, const CMat4Dfp32 &_M1o, fp32 _fpTime, CMat4Dfp32 &_Res);
void MCCDLLEXPORT MatrixReflect(const CMat4Dfp32 &_Source, const CMat4Dfp32 &_Pivot, CMat4Dfp32 &_Dest);
void MCCDLLEXPORT MatrixSpline(const CMat4Dfp32 &_M0o, const CMat4Dfp32 &_M1o, const CMat4Dfp32 &_M2o, const CMat4Dfp32 &_M3o, fp32 _TimeFraction, CMat4Dfp32 &_Res);

// Added by Anton. 4x3 versions of the above..
void MCCDLLEXPORT MatrixLerp(const CMat43fp32 &_M0o, const CMat43fp32 &_M1o, fp32 _fpTime, CMat43fp32 &_Res);
void MCCDLLEXPORT MatrixReflect(const CMat43fp32 &_Source, const CMat43fp32 &_Pivot, CMat43fp32 &_Dest);
void MCCDLLEXPORT MatrixSpline(const CMat43fp32 &_M0o, const CMat43fp32 &_M1o, const CMat43fp32 &_M2o, const CMat43fp32 &_M3o, fp32 _TimeFraction, CMat43fp32 &_Res);

template <typename t_CType0, typename t_CType1>
static M_INLINE bool AlmostEqual(const t_CType0& _Number0, const t_CType0& _Number1, t_CType1 _Margin)
{
	CGenerateCompileTimeError<int>::GenerateError(); // Implement type
	return 0;
} 

template <> MCCDLLEXPORT bool TBox<fp32>::IntersectLine(const V& _p0, const V& _p1, V& _RetHitPos) const;

// -------------------------------------------------------------------

template <>
static M_INLINE bool AlmostEqual<fp32, fp32>(const fp32& _Number0, const fp32& _Number1, fp32 _Margin)
{
	if (M_Fabs(_Number0 - _Number1) < _Margin)
		return true;
	return false;
}

template <>
static M_INLINE bool AlmostEqual<fp32, fp64>(const fp32& _Number0, const fp32& _Number1, fp64 _Margin)
{
	if (M_Fabs(_Number0 - _Number1) < _Margin)
		return true;
	return false;
}

template <>
static M_INLINE bool AlmostEqual<fp64, fp64>(const fp64& _Number0, const fp64& _Number1, fp64 _Margin)
{
	if (M_Fabs(_Number0 - _Number1) < _Margin)
		return true;
	return false;
}

template <>
static M_INLINE bool AlmostEqual<fp64, fp32>(const fp64& _Number0, const fp64& _Number1, fp32 _Margin)
{
	if (M_Fabs(_Number0 - _Number1) < _Margin)
		return true;
	return false;
}

// -------------------------------------------------------------------

class M_ALIGN(8) CScissorRect
{
protected:
	uint32 m_Min;		// Low 16 is X, High 16 is Y
	uint32 m_Max;		// Low 16 is X, High 16 is Y
public:
	typedef TVector2<uint16> V;

	M_INLINE void SetRect(uint32 _MinX, uint32 _MinY, uint32 _MaxX, uint32 _MaxY)
	{
		m_Min = _MinX | (_MinY << 16);
		m_Max = _MaxX | (_MaxY << 16);
	}

	M_INLINE void SetRect(uint32 _Min, uint32 _Max)
	{
		m_Min = _Min | (_Min << 16);
		m_Max = _Max | (_Max << 16);
	}

	M_INLINE void GetRect(uint32& _MinX, uint32& _MinY, uint32& _MaxX, uint32& _MaxY) const
	{
		_MinX = m_Min & 0xffff;
		_MinY = (m_Min >> 16) & 0xffff;
		_MaxX = m_Max & 0xffff;
		_MaxY = (m_Max >> 16) & 0xffff;
	}

	M_INLINE void And(const CScissorRect& _Other)
	{
		uint32 MinX = Max(m_Min & 0xffff, _Other.m_Min & 0xffff);
		uint32 MinY = Max((m_Min >> 16) & 0xffff, (_Other.m_Min >> 16) & 0xffff);
		uint32 MaxX = Min(m_Max & 0xffff, _Other.m_Max & 0xffff);
		uint32 MaxY = Min((m_Max >> 16) & 0xffff, (_Other.m_Max >> 16) & 0xffff);
		m_Min = MinX | (MinY << 16);
		m_Max = MaxX | (MaxY << 16);
	}

	M_INLINE void Expand(const CScissorRect& _Other)
	{
		uint32 MinX = Min(m_Min & 0xffff, _Other.m_Min & 0xffff);
		uint32 MinY = Min((m_Min >> 16) & 0xffff, (_Other.m_Min >> 16) & 0xffff);
		uint32 MaxX = Max(m_Max & 0xffff, _Other.m_Max & 0xffff);
		uint32 MaxY = Max((m_Max >> 16) & 0xffff, (_Other.m_Max >> 16) & 0xffff);
		m_Min = MinX | (MinY << 16);
		m_Max = MaxX | (MaxY << 16);
	}

	M_INLINE void Expand(const CRect2Duint16& _Other)
	{
		uint32 MinX = Min(m_Min & 0xffff, (uint32)_Other.m_Min[0]);
		uint32 MinY = Min((m_Min >> 16) & 0xffff, (uint32)_Other.m_Min[1]);
		uint32 MaxX = Max(m_Max & 0xffff, (uint32)_Other.m_Max[0]);
		uint32 MaxY = Max((m_Max >> 16) & 0xffff, (uint32)_Other.m_Max[1]);
		m_Min = MinX | (MinY << 16);
		m_Max = MaxX | (MaxY << 16);
	}

	M_INLINE void Expand(const V& _v)
	{
		uint32 MinX = Min(m_Min & 0xffff, (uint32)_v.k[0]);
		uint32 MinY = Min((m_Min >> 16) & 0xffff, (uint32)_v.k[1]);
		uint32 MaxX = Max(m_Max & 0xffff, (uint32)_v.k[0]);
		uint32 MaxY = Max((m_Max >> 16) & 0xffff, (uint32)_v.k[1]);
		m_Min = MinX | (MinY << 16);
		m_Max = MaxX | (MaxY << 16);
	}

	M_INLINE bool IsEqual(const CScissorRect& _Other) const
	{
		return (m_Min == _Other.m_Min) && (m_Max == _Other.m_Max);
	}

	M_INLINE bool IsValid() const
	{
		return ((m_Min & 0xffff) < (m_Max & 0xffff)) && (((m_Min >> 16) & 0xffff) < ((m_Max >> 16) & 0xffff));
	}

	M_INLINE uint32 GetMinX() const
	{
		return m_Min & 0xffff;
	}

	M_INLINE uint32 GetMinY() const
	{
		return (m_Min >> 16) & 0xffff;
	}

	M_INLINE uint32 GetMaxX() const
	{
		return m_Max & 0xffff;
	}

	M_INLINE uint32 GetMaxY() const
	{
		return (m_Max >> 16) & 0xffff;
	}

	M_INLINE uint32 GetWidth() const
	{
		return GetMaxX() - GetMinX();
	}

	M_INLINE uint32 GetHeight() const
	{
		return GetMaxY() - GetMinY();
	}
};

// -------------------------------------------------------------------

#include "MMath_Vec128_TemplateSpec.h"
#include "MMath_SSE.h"
#include "MMath_Xenon.h"
#include "MMath_PS3.h"


#endif
