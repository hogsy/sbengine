#ifndef __WModel_GlassSystem_h__
#define __WModel_GlassSystem_h__

#include "ModelsMisc.h"
//#include "MFloat.h"
//#include "../../../../Shared/MCC/MMisc.h"
//#include "../../../../Shared/MOS/XR/XR.h"
//#include "../../../../Shared/MOS/XR/XRCustomModel.h"
//#include "../../../../Shared/MOS/XRModels/Model_TriMesh/WTriMesh.h"

class CPClu_PTCollision;
typedef TVector4<uint8> CShardConn;

// Rendering controll
#define GLASS_SYSTEM_VOLUME
//#define GLASS_SYSTEM_FLAT

enum
{
	SHARD_CONN_U	= 0,	// Shards upper connection index
	SHARD_CONN_D	= 1,	// Shards lower connection index
	SHARD_CONN_R	= 2,		// Shards rightmost connection index
	SHARD_CONN_L	= 3,	// Shards leftmost connection index

	SHARD_BASE		= -1,	// This is a base, connected to a frame
	SHARD_HOLE		= -2,	// A gap in the glass entity
};

class CXR_Delaunay2D
{
public:
	// Delaunay2D edge class used in triangulation routine
	class CDelaunay2DEdge
	{
	public:
		CDelaunay2DEdge() : m_iIndex(-1) { }
		CDelaunay2DEdge(const int16& _i0, const int16& _i1) : m_iIndex(_i0,_i1) {}
		
		void Set(const int16& _i0, const int16& _i1)
		{
			m_iIndex = CVec2Dint16(_i0, _i1);
		}

		bool operator == (const CDelaunay2DEdge& _Edge) const
		{
			const CVec2Dint16& iIndex = _Edge.m_iIndex;
			
			if(iIndex[0] == m_iIndex[0] && iIndex[1] == m_iIndex[1])
				return true;
//			else if(iIndex[0] == m_iIndex[1] && iIndex[1] == m_iIndex[0])
//				return true;
			return false;
		}

		CVec2Dint16 m_iIndex;
	};

	// Delaunay2D triangle class used in triangulation routine
	class CDelaunay2DTriangle
	{
	public:
		CDelaunay2DTriangle() : m_R(0), m_R2(0), m_iIndex(0) { }
		CDelaunay2DTriangle(const CVec2Dfp32* _pPoints, const int16 _i0 = -1, const int16& _i1 = -2, const int16& _i2 = -3) : m_iIndex(_i0, _i1, _i2)
		{
//			m_iIndex[0] = MinMT(MinMT(_i0, _i1), _i2);
//			m_iIndex[2] = MaxMT(MaxMT(_i0, _i1), _i2);
//			m_iIndex[1] = (_i0 + _i1 + _i2) - (m_iIndex[0] + m_iIndex[2]);
			SetCircumCircle(_pPoints);
		}

		bool CCEncompasses(const CVec2Dfp32& _Point) const;
		bool IsLeftOf(const CVec2Dfp32& _Point) const
		{
			return _Point[0] > (m_Center[0] + m_R);
		}

		bool TriEncompasses(const CVec2Dfp32* _pPoints2D, const CVec2Dfp32& _Point) const;
		fp32 IsLeftOfSegment(const CVec2Dfp32& _Point, const CVec2Dfp32& _Point0, const CVec2Dfp32& _Point1) const;

	protected:
		void SetCircumCircle(const CVec2Dfp32* _pPoints);

		fp32 m_R, m_R2;
	public:
		CVec3Dint16 m_iIndex;
		CVec2Dfp32 m_Center;
	};

	CXR_Delaunay2D(const int32& _Seed = 0);
	~CXR_Delaunay2D();

	// Adds a base triangle to put points in
	void AddBaseTriangle(const CVec2Dfp32* _pPoints);
	void AddBaseTriangleRecalc(const CVec2Dfp32* _pPoints);

	// Scatter points over whole surface
	void GeneratePoints(const int& _nPoints);
	
	// Scatter points around a certain position, returns false if impact point doesn't hit a triangle
	bool GeneratePoints(const CVec2Dfp32& _LocalPosition, const int& _nPoints, const fp32& _Area);

	// Takes care of triangulation of a 2d shape
	void Triangulate();
	void Triangulate(const uint32* piPoints2D, const int& _i0, const int& _i1, const int& _i2, const int& _i3);
	
	// Checks if edge exist in parameter list
	int16 FindEdge(const CDelaunay2DEdge& _Edge, const TArray<CDelaunay2DEdge>& _lEdgeList);
	
	// Adds edges to list
	void HandleEdge(int16& _i0, int16& _i1, TArray<CDelaunay2DEdge>& _lEdgeList);

