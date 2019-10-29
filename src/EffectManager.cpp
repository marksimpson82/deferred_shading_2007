#include "effectmanager.h"
#include "dxcommon.h"

#include <cassert>
using namespace std;

CEffectManager::CEffectManager( LPDIRECT3DDEVICE9 pDevice )
{
	assert( pDevice && "Invalid Device" );
	m_pDevice = pDevice;
}

CEffectManager::~CEffectManager( )
{
	FreeAllResources();
}

void CEffectManager::FreeAllResources( )
{
	map< string, ID3DXEffect* >::iterator i = m_effectMap.begin();
	while( i != m_effectMap.end() )
	{
		SAFE_RELEASE( i->second );
		i++;
	}
	m_effectMap.clear();
}

HRESULT CEffectManager::LoadEffect( const string& name )
{
	HRESULT hr = S_OK;

	// check to see whether it's already loaded
	if( GetEffect( name ) != NULL )
		return S_OK;
    	
	// it ain't there, so load it
	ID3DXEffect* pEffect = NULL;
	ID3DXBuffer* pErrors = NULL;

#if defined( DEBUG_PS )
	V_RETURN( D3DXCreateEffectFromFile( m_pDevice, name.c_str(), 0, 0, D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION, 0, &pEffect, &pErrors ));
#else
	V_RETURN( D3DXCreateEffectFromFile( m_pDevice, name.c_str(), 0, 0, 0, 0, &pEffect, &pErrors ));
#endif

	m_effectMap.insert( pair< string, ID3DXEffect* >( name, pEffect ) );
	return hr;
}

LPD3DXEFFECT CEffectManager::GetEffect( const string& name ) const
{
	map< string, ID3DXEffect* >::const_iterator i = m_effectMap.find( name );
	if( i == m_effectMap.end() )
		return NULL;
	return i->second;
}
