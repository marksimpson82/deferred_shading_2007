#ifndef _SPATIAL_H_
#define _SPATIAL_H_

#include <d3dx9.h>
#include "model.h"
#include "bounding.h"

//---------------------------------------------------------------------------------//
// Spatial representation class.  Allows objects to have position, orientation etc
//---------------------------------------------------------------------------------//

class CSpatial
{
public:
	CSpatial( );
	virtual ~CSpatial( );

	// Rotates the object around the x, y and z axes by the amount of DEGREES specified for each
	void RotateBy( float fX, float fY, float fZ );

	// Rotates by a quaternion.  New orientation = old quat * rot
	void RotateBy( const D3DXQUATERNION& rot );

	// Sets orientation to new pitch, roll & yaw.  DEGREES.
    void SetOrientation( float pitch, float roll, float yaw );
	
	// Returns the object's world matrix in both the pOut parameter and the return value
	D3DXMATRIX* GetWorldMatrix( D3DXMATRIX* pOut );

	// Object's Update function.  Call once per frame
	virtual void Update( float fTimeDelta );

	// INLINES ///////////////////////////////////////////////////////////////////////

	// Move BY displacement delta vector specified by parameter
	void MoveBy( const D3DXVECTOR3& delta )						{ m_position += delta; }
	// Move by x, y z displacement deltas
	void MoveBy( const float x, const float y, const float z )	{ m_position.x += x; m_position.y += y; m_position.z +=z; }
	// Set the position to this explicit position vector
	void SetPosition( const D3DXVECTOR3& position )				{ m_position = position; }
	// Set the orientation to this explicit rotation quaternion 
	void SetOrientation( const D3DXQUATERNION& orientation )	{ m_orientation = orientation; }
	
	// Return position as a vector
	const D3DXVECTOR3&		GetPosition()		const	{ return m_position; }

	// Return orientation as a quaternion
	const D3DXQUATERNION&	GetOrientation()	const	{ return m_orientation; }	

protected:
	D3DXVECTOR3		m_position;
	D3DXQUATERNION	m_orientation;
};

//---------------------------------------------------------------------------//
// Node class.  Derived from Spatial; allows physical mesh & bounding sphere
//---------------------------------------------------------------------------//

class CNode : public CSpatial
{
public:
	CNode()
		: m_pModel( NULL ),
		m_bVisible( false ),
		m_uiIlluminationFlags( 0 ),
		m_uiShadowFlags( 0 ) 
	{
		m_BoundingSphere.Empty();
	}

	CNode( CModel* pModel )
		: m_pModel( pModel),
		m_bVisible( false ),
		m_uiIlluminationFlags( 0 ),
		m_uiShadowFlags( 0 )
	{ 		
		
	}

	~CNode();

	// Object's update function.  Call once per frame
	void Update( float fTimeDelta );

	// INLINES /////////////////////////////////////////////////////

	// Returns a pointer to the model used by this node
	CModel*	GetModel( ) const	{ return m_pModel; }
	
	// Sets the model used by the node and updates the node's bounding sphere
	void SetModel( CModel* pModel )
	{
		m_pModel = pModel;		
	}	

//private:	
	CModel*			m_pModel;
	CBoundingSphere m_BoundingSphere;
	CAABB			m_BoundingBox;

	bool			m_bVisible;
	UINT			m_uiIlluminationFlags;
	UINT			m_uiShadowFlags;
};

#endif // _SPATIAL_H_