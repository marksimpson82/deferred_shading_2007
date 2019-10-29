#ifndef _D3DRENDERER_H_
#define _D3DRENDERER_H_

#include "scene.h"
#include "dxcommon.h"
#include <string>
#include <vector>
#include <list>
#include <d3dx9.h>

// forwar decs
class Camera;

const unsigned int kiShadowMapDimensions		= 512;
const unsigned int MAX_LUM_TEX					= 3;
const unsigned int MAX_SAMPLES					= 16;

// ####### NVIDIA STUFF #########
const unsigned int LUMINANCE_DOWNSAMPLE_LOG		= 0;
const unsigned int LUMINANCE_DOWNSAMPLE         = 1;
const unsigned int LUMINANCE_DOWNSAMPLE_EXP		= 2;

// Texture coordinate rectangle
struct CoordRect
{
	float fLeftU,  fTopV;
	float fRightU, fBottomV;
};

// Vertex structure for different postprocessing effects
struct ScreenVertex0
{
	D3DXVECTOR4 p; // position
};

struct ScreenVertex1
{
	D3DXVECTOR4 p; // position
	D3DXVECTOR2 t; // texture coordinate
};

struct ScreenVertex4
{
	D3DXVECTOR4 p;    // position
	D3DXVECTOR4 t[2]; // texture coordinate
};

struct ScreenVertex8
{
	D3DXVECTOR4 p;    // position
	D3DXVECTOR4 t[4]; // texture coordinate
};

const D3DVERTEXELEMENT9 vePosTex[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, 
	{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END()
};

const D3DVERTEXELEMENT9 veFVF0[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		D3DDECL_END()
};

const D3DVERTEXELEMENT9 veFVF1[] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 }, 
	{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END()
};

const D3DVERTEXELEMENT9 veFVF4[] = 
{
	{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 }, 
	{ 0, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },		
	D3DDECL_END()
};

