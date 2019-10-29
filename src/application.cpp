#include "application.h"
#include <string>
#include <iostream>
#include <istream>

#include "EffectManager.h"
#include "ModelManager.h"
#include "TextureManager.h"

#include "D3DRenderer.h"
#include "D3DDeferredRenderer.h"
#include "D3DForwardRenderer.h"


using namespace std;

CApplication::CApplication( )
{
	m_pRenderer				= NULL;
	
	m_eSelectedRenderer		= SR_DEFERRED;

	m_pDevice				= NULL;
	m_pFont					= NULL;
	m_pEffectManager		= NULL;
	m_pModelManager			= NULL;
	m_pTextureManager		= NULL;
	
	// NORMAL
	
	m_bInitialised			= false;
	m_hWnd					= (HWND)0;		
	m_hInstance				= (HINSTANCE)0;
	m_iWidth				= 0;
	m_iHeight				= 0;    
	m_bLostDevice			= false;
	m_dwFrameRate			= 60;
	m_bActive				= false;
}

CApplication::~CApplication( )
{		
	HRESULT hr = S_OK;

	V( OnLostDevice() );

	V( m_pRenderer->Cleanup());
	SAFE_DELETE( m_pRenderer );
		
	SAFE_DELETE( m_pEffectManager );
	SAFE_DELETE( m_pModelManager );
	SAFE_DELETE( m_pTextureManager );

	SAFE_RELEASE( m_pDevice );	
}


HRESULT CApplication::OneTimeInit( int iWidth, int iHeight, HWND hWnd, HINSTANCE hInstance, CApplication::SelectedRenderer eRenderer,
								  std::string& sceneFileName, bool bWindowed )
{
	HRESULT hr = S_OK;

	if( m_bInitialised )
		return S_OK;

	//--------------------------------------------------------------//
	// Init Vars
	//--------------------------------------------------------------//

	m_iWidth	= iWidth;
	m_iHeight	= iHeight;
	m_hWnd		= hWnd;
	m_hInstance = hInstance;
	m_bWindowed = bWindowed;

	V_RETURN( InitialiseD3D( iWidth, iHeight, hWnd, bWindowed ) );

	//--------------------------------------------------------------//
	// Create dynamically allocated stuff
	//--------------------------------------------------------------//

	// effect manager must be created after the device is initialised
	m_pEffectManager	= new CEffectManager( m_pDevice );
	m_pModelManager		= new CModelManager( m_pDevice );
    m_pTextureManager	= new CTextureManager( m_pDevice );

	if( eRenderer == SR_DEFERRED )
	{
		m_pRenderer = new CD3DDeferredRenderer();
		m_eSelectedRenderer = SR_DEFERRED;
	}
	else if( eRenderer == SR_FORWARD )
	{
        m_pRenderer = new CD3DForwardRenderer();
		m_eSelectedRenderer = SR_FORWARD;
	}
	else if( eRenderer == SR_DUMMY )
	{
        m_pRenderer = new CDummyRenderer();
		m_eSelectedRenderer = SR_DUMMY;
	}
	else
	{
		FATAL( string( "Failed to Initialise Renderer.  Renderer value passed is: " ) + toString( UINT( eRenderer ) ));
		return E_FAIL;
	}
	
	V_RETURN( m_pRenderer->OneTimeInit( iWidth, iHeight, hWnd, bWindowed, m_pDevice ) );
		
	V_RETURN( InitialiseMouse() );

	Camera SceneCamera;
	SceneCamera.SetWidth( m_iWidth );
	SceneCamera.SetHeight( m_iHeight );
	SceneCamera.SetFOV( DegreesToRadians( 70.0f ));
	SceneCamera.SetNear( 1.0f );
	SceneCamera.SetFar( 2048.0f );
	SceneCamera.SetPosition( D3DXVECTOR3( 0.0f, 10.0f, 0.0f ));	
	SceneCamera.Update( );		
	
	V_RETURN( m_Scene.OneTimeInit( m_pDevice, SceneCamera, sceneFileName ));	
	V_RETURN( OnResetDevice() );

	m_bInitialised	= true;
	m_bActive		= true;
	
	return S_OK;;
}

