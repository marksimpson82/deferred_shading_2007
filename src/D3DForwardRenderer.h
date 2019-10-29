#ifndef _D3DFORWARDRENDERER_H_
#define _D3DFORWARDRENDERER_H_

#include "D3DRenderer.h"
#include <string>
#include <d3dx9.h>

class CD3DForwardRenderer :	public ID3DRenderer
{
public:
	CD3DForwardRenderer( );
	~CD3DForwardRenderer( );

	// Initialises the renderer
	HRESULT OneTimeInit( int iWidth, int iHeight, HWND hWnd, bool bWindowed, LPDIRECT3DDEVICE9 pDevice );

	// Cleans up the renderer's resources
	HRESULT Cleanup( );

	// Top level rendering function
	HRESULT Render( Camera* pCamera, CScene* pScene, float fTimeDelta );

	// Determine which meshes are both visible to the viewer AND inside each light's area of influence.  
	// Must be run after ID3DRenderer::FindVisibleEntities()
	HRESULT BuildIlluminationSet( Camera* pCamera, CScene* pScene );

	// Draws a fullscreen quad that calculates the ambient term for the scene.  Does depth, too.
	HRESULT LightAmbient( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect );

	// Additively blends each separate light into the light transport buffer
	HRESULT LightDirectionals( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pForward, LPD3DXEFFECT pShadow );

	// Additively blends each separate light into the light transport buffer
	HRESULT LightOmnis( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pForward, LPD3DXEFFECT pShadow );

	// Additively blends each separate light into the light transport buffer
	HRESULT LightSpots( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pForward, LPD3DXEFFECT pShadow );
	
	// On Reset Device -- Reset resources
	HRESULT OnResetDevice( );

	// Lost Device -- Destroy resources
	HRESULT OnLostDevice( );

private:
	std::string		m_ForwardShadingEffectName;
};


#endif // _D3DFORWARDRENDERER_H_