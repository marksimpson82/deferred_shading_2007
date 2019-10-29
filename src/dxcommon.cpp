#include "dxcommon.h"
#include <string>
using namespace std;

// Utility function.  Breaks a filename path up into the the path and the filename, then returns the filename.

string getFilename( const string& filePath )
{
	// get the end of the string only.  Get rid of the path.
	unsigned int uiPathNameLength = 0;
	for( unsigned int i = 0; i < filePath.length(); i++ )
	{
		if( filePath[i] == '/' || filePath[i] == '\\' )
			uiPathNameLength = i + 1;	
	}

	return( filePath.substr( uiPathNameLength, filePath.length() - uiPathNameLength ));
}

string getPathname( const string& filePath )
{
    // get the pathname and omit the filename	
	unsigned int uiPathNameLength = 0;
	for( unsigned int i = 0; i < filePath.length(); i++ )
	{
		if( filePath[i] == '/' || filePath[i] == '\\' )
			uiPathNameLength = i + 1;	
	}

	return( filePath.substr( 0, uiPathNameLength ));
}