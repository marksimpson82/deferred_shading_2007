#include "D3DForwardRenderer.h"


#include "dxcommon.h"
using namespace std;

CD3DForwardRenderer::CD3DForwardRenderer( )
{
}

CD3DForwardRenderer::~CD3DForwardRenderer( )
{
}

HRESULT CD3DForwardRenderer::OneTimeInit( int iWidth, int iHeight, HWND hWnd, bool bWindowed, LPDIRECT3DDEVICE9 pDevice )
{
	HRESULT hr = S_OK;

	V_RETURN( ID3DRenderer::OneTimeInit( iWidth, iHeight, hWnd, bWindowed, pDevice ) );
	m_ForwardShadingEffectName = "effects/Textured_Phong.fx";
	m_bInitialised			= true;

	return S_OK;
}

HRESULT CD3DForwardRenderer::Cleanup( )
{
	HRESULT hr = S_OK;
	
	V( OnLostDevice() );
	V( ID3DRenderer::Cleanup() );

	return hr;
}

HRESULT CD3DForwardRenderer::Render( Camera* pCamera, CScene* pScene, float fTimeDelta )
{		
	HRESULT hr = S_OK;
	m_pCamera = pCamera;

	// EFFECT STUFF
	LPD3DXEFFECT pForward = g_EffectManager.GetEffect( m_ForwardShadingEffectName );
	LPD3DXEFFECT pShared = g_EffectManager.GetEffect( m_SharedEffectName );

	if( ! pForward || ! pShared )
	{
		assert( pForward && "Failed to load Effect" );
		string output = string("Failed to load Effect");
		FATAL( output );
		return E_FAIL;
	}		
	
	if( m_bUse2xFp )
	{
		V( m_pDevice->SetRenderTarget( 0, m_SceneA.pSurf ));
		V( m_pDevice->SetRenderTarget( 1, m_SceneB.pSurf ));
	}
	else
	{
		V( m_pDevice->SetRenderTarget( 0, m_fpSceneF.pSurf ));
	}

	V( LightAmbient( pCamera, pScene, pForward ));
	V( LightDirectionals( pCamera, pScene, pForward, pShared ));
	V( LightOmnis( pCamera, pScene, pForward, pShared ));
	V( LightSpots( pCamera, pScene, pForward, pShared ));	
	V( DrawSkybox( pCamera, pScene, pForward ));

	//--------------------------------------------------------------//
	// Copy scene to the framebuffer
	//--------------------------------------------------------------//

	if( m_bDebugDisplayMeshBoundingWireframes )
	{
		V( DebugShowBoundingSphereWireframes( pCamera, pScene, pShared ));
	}

	if( m_bDebugDisplayLightWireframes )
	{
		V( DebugShowLightMeshWireframes( pCamera, pScene, pShared ));
	}		

	// Calculate luminance
	if( m_bUseHDR )
	{
		V( m_pDevice->SetRenderTarget( 1, NULL ));

		if( m_bUse2xFp )
		{
			V( CalculateLuminance( fTimeDelta, m_SceneA.pTex, m_SceneB.pTex ));
			V( GlowPass( m_SceneA.pTex, m_SceneB.pTex ));
			V( ToneMapping( m_SceneA.pTex, m_SceneB.pTex ));		
		}
		else
		{
			V( CalculateLuminance( fTimeDelta, m_fpSceneF.pTex, NULL ));
			V( GlowPass( m_fpSceneF.pTex, NULL ));
			V( ToneMapping( m_fpSceneF.pTex, NULL ));		
		}
	}
	else
	{
		V( m_pDevice->SetRenderTarget( 0, m_pBackBufferSurf ));
		V( pShared->SetTechnique( "PassThrough" ));
		V( pShared->SetTexture( "TextureA", m_fpSceneF.pTex ));
		UINT uiNumPasses;
		V( pShared->Begin( &uiNumPasses, 0 ));
		V( pShared->BeginPass( 0 ));
		
		V( pShared->CommitChanges( ));
		V( DrawNVIDIAQuad( m_fullscreenQuadCoords, m_pVertexDecFVF1, sizeof( ScreenVertex1 ) ));
		
		V( pShared->EndPass( ));
		V( pShared->End( ));

		//	V( PassThrough( pShared, "LightTransport", m_LTSurfaces.pTex, m_pBackBufferSurf ));
	}

	if( m_bDebugDisplayShadowMap )
		V( DebugShowShadowMap( pShared ));		

	V( m_pDevice->SetRenderTarget( 0, m_pBackBufferSurf ));
	V( m_pDevice->SetDepthStencilSurface( m_pZBufferSurf ));
    	
	return S_OK;
}

