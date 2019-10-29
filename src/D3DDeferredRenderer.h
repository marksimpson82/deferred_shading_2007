#ifndef _D3DDEFERREDRENDERER_H_
#define _D3DDEFERREDRENDERER_H_

#include "D3DRenderer.h"
#include <string>
#include <d3dx9.h>

const unsigned int kiNumRTs = 3;

class CD3DDeferredRenderer : public ID3DRenderer
{
public:

	enum DebugOutputRT
	{		
		DO_MRT0 = 0,
		DO_MRT1,
		DO_MRT2,
		DO_NORMAL,
		DO_TOTAL,
	};

	CD3DDeferredRenderer( );
	~CD3DDeferredRenderer( );

	// Initialise the renderer
	HRESULT OneTimeInit( int iWidth, int iHeight, HWND hWnd, bool bWindowed, LPDIRECT3DDEVICE9 pDevice );

	// Cleanup the renderer's resources
	HRESULT Cleanup( );

	// SCENE DRAWING METHODS ///////////////////////////////////////////////

	// Renders the scene given a camera & scene file.  This is the top level entry function for the renderer
	HRESULT Render( Camera* pCamera, CScene* pScene, float fTimeDelta );

	// Fills the G-Buffer with scene attributes.  This is run prior to the lighting algorithms
	HRESULT FillGBuffer( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect );
	
	// LIGHTING METHODS ////////////////////////////////////////////////////

	// Draws a fullscreen quad that calculates the ambient term for the scene.  Does depth, too.
	HRESULT LightAmbient( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect );

	// Additively blends each separate light into the light transport buffer
	HRESULT LightDirectionals( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow );

	// Additively blends each separate light into the light transport buffer
	HRESULT LightOmnis( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow );

	// Additively blends each separate light into the light transport buffer
	HRESULT LightSpots( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow );

	// HRESULT Filter()
	// HRESULT Blur
	// HRESULT ConvertToLuminance
	// HRESULT ToneMap	

	// UTILITY DRAWING METHODS /////////////////////////////////////////////

	/* Utility function.  Binds or unbinds the g-buffer renderable textures as render targets.  
		If choosing to unbind, the function DOES NOT UNBIND RT 0. It is illegal to unbind RT 0.
		The user must manually bind their own desired RT 0 after calling this function */
	HRESULT BindGBufferAsRTs( bool bBindTexturesAsRT );

	/* Takes an enumerated RT value and displays the corresponding GBuffer RT in the viewport.  
		E.g. DO_MRT0 will output RT0. DO_NORMAL disables outputting a specific RT and displays a normal image */	
	HRESULT SetDebugOutputRT( DebugOutputRT eDebugOutputRT );

	// Recreate resources on start of app & resetting of device
	HRESULT OnResetDevice();

	// Lost device, destroy resources
	HRESULT OnLostDevice();

private:
	HRESULT DisplayRT( LPD3DXEFFECT pEffect, unsigned int iRT );
	
	RTT_s							m_GBufferRTs[ kiNumRTs ];	

	// need to be able to generate a cone mesh too for spot lights
	DebugOutputRT					m_eDebugOutputRT;

	std::string						m_DeferredEffectName;
	
};

#endif // _D3DDEFERREDRENDERER_H_