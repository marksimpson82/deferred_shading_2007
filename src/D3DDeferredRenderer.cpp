#include "D3DDeferredRenderer.h"
#include "dxcommon.h"
using namespace std;

CD3DDeferredRenderer::CD3DDeferredRenderer()
{

}

CD3DDeferredRenderer::~CD3DDeferredRenderer()
{

}

HRESULT CD3DDeferredRenderer::OneTimeInit( int iWidth, int iHeight, HWND hWnd, bool bWindowed, LPDIRECT3DDEVICE9 pDevice )
{
	HRESULT hr = S_OK;

	V_RETURN( ID3DRenderer::OneTimeInit( iWidth, iHeight, hWnd, bWindowed, pDevice ));

	m_eDebugOutputRT	= DO_NORMAL;
	m_DeferredEffectName = "effects/deferred.fx";

	m_bInitialised = true;

	return true;
}

HRESULT CD3DDeferredRenderer::Cleanup( )
{	
	HRESULT hr = S_OK;

	V( OnLostDevice() );
	V( ID3DRenderer::Cleanup() );

	return hr;
}

HRESULT CD3DDeferredRenderer::Render( Camera* pCamera, CScene* pScene, float fTimeDelta )
{	
	HRESULT hr = S_OK;
	m_pCamera = pCamera;

	CFrustum myFrustum; 	
	pCamera->GetFrustum( &myFrustum );
	
	/*
	CConvexHull myConvexHull;
	D3DXVECTOR3 pointToAdd( 0, 10000, 500 );
	myConvexHull.AddPointToFrustum( myFrustum, pointToAdd );
	UINT numPlanes = myConvexHull.m_planes.size();
	*/

	// EFFECT STUFF	
	LPD3DXEFFECT pDeferred = g_EffectManager.GetEffect( m_DeferredEffectName );
	LPD3DXEFFECT pShared = g_EffectManager.GetEffect( m_SharedEffectName );	
	
	if( ! pDeferred || ! pShared )
	{
		assert( false && "Failed to load Effect" );
		string output = string("Failed to load Effect");
		FATAL( output );
		return E_FAIL;
	}	

	//--------------------------------------------------------------//
	// Obtain handles & setup common variables
	//--------------------------------------------------------------//

	D3DXVECTOR2 screenDimensions( (float)m_iWidth, (float)m_iHeight );
	D3DXVECTOR2 UVAdjust( 0.5f / screenDimensions.x, 0.5f / screenDimensions.y );

	V( pDeferred->SetFloatArray( "g_fScreenSize", (float*)&screenDimensions, 2 ));
	V( pDeferred->SetFloatArray( "g_fUVAdjust", (float*)&UVAdjust, 2 ));
	V( pDeferred->SetBool( "g_bUseHDR", true ));
	V( pDeferred->SetBool( "g_bCastsShadows", false ));

	//V( pShared->SetTechnique( "DepthOnly" ))
	//V( DrawObjects( pCamera, pShared, true ));
			
	// set up our renderable textures as the active render targets.
	V( BindGBufferAsRTs( true ));

	// Fill Mr. G Buffer
	V( FillGBuffer( pCamera, pScene, pDeferred ));

    // unbind RTs and set the light accumulation buffer as our dest RT
	V( BindGBufferAsRTs( false ));
	
	//V( m_pDevice->SetRenderTarget( 0, m_LTSurfaces.pSurf ));
	//V( m_pDevice->SetRenderTarget( 0, m_pBackBufferSurf ));	

	if( DO_NORMAL == m_eDebugOutputRT )	// business as usual.  Do normal lighting calcs 
	{	
		if( m_bUse2xFp )
		{
			V( m_pDevice->SetRenderTarget( 0, m_SceneA.pSurf ));
			V( m_pDevice->SetRenderTarget( 1, m_SceneB.pSurf ));
		}
		else
		{
			V( m_pDevice->SetRenderTarget( 0, m_fpSceneF.pSurf ));
		}
		
		V( LightAmbient( pCamera, pScene, pDeferred ));
		V( LightDirectionals( pCamera, pScene, pDeferred, pShared ));
		V( LightOmnis( pCamera, pScene, pDeferred, pShared ));
		V( LightSpots( pCamera, pScene, pDeferred, pShared ));

		if( pScene->m_bFogEnable )
		{
			UINT uiNumPasses;
			V( pDeferred->SetTechnique( "Fogging" ));

			V( pDeferred->SetFloatArray( "g_fvFogColour", (float*)&pScene->m_fogColour, 3 ));
			V( pDeferred->SetFloat( "g_fFogStart", pScene->m_fFogStart ));
			V( pDeferred->SetFloat( "g_fFogEnd", pScene->m_fFogEnd ));

			V( pDeferred->Begin( &uiNumPasses, 0 ));

			for( UINT pass = 0; pass < uiNumPasses; pass++ )
			{
				V( pDeferred->BeginPass( pass ));
				V( DrawFullScreenQuad( pDeferred ));
				V( pDeferred->EndPass( ));
			}
			V( pDeferred->End( ));
		}		

		V( DrawSkybox( pCamera, pScene, pDeferred ) );
			

		//--------------------------------------------------------------//
		// Debug Wireframe Pass
		//--------------------------------------------------------------//
		
		if( m_bDebugDisplayMeshBoundingWireframes )
		{
			V( DebugShowBoundingSphereWireframes( pCamera, pScene, pShared ));
		}

		if( m_bDebugDisplayLightWireframes )
		{
			V( DebugShowLightMeshWireframes( pCamera, pScene, pShared ));
		}		
	
		// HDR STUFF

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
			
			V( pShared->CommitChanges( ));			
			V( pShared->Begin( &uiNumPasses, 0 ));

			for( UINT i = 0; i < uiNumPasses; i++ )
			{
				V( pShared->BeginPass( i ));			
				V( DrawNVIDIAQuad( m_fullscreenQuadCoords, m_pVertexDecFVF1, sizeof( ScreenVertex1 ) ));
				V( pShared->EndPass( ));
			}
			
			V( pShared->End( ));
		//	V( PassThrough( pShared, "LightTransport", m_LTSurfaces.pTex, m_pBackBufferSurf ));
		}		
		
		if( m_bDebugDisplayShadowMap )
			V( DebugShowShadowMap( pShared ));		
		
	}	// END if( DO_NORMAL )
	else
	{
		V( m_pDevice->SetRenderTarget( 0, m_pBackBufferSurf ));
		V( DisplayRT( pDeferred, m_eDebugOutputRT ));
	}

	V( m_pDevice->SetRenderTarget( 0, m_pBackBufferSurf ));
	V( m_pDevice->SetDepthStencilSurface( m_pZBufferSurf ));

	return S_OK;
}