HRESULT CD3DForwardRenderer::BuildIlluminationSet( Camera* pCamera, CScene* pScene )
{
	HRESULT hr = S_OK;

	UINT uiLightNum = (UINT)pScene->m_pLightDirectional.size();

	for( list< LightOmni_s* >::iterator i = pScene->m_pLightOmni.begin(); i != pScene->m_pLightOmni.end(); i++, uiLightNum++ )
	{
		LightOmni_s* pLight = *i;

		// if no part of the light is inside the frustum, don't bother working out any lighting influences as it won't illuminate anything visible
		if( ! pLight->m_bVisible )
			continue;

		CBoundingSphere LightBS;
		LightBS.m_Position = pLight->m_position;
		LightBS.m_radius		= pLight->m_fMaxRange;

		// we need to determine whether the light touches anything in the scene.  If it does, flag the mesh as being lit by the light
		for( list< CNode* >::iterator nodeIter = pScene->m_pNodes.begin(); nodeIter != pScene->m_pNodes.end(); nodeIter++ )
		{	
			CNode* pNode = *nodeIter;

			// if the mesh isn't visible, we don't care about any flags as it won't get rendered regardless
			if( ! pNode->m_bVisible )
				continue;            

			// if the light is on screen AND it intersects a visible mesh's bounding sphere, bitwise OR the light to the light flags
			//if( IntersectBoundingSpheres( LightBS, pNode->m_BoundingSphere ))
			if( AABBSphereIntersection( pNode->m_BoundingBox, LightBS ))
			{
				pNode->m_uiIlluminationFlags |= GetLightFlag( uiLightNum );
			}		

		}	// end for

	}

	for( list< LightSpot_s* >::iterator i = pScene->m_pLightSpot.begin(); i != pScene->m_pLightSpot.end(); i++, uiLightNum++ )
	{
		LightSpot_s* pLight = *i;

		if( ! pLight->m_bVisible )
			continue;

		D3DXMATRIX matRot;
		D3DXMatrixRotationYawPitchRoll( &matRot, pLight->m_fYaw, pLight->m_fPitch, 0.0f );
		D3DXVECTOR3 vecLook( matRot(2,0), matRot(2,1), matRot(2,2) );
		D3DXVec3Normalize( &vecLook, &vecLook );

		D3DXVECTOR3 vecOffset = vecLook * pLight->m_fMaxRange * 0.5f;
		float fAdjustedRadius = pLight->m_fMaxRange * 0.55f;

		CBoundingSphere LightBS;
		LightBS.m_Position = pLight->m_position + vecOffset;
		LightBS.m_radius = fAdjustedRadius;

		for( list< CNode* >::iterator nodeIter = pScene->m_pNodes.begin(); nodeIter != pScene->m_pNodes.end(); nodeIter++ )
		{	
			CNode* pNode = *nodeIter;

			// if the mesh isn't visible, we don't care about any flags as it won't get rendered regardless
			if( ! pNode->m_bVisible )
				continue;            

			// if the light is on screen AND it intersects a mesh's bounding sphere, bitwise OR the light to the light flags
			//if( IntersectBoundingSpheres( LightBS, pNode->m_BoundingSphere ))
			if( AABBSphereIntersection( pNode->m_BoundingBox, LightBS ))
			{
				pNode->m_uiIlluminationFlags |= GetLightFlag( uiLightNum );
			}		

		}	// end for

	}	// end for

	return hr;
}

