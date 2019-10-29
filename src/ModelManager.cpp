#include "modelmanager.h"
#include "model.h"
#include "dxcommon.h"
using namespace std;

CModelManager::CModelManager( LPDIRECT3DDEVICE9 pDevice )
{
	assert( pDevice && "Invalid Device" );
	m_pDevice = pDevice;
}

CModelManager::~CModelManager( )
{
	FreeAllResources();	
}

void CModelManager::FreeAllResources(  )
{
	map< string, CModel* >::iterator i = m_modelMap.begin();

	while( i != m_modelMap.end() )
	{
		SAFE_DELETE( i->second );
		i++;
	}
	m_modelMap.clear();
}

HRESULT CModelManager::LoadModel( const string& name )
{
	HRESULT hr = S_OK;

	// already loaded
	if( GetModel( name ) != NULL )
		return S_OK;

	CModel*	pModel = new CModel;
	V_RETURN( pModel->LoadModel( name, m_pDevice ));	

	m_modelMap.insert( pair< string, CModel* >( name, pModel ) );
	return S_OK;    
}

HRESULT	CModelManager::LoadSkybox( const std::string& skyName )
{
	HRESULT hr = S_OK;

	if( GetModel ( kSkyboxFileName ) != NULL )
		return S_OK;

	CModel* pSkybox = new CModel;
	V_RETURN( pSkybox->LoadModel( kSkyboxFileName, m_pDevice, true, skyName ));

	m_modelMap.insert( pair< string, CModel* >( kSkyboxFileName, pSkybox ) );
	return S_OK;	
}

CModel* CModelManager::GetModel( const string& name ) const
{
	map< string, CModel* >::const_iterator i = m_modelMap.find( name );
	if( i == m_modelMap.end() )
		return NULL;
	return i->second;
}