HRESULT CD3DDeferredRenderer::FillGBuffer( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect )
{	
	HRESULT hr = S_OK;
	D3DXHANDLE hWorldView		= pEffect->GetParameterBySemantic( NULL, "WorldView" );
	D3DXHANDLE hWVP				= pEffect->GetParameterBySemantic( NULL, "WorldViewProjection" );
	
	D3DXHANDLE hDebugWireframeColour = pEffect->GetParameterByName( NULL, "g_fvDebugWireframeColour" );

	D3DXMATRIX ortho;
	D3DXMatrixOrthoOffCenterLH( &ortho, 0, (float)m_iWidth, (float)m_iHeight, 0, -1, 1 );

	// Set effect params
	D3DXMATRIX matView, matProjPersp;

	m_pCamera->GetViewMatrix( &matView );
	V( pEffect->SetMatrix( "matView", &matView ));
	m_pCamera->GetProjMatrix( &matProjPersp );

	D3DXHANDLE hFillGBufferNormalMap = pEffect->GetTechniqueByName( "FillGBufferNormalMapping" );
	assert( hFillGBufferNormalMap );

	//--------------------------------------------------------------//
	// GBuffer Filling Pass (also does depth)
	//--------------------------------------------------------------//		

	V( pEffect->SetTechnique( hFillGBufferNormalMap ) );

	D3DXMATRIX matWorld;
	D3DXMatrixIdentity( &matWorld );

	D3DXMATRIX matWorldView = matWorld * matView;
	V( pEffect->SetMatrix( hWorldView, &matWorldView ));

	D3DXMATRIX matWVP = matWorld * matView * matProjPersp;
	V( pEffect->SetMatrix( hWVP, &matWVP ));

	UINT uiNumPasses;
	V( pEffect->Begin( &uiNumPasses, 0 ));

	for( UINT i = 0; i < uiNumPasses; i++ )
	{
		V( pEffect->BeginPass( i ));	
		V( DrawObjects( pCamera, pEffect ));
		V( pEffect->EndPass() );
	}
	
	V( pEffect->End() );

	/*
	D3DXHANDLE hFillGBuffer = pEffect->GetTechniqueByName( "FillGBuffer" );
	assert( hFillGBuffer );

	// for non-normal mapped objects (TEMP)
	V( pEffect->SetTechnique( hFillGBuffer ) );
	V( pEffect->Begin( &uiNumPasses, 0 ) );

	for( UINT i = 0; i < uiNumPasses; i++ )
	{
        // Geometry g-buffer pass
		V( pEffect->BeginPass( i ));

		for( list< LightOmni_s* >::iterator i = pScene->m_pLightOmni.begin(); i != pScene->m_pLightOmni.end(); i++ )
		{
			//	if( ! (*i)->bActive )
			//		continue;

			LightOmni_s* pLight = (*i);

			D3DXVECTOR3 lightPos = pLight->m_position;
			D3DXMATRIX matLightWorld;

			D3DXMatrixTranslation( &matLightWorld, lightPos.x, lightPos.y, lightPos.z );

			D3DXMATRIX matWorldView = matLightWorld * matView;
			D3DXMATRIX matWVP = matWorldView * matProjPersp;

			V( pEffect->SetMatrix( hWorldView, &matWorldView ));
			V( pEffect->SetMatrix( hWVP, &matWVP ));

			V( DrawSingleObject( pCamera, pScene->m_pLightBulb, pEffect ));
		}		

		for( list< LightSpot_s* >::iterator i = pScene->m_pLightSpot.begin(); i != pScene->m_pLightSpot.end(); i++ )
		{
			LightSpot_s* pLight = (*i);

			D3DXVECTOR3 lightPos = pLight->m_position;

			D3DXMATRIX matLightWorld;
			D3DXMatrixRotationYawPitchRoll( &matLightWorld, pLight->m_fYaw, pLight->m_fPitch, 0.0f );
			matLightWorld._41 = lightPos.x;
			matLightWorld._42 = lightPos.y;
			matLightWorld._43 = lightPos.z;

			D3DXMATRIX matWorldView = matLightWorld * matView;
			D3DXMATRIX matWVP = matWorldView * matProjPersp;

			V( pEffect->SetMatrix( hWorldView, &matWorldView ));
			V( pEffect->SetMatrix( hWVP, &matWVP ));

			V( DrawSingleObject( pCamera, pScene->m_pSpotLight, pEffect ));
		}

		V( pEffect->EndPass( ));
	}
	
	V( pEffect->End());
	*/

	return S_OK;
}

