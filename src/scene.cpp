#include "scene.h"
#include "dxcommon.h"
#include <vector>
#include <string>
using namespace std;

ID3DXMesh* CScene::CreateConeMesh( float fOuterAngle, float fMaxRange )
{
	HRESULT hr = S_OK;	

	// check that the data is valid
	if( fMaxRange < 0.1f || fOuterAngle < 0.01  )
	{
		return NULL;        
	}

	// we want to create the cone with the vertex (start of light source) 
	// at the origin and the distance extending down the +z axis.  
	// For a cone with x faces, we need x + 1 vertices with the extra vertex being for the 'tip' or 'point' 
	// Let's go with 16 faces, so 17 vertices.  Worry about the 17th face (cap) later

	// We can form the cone out of a triangle fan, specifying the tip of the cone first, then winding our way around the circle

	float fRadius = tan( fOuterAngle * 0.5f ) * fMaxRange;

	vector< D3DXVECTOR3 > coneVerts;
	coneVerts.clear();	
	

	// the circle will be traced out on the XY plane at a distance of fMaxRange from the origin
	// So each point will be ( X, Y, fMaxRange )
	
	float fTwoPi = D3DX_PI * 2.0f;
	float fIncrement = fTwoPi / kiConeFaces;
	float fParameter = 0.0f;

	// the tip is always (0,0,0) so fill it out manually
	coneVerts.push_back( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ));

	// then fill out the 2nd to the last vertices
	for( UINT i = 1; i < kiConeVertices; i++ )	
	{
		D3DXVECTOR3 newPoint;

		newPoint.x = fRadius * cos( fParameter );
		newPoint.y = fRadius * sin( fParameter );
		newPoint.z = fMaxRange;

		coneVerts.push_back( newPoint );

		// clockwise winding order
		fParameter -= fIncrement;
	}
	
	ID3DXMesh* pMesh = NULL;
	
	// We've now got the vertices to place in the mesh.  Create the Mesh	
	V( D3DXCreateMesh( kiConeTotalFaces, kiConeFaces * 3, D3DXMESH_WRITEONLY , vePos, m_pDevice, &pMesh ));

	posVertex* vert;
	V( pMesh->LockVertexBuffer( 0, (void**)&vert ));

	// copy all unique vertices to the vertex buffer
	for( UINT i = 0; i < kiConeVertices; i++ )
	{
		// first vert in tri
		vert[i] = coneVerts[ i ];
	}
	
	V( pMesh->UnlockVertexBuffer( ) );

	vector< UINT > coneIndices;
	coneIndices.clear();

	// fill out the indices per face
	for( UINT i = 0; i < kiConeFaces - 1; i++ )
	{
		coneIndices.push_back( 0 );
		coneIndices.push_back( i + 1 );
		coneIndices.push_back( i + 2 );
	}

	// fill out the last face manually
	coneIndices.push_back( 0 );
	coneIndices.push_back( kiConeFaces );
	coneIndices.push_back( 1 );

	// now fill out the indices for the cap faces
	for( UINT i = 0; i < kiConeCapFaces; i++ )
	{		
		// The cone cap is aligned with our view vector (+z), so reverse the order of the vertices to generate the correct winding
		coneIndices.push_back( i + 3 );
		coneIndices.push_back( i + 2 );
		coneIndices.push_back( 1 );
	}

	WORD* indices = 0;
    V( pMesh->LockIndexBuffer( 0, (void**)&indices ));

	for( UINT i = 0; i < coneIndices.size(); i++ )
	{
		indices[i] = coneIndices[i];		
	}

	V( pMesh->UnlockIndexBuffer( ));

	// dump to file if we want verbose output
#if defined( DEBUG_VERBOSE )

	for( UINT i = 0; i < coneVerts.size(); i++ )
	{
		g_Log.Write( string("Vector ") + toString( i ) + string( " has values: X: " ) + toString( coneVerts[i].x )
			+ string( ", Y: " ) + toString( coneVerts[i].y ) + string( ", Z: " ) + toString( coneVerts[i].z) );
	}

	for( UINT i = 0; i < coneIndices.size(); i++ )
	{		
		g_Log.Write( string( "Indice #") + toString( i ) + string( " : ") + toString( coneIndices[i] ));
	}

