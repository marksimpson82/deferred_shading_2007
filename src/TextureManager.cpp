#include "texturemanager.h"
#include "dxcommon.h"
#include <cassert>
using namespace std;

CTextureManager::CTextureManager( LPDIRECT3DDEVICE9 pDevice )
{
	assert( pDevice && "Invalid Device" );
	m_pDevice = pDevice;
}

CTextureManager::~CTextureManager( )
{
	FreeAllResources();	
}

void CTextureManager::FreeAllResources( )
{
	map< string, LPDIRECT3DTEXTURE9 >::iterator i = m_textureMap.begin();

	while( i != m_textureMap.end() )
	{
		SAFE_RELEASE( i->second );
		i++;
	}
	m_textureMap.clear();
}

HRESULT CTextureManager::LoadTexture( const string& name )
{
	HRESULT hr = S_OK;

	if( GetTexture( name ) != NULL )
		return S_OK;
	
	LPDIRECT3DTEXTURE9 pTex = NULL;
	if( FAILED ( hr = D3DXCreateTextureFromFile( m_pDevice, name.c_str(), &pTex )))
	{
		g_Log.Write( string("Failed to load texture: ") + name, DL_WARN );
		return hr;
	}

	m_textureMap.insert( pair< string, LPDIRECT3DTEXTURE9 >( name, pTex ) );

	return S_OK;
}

LPDIRECT3DTEXTURE9 CTextureManager::GetTexture( const string& name ) const
{
	map< string, LPDIRECT3DTEXTURE9 >::const_iterator i = m_textureMap.find( name );
	if( i == m_textureMap.end() )
		return NULL;
	else
		return i->second;
}

