#ifndef _SCENE_H_
#define _SCENE_H_

#include "dxcommon.h"
#include "spatial.h"
#include "camera.h"
#include <d3dx9.h>
#include <list>
#include <string>

const std::string defaultSceneFileName = "scenes/generator.txt";

const UINT kiConeFaces		= 16;				// faces around the cone, doesn't include the cap
const UINT kiConeVertices	= kiConeFaces + 1;	// all vertices
const UINT kiConeCapFaces	= kiConeFaces - 2;	// faces on the cap of the cone.  For n faces this is n - 2 cap faces
const UINT kiConeTotalFaces = kiConeFaces + kiConeCapFaces;

const UINT kiMaxLights		= 16;

inline UINT GetLightFlag( UINT x ) { return 1 << x; }

const D3DVERTEXELEMENT9 vePos[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, 	
	D3DDECL_END()
};

struct posVertex
{
	posVertex()
	{

	}

	posVertex( const D3DXVECTOR3& position )
		: pos( position ) {}

	D3DXVECTOR3 pos;	
};

//--------------------------------------------------------------//
// Light structures 
//--------------------------------------------------------------//

struct LightOmni_s
{	
	LightOmni_s() 
		:	m_colour( D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f ) ),
			m_position( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) ),
			m_fMaxRange( 0.0f ),			
			m_bActive( false ),
			m_bCastsShadows( false ),
			m_bVisible( false ) {}

	LightOmni_s( D3DXVECTOR4& colour, D3DXVECTOR3& position, float maxrange, bool bActive = true, bool bCastsShadows = true ) 
		:	m_colour(colour), 
			m_position(position),
			m_fMaxRange(maxrange),
			m_bActive(bActive),
			m_bCastsShadows(bCastsShadows),
			m_bVisible( false ) {}

	~LightOmni_s() {}
	
	D3DXVECTOR4 m_colour;
	D3DXVECTOR3 m_position;
	float		m_fMaxRange;
	bool		m_bActive;
	bool		m_bCastsShadows;	

	// house keeping
	bool		m_bVisible;
};

struct LightDirectional_s
{
	LightDirectional_s( )
		:	m_colour( D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f )),
			m_fPitch( -90.0f ),
			m_fYaw( 0.0f ),
			m_bActive( false ),
			m_bCastsShadows( false ),
			m_bVisible( false ) {}

	LightDirectional_s( D3DXVECTOR4& colour, float fPitch, float fYaw, bool bActive = true, bool bCastsShadows = true ) 
		:	m_colour( colour ),
			m_fPitch( fPitch ),
			m_fYaw( fYaw ),
			m_bActive( bActive ),
			m_bCastsShadows( bCastsShadows ),
			m_bVisible( false ) {}

	~LightDirectional_s() {}

	D3DXVECTOR4 m_colour;
	float		m_fPitch;		// RADIANS
	float		m_fYaw;			// RADIANS
	bool		m_bActive;
	bool		m_bCastsShadows;

	// house keeping
	bool		m_bVisible;
};

struct LightSpot_s
{
	LightSpot_s( ) 
		:	m_colour( D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f ) ),
			m_position( D3DXVECTOR3( 0.0f, 0.0f, 0.0f )),
			m_fPitch( 0.0f ),
			m_fYaw( 0.0f ),
			m_fMaxRange( 8.0f ),
			m_fInnerAngle( 0.0f ),
			m_fOuterAngle( 0.0f ),
			m_fFalloff( 1.0f ),
			m_bActive( false ),
			m_bCastsShadows( false ),
			m_bVisible( false ),
			m_pConeMesh( NULL ) {}

	LightSpot_s( D3DXVECTOR4& colour, D3DXVECTOR3& position, float fPitch, float fYaw, float fMaxRange,
					float fInnerAngle, float fOuterAngle, float fFalloff, bool bActive = true, bool bCastsShadows = true ) 

		:	m_colour( colour ), 
			m_position( position ),
			m_fPitch( fPitch ),
			m_fYaw( fYaw ),
			m_fMaxRange( fMaxRange ),
			m_fInnerAngle( fInnerAngle ),
			m_fOuterAngle( fOuterAngle ),
			m_fFalloff( fFalloff ),
			m_bActive( bActive ),
			m_bCastsShadows( bCastsShadows ),
			m_bVisible( false ),
			m_pConeMesh( NULL )	{ }

	~LightSpot_s( ) {}	
	
	D3DXVECTOR4 m_colour;
	D3DXVECTOR3 m_position;
	float		m_fPitch;		// RADIANS
	float		m_fYaw;			// RADIANS
	float		m_fMaxRange;
	float		m_fInnerAngle;	// Full angle RADIANS -- not the half angle.  Remember to mult by 0.5 before doing cone calcs in shader
	float		m_fOuterAngle;	// Full angle RADIANS -- not the half angle.  Remember to mult by 0.5 before doing cone calcs in shader	
	float		m_fFalloff;		// 1.0f = standard
	bool		m_bActive;
	bool		m_bCastsShadows;

	ID3DXMesh*	m_pConeMesh;

	// house keeping
	bool		m_bVisible;
};