HRESULT CD3DForwardRenderer::LightAmbient( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hDepthAmbient = pEffect->GetTechniqueByName( "DepthAmbient" );
	V( pEffect->SetTechnique( hDepthAmbient ));

	// render
	UINT uiNumPasses;

	V( pEffect->SetVector( "g_fvAmbientColour", &pScene->m_ambientColour ));
	V( pEffect->SetBool( "g_bUseHDR", m_bUseHDR ));

	V( pEffect->Begin( &uiNumPasses, 0 ) );
	V( pEffect->BeginPass( 0 ));

	V( DrawObjects( pCamera, pEffect ));
	
	V( pEffect->End() );
	V( pEffect->EndPass( ));

	return hr;
}

// Additively blends each separate light into the light transport buffer
HRESULT CD3DForwardRenderer::LightDirectionals( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pForward, LPD3DXEFFECT pShadow )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hPhongDirectionalNormalMapping			= pForward->GetTechniqueByName( "PhongDirectionalNormalMapping" );
	D3DXHANDLE hPhongDirectionalNormalMappingShadowed	= pForward->GetTechniqueByName( "PhongDirectionalNormalMappingShadowed" );	

	//--------------------------------------------------------------//
	// Set up variables & effect params
	//--------------------------------------------------------------//
	D3DXMATRIX matView, matProj;

	m_pCamera->GetViewMatrix( &matView );
	m_pCamera->GetProjMatrix( &matProj );

	D3DXHANDLE hView = pForward->GetParameterBySemantic( NULL, "View" );
	V( pForward->SetMatrix( hView, &matView ));

	// get handles to variables that will be constantly changed (faster than retrieving via name)
	D3DXHANDLE hWVP				= pForward->GetParameterBySemantic( NULL, "WorldViewProjection" );
	D3DXHANDLE hLightColour		= pForward->GetParameterByName( NULL, "g_fvLightColour" );
	
	// object space stuff is required for tangent space normal mapping
	D3DXHANDLE hObjectSpaceEyePosition		= pForward->GetParameterByName( NULL, "g_fvObjectSpaceEyePosition" );
	D3DXHANDLE hObjectSpaceLightDirection	= pForward->GetParameterByName( NULL, "g_fvObjectSpaceLightDirection" );
	
	//--------------------------------------------------------------//
	// Directional Light Pass
	//--------------------------------------------------------------//

	UINT uiLightNum = 0;
	D3DXHANDLE hMatObjectToLightProjSpace	= pForward->GetParameterByName( NULL, "matObjectToLightProjSpace" );
	D3DXHANDLE hShadowMap					= pForward->GetParameterByName( NULL, "shadowMapRT_Tex" );

	for( list< LightDirectional_s* >::iterator lightIter = pScene->m_pLightDirectional.begin();
		lightIter != pScene->m_pLightDirectional.end(); lightIter++, uiLightNum++ )
	{
		LightDirectional_s* pLight = (*lightIter);

		if( ! pLight->m_bActive )
			continue;

		// set light properties here
		D3DXMATRIX matLightRot;
		D3DXMatrixRotationYawPitchRoll( &matLightRot, pLight->m_fYaw, pLight->m_fPitch, 0.0f );

		D3DXVECTOR3 worldSpaceLightDirection = D3DXVECTOR3( matLightRot._31, matLightRot._32, matLightRot._33 ); 
		D3DXMATRIX matLightViewProj;

		if( pLight->m_bCastsShadows )
		{
			V( GenerateDirectionalShadowMatrix( &matLightViewProj, pLight, pScene->m_sceneFileName ));

			// render scene to shadow map RT / Depth surface.  The DrawSceneToShadowMap function takes care of setting
			// the appropriate render target / surfs etc and also restoring the light accum buffer & regular depth buffer.
			if( m_bUse2xFp )
			{
				V( m_pDevice->SetRenderTarget( 1, NULL ));
			}

			// naive directional light shadow mapping.  Bruteforce it by rendering entire scene into shadow map
			V( DrawSceneToShadowMap( pScene, pShadow, matLightViewProj, uiLightNum, false, true ));

			if( m_bUse2xFp )
			{
				V( m_pDevice->SetRenderTarget( 1, m_SceneB.pSurf ));
			}		

			// set the results of the shadow map rendering as our shadow map texture
			V( pForward->SetTechnique( hPhongDirectionalNormalMappingShadowed ));
			V( pForward->SetTexture( hShadowMap, m_ShadowMapRT.pTex ));			
		}
		else
		{
			V( pForward->SetTechnique( hPhongDirectionalNormalMapping ));
		}

		UINT uiNumPasses; 	
		V( pForward->SetVector( hLightColour, &pLight->m_colour ));
		
		V( pForward->Begin( &uiNumPasses, 0 ));
        for( UINT pass = 0; pass < uiNumPasses; pass++ )
		{
			V( pForward->BeginPass( pass ));

			for( list< CNode* >::iterator i = pScene->m_pNodes.begin(); i != pScene->m_pNodes.end(); i++ )
			{
				CNode* pNode = *i;

				if( ! pNode->m_bVisible )
					continue;

				D3DXMATRIX matWorld;
				pNode->GetWorldMatrix( &matWorld );
				//pForward->SetMatrix( hWorld, (*i)->GetWorldMatrix( &matWorld ) );

				//			D3DXMATRIX matWorldView = matWorld * matView;
				//			pForward->SetMatrix( hWorldView, &matWorldView );

				// new stuff for normal mapping
				D3DXMATRIX matWorldInverse;
				D3DXMatrixInverse( &matWorldInverse, NULL, &matWorld );

				D3DXVECTOR3 objectSpaceLightDir;
				D3DXVec3TransformNormal( &objectSpaceLightDir, &worldSpaceLightDirection, &matWorldInverse );
				V( pForward->SetFloatArray( hObjectSpaceLightDirection, (float*)&objectSpaceLightDir, 3 ));

				D3DXVECTOR3 objectSpaceEyePosition;
				D3DXVECTOR3 eyePos = pCamera->GetPosition();
				D3DXVec3TransformCoord( &objectSpaceEyePosition, &eyePos, &matWorldInverse );
				V( pForward->SetFloatArray( hObjectSpaceEyePosition, (float*)&objectSpaceEyePosition, 3 ));

				D3DXMATRIX matWVP = matWorld * matView * matProj;
				V( pForward->SetMatrix( hWVP, &matWVP ));

				if( pLight->m_bCastsShadows )
				{
					// now set the matrix that allows the lighting shader to move from object to lightViewProj space 
					D3DXMATRIX matObjectToLightProjSpace;
					matObjectToLightProjSpace = matWorld * matLightViewProj * m_matTexScaleBias;
					V( pForward->SetMatrix( hMatObjectToLightProjSpace, &matObjectToLightProjSpace ));		
				}

				V( DrawSingleObject( pCamera, pNode, pForward  ));

			}	// end node for loop

			V( pForward->EndPass() );

		}	// end render pass for loop

		V( pForward->End() );	

	}	// end light for loop
	
	return hr;
}

