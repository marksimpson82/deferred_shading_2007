#ifndef ISINGLETON_H_
#define ISINGLETON_H_

#include <cassert>

/* Copyright (C) Scott Bilas, 2000. 
* All rights reserved worldwide.
*
* This software is provided "as is" without express or implied
* warranties. You may freely copy and compile this source into
* applications you distribute provided that the copyright text
* below is included in the resulting source code, for example:
* "Portions Copyright (C) Scott Bilas, 2000"
*/

template <typename T> class ISingleton
{
	static T* ms_Singleton;

public:
	ISingleton()
	{
		assert( !ms_Singleton && "Attempt to Create Multiple Singletons.  ISingleton::ISingleton()" );
		//int offset = (int)(T*)1 - (int)(ISingleton <T>*)(T*)1;
		//ms_Singleton = (T*)((int)this + offset);
		ms_Singleton = static_cast <T*> (this);				// new
	}
	virtual ~ISingleton( void )
	{  assert( ms_Singleton && "ISingleton pointer invalid.  ISingleton::~ISingleton()" );  ms_Singleton = 0;  }

	static T& GetSingleton()
	{  assert( ms_Singleton && "ISingleton pointer invalid.  ISingleton::GetSingleton()" );  return ( *ms_Singleton );  }

	static T* GetSingletonPtr()
	{  return ( ms_Singleton );  }
};

template <typename T> T* ISingleton <T>::ms_Singleton = 0;

#endif
