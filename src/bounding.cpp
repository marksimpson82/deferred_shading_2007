#include "bounding.h"
#include "dxcommon.h"

#if defined( DEBUG_VERBOSE )
#include <string>
#endif

using namespace std;

// UTIL

//  returns if a floating point number is an IEEE special (NaN/Inf)
//	Taken from NVIDIA util functions
inline bool IsSpecial( const float& f )
{
	DWORD bits = *(const DWORD*)(&f);
	return ((bits&0x7f800000)==0x7f800000);
}

///////////////////////////////////////////////////////////////////////////
//  PlaneIntersection
//    computes the point where three planes intersect
//    returns whether or not the point exists.
bool PlaneIntersection( D3DXVECTOR3* intersectPt, const D3DXPLANE& Plane0, const D3DXPLANE& Plane1, const D3DXPLANE& Plane2 )
{
	D3DXVECTOR3 n0( Plane0.a, Plane0.b, Plane0.c );
	D3DXVECTOR3 n1( Plane1.a, Plane1.b, Plane1.c );
	D3DXVECTOR3 n2( Plane2.a, Plane2.b, Plane2.c );

	D3DXVECTOR3 n1_n2, n2_n0, n0_n1;  

	D3DXVec3Cross( &n1_n2, &n1, &n2 );
	D3DXVec3Cross( &n2_n0, &n2, &n0 );
	D3DXVec3Cross( &n0_n1, &n0, &n1 );

	float denominator = 1.f / D3DXVec3Dot( &n0, &n1_n2 );

	n1_n2 = n1_n2 * Plane0.d;
	n2_n0 = n2_n0 * Plane1.d;
	n0_n1 = n0_n1 * Plane2.d;

	*intersectPt = -(n1_n2 + n2_n0 + n0_n1) * denominator;

	if ( IsSpecial(intersectPt->x) || IsSpecial(intersectPt->y) || IsSpecial(intersectPt->z) )
		return false;

	return true;
}

// http://www.gamasutra.com/features/19991018/Gomez_4.htm (retrieved 2007/04/27)
// [1] J. Arvo. A simple method for box-sphere intersection testing. In A. Glassner, editor, Graphics Gems, pp. 335-339, Academic Press, Boston, MA, 1990.
bool AABBSphereIntersection ( const CAABB& AABB, const CBoundingSphere& Sphere )
{
    float s, d = 0; 

    //find the square of the distance
    //from the sphere to the box
    for( UINT i = 0; i < 3; i++ )
    {
        if( Sphere.m_Position[i] < AABB.m_min[i] )
        {
            s = Sphere.m_Position[i] - AABB.m_min[i];
            d += s * s; 
        }

        else if( Sphere.m_Position[i] > AABB.m_max[i] )
        {
            s = Sphere.m_Position[i] - AABB.m_max[i];
            d += s * s; 
        }

    }
    return d <= Sphere.m_radius * Sphere.m_radius;

}

// BOUNDING SPHERE //

CBoundingSphere::CBoundingSphere( )
{
	Empty();
}

CBoundingSphere::~CBoundingSphere( )
{
}

void CBoundingSphere::Empty( )
{
	m_Position = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );	
	m_radius = -kBigNumber;
}

void CBoundingSphere::AddPoint( const D3DXVECTOR3& newPoint )
{
	if( ! Contains( newPoint) )
	{
		D3DXVECTOR3 transVector = m_Position - newPoint;
		float fDistance = D3DXVec3Length( &transVector );		

		// double check
		if( fDistance > m_radius )		
			m_radius = fDistance;
	}	
}

HRESULT CBoundingSphere::ComputeFromMesh( ID3DXMesh* mesh )
{
	HRESULT hr = 0;

	if( ! mesh )
	{
		g_Log.Write( "Invalid Mesh passed to bounding sphere", DL_WARN );
		return E_FAIL;		
	}

	BYTE* v = 0;
	mesh->LockVertexBuffer(0, (void**)&v);

	// First of all, try to use the FVF.  FVF will generally work, but in some cases (when using vertex declarations)
	// the format of a vertex declaration cannot be represented using a FVF.  

	/*
	if( 0 != mesh->GetFVF() )	// will return 0 if the vertex format cannot be represented using the FVF
	{
	hr = D3DXComputeBoundingSphere(	(D3DXVECTOR3*)v,
	mesh->GetNumVertices(),
	D3DXGetFVFVertexSize( mesh->GetFVF() ),
	&m_position,
	&m_radius);
	}
	else	// so use the vertex declaration to get the format if this is the case
	{
	*/
	D3DVERTEXELEMENT9 decl[ MAX_FVF_DECL_SIZE ];
	V( mesh->GetDeclaration( decl ));

	UINT vertSize = D3DXGetDeclVertexSize( decl, 0 );

	V( D3DXComputeBoundingSphere(	(D3DXVECTOR3*)v,
		mesh->GetNumVertices(),
		vertSize,
		&m_Position,
		&m_radius));

	V( mesh->UnlockVertexBuffer() );

	return S_OK;
}