HRESULT CApplication::InitialiseD3D( int iWidth, int iHeight, HWND hWnd, bool bWindowed )
{
	HRESULT hr = S_OK;

	if( m_bInitialised )
		return S_OK;

	// Step 1: Create the IDirect3D9 object.

	IDirect3D9* d3d9 = 0;
	d3d9 = Direct3DCreate9( D3D_SDK_VERSION );
	
	if( ! d3d9 )
	{
		FATAL( "Failed to Create D3D9 Object" );
		return E_FAIL;
	}
	
	// Step 2: Check for hardware vp.
#if defined( DEBUG_PS )
	D3DDEVTYPE deviceType = D3DDEVTYPE_REF;
#else
	D3DDEVTYPE deviceType = D3DDEVTYPE_HAL;
#endif
	
	D3DCAPS9 caps;
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, deviceType, &caps);

	int vp = 0;
	if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// Must support pixel shader 3.0
	if( caps.PixelShaderVersion < D3DPS_VERSION( 3, 0 ) )
	{
		assert( 0 && "Graphics card does not support Shader Model 3.0" );
#if ! defined( _DEBUG )
		ERRORBOX( "This application requires a graphics card with Shader Model 3.0. Quitting" );
#endif
		FATAL( "PC has pixel shader version < 3.0" );
		d3d9->Release();
		return E_FAIL;		
	}
	
	// check for other things
	if( FAILED ( d3d9->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, D3DFMT_D24S8 )))
	{
		assert( 0 && "Graphics card does not support depth stencil surfaces" );
#if ! defined( _DEBUG )
		ERRORBOX( "This application requires a graphics card that supports depth stencil surfaces" );
#endif
		FATAL( "Graphics card does not support depth stencil surfaces" );
		d3d9->Release();
		return E_FAIL;	
	}

	// Step 3: Fill out the D3DPRESENT_PARAMETERS structure.
	
	m_D3DPP.BackBufferWidth            = iWidth;
	m_D3DPP.BackBufferHeight           = iHeight;
	m_D3DPP.BackBufferFormat           = D3DFMT_A8R8G8B8;
	m_D3DPP.BackBufferCount            = 1;
	m_D3DPP.MultiSampleType            = D3DMULTISAMPLE_NONE;
	m_D3DPP.MultiSampleQuality         = 0;
	m_D3DPP.SwapEffect                 = D3DSWAPEFFECT_DISCARD; 
	m_D3DPP.hDeviceWindow              = hWnd;
	m_D3DPP.Windowed                   = bWindowed;
	m_D3DPP.EnableAutoDepthStencil     = true; 
	m_D3DPP.AutoDepthStencilFormat     = D3DFMT_D24S8;
	m_D3DPP.Flags                      = 0;
	m_D3DPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	m_D3DPP.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Step 4: Create the device.

	// Set default settings 
	UINT AdapterToUse=D3DADAPTER_DEFAULT; 
	D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL; 

	// Look for 'NVIDIA NVPerfHUD' adapter 
	// If it is present, override default settings 
	for( UINT Adapter = 0; Adapter < d3d9->GetAdapterCount(); Adapter++ )  
	{ 
		D3DADAPTER_IDENTIFIER9  Identifier; 
		HRESULT    Res; 

		Res = d3d9->GetAdapterIdentifier( Adapter, 0, &Identifier ); 
		if ( strstr( Identifier.Description, "NVPerfHUD" ) != 0 ) 
		{ 
			AdapterToUse = Adapter; 
			DeviceType = D3DDEVTYPE_REF; 
			break; 
		} 
	} 

	if( FAILED( d3d9->CreateDevice( AdapterToUse, DeviceType, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_D3DPP, &m_pDevice ))) 
	{ 
		assert( 0 && "Device is Invalid.  Failed to creat device" );
#if !defined( _DEBUG )
		ERRORBOX( "Failed to create Device" );
#endif
		FATAL( "Failed to create Device" );
		d3d9->Release();
		return hr;	
	} 

	d3d9->Release(); // done with d3d9 object

	m_hWnd = hWnd;
	V( m_pDevice->GetViewport( &m_Viewport ) );
	V( m_pDevice->SetFVF( 0 ));
	V( m_pDevice->SetRenderState( D3DRS_LIGHTING, FALSE ));
	
	return S_OK;
}

HRESULT CApplication::InitialiseFont( )
{
	HRESULT hr = S_OK;

	V_RETURN( D3DXCreateFont( m_pDevice, 20, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &m_pFont ));
	
	return S_OK;
}