// Additively blends each separate light into the light transport buffer
HRESULT CD3DForwardRenderer::LightOmnis( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pForward, LPD3DXEFFECT pShadow )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hPhongPointLightNormalMapping	= pForward->GetTechniqueByName( "PhongPointLightNormalMapping" );
	//D3DXHANDLE hPhongPointLightNormalMappingShadowed = pForward->GetTechniqueByName( "PhongPointLightNormalMappingShadowed" );

	//--------------------------------------------------------------//
	// Set up variables & effect params
	//--------------------------------------------------------------//
	D3DXMATRIX matView, matProj;

	m_pCamera->GetViewMatrix( &matView );
	m_pCamera->GetProjMatrix( &matProj );

	D3DXHANDLE hView = pForward->GetParameterBySemantic( NULL, "View" );
	V( pForward->SetMatrix( hView, &matView ));

	// get handles to variables that will be constantly changed (faster than retrieving via name)
	D3DXHANDLE hWVP				= pForward->GetParameterBySemantic( NULL, "WorldViewProjection" );

	D3DXHANDLE hLightColour		= pForward->GetParameterByName( NULL, "g_fvLightColour" );
	D3DXHANDLE hLightMaxRange	= pForward->GetParameterByName( NULL, "g_fLightMaxRange" );

	// object space stuff is required for tangent space normal mapping
	D3DXHANDLE hObjectSpaceEyePosition		= pForward->GetParameterByName( NULL, "g_fvObjectSpaceEyePosition" );	
	D3DXHANDLE hObjectSpaceLightPosition	= pForward->GetParameterByName( NULL, "g_fvObjectSpaceLightPosition" );

	UINT uiLightNum = (UINT)pScene->m_pLightDirectional.size();
	UINT uiNumPasses;	
	
	for( list< LightOmni_s* >::iterator lightIter = pScene->m_pLightOmni.begin();
		lightIter != pScene->m_pLightOmni.end(); lightIter++, uiLightNum++ )
	{
		LightOmni_s* pLight = (*lightIter);

		if( ! pLight->m_bActive || ! pLight->m_bVisible )
			continue;		

		if( pLight->m_bCastsShadows )
		{
			// shadow stuff goes here
			V( pForward->SetTechnique( hPhongPointLightNormalMapping /*Shadowed*/ ) )
		}
		else
		{
			V( pForward->SetTechnique( hPhongPointLightNormalMapping ) )
		}

		D3DXVECTOR3 worldSpaceLightPosition = pLight->m_position;

		V( pForward->SetVector( hLightColour, &pLight->m_colour ));
		V( pForward->SetFloat( hLightMaxRange, pLight->m_fMaxRange ));
		
		V( pForward->Begin( &uiNumPasses, 0 ));
        
		for( UINT pass = 0; pass < uiNumPasses; pass++ )
		{
			V( pForward->BeginPass( 0 ));

			for( list< CNode* >::iterator i = pScene->m_pNodes.begin(); i != pScene->m_pNodes.end(); i++ )
			{
				CNode* pNode = *i;

				if( ! pNode->m_bVisible  )
					continue;

				// if the node is not flagged as being lit by this specific light number, bail
				if( ! ( pNode->m_uiIlluminationFlags & GetLightFlag( uiLightNum ) ) )
					continue;				

				D3DXMATRIX matWorld;
				pNode->GetWorldMatrix( &matWorld );
				//			pForward->SetMatrix( hWorld, (*nodeIter)->GetWorldMatrix( &matWorld ) );

				D3DXMATRIX matWorldInverse;
				D3DXMatrixInverse( &matWorldInverse, NULL, &matWorld );

				D3DXVECTOR3 objectSpaceLightPosition;
				D3DXVec3TransformCoord( &objectSpaceLightPosition, &worldSpaceLightPosition , &matWorldInverse );
				V( pForward->SetFloatArray( hObjectSpaceLightPosition, (float*)&objectSpaceLightPosition, 3 ));

				D3DXVECTOR3 eyePos = pCamera->GetPosition( );
				D3DXVECTOR3 objectSpaceEyePosition;
				D3DXVec3TransformCoord( &objectSpaceEyePosition, &eyePos, &matWorldInverse );
				V( pForward->SetFloatArray( hObjectSpaceEyePosition, (float*)&objectSpaceEyePosition, 3 ));

				D3DXMATRIX matWVP = matWorld * matView * matProj;
				V( pForward->SetMatrix( hWVP, &matWVP ));			
				V( DrawSingleObject( pCamera, pNode, pForward ));

			}	// end node for loop

			V( pForward->EndPass( ) );

		}	// end render pass for loop

		V( pForward->End() );

	}	// end light for loop
	
	return hr;
}