	CDelaunay2DTriangle*	GetOutputBasePtr() { return m_lOutput.GetBasePtr(); }
	CVec2Dfp32*				GetPointsBasePtr() { return m_lPoints2D.GetBasePtr(); }

	int			GetNumTriangles() { return m_lOutput.Len(); }
	int			GetNumPoints() { return m_lPoints2D.Len(); }

protected:
	int32						m_Seed;
	TArray<CVec2Dfp32>			m_lPoints2D;
	TArray<uint16>				m_liPoints2D;
	uint16						m_nBasePoints;
	TArray<CDelaunay2DTriangle>	m_lBaseTriangle;
	uint16						m_nBaseEdges;
	TArray<CDelaunay2DEdge>		m_lBaseEdges;
	TArray<CDelaunay2DTriangle>	m_lOutput;
};

class CGlassModelEntity : public CXR_ModelInstance
{
	friend class CXR_Model_GlassSystem;

	protected:
	public:
		class CGlassShard
		{
		public:
			CGlassShard() : m_Position(0), m_Rotations(0), m_Velocity(0) {}
			~CGlassShard() {}
			
			CVec3Dfp32	m_Position;
			CVec3Dfp32	m_Rotations;
			CVec3Dfp32	m_Velocity;
//			CVec2Dfp32	m_Dimension;
			CVec2Dfp32	m_UV[3];
			//CVec2Dfp32	m_2DVert[2];
			//CVec3Dfp32	m_Vertices[3];
//			CShardConn	m_Connection;
		};

		class CGlassEntity
		{
		public:
			CGlassEntity() : m_Resolution(0), m_SurfaceID(0), m_BrokenSurfaceID(0), m_Durability(1), m_bDisturbed(false)
			{
				m_Pos.Unit();
				m_lShards.SetLen(0);
			}

			~CGlassEntity() { m_lShards.Clear(); }

			// Shattering functions
			void PreShatter();		// Devide the glass entity into shards, preparing it for collapsing
			void ShatterPoint(const CVec3Dfp32& _CenterPos, const CVec3Dfp32& _Force);	// Shatter a piece of the glass at point with force
			
			void SplitConnection(const CGlassShard* _pShards, const int& _Shard);
			void CollapseConnection(const int& _Shard);
			void SearchConnection(const int& _Shard, const int &_Direction, CGlassShard* pBase);

			TArray<CGlassShard>		m_lShards;
			CMat4Dfp32				m_Pos;
			uint16					m_Resolution;
			int						m_SurfaceID;
			int						m_BrokenSurfaceID;
			int8					m_Durability;
			bool					m_bDisturbed;

//			TArray<uint16>			m_liShardsMapping;
//			fp32						m_HalfWidth;
//			fp32						m_HalfHeight;
//			TThinArray<CVec3Dfp32>	m_lVertices;
//			TArray<uint16>			m_lIndices;
//			CStr					m_SurfaceName;
		};

	public:
		CGlassModelEntity(const int& _Len);

		virtual void Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext* _pContext);
		virtual void OnRefresh(const CXR_ModelInstanceContext* _pContext, int _GameTick, const CMat4Dfp32* _pMat = NULL, int _nMat = 0, int _Flags = 0);
		virtual bool NeedRefresh(class CXR_Model* _pModel, const CXR_ModelInstanceContext* _pContext);

		virtual TPtr<CXR_ModelInstance> Duplicate() const;
		virtual void operator = (const CXR_ModelInstance& _Instance);
		void operator = (const CGlassModelEntity& _Instance);

		void AllocGlassObj(const int& _Len);
		void CreateGlassObj(const int& _iObj, const TThinArray<CVec3Dfp32>& _lV, const int& _SurfaceID, const int& _BrokenSurfaceID, const CMat4Dfp32& _WMat, const uint16& _Resolution, const fp32& _Durability);

		void BreakGlass(const int& _iGlassObj);

		const TArray<CGlassEntity>& GetNumGlass() { return m_lGlasses; }

		TArray<CGlassEntity>	m_lGlasses;
};


class CXR_Model_GlassSystem : public CXR_Model_Custom
{
	MRTC_DECLARE;

	public:
		// Overridables
		//virtual void Create(const char* _pParam);
		virtual void OnCreate(const char* _pParam);
		virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
		virtual TPtr<CXR_ModelInstance> CreateModelInstance();

		virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = 0);
		virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = 0);

		// Class functions
		virtual void LineIntersectGlass(CPClu_PTCollision& _PTCollision, CGlassModelEntity* _pGlassModelEntity);
	private:
};

#endif