//--------------------------------------------------------------//
// Scene class
//--------------------------------------------------------------//

class CScene
{
public:
	CScene();
	~CScene();

	enum ClassDataType
	{
		ST_CLASSNAME = 0,
		ST_NAME,
		ST_COLOUR,
		ST_POSITION,
		ST_MODEL,
		ST_ORIENTATION,		// EULER ANGLES.  Pitch Roll Yaw
		ST_MAXRANGE,
		ST_INNERANGLE,
		ST_OUTERANGLE,
		ST_FALLOFF,
		ST_SHADOWS,
		ST_FOGSTART,
		ST_FOGEND,
		ST_FOGCOLOUR,
		ST_NEAR,
		ST_FAR,
		ST_FOV,
		ST_SPEED,
		ST_TOTAL,
	};

	enum ClassType
	{
		CT_NODE = 0,
		CT_SKYBOX,
		CT_FOG,
		CT_CAMERA,
		CT_LIGHT_AMBIENT,
		CT_LIGHT_DIRECTIONAL,
		CT_LIGHT_POINT,
		CT_LIGHT_SPOT,		
		CT_TOTAL,
	};

	typedef std::list< std::string > stringList;
	typedef std::list< std::string >::iterator stringListIter;

	// One time initialisation function
	HRESULT OneTimeInit( LPDIRECT3DDEVICE9 pDevice, const Camera& SceneCamera, const std::string& fileName = "scenes/generator.txt" );	

	// Load the scene from a file.  If no filename is specified, the default scene "scenes/default.txt" is loaded
	HRESULT LoadScene( );
	
	// Extracts the next entity from the list of strings at the position passed.  Returns the iterator pointing at the next location
	HRESULT ExtractEntity( stringListIter& i, stringList& listIn );

	// Takes a string featuring multiple components separated by spaces (e.g. "25.0 0.5 0.43") and separates into multiple variables
	void ExtractFloatArrayFromString( float* pVecOut, const std::string& mystring, UINT numVecElements );

	ID3DXMesh*  CreateConeMesh( float fOuterAngle, float fMaxRange );

	// Create all D3D resources / resources that rely on D3D
	HRESULT OnResetDevice( );

	// When we lose the device, nuke all of the resources
	HRESULT OnLostDevice( );	

	D3DXVECTOR4							m_ambientColour;
	bool								m_bFogEnable;
	D3DXVECTOR3							m_fogColour;
	float								m_fFogStart;
	float								m_fFogEnd;	

//private:

	Camera								m_Camera;

	IDirect3DDevice9*					m_pDevice;

	CNode*								m_pLightBulb;
	CNode*								m_pSpotLight;
	CNode*								m_pSkybox;
	std::list< CNode* >					m_pNodes;

	std::list< LightOmni_s* >			m_pLightOmni;
	std::list< LightDirectional_s* >	m_pLightDirectional;
	std::list< LightSpot_s* >			m_pLightSpot;

	UINT								m_iNumLights;

	// housekeeping stuff
	std::map< std::string, ClassDataType >	m_dataTypeList;
	std::map< std::string, ClassType >		m_classList;

	std::string							m_sceneFileName;

	IDirect3DVertexDeclaration9*		m_pPosVertDec;
};


#endif // _SCENE_H_