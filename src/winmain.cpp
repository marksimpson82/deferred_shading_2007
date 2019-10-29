// Deferred Shading Application

#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include "window.h"
#include "dxcommon.h"
#include <string>
#include <vector>

using namespace std;

int WINAPI WinMain(	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{	
	CDebugLogger DL;

	// this disables writing to the log file.  The debug log checks whether the file is open prior to writing anything
	// so although the function calls still occur, no file will be opened and no data will be written.
#if !defined( SHIPPING_VERSION )

	bool bOpenFile = false;
	bOpenFile = g_Log.OpenFile( "DebugLog.html" );

	if( ! bOpenFile )
	{
		ERRORBOX( "Failed to open debug log" );
	}

#endif

	CApplication::SelectedRenderer eRenderer = CApplication::SR_DEFERRED;
	string sceneFileName = "";

	if( lpCmdLine )
	{
		vector<string> parameters;
		parameters.reserve( 4 );
	
		char * token;
		char * nextToken;

		char seps[] = " -";

		token = strtok_s( lpCmdLine, " -", &nextToken );
		
		while ( token != NULL && parameters.size() < 4 )
		{
			parameters.push_back( token );
			token = strtok_s( NULL, " -", &nextToken);		
		}

#if defined( DEBUG_VERBOSE )
		g_Log.Write( toString( parameters.size()) + string( " commandline arguments detected") );
		for( UINT i = 0; i < parameters.size(); i++ )
		{
			g_Log.Write( string( "argument ") + toString(i) + string( " : " ) + parameters[i] );
		}
#endif

		UINT paramNum = 0;
		bool bContinue = true;

		while( paramNum < parameters.size() && bContinue )
		{
			if( paramNum + 1 >= parameters.size() )
			{
				g_Log.Write( "Invalid number of command line arguments; ignoring.", DL_WARN );	
				bContinue = false;
			}
			else if( parameters[ paramNum ] == "renderer" || parameters[ paramNum ] == "Renderer" )
			{				
				if( parameters[ paramNum + 1 ] == "Deferred" || parameters[ paramNum + 1 ] == "deferred" )
				{
					eRenderer = CApplication::SR_DEFERRED;
					g_Log.Write( "Deferred Shading Renderer Selected" );
				}
				else if( parameters[ paramNum + 1 ] == "Forward" || parameters[ paramNum + 1 ] == "forward" )
				{
					eRenderer = CApplication::SR_FORWARD;
					g_Log.Write( "Forward Shading Renderer Selected" );
				}
				else if( parameters[ paramNum + 1 ] == "Dummy" || parameters[ paramNum + 1 ] == "dummy" )
				{
					eRenderer = CApplication::SR_DUMMY;
					g_Log.Write( "Dummy (base line) Renderer Selected" );
				}
				else
				{
					g_Log.Write( "Invalid renderer specified.  Default selected.  Valid choices are deferred, forward and dummy.", DL_WARN );									
				}

			}
			else if( parameters[ paramNum ] == "scene" || parameters[ paramNum ] == "Scene" )
			{
				sceneFileName = string( string( "scenes/") + parameters[ paramNum + 1 ]);					
			}
			else
			{
				g_Log.Write( "Unknown command line argument; ignoring.", DL_WARN );		
			}

			paramNum += 2;
		}	
	}
	
	MSG Msg;		
	ZeroMemory( &Msg, sizeof(Msg) );

	CWindow* pWindow = new CWindow;

	if( FAILED( pWindow->Initialise( hInstance, "Shading Demo", "MJS", 0, 0, 1024, 768, eRenderer, sceneFileName, true )))
	{
		ERRORBOX( "Error Initialising Application.  Exiting." );
		FATAL( "Failed to Initialise Window" );	
		Msg.wParam = -1;
	}
	else
	{
		Msg = pWindow->EnterMessageLoop();			
	}
	
	delete pWindow;
	g_Log.CloseFile();	
			
	return (int)Msg.wParam;	
}