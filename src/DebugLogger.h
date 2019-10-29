#ifndef _DEBUGLOGGER_H_
#define _DEBUGLOGGER_H_

#include <Windows.h>
#include <fstream>
#include <string>
#include "singleton.h"

enum DebugLogSeverity
{
	DL_NORMAL = 0,
	DL_WARN,
	DL_TOTAL
};

class CDebugLogger : public ISingleton < CDebugLogger > 
{
public:
	CDebugLogger( );
	~CDebugLogger( );

	bool OpenFile( const std::string& FileName );
	bool CloseFile( );

	// Writes a fatal error to the debug log.  Writes the error, the fileName & line that generated the error
	bool Fatal( const std::string& Buffer, const std::string& fileName, DWORD dwLine );

	// Writes a line to the debug log.  Set severity to DL_WARN for possible causes of concern.  
	bool Write( const std::string& Buffer, DebugLogSeverity severity = DL_NORMAL );

	/* Writes a D3D error to the debug log.  Includes the fileName & line that generated the error
		along with the HRESULT, the description of the HRESULT and the expression that caused the error */
	bool WriteD3DError( const std::string& fileName, DWORD dwLine, LONG hr, const char* hrDescription, const char* stringifiedExpression );

	// Places a horizontal separator into the debug log along with some text, if desired
	bool Separator( const std::string& Buffer );

	// Inlines ///////////////////////////////////////////////////////////////////

	// Checks whether file is open
	bool IsFileOpen( ) const { return m_bIsOpen; }
private:
	std::ofstream m_out;	
	bool m_bIsOpen;
};

// define for clarity / laziness
#define g_Log CDebugLogger::GetSingleton()

#endif // _DEBUGLOGGER_H_