const D3DVERTEXELEMENT9 veFVF8[] = 
{
	{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 }, 
	{ 0, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
	{ 0, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
	{ 0, 64, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 },
	D3DDECL_END()
};

// Vertex  format for position & texture
struct posTexVertex
{
	posTexVertex()
	{

	}
	posTexVertex( const D3DXVECTOR3& position, const D3DXVECTOR2& texCoords )
	{
		pos = position;
		tex = texCoords;
	}

	D3DXVECTOR3 pos;
	D3DXVECTOR2 tex;
};

// RTT structure represents a D3D Texture & its surface.
struct RTT_s
{
	// constructors
	RTT_s( ) :	pTex( NULL ),
				pSurf( NULL ) { }

	RTT_s( LPDIRECT3DTEXTURE9 pRT, LPDIRECT3DSURFACE9 pRTSurf ) :	pTex( pRT ),
																	pSurf( pRTSurf ) {}

	// destructor
	~RTT_s( )
	{
		SAFE_RELEASE( pTex );
		SAFE_RELEASE( pSurf );
	}

	// for statically allocated objects, call this before program ends
	void Release()
	{
		SAFE_RELEASE( pTex ); 
		SAFE_RELEASE( pSurf );
	}

	LPDIRECT3DTEXTURE9	pTex;
	LPDIRECT3DSURFACE9	pSurf;
};

// Render Queue stuff to enable (crude) batching
struct RendererData_s
{
	RendererData_s( ) 	:	
		m_pVB( NULL ),
		m_pIB( NULL ),
		m_pVertexDeclaration( NULL ) {}
		
	RendererData_s( LPD3DXMATRIX pMatWorld, LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB,
        DWORD dwNumBytesPerVertex, LPDIRECT3DVERTEXDECLARATION9 pVertexDeclaration, 
		LPD3DXATTRIBUTERANGE pAttrib, LPDIRECT3DTEXTURE9 pNormalTex, bool bEmissive ) 
		:	m_matWorld(*pMatWorld),
		m_pVB(pVB),
		m_pIB(pIB),
		m_dwNumBytesPerVertex( dwNumBytesPerVertex ),
		m_pVertexDeclaration( pVertexDeclaration ),
		m_attribs( *pAttrib ),
		m_normalMap( pNormalTex ),
		m_bEmissive( bEmissive )	{ }
		
	~RendererData_s() 
	{ 
		//SAFE_RELEASE( m_pVB ); 
		//SAFE_RELEASE( m_pIB );
	}

	D3DXMATRIX						m_matWorld;
	LPDIRECT3DVERTEXBUFFER9			m_pVB;
	LPDIRECT3DINDEXBUFFER9			m_pIB;
	DWORD							m_dwNumBytesPerVertex;
	LPDIRECT3DVERTEXDECLARATION9	m_pVertexDeclaration;
	D3DXATTRIBUTERANGE				m_attribs;
	LPDIRECT3DTEXTURE9				m_normalMap;
	bool							m_bEmissive;
};


struct RendererBucket_s
{
	RendererBucket_s( RendererData_s* pRendererData ) 
	{	
		m_RendererDataList.clear();
		m_RendererDataList.push_back( pRendererData );
	}

	~RendererBucket_s()
	{
		for( std::list< RendererData_s* >::iterator i = m_RendererDataList.begin(); i != m_RendererDataList.end(); i++ )
			SAFE_DELETE( (*i) );
	}
	
	std::list< RendererData_s* >		m_RendererDataList;	
};


// Interface class.  Derive renderers from this and fill out the blanks
class ID3DRenderer
{
public:
	ID3DRenderer( );

	//###################################################
	// VIRTUAL
	//###################################################
	virtual ~ID3DRenderer( );

	// Initialises the Renderer.  
	virtual HRESULT OneTimeInit( int iWidth, int iHeight, 
		HWND hWnd, bool bWindowed, LPDIRECT3DDEVICE9 pDevice )					= PURE_VIRTUAL;

	// Cleans up resources
	virtual HRESULT Cleanup( )													= PURE_VIRTUAL;

	// Renders the Scene
	virtual HRESULT Render( Camera* pCamera, CScene* pScene, float fTimeDelta )	= PURE_VIRTUAL;

	// On Reset Device -- Reset resources
	virtual HRESULT OnResetDevice( )											= PURE_VIRTUAL;

	// Lost Device -- Destroy resources
	virtual HRESULT OnLostDevice( )												= PURE_VIRTUAL;

	/* Breaks the scene up into an ordered list (m_RendererBuckets) of meshes for optimised rendering
		though in this application's case it's more of a naive method rather than anything useful.
		This is just so that we can explore the issues of shadowing techniques breaking batching */
	virtual HRESULT SortScene( Camera* pCamera, CScene* pScene );

	/* Adds a new bucket to the batched map.  The function checks the texture belonging to the mesh and then
		either adds it to a bucket already using that texture, or makes a new bucket using that texture as key */
	virtual HRESULT AddObjectToList( LPDIRECT3DTEXTURE9 pTexture, RendererData_s* pRendererData );

	// Runs any routines that are required at the end of a frame.  This typically involves emptying the render buckets etc.
	virtual HRESULT OnEndOfFrame( );

	// Draws a fullscreen quad that calculates the ambient term for the scene.  Does depth, too.
	virtual HRESULT LightAmbient( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect )								= PURE_VIRTUAL;

	// Additively blends each separate light into the light transport buffer
	virtual HRESULT LightDirectionals( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )	= PURE_VIRTUAL;

	// Additively blends each separate light into the light transport buffer
	virtual HRESULT LightOmnis( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )		= PURE_VIRTUAL;

	// Additively blends each separate light into the light transport buffer
	virtual HRESULT LightSpots( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )			= PURE_VIRTUAL;
	

	//###################################################
	// NON VIRTUAL
	//###################################################

	// Generates the required viewproj matrix for a directional light source's shadow mapping pass
	HRESULT GenerateDirectionalShadowMatrix( D3DXMATRIX* pViewProjOut, LightDirectional_s* pLight, const std::string& sceneName );

	// Generates the required viewproj matrix for a spot light source's shadow mapping pass
	HRESULT GenerateSpotShadowMatrix( D3DXMATRIX* pViewProjOut, LightSpot_s* pLight );

	// Flags entities as being either visible to the scene, or off-screen.  This includes meshes & lights
	HRESULT FindVisibleEntities( Camera* pCamera, CScene* pScene );
	
	// Determine which meshes will cast shadows that fall inside the view frustum (per light).
	// Must be run after FindVisibleEntities & BuildIlluminationSet()
	HRESULT BuildShadowCastingSet( Camera* pCamera, CScene* pScene );

	// Clears the batched render list
	HRESULT ClearRenderBuckets( );

	// Creates a quad given some dimensions & a vertex buffer to fill
	HRESULT CreateQuad( IDirect3DVertexBuffer9* &quadVB, unsigned int width, unsigned int height );

	// Draws the scene's objects
	HRESULT DrawObjects( Camera* pCamera, LPD3DXEFFECT pEffect, bool bDepthOnly = false );

	// Draws a single object.  Useful for objects such as skyboxes, debug models (lights) etc.
	HRESULT DrawSingleObject( Camera* pCamera, CNode* pNode, LPD3DXEFFECT pEffect );

	// Draws a full screen quad.  Note: This function sets its own view/projection matrix and commits changes.
	HRESULT DrawFullScreenQuad( LPD3DXEFFECT pEffect );	

	// Different method of full screen quad rendering purloined from NVIDIA's 2xFP16 HDR rendering demo
	HRESULT DrawNVIDIAQuad( void *scoords, LPDIRECT3DVERTEXDECLARATION9 pVertDec, unsigned int size );

	// Render Skybox
	HRESULT DrawSkybox( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect );

	// Draws a scene file's objects to a shadow map.  In future this will use a shadow casting set rather than a 'scene' for its geometry source
	HRESULT DrawSceneToShadowMap( CScene* pScene, LPD3DXEFFECT pEffect, const D3DXMATRIX& matLightViewProj,
		const UINT uiLightNum, bool bBackFaceCulling = true, bool bBruteForce = false );

	// Copies source to dest
	HRESULT PassThrough( LPD3DXEFFECT pEffect, const std::string& texNameInShader, LPDIRECT3DTEXTURE9 pSourceTex, LPDIRECT3DSURFACE9 pDestSurf );
	
	// DEBUG DRAWING METHODS ///////////////////////////////////////////////

	// Displays the last rendererd shadow map in the viewport
	HRESULT DebugShowShadowMap( LPD3DXEFFECT pEffect );

	// Renders a wireframe mesh bounding scene lights to aid in the visualisation of light boundaries / influence
	HRESULT DebugShowLightMeshWireframes( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect );

	// Renders a wireframe mesh representing a visualisation of each object's bounding sphere
	HRESULT DebugShowBoundingSphereWireframes( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect );

	// Turn HDR on/off
	void ToggleHDR( ) { m_bUseHDR = ! m_bUseHDR; }

	// Returns whether application is using HDR
	bool IsUsingHDR( ) { return m_bUseHDR; }

	// Debug method that displays the last generated shadow map.
	void DebugToggleShadowMapDisplay( ) { m_bDebugDisplayShadowMap = ! m_bDebugDisplayShadowMap; }

	// Toggles whether to display wireframe bounding volumes for lights
	void DebugToggleLightBoundingWireframes( )	{ m_bDebugDisplayLightWireframes = ! m_bDebugDisplayLightWireframes; }

	// Toggles whether to display wireframe bounding volumes for meshes
	void DebugToggleMeshBoundingWireframes( )	{ m_bDebugDisplayMeshBoundingWireframes = ! m_bDebugDisplayMeshBoundingWireframes; }
	
	UINT GetNumVisibleObjects( )	const		{ return m_uiNumVisibleObjects; }
	UINT GetNumVisibleLights( )		const		{ return m_uiNumVisibleLights; }

	// ####### NVIDIA STUFF ###########################################	//
	
	HRESULT CalculateLuminance ( const float frameDelta, LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 channelB = NULL );
	HRESULT ToneMapping        (LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 channelB = NULL);
	HRESULT GlowPass           (LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 channelB = NULL);
	//void DownscaleScene     (LPDIRECT3DSURFACE9 RT0, LPDIRECT3DTEXTURE9 channelA);
	HRESULT DownscaleSceneAniso(LPDIRECT3DSURFACE9 RT0, LPDIRECT3DSURFACE9 RT_int, LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 Tex_int);
	HRESULT LuminancePass      (LPDIRECT3DSURFACE9 RT0, int method, ScreenVertex4 *vertArray, LPDIRECT3DTEXTURE9 channelA,  LPDIRECT3DTEXTURE9 channelB = NULL );
	HRESULT LuminanceAdaptation(LPDIRECT3DSURFACE9 dst, LPDIRECT3DTEXTURE9 src, LPDIRECT3DTEXTURE9 src2, const float frameDelta);
	HRESULT BlurTarget         (LPDIRECT3DSURFACE9 RT0, bool verticalDir, LPDIRECT3DTEXTURE9 channelA);


	// Set up variables required for full screen post-processing quad
	void CalculateOffsetsAndQuadCoords( );
	void ApplyTexelToPixelCorrection(ScreenVertex0 dst[], int srcW, int srcH, CoordRect *coords);
	void ApplyTexelToPixelCorrection(ScreenVertex1 dst[], int srcW, int srcH, CoordRect *coords);
	void ApplyTexelToPixelCorrection(ScreenVertex4 dst[], int srcW, int srcH, CoordRect *coords, D3DXVECTOR2 offsets[]);
	void ApplyTexelToPixelCorrection(ScreenVertex8 dst[], int srcW, int srcH, CoordRect *coords, D3DXVECTOR2 offsets[]);
	void CalculateSampleOffset_4x4Bilinear( DWORD dwWidth, DWORD dwHeight, D3DXVECTOR2 avSampleOffsets[] );
	void CalculateOffsets_GaussianBilinear( float texSize, float *coordOffset, float *gaussWeight, int kernelRadius );
	float* GenerateGaussianWeights( int kernelRadius );
	

protected:
	IDirect3DDevice9*	m_pDevice;
	Camera*				m_pCamera;

	// status vars
	bool			m_bInitialised;
	
	int				m_iWidth;
	int				m_iHeight;
	HWND			m_hWnd;
	bool			m_bWindowed;	
	
	// Other RT stuff	
	
	// shadow mapping stuff
	float			m_fDepthBias;
	float			m_fBiasSlope;

	RTT_s			m_ShadowMapRT;
	RTT_s			m_ShadowMapColourRT;

	D3DXMATRIX		m_matTexScaleBias;

	bool			m_bDebugDisplayShadowMap;
	bool			m_bDebugDisplayLightWireframes;
	bool			m_bDebugDisplayMeshBoundingWireframes;

	std::string		m_SharedEffectName;

	// HDR stuff
	bool			m_bUseHDR;
	
	// batching stuff
	std::map< LPDIRECT3DTEXTURE9, RendererBucket_s* >	m_RendererBuckets;	

	// Sphere mesh for omni point lights
	ID3DXMesh* 						m_pSphereMesh;
	IDirect3DVertexDeclaration9*	m_pSphereMeshVertexDeclaration;
	
	// TEMP #####################################################
	IDirect3DVertexBuffer9*			m_pQuadVB;
	
	D3DXMATRIX	m_matQuad;
	// TEMP #####################################################

	// Vertex decs
	IDirect3DVertexDeclaration9*	m_pQuadVertexDec;
	IDirect3DVertexDeclaration9*	m_pVertexDecFVF0;
	IDirect3DVertexDeclaration9*	m_pVertexDecFVF1;
	IDirect3DVertexDeclaration9*	m_pVertexDecFVF4;
	IDirect3DVertexDeclaration9*	m_pVertexDecFVF8;


	// ########## NVIDIA HDR STUFF ###############
	// Flags	
	bool					m_bUse2xFp;
	float                   m_fExposure;
	int                     m_iPreviousLuminance;

	int                     m_iCroppedWidth;
	int                     m_iCroppedHeight;
	int                     m_iShrinkWidth;
	int                     m_iShrinkHeight;

	// back buffer & z buffer surfaces
	IDirect3DSurface9*	m_pBackBufferSurf;
	IDirect3DSurface9*	m_pZBufferSurf;

	// single render targets for 1xfp16 
	RTT_s					m_fpSceneF; 
	RTT_s					m_fp16ShrinkF;
	RTT_s					m_fp16ShrinkF_1;
	RTT_s					m_fp16ShrinkF_2;

	// Render targets and associated surfaces for 2xFP16 path
	RTT_s					m_SceneA;
	RTT_s					m_SceneB;
    	
	RTT_s					m_fp16ShrinkA;
	RTT_s					m_fp16ShrinkB;

	RTT_s					m_fp16ShrinkA_1;
	RTT_s					m_fp16ShrinkA_2;

	RTT_s					m_fp16Lum[MAX_LUM_TEX];
	RTT_s					m_fp32Lum[3];				// The 1x1 texture to use VTF for the luminance
	
	// Art assets
	//HDRTexture*             m_pHDREnvMap;
	//HDRTexture*             m_pHDRDifMap;
	//Skybox*                 m_pSkybox;
	//Model3D*                m_pModel3D;

	// Global variables for the sampling on downscale and blurring
	float                   m_gaussWeights[MAX_SAMPLES];

	ScreenVertex0           m_fullscreenVTFTonemapPass[3];
	ScreenVertex1           m_fullscreenQuadCoords[3];
	ScreenVertex4           m_downsampleQuadCoords[3];
	ScreenVertex1           m_downsampleAnisoQuadCoords[2][3];
	ScreenVertex4           m_luminanceQuadCoords[MAX_LUM_TEX + 1][3];
	ScreenVertex8           m_blurQuadCoordsV[3];
	ScreenVertex8           m_blurQuadCoordsH[3];

	UINT					m_uiNumVisibleObjects;
	UINT					m_uiNumVisibleLights;
};

class CDummyRenderer : public ID3DRenderer
{
public:
	CDummyRenderer();
	~CDummyRenderer();

	HRESULT OneTimeInit( int iWidth, int iHeight, HWND hWnd, bool bWindowed, LPDIRECT3DDEVICE9 pDevice );
	HRESULT Cleanup( );
	HRESULT Render( Camera* pCamera, CScene* pScene, float fTimeDelta );
	HRESULT DrawObjects( Camera* pCamera, CNode* pNode, LPD3DXEFFECT pEffect );
	HRESULT OnResetDevice( );
	HRESULT OnLostDevice( );

	HRESULT LightAmbient( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect );
	HRESULT LightDirectionals( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow );
	HRESULT LightOmnis( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow );
	HRESULT LightSpots( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow );

private:

};


#endif // _D3DRENDERER_H_