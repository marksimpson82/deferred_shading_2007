//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: camera.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Defines a camera's position and orientation.
//         
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "camera.h"
#include "dxcommon.h"
#include <cassert>

Camera::Camera()
{
	Initialise( Camera::LANDOBJECT );
}

Camera::Camera(CameraType cameraType)
{
	Initialise( cameraType );
}

Camera::~Camera()
{

}

void Camera::Initialise( CameraType cameraType )
{
	m_cameraType = cameraType;

	m_fSpeed	= 25.0f; 

	m_pos   = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_right = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	m_up    = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	m_look  = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	SetFOV( D3DX_PI * 0.5 );
	SetNear( 1.0f );
	SetFar( 1.0f );

	Update();
}

void Camera::Update( )
{
	UpdateViewMatrix();
	UpdateProjectionMatrix();
	UpdateFrustum();
}

void Camera::Walk(float units)
{
	// move only on xz plane for land object
	if( m_cameraType == LANDOBJECT )
		//m_pos += D3DXVECTOR3(m_look.x, 0.0f, m_look.z) * units;
		m_pos += m_look * units;

	if( m_cameraType == AIRCRAFT )
		m_pos += m_look * units;
}

void Camera::Strafe(float units)
{
	// move only on xz plane for land object
	if( m_cameraType == LANDOBJECT )
		m_pos += D3DXVECTOR3(m_right.x, 0.0f, m_right.z) * units;

	if( m_cameraType == AIRCRAFT )
		m_pos += m_right * units;
}

void Camera::Fly(float units)
{
	// move only on y-axis for land object
	if( m_cameraType == LANDOBJECT )
		m_pos.y += units;

	if( m_cameraType == AIRCRAFT )
		m_pos += m_up * units;
}

void Camera::Pitch(float angle)
{
	D3DXMATRIX T;
	D3DXMatrixRotationAxis(&T, &m_right,	angle);

	// rotate m_up and m_look around m_right vector
	D3DXVec3TransformCoord(&m_up,&m_up, &T);
	D3DXVec3TransformCoord(&m_look,&m_look, &T);
}

void Camera::Yaw(float angle)
{
	D3DXMATRIX T;

	// rotate around world y (0, 1, 0) always for land object
	if( m_cameraType == LANDOBJECT )
		D3DXMatrixRotationY(&T, angle);

	// rotate around own up vector for aircraft
	if( m_cameraType == AIRCRAFT )
		D3DXMatrixRotationAxis(&T, &m_up, angle);

	// rotate m_right and m_look around m_up or y-axis
	D3DXVec3TransformCoord(&m_right,&m_right, &T);
	D3DXVec3TransformCoord(&m_look,&m_look, &T);
}

void Camera::Roll(float angle)
{
	// only roll for aircraft type
	if( m_cameraType == AIRCRAFT )
	{
		D3DXMATRIX T;
		D3DXMatrixRotationAxis(&T, &m_look,	angle);

		// rotate m_up and m_right around m_look vector
		D3DXVec3TransformCoord(&m_right,&m_right, &T);
		D3DXVec3TransformCoord(&m_up,&m_up, &T);
	}
}

void Camera::GetViewMatrix( D3DXMATRIX* pOut )	const
{
	assert( pOut );
    *pOut = m_view;
}

void Camera::UpdateViewMatrix( )
{
	// Keep camera's axes orthogonal to eachother
	D3DXVec3Normalize(&m_look, &m_look);

	D3DXVec3Cross(&m_up, &m_look, &m_right);
	D3DXVec3Normalize(&m_up, &m_up);

	D3DXVec3Cross(&m_right, &m_up, &m_look);
	D3DXVec3Normalize(&m_right, &m_right);

	// Build the view matrix:
	float x = -D3DXVec3Dot(&m_right, &m_pos);
	float y = -D3DXVec3Dot(&m_up, &m_pos);
	float z = -D3DXVec3Dot(&m_look, &m_pos);

	m_view(0,0) = m_right.x;	m_view(0, 1) = m_up.x;	m_view(0, 2) = m_look.x;	m_view(0, 3) = 0.0f;
	m_view(1,0) = m_right.y;	m_view(1, 1) = m_up.y;	m_view(1, 2) = m_look.y;	m_view(1, 3) = 0.0f;
	m_view(2,0) = m_right.z;	m_view(2, 1) = m_up.z;	m_view(2, 2) = m_look.z;	m_view(2, 3) = 0.0f;
	m_view(3,0) = x;			m_view(3, 1) = y;		m_view(3, 2) = z;			m_view(3, 3) = 1.0f;
}

void Camera::GetProjMatrix( D3DXMATRIX* pOut ) const
{
	assert( pOut );
    (*pOut) = m_proj;
}

void Camera::UpdateFrustum( )
{
	D3DXMATRIX matView;
	GetViewMatrix( &matView );
	D3DXMATRIX matViewProj = matView * m_proj;
    
	m_Frustum.BuildFrustum( &matViewProj );
}

void Camera::GetFrustum( CFrustum* pOut ) const
{
	assert(pOut);
	(*pOut) = m_Frustum;
}

void Camera::UpdateProjectionMatrix( )
{	
	float fAspect	= (float)m_iWidth / (float)m_iHeight;

	D3DXMatrixPerspectiveFovLH(
		&m_proj,
		m_fFOV, // 90 - degree
		fAspect,
		m_fNear,
		m_fFar);
}