//--------------------------------------------------------------//
// Ambient & Depth Pass
//--------------------------------------------------------------//

HRESULT CD3DDeferredRenderer::LightAmbient( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hWVP				= pEffect->GetParameterBySemantic( NULL, "WorldViewProjection" );
	D3DXHANDLE hAmbientLighting = pEffect->GetTechniqueByName( "AmbientLighting" );
	assert( hAmbientLighting );

	V( pEffect->SetTechnique( hAmbientLighting ));
		
	V( pEffect->SetTexture( "diffuseRT_Tex",	m_GBufferRTs[0].pTex ));
	V( pEffect->SetTexture( "positionRT_Tex",	m_GBufferRTs[1].pTex ));
	V( pEffect->SetTexture( "normalRT_Tex",	m_GBufferRTs[2].pTex ));

	V( pEffect->SetVector( "g_fvAmbientColour", &pScene->m_ambientColour ));

	UINT uiNumPasses;
	V( pEffect->Begin( &uiNumPasses, 0 ));

	for( UINT i = 0; i < uiNumPasses; i++ )
	{
		V( pEffect->BeginPass( i ));
		V( DrawFullScreenQuad( pEffect ));
		V( pEffect->EndPass() );
	}
	
	V( pEffect->End() );

	return S_OK;
}