bool CBoundingSphere::IsEmpty( ) const
{
	if( m_radius <= 0.001f )
		return true;

	return false;
}

bool CBoundingSphere::Contains( const D3DXVECTOR3& point ) const
{
	// get a vector describing the translation between points, then calculate its magnitude
	D3DXVECTOR3 transVector =	m_Position - point;
	float fDistanceSquared	=	transVector.x * transVector.x + transVector.y * transVector.y + transVector.z * transVector.z;

	// if the distance between the two points is smaller than the sphere's radius, then the point is inside the sphere
	if( fDistanceSquared < m_radius * m_radius )
		return true;

	return false;
}

bool IntersectBoundingSpheres( const CBoundingSphere &SphereA, const CBoundingSphere &SphereB )
{
	D3DXVECTOR3 transVector = SphereA.m_Position - SphereB.m_Position;
	float fDistanceSquared	=	transVector.x * transVector.x
		+ transVector.y * transVector.y
		+ transVector.z * transVector.z;

	if( fDistanceSquared < ( SphereA.m_radius * SphereA.m_radius ) + ( SphereB.m_radius * SphereB.m_radius ) )
		return true;

	return false;
}

// AABB

CAABB::CAABB( )
{
	Empty();	
}

CAABB::~CAABB( )
{

}

void CAABB::Empty()
{
	m_min = D3DXVECTOR3( kBigNumber, kBigNumber, kBigNumber );
	m_max = D3DXVECTOR3( -kBigNumber, -kBigNumber, -kBigNumber );
}

bool CAABB::IsEmpty() const
{
	return m_min > m_max;
}

HRESULT CAABB::ComputeFromMesh( ID3DXMesh* pMesh )
{
	HRESULT hr = 0;

	if( ! pMesh )
	{
		g_Log.Write( "Invalid Mesh passed to bounding box", DL_WARN );
		return E_FAIL;		
	}

	BYTE* v = 0;
	V( pMesh->LockVertexBuffer(0, (void**)&v));

	D3DVERTEXELEMENT9 decl[ MAX_FVF_DECL_SIZE ];
	V( pMesh->GetDeclaration( decl ));

	UINT vertSize = D3DXGetDeclVertexSize( decl, 0 );

	V( D3DXComputeBoundingBox(	(D3DXVECTOR3*)v,
		pMesh->GetNumVertices(),
		vertSize,
		&m_min,
		&m_max ));

	V( pMesh->UnlockVertexBuffer() );

	return hr;
}

// courtesy of toymaker.info
// http://www.toymaker.info/Games/html/collisions.html (retrieved 2007/04/24)

void CAABB::GetVertices( D3DXVECTOR3* pOut ) const
{
	pOut[0] = D3DXVECTOR3( m_min.x, m_min.y, m_min.z ); // xyz
	pOut[1] = D3DXVECTOR3( m_max.x, m_min.y, m_min.z ); // Xyz
	pOut[2] = D3DXVECTOR3( m_min.x, m_max.y, m_min.z ); // xYz
	pOut[3] = D3DXVECTOR3( m_max.x, m_max.y, m_min.z ); // XYz
	pOut[4] = D3DXVECTOR3( m_min.x, m_min.y, m_max.z ); // xyZ
	pOut[5] = D3DXVECTOR3( m_max.x, m_min.y, m_max.z ); // XyZ
	pOut[6] = D3DXVECTOR3( m_min.x, m_max.y, m_max.z ); // xYZ
	pOut[7] = D3DXVECTOR3( m_max.x, m_max.y, m_max.z ); // XYZ	
}

// Frustum

