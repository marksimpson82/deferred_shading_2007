#ifndef _EFFECTMANAGER_H_
#define _EFFECTMANAGER_H_

#include "singleton.h"
#include <d3dx9.h>
#include <map>
#include <string>

class CEffectManager : public ISingleton< CEffectManager >
{
public:
	CEffectManager( LPDIRECT3DDEVICE9 pDevice );
	~CEffectManager( );
	
	void FreeAllResources( );

	// Loads an effect file and stores it ready for lookup
	HRESULT LoadEffect( const std::string& name );

	// Finds the effect file by name and returns a pointer to it.  If not found, returns NULL
	LPD3DXEFFECT GetEffect( const std::string& name ) const;

private:
	LPDIRECT3DDEVICE9			m_pDevice;
	std::map<std::string, ID3DXEffect*>	m_effectMap; 

};

#define g_EffectManager CEffectManager::GetSingleton()

#endif // _EFFECTMANAGER_H_