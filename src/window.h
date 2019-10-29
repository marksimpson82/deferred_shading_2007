#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <windows.h>
#include "application.h"
#include <string>

class CWindow
{
public:

	CWindow();
	~CWindow();

	HRESULT Initialise( HINSTANCE hInstance,	// Application Instance handle
		char *szTitle,						// Title bar text
		char *szClassName,					// Class name
		int iX,								// Window top left x position
		int iY,								// Window top left y position
		int iWidth,							// Window width
		int iHeight,						// Window height )
		CApplication::SelectedRenderer eRenderer,
		std::string& sceneFileName,
		bool bWindowed = true );			// Windowed?

	// WndProc must be static, as the wndclass cannot handle a function with an implicit this->pointer.
	// Declaring it static gets around this problem, but creates another one:
	// Namely that we can't reference non static vars from a static function. So... hackity hack
	static LRESULT CALLBACK StaticWindowProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK IDD_DIALOG1DlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK IDD_DIALOG2DlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );


	// Normal WinProc
	LRESULT CALLBACK WndProc( UINT msg, WPARAM wParam, LPARAM lParam );
	
	MSG EnterMessageLoop();		

	void ActivateWindow();
	void DeactivateWindow();

	// inlines
	HWND GetHandle()			const { return m_hWnd; }
	HINSTANCE GetHInstance()	const { return m_hInstance; }
	int GetWidth()				const { return m_iWidth; }
	int GetHeight()				const { return m_iHeight; }

private:

	HWND		m_hWnd;	
	HINSTANCE	m_hInstance;	
	int			m_iWidth;
	int			m_iHeight;
	bool		m_bIsActive;
	
    CApplication*	m_pApp;	
};

#endif // _WINDOW_H_