CFrustum::CFrustum( )
{	
	m_planes.reserve( 6 );
	m_points.reserve( 8 );
	m_nVertexLUT.reserve( 6 );

	Clear();
}

CFrustum::CFrustum( D3DXMATRIX* pMat )
{
	m_planes.reserve( 6 );
	m_points.reserve( 8 );
	m_nVertexLUT.reserve( 6 );

	if( FAILED ( BuildFrustum( pMat )))
	{	
		Clear();
	}
}

CFrustum::~CFrustum( )
{

}

void CFrustum::Clear( )
{
	m_planes.clear();
	m_points.clear();
	m_nVertexLUT.clear();

	for( int i = 0; i < 6; i++ )
	{
		m_planes.push_back( D3DXPLANE( 0.0f, 0.0f, 0.0f, 0.0f ) );   
		m_nVertexLUT.push_back( 0 );
	}

	for( int i = 0; i < 8; i++ )
	{
		m_points.push_back( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) );
	}
}
// extract from a matrix the planes that make up the frustum
// See: http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf (retrieved 22nd March 2007)

// Summary of method: 

/*	If we take a vector v and a projection matrix M, the transformed point v' is = vM.  
A transformed point in clip space will be inside the frustum (AABB) if -w' < x' < w', -w' < y' < w' and 0 < z' < w'

We can deduce that it is on the positive side of the left plane's half space if x' > -w'
Since w' = v.col4 and x' = v.col1, we can rewrite this as v.col1 > -v.col4
thus v.col1 + v.col4 > 0   ... and ... v.(col1 + col4) > 0

This is just another way of writing the following: x( m14 + m11 ) + y( m24 + m21 ) + z( m34 + m31 ) + w( m44 + m41 )
For the untransformed point w = 1, so we can simplify to x( m14 + m11 ) + y( m24 + m21 ) + z( m34 + m31 ) + m44 + m41
Which is the same as the standard plane equation ax + by + cz + d = 0
Where a = m14 + m11, b = m24 + m21, c = m34 + m31 and d = m44 + m41

Running this algorithm on a projection matrix yields the clip planes in view space
Running it on a view projection matrix yields the clip planes in world space
.... for world view projection it yields the clip planes in object/local space

World space is usually ideal, so a view projection matrix is usually passed in.
*/

HRESULT CFrustum::BuildFrustum( D3DXMATRIX* pMat )
{
	HRESULT hr = S_OK;
    
	if( ! pMat )
		return E_FAIL;

	// break matrix into its constituent columns for notational convenience
	D3DXVECTOR4 column1( pMat->_11, pMat->_21, pMat->_31, pMat->_41 );
	D3DXVECTOR4 column2( pMat->_12, pMat->_22, pMat->_32, pMat->_42 );
	D3DXVECTOR4 column3( pMat->_13, pMat->_23, pMat->_33, pMat->_43 );
	D3DXVECTOR4 column4( pMat->_14, pMat->_24, pMat->_34, pMat->_44 );

	// store the planes in a vector4 because the D3DX plane normalise function fails to normalise d (afaik!)

	D3DXVECTOR4 planes[6];

	planes[P_LEFT]		= column4 + column1;	// -w' < x'		-> -(v.col4) < v.col -> 0 < (v.col4) + (v.col1) -> 0 < v.(col1 + col4)
	planes[P_RIGHT]		= column4 - column1;	// x' < w'		-> 0 < v.(col4 - col1)
	planes[P_TOP]		= column4 - column2;	// y' < w'		-> 0 < v.(col4 - col2)
	planes[P_BOTTOM]	= column4 + column2;	// -w' < y'		-> 0 < v.(col4 + col2)
	planes[P_NEAR]		= column3;				// 0 < z'		-> 0 < v.col3
	planes[P_FAR]		= column4 - column3;	// z' < w'		-> 0 < v.(col4 - col3)	
		
	for( int i = 0; i < 6; i++ )
	{
		// calc magnitude of the plane's normal vector and divide by it to normalise
		float fDot = planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z;
		float fScaleFactor = 1.0f / sqrt( fDot );
		planes[i] *= fScaleFactor;

		// copy normalised plane into our class member vars
		m_planes[i] = D3DXPLANE( planes[i].x, planes[i].y, planes[i].z, planes[i].w );
	}

	// NVIDIA sample code
	// build a bit-field that will tell us the indices for the nearest and farthest vertices from each plane
	for( int i = 0; i < 6; i++ )
	{
		m_nVertexLUT[i] = (( planes[i].x < 0.f ) ? 1 : 0 ) | ((planes[i].y < 0.f ) ? 2 : 0 ) | ((planes[i].z < 0.f ) ? 4 : 0 );		
	}


	// build point list.  Taken from NVidia's utility.h/cpp classes
	for ( int i = 0; i < 8; i++ )  
	{
		const D3DXPLANE& p0 = (i & 1) ? m_planes[P_NEAR]	: m_planes[P_FAR];		// alternate between near & far every iteration
		const D3DXPLANE& p1 = (i & 2) ? m_planes[P_BOTTOM]	: m_planes[P_TOP];		// bottom & top 
		const D3DXPLANE& p2 = (i & 4) ? m_planes[P_LEFT]	: m_planes[P_RIGHT];	// left & right

#if defined( DEBUG_VERBOSE )

		g_Log.Separator( string("Frustum Vertex ") + toString( i ) + string(" is the intersection of these 3 planes:") );

		g_Log.Write( (i & 1) ? string( toString(UINT(P_NEAR))	+ string(" : Near"))	: string( toString(UINT(P_FAR))		+ string(" : Far") ));
		g_Log.Write( (i & 2) ? string( toString(UINT(P_BOTTOM))	+ string(" : Bottom"))	: string( toString(UINT(P_TOP))		+ string(" : Top") ));
		g_Log.Write( (i & 4) ? string( toString(UINT(P_LEFT))	+ string(" : Left"))	: string( toString(UINT(P_RIGHT))	+ string(" : Right") ));	

#endif
		// For each three planes tested, there will be one common intersection point.  
		// These points make up the 8 vertices of the view frustum
		D3DXVECTOR3 point;
		PlaneIntersection( &m_points[i], p0, p1, p2 );		 
	}

	return hr;
}

