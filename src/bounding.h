#ifndef BOUNDING_H_
#define BOUNDING_H_

#include <d3dx9.h>
#include <vector>

class CAABB;
class CBoundingSphere;

// Function prototypes
bool PlaneIntersection( D3DXVECTOR3* intersectPt, const D3DXPLANE& Plane0, const D3DXPLANE& Plane1, const D3DXPLANE& Plane2 );
bool ConvexHullSphereIntersection( const D3DXPLANE* pPlanes, UINT numPlanes, const CBoundingSphere& sphere );
bool ConvexHullPointIntersection( const D3DXPLANE* pPlanes, UINT numPlanes, const D3DXVECTOR3& point );
bool AABBSphereIntersection ( const CAABB& AABB, const CBoundingSphere& Sphere );


// Class declarations
class CBoundingSphere
{
public:
	CBoundingSphere( );
	~CBoundingSphere( );

	// Empties & invalidates the bounding sphere 
	void Empty();

	// Adds a new position point to the bounding sphere and recalculates the new bounds
	void AddPoint( const D3DXVECTOR3& newPoint );

	// Takes a D3DX Mesh and updates the bounding sphere bounds
	HRESULT ComputeFromMesh( ID3DXMesh* mesh );	

	// Tells us whether the bounding sphere is empty / invalid.  Returns true if empty, false if valid
	bool IsEmpty() const;

	// If the bounding sphere contains (encloses) the point passed to the function, function returns true.
	bool Contains( const D3DXVECTOR3& point ) const;

	// member vars
	D3DXVECTOR3 m_Position;	// object/local space sphere centre 
	float		m_radius;			
};

// Takes two bounding spheres and tests for intersection.  Returns true to indicate a collision or false otherwise.
bool	IntersectBoundingSpheres( const CBoundingSphere &SphereA, const CBoundingSphere &SphereB );

class CAABB
{
public:
	CAABB();
	~CAABB();

	void Empty();

	//void AddPoint( const D3DXVECTOR3& newPoint );

	HRESULT ComputeFromMesh( ID3DXMesh* pMesh );

	// Takes an axis-aligned bounding box and returns the eight vertices that it is composed of.
	// The pointer to a vector must be an existing array of 8 vectors
	void GetVertices( D3DXVECTOR3* pOut ) const;

	bool IsEmpty() const;

	bool Contains( const D3DXVECTOR3& point ) const;

	// member vars
	//D3DXVECTOR3 m_centroid;
	D3DXVECTOR3 m_min;
	D3DXVECTOR3 m_max;	
};

/*
float	intersectMovingBoundingSpheres( const CBoundingSphere &StationarySphere, const CBoundingSphere &MovingSphere,
const D3DXVECTOR3& d ); 
*/

enum ePlanes
{
	P_LEFT = 0,
	P_RIGHT = 1,
	P_TOP = 2,
	P_BOTTOM = 3,
	P_NEAR = 4,
	P_FAR
};

// Frustum class.  Modified version of NVIDIA's PSM shadow mapping sample's frustum class

class CFrustum
{
public:
	CFrustum( );
	CFrustum( D3DXMATRIX* pMat );
	~CFrustum( );

	// Clears / Invalidates the frustum
	void Clear( );

	// Given an arbitrary matrix, extracts the six planes that make up the frustum.  Typically used with the view projection matrix
	// To yield the planes in world space for easy testing / reference
	HRESULT BuildFrustum( D3DXMATRIX* pMat );	
	int AABBIntersection( const CAABB& AABB ) const;

	std::vector< UINT >			m_nVertexLUT;
	std::vector< D3DXPLANE >	m_planes;	// six planes making up the frustum
	std::vector< D3DXVECTOR3 >  m_points;	// eight points making up the frustum (extremes = intersection points of planes)	
};

class CConvexHull
{
public:
	CConvexHull( )	
	{
		Clear(); 
		m_planes.reserve(20);
	}
	~CConvexHull( ) {}

	void Clear()	{ m_planes.clear(); }
	void AddPointToFrustum( const CFrustum& Frustum, const D3DXVECTOR3& newPoint );

	std::vector< D3DXPLANE > m_planes;
};

#endif