#endif 
	
	//vector< DWORD > adjacencyBuffer( pMesh->GetNumFaces() * 3 );
	//V( pMesh->GenerateAdjacency( 0.0f, &adjacencyBuffer[0] ));
	//V( pMesh->OptimizeInplace( D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_COMPACT | D3DXMESHOPT_VERTEXCACHE, &adjacencyBuffer[0], 0, 0, 0 ));

    coneVerts.clear();
	return pMesh;
}

CScene::CScene()
{
	m_ambientColour = D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f );
	m_fogColour		= D3DXVECTOR3( 1.0f, 1.0f, 1.0f );
	
	m_bFogEnable	= false;
	m_fFogStart		= 128.0f;
	m_fFogEnd		= 4096.0f;

	m_pDevice		= NULL;
	m_pLightBulb	= NULL;
	m_pSpotLight	= NULL;
	m_pSkybox		= NULL;

	m_pLightDirectional.clear();
	m_pLightOmni.clear();
	m_pLightSpot.clear();
	m_pNodes.clear();

	m_dataTypeList.clear();
	m_classList.clear();
    
	m_iNumLights = 0;

	// load up the data types
	m_dataTypeList.insert( pair< string, ClassDataType >( "classname", ST_CLASSNAME ) );
	m_dataTypeList.insert( pair< string, ClassDataType >( "name", ST_NAME ) );
	m_dataTypeList.insert( pair< string, ClassDataType >( "colour", ST_COLOUR ) );
	m_dataTypeList.insert( pair< string, ClassDataType >( "position", ST_POSITION ) );
	m_dataTypeList.insert( pair< string, ClassDataType >( "orientation", ST_ORIENTATION ) );
	m_dataTypeList.insert( pair< string, ClassDataType >( "model", ST_MODEL ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "maxrange", ST_MAXRANGE ) );
	m_dataTypeList.insert( pair< string, ClassDataType >( "innerangle", ST_INNERANGLE ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "outerangle", ST_OUTERANGLE ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "falloff", ST_FALLOFF ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "shadows", ST_SHADOWS ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "fogstart", ST_FOGSTART ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "fogend", ST_FOGEND ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "fogcolour", ST_FOGCOLOUR ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "near", ST_NEAR ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "far", ST_FAR ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "fov", ST_FOV ));
	m_dataTypeList.insert( pair< string, ClassDataType >( "speed", ST_SPEED ));

	// and the entity class table
	m_classList.insert( pair< string, ClassType >( "Skybox", CT_SKYBOX ) );
	m_classList.insert( pair< string, ClassType >( "Fog", CT_FOG ));
	m_classList.insert( pair< string, ClassType >( "Camera", CT_CAMERA ));
	m_classList.insert( pair< string, ClassType >( "LightAmbient", CT_LIGHT_AMBIENT ) );	
	m_classList.insert( pair< string, ClassType >( "CNode", CT_NODE ) );
	m_classList.insert( pair< string, ClassType >( "LightDirectional", CT_LIGHT_DIRECTIONAL ) );
	m_classList.insert( pair< string, ClassType >( "LightOmni", CT_LIGHT_POINT ) );
	m_classList.insert( pair< string, ClassType >( "LightSpot", CT_LIGHT_SPOT ) );	
}

HRESULT CScene::OneTimeInit( LPDIRECT3DDEVICE9 pDevice, const Camera& SceneCamera, const string& fileName )
{
	HRESULT hr = S_OK;
	
	m_pDevice		= pDevice;
	
	if( fileName == "" )
	{
		m_sceneFileName = "scenes/generator.txt";
	}
	else
	{
		m_sceneFileName = fileName;	
	}	

	m_pPosVertDec	= NULL;

	m_Camera = SceneCamera;

	return hr;
}

CScene::~CScene()
{
	m_dataTypeList.clear();
	m_classList.clear();	
}