HRESULT CApplication::InitialiseMouse( )
{
	HRESULT hr = S_OK;

	GetCursorPos( &m_LastMousePos );
	m_NewMousePos = m_LastMousePos;
	m_MouseDelta.x = m_MouseDelta.y = 0;

	return S_OK;
}

HRESULT CApplication::OnIdle( float fTimeDelta )
{
	HRESULT hr = S_OK;

	if( m_bLostDevice || ! m_bActive )
	{
		Sleep( 200 );
	}

	// constantly update the mouse co-ordinates regardless of whether user is holding a button
	m_MouseDelta.x = m_NewMousePos.x - m_LastMousePos.x;
	m_MouseDelta.y = m_NewMousePos.y - m_LastMousePos.y;

	m_LastMousePos = m_NewMousePos;

	UpdateCamera( fTimeDelta );
//	UpdateNodes( fTimeDelta );

	// rotate the first model in the scene so we can root out lighting errors etc.
	
	if( ! m_Scene.m_pNodes.empty() )
	{
		// animate if we're using the default scene
		/*
		if( m_Scene.m_sceneFileName == defaultSceneFileName )
		{			
			list< CNode* >::iterator i = m_Scene.m_pNodes.begin();
			//(*i)->MoveBy( D3DXVECTOR3( 1.0f * fTimeDelta, 0.0f, 0.0f ) );
			//(*i)->RotateBy( 0.0f, 90.0f * fTimeDelta, 0.0f );

			D3DXVECTOR3 position = (*i)->GetPosition();

			D3DXMATRIX rotMat;
			D3DXMatrixRotationY( &rotMat, DegreesToRadians( 45.0f ) * fTimeDelta );
			D3DXVec3TransformCoord( &position, &position, &rotMat );

			(*i)->RotateBy( 0.0f, -45.0f * fTimeDelta, 0.0f );
			(*i)->SetPosition( position );

			(*++i)->RotateBy( 0.0f, -22.5f * fTimeDelta, 0.0f );	
		}*/
		
		for( list< CNode* >::iterator i = m_Scene.m_pNodes.begin(); i != m_Scene.m_pNodes.end(); i++ )
		{
			(*i)->Update( fTimeDelta );
		}
	}

	if( m_Keyboard.JustPressed( 'R' ) )
	{		
		unsigned int iRenderer = m_eSelectedRenderer;
		
		++iRenderer;
		
		if( iRenderer == 3 )
			iRenderer = 0;

		m_eSelectedRenderer = (CApplication::SelectedRenderer)iRenderer;		
		
		m_pRenderer->Cleanup();
		SAFE_DELETE( m_pRenderer );
		
		ID3DRenderer* pRenderer = NULL;

        if( m_eSelectedRenderer == SR_DEFERRED )
		{
			pRenderer = new CD3DDeferredRenderer;			
		}
		else if( m_eSelectedRenderer == SR_FORWARD )
		{
			pRenderer = new CD3DForwardRenderer;			
		}
		else if( m_eSelectedRenderer == SR_DUMMY )
		{
			pRenderer = new CDummyRenderer;
		}
		else
		{
			// choose default in the event of error
			m_eSelectedRenderer = SR_DEFERRED;
			pRenderer = new CD3DDeferredRenderer;
		}
		V_RETURN( pRenderer->OneTimeInit( m_iWidth, m_iHeight, m_hWnd, m_bWindowed, m_pDevice ));
		V_RETURN( pRenderer->OnResetDevice() );
		m_pRenderer = pRenderer;
	}

	// if we're in deferred mode, then allow user to select debug output

	if( SR_DEFERRED == m_eSelectedRenderer )
	{
		CD3DDeferredRenderer* pRenderer = static_cast< CD3DDeferredRenderer* >( m_pRenderer );		

		if( m_Keyboard.JustPressed( VK_NUMPAD0 ) )
		{
			V( pRenderer->SetDebugOutputRT( CD3DDeferredRenderer::DO_MRT0 ));
		}
		else if( m_Keyboard.JustPressed( VK_NUMPAD1 ) )
		{
			V( pRenderer->SetDebugOutputRT( CD3DDeferredRenderer::DO_MRT1 ));
		}

		else if( m_Keyboard.JustPressed( VK_NUMPAD2 ) )
		{
            V( pRenderer->SetDebugOutputRT( CD3DDeferredRenderer::DO_MRT2 ));
		}

		else if ( m_Keyboard.JustPressed( VK_NUMPAD3 ) )
		{
			V( pRenderer->SetDebugOutputRT( CD3DDeferredRenderer::DO_NORMAL ));
		}		
	}

	if( m_Keyboard.JustPressed( VK_RETURN ))
	{
		m_pRenderer->DebugToggleShadowMapDisplay();
	}

	// control lights

	if( m_Keyboard.JustPressed( 'I' ) )
	{
		for( list< LightDirectional_s* >::iterator i = m_Scene.m_pLightDirectional.begin(); i != m_Scene.m_pLightDirectional.end(); i++ )
		{
			(*i)->m_bActive = !(*i)->m_bActive;
		}
	}

	if( m_Keyboard.JustPressed( 'O' ) )
	{		
		for( list< LightOmni_s* >::iterator i = m_Scene.m_pLightOmni.begin(); i != m_Scene.m_pLightOmni.end(); i++ )
		{
			(*i)->m_bActive = !(*i)->m_bActive;
		}
	}

	if( m_Keyboard.JustPressed( 'P' ) )
	{		
		for( list< LightSpot_s* >::iterator i = m_Scene.m_pLightSpot.begin(); i != m_Scene.m_pLightSpot.end(); i++ )
		{
			(*i)->m_bActive = !(*i)->m_bActive;
		}
	}

	if( m_Keyboard.JustPressed( 'H' ) )
	{
		m_pRenderer->ToggleHDR();		
	}

	if( m_Keyboard.JustPressed( 'B' ) )
	{
		m_pRenderer->DebugToggleMeshBoundingWireframes();
	}

	if( m_Keyboard.JustPressed( 'L' ) )
	{
		m_pRenderer->DebugToggleLightBoundingWireframes();	
	}

	// Update the camera's internals
	m_Scene.m_Camera.Update( );

	V_RETURN( Render( fTimeDelta ) );
	
	m_Keyboard.Update();	
	return S_OK;

}

