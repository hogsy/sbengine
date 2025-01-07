
#include "PCH.h"

#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

bool Phys_Intersect_TriSphere(const CVec3Dfp32 _tri[3], const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, float _Radius, bool _bOrder, CCollisionInfo* _pCollisionInfo)
{      
	MAUTOSTRIP(Phys_Intersect_TriSphere, false);
  CPlane3Dfp32 PolyPlane;  
  float       pDist[2];  

  CVec3Dfp32 Triangle[3];

  PolyPlane.n = (_tri[0] - _tri[1]) / (_tri[2] - _tri[1]);  
  PolyPlane.n.Normalize();
  PolyPlane.d = -_tri[0] * PolyPlane.n;
 
  pDist[0] = PolyPlane.n * _Origin + PolyPlane.d;
  pDist[1] = PolyPlane.n * _Dest + PolyPlane.d;  
   
  // To check intersection in both directions we reverse the triangle
  if(pDist[0] < 0)
  {    
    pDist[0] = -pDist[0];
    pDist[1] = -pDist[1];          

    // Reverse the triangle
    Triangle[0] = _tri[2];
    Triangle[1] = _tri[1];
    Triangle[2] = _tri[0];

    PolyPlane.n = -PolyPlane.n;
    PolyPlane.d = -PolyPlane.d;
  }
  else
  {       
    Triangle[0] = _tri[0];
    Triangle[1] = _tri[1];
    Triangle[2] = _tri[2];
  }

  // Does the sphere pass the plane at all?
  if((pDist[0] >=  _Radius) && (pDist[1] >=  _Radius))
  {
    return false;
  }

  CPlane3Dfp32 EdgePlane;
  CPlane3Dfp32 PerpPlane;  
  float       pPerpDist[2];
  float       fEdgeDist;
  float       fAllowed;
  float       t;  
  int         i;
  int         j;
  bool        bEdge = false;  
  CVec3Dfp32   CurrentEdge;
  CPlane3Dfp32 CollisionPlane(CVec3Dfp32(0, 0, 1), 0);

  // PerpPlane is a plane that is perpendicular to (_Origin - _Dest) and passes through the current edge
  PerpPlane.n = (_Origin - _Dest);
  PerpPlane.n.Normalize();

  float fLength;

  float fProjection;

  // Any number larger than 1.0 will suffice
  float       MinDist = 2.0f;  

  int k;  

  for(i = 0; i < 3; i++)
  {    
    j = (i + 1) % 3;
    k = (i + 2) % 3;
    
    CurrentEdge = Triangle[i] - Triangle[j];

    // EdgePlane is a plane parallell with (_Origin - _Dest) and passes passes through the current edge    
    EdgePlane.n = -(_Origin - _Dest) / (CurrentEdge);
    fLength = EdgePlane.n.Length();
    
    if(fLength < 0.0001f)
      continue;

    EdgePlane.n = EdgePlane.n * (1.0f / fLength);
    EdgePlane.d = -EdgePlane.n * Triangle[i];    

    // This depends on wich way the sphere is going
    if((EdgePlane.n * Triangle[k] + EdgePlane.d) > 0.0f)
    {
      EdgePlane.n = -EdgePlane.n;
      EdgePlane.d = -EdgePlane.d;    
    }

    // The sphere does not intersect the polygon
    fEdgeDist = EdgePlane.n * _Origin + EdgePlane.d;
    
    if(fEdgeDist >= _Radius)
      return false;
          
    if(fEdgeDist > -_Radius)
    {
      // Check for edge hits
      PerpPlane.d = -PerpPlane.n * Triangle[i];

      fAllowed = M_Sqrt(_Radius * _Radius - fEdgeDist * fEdgeDist);   

      pPerpDist[0] = PerpPlane.n * _Origin + PerpPlane.d;
      pPerpDist[1] = PerpPlane.n * _Dest + PerpPlane.d;
      
      if((pPerpDist[0] >=  fAllowed) && (pPerpDist[1] >=  fAllowed))      
        return false;      

      // Ok, we have collided get t value
      t = (fAllowed - pPerpDist[0]) / (pPerpDist[1] - pPerpDist[0]);                 

      bEdge = true;

      if(t < MinDist)      
      {
        MinDist = t;
        
        // This is a vector to the spheres location at the collision from
        // one of the vertices of the current edge
        CollisionPlane.n = (_Origin + (_Dest - _Origin) * MinDist) - Triangle[i]; 

        // By subtracting a piece of the edge we get the collision normal
        fProjection = (CollisionPlane.n * CurrentEdge) / (CurrentEdge * CurrentEdge);
        CollisionPlane.n = CollisionPlane.n - CurrentEdge * fProjection;                

        CollisionPlane.n.Normalize();
        CollisionPlane.d = -Triangle[i] * CollisionPlane.n;        
      }        
    }                
  }

  // Check if the projected edge collision is within the triangle, if it is
  // it wasnt really an edge collision
  if(bEdge)
  {
    CVec3Dfp32   CollisionPoint = _Origin + (_Dest - _Origin) * MinDist;
    CPlane3Dfp32 EdgePolyPlane;

    for(i = 0; i < 3; i++)
    {
      j = (i + 1) % 3;      

      CurrentEdge = Triangle[i] - Triangle[j];
    
      EdgePolyPlane.n = CurrentEdge / PolyPlane.n;
      EdgePolyPlane.n.Normalize();
      EdgePolyPlane.d = -EdgePolyPlane.n * Triangle[i];

      if((EdgePolyPlane.n * CollisionPoint + EdgePolyPlane.d) > 0.0f)
        break;      
    }

    if(i != 3)
      bEdge = false;
  }

  // If we get here we know that the sphere has collided with the polygon.
  // If bEdge is true it has collided with an edge otherwise get t value
  // from PolyPlane intersection
  
  if(!bEdge)
  {
    t = (_Radius - pDist[0]) / (pDist[1] - pDist[0]);    

    if( t < MinDist )          
    {
      CollisionPlane = PolyPlane;
      MinDist = t;        
    }
  }    

  if(MinDist > 1.0f)  
    return false;    

  if(!_pCollisionInfo)
    return true; 

  // If MinDist is less than zero it means that the start position intersected the triangle
  if(MinDist > 0.0f)
  {
    _pCollisionInfo->m_bIsValid    = true;
	_pCollisionInfo->m_bIsCollision = true;
    _pCollisionInfo->m_Time        = MinDist;
    _pCollisionInfo->m_LocalPos    = (_Origin + (_Dest - _Origin) * MinDist) - CollisionPlane.n * _Radius;
    _pCollisionInfo->m_Velocity    = CVec3Dfp32(0.0f, 0.0f, 0.0f);
    _pCollisionInfo->m_RotVelocity = 0.0f;  
    _pCollisionInfo->m_Plane       = CollisionPlane;    
  }
  else
  {
    _pCollisionInfo->m_bIsValid    = true;
	_pCollisionInfo->m_bIsCollision = true;
    _pCollisionInfo->m_Time        = 0.0f;
    _pCollisionInfo->m_LocalPos    = _Origin;
    _pCollisionInfo->m_Velocity    = CVec3Dfp32(0.0f, 0.0f, 0.0f);
    _pCollisionInfo->m_RotVelocity = 0.0f;  
    _pCollisionInfo->m_Plane       = CollisionPlane;      
  }

  return true;
}
