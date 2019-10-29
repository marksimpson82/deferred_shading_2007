#ifndef _TEXTUREMANAGER_H_
#define _TEXTUREMANAGER_H_

#include "singleton.h"
#include <d3dx9.h>
#include <map>
#include <string>

class CTextureManager : public ISingleton < CTextureManager >
{
public:
	CTextureManager( LPDIRECT3DDEVICE9 pDevice );
	~CTextureManager( );

	void FreeAllResources( );

	// Checks to see if texture is loaded.  Returns a pointer to a D3DTEX9 object.  Returns NULL if not found.
	LPDIRECT3DTEXTURE9	GetTexture( const std::string& name ) const;

	// Loads a texture.  Check return value to determine whether load was successful
	// it loade succeeded, then retrieve texture using GetTexture() 
	HRESULT				LoadTexture( const std::string& name );

private:
	LPDIRECT3DDEVICE9 m_pDevice;
	std::map< std::string, LPDIRECT3DTEXTURE9 > m_textureMap;
};

#define g_TextureManager CTextureManager::GetSingleton()

#endif // _TEXTUREMANAGER_H_