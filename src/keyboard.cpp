#include "keyboard.h"

CKeyboard::CKeyboard( )
{
	for( unsigned int i = 0; i < kNumKeys; i++ )
	{
		m_bKeysCurrentlyPressed[ i ] = false;
		m_bKeysPreviouslyPressed[ i ] = false;
	}
}

CKeyboard::~CKeyboard( )
{

}

void CKeyboard::Update( )
{
	for( unsigned int i = 0; i < kNumKeys; i++ )
	{
		m_bKeysPreviouslyPressed[ i ] = m_bKeysCurrentlyPressed[ i ];	
	}
}