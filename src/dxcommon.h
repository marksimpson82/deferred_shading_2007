#ifndef _DXAPPDEFINES_H_
#define _DXAPPDEFINES_H_

// defines

//#define DEBUG_PS
//#define DEBUG_VS

//#define DEBUG_VERBOSE

// define this in the project settings instead
//#define SHIPPING_VERSION

// common includes
#include "EffectManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "singleton.h"
#include "DebugLogger.h"
#include <string>
#include <sstream>
#include <d3dx9.h>

#if defined(DEBUG) | defined( _DEBUG )
	#include <dxerr9.h>
#endif 

// MACROS ////////////////////////////////////////////////////////////////////////////

// Checks pointer is non-null, then releases COM object
#define SAFE_RELEASE(p)			{ if(p) { (p)->Release(); (p)=NULL; } }		

// Checks pointer is non-null, then deletes using delete.  USE SAFE_DELETE_ARRAY for Arrays
#define SAFE_DELETE(p)			{ if(p) { delete (p);     (p)=NULL; } }		

// Checks pointer is non-null, then deletes array using delete[]
#define SAFE_DELETE_ARRAY(p)	{ if(p) { delete[] (p);   (p)=NULL; } }		

// Easy error message box
#define ERRORBOX( Message ) { MessageBox( 0, Message, "Oh No!", MB_ICONERROR | MB_OK ); }

// logging & debugging macros adapted from DXUT V macro.

/* Use the V(x) macro for non-critical errors where program disruption is minimal (e.g. failing to render something or bind a texture).  
	this is typically used for standard functionality in the program (e.g. in render loops). In release mode, this macro doesn't log & has no side effects. */

/* Use the V_RETURN(x) macro for critical errors that will interrupt the running of the program. (e.g. failing to create g-buffer render targets)
	This is typically used for initialisation & cleanup functions and any other function that absolutely requires flawless execution.
	In release mode, this macro stops logging but still returns the HRESULT so we can detect errors and exit cleanly. */

#if defined( DEBUG ) | defined( _DEBUG )
	#define V(x)		{ hr = x; if( FAILED( hr )) { g_Log.WriteD3DError( __FILE__, (DWORD)__LINE__, hr, DXGetErrorString9(hr), #x ); DebugBreak(); }}
	#define V_RETURN(x) { hr = x; if( FAILED( hr )) { g_Log.WriteD3DError( __FILE__, (DWORD)__LINE__, hr, DXGetErrorString9(hr), #x ); DebugBreak(); return hr; }} 
#else
	#define V(x)		{ hr = x; }
	#define V_RETURN(x) { hr = x; if( FAILED( hr )) { return hr; } }
#endif

// Calls the Debug Logger's Fatal function, automatically passing the __FILE__ and __LINE__ as extra parameters
#define FATAL(x) { g_Log.Fatal( x, __FILE__, __LINE__ ); }

// Signifies a function is pure virtual and must be defined in a derived class
#define PURE_VIRTUAL 0

// PROTOTYPES ////////////////////////////////////////////////////////////////////////////

/* Takes a fully qualified or relative path string and returns the last portion to yield the fileName.
	E.g. Passing "colours/red/crimson.bmp" would return "crimson.bmp" */
std::string getFilename( const std::string& filePath );

/* Takes a full qualified or relative path string and returns the path without the filename.
	E.g. Passing "colours/red/crimson.bmp" would return "colours/red/" */
std::string getPathname( const std::string& filePath );

// Constants
// Resolution
const int kWidth  = 800;
const int kHeight = 600;

const float kBigNumber				= 3.402823466e+38F;
const float kEpsilon				= 0.001f;

// camera and other stuff
const float kFlySpeed				= 50.0f;

const std::string kSkyboxFileName	= "meshes/skybox.x";

// Takes an angle in degrees and returns the equivalent angle in radians.  returns( fDegrees * ( PI/180 ))
inline float DegreesToRadians( float fDegrees ) { return fDegrees * ( D3DX_PI / 180 ); }

// Takes an angle in radians and returns the equivalent angle in degrees.  returns ( fRadians * ( 180/PI))
inline float RadiansToDegrees( float fRadians ) { return fRadians * ( 180 / D3DX_PI ); }

// Standard template swap routine

template <class Type>
inline void swap(Type &a, Type &b) {
	Type tmp(a);
	a = b;
	b = tmp;
}

// Toymaker.info utility string template.  
// Simply use toString(var) to turn it into a string variable.  Useful for debug output strings.  Type safe.
template <class T>
std::string toString(const T & t)
{
	std::ostringstream oss;		// create a stream
	oss << t;					// insert value to stream
	return oss.str();			// return as a string
}

// Converts to a specified type from string.
// Usage : fromString< type >( outVal, stringToConvert, base type (e.g. std::dec ) )
// e.g. fromString< float >( fPutAnswerHere, "253.43", std::dec ); 
// If the conversion fails, the function returns false. Taken from http://www.codeguru.com/forum/showthread.php?t=231054 

template <class T>
bool fromString( T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&) )
{
	std::istringstream iss(s);
	return !(iss >> f >> t).fail();
}

#endif // _DXAPPDEFINES_H_