// Additively blends each separate light into the light transport buffer
HRESULT CD3DDeferredRenderer::LightDirectionals( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hWVP							= pDeferred->GetParameterBySemantic( NULL, "WorldViewProjection" );	
	D3DXHANDLE hPhongDirectionalShadowed	= pDeferred->GetTechniqueByName( "PhongDirectionalShadowed" );
	D3DXHANDLE hCastsShadows				= pDeferred->GetParameterByName( NULL, "g_bCastsShadows" );

	D3DXMATRIX matProjPersp;
	pCamera->GetProjMatrix( &matProjPersp );

	D3DXMATRIX matView;
	pCamera->GetViewMatrix( &matView );

	D3DXMATRIX matInvView;
	D3DXMatrixInverse( &matInvView, NULL, &matView );

	D3DXHANDLE hLightDirection	= pDeferred->GetParameterByName( NULL, "g_fvViewSpaceLightDirection" );
	D3DXHANDLE hLightColour		= pDeferred->GetParameterByName( NULL, "g_fvLightColour" );
	D3DXHANDLE hMatViewToLightProjSpace = pDeferred->GetParameterByName( NULL, "matViewToLightProjSpace" );
	D3DXHANDLE hShadowMap		= pDeferred->GetParameterByName( NULL, "shadowMapRT_Tex" );


	UINT uiLightNum = 0;

	for( list< LightDirectional_s* >::iterator i = pScene->m_pLightDirectional.begin(); i != pScene->m_pLightDirectional.end(); i++, uiLightNum++ )
	{
		LightDirectional_s* pLight = (*i);

		if( ! pLight->m_bActive )
			continue;

		D3DXMATRIX matLightRot;
		D3DXMatrixRotationYawPitchRoll( &matLightRot, pLight->m_fYaw, pLight->m_fPitch, 0.0f );
		D3DXVECTOR3 worldSpaceLightDirection = D3DXVECTOR3( matLightRot._31, matLightRot._32, matLightRot._33 ); 

		if( pLight->m_bCastsShadows )
		{
			D3DXMATRIX matLightViewProj;

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
			V( pDeferred->SetTexture( hShadowMap, m_ShadowMapRT.pTex ));

			// now set the matrix that allows the lighting shader to move from view to lightViewProj space 
			D3DXMATRIX matViewToLightProjSpace;
			matViewToLightProjSpace = matInvView * matLightViewProj * m_matTexScaleBias;
			V( pDeferred->SetMatrix( hMatViewToLightProjSpace, &matViewToLightProjSpace ));			
		}
		// this stuff is common to whatever technique we use.				
		V( pDeferred->SetTechnique( hPhongDirectionalShadowed ));

		D3DXVECTOR3 viewSpaceLightDirection;
		D3DXVec3TransformNormal( &viewSpaceLightDirection, &worldSpaceLightDirection, &matView );

		V( pDeferred->SetBool( hCastsShadows, pLight->m_bCastsShadows ));
		V( pDeferred->SetVector( hLightColour, &pLight->m_colour ));
		V( pDeferred->SetFloatArray( hLightDirection, (float*)&viewSpaceLightDirection, 3 ));	

		UINT uiNumPasses;
		V( pDeferred->Begin( &uiNumPasses, 0 ));

		for( UINT i = 0; i < uiNumPasses; i++ )
		{
			V( pDeferred->BeginPass( i ));
			V( DrawFullScreenQuad( pDeferred ) );
			V( pDeferred->EndPass( ));
		}
		
		V( pDeferred->End( ));

	}

	return hr;
}

