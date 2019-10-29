#include "debuglogger.h"
#include "dxcommon.h"

#include <ctime>
using namespace std;

CDebugLogger::CDebugLogger( )
{	
	m_bIsOpen = false;
}

CDebugLogger::~CDebugLogger( )
{
	// double check the file is closed
	if( m_bIsOpen )
		m_out.close( );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// OpenFile
//
// Purpose:		Opens a file specified by the filename passed to the function
// Parameters:	Char array (FileName)
// returns:		bool.  True = success, False = failure
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CDebugLogger::OpenFile( const std::string& FileName )
{
	// debug logger supports one file per instance, so don't react to another file open request if we've
	// already got one open

	if( m_bIsOpen )
		return false;

	m_out.open( FileName.c_str() );		// otherwise open the file
	
	if( ! m_out )				// and check everything is A-OK
		return false;		

	m_out << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << endl;
	m_out << "<html>" << endl;
	m_out << "<head>" << endl;
	m_out << "<title>Debug Log</title>" << endl;
	m_out << "</head>" << endl;
	m_out << "<body>" << endl;	

	m_out << "<font face=\"Arial\">" << endl;
		
	// print time in debug file as windows seems to love putting my debug files in random directories
	// regardless of how I execute my programs... This just helps me ensure I'm reading the most recent one
	
	time_t curr = time( 0 );
	char* pTime = ctime( &curr );	// pTime is statically allocated by ctime, so don't delete it!
	
	m_out << "<h3>Debug Log Opened: " << pTime << "</h3><br>" << endl;
	

	/*
	time_t lTime;
	wchar_t buf[26];
	errno_t err;

	time( &lTime );
	err = _wctime_s( buf, 26, &lTime );
	
	if( err != 0 )
	{
		m_out << "<h3>Debug Log Opened: " << buf << "</h3><br>" << endl;	
	}
	else
	{
		m_out << "<h3>Debug Log Opened: " << "Invalid Time" << "</h3><br>" << endl;	
	}	
	*/

	m_bIsOpen = true;	

	return true;				// success

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// CloseFile
//
// Purpose:		Closes the currently opened file
// Parameters:	none
// returns:		bool.  True = success, False = failure
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CDebugLogger::CloseFile( )
{

	if( m_bIsOpen )		// only close the file if it's actually open... 
	{
		m_out << "Debug Log closing...<br>" << endl;
		m_out << "</font face>" << endl;
		m_out << "</body>" << endl;
		m_out << "</html>" << endl;

		m_out.close( );
		m_bIsOpen = false;
		return true;	// success
	}
	else
		return false;	// failure
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// WriteToFile
//
// Purpose:		Writes the specified char buffer to the currently opened file
// Parameters:	Char array (Buffer)
// returns:		bool.  True = success, False = failure
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDebugLogger::Write( const std::string& Buffer, DebugLogSeverity severity )
{
	if( m_bIsOpen )
	{
		if( severity == DL_WARN )
		{
			m_out << "<font color=red>" << "Warning: " << "</font>" << Buffer << "<br>" << endl;
			return true;
		}
		
		m_out << Buffer << "<br>" << endl;
		return true;
	}

    return false;
}

bool CDebugLogger::Fatal( const std::string& Buffer, const std::string& fileName, DWORD dwLine )
{
	if( m_bIsOpen )
	{
		string cppFileName = getFilename( fileName );

		m_out << "<b><font color=\"red\">Error!</font></b><br>" << endl;
		m_out << "<font color=\"#38ACEC\">File: </font>" << fileName << ".  Line: " << dwLine << "<br>" << endl;
		m_out << "<font color=\"#38ACEC\">Description: </font>" << Buffer << "<br>" << endl;
		m_out << "<br>" << endl;

		return true;
	}

	return false;
}

bool CDebugLogger::WriteD3DError( const std::string& fileName, DWORD dwLine, LONG hr, const char* hrDescription, const char* stringifiedExpression )
{
	if( m_bIsOpen )
	{		
		string cppFileName = getFilename( fileName );

		m_out << "<b><font color=\"red\">D3D Error!</font></b><br>" << endl;
		m_out << "<b><font color=\"#38ACEC\">File: </font></b>" << cppFileName << ".  <b><font color=\"#38ACEC\">Line: </font></b>" << dwLine << "<br>" << endl;
		m_out << "<b><font color=\"#38ACEC\">Description: </font></b>" << hrDescription << "<br>" << endl;
		m_out << "<b><font color=\"#38ACEC\">Expression: </font></b>" << stringifiedExpression << "<br>" << endl;	
		m_out << "<br>" << endl;
		
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Separator
//
// Purpose:		Writes a line separator to the file to aid in consistency
// Parameters:	void
// returns:		bool.  True = success, False = failure
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDebugLogger::Separator( const std::string& Buffer )
{
	if( m_bIsOpen )
	{
		m_out << "<hr size=\"4\" noshade>" << endl;
		m_out << "<h4>" << Buffer << "</h4>" << endl;
		return true;
	}	
	
	return false;
}