HRESULT CScene::LoadScene( )
{
	HRESULT hr = S_OK;

	//--------------------------------------------------------------//
	// Load the scene specific stuff
	//--------------------------------------------------------------//

	DWORD timeStart = timeGetTime( );

	ifstream sceneFile;
	sceneFile.open( m_sceneFileName.c_str(), ifstream::in );
	if( ! sceneFile )
	{
		sceneFile.close( );
		g_Log.Write( "Failed to open Scene file. Opening default instead", DL_WARN );

		m_sceneFileName = defaultSceneFileName;
		sceneFile.open( m_sceneFileName.c_str(), ifstream::in );
		if( ! sceneFile )
		{
			FATAL( "Couldn't open default scene file, either." );
			sceneFile.close();
			return E_FAIL;
		}		
	}

	// check to make sure the first line of the file says "Scene", indicating it's a scene file
	string header = "\"Scene\"";
	string headerIn;	
	sceneFile >> headerIn;

	if ( 0 != header.compare( headerIn ) )
	{
		FATAL( "Invalid Scene File Format; File must start with \"Scene\"" );
		sceneFile.close( );
		return E_FAIL;		
	}

	char ch;

	std::list< string >	stringList;
	stringList.clear();

	// parse file.  Every time we find a data type, shove it into a string and push it into a list of strings.  Once we have a full list
	// we can sort it out without messing about with a file any longer

	while( sceneFile )
	{
		sceneFile.get( ch );

		// if we find a left bracket then we need to look for a " character that surrounds entity data
		// e.g. "classname" "CNode"

		if( ch == '{' )
		{
			while( sceneFile )
			{
				sceneFile.get( ch );

				if( ch == '"' )			// we found a new piece of entity data 
				{
					string dataType;
					do
					{
						sceneFile.get( ch );

						// put the data into the string, but ignore new lines and " characters
						if( ch != '\n' && ch != '"' )
						{
							dataType.push_back( ch );
						}

					} while( sceneFile && ch != '"' );		// if we find a " character then it signifies the end of this variable data

					// add the data type to the list.  We will re-assemble it later.
					stringList.push_back( dataType );
				}
			}			
		}
	}	

	sceneFile.close( );

	for( list< string >::iterator i = stringList.begin(); i != stringList.end(); i++ )
	{		
		map< string, ClassDataType >::iterator mapIter = m_dataTypeList.find( (*i) );

		if( mapIter == m_dataTypeList.end() )	
		{
			list< string >::iterator temp = i;
			
			if( ++temp != stringList.end() )	// if it's the end of the file, then just ignore any erroneous data
			{
				// couldn't find a suitable data type			
				g_Log.Write( string("Invalid type found in scene file.  Erroneous data ~near string : ") + toString(*i), DL_WARN );
				FATAL( "Failed to load scene file.  Application terminating" );
				return E_FAIL;
			}			
		}
		else if( mapIter->second == ST_CLASSNAME )
		{
			// found classname label.  The next string in the list should be the class's name (e.g. CNode)
			++i;	// move it on one so we can get the class name and process the next string

			V_RETURN( ExtractEntity( i, stringList ));
		}
	}

	stringList.clear();

	DWORD timeEnd = timeGetTime();
	float timeTaken = (float)(timeEnd - timeStart) * 0.001f;

	g_Log.Write( "Loading resources took " + toString(timeTaken) + " seconds" );

	return S_OK;
}

// not my finest hour, but I needed to hack something together that works.  
// It does work, but it's convoluted, can't recover from bad data and it's also a pain to update...  

// What I really ought to do is create class definition files which are parsed on startup.  
// These files would tell the application the structure of each class, the number of parameters and the type of data expected
// then I could just parse it using a compact function without having to write explicit rules for each type of class.
// If I derived all entities from a base class, I could round it off using a factory or similar so that I 
// didn't even have to create the entities explicitly. e.g. Factory.CreateEntity( ENTITY_TYPE, parameters )