// Taken from NVIDIA sample & http://www.flipcode.com/articles/article_frustumculling.shtml (retrieved 2007/04/27)
// Tests if an AABB is inside/intersecting the view frustum
int CFrustum::AABBIntersection( const CAABB& AABB ) const
{

	bool intersect = false;

	for ( int i = 0; i < 6; i++ )
	{
		UINT nV = m_nVertexLUT[i];
		// pVertex is diagonally opposed to nVertex
		D3DXVECTOR3 nVertex( ( nV & 1 ) ? AABB.m_min.x : AABB.m_max.x, ( nV & 2 ) ? AABB.m_min.y : AABB.m_max.y, ( nV & 4 ) ? AABB.m_min.z : AABB.m_max.z );
		D3DXVECTOR3 pVertex( ( nV & 1 ) ? AABB.m_max.x : AABB.m_min.x, ( nV & 2 ) ? AABB.m_max.y : AABB.m_min.y, ( nV & 4 ) ? AABB.m_max.z : AABB.m_min.z );

		if ( D3DXPlaneDotCoord( &m_planes[i], &nVertex ) < 0.f )
			return 0;

		if ( D3DXPlaneDotCoord( &m_planes[i], &pVertex ) < 0.f )
			intersect = true;
	}

	return ( intersect ) ? 2 : 1;


	/*
	D3DXVECTOR3 BoxVerts[8];
	AABB.GetVertices( BoxVerts );

	// for each plane, test whether the point is in the positive or negative half-space.
	for( int plane = 0; plane < 6; plane++ )
	{
        int iInCount = 8;
		int iPtIn = 0;

		for( int vert = 0; vert < 8; vert++ )
		{
			if( D3DXPlaneDotCoord( &m_planes[plane], &BoxVerts[vert] ) < 0 )
			{
				iPtIn = 0;
				iInCount--;
			}
		}

		if( iInCount == 0 )	// if none of the points were in the positive half space of the plane we just checked
			return( 0 );	// we know it is totally outside the frustum and can trivially reject it
	}

	return 1;
	*/
}

// Convex Hull Stuff

/*int edges[12][2]={{0,2},{0,3},{0,4},{0,5},{1,2},{1,3},{1,4},{1,5},{2,4},{2,5},{3,4},{3,5}};*/
// faces to which each edge belongs (each face partially defines 4 edges)


