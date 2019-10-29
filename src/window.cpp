#include "window.h"
#include "resource.h"
#include "dxcommon.h"
#include "keyboard.h"
#include <cassert>
#include <string>
using namespace std;

#include "resource.h"

CWindow::CWindow(  )
{
	m_pApp		= NULL;
	m_hInstance	= NULL;
	m_hWnd		= NULL;
	m_bIsActive	= false;
	
	m_iWidth	= -1;
	m_iHeight	= -1;
	

}

CWindow::~CWindow()
{	
	SAFE_DELETE( m_pApp );	
}

HRESULT CWindow::Initialise( HINSTANCE hInstance,
						 char *szTitle,
						 char *szClassName,
						 int iX,
						 int iY,
						 int iWidth,
						 int iHeight,
						 CApplication::SelectedRenderer eRenderer,
						 string& sceneFileName,
						 bool bWindowed )
{	
	HRESULT hr = S_OK;

	m_hInstance				= hInstance;
	m_iWidth				= iWidth;
	m_iHeight				= iHeight;

	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = StaticWindowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = m_hInstance;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = MAKEINTRESOURCE( IDR_MENU1 );
	wc.lpszClassName = szClassName;	

	if( !RegisterClass(&wc) ) 
	{		
		FATAL( "Failed to register windows class" );
		return E_FAIL;
	}

	// create window and pass the this pointer as the last parameter
	m_hWnd = CreateWindow( szClassName, szTitle, 
		WS_EX_TOPMOST,
		50, 50, m_iWidth, m_iHeight,
		0, /*parent hwnd*/ 
		LoadMenu( hInstance, "IDR_MENU1" ),
		m_hInstance, this ); 	

	if( ! m_hWnd )
	{
		FATAL( "Failed to Create Window" );
		return E_FAIL;
	}

	RECT rcClient = { 0, 0, m_iWidth, m_iHeight };
	AdjustWindowRect( &rcClient, GetWindowLong( m_hWnd, GWL_STYLE ), TRUE );
	MoveWindow( m_hWnd, 50, 50, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, TRUE );

	ShowWindow( m_hWnd, SW_SHOW );
	UpdateWindow( m_hWnd );

	// NORMAL
	m_bIsActive = true;	

	// DYNAMIC
		
	m_pApp = new CApplication;
	V_RETURN( m_pApp->OneTimeInit( m_iWidth, m_iHeight, m_hWnd, m_hInstance, eRenderer, sceneFileName, bWindowed ));	

	return hr;
}

// Message loop
MSG CWindow::EnterMessageLoop()
{
	MSG msg;
	ZeroMemory( &msg, sizeof(MSG) );

	// Used for calculating time delta between frames
	static DWORD s_dwStartTime = timeGetTime();
	static DWORD s_dwLastTime = s_dwStartTime;
	static DWORD s_dwFrameCounter = 0;

	// Used for calculating the average framerate for the last second 
	static DWORD s_dwLastFrameCountTime = timeGetTime();
	static DWORD s_dwLastFrameCount = 0;		
	
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{			
			DWORD dwCurrTime = timeGetTime();
			DWORD dwTimeDelta = dwCurrTime - s_dwLastTime;

			DWORD dwTimePassedSinceLastFPSUpdate = dwCurrTime - s_dwLastFrameCountTime;
			if( dwTimePassedSinceLastFPSUpdate >= 1000 )
			{
				DWORD dwFrameDelta = s_dwFrameCounter - s_dwLastFrameCount;

				s_dwLastFrameCountTime = dwCurrTime;
				s_dwLastFrameCount = s_dwFrameCounter;

				m_pApp->SetFrameRate( dwFrameDelta );
			}

			// we're measuring in milliseconds currently, convert into a fraction of a second
			float fTime = (float)dwTimeDelta * 0.001f;
			if( FAILED( m_pApp->OnIdle( fTime )))	
			{
				FATAL( "Application OnIdle Loop Failed" );				
				assert( 0 && "Application OnIdle Loop Failed" );
#if !defined( _DEBUG )
				ERRORBOX( "There has been an error.  Quitting" );
#endif
				PostQuitMessage( -1 );
			}
			
			s_dwLastTime = dwCurrTime;	
			s_dwFrameCounter++;
		}
	}

	g_Log.Separator( "Application Finished.  Statistics: " );
	DWORD dwTotalTime = s_dwLastTime - s_dwStartTime;
	
	g_Log.Write( string("Total Time: ") + toString( dwTotalTime) );
	g_Log.Write( string("Total Frames: ") + toString( s_dwFrameCounter ) );
	
	float fAvgFPS = (float)s_dwFrameCounter / (float)( dwTotalTime * 0.001f );
	g_Log.Write( string("Average Framerate: ") + toString( fAvgFPS ) );

	return msg;
}

void CWindow::ActivateWindow( )
{
    m_bIsActive = true;
	
	if( m_pApp )
		m_pApp->Activate();
}

void CWindow::DeactivateWindow( )
{
	m_bIsActive = false;

	if( m_pApp )
		m_pApp->Deactivate();
}

// Windows wrapping trick courtesy of Evil Steve's gamedev.net topic:
// http://www.gamedev.net/community/forums/topic.asp?topic_id=303854