// Additively blends each separate light into the light transport buffer
HRESULT CD3DForwardRenderer::LightSpots( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pForward, LPD3DXEFFECT pShadow )
{
	HRESULT hr = S_OK;

	//--------------------------------------------------------------//
	// Set up variables & effect params
	//--------------------------------------------------------------//
	
	D3DXHANDLE hPhongSpotLightNormalMapping	= pForward->GetTechniqueByName( "PhongSpotLightNormalMapping" );
	D3DXHANDLE hPhongSpotLightNormalMappingShadowed = pForward->GetTechniqueByName( "PhongSpotLightNormalMappingShadowed" );

	D3DXMATRIX matView, matProj;

	m_pCamera->GetViewMatrix( &matView );
	m_pCamera->GetProjMatrix( &matProj );

	D3DXHANDLE hView = pForward->GetParameterBySemantic( NULL, "View" );
	V( pForward->SetMatrix( hView, &matView ));

	// get handles to variables that will be constantly changed (faster than retrieving via name)
	D3DXHANDLE hWVP				= pForward->GetParameterBySemantic( NULL, "WorldViewProjection" );

	D3DXHANDLE hLightColour		= pForward->GetParameterByName( NULL, "g_fvLightColour" );
	D3DXHANDLE hLightMaxRange	= pForward->GetParameterByName( NULL, "g_fLightMaxRange" );

	// object space stuff is required for tangent space normal mapping
	D3DXHANDLE hObjectSpaceEyePosition		= pForward->GetParameterByName( NULL, "g_fvObjectSpaceEyePosition" );
	D3DXHANDLE hObjectSpaceLightDirection	= pForward->GetParameterByName( NULL, "g_fvObjectSpaceLightDirection" );
	D3DXHANDLE hObjectSpaceLightPosition	= pForward->GetParameterByName( NULL, "g_fvObjectSpaceLightPosition" );
	
	// spotlight stuff
	D3DXHANDLE hLightSpotFalloff = pForward->GetParameterByName( NULL, "g_fSpotlightFalloff" );
	D3DXHANDLE hInnerAngle		= pForward->GetParameterByName( NULL, "g_fInnerAngle" );
	D3DXHANDLE hOuterAngle		= pForward->GetParameterByName( NULL, "g_fOuterAngle" );   

	D3DXHANDLE hMatObjectToLightProjSpace	= pForward->GetParameterByName( NULL, "matObjectToLightProjSpace" );
	D3DXHANDLE hShadowMap					= pForward->GetParameterByName( NULL, "shadowMapRT_Tex" );

	UINT uiLightNum = (UINT)( pScene->m_pLightDirectional.size() + pScene->m_pLightOmni.size() );

	for( list< LightSpot_s* >::iterator lightIter = pScene->m_pLightSpot.begin(); lightIter != pScene->m_pLightSpot.end(); lightIter++, uiLightNum++ )
	{
		LightSpot_s* pLight = (*lightIter);

		if( ! pLight->m_bActive || ! pLight->m_bVisible )
			continue;
			
		// set light properties here
		D3DXMATRIX matLightRot;
		D3DXMatrixRotationYawPitchRoll( &matLightRot, pLight->m_fYaw, pLight->m_fPitch, 0.0f );

		D3DXVECTOR3 worldSpaceLightDirection = D3DXVECTOR3( matLightRot._31, matLightRot._32, matLightRot._33 ); 
		D3DXVECTOR3 worldSpaceLightPosition = pLight->m_position;

		D3DXMATRIX matLightViewProj;

		if( pLight->m_bCastsShadows )
		{
			V( GenerateSpotShadowMatrix( &matLightViewProj, pLight ));
			// render scene to shadow map RT / Depth surface.  The DrawSceneToShadowMap function takes care of setting
			// the appropriate render target / surfs etc and also restoring the light accum buffer & regular depth buffer.

			if( m_bUse2xFp )
			{
				V( m_pDevice->SetRenderTarget( 1, NULL ));
			}

			V( DrawSceneToShadowMap( pScene, pShadow, matLightViewProj, uiLightNum ));

			if( m_bUse2xFp )
			{
				V( m_pDevice->SetRenderTarget( 1, m_SceneB.pSurf ));
			}

			// set the results of the shadow map rendering as our shadow map texture			
			V( pForward->SetTexture( hShadowMap, m_ShadowMapRT.pTex ));			
			V( pForward->SetTechnique( hPhongSpotLightNormalMappingShadowed ) );	
		}
		else
		{
			V( pForward->SetTechnique( hPhongSpotLightNormalMapping ) );	
		}			

		float fInnerAngle = pLight->m_fInnerAngle;
		float fOuterAngle = pLight->m_fOuterAngle;

		fInnerAngle = cos( fInnerAngle * 0.5f );
		fOuterAngle = cos( fOuterAngle * 0.5f );

		V( pForward->SetFloat( hOuterAngle, fOuterAngle ));
		V( pForward->SetFloat( hInnerAngle, fInnerAngle ));		
		V( pForward->SetFloat( hLightSpotFalloff, pLight->m_fFalloff ));
		V( pForward->SetFloat( hLightMaxRange, pLight->m_fMaxRange ));
		V( pForward->SetVector( hLightColour, &pLight->m_colour ));

		UINT uiNumPasses;
		V( pForward->Begin( &uiNumPasses, 0 ));
		for( UINT pass = 0; pass < uiNumPasses; pass++ )
		{
			V( pForward->BeginPass( pass ));			

			for( list< CNode* >::iterator i = pScene->m_pNodes.begin(); i != pScene->m_pNodes.end(); i++ )
			{
				CNode* pNode = *i;

				if( ! pNode->m_bVisible  )
					continue;

				// if the node is not flagged as being lit by this specific light number, bail
				if( ! ( pNode->m_uiIlluminationFlags & GetLightFlag( uiLightNum ) ) )
					continue;

				D3DXMATRIX matWorld;
				pNode->GetWorldMatrix( &matWorld );
				//			pForward->SetMatrix( hWorld, (*nodeIter)->GetWorldMatrix( &matWorld ) );

				D3DXMATRIX matWorldInverse;
				D3DXMatrixInverse( &matWorldInverse, NULL, &matWorld );

				D3DXVECTOR3 objectSpaceLightPosition;
				D3DXVec3TransformCoord( &objectSpaceLightPosition, &worldSpaceLightPosition , &matWorldInverse );
				V( pForward->SetFloatArray( hObjectSpaceLightPosition, (float*)&objectSpaceLightPosition, 3 ));

				D3DXVECTOR3 objectSpaceLightDirection;
				D3DXVec3TransformNormal( &objectSpaceLightDirection, &worldSpaceLightDirection, &matWorldInverse );
				V( pForward->SetFloatArray( hObjectSpaceLightDirection, (float*)&objectSpaceLightDirection, 3 ));

				D3DXVECTOR3 eyePos = pCamera->GetPosition( );
				D3DXVECTOR3 objectSpaceEyePosition;
				D3DXVec3TransformCoord( &objectSpaceEyePosition, &eyePos, &matWorldInverse );
				V( pForward->SetFloatArray( hObjectSpaceEyePosition, (float*)&objectSpaceEyePosition, 3 ));

				D3DXMATRIX matWVP = matWorld * matView * matProj;
				V( pForward->SetMatrix( hWVP, &matWVP ));

				if( pLight->m_bCastsShadows )
				{
					// now set the matrix that allows the lighting shader to move from object to lightViewProj space 
					D3DXMATRIX matObjectToLightProjSpace;
					matObjectToLightProjSpace = matWorld * matLightViewProj * m_matTexScaleBias;
					V( pForward->SetMatrix( hMatObjectToLightProjSpace, &matObjectToLightProjSpace ));							
				}			

				V( DrawSingleObject( pCamera, pNode, pForward ));
			
			}	// end object for loop

			V( pForward->EndPass() );

		}	// end render pass for loop

		V( pForward->End() );	
		
	}	// end light for loop

	return hr;
}

HRESULT CD3DForwardRenderer::OnResetDevice( )
{
	HRESULT hr = S_OK;
    V_RETURN( g_EffectManager.LoadEffect( m_ForwardShadingEffectName ) );
	V_RETURN( ID3DRenderer::OnResetDevice() );
	return hr;
}

HRESULT CD3DForwardRenderer::OnLostDevice( )
{
	HRESULT hr = S_OK;

	V_RETURN( ID3DRenderer::OnLostDevice() );

	return hr;
}