static const int edges[12][2] =	{{ P_LEFT,	P_TOP },	{ P_LEFT ,	P_BOTTOM },	{ P_LEFT,	P_NEAR },	{ P_LEFT,	P_FAR },
								{ P_RIGHT,	P_TOP },	{ P_RIGHT,	P_BOTTOM },	{ P_RIGHT,	P_NEAR },	{ P_RIGHT,	P_FAR },
								{ P_TOP,	P_NEAR },	{ P_TOP,	P_FAR },	{ P_BOTTOM, P_NEAR },	{ P_BOTTOM, P_FAR }};

/*
//int MakeShadowCasterVolume( plane *out, plane *frustum, vector3f &light )
void CConvexHull::AddPointToFrustum( const CFrustum& Frustum, const D3DXVECTOR3 newPoint )
{
	int		visible[6];
	float	distance[6];
	int		p1, p2, planes, i;

	planes = 0;

	for( i = 0; i < 6; i++ )
	{
		//distance[i] = frustum[i].Distance( light );
		// this should give the distance assuming the plane's normal (a b c) is normalised
		distance[i] = D3DXPlaneDotCoord( &Frustum.m_planes[i], &newPoint );
		if( distance[i] > 0 )
		{
			visible[i] = 1; 
			m_planes.push_back( Frustum.m_planes[i] );
		} 
		else
		{
			visible[i] = 0;
		}
	}

	for ( i = 0; i < 12; i++ )
	{
		p1 = edges[i][0];
		p2 = edges[i][1];

		// if one plane is visible but the other is not, we need to add a sillhouette edge?
		if( visible[p1] + visible[p2] == 1 )
		{
			//Pa = Plane1 * Dist2 - Plane2 * Dist1;
			//or
			//Pb=P2*t1-P1*t2
			
			if( visible[p2] == 1 )
			{
				//out[planes++].Combine( frustum[p1], distance[p2], frustum[p2], -distance[p1] );
				//out[planes++] = frustum[p1] * distance[p2] - frustum[p2] * distance[p1];

				// this is a load of rubbish... no idea how to combine planes 
				D3DXVECTOR4 frustumP1( Frustum.m_planes[p1].a, Frustum.m_planes[p1].b, Frustum.m_planes[p1].c, Frustum.m_planes[p1].d );
				D3DXVECTOR4 frustumP2( Frustum.m_planes[p2].a, Frustum.m_planes[p2].b, Frustum.m_planes[p2].c, Frustum.m_planes[p2].d );

				D3DXVECTOR4 tempResult = frustumP1 * distance[p2] - frustumP2 * distance[p1];
				D3DXPLANE finalResult( tempResult.x, tempResult.y, tempResult.z, tempResult.w );
				m_planes.push_back( finalResult );
			} 
			else
			{
				//out[planes++].Combine( frustum[p2], distance[p1], frustum[p1], -distance[p2] );

				// this is a load of rubbish... no idea how to combine planes 
				D3DXVECTOR4 frustumP1( Frustum.m_planes[p1].a, Frustum.m_planes[p1].b, Frustum.m_planes[p1].c, Frustum.m_planes[p1].d );
				D3DXVECTOR4 frustumP2( Frustum.m_planes[p2].a, Frustum.m_planes[p2].b, Frustum.m_planes[p2].c, Frustum.m_planes[p2].d );

				D3DXVECTOR4 tempResult = frustumP2 * distance[p1] - frustumP1 * distance[p2];
				D3DXPLANE finalResult( tempResult.x, tempResult.y, tempResult.z, tempResult.w );
				m_planes.push_back( finalResult );
			}
		}
	}	
}
*/

