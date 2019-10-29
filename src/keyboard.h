#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

const unsigned int kNumKeys = 145;

class CKeyboard
{
public:
	CKeyboard( );
	~CKeyboard( );

	// Updates the keyboard states.  Flags the keys currently pressed as being previously pressed
	void Update( );

	// INLINES ///////////////////////////////////////////////////////////////////////////////

	// Determines whether a specific key is held.  Returns true for held, false for not held
	bool KeyDown( int key )			const	{ return m_bKeysCurrentlyPressed[ key ]; }

	// Determines whether a specific key has just been pressed.  Returns true for yes, false for no.
	bool JustPressed( int key )		const	{ return m_bKeysCurrentlyPressed[ key ] & !m_bKeysPreviouslyPressed[ key ]; }

	// Determines whether a specific key has just been released.  Returns true for yes, false for no.
	bool JustReleased( int key )	const	{ return m_bKeysPreviouslyPressed[ key ] & !m_bKeysCurrentlyPressed[ key ]; } 

	// Every time we detect a key press in the windows proc, this function should be called with the key as a parameter
	void SetKeyPressed( int key )	{ m_bKeysCurrentlyPressed[ key ] = true; }

	// Every time we detect a key release in the windows proc, this function should be called with the key as a parameter
	void SetKeyReleased( int key )	{ m_bKeysCurrentlyPressed[ key ] = false; }
	
private:

	bool m_bKeysCurrentlyPressed[ kNumKeys ];
	bool m_bKeysPreviouslyPressed[ kNumKeys ]; // last update


};

#endif // _KEYBOARD_H_