LRESULT CALLBACK CWindow::StaticWindowProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CWindow* pParent;

	// Get pointer to window
	if( msg == WM_CREATE)
	{
		pParent = (CWindow*)((LPCREATESTRUCT)lParam)->lpCreateParams;
		SetWindowLongPtr(hWnd,GWL_USERDATA,(LONG_PTR)pParent);
	}
	else
	{
		pParent = (CWindow*)GetWindowLongPtr( hWnd, GWL_USERDATA );
		if(!pParent) return DefWindowProc( hWnd, msg, wParam, lParam );
	}

	pParent->m_hWnd = hWnd;
	return pParent->WndProc( msg, wParam, lParam );
}

// Non-Static windows message processing function
// Can access member variables of the CWindow class

LRESULT CALLBACK CWindow::WndProc( UINT msg, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	// need to keep track of whether the mouse button is pressed so that I can capture the mouse
	// and move the view
	static bool sbRightMousePressed				= false;	
	static bool sbWindowCurrentlyBeingResized	= false;
	static bool sbWindowCurrentlyMinimised		= false;
		
	switch( msg )
	{
		
	case WM_PAINT:
		{
			hdc = BeginPaint( m_hWnd, &ps );
			RECT rt;
			GetClientRect( m_hWnd, &rt );
			EndPaint( m_hWnd, &ps );
			return 0;
			break;
		}

	// BASIC INPUT
	// KEYS

	case WM_KEYDOWN:
		{
			if( wParam == VK_ESCAPE )
				DestroyWindow( m_hWnd );
			else
			{
				CKeyboard* pKB = m_pApp->GetKeyboard();
				pKB->SetKeyPressed( wParam );

			}
			
			break;
		}	

	case WM_KEYUP:
		{
			CKeyboard* pKB = m_pApp->GetKeyboard();
			pKB->SetKeyReleased( wParam );
		}

	// MOUSE
	// BUTTONS
	case WM_RBUTTONDOWN:
		{		
			SetCapture( m_hWnd );
			SetCursor( LoadCursor ( NULL, IDC_CROSS ) );
			sbRightMousePressed = true;

			return 0;
			break;
		}

	case WM_RBUTTONUP:
		{
			if( sbRightMousePressed )
			{
				sbRightMousePressed = false;
				ReleaseCapture();
				SetCursor( LoadCursor ( NULL, IDC_ARROW )) ;
			}	
			return 0;
			break;
		}

	// MOUSE MOVEMENT
	case WM_MOUSEMOVE:
		{
			POINT newPos;
			newPos.x = LOWORD (lParam);
			newPos.y = HIWORD (lParam);
			
			m_pApp->MouseUpdatePos( newPos );			
		
			return 0 ;
			break;
		}
        	
		case WM_ACTIVATE:
			{
            if ( LOWORD( wParam ) == WA_ACTIVE )
				ActivateWindow();
			else
				DeactivateWindow();
		
			return( 0 );	
			break;
			}

		case WM_ENTERSIZEMOVE:
			{
				// pause rendering until we receive WM_EXITSIZEMOVE (when user finishes resizing the window)
				DeactivateWindow();
				sbWindowCurrentlyBeingResized = true;
				break;
			}

		case WM_EXITSIZEMOVE:
			{
				// Re-activate rendering
				// check to see if during the course of being in move mode the user resized the window.  
				// If they did resize it, reset the device.
				ActivateWindow();
				sbWindowCurrentlyBeingResized = false;
				break;
			}

		case WM_SIZE:
			{
				switch( wParam )
				{

				case SIZE_MINIMIZED:

					// PAUSE RENDERING
					break;

				case SIZE_RESTORED:

					// i.	If a SIZE_MINIMIZED previously occurred then un-pause rendering
					// ii.  If between a WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE then ignore
					// iii. Otherwise, check for the host window changing size. If the window has changed size reset the device

					break;
				
				case SIZE_MAXIMIZED:	

					// UNPAUSE RENDERING
					break;

				case SIZE_MAXSHOW:
					break;
				}
				break;
			}
		
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
			break;
		} 		

		
	case WM_COMMAND:
		{
			// if a command message has been sent find out what it is			
			switch( LOWORD ( wParam ))
			{

			case ID_FILE_EXIT:
				{
					PostMessage( m_hWnd, WM_CLOSE, 0, 0 );
				}
				break;

			case ID_HELP_ABOUT:
				{
					//MessageBox( m_hWnd, "Deferred Shading Demo.  Created by Mark Simpson", "About", MB_OK );
					int ret = DialogBox( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), m_hWnd, IDD_DIALOG2DlgProc );	
				}
				break;

			case ID_HELP_CONTROLS:
				{
					int ret = DialogBox( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), m_hWnd, IDD_DIALOG1DlgProc );	
				}
				break;
			
			default:
				break;
			}
			return( 0 );
		} break;
		
	default:
		break;
	}

	return DefWindowProc( m_hWnd, msg, wParam, lParam );
}

BOOL CALLBACK CWindow::IDD_DIALOG1DlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch( LOWORD(wParam))
		{

		case IDOK:
			{
				EndDialog( hDlg, IDOK );				
				break;
			}
			break;
		}

	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK CWindow::IDD_DIALOG2DlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch( LOWORD(wParam))
		{

		case IDOK:
			{
				EndDialog( hDlg, IDOK );				
				break;
			}
			break;
		}

	default:
		return FALSE;
	}
	return TRUE;
}