void CConvexHull::AddPointToFrustum( const CFrustum& Frustum, const D3DXVECTOR3& newPoint )
{

	// Generate the 8 extreme points of the view frustum
	//D3DXVECTOR3 vertices[8];
	//frustum.GetPointExtremes(vertices);

	// if a plane's dot product with a point is positive, then the point is in the plane's positive half space,
	// therefore we want to retain this plane as part of the convex hull.  If the point is in the negative half space, 
	// we need to discard the plane and generate new ones to cap the hole
	
	// by visible, it means the plane needs to be replaced
	bool bPlaneVisible[ 6 ];
	for( int j = 0; j < 6; j++)
	{
		float fDistance = D3DXPlaneDotCoord( &Frustum.m_planes[j], &newPoint );
		bPlaneVisible[j] = fDistance < 1e-4f;// ? true : false;

		//plane_visible[j] = D3DXPlaneDotCoord( &m_planes[j], &newPoint );
		//plane_visible[j] = (Frustum.planes[j].DistanceFrom(light_pos) < 1e-4f);
	}

	// Add all non-visible planes to the convex hull
	for (int j = 0; j < 6; j++)
	{
		if ( ! bPlaneVisible[j] )
		{
			m_planes.push_back( Frustum.m_planes[j] );
		}		
	}

	struct Face
	{
		int vertices[4];
		int faces[4];
	};

	/*
	static Face faces[] = 
	{
		{	// NEAR
			{ 4, 5, 7, 6 },
			{ cFrustum::TOP, cFrustum::RIGHT, cFrustum::BOTTOM, cFrustum::LEFT }
		},
		{	// FAR
			{ 0, 2, 3, 1 },
			{ cFrustum::LEFT, cFrustum::BOTTOM, cFrustum::RIGHT, cFrustum::TOP }
		},
		{	// LEFT
			{ 0, 4, 6, 2 },
			{ cFrustum::TOP, cFrustum::NEAR, cFrustum::BOTTOM, cFrustum::FAR }
		},
		{	// RIGHT
			{ 5, 1, 3, 7 },
			{ cFrustum::TOP, cFrustum::FAR, cFrustum::BOTTOM, cFrustum::NEAR }
		},
		{	// TOP
			{ 0, 1, 5, 4 },
			{ cFrustum::FAR, cFrustum::RIGHT, cFrustum::NEAR, cFrustum::LEFT }
		},
		{	// BOTTOM
			{ 6, 7, 3, 2 },
			{ cFrustum::NEAR, cFrustum::RIGHT, cFrustum::FAR, cFrustum::LEFT }
		}
	};
	*/

	// Search for faces that belong to visible planes

	static Face faces[] = 
	{
		{	// LEFT
			{ 5, 4, 6, 7 },
			{ P_TOP, P_NEAR, P_BOTTOM, P_FAR }
		},
		{	// RIGHT
			{ 0, 1, 3, 2 },
			{ P_TOP, P_FAR, P_BOTTOM, P_NEAR }
		},		
		{	// TOP
			{ 4, 5, 1, 0 },
			{ P_FAR, P_RIGHT, P_NEAR, P_LEFT }
		},
		{	// BOTTOM
			{ 2, 3, 7, 6 },
			{ P_NEAR, P_RIGHT, P_FAR, P_LEFT }
		},
		{	// NEAR
			{ 1, 5, 7, 3 },
			{ P_TOP, P_RIGHT, P_BOTTOM, P_LEFT }
		},
		{	// FAR
			{ 4, 0, 2, 6 },
			{ P_LEFT, P_BOTTOM, P_RIGHT, P_TOP }
		}
	};

	
	for ( int j = 0; j < 6; j++ )
	{
		if( bPlaneVisible[j] )
		{
			// Search for an adjacent face that is non-visible, representing a terminator edge
			Face& faceToReplace = faces[j];

			// for each adjacent face
			for ( int k = 0; k < 4; k++ )
			{
				// get the next adjacent face
				int fi = faceToReplace.faces[ k ];

				// If the adjacent face is not visible, then we must add a plane between the terminator edge and light position
				if ( ! bPlaneVisible[ fi ] )
				{
					// determine new vertices
					D3DXVECTOR3 vert1, vert2, vert3;
					vert1 = newPoint;
					vert2 = Frustum.m_points[ faceToReplace.vertices[ ( k + 1 ) & 3 ]];
					vert3 = Frustum.m_points[ faceToReplace.vertices[ k ]];

					// create plane from these vertices
					D3DXPLANE newPlane;
					D3DXPlaneFromPoints( &newPlane, &vert1, &vert2, &vert3 );

					// add plane to convex hull
					m_planes.push_back( newPlane );

					//m_planes.push_back( cPlane( newPoint, vertices[face.v[(k + 1) & 3]], vertices[ face.v[ k ]] ));
				}
			}
		}
	}
}