// Additively blends each separate light into the light transport buffer
HRESULT CD3DDeferredRenderer::LightOmnis( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hWVP							= pDeferred->GetParameterBySemantic( NULL, "WorldViewProjection" );	
	D3DXHANDLE hCastsShadows				= pDeferred->GetParameterByName( NULL, "g_bCastsShadows" );

	D3DXMATRIX matProjPersp;
	pCamera->GetProjMatrix( &matProjPersp );

	D3DXMATRIX matView;
	pCamera->GetViewMatrix( &matView );

	D3DXMATRIX matInvView;
	D3DXMatrixInverse( &matInvView, NULL, &matView );

	D3DXHANDLE hLightColour		= pDeferred->GetParameterByName( NULL, "g_fvLightColour" );
	D3DXHANDLE hMatViewToLightProjSpace = pDeferred->GetParameterByName( NULL, "matViewToLightProjSpace" );
	D3DXHANDLE hShadowMap		= pDeferred->GetParameterByName( NULL, "shadowMapRT_Tex" );

	//--------------------------------------------------------------//
	// Point Light Pass (Stencil optimised)
	//--------------------------------------------------------------//

	D3DXHANDLE hPhongPointStencil = pDeferred->GetTechniqueByName( "PhongPointStencil" );
	assert( hPhongPointStencil );

	D3DXHANDLE hLightPosition	= pDeferred->GetParameterByName( NULL, "g_fvViewSpaceLightPosition" );
	D3DXHANDLE hLightMaxRange	= pDeferred->GetParameterByName( NULL, "g_fLightMaxRange" );

	UINT uiLightNum = (UINT)pScene->m_pLightDirectional.size();

	for( list< LightOmni_s* >::iterator i = pScene->m_pLightOmni.begin(); i != pScene->m_pLightOmni.end(); i++, uiLightNum++ )
	{
		LightOmni_s* pLight = (*i);

		if( ! pLight->m_bActive || ! pLight->m_bVisible )
			continue;

		// set up shadowing stuff.  Render shadow maps & variables
		if( pLight->m_bCastsShadows )
		{
			V( pDeferred->SetTechnique( hPhongPointStencil ));
		}
		else	// no shadows, so just set appropriate technique
		{
			V( pDeferred->SetTechnique( hPhongPointStencil ));
		}

		// this stuff is common to whatever shadowing algorithm we're using
		
		V( m_pDevice->SetVertexDeclaration( m_pSphereMeshVertexDeclaration ));
		V( m_pDevice->Clear(0, NULL, D3DCLEAR_STENCIL, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 1 ));

		// set light position in world & transform into view space then set up light colour
		D3DXVECTOR3 lightWorldPos = pLight->m_position;
		D3DXVECTOR3 lightViewPos;
		D3DXVec3TransformCoord( &lightViewPos, &lightWorldPos, &matView );
		
		// scale up the mesh very slightly; this is because the sphere's tesselation level sometimes won't be sufficient
		// to avoid ugly borders showing up.  I.e. instead of smoothly fading out with attenuation, there would be a visible line
		// where the sphere ended but where the lighting contribution was > 0. If the sphere could be _perfectly_ round, we wouldn't need to do this.
		float fScale = pLight->m_fMaxRange * 1.1f; 

		// scale light volume mesh to match light's max range
		D3DXMATRIX scale, translate, sphereMatWVP;

		D3DXMatrixTranslation( &translate, lightWorldPos.x, lightWorldPos.y, lightWorldPos.z );
		D3DXMatrixScaling( &scale, fScale, fScale, fScale );

		sphereMatWVP = scale * translate * matView * matProjPersp;

		V( pDeferred->SetFloatArray( hLightPosition, (float*)&lightViewPos, 3 ));
		V( pDeferred->SetBool( hCastsShadows, pLight->m_bCastsShadows ));
		V( pDeferred->SetVector( hLightColour, &pLight->m_colour ));
		V( pDeferred->SetFloat( hLightMaxRange, pLight->m_fMaxRange ));
		V( pDeferred->SetMatrix( hWVP, &sphereMatWVP ));

		V( pDeferred->CommitChanges( ));

		// render light volume to stencil buffer, leaving 0s where the light intersects scene geometry				
		// render second pass, evaluating the lighting contribution for each pixel with stencil bit set to 0
		
		UINT uiNumPasses;
		V( pDeferred->Begin( &uiNumPasses, 0 ));

		for( UINT i = 0; i < uiNumPasses; i++ )
		{
			V( pDeferred->BeginPass( i ));
			V( m_pSphereMesh->DrawSubset( 0 ));
			V( pDeferred->EndPass() );
		}

		V( pDeferred->End());

	}					

	return hr;
}

