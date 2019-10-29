#include "spatial.h"
#include "dxcommon.h"
#include <cassert>
#include <vector>
using namespace std;

CSpatial::CSpatial( )
{
	m_position = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	D3DXQuaternionIdentity( &m_orientation );	
}

CSpatial::~CSpatial( )
{
}

void CSpatial::Update( float fTimeDelta )
{

}

D3DXMATRIX* CSpatial::GetWorldMatrix( D3DXMATRIX* pOut )
{
	assert( pOut );
	D3DXMatrixRotationQuaternion( pOut, &m_orientation );
	pOut->_41 = m_position.x;
	pOut->_42 = m_position.y;
	pOut->_43 = m_position.z;

	return pOut;
}

void CSpatial::RotateBy( float fX, float fY, float fZ )
{
	fX = DegreesToRadians( fX );
	fY = DegreesToRadians( fY );
	fZ = DegreesToRadians( fZ );

	D3DXQUATERNION rotQuat, rotResult;
	D3DXQuaternionRotationYawPitchRoll( &rotQuat, fY, fX, fZ );

	D3DXQuaternionMultiply( &rotResult, &m_orientation, &rotQuat );
	m_orientation = rotResult;	
}

//////////////////////////////////////////////////////////////////////////
// Rotates node by a quaternion
// In: Quaternion to rotate by
// Out: void
//////////////////////////////////////////////////////////////////////////

void CSpatial::RotateBy( const D3DXQUATERNION& rotBy )
{
	D3DXQUATERNION quatResult;

	D3DXQuaternionMultiply( &quatResult, &m_orientation, &rotBy );
	m_orientation = quatResult;
}

// DEGREES
void CSpatial::SetOrientation( float pitch, float roll, float yaw )
{
	pitch = DegreesToRadians( pitch );
	roll = DegreesToRadians( roll );
	yaw = DegreesToRadians( yaw );

	D3DXQUATERNION newRot;
	D3DXQuaternionRotationYawPitchRoll( &newRot, yaw, pitch, roll );
	
	m_orientation = newRot;
}

// CNODE

CNode::~CNode( )
{
	//SAFE_DELETE( m_pModel );
	m_pModel	= NULL;
	m_bVisible	= false;	
}

void CNode::Update( float fTimeDelta )
{
	// Since bounding spheres don't necessarily originate at the origin of the node's object space (e.g. a mesh may be in a certain position
	// to make pivoting more intuitive, so that it starts at 0,0,0 and ends at 128,0,0.  The bounding sphere centre will be at ~64,0,0 )
	// we must transform the bounding sphere's object space centre point by the node's world matrix to get the correct position.  
	
	D3DXMATRIX nodeWorld;
	GetWorldMatrix( &nodeWorld );
	D3DXVec3TransformCoord( &m_BoundingSphere.m_Position, &m_pModel->m_ObjectSpaceBS.m_Position, &nodeWorld );
	m_BoundingSphere.m_radius = m_pModel->m_ObjectSpaceBS.m_radius;

	// sort out the bounding boxes

	vector< D3DXVECTOR3 > WorldOBBVertices;
	WorldOBBVertices.resize( 8 );

	// Transform the node's object space bounding box vertices into world space
	for( UINT i = 0; i < 8; i++ )
	{
		D3DXVECTOR3 worldCoord;
		D3DXVec3TransformCoord( (D3DXVECTOR3*)&WorldOBBVertices[i], (D3DXVECTOR3*)&m_pModel->m_BoundingBoxVertices[i], &nodeWorld );		      
	}

	D3DXVECTOR3 min, max;
	// then create a new AABB that bounds the extremities
	min.x = max.x = WorldOBBVertices[0].x;
	min.y = max.y = WorldOBBVertices[0].y;
	min.z = max.z = WorldOBBVertices[0].z;

	for( UINT i = 1; i < 8 ; i++ )
	{
		if ( WorldOBBVertices[i].x < min.x ) 
			min.x = WorldOBBVertices[i].x;

		if ( WorldOBBVertices[i].x > max.x ) 
			max.x = WorldOBBVertices[i].x;

		if ( WorldOBBVertices[i].y < min.y )
			min.y = WorldOBBVertices[i].y;
		
		if ( WorldOBBVertices[i].y > max.y )
			max.y = WorldOBBVertices[i].y;
		
		if ( WorldOBBVertices[i].z < min.z )
			min.z = WorldOBBVertices[i].z;
		
		if ( WorldOBBVertices[i].z > max.z )
			max.z = WorldOBBVertices[i].z;
	}

	m_BoundingBox.m_min = min;
	m_BoundingBox.m_max = max;
}