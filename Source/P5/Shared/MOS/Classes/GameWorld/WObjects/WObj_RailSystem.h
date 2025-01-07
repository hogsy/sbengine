class CRailSystem
{
public:
	enum { INVALID_POINT = -1 };
	typedef int32 PointIndex;

	class CPoint
	{
	public:
		CMat4Dfp32 m_RailMatrix;
		CMat4Dfp32 m_Matrix;
		uint8 m_Flags;

		CVec3Dfp32 GetPosition() { return CVec3Dfp32::GetRow(m_Matrix, 3); }

		//TArray<PointIndex> m_lConnections;
	};

	class CConnection
	{
	public:
		CConnection()
		{
			m_aiPoints[0] = INVALID_POINT;
			m_aiPoints[1] = INVALID_POINT;
			m_Flags = 0;
		}

		CConnection(PointIndex _Point0, PointIndex _Point1, uint8 _Flags)
		{
			m_aiPoints[0] = _Point0;
			m_aiPoints[1] = _Point1;
			m_Flags = _Flags;
		}

		PointIndex m_aiPoints[2];
		uint8 m_Flags;
	};

	template<class VECTOR, class SCALAR>
	VECTOR ClosestPointOnLine(VECTOR _LinePoint1, VECTOR _LinePoint2, VECTOR _Point)
	{
		VECTOR c = _Point - _LinePoint1;
		VECTOR Dir = (_LinePoint2 - _LinePoint1);
		Dir.Normalize();
		SCALAR Length = (_LinePoint1-_LinePoint2).Length();
		SCALAR t = Dir * c;
			
		if (t < 0) return _LinePoint1;
		if (t > Length) return _LinePoint2;
			
		Dir = Dir * t;
		return _LinePoint1 + Dir;
	}

	//
	TArray<CPoint> m_lPoints;
	TArray<CConnection> m_lConnections;

	//
	fp32 GetNumConnections(int32 p)
	{
		int32 Count = 0;
		for(int32 i = 0; i < m_lConnections.Len(); i++)
			if(m_lConnections[i].m_aiPoints[0] == p || m_lConnections[i].m_aiPoints[1] == p)
				Count++;

		return Count;
	}


	//
	//
	//
	PointIndex AddPoint(const CMat4Dfp32& _Matrix, int32 _Flags)
	{
//		PointIndex iPoint = INVALID_POINT;
		CVec3Dfp32 Pos = CVec3Dfp32::GetRow(_Matrix,3);

		// check to see if such point already exists
		for(int32 i = 0; i < m_lPoints.Len(); i++)
		{
			if(Pos.AlmostEqual(CVec3Dfp32::GetRow(m_lPoints[i].m_Matrix,3), 2))
				return i;
		}
		
		// create new one
		CPoint Point;
		Point.m_Matrix = _Matrix;
		Point.m_Flags = _Flags;
		return m_lPoints.Add(Point);
	}

	//
	//
	//
	void AddConnection(const CMat4Dfp32& _Matrix0, const CMat4Dfp32& _Matrix1, uint8 _Flags, uint8 _PFlags0=0, uint8 _PFlags1=0)
	{
		//M_ASSERT(_iPoint0!=INVALID_POINT, "Invalid point");
		//M_ASSERT(_iPoint1!=INVALID_POINT, "Invalid point");
		PointIndex aiPoints[2] = {	AddPoint(_Matrix0, _PFlags0),
									AddPoint(_Matrix1, _PFlags1)};

		m_lConnections.Add(CConnection(aiPoints[0], aiPoints[1], _Flags));

		/*
		PointIndex aiPoints[2] = {INVALID_POINT, INVALID_POINT};
		//CVec3Dfp32 aPos[2] = {CVec3Dfp32::GetRow(_Matrix0,3), CVec3Dfp32::GetRow(_Matrix1,3)};

		for(int32 i = 0; i < m_lPoints.Len(); i++)
		{
			CVec3Dfp32 Pos = CVec3Dfp32::GetRow(m_lPoints[i].m_Matrix,3)
			if(Pos.AlmostEqual(aPos[0], 0.001f))
				aiPoints[0] = i;

			if(Pos.AlmostEqual(aPos[1], 0.001f))
				aiPoints[1] = i;
		}
		*/
		/*

		for(int32 i = 0; i < 2; i++)
		{
			if(aiPoints[i] == INVALID_POINT)
			{
			}
		}*/
	}

	//
	// used for autobend
	//
	void StraightCheck(int32 p, int32 _iCon1, int32 _iCon2)
	{
		CVec3Dfp32 Pos = CVec3Dfp32::GetRow(m_lPoints[p].m_Matrix,3);

		// stright check
		PointIndex aiPoints[2];
		if(m_lConnections[_iCon1].m_aiPoints[0] != p)
			aiPoints[0] = m_lConnections[_iCon1].m_aiPoints[0];
		else
			aiPoints[0] = m_lConnections[_iCon1].m_aiPoints[1];

		if(m_lConnections[_iCon2].m_aiPoints[0] != p)
			aiPoints[1] = m_lConnections[_iCon2].m_aiPoints[0];
		else
			aiPoints[1] = m_lConnections[_iCon2].m_aiPoints[1];

		CVec3Dfp32 aPos[2] = {	CVec3Dfp32::GetRow(m_lPoints[aiPoints[0]].m_Matrix,3),
								CVec3Dfp32::GetRow(m_lPoints[aiPoints[1]].m_Matrix,3)};

		CVec3Dfp32 aDir[2] = {aPos[0]-Pos, aPos[1]-Pos};
		aDir[0].Normalize();
		aDir[1].Normalize();

		if(Abs(aDir[0]*aDir[1]) > 0.90f)
		{
			m_lConnections[_iCon1].m_Flags &= ~1;
			m_lConnections[_iCon1].m_Flags |= 8;
			m_lConnections[_iCon2].m_Flags &= ~1;
			m_lConnections[_iCon2].m_Flags |= 8;
		}
	}

	//
	//
	//
	void ConstructExtraPoints()
	{
		bool Restart = true;
		while(Restart)
		{
			Restart = false;

			for(int32 c = 0; c < m_lConnections.Len() && !Restart; c++)
			{
				PointIndex k = m_lConnections[c].m_aiPoints[0];
				PointIndex m = m_lConnections[c].m_aiPoints[1];
				uint8 Flags = m_lConnections[c].m_Flags;

				for(int32 p = 0; p < m_lPoints.Len() && !Restart; p++)
				{
					if(p == k || p == m)
						continue;

					CVec3Dfp32 Point = ClosestPointOnLine<CVec3Dfp32,fp32>(m_lPoints[k].GetPosition(),
																		m_lPoints[m].GetPosition(),
																		m_lPoints[p].GetPosition());

					// skip this if it's not on the line
					if((Point-m_lPoints[p].GetPosition()).Length() > 0.1f)
						continue;


					m_lConnections.Del(c);

					m_lConnections.Add(CConnection(k, p, Flags));
					m_lConnections.Add(CConnection(m, p, Flags));

					Restart = true; // restart the loop
				}
			}
		}

	}

	//
	//
	//
	void RemoveUnnessesaryData()
	{
		bool Restart = true;
		while(Restart)
		{
			Restart = false;

			for(int32 p = 0; p < m_lConnections.Len() && !Restart; p++)
			{
				for(int32 c = 0; c < m_lConnections.Len() && !Restart; c++)
				{
					if(c == p)
						continue;

					bool Remove = false;

					if(m_lConnections[c].m_aiPoints[0] == m_lConnections[p].m_aiPoints[0] &&
						m_lConnections[c].m_aiPoints[1] == m_lConnections[p].m_aiPoints[1])
						Remove = true;

					if( m_lConnections[c].m_aiPoints[1] == m_lConnections[p].m_aiPoints[0] &&
						m_lConnections[c].m_aiPoints[0] == m_lConnections[p].m_aiPoints[1])
						Remove = true;

					if(Remove)
					{
						if(m_lConnections[c].m_Flags != m_lConnections[p].m_Flags)
							Remove = Remove;

						m_lConnections.Del(c);
						Restart = true;
					}
				}
			}
		}
	}

	//
	//
	//
	void DumpInfo()
	{
		for(int32 p = 0; p < m_lPoints.Len(); p++)
		{
			CVec3Dfp32 Pos = CVec3Dfp32::GetRow(m_lPoints[p].m_Matrix,3);
			M_TRACEALWAYS("%d (%f,%f,%f)\n", p, Pos.k[0], Pos.k[1], Pos.k[2]);
		}

		for(int32 c = 0; c < m_lConnections.Len(); c++)
		{
			M_TRACEALWAYS("%d (%d<->%d) %d\n", c, m_lConnections[c].m_aiPoints[0], m_lConnections[c].m_aiPoints[1],
												m_lConnections[c].m_Flags);
		}
	}

	//
	//
	//
	void ConstructMatrices()
	{
		for(int32 p = 0; p < m_lPoints.Len(); p++)
		{
			TArray<CConnection> lAffectingConnections;
			for(int32 c = 0; c < m_lConnections.Len(); c++)
			{
				if(m_lConnections[c].m_aiPoints[0] == p || m_lConnections[c].m_aiPoints[1] == p)
					lAffectingConnections.Add(m_lConnections[c]);
			}

			bool GotStriaght = false;
			for(int32 i = 0; i < lAffectingConnections.Len(); i++)
				if(!(lAffectingConnections[i].m_Flags&1))
				{
					GotStriaght = true;
					break;
				}

			if(GotStriaght)
			{
				for(int32 i = 0; i < lAffectingConnections.Len(); i++)
					if(lAffectingConnections[i].m_Flags&1)
					{
						lAffectingConnections.Del(i);
						i--;
					}

				if(lAffectingConnections.Len() == 1 || lAffectingConnections.Len() == 2)
				{
					// Get matrices
					CMat4Dfp32 Mat[2];

					if(lAffectingConnections.Len() == 1)
					{
						Mat[0] = m_lPoints[lAffectingConnections[0].m_aiPoints[0]].m_Matrix;
						Mat[1] = m_lPoints[lAffectingConnections[0].m_aiPoints[1]].m_Matrix;
					}
					else // 2 affecting
					{
						Mat[0] = m_lPoints[lAffectingConnections[0].m_aiPoints[0]].m_Matrix;
						Mat[1] = m_lPoints[lAffectingConnections[1].m_aiPoints[0]].m_Matrix;

						if(lAffectingConnections[0].m_aiPoints[0] == p)
							Mat[0] = m_lPoints[lAffectingConnections[0].m_aiPoints[1]].m_Matrix;

						if(lAffectingConnections[1].m_aiPoints[0] == p)
							Mat[1] = m_lPoints[lAffectingConnections[1].m_aiPoints[1]].m_Matrix;
					}

					CVec3Dfp32 Up = CVec3Dfp32::GetRow(m_lPoints[p].m_Matrix,0);
					CVec3Dfp32 Direction = (CVec3Dfp32::GetRow(Mat[1],3)-CVec3Dfp32::GetRow(Mat[0],3));
					Direction.Normalize();

					// Apply data and recreate matrix
					CMat4Dfp32 Matrix;
					Direction.SetRow(Matrix, 0);	// Set direction
					Up.SetRow(Matrix, 2);			// Set Up vector
					Matrix.RecreateMatrix(0, 2);	// Recreate

					(CVec3Dfp32::GetRow(m_lPoints[p].m_Matrix,3)).SetRow(Matrix,3);		// Set position
					m_lPoints[p].m_RailMatrix = Matrix;
				}
			}
			else
			{
				// problems!
				p=p;
				m_lPoints[p].m_RailMatrix = m_lPoints[p].m_Matrix;
			}
		}
	}
};