/*
// int edges[12][2]={{0,2},{0,3},{0,4},{0,5},{1,2},{1,3},{1,4},{1,5},{2,4},{2,5},{3,4},{3,5}};
// faces to which each edge belongs (each face partially defines 4 edges)


static const int edges[12][2] =	{{ P_LEFT,	P_TOP },	{ P_LEFT ,	P_BOTTOM },	{ P_LEFT,	P_NEAR },	{ P_LEFT,	P_FAR },
								{ P_RIGHT,	P_TOP },	{ P_RIGHT,	P_BOTTOM },	{ P_RIGHT,	P_NEAR },	{ P_RIGHT,	P_FAR },
								{ P_TOP,	P_NEAR },	{ P_TOP,	P_FAR },	{ P_BOTTOM, P_NEAR },	{ P_BOTTOM, P_FAR }};

//int MakeShadowCasterVolume( plane *out, plane *frustum, vector3f &light )
int MakeShadowCasterVolume( D3DXPLANE* out, D3DXPLANE *frustum, D3DXVECTOR3 &light )
{
	int		visible[6];
	float	distance[6];
	int		p1, p2, planes, i;

	planes = 0;
	
	for( i = 0; i < 6; i++ )
	{
		//distance[i] = frustum[i].Distance( light );
		// this should give the distance assuming the plane's normal (a b c) is normalised
		distance[i] = D3DXPlaneDotCoord( &frustum[i], &light );
		if( distance[i] > 0 )
		{
			visible[i] = 1; 
			out[planes++] = frustum[i];
		} 
		else
		{
			visible[i] = 0;
		}
	}

	for ( i = 0; i < 12; i++ )
	{
		p1 = edges[i][0];
		p2 = edges[i][1];

		// if one plane is visible but the other is not, we need to add a sillhouette edge?
		if( visible[p1] + visible[p2] == 1 )
		{
			
			//Pa = Plane1 * Dist2 - Plane2 * Dist1;
			//or
			//Pb=P2*t1-P1*t2
			
			if( visible[p2] == 1 )
			{
				//out[planes++].Combine( frustum[p1], distance[p2], frustum[p2], -distance[p1] );
				//out[planes++] = frustum[p1] * distance[p2] - frustum[p2] * distance[p1];
				D3DXVECTOR4 frustumP1( frustum[p1].a, frustum[p1].b, frustum[p1].c, frustum[p1].d );
				D3DXVECTOR4 frustumP2( frustum[p2].a, frustum[p2].b, frustum[p2].c, frustum[p2].d );

				D3DXVECTOR4 tempResult = frustumP1 * distance[p2] - frustumP2 * distance[p1];
				D3DXPLANE finalResult( tempResult.x, tempResult.y, tempResult.z, tempResult.w );
				out[planes++] = finalResult;
			} 
			else
			{
				//out[planes++].Combine( frustum[p2], distance[p1], frustum[p1], -distance[p2] );
				D3DXVECTOR4 frustumP1( frustum[p1].a, frustum[p1].b, frustum[p1].c, frustum[p1].d );
				D3DXVECTOR4 frustumP2( frustum[p2].a, frustum[p2].b, frustum[p2].c, frustum[p2].d );

				D3DXVECTOR4 tempResult = frustumP2 * distance[p1] - frustumP1 * distance[p2];
				D3DXPLANE finalResult( tempResult.x, tempResult.y, tempResult.z, tempResult.w );
				out[planes++] = finalResult;
			}
		}
	}
	return planes;
}
*/

bool ConvexHullSphereIntersection( const D3DXPLANE* pPlanes, UINT numPlanes, const CBoundingSphere& sphere )
{
	// assume sphere is inside by default
	bool bInside = true;

	for( UINT i = 0; i < numPlanes && bInside; i++ )
	{
		// test whether the sphere is on the positive half space of each frustum plane.  
		// If it is on the negative half space of one plane we can reject it.
		bInside &= (( D3DXPlaneDotCoord( &pPlanes[i], &sphere.m_Position ) + sphere.m_radius ) >= 0.0f);
	}

	return bInside;

}
bool ConvexHullPointIntersection( const D3DXPLANE* pPlanes, UINT numPlanes, const D3DXVECTOR3& point )
{
	bool bInside = true;

	for( UINT i = 0; i < numPlanes && bInside; i++ )
	{
		bInside&= ( D3DXPlaneDotCoord( &pPlanes[i], &point ) >= 0.0f );
	}
	return bInside;
}