// Additively blends each separate light into the light transport buffer
HRESULT CD3DDeferredRenderer::LightSpots( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hWVP							= pDeferred->GetParameterBySemantic( NULL, "WorldViewProjection" );	
	D3DXHANDLE hCastsShadows				= pDeferred->GetParameterByName( NULL, "g_bCastsShadows" );

	D3DXMATRIX matProjPersp;
	pCamera->GetProjMatrix( &matProjPersp );

	D3DXMATRIX matView;
	pCamera->GetViewMatrix( &matView );

	D3DXMATRIX matInvView;
	D3DXMatrixInverse( &matInvView, NULL, &matView );

	D3DXHANDLE hLightDirection	= pDeferred->GetParameterByName( NULL, "g_fvViewSpaceLightDirection" );
	D3DXHANDLE hLightColour		= pDeferred->GetParameterByName( NULL, "g_fvLightColour" );
	D3DXHANDLE hMatViewToLightProjSpace = pDeferred->GetParameterByName( NULL, "matViewToLightProjSpace" );
	D3DXHANDLE hShadowMap		= pDeferred->GetParameterByName( NULL, "shadowMapRT_Tex" );

	D3DXHANDLE hPhongSpotStencilShadowed	= pDeferred->GetTechniqueByName( "PhongSpotStencilShadowed" );
	assert( hPhongSpotStencilShadowed );

	D3DXHANDLE hLightPosition	= pDeferred->GetParameterByName( NULL, "g_fvViewSpaceLightPosition" );
	D3DXHANDLE hLightMaxRange	= pDeferred->GetParameterByName( NULL, "g_fLightMaxRange" );
	D3DXHANDLE hLightSpotFalloff = pDeferred->GetParameterByName( NULL, "g_fSpotlightFalloff" );
	D3DXHANDLE hInnerAngle		= pDeferred->GetParameterByName( NULL, "g_fInnerAngle" );
	D3DXHANDLE hOuterAngle		= pDeferred->GetParameterByName( NULL, "g_fOuterAngle" );

	UINT uiLightNum = (UINT)( pScene->m_pLightDirectional.size() + pScene->m_pLightOmni.size());

	for( list< LightSpot_s* >::iterator i = pScene->m_pLightSpot.begin(); i != pScene->m_pLightSpot.end(); i++, uiLightNum++ )
	{
		LightSpot_s* pLight = (*i);

		if( ! pLight->m_bActive )
			continue;

		if( ! pLight->m_bVisible )
			continue;

		// if this is a shadow casting light, render out a shadow map, set the appropriate 
		// spotlight technique and then set up the variables required only by shadow mapping

		if( pLight->m_bCastsShadows )
		{
			// Generate Shadow Map
			D3DXMATRIX matLightViewProj;

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
			V( pDeferred->SetTexture( hShadowMap, m_ShadowMapRT.pTex ));

			// now set the matrix that allows the lighting shader to move from view to lightViewProj space 
			D3DXMATRIX matViewToLightProjSpace;
			matViewToLightProjSpace = matInvView * matLightViewProj * m_matTexScaleBias;
			V( pDeferred->SetMatrix( hMatViewToLightProjSpace, &matViewToLightProjSpace ));
		}

		// use lighting technique featuring shadowing 
		V( pDeferred->SetTechnique( hPhongSpotStencilShadowed ));			

		// This stuff is common to shadowed and non-shadowed spotlight shaders, so we can run this last to save re-writing it for both

		V( m_pDevice->Clear( 0, NULL, D3DCLEAR_STENCIL, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 1 ));
		V( m_pDevice->SetVertexDeclaration( pScene->m_pPosVertDec ));	

		// Calculate light attributes 
		D3DXVECTOR3 lightWorldPos = pLight->m_position;
		D3DXVECTOR3 lightViewPos;
		D3DXVec3TransformCoord( &lightViewPos, &lightWorldPos, &matView );

		D3DXMATRIX matLightRot;
		D3DXMatrixRotationYawPitchRoll( &matLightRot, pLight->m_fYaw, pLight->m_fPitch, 0.0f );

		D3DXVECTOR3 worldSpaceLightDirection = D3DXVECTOR3( matLightRot._31, matLightRot._32, matLightRot._33 ); 

		D3DXVECTOR3 lightWorldDirection = worldSpaceLightDirection;
		D3DXVECTOR3 lightViewDirection;
		D3DXVec3TransformNormal( &lightViewDirection, &lightWorldDirection, &matView );

		float fInnerAngle = pLight->m_fInnerAngle;
		float fOuterAngle = pLight->m_fOuterAngle;

		fInnerAngle = cos( fInnerAngle * 0.5f );
		fOuterAngle = cos( fOuterAngle * 0.5f );

		// Set vars for shader
		V( pDeferred->SetBool( hCastsShadows, pLight->m_bCastsShadows ));
		V( pDeferred->SetFloatArray( hLightPosition, (float*)&lightViewPos, 3 ));
		V( pDeferred->SetFloatArray( hLightDirection, (float*)&lightViewDirection, 3 ));
		V( pDeferred->SetVector( hLightColour, &pLight->m_colour ));
		V( pDeferred->SetFloat( hLightSpotFalloff, pLight->m_fFalloff ));
		V( pDeferred->SetFloat( hLightMaxRange, pLight->m_fMaxRange ));
		V( pDeferred->SetFloat( hInnerAngle, fInnerAngle ));
		V( pDeferred->SetFloat( hOuterAngle, fOuterAngle ));

		D3DXMATRIX rotate, translate, matWVP;

		D3DXMatrixRotationYawPitchRoll( &rotate, pLight->m_fYaw, pLight->m_fPitch, 0.0f );
		D3DXMatrixTranslation( &translate, lightWorldPos.x, lightWorldPos.y, lightWorldPos.z );

		matWVP = rotate * translate * matView * matProjPersp;
		V( pDeferred->SetMatrix( hWVP, &matWVP ));
		V( pDeferred->CommitChanges( ));		

		UINT uiNumPasses;
		V( pDeferred->Begin( &uiNumPasses, 0 ));

		for( UINT i = 0; i < uiNumPasses; i++ )
		{
			V( pDeferred->BeginPass( i ));
			V( pLight->m_pConeMesh->DrawSubset( 0 ));
			V( pDeferred->EndPass() );
		}
		V( pDeferred->End() );	
	}

	return hr;
}