HRESULT CApplication::Render( float fTimeDelta )
{
	HRESULT hr = S_OK;
	
	if( ! m_pRenderer )
		return E_FAIL;

	// Check to see if device is lost
    hr = m_pDevice->TestCooperativeLevel( );

	// Render as normal if it's ok
	if( hr == D3D_OK )
	{		
		V( m_pDevice->Clear( 0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 ));
		V( m_pDevice->BeginScene() );

		V_RETURN( m_pRenderer->FindVisibleEntities( &m_Scene.m_Camera, &m_Scene ));

		// if we're using the deferred renderer, knowing which light affects which mesh is not required since lighting is an image space technique
		if( m_eSelectedRenderer == SR_FORWARD )
		{
			CD3DForwardRenderer* pForwardRenderer = (CD3DForwardRenderer*)m_pRenderer;
            V_RETURN( pForwardRenderer->BuildIlluminationSet( &m_Scene.m_Camera, &m_Scene ));
		}

		V_RETURN( m_pRenderer->BuildShadowCastingSet( &m_Scene.m_Camera, &m_Scene ));		
		V_RETURN( m_pRenderer->SortScene( &m_Scene.m_Camera, &m_Scene ));
		V_RETURN( m_pRenderer->Render( &m_Scene.m_Camera, &m_Scene, fTimeDelta ));

		// draw text
		
		int textX = 5;
		int textY = 5;
		const int kTextInc = 17;

		RECT font_rect;
		SetRect( &font_rect, textX, textY, kWidth, kHeight );
		string frameRate = string( "FPS: " ) + toString( m_dwFrameRate );

		V( m_pFont->DrawText(	NULL,			//pSprite
			frameRate.c_str(),					//pString
			-1,									//Count
			&font_rect,							//pRect
			DT_LEFT|DT_NOCLIP,					//Format,
			D3DCOLOR_ARGB(255, 255, 255, 0 )));	//colour

		textY += kTextInc;

		string renderer;

		switch( m_eSelectedRenderer )
		{
		case SR_DEFERRED:
			renderer = "Renderer: Deferred Shading";
			break;
		case SR_FORWARD:
			renderer = "Renderer: Forward Shading";
			break;
		case SR_DUMMY:
			renderer = "Renderer: Dummy (base line)";
			break;
		default:
			break;
		}

		SetRect( &font_rect, textX, textY, kWidth, kHeight );
		V( m_pFont->DrawText( NULL,	renderer.c_str(), -1, &font_rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 0 )));

		textY += kTextInc;

		SetRect( &font_rect, textX, textY, kWidth, kHeight );
		textY += kTextInc;

		string visibleInfo = string( "Visible Objects & Lights: " ) + toString( m_pRenderer->GetNumVisibleObjects( ) )
				+ string( " : " ) + toString( m_pRenderer->GetNumVisibleLights( ) );

		V( m_pFont->DrawText( NULL, visibleInfo.c_str(), -1, &font_rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255,255,255,0)));

		if( ! m_bActive )
		{
			SetRect( &font_rect, textX, textY, kWidth, kHeight );
			textY += kTextInc;
			string lostFocus = "Application has lost focus; ALT-TAB or move window to regain focus";

			V( m_pFont->DrawText(	NULL,			//pSprite
				lostFocus.c_str(),					//pString
				-1,									//Count
				&font_rect,							//pRect
				DT_LEFT|DT_NOCLIP,					//Format,
				D3DCOLOR_ARGB(255, 255, 255, 0 )));	//colour			
		}
		
		V( m_pDevice->EndScene() );
		V( m_pDevice->Present( 0, 0, 0, 0 ) );
		V( m_pRenderer->OnEndOfFrame() );
	}
	
	// If we've just lost the device
	else if( hr == D3DERR_DEVICELOST ) 
	{			
		m_bLostDevice = true;
	}
	else if( hr == D3DERR_DEVICENOTRESET )		// if the device is ready to use but needs reset
	{
		// Release the resources, reset the device and then reload the resources
		V( OnLostDevice() );
		V( m_pDevice->Reset( &m_D3DPP ));
		V( OnResetDevice() );

		m_bLostDevice = false;
	}
    
	return S_OK;
}

