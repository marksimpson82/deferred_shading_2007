#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <d3dx9.h>
#include <vector>

#include "dxcommon.h"
#include "keyboard.h"
#include "scene.h"

// forward declarations
class CEffectManager;
class CModelManager;
class CTextureManager;
class ID3DRenderer;

struct KeyState_s
{
	KeyState_s() { bMouseLeft = false; bMouseRight = false; }
	bool bMouseLeft;
	bool bMouseRight;
};

class CApplication
{
public:
	CApplication( );
	~CApplication( );

	enum CameraState
	{ 
		CS_FREELOOK = 0,		
		CS_TOTAL,
	};

	enum SelectedRenderer
	{
		SR_DEFERRED = 0,
		SR_FORWARD,
		SR_DUMMY,
		SR_TOTAL,
	};

	// Initialises the application
	HRESULT OneTimeInit( int iWidth, int iHeight, HWND hWnd, HINSTANCE hInstance, CApplication::SelectedRenderer eRenderer,
		std::string& sceneFileName, bool bWindowed );	
		
	HRESULT OnIdle( float fTimeDelta );		// Entry point for updates.  This should be called when the application is idle 
	void UpdateCamera( float fTimeDelta );	// Update the camera
	
	HRESULT Render( float fTimeDelta );		// Entry point for rendering
	
	void Deactivate( );						// Deactivates the application.  Stops audio & graphics
	void Activate( );						// Reactivates the application.  Starts audio & graphics

	HRESULT OnResetDevice( );					// Recreate resources.
	HRESULT OnLostDevice( );					// Handle lost device.

	// INLINES /////////////////////////////////////////////////////////////////////////////////////////
	
	void MouseUpdatePos( POINT& newPos )	{ m_NewMousePos = newPos; }		// Pass in new mouse location on mouse move
	CKeyboard* GetKeyboard( )				{ return &m_Keyboard; }						
	void SetFrameRate( DWORD dwFrameRate )	{ m_dwFrameRate = dwFrameRate; }
	
protected:	
	// Statically Allocated
	HWND				m_hWnd;
	HINSTANCE			m_hInstance;
	int					m_iWidth;
	int					m_iHeight;
	bool				m_bWindowed;
	bool				m_bInitialised;

	DWORD				m_dwFrameRate;

	D3DPRESENT_PARAMETERS	m_D3DPP;
	bool				m_bLostDevice;

	CKeyboard			m_Keyboard;
	D3DVIEWPORT9		m_Viewport;

	// Geometry Objects
	CScene				m_Scene;

	bool				m_bActive;	

	// Alicia Keys & Danger Mouse
	KeyState_s			m_Keys;	
	POINT				m_LastMousePos;
	POINT				m_NewMousePos;
	POINT				m_MouseDelta;	

	IDirect3DDevice9*	m_pDevice;

	// Dynamically Allocated
	CEffectManager*		m_pEffectManager;
	CModelManager*		m_pModelManager;
	CTextureManager*	m_pTextureManager;
	
	ID3DRenderer*		m_pRenderer;
	SelectedRenderer	m_eSelectedRenderer;	
		
	// DA D3D Objects
	ID3DXFont*			m_pFont;	

	// Initialises the D3D object, creates device etc.
	HRESULT InitialiseD3D( int iWidth, int iHeight, HWND hWnd, bool bWindowed );

	// Initialises the D3DX font used by the application
	HRESULT InitialiseFont( );

	// Initialises the mouse values
	HRESULT InitialiseMouse();
};

#endif // _APPLICATION_H_