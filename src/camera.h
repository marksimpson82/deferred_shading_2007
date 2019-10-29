//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: camera.h
// 
// Original Author: Frank Luna (C) All Rights Reserved
//
// Modifications:
// Modified to incorporate addition camera variables & reduce processing time.
// View & Projection matrices are calculated on demand to reduce duplicate calculations.
// Camera also contains its own frustum class.  Again, this is only calculated on demand.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <d3dx9.h>
#include "bounding.h"

class Camera
{
public:
	enum CameraType { LANDOBJECT, AIRCRAFT };

	Camera();
	Camera(CameraType cameraType);
    ~Camera();

	void Initialise( CameraType cameraType = AIRCRAFT );
		
	void Strafe(float units);	// Moves the camera along its right (x) vector	
	void Fly(float units);		// Moves the camera along its up (y) vector	
	void Walk(float units);		// Moves the camera along its look (z) vector
	void Pitch(float angle);	// Pitches the camera.  Rotates about the camera's right (x) vector
	void Yaw(float angle);		// Yaws the camera.  Rotates about the camera's up (y) vector
	void Roll(float angle);		// Rolls the camera.  Rotates about the camera's look (z) vector
	
	// Calculates new view, projection and frustum values and stores them inside class.  
	// Call this once per frame after you've finished manipulating the camera
	void Update( );

	// Returns the already calculated view matrix.  Pass in a pointer to an EXISTING matrix 
	void GetViewMatrix( D3DXMATRIX* pOut )	const;

	// Returns the already calculated projection matrix.  Pass in a pointer to an EXISTING matrix 
	void GetProjMatrix(D3DXMATRIX* pOut )	const;

	// returns the already calculated frustum.  Pass in a pointer to an EXISTING frustum
	void GetFrustum( CFrustum* pOut ) const;
			
	// inlines

	void SetCameraType	( CameraType cameraType )	{ m_cameraType = cameraType; }	// Set camera type ( Camera::CameraType )
	void SetPosition	( D3DXVECTOR3& pos )		{ m_pos = pos;		} 			// Set camera's position vector
	void SetRight		( D3DXVECTOR3& right )		{ m_right = right;	}			// Set camera's right (x) vector
	void SetUp			( D3DXVECTOR3& up )			{ m_up = up;		}			// Set camera's up (y) vector
	void SetLook		( D3DXVECTOR3& look )		{ m_look = look;	}			// Set camera's look (z) vector
	
	void SetFOV			( float fFOV )				{ m_fFOV = fFOV;	}			// Set the camera's field of view (radians)
	void SetNear		( float fNear )				{ m_fNear = fNear;	}			// Set the camera's near clipping plane distance
	void SetFar			( float fFar )				{ m_fFar = fFar;	}			// Set the camera's far clipping plane distance

	void SetWidth		( int iWidth )				{ m_iWidth = iWidth;	}		// set display width	
	void SetHeight		( int iHeight )				{ m_iHeight = iHeight;	}		// set display height
	void SetSpeed		( float fSpeed )			{ m_fSpeed = fSpeed;	}		// set camera move speed

	const D3DXVECTOR3&	GetPosition( )	const	{ return m_pos;		}	// Returns camera's position
	const D3DXVECTOR3&	GetRight( )		const	{ return m_right;	}	// Returns camera's right (x) vector
	const D3DXVECTOR3&	GetUp( )		const	{ return m_up;		}	// Returns camera's up (y) vector
	const D3DXVECTOR3&	GetLook( )		const	{ return m_look;	}	// Returns camera's look (z) vector

	const float			GetFOV( )		const	{ return m_fFOV;	}	// Returns camera's Field of View in RADIANS
	const float			GetNear( )		const	{ return m_fNear;	}	// Returns camera's near plane
	const float			GetFar( )		const	{ return m_fFar;	}	// Returns camera's far plane

	const float			GetSpeed( )		const	{ return m_fSpeed;	}
		
private:
	CameraType  m_cameraType;
	
	float		m_fSpeed;

	D3DXVECTOR3 m_right;
	D3DXVECTOR3 m_up;
	D3DXVECTOR3 m_look;
	D3DXVECTOR3 m_pos;	

	int			m_iWidth;
	int			m_iHeight;

	float		m_fFOV;	
	float		m_fNear;
	float		m_fFar;

	D3DXMATRIX	m_view;
	D3DXMATRIX	m_proj;

	CFrustum	m_Frustum;

	// Generates a new view matrix based on parameters currently held in the camera class
	void UpdateViewMatrix( );

	// Generates a new projection matrix based on parameters currently held in camera class
	void UpdateProjectionMatrix( );

	// For the current view & projection matrix, generates a frustum 
	void UpdateFrustum( );
};
#endif // _CAMERA_H_