void CApplication::UpdateCamera( float fTimeDelta )
{
	float fRatio = 1.0f;

	if( m_Keyboard.KeyDown( VK_SHIFT ) )
		fRatio = 2.0f;

	float fCameraSpeed = m_Scene.m_Camera.GetSpeed();
	    	
	if( m_Keyboard.KeyDown('W') )
		m_Scene.m_Camera.Walk( fCameraSpeed * fRatio * fTimeDelta );

	if( m_Keyboard.KeyDown('S') )
		m_Scene.m_Camera.Walk( -fCameraSpeed * fRatio * fTimeDelta );

	if( m_Keyboard.KeyDown('A') )
		m_Scene.m_Camera.Strafe( -fCameraSpeed * fRatio * fTimeDelta );

	if( m_Keyboard.KeyDown('D') )
		m_Scene.m_Camera.Strafe( fCameraSpeed * fRatio * fTimeDelta );

	// Right button used to activate camera
	if( GetAsyncKeyState( VK_RBUTTON ) & 0x8000f )
	{
		// if the right mouse button was held during the last frame, move the view
		if( m_Keys.bMouseRight )	
		{
			if( m_MouseDelta.x )
				m_Scene.m_Camera.Yaw( (float)m_MouseDelta.x * 0.005f );

			if( m_MouseDelta.y )
				m_Scene.m_Camera.Pitch( (float)m_MouseDelta.y * 0.005f );				
		}
		else	// flag button as held for next frame
		{				
			m_Keys.bMouseRight = true;
		}
	}
	else	
	{
		m_Keys.bMouseRight = false;
	}		

}

void CApplication::Deactivate( )
{
	// pause renderer, kill sound etc
	m_bActive = false;
}

void CApplication::Activate( ) 
{
	// resume rendering, resume sound etc
	m_bActive = true;
}

HRESULT CApplication::OnResetDevice(  )
{
	HRESULT hr = S_OK;
	V_RETURN( InitialiseFont( ) );

	V_RETURN( m_pRenderer->OnResetDevice());
	
	V_RETURN( m_Scene.OnResetDevice() );

	return hr;

}

HRESULT CApplication::OnLostDevice( )
{	
	HRESULT hr = S_OK;

	m_pModelManager->FreeAllResources();
	m_pTextureManager->FreeAllResources();
	m_pEffectManager->FreeAllResources();

	m_pRenderer->OnLostDevice();
	
	V_RETURN( m_Scene.OnLostDevice());

	SAFE_RELEASE( m_pFont );
	return hr;
}