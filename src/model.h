#ifndef _CMODEL_H_
#define _CMODEL_H_

#include <string>
#include <vector>
#include <d3dx9.h>
#include "bounding.h"

const D3DVERTEXELEMENT9 elementsNormalMapping[] =
{
	{ 0, sizeof( float ) * 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,		0 },
	{ 0, sizeof( float ) * 3, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,		0 },
	{ 0, sizeof( float ) * 5, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,		0 },
	{ 0, sizeof( float ) * 8, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,		0 },
	{ 0, sizeof( float ) * 11, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,		0 },
	D3DDECL_END()
};

class CModel
{
public:
	CModel() : m_pMesh(NULL), m_pVertexDeclaration(NULL), m_bHasNormals(false), m_bHasEmissive(false)
	{
		m_mtrls.clear();
		m_pTextures.clear();
		m_pNormalTextures.clear();		
		m_ObjectSpaceBS.Empty();
		m_BoundingBoxVertices.resize( 8 );
	}

	~CModel();

	// Loads a D3DX .x mesh with the specified file name / path & name.  This is typically invoked by the model manager to stop redundancy	
	HRESULT LoadModel( const std::string& name, LPDIRECT3DDEVICE9 pDevice, bool bSkybox = false, const std::string& skyName = "canyon" );

	// Renders the mesh.  This is largely deprecated and the model should be rendered manually via obtaining pointers to the individual buffers
	HRESULT Render( LPDIRECT3DDEVICE9 pDevice ); // simple render method

	// inlines
	// std::string& GetName( ) { return m_name; }	

//private:
	LPD3DXMESH						m_pMesh;	
	LPDIRECT3DVERTEXDECLARATION9	m_pVertexDeclaration;
		
	bool							m_bHasNormals;
	bool							m_bHasEmissive;
	std::string						m_name;

	// Object space bounding sphere.  This must be transformed to world space by the parenting node for accurate collision detection
	CBoundingSphere					m_ObjectSpaceBS;
	std::vector< D3DXVECTOR3 >		m_BoundingBoxVertices;

	std::vector< D3DMATERIAL9 >			m_mtrls;
	std::vector< LPDIRECT3DTEXTURE9 >	m_pTextures;
	std::vector< LPDIRECT3DTEXTURE9 >	m_pNormalTextures;
};

#endif // _CMODEL_H_