HRESULT CD3DDeferredRenderer::BindGBufferAsRTs( bool bBindTexturesAsRT )
{
	HRESULT hr = S_OK;

	if( bBindTexturesAsRT )
	{
		// Get surfs for each RT and set up the RTs.  0 = diffuse (rgb = colour, w = spec), 1 = pos, 2 = normal
		for( unsigned int i = 0; i < kiNumRTs; i++ )
		{			
			V_RETURN( m_pDevice->SetRenderTarget( i, m_GBufferRTs[ i ].pSurf ) );

			// Clear diffuse buffer
			if( 0 == i )	
				V( m_pDevice->ColorFill( m_GBufferRTs[ i ].pSurf, NULL, D3DCOLOR_ARGB( 0, 0, 0, 0 )));
		}
	}
	else
	{
		// unbind RTs 
		for( unsigned int i = 1; i < kiNumRTs; i++ ) // go from 1 to kiNumRTs as it is illegal to unbind RT 0
		{
			V_RETURN( m_pDevice->SetRenderTarget( i, NULL ));
		}		
	}

	return S_OK;
}

HRESULT CD3DDeferredRenderer::DisplayRT( LPD3DXEFFECT pEffect, unsigned int iRT )
{		
	HRESULT hr = S_OK;
	
	D3DXHANDLE hDebug_ShowRT = pEffect->GetTechniqueByName( "Debug_ShowRT" );
	
	V_RETURN( pEffect->SetTechnique( hDebug_ShowRT ));

	UINT numpasses;
	V( pEffect->Begin( &numpasses, 0 ));
	V( pEffect->BeginPass( iRT ));

	D3DXMATRIX ortho;
	D3DXMatrixOrthoOffCenterLH( &ortho, 0, (float)m_iWidth, (float)m_iHeight, 0, -1, 1 );

	switch( iRT )
	{
	case 0:
		pEffect->SetTexture( "diffuseRT_Tex",	m_GBufferRTs[0].pTex );
		break;

	case 1:
		pEffect->SetTexture( "positionRT_Tex",	m_GBufferRTs[1].pTex );
		break;

	case 2:
		pEffect->SetTexture( "normalRT_Tex",	m_GBufferRTs[2].pTex );
		break;

	default:
		break;
	}

	pEffect->SetMatrix( "matWorldViewProjection", &ortho );
	pEffect->CommitChanges();
	V( DrawFullScreenQuad( pEffect ));

	V( pEffect->EndPass());
	V( pEffect->End());

	return S_OK;
}

