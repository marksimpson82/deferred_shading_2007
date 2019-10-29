#ifndef _MESHMANAGER_H_
#define _MESHMANAGER_H_

#include "singleton.h"
#include <string>
#include <map>
#include <d3dx9.h>

// forward decs
class CModel;

class CModelManager : public ISingleton < CModelManager >
{
public:
	CModelManager( LPDIRECT3DDEVICE9 pDevice );
	~CModelManager( );

	void	FreeAllResources( );
	// Attempts to retrieve the model from the map of already-loaded models.  If found, returns a pointer.  If not, returns NULL
	CModel*		GetModel( const std::string& name ) const;

	// Attempts to load a model specified by the user.  If the model isn't found in the existing
	// map of loaded models the function fails and returns an appropriate HRESULT.  
	// Users should test the HRESULT and, if it signifies SUCCESS
	// then call GetModel with the same parameter to retrieve the (now loaded) model.  
	HRESULT		LoadModel( const std::string& name );

	// loads a skybox.  This has a separate function because it means I can reuse one skybox mesh while swapping textures
	HRESULT		LoadSkybox( const std::string& skyName );

private:
	LPDIRECT3DDEVICE9 m_pDevice;
	std::map< std::string, CModel* > m_modelMap;
	
};

#define g_ModelManager CModelManager::GetSingleton()

#endif // _MESHMANAGER_H_