HRESULT CScene::ExtractEntity( list< string >::iterator& i, list< string >& listIn )
{
	HRESULT hr = S_OK;
	// the next data in the list should be the class's name (e.g. node) and then the data that follows it should be used
	// to construct the entity's data fields, then store the entity in the scene class.
	
	map< string, ClassType >::iterator mapIter = m_classList.find( (*i) );
	if( mapIter != m_classList.end() )
	{
		map< string, ClassDataType >::iterator dataTypeIter;

		switch( mapIter->second )
		{
		case CT_SKYBOX:
			{
				g_Log.Write( "Loading Skybox" );

				string skyName;

				dataTypeIter = m_dataTypeList.find( (*++i) );
				if( dataTypeIter != m_dataTypeList.end() )
				{
					if( dataTypeIter->second == ST_NAME )
					{
						//g_Log.Write( "Model found" );

						// next string will be the file name
						++i;

						skyName = *i;
					}
				}

				V( g_ModelManager.LoadSkybox( skyName ) );
				CModel* pNewModel = g_ModelManager.GetModel( kSkyboxFileName );
				CNode* pNode = new CNode( pNewModel );
				m_pSkybox = pNode;
			}		
			break;

		case CT_CAMERA:
			{
				g_Log.Write( "Loading Camera" );

				float fNear = 1.0f;
				float fFar	= 2048.0f;
				float fFov	= 70.0f;
				float fSpeed = 50.0f;

				D3DXVECTOR3 position( 0.0f, 0.0f, 0.0f );
				D3DXVECTOR3 orientation( 0.0f, 0.0f, 0.0f );

				UINT numSearches = 0;
				
				while( i != listIn.end() && numSearches < 6 )
				{
					dataTypeIter = m_dataTypeList.find( (*++i) );
					if( dataTypeIter != m_dataTypeList.end() )
					{
						++i;

						switch( dataTypeIter->second  )
						{
						case ST_POSITION:
							ExtractFloatArrayFromString( (float*)&position, (*i), 3 );								
							break;

						case ST_ORIENTATION:
							ExtractFloatArrayFromString( (float*)&orientation, (*i), 3 );
							break;

						case ST_NEAR:
							if( ! fromString< float >( fNear, (*i), std::dec ) )
							{
								g_Log.Write( "Failed to extract camera near value", DL_WARN );											
							}
							break;

						case ST_FAR:
							if( ! fromString< float >( fFar, (*i), std::dec ) )
							{
								g_Log.Write( "Failed to extract camera far value", DL_WARN );											
							}
							break;

						case ST_FOV:
							if( ! fromString< float >( fFov, (*i), std::dec ) )
							{
								g_Log.Write( "Failed to extract camera FOV value", DL_WARN );											
							}
							break;

						case ST_SPEED:
							if( ! fromString< float >( fSpeed, (*i), std::dec ) )
							{
								g_Log.Write( "Failed to extract camera speed", DL_WARN );											
							}
							break;

						default:
							break;

						}	// end switch

					}	// end while

					numSearches++;

				}	// end while

				m_Camera.SetPosition( position );
				
				D3DXMATRIX rotation;
				D3DXMatrixRotationYawPitchRoll( &rotation, DegreesToRadians( orientation.z ),
					DegreesToRadians(orientation.x), DegreesToRadians(orientation.y) );
				m_Camera.SetRight( D3DXVECTOR3( rotation._11, rotation._12, rotation._13) );
				m_Camera.SetUp( D3DXVECTOR3( rotation._21, rotation._22, rotation._23) );
				m_Camera.SetLook( D3DXVECTOR3( rotation._31, rotation._32, rotation._33) );			

				m_Camera.SetNear( fNear );
				m_Camera.SetFar( fFar );
				m_Camera.SetFOV( DegreesToRadians( fFov ) );				
				m_Camera.SetSpeed( fSpeed );
			}
			break;

		case CT_LIGHT_AMBIENT:
			// next entry should be "colour" and then 4 floats "r g b a"				
			g_Log.Write( "Loading LightAmbient" );			

			dataTypeIter = m_dataTypeList.find( (*++i) );
			if( dataTypeIter != m_dataTypeList.end() )
			{
				if( dataTypeIter->second == ST_COLOUR )
				{
					//g_Log.Write( "Colour found" );
					++i;

					// read in colour
					D3DXVECTOR4 ambientColour;
					ExtractFloatArrayFromString( (float*)&ambientColour, (*i), 4 );						
					m_ambientColour = ambientColour;
				}
			}
			break;

		case CT_FOG:
			{
                g_Log.Write( "Loading Fog" );
				
				UINT numSearches = 0;
				m_bFogEnable = true;

				while( i != listIn.end() && numSearches < 3 )
				{
					dataTypeIter = m_dataTypeList.find( (*++i) );
					if( dataTypeIter != m_dataTypeList.end() )
					{
						++i;

						switch( dataTypeIter->second  )
						{
						case ST_FOGCOLOUR:
							ExtractFloatArrayFromString( (float*)&m_fogColour, (*i), 3 );								
							break;

						case ST_FOGSTART:
							if( ! fromString< float >( m_fFogStart, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract fog start value", DL_WARN );			
								m_bFogEnable = false;
							}
							break;

						case ST_FOGEND:
							if( ! fromString< float >( m_fFogEnd, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract fog end value", DL_WARN );
								m_bFogEnable = false;
							}
							break;					

						default:
							break;

						}	// end switch


					}	// end while

					numSearches++;

				}	// end while
			}
			break;

		case CT_NODE:
			{
				// next entry should be "position" and then 3 floats "x y z"
				g_Log.Write( "Loading Node" );
				D3DXVECTOR3 position( 0.0f, 0.0f, 0.0f );
				D3DXVECTOR3 orientation( 0.0f, 0.0f, 0.0f );
				string fileName;		

				UINT numSearches = 0;

				while( i != listIn.end() && numSearches < 3 )
				{
					dataTypeIter = m_dataTypeList.find( (*++i) );
					if( dataTypeIter != m_dataTypeList.end() )
					{
						++i;

						switch( dataTypeIter->second  )
						{
						case ST_POSITION:
							ExtractFloatArrayFromString( (float*)&position, (*i), 3 );								
							break;

						case ST_ORIENTATION:
							ExtractFloatArrayFromString( (float*)&orientation, (*i), 3 );
							break;

						case ST_MODEL:
							fileName = (*i);
							break;					

						default:
							break;

						}	// end switch


					}	// end while


					numSearches++;

				}	// end while

				V( g_ModelManager.LoadModel( fileName ) );
				CModel* pNewModel = g_ModelManager.GetModel( fileName );
				V( pNewModel->LoadModel( fileName, m_pDevice  ));

				CNode* pNode = new CNode( pNewModel );
				pNode->SetPosition( position );
				pNode->SetOrientation( orientation.x, orientation.y, orientation.z );
				m_pNodes.push_back( pNode );
			}	// end switch
			break;

		case CT_LIGHT_DIRECTIONAL:
			{
				g_Log.Write( "Loading LightDirectional" );
				D3DXVECTOR4 colour( 0.0f, 0.0f, 0.0f, 0.0f );
				D3DXVECTOR3 orientation( 0.0f, 0.0f, 0.0f );
				bool bCastsShadows = false;

				UINT numSearches = 0;

				while( i != listIn.end() && numSearches < 3 )
				{
					dataTypeIter = m_dataTypeList.find( (*++i) );
					if( dataTypeIter != m_dataTypeList.end() )
					{
						++i;

						switch( dataTypeIter->second  )
						{
						case ST_COLOUR:
							ExtractFloatArrayFromString( (float*)&colour, *i, 4 );								
							break;

						case ST_ORIENTATION:
							ExtractFloatArrayFromString( (float*)&orientation, *i, 3 );
							break;		

						case ST_SHADOWS:
							if( ! fromString< bool >( bCastsShadows, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract bool", DL_WARN );
								bCastsShadows = false;
							}
							break;

						default:
							break;

						}	// end switch


					}	// end while

					numSearches++;

				}	// end while

				if( m_iNumLights < kiMaxLights )
				{
					LightDirectional_s* pLight = new LightDirectional_s( colour, DegreesToRadians( orientation.x ), DegreesToRadians( orientation.z ), true, bCastsShadows);
					m_pLightDirectional.push_back( pLight );
					m_iNumLights++;
				}
				
			}
			break;

		case CT_LIGHT_POINT:
			{
				g_Log.Write( "Loading LightOmni" );
				D3DXVECTOR4 colour( 0.0f, 0.0f, 0.0f, 0.0f );
				D3DXVECTOR3 position( 0.0f, 0.0f, 0.0f );
				float fMaxRange = 0.0f;
				bool bCastsShadows = false;

				UINT numSearches = 0;

				while( i != listIn.end() && numSearches < 4 )
				{
					dataTypeIter = m_dataTypeList.find( (*++i) );
					if( dataTypeIter != m_dataTypeList.end() )
					{
						++i;

						switch( dataTypeIter->second  )
						{
						case ST_COLOUR:
							ExtractFloatArrayFromString( (float*)&colour, *i, 4 );								
							break;

						case ST_POSITION:
							ExtractFloatArrayFromString( (float*)&position, *i, 3 );
							break;			

						case ST_MAXRANGE:							
							if( ! fromString< float >( fMaxRange, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract float", DL_WARN );
								fMaxRange = 0.0f;
							}
							break;

						case ST_SHADOWS:
							if( ! fromString< bool >( bCastsShadows, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract float", DL_WARN );
								bCastsShadows = false;
							}
							break;

						default:
							break;

						}	// end switch

					}	// end while

					numSearches++;

				}	// end while

				if( m_iNumLights < kiMaxLights )
				{
					LightOmni_s* pLight = new LightOmni_s( colour, position, fMaxRange, true, bCastsShadows );
					m_pLightOmni.push_back( pLight );
					m_iNumLights++;
				}				
			}
			break;

		case CT_LIGHT_SPOT:
			{
				g_Log.Write( "Loading LightSpot" );
				D3DXVECTOR4 colour( 0.0f, 0.0f, 0.0f, 0.0f );
				D3DXVECTOR3 position( 0.0f, 0.0f, 0.0f );
				D3DXVECTOR3 orientation( 0.0f, 0.0f, 0.0f );

				float fInnerAngle = 15.0f;
				float fOuterAngle = 30.0f;
				float fMaxRange = 0.0f;
				float fFalloff = 1.0f;
				bool bCastsShadows = false;

				UINT numSearches = 0;

				while( i != listIn.end() && numSearches < 8 )
				{
					dataTypeIter = m_dataTypeList.find( (*++i) );
					if( dataTypeIter != m_dataTypeList.end() )
					{
						++i;

						switch( dataTypeIter->second  )
						{
						case ST_COLOUR:
							ExtractFloatArrayFromString( (float*)&colour, *i, 4 );								
							break;

						case ST_POSITION:
							ExtractFloatArrayFromString( (float*)&position, *i, 3 );
							break;			

						case ST_ORIENTATION:
							ExtractFloatArrayFromString( (float*)&orientation, *i, 3 );
							break;

						case ST_MAXRANGE:							
							if( ! fromString< float >( fMaxRange, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract float", DL_WARN );
								fMaxRange = 0.0f;
							}
							break;

						case ST_INNERANGLE:
							if( ! fromString< float>( fInnerAngle, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract float", DL_WARN );
								fInnerAngle = 0.0f;
							}
							break;

						case ST_OUTERANGLE:
							if( ! fromString< float >( fOuterAngle, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract float", DL_WARN );
								fOuterAngle = 0.0f;
							}
							break;

						case ST_FALLOFF:
							if( ! fromString< float >( fFalloff, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract float", DL_WARN );
								fFalloff = 1.0f;
							}							
							break;
						
						case ST_SHADOWS:
							if( ! fromString< bool >( bCastsShadows, *i, std::dec ) )
							{
								g_Log.Write( "Failed to extract bool", DL_WARN );
								bCastsShadows = false;
							}
							break;

						default:
							break;

						}	// end switch

					}	// end while

					numSearches++;

				}	// end while

				if( m_iNumLights < kiMaxLights )
				{
					LightSpot_s* pLight = new LightSpot_s( colour, position, 
						DegreesToRadians( orientation.x ), DegreesToRadians( orientation.z ), fMaxRange, 
						DegreesToRadians( fInnerAngle ), DegreesToRadians( fOuterAngle ), fFalloff, true, bCastsShadows );

					// Create the light's cone mesh
					pLight->m_pConeMesh = CreateConeMesh( pLight->m_fOuterAngle, pLight->m_fMaxRange );

					assert( pLight->m_pConeMesh );

					m_pLightSpot.push_back( pLight );

					m_iNumLights++;
				}				
			
			}	// end spotlight case
			break;		

			default:
				g_Log.Write( "Unknown entity type found.", DL_WARN );
				break;			

		}	// end switch
	}
	else	// it wasn't a class type we found, so maybe we've run into the next "classname" specifier.
	{
		// check to see whether it is "classname".  If it is, then return with this position so we can pick up from here and extract the next ent
		map< string, ClassDataType >::iterator dataTypeIter = m_dataTypeList.find( (*i) );
		if( dataTypeIter != m_dataTypeList.end() )
		{
			if( dataTypeIter->second == ST_CLASSNAME )
			{
				return S_OK;
			}
		}
		else
		{
			// couldn't find a suitable data type			
			g_Log.Write( string("Invalid type found in scene file.  Erroneous data ~near: ") + toString(*i), DL_WARN );
			FATAL( "Failed to load scene file.  Application terminating" );
			return E_FAIL;
		}
	}

	return hr;
}

void CScene::ExtractFloatArrayFromString( float* pVecOut, const std::string& mystring, UINT numVecElements )
{
	assert( numVecElements > 0 && numVecElements < 5 );

	UINT currentComponent = 0;
	string* pComponents = new string[ numVecElements ];

	// parse the entire string until we've got all components of the vector
	for( UINT stringCount = 0; stringCount < mystring.length(); stringCount++ )
	{
		// keep going until we find a space
		if ( mystring[ stringCount ] != ' ' )
		{
			pComponents[ currentComponent ].push_back( mystring[ stringCount ] );
		}
		else	// if we find a space, then we move on to the next component
		{
			if( ++currentComponent > numVecElements - 1 ) 
				break;																// BREAK
		}				
	}

	// turn the individual strings into floats.  If a component cannot be converted, zero it.
	for( UINT blah = 0; blah < numVecElements; blah++ )
	{
		if( ! fromString< float >( pVecOut[blah], pComponents[blah], std::dec ) )
		{
			g_Log.Write( "Failed to extract float element", DL_WARN );
			pVecOut[blah] = 0.0f;
		}
	}

	delete[] pComponents;
}

HRESULT CScene::OnResetDevice( )
{
	HRESULT hr = S_OK;	

	// recreate vertex declarations
	V( m_pDevice->CreateVertexDeclaration( vePos, &m_pPosVertDec ));

	// load models
	V( g_ModelManager.LoadModel( "meshes/lightbulb.x" ));
	V( g_ModelManager.LoadModel( "meshes/spotlight.x" ));
	
	CModel* pLightBulbModel = g_ModelManager.GetModel( "meshes/lightbulb.x" );
	assert( pLightBulbModel );	
	m_pLightBulb = new CNode( pLightBulbModel );

	CModel* pSpotLightModel = g_ModelManager.GetModel( "meshes/spotlight.x" );
	assert( pSpotLightModel );
	m_pSpotLight = new CNode( pSpotLightModel );

	D3DXQUATERNION rot;
	D3DXQuaternionRotationYawPitchRoll( &rot, 0.0f, 0.0f, 0.0f );	

	// reload the scene
	V_RETURN( LoadScene( ) );

	// re-create light meshes

	return hr;
}

HRESULT CScene::OnLostDevice( )
{
	HRESULT hr = S_OK;

	// release vertex declarations
	SAFE_RELEASE( m_pPosVertDec );

	// delete scene resources that aren't used elsewhere
	SAFE_DELETE( m_pLightBulb );
	SAFE_DELETE( m_pSpotLight );
	
	// delete & clear nodes
	list< CNode* >::iterator nodeIter = m_pNodes.begin();
	while( nodeIter != m_pNodes.end() )
	{
		SAFE_DELETE( *nodeIter );
		++nodeIter;
	}
	m_pNodes.clear();

	// delete & clear lights
	list< LightOmni_s* >::iterator pointIter = m_pLightOmni.begin();
	while( pointIter != m_pLightOmni.end() )
	{
		SAFE_DELETE( *pointIter );
		++pointIter;
	}
	m_pLightOmni.clear();

	list< LightDirectional_s* >::iterator directIter = m_pLightDirectional.begin();
	while( directIter != m_pLightDirectional.end() )
	{
		SAFE_DELETE( *directIter );
		++directIter;
	}	
	m_pLightDirectional.clear();

	list< LightSpot_s* >::iterator spotIter = m_pLightSpot.begin();
	while( spotIter != m_pLightSpot.end() )
	{
		LightSpot_s* pLight = *spotIter;

		SAFE_RELEASE( pLight->m_pConeMesh );
		SAFE_DELETE( pLight );		
		
		++spotIter;
	}
	m_pLightSpot.clear();

	m_iNumLights = 0;

	return hr;
}