HRESULT CD3DDeferredRenderer::SetDebugOutputRT( DebugOutputRT eDebugOutputRT )
{
	HRESULT hr = S_OK;

	if( eDebugOutputRT < DO_TOTAL )
	{
		m_eDebugOutputRT = eDebugOutputRT;
		return hr;
	}
	
	g_Log.Write( string( "Invalid G-Buffer RT Display: " ) + toString( (int)eDebugOutputRT ), DL_WARN );
	return E_FAIL;
}

HRESULT CD3DDeferredRenderer::OnResetDevice()
{
	HRESULT hr = S_OK;

	// Create effects
	V_RETURN( g_EffectManager.LoadEffect( m_DeferredEffectName ));

	// Create G-Buffer RTs FIRST so they get the best possible speed (No idea why but performance dies if this isn't done first).
	for( unsigned int i = 0; i < kiNumRTs; i++ )
	{		
		// Create Render Targets
		V_RETURN( D3DXCreateTexture( m_pDevice, m_iWidth, m_iHeight, 1,
			D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &m_GBufferRTs[ i ].pTex ));

		V_RETURN( m_GBufferRTs[ i ].pTex->GetSurfaceLevel( 0, &m_GBufferRTs[ i ].pSurf ));		
	}	

	V_RETURN( ID3DRenderer::OnResetDevice() );

	return hr;
}

HRESULT CD3DDeferredRenderer::OnLostDevice()
{
	HRESULT hr = S_OK;

	for( unsigned int i = 0; i < kiNumRTs; i++ )
	{
		m_GBufferRTs[ i ].Release();	
	}

	V_RETURN( ID3DRenderer::OnLostDevice() );

	return hr;
}