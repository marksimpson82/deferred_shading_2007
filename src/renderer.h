#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <d3dx9.h>
#include "node.h"
#include "camera.h"
#include "d3dUtility.h"


class CRenderer
{
public:
	CRenderer( );
	~CRenderer( );
	
	bool Initialise( int iWidth, int iHeight, HWND hwnd, bool bWindowed );
	bool Cleanup();	
    
	void StartScene( );
	void EndScene( );

	void Draw( Geometry* pGeometry );

	// inline
	void AttachCamera( Camera* pCamera ) { m_pCamera = pCamera; }

private:
	IDirect3DDevice9*	m_pDevice;
	Camera*				m_pCamera;
	//ID3DXMatrixStack	m_matStack;

	bool				m_bSceneInProgress;

	
};

#endif // _RENDERER_H_