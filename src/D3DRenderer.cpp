#include "d3drenderer.h"
#include "dxcommon.h"
#include "EffectManager.h"
#include "ModelManager.h"
#include "bounding.h"

using namespace std;

ID3DRenderer::ID3DRenderer( )
{
	
}

ID3DRenderer::~ID3DRenderer( )
{
}

HRESULT ID3DRenderer::OneTimeInit( int iWidth, int iHeight, HWND hWnd, bool bWindowed, LPDIRECT3DDEVICE9 pDevice )
{
	HRESULT hr = S_OK;

	//--------------------------------------------------------------//
	// Init Vars
	//--------------------------------------------------------------//
	m_iWidth	= iWidth;
	m_iHeight	= iHeight;
	m_hWnd		= hWnd;
	m_bWindowed	= bWindowed;
	m_pDevice	= pDevice;

	m_fDepthBias	= 0.001f;
	m_fBiasSlope	= 0.1f;

	m_bUse2xFp		= false;
	m_bUseHDR		= false;
	
	m_fExposure             = 0.18f;
	m_iPreviousLuminance    = 1;

	m_uiNumVisibleObjects	= 0;
	m_uiNumVisibleLights	= 0;

	//set special texture matrix for shadow mapping
	float fOffsetX = 0.5f + (0.5f / (float)kiShadowMapDimensions );
	float fOffsetY = 0.5f + (0.5f / (float)kiShadowMapDimensions );
	
	float fZScale = 1.0f;
	//float fZScale = (float)(1<<24);
	//float fZScale = 16777215;
	
	m_matTexScaleBias			= D3DXMATRIX(	0.5f,     0.0f,     0.0f,    0.0f,
												0.0f,    -0.5f,     0.0f,    0.0f,	
												0.0f,     0.0f,     fZScale, 0.0f,	
												fOffsetX, fOffsetY, 0.0f,    1.0f	);	

	m_bDebugDisplayShadowMap				= false;
	m_bDebugDisplayLightWireframes			= false;
	m_bDebugDisplayMeshBoundingWireframes	= false;
	m_SharedEffectName						= "effects/shared.fx";	

	m_pVertexDecFVF0	= NULL;
	m_pVertexDecFVF1	= NULL;
	m_pVertexDecFVF4	= NULL;
	m_pVertexDecFVF8	= NULL;

	m_pQuadVB			= NULL;
	m_pQuadVertexDec	= NULL;

	m_pBackBufferSurf	= NULL;
	m_pZBufferSurf		= NULL;
	
	//--------------------------------------------------------------//
	// Set up FullScreenQuad vertex buffers & formats etc.
	//--------------------------------------------------------------//

	//setup quad matrix to render a fullscreen quad
	D3DXMATRIX matWorld;
	D3DXMATRIX matView;
	D3DXMATRIX matProj;
	D3DXMATRIX matViewProj;	

	D3DXVECTOR3 const vEyePt      = D3DXVECTOR3( 0.0f, 0.0f, -5.0f );
	D3DXVECTOR3 const vLookatPt   = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 const   vUp       = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

	// Set World, View, Projection, and combination matrices.
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUp);
	D3DXMatrixOrthoLH(&matProj, 4.0f, 4.0f, 0.2f, 20.0f);
	matViewProj = matView * matProj;
	D3DXMatrixScaling(&matWorld, 2.0f, 2.0f, 1.0f);
	m_matQuad = matWorld * matViewProj;
	
	return S_OK;
}


HRESULT ID3DRenderer::CreateQuad( IDirect3DVertexBuffer9* &quadVB, unsigned int width, unsigned int height )
{
	HRESULT hr = S_OK;

	// create vertex buffer 
	V_RETURN( m_pDevice->CreateVertexBuffer( 4 * sizeof( posTexVertex ), D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &quadVB, NULL));

	posTexVertex *pBuff;

	// account for DirectX's texel center standard:
	float u_adjust = 0.5f / width;
	float v_adjust = 0.5f / height;

	if (quadVB)
	{
		V_RETURN( quadVB->Lock( 0, 0,(void**)&pBuff, 0));	

		for (int i = 0; i < 4; ++i)
		{
			pBuff->pos = D3DXVECTOR3((i==0 || i==1) ? -1.0f : 1.0f,
				(i==0 || i==3) ? -1.0f : 1.0f,
				0.0f);
			pBuff->tex  = D3DXVECTOR2(((i==0 || i==1) ? 0.0f : 1.0f) + u_adjust, 
				((i==0 || i==3) ? 1.0f : 0.0f) + v_adjust);
			pBuff++; 
		}
		quadVB->Unlock();
	}

	return hr;
}

HRESULT ID3DRenderer::Cleanup() 
{
	HRESULT hr = S_OK;
	
	V( ID3DRenderer::OnLostDevice() );
	return S_OK;
}

HRESULT ID3DRenderer::GenerateDirectionalShadowMatrix( D3DXMATRIX* pViewProjOut, LightDirectional_s* pLight, const string& sceneName  )
{
	HRESULT hr = S_OK;

	D3DXMATRIX matLightView, matLightProj;

	// for an directional light, the light point is 'infinitely' far away.  However, since we want our shadow map
	// to maximise the texture space (i.e. view the scene from a distance that just packs in all the interesting geometry and no more)
	// then we want to find the centre point of interest (centre of scene bounding box/sphere) and then move the light's position
	// back along its direction vector to give the best view.

	// currently we don't have a hierarchical AABB or bounding sphere, so just... guess for the moment.  Change this later

	D3DXMATRIX matLightRot;
	D3DXMatrixRotationYawPitchRoll( &matLightRot, pLight->m_fYaw, pLight->m_fPitch, 0.0f );
	D3DXVECTOR3 worldSpaceLightDirection = D3DXVECTOR3( matLightRot._31, matLightRot._32, matLightRot._33 ); 

	D3DXVECTOR3 sceneCentre( 0.0f, 0.0f, 0.0f );
	float fOffset = 128.0f;
	float fSize = 512.0f;
	
	if( sceneName == "scenes/generator.txt" )
	{
		fOffset = 384.0f;
		fSize = 1024.0f;
	}
	else if( sceneName == "scenes/default.txt" )
	{
        fOffset = 16.0f;
		fSize = 128.0f;
	}
	
	D3DXVECTOR3 lightPosition = sceneCentre + ( -worldSpaceLightDirection * fOffset );;

	D3DXVECTOR3 worldY( 0.0f, 1.0f, 0.0f );
	D3DXVECTOR3 worldZ( 0.0f, 0.0f, 1.0f );

	D3DXVECTOR3 up = worldY;

	// if the light's direction is miles away from or very close to being a match for the world up, choose the z axis as up instead
	if( fabs( D3DXVec3Dot( &worldSpaceLightDirection, &worldY )) > 0.99f )
		up = worldZ;

	// build light's view matrix
	D3DXMatrixLookAtLH( &matLightView, &lightPosition, &sceneCentre, &up );

	// build orthographic projection matrix.  Change this later to take into account the frustum sides in the light's view space
	//D3DXMatrixOrthoOffCenterLH( &matLightProj,   )
	D3DXMatrixOrthoLH( &matLightProj, fSize, fSize, 1.0f, 1000.0f );

	*pViewProjOut = matLightView * matLightProj;

	
	return hr;
}

HRESULT ID3DRenderer::GenerateSpotShadowMatrix( D3DXMATRIX* pViewProjOut, LightSpot_s* pLight )
{
	HRESULT hr = S_OK;

	// Generate Shadow Map
	D3DXMATRIX matLightView, matLightProj;

	D3DXVECTOR3 worldY( 0.0f, 1.0f, 0.0f );
	D3DXVECTOR3 worldZ( 0.0f, 0.0f, 1.0f );

	D3DXVECTOR3 up = worldY;

	// if the light's direction is miles away from or very close to being a match for the world up, choose the z axis as up instead

	D3DXMATRIX lightRot;
	D3DXMatrixRotationYawPitchRoll( &lightRot, pLight->m_fYaw, pLight->m_fPitch, 0.0f );

	D3DXVECTOR3 lightDir = D3DXVECTOR3( lightRot._31, lightRot._32, lightRot._33 );

	if( fabs( D3DXVec3Dot( &lightDir, &worldY )) > 0.99f )
		up = worldZ;

	D3DXVECTOR3 lookAt;
	lookAt = pLight->m_position + lightDir;

	// build light's view matrix
	D3DXMatrixLookAtLH( &matLightView, &pLight->m_position, &lookAt, &up );

	// build persp projection matrix.  
	D3DXMatrixPerspectiveFovLH( &matLightProj, pLight->m_fOuterAngle, 1.0f, 1.0f, pLight->m_fMaxRange );

	*pViewProjOut = matLightView * matLightProj;


	return hr;
}


HRESULT ID3DRenderer::FindVisibleEntities( Camera* pCamera, CScene* pScene )
{
	HRESULT hr = S_OK;

	m_uiNumVisibleLights = m_uiNumVisibleObjects = 0;

	CFrustum Frustum;
	pCamera->GetFrustum( &Frustum );
	
	// Flag nodes as being inside the frustum (visible) or outside (not visible)
	for( list< CNode* >::iterator i = pScene->m_pNodes.begin(); i != pScene->m_pNodes.end(); i++ )
	{		
		(*i)->m_bVisible = Frustum.AABBIntersection( (*i)->m_BoundingBox );
		//(*i)->m_bVisible =  ConvexHullSphereIntersection( &Frustum.m_planes[0], Frustum.m_planes.size(), (*i)->m_BoundingSphere  );
		//(*i)->m_bVisible = Frustum.TestSphere( (*i)->m_BoundingSphere );			
		(*i)->m_uiIlluminationFlags = 0;
		(*i)->m_uiShadowFlags		= 0;

		if( (*i)->m_bVisible )
			m_uiNumVisibleObjects++;
	}

	// do the same for lights.  If a light does not intersect the frustum then it does not contribute to the final rendered image
	// thus we can avoid costly lighting calculations on lights that do not influence the scene

	// lightnumbers go from 0 to x.  Directional lights = 0 to directionalTotal -1
	// Point lights go from directionalTotal to directionalTotal + PointTotal -1
	// Spot lights go from directionalTotal + PointTotal to directional + PointTotal - 1

	UINT uiLightNum = (UINT)pScene->m_pLightDirectional.size();

	for( list< LightOmni_s* >::iterator i = pScene->m_pLightOmni.begin(); i != pScene->m_pLightOmni.end(); i++ )
	{
		LightOmni_s* pLight = *i;

		CBoundingSphere LightBS;
		LightBS.m_Position = pLight->m_position;
		LightBS.m_radius		= pLight->m_fMaxRange;

		(*i)->m_bVisible = ConvexHullSphereIntersection( &Frustum.m_planes[0], Frustum.m_planes.size(), LightBS  );
		//(*i)->m_bVisible = Frustum.TestSphere( LightBS );    		

		if( (*i)->m_bVisible )
			m_uiNumVisibleLights++;

        ++uiLightNum;
	}

	
	// temp check using bounding spheres for the light spots.
	// This should be replaced with some kind of a cone test

	for( list< LightSpot_s* >::iterator i = pScene->m_pLightSpot.begin(); i != pScene->m_pLightSpot.end(); i++ )
	{
		LightSpot_s* pLight = *i;

		// currently, the bounding sphere is based on using the max distance as radius. This means that, instead of the bounding sphere
		// being centred at the middle of the spotlight cone's look vector * fMaxRange, it's actually centred at
		// the spotlight's position and extends out in all directions.  To hack around this, just move the centre
		// of the bounding sphere by the look vector * fMaxrange/2, then reduce halve the radius by ~almost half.

		//float fOffset = pLight->m_angle
		D3DXMATRIX matRot;
		D3DXMatrixRotationYawPitchRoll( &matRot, pLight->m_fYaw, pLight->m_fPitch, 0.0f );
		D3DXVECTOR3 vecLook( matRot(2,0), matRot(2,1), matRot(2,2) );
		D3DXVec3Normalize( &vecLook, &vecLook );
        
		D3DXVECTOR3 vecOffset = vecLook * pLight->m_fMaxRange * 0.5f;
		float fAdjustedRadius = pLight->m_fMaxRange * 0.55f;

		CBoundingSphere LightBS;
		LightBS.m_Position = pLight->m_position + vecOffset;
		LightBS.m_radius = fAdjustedRadius;

		(*i)->m_bVisible = ConvexHullSphereIntersection( &Frustum.m_planes[0], Frustum.m_planes.size(), LightBS );
		//(*i)->m_bVisible = Frustum.TestSphere( LightBS );  

		if( (*i)->m_bVisible )
			m_uiNumVisibleLights++;

		++uiLightNum;
	}

	return hr;
}



// shadow sets are required for efficient rendering (to minimise the meshes drawn per shadow map).  Basically, it goes like this:
// 1.	Only operates on visible lights.  If none of a light's rays reach the frustum it can't cast shadows into the frustum. 

// 2.	If a light's centre is fully inside the frustum, then potential shadow casters for that light must be 
//		visible to the light AND members of the visible set, as lights project shadows away from the light source.
//		I.e. For lights inside the frustum, any object outside the frustum will cast shadows away from the frustum (not visible).

// 3.	If a light's centre is off screen but it casts rays into the frustum, then we must build a convex hull 
//		that contains the frustum and the light, then find all objects inside this hull.  
//		Objects inside the hull AND visible to the light are the shadow casters.

HRESULT ID3DRenderer::BuildShadowCastingSet( Camera* pCamera, CScene* pScene )
{
    HRESULT hr = S_OK;

	UINT uiLightNum = (UINT)pScene->m_pLightDirectional.size();

	CFrustum Frustum;
	pCamera->GetFrustum( &Frustum );

	for( list< LightOmni_s* >::iterator i = pScene->m_pLightOmni.begin(); i != pScene->m_pLightOmni.end(); i++, uiLightNum++ )
	{
		LightOmni_s* pLight = *i;

		bool bCentreOfLightInsideFrustum = false;

		// if no part of the light is inside the frustum, don't bother working out any lighting influences as it won't illuminate anything visible
		if( ! pLight->m_bVisible )
			continue;

		CBoundingSphere LightBS;
		LightBS.m_Position	= pLight->m_position;
		LightBS.m_radius		= pLight->m_fMaxRange;

		bCentreOfLightInsideFrustum = ConvexHullPointIntersection( &Frustum.m_planes[0], Frustum.m_planes.size(), pLight->m_position );

		for( list< CNode* >::iterator nodeIter = pScene->m_pNodes.begin(); nodeIter != pScene->m_pNodes.end(); nodeIter++ )
		{	
			CNode* pNode = *nodeIter;

			// if the light's centre is inside the frustum, then all objects in the shadow casting set must also be inside the frustum (i.e. visible)
			if( bCentreOfLightInsideFrustum )
			{					
				if( pNode->m_bVisible )
				{
					//if( IntersectBoundingSpheres( LightBS, pNode->m_BoundingSphere ))
					if( AABBSphereIntersection( pNode->m_BoundingBox, LightBS ))
					{
						pNode->m_uiShadowFlags |= GetLightFlag( uiLightNum );
					}		
				}
			}		
			// else the light's centre is outside the frustum and we have to check objects outside the frustum 
			// to see if they cast INTO the frustum.  This is a lot more complicated than the first case.
			else	
			{
				// for the moment, just brute force it.  Add the object to this light's shadow set.
				// This should be replaced with something along the lines of case 3.
				pNode->m_uiShadowFlags |= GetLightFlag( uiLightNum );
			}

		}	// end for		
	}

	for( list< LightSpot_s* >::iterator i = pScene->m_pLightSpot.begin(); i != pScene->m_pLightSpot.end(); i++, uiLightNum++ )
	{
		LightSpot_s* pLight = *i;

		bool bCentreOfLightInsideFrustum = false;

		// if no part of the light is inside the frustum, don't bother working out any lighting influences as it won't illuminate anything visible
		if( ! pLight->m_bVisible )
			continue;

		CBoundingSphere LightBS;
		LightBS.m_Position	= pLight->m_position;
		LightBS.m_radius		= pLight->m_fMaxRange;

		bCentreOfLightInsideFrustum = ConvexHullPointIntersection( &Frustum.m_planes[0], Frustum.m_planes.size(), pLight->m_position );
		CConvexHull frustumAndLightHull;

		// if the light's centre is outside the frustum, we need to generate a new convex hull surrounding the view frustum AND the light's position
		if( ! bCentreOfLightInsideFrustum )
		{
			frustumAndLightHull.AddPointToFrustum( Frustum, pLight->m_position );
		}

		for( list< CNode* >::iterator nodeIter = pScene->m_pNodes.begin(); nodeIter != pScene->m_pNodes.end(); nodeIter++ )
		{	
			CNode* pNode = *nodeIter;

			// if the light's centre is inside the frustum, then all objects in the shadow casting set must also be inside the frustum (i.e. visible)
			if( bCentreOfLightInsideFrustum )
			{					
				if( pNode->m_bVisible )
				{
					//if( IntersectBoundingSpheres( LightBS, pNode->m_BoundingSphere ))
					if( AABBSphereIntersection( pNode->m_BoundingBox, LightBS ))					
					{
						pNode->m_uiShadowFlags |= GetLightFlag( uiLightNum );
					}		
				}
			}		
			// else the light's centre is outside the frustum and we have to check objects outside the frustum 
			// to see if they cast INTO the frustum.  This is a lot more complicated than the first case.
			else	
			{
				// for the moment, just brute force it.  Add the object to this light's shadow set.
				// This should be replaced with something along the lines of case 3.
				//pNode->m_uiShadowFlags |= GetLightFlag( uiLightNum );

				// Test.  Not working right now.
				//if( ConvexHullPointIntersection( &frustumAndLightHull.m_planes[0], frustumAndLightHull.m_planes.size(), pNode->GetPosition() ))
				//{
					pNode->m_uiShadowFlags |= GetLightFlag( uiLightNum );
				//}
			}

		}	// end for		
	}

	return hr;
}


// Takes the a scene and breaks each node up into its constituent submeshes then sorts by texture.
// Naive implementation used solely so that the application has a sorted list of meshes to use
// This allows us to determine how deferred/forward shading aids in or hinders using the with multipass lighting
HRESULT ID3DRenderer::SortScene( Camera* pCamera, CScene* pScene )
{
	HRESULT hr = S_OK;
	
	for( list< CNode* >::iterator i = pScene->m_pNodes.begin(); i != pScene->m_pNodes.end(); i++ )
	{
		if( ! (*i)->m_bVisible )
			continue;

		D3DXMATRIX matWorld;
		(*i)->GetWorldMatrix( &matWorld );

		CModel* pModel = (*i)->GetModel();
		LPD3DXMESH pMesh = pModel->m_pMesh;
        	
		LPDIRECT3DVERTEXBUFFER9 pVB;
		// this increases ref count.  Must call release later on
		V_RETURN( pMesh->GetVertexBuffer( &pVB ));

		LPDIRECT3DINDEXBUFFER9 pIB;
		// this increases ref count.  Must call release later on
		V_RETURN( pMesh->GetIndexBuffer( &pIB ));

		DWORD dwNumBytesPerVertex = pMesh->GetNumBytesPerVertex();

		DWORD dwAttribTableSize;
		V_RETURN( pMesh->GetAttributeTable( NULL, &dwAttribTableSize ));

		
		D3DXATTRIBUTERANGE* pAttribRange = new D3DXATTRIBUTERANGE[dwAttribTableSize]; 
		if( FAILED ( hr = pMesh->GetAttributeTable( pAttribRange, &dwAttribTableSize )))
		{
			g_Log.Write( "Failed to get mesh's attribute table", DL_WARN );
			SAFE_DELETE_ARRAY( pAttribRange );
			return hr;
		}

		LPDIRECT3DVERTEXDECLARATION9 pVertexDeclaration = pModel->m_pVertexDeclaration;

		for( unsigned int i = 0; i < dwAttribTableSize; i++ )
		{				
			RendererData_s* pRenderData = new RendererData_s( &matWorld, pVB, pIB, dwNumBytesPerVertex, 
				pVertexDeclaration, &pAttribRange[i], pModel->m_pNormalTextures[i], pModel->m_bHasEmissive );
			V( AddObjectToList( pModel->m_pTextures[i], pRenderData ));
		}

		SAFE_DELETE_ARRAY( pAttribRange );	

		SAFE_RELEASE( pVB );
		SAFE_RELEASE( pIB );
		
	}

	return S_OK;
}

HRESULT ID3DRenderer::AddObjectToList( LPDIRECT3DTEXTURE9 pTexture, RendererData_s* pRendererData )
{
	HRESULT hr = S_OK;

	map< LPDIRECT3DTEXTURE9, RendererBucket_s* >::iterator i = m_RendererBuckets.find( pTexture );
	
	// find the texture in the map.  
	if( i == m_RendererBuckets.end() )
	{	
		// couldn't find it, so create a new render entry 
		RendererBucket_s* pRendererBucket = new RendererBucket_s( pRendererData );
		m_RendererBuckets.insert( pair< LPDIRECT3DTEXTURE9, RendererBucket_s* >( pTexture, pRendererBucket ) );                
		return S_OK;
	}

	// if we have a valid iterator, then we've already got a bucket with this texture, so append the new data to it
    ( i->second )->m_RendererDataList.push_back( pRendererData );

	return S_OK;;
}

HRESULT ID3DRenderer::OnEndOfFrame()
{
	HRESULT hr = S_OK;

	V( ClearRenderBuckets( ) );

	return S_OK;
}

HRESULT ID3DRenderer::ClearRenderBuckets( )
{
	HRESULT hr = S_OK;

	// delete stuff in list and clear list
	map< LPDIRECT3DTEXTURE9, RendererBucket_s* >::iterator i;

	for( i = m_RendererBuckets.begin(); i != m_RendererBuckets.end(); i++ )
	{
		SAFE_DELETE( i->second );
	}
	m_RendererBuckets.clear();	

	return S_OK;

}

HRESULT ID3DRenderer::DrawSceneToShadowMap( CScene* pScene, LPD3DXEFFECT pEffect, const D3DXMATRIX& matLightViewProj, 
										   const UINT uiLightNum, bool bBackFaceCulling, bool bBruteForce )
{	
	HRESULT hr = S_OK;

	// for each object, I need to set the shader's WVP matrix to the mesh's world matrix multiplied by the light's ViewProj matrix.
	// TEMP

	//--------------------------------------------------------------//
	// set up render targets & parameters for rendering to shadow map 
	//--------------------------------------------------------------//

	//  render color if it is going to be displayed (or necessary for shadow map) -- otherwise don't
#if defined( _DEBUG )
	V( m_pDevice->SetRenderState( D3DRS_COLORWRITEENABLE, 0xf ));
#else
	V( m_pDevice->SetRenderState( D3DRS_COLORWRITEENABLE, 0 ));
#endif

	DWORD dwOldCullMode = 0;
	V( m_pDevice->GetRenderState( D3DRS_CULLMODE, &dwOldCullMode ));

	// We can get slightly better precision using front face culling, but some meshes don't work well with it.  Default = back faces
	V( m_pDevice->SetRenderState( D3DRS_CULLMODE, bBackFaceCulling ? D3DCULL_CCW : D3DCULL_CW ));

	// save old surfaces
	LPDIRECT3DSURFACE9 pOldDepthSurf;
	LPDIRECT3DSURFACE9 pOldRenderTargetSurf;

	V( m_pDevice->GetDepthStencilSurface( &pOldDepthSurf ));
	V( m_pDevice->GetRenderTarget( 0, &pOldRenderTargetSurf ));

	// save old viewport
	D3DVIEWPORT9 oldViewport;
	V( m_pDevice->GetViewport( &oldViewport ) );

	D3DVIEWPORT9 shadowMapViewport;
	shadowMapViewport.X			= 0;
	shadowMapViewport.Y			= 0;
	shadowMapViewport.Width		= kiShadowMapDimensions;
	shadowMapViewport.Height	= kiShadowMapDimensions;
	shadowMapViewport.MinZ		= 0.0f;
	shadowMapViewport.MaxZ		= 1.0f;

	V( m_pDevice->SetViewport( &shadowMapViewport ));

	D3DXHANDLE hRenderToShadowMap = pEffect->GetTechniqueByName( "RenderToShadowMap" );

	V( pEffect->SetTechnique( hRenderToShadowMap ));
	V( m_pDevice->SetRenderTarget( 0, m_ShadowMapColourRT.pSurf ));
	V( m_pDevice->SetDepthStencilSurface( m_ShadowMapRT.pSurf ));

	V( m_pDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00FFFFFF, 1.0f, 0L ));

	V( m_pDevice->SetRenderState( D3DRS_DEPTHBIAS, *(DWORD*)&m_fDepthBias ));
	V( m_pDevice->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&m_fBiasSlope ));

	D3DXHANDLE hDiffuseMap	= pEffect->GetParameterByName( NULL, "base_Tex" );
	D3DXHANDLE hNormalMap	= pEffect->GetParameterByName( NULL, "normal_Tex" );
	D3DXHANDLE hWVP			= pEffect->GetParameterBySemantic( NULL, "WorldViewProjection" );

	//V( pEffect->SetTexture( hDiffuseMap, NULL ));
	//V( pEffect->SetTexture( hNormalMap, NULL ));

	UINT uiNumPasses;
	V( pEffect->Begin( &uiNumPasses, 0 ));
	V( pEffect->BeginPass( 0 ));	

	// Ultimately, all we're doing is taking a series of meshes and doing world * lightView * lightProj
	// So we can treat all lights as using the same function.  
	// Directional light	= ortho matrix 
	// Spot light			= persp matrix
	// Omni light can be done doing 6 * spotlights... but probably be better to use a virtual depth cube shadow map

	for( list< CNode* >::iterator i = pScene->m_pNodes.begin(); i != pScene->m_pNodes.end(); i++ )
	{
		CNode* pNode = (*i);

		CModel* pModel = pNode->GetModel();
		LPD3DXMESH pMesh = pModel->m_pMesh;

		if( ! bBruteForce )
		{
			// if the object is not marked as being part of this light's shadow set, don't render it.
			if( ! ( pNode->m_uiShadowFlags & GetLightFlag( uiLightNum ) ) )
				continue;
		}

		LPDIRECT3DVERTEXBUFFER9 pVB;
		// this increases ref count.  Must call release later on
		V( pMesh->GetVertexBuffer( &pVB ));

		LPDIRECT3DINDEXBUFFER9 pIB;
		// this increases ref count.  Must call release later on
		V( pMesh->GetIndexBuffer( &pIB ));

		DWORD dwAttribTableSize;
		V( pMesh->GetAttributeTable( NULL, &dwAttribTableSize ));

		D3DXATTRIBUTERANGE* pAttribRange = new D3DXATTRIBUTERANGE[ dwAttribTableSize ]; 
		V( pMesh->GetAttributeTable( pAttribRange, &dwAttribTableSize ));

		V( m_pDevice->SetStreamSource( 0, pVB, 0, pMesh->GetNumBytesPerVertex() ));
		V( m_pDevice->SetIndices( pIB ));
		V( m_pDevice->SetVertexDeclaration( pModel->m_pVertexDeclaration ));

		D3DXMATRIX matWorld;
		pNode->GetWorldMatrix( &matWorld );

		// We need the world view projection matrix to represent a transformation from object to the light's view project space
		// this is the equivalent of the camera's view space projection, but for the light's fov instead

		D3DXMATRIX matWVP = matWorld * matLightViewProj; // m_matLightViewProj;

		V( pEffect->SetMatrix( hWVP, &matWVP ));
		V( pEffect->CommitChanges());

		// render each submesh
		for( unsigned int j = 0; j < dwAttribTableSize; j++ )
		{
			V( m_pDevice->DrawIndexedPrimitive(	D3DPT_TRIANGLELIST,
				0,
				pAttribRange[j].VertexStart, 
				pAttribRange[j].VertexCount,
				pAttribRange[j].FaceStart * 3,
				pAttribRange[j].FaceCount		));
		}

		SAFE_DELETE_ARRAY( pAttribRange );
		SAFE_RELEASE( pVB );
		SAFE_RELEASE( pIB );
	}

	V( pEffect->EndPass());
	V( pEffect->End());

	//--------------------------------------------------------------//
	// Restore states & Surfs
	//--------------------------------------------------------------//
	
	V( m_pDevice->SetRenderState( D3DRS_CULLMODE, dwOldCullMode ));
	
	float zero = 0.0f;
	V( m_pDevice->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&zero));
	V( m_pDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&zero));

	// re-enable colour writes
#if !defined( _DEBUG )
	V( m_pDevice->SetRenderState( D3DRS_COLORWRITEENABLE, 0xf ));
#endif

	V( m_pDevice->SetRenderTarget( 0, pOldRenderTargetSurf ));
	V( m_pDevice->SetDepthStencilSurface( pOldDepthSurf ));

	V( m_pDevice->SetViewport( &oldViewport ));
	SAFE_RELEASE( pOldRenderTargetSurf );
	SAFE_RELEASE( pOldDepthSurf );

	return S_OK;
}

HRESULT ID3DRenderer::DrawObjects( Camera* pCamera, LPD3DXEFFECT pEffect, bool bDepthOnly )
{	
	HRESULT hr = S_OK;
	// get handles to variables that will be constantly changed (faster than retrieving via name)
	//D3DXHANDLE hWorld		= pEffect->GetParameterBySemantic( NULL, "World" );
	D3DXHANDLE hWorldView		= pEffect->GetParameterBySemantic( NULL, "WorldView" );
	D3DXHANDLE hWVP				= pEffect->GetParameterBySemantic( NULL, "WorldViewProjection" );

	// Set effect params
	D3DXMATRIX matView, matProjPersp;

	if( ! bDepthOnly )
	{
		m_pCamera->GetViewMatrix( &matView );
		V( pEffect->SetMatrix( "matView", &matView ));
		pCamera->GetProjMatrix( &matProjPersp );
	}	

	D3DXHANDLE hDiffuseMap	= pEffect->GetParameterByName( NULL, "base_Tex" );
	D3DXHANDLE hNormalMap	= pEffect->GetParameterByName( NULL, "normal_Tex" );
	D3DXHANDLE hEmissive	= pEffect->GetParameterByName( NULL, "g_bEmissive" );

	map< LPDIRECT3DTEXTURE9, RendererBucket_s* >::iterator rendBucketItor;

	for( rendBucketItor = m_RendererBuckets.begin(); rendBucketItor != m_RendererBuckets.end(); rendBucketItor++ )
	{
		LPDIRECT3DTEXTURE9 pTex = (rendBucketItor->first);
		RendererBucket_s* pBucket = (rendBucketItor->second);

		if( ! bDepthOnly )
		{
			V( pEffect->SetTexture( hDiffuseMap, pTex ));
		}		

		list< RendererData_s* >::iterator rdIter;
		for( rdIter = pBucket->m_RendererDataList.begin(); rdIter != pBucket->m_RendererDataList.end(); rdIter++ )
		{
			RendererData_s* pRD = (*rdIter);

			D3DXMATRIX matWorldView = pRD->m_matWorld * matView;
			D3DXMATRIX matWorldViewProj = matWorldView * matProjPersp;
			V( pEffect->SetMatrix( hWVP, &matWorldViewProj ));

			if( !bDepthOnly )
			{
				V( pEffect->SetMatrix( hWorldView, &matWorldView ));
				V( pEffect->SetTexture( hNormalMap, pRD->m_normalMap ));		
				V( pEffect->SetBool( hEmissive, pRD->m_bEmissive ));
			}			
			V( m_pDevice->SetStreamSource( 0, pRD->m_pVB, 0, pRD->m_dwNumBytesPerVertex ));
			V( m_pDevice->SetIndices( pRD->m_pIB ));
			V( m_pDevice->SetVertexDeclaration( pRD->m_pVertexDeclaration ));

			V( pEffect->CommitChanges());

			V( m_pDevice->DrawIndexedPrimitive(	D3DPT_TRIANGLELIST,
				0,
				pRD->m_attribs.VertexStart, 
				pRD->m_attribs.VertexCount,
				pRD->m_attribs.FaceStart * 3,
				pRD->m_attribs.FaceCount		));
		}
	}

	return S_OK;
}


HRESULT ID3DRenderer::DrawSingleObject( Camera* pCamera, CNode* pNode, LPD3DXEFFECT pEffect )
{
	HRESULT hr = S_OK;

	CModel* pModel = pNode->GetModel();
	LPD3DXMESH pMesh = pModel->m_pMesh;

	LPDIRECT3DVERTEXBUFFER9 pVB;
	// this increases ref count.  Must call release later on
	V( pMesh->GetVertexBuffer( &pVB ));

	LPDIRECT3DINDEXBUFFER9 pIB;
	// this increases ref count.  Must call release later on
	V( pMesh->GetIndexBuffer( &pIB ));

	DWORD dwAttribTableSize;
	V( pMesh->GetAttributeTable( NULL, &dwAttribTableSize ));

	D3DXATTRIBUTERANGE* pAttribRange = new D3DXATTRIBUTERANGE[dwAttribTableSize]; 
	V( pMesh->GetAttributeTable( pAttribRange, &dwAttribTableSize ));

	V( m_pDevice->SetStreamSource( 0, pVB, 0, pMesh->GetNumBytesPerVertex() ));
	V( m_pDevice->SetIndices( pIB ));
	V( m_pDevice->SetVertexDeclaration( pModel->m_pVertexDeclaration ));
	D3DXHANDLE hDiffuseMap	= pEffect->GetParameterByName( NULL, "base_Tex" );
	D3DXHANDLE hNormalMap	= pEffect->GetParameterByName( NULL, "normal_Tex" );

	// render each submesh
	for( unsigned int i = 0; i < dwAttribTableSize; i++ )
	{
		V( pEffect->SetTexture( hDiffuseMap, pModel->m_pTextures[i] ));
		V( pEffect->SetTexture( hNormalMap, pModel->m_pNormalTextures[i] ));
		V( pEffect->CommitChanges());
		V( m_pDevice->DrawIndexedPrimitive(	D3DPT_TRIANGLELIST,
			0,
			pAttribRange[i].VertexStart, 
			pAttribRange[i].VertexCount,
			pAttribRange[i].FaceStart * 3,
			pAttribRange[i].FaceCount		));
	}

	SAFE_DELETE_ARRAY( pAttribRange );
	SAFE_RELEASE( pVB );
	SAFE_RELEASE( pIB );

	return S_OK;
}


HRESULT ID3DRenderer::DrawFullScreenQuad( LPD3DXEFFECT pEffect )
{	
	HRESULT hr = S_OK;
	
	D3DXHANDLE hWVP = pEffect->GetParameterBySemantic( 0, "WorldViewProjection" );
	V( pEffect->SetMatrix( hWVP, &m_matQuad ));
	V( m_pDevice->SetVertexDeclaration( m_pQuadVertexDec ));
	V( m_pDevice->SetStreamSource( 0, m_pQuadVB, 0, sizeof( posTexVertex ) ));
	V( pEffect->CommitChanges() );
	V( m_pDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 ));

	return S_OK;
}

HRESULT ID3DRenderer::DrawNVIDIAQuad( void *scoords, LPDIRECT3DVERTEXDECLARATION9 pVertDec, unsigned int size ) 
{		
	HRESULT hr = S_OK;

	V( m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE));
	V( m_pDevice->SetVertexDeclaration( pVertDec ));	
	V_RETURN( m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 1, scoords, size));
	V( m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE));

	return hr;
}

HRESULT ID3DRenderer::DrawSkybox( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hSkybox		= pEffect->GetTechniqueByName( "SkyboxLast" );
	assert( hSkybox );

	D3DXHANDLE hWVP			= pEffect->GetParameterBySemantic( NULL, "WorldViewProjection" );

	V( pEffect->SetTechnique( hSkybox ));

	D3DXMATRIX matView, matProjPersp;
	pCamera->GetViewMatrix( &matView );
	pCamera->GetProjMatrix( &matProjPersp );

	//--------------------------------------------------------------//
	// Skybox Pass (no depth, no lighting)
	//--------------------------------------------------------------//

	D3DXMATRIX matCentredView = matView;
	matCentredView._41 = 0.0f;
	matCentredView._42 = 0.0f;
	matCentredView._43 = 0.0f;

	D3DXMATRIX matWVP = matCentredView * matProjPersp;

	// render		
	UINT uiNumPasses;
	V( pEffect->Begin( &uiNumPasses, 0 ));	

	V( pEffect->SetMatrix( hWVP, &matWVP ));
	V( pEffect->CommitChanges( ));

	for( UINT i = 0; i < uiNumPasses; i++ )
	{
		V( pEffect->BeginPass( i ));
		V( DrawSingleObject( pCamera, pScene->m_pSkybox, pEffect ));
		V( pEffect->EndPass( ));	
	}	

	V( pEffect->End());

	return hr;
}

HRESULT ID3DRenderer::PassThrough( LPD3DXEFFECT pEffect, const std::string& texNameInShader, LPDIRECT3DTEXTURE9 pSourceTex, LPDIRECT3DSURFACE9 pDestSurf )
{
	HRESULT hr = S_OK;
	
	V( m_pDevice->SetRenderTarget( 0, pDestSurf ));

	D3DXHANDLE hPassThrough = pEffect->GetTechniqueByName( "PassThrough" );
	assert( hPassThrough );

	V( pEffect->SetTechnique( hPassThrough ));

	UINT uiNumPasses = 0;
	V( pEffect->Begin( &uiNumPasses, 0 ));
	V( pEffect->BeginPass( 0 ));
		
	V( pEffect->SetTexture( texNameInShader.c_str(), pSourceTex ));
	V( DrawFullScreenQuad( pEffect ));

	V( pEffect->EndPass() );
	V( pEffect->End() );

	
	return hr;
}

HRESULT ID3DRenderer::OnResetDevice()
{
	HRESULT hr = S_OK;

	// Load effects

	V_RETURN( g_EffectManager.LoadEffect( m_SharedEffectName ));

	// Load other resources
	D3DFORMAT lightFormat = D3DFMT_A16B16G16R16F;

	// Obtain back buffer & depth buffer surfaces
	V_RETURN( m_pDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBufferSurf ) );
	V_RETURN( m_pDevice->GetDepthStencilSurface( &m_pZBufferSurf ));

	// initialise vertex declarations /////////////////////////////////////////////////	

	V_RETURN( m_pDevice->CreateVertexDeclaration( vePosTex, &m_pQuadVertexDec ));
	V_RETURN( m_pDevice->CreateVertexDeclaration( veFVF0, &m_pVertexDecFVF0 ));	
	V_RETURN( m_pDevice->CreateVertexDeclaration( veFVF1, &m_pVertexDecFVF1 ));	
	V_RETURN( m_pDevice->CreateVertexDeclaration( veFVF4, &m_pVertexDecFVF4 ));
	V_RETURN( m_pDevice->CreateVertexDeclaration( veFVF8, &m_pVertexDecFVF8 ));
	
	// Vertex Buffers & Meshes /////////////////////////////////////////////////////////
		
	V_RETURN( CreateQuad( m_pQuadVB, m_iWidth, m_iHeight ));
	V_RETURN( D3DXCreateSphere( m_pDevice, 1.0f, 16, 8, &m_pSphereMesh, NULL ));

	D3DVERTEXELEMENT9 vertexElements[MAX_FVF_DECL_SIZE];
	V_RETURN( m_pSphereMesh->GetDeclaration( vertexElements ));
	V_RETURN( m_pDevice->CreateVertexDeclaration( vertexElements, &m_pSphereMeshVertexDeclaration ));

	// NVIDIA STUFF
	// Crop the scene texture so width and height are evenly divisible by 8.
	// This cropped version of the scene will be used for post processing effects,
	// and keeping everything evenly divisible allows precise control over
	// sampling points within the shaders.

	m_iCroppedWidth  = m_iWidth - m_iWidth  % 8;
	m_iCroppedHeight = m_iHeight - m_iHeight % 8;
	m_iShrinkWidth   = m_iCroppedWidth / 4;
	m_iShrinkHeight  = m_iCroppedHeight / 4;
	
	CalculateOffsetsAndQuadCoords();

	if( m_bUse2xFp )
	{
		D3DFORMAT twoRTs = D3DFMT_G16R16F;

		V_RETURN( D3DXCreateTexture( m_pDevice, m_iWidth, m_iHeight, 1, D3DUSAGE_RENDERTARGET, twoRTs, D3DPOOL_DEFAULT, &m_SceneA.pTex ));
		V_RETURN( m_SceneA.pTex->GetSurfaceLevel(0, &m_SceneA.pSurf ));

		V_RETURN( D3DXCreateTexture( m_pDevice, m_iWidth, m_iHeight, 1, D3DUSAGE_RENDERTARGET, twoRTs, D3DPOOL_DEFAULT, &m_SceneB.pTex));
		V_RETURN( m_SceneB.pTex->GetSurfaceLevel(0, &m_SceneB.pSurf));

		V_RETURN( D3DXCreateTexture( m_pDevice, m_iShrinkWidth, m_iShrinkHeight, 1, D3DUSAGE_RENDERTARGET, twoRTs, D3DPOOL_DEFAULT, &m_fp16ShrinkA.pTex ));
		V_RETURN( m_fp16ShrinkA.pTex->GetSurfaceLevel(0, &m_fp16ShrinkA.pSurf ));

		V_RETURN( D3DXCreateTexture( m_pDevice, m_iShrinkWidth, m_iShrinkHeight, 1, D3DUSAGE_RENDERTARGET, twoRTs, D3DPOOL_DEFAULT, &m_fp16ShrinkB.pTex ));
		V_RETURN( m_fp16ShrinkB.pTex->GetSurfaceLevel(0, &m_fp16ShrinkB.pSurf ));

		V_RETURN( D3DXCreateTexture( m_pDevice, m_iShrinkWidth, m_iHeight / 2, 1, D3DUSAGE_RENDERTARGET, twoRTs, D3DPOOL_DEFAULT, &m_fp16ShrinkA_1.pTex ));
		V_RETURN( m_fp16ShrinkA_1.pTex->GetSurfaceLevel(0, &m_fp16ShrinkA_1.pSurf ));

		V_RETURN( D3DXCreateTexture( m_pDevice, m_iShrinkWidth, m_iShrinkHeight, 1, D3DUSAGE_RENDERTARGET, twoRTs, D3DPOOL_DEFAULT, &m_fp16ShrinkA_2.pTex ));
		V_RETURN( m_fp16ShrinkA_2.pTex->GetSurfaceLevel(0, &m_fp16ShrinkA_2.pSurf ));

	}
	else
	{
		D3DFORMAT oneRT = D3DFMT_A16B16G16R16F;

		V_RETURN( D3DXCreateTexture(m_pDevice, m_iWidth, m_iHeight, 1, D3DUSAGE_RENDERTARGET, oneRT, D3DPOOL_DEFAULT, &m_fpSceneF.pTex ));
		V_RETURN( m_fpSceneF.pTex->GetSurfaceLevel(0, &m_fpSceneF.pSurf ));
		
		V_RETURN( D3DXCreateTexture( m_pDevice, m_iShrinkWidth, m_iHeight/2, 1, D3DUSAGE_RENDERTARGET, oneRT, D3DPOOL_DEFAULT, &m_fp16ShrinkF_1.pTex));
		V_RETURN( m_fp16ShrinkF_1.pTex->GetSurfaceLevel(0, &m_fp16ShrinkF_1.pSurf ));
		
		// Shrinked RT for blur effect
		V_RETURN( D3DXCreateTexture( m_pDevice, m_iShrinkWidth, m_iShrinkHeight, 1, D3DUSAGE_RENDERTARGET, oneRT, D3DPOOL_DEFAULT, &m_fp16ShrinkF.pTex ));
		V_RETURN( m_fp16ShrinkF.pTex->GetSurfaceLevel( 0, &m_fp16ShrinkF.pSurf ));

		V_RETURN( D3DXCreateTexture( m_pDevice, m_iShrinkWidth, m_iShrinkHeight, 1, D3DUSAGE_RENDERTARGET, oneRT, D3DPOOL_DEFAULT, &m_fp16ShrinkF_2.pTex));
		V_RETURN( m_fp16ShrinkF_2.pTex->GetSurfaceLevel(0, &m_fp16ShrinkF_2.pSurf ));
	}	

	// Shadow map
	// when using hardware shadow maps, we don't write anything to the shadow map colour buffer unless we want to display debug output.
	// If we turn off colour writes, it saves on rendering time.  Therefore, we should disable colour writes when in release mode

	// IT IS NOT POSSBLE TO CREATE A DEPTH STENCIL TEXTURE WHEN USING THE REF DEVICE
	// REMEMBER THIS.  Remember that the NVPerfHUD requires a REF device but actually runs using HAL via some trickery
	// So if you're compiling for debugging via NVPerfHUD, it won't run without it.  
	// If you end up here in a debug break... you know why.  Remove the #NVPERFHUD definition or run it through NVPerfHUD 

	// colour buffer.  In general this is just a convenience since unbinding RT 0 is illegal even if we're not using colour writes.
	V_RETURN( m_pDevice->CreateTexture( kiShadowMapDimensions, kiShadowMapDimensions, 1,
		D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_ShadowMapColourRT.pTex, NULL ));

	V_RETURN( m_ShadowMapColourRT.pTex->GetSurfaceLevel( 0, &m_ShadowMapColourRT.pSurf ));

	// hardware shadow map
	V_RETURN( m_pDevice->CreateTexture( kiShadowMapDimensions, kiShadowMapDimensions, 1,D3DUSAGE_DEPTHSTENCIL, D3DFMT_D24S8, D3DPOOL_DEFAULT, &m_ShadowMapRT.pTex, NULL ));
	V_RETURN( m_ShadowMapRT.pTex->GetSurfaceLevel( 0, &m_ShadowMapRT.pSurf ));

	// Luminance 

	int dim = 64;
	for(int i = 0; i < MAX_LUM_TEX; i++)
	{
		V_RETURN( D3DXCreateTexture( m_pDevice, dim, dim, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &m_fp16Lum[i].pTex ));
		V_RETURN( m_fp16Lum[i].pTex->GetSurfaceLevel(0, &m_fp16Lum[i].pSurf ));
		dim >>= 2;
	}

	V_RETURN( D3DXCreateTexture( m_pDevice, 1, 1, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &m_fp32Lum[ 0 ].pTex ));
	V_RETURN( m_fp32Lum[0].pTex->GetSurfaceLevel(0, &m_fp32Lum[0].pSurf ));

	V_RETURN( D3DXCreateTexture( m_pDevice, 1, 1, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &m_fp32Lum[ 1 ].pTex ));
	V_RETURN( m_fp32Lum[1].pTex->GetSurfaceLevel(0, &m_fp32Lum[ 1 ].pSurf ));

	V_RETURN( D3DXCreateTexture( m_pDevice, 1, 1, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &m_fp32Lum[2].pTex ));
	V_RETURN( m_fp32Lum[2].pTex->GetSurfaceLevel(0, &m_fp32Lum[2].pSurf ));

	m_iPreviousLuminance = 1;

	// Fill the ping-pong luminance textures with some intial value which will be our targeted luminance
	RECT texelRect;

	texelRect.top    = 0;
	texelRect.bottom = 1;
	texelRect.left   = 0;
	texelRect.right  = 1;

	float texel = m_fExposure;

	V_RETURN( D3DXLoadSurfaceFromMemory( m_fp32Lum[0].pSurf, NULL, NULL, &texel, D3DFMT_R32F, sizeof(float), NULL, &texelRect, D3DX_FILTER_POINT, 0));
	V_RETURN( D3DXLoadSurfaceFromMemory( m_fp32Lum[1].pSurf, NULL, NULL, &texel, D3DFMT_R32F, sizeof(float), NULL, &texelRect, D3DX_FILTER_POINT, 0));

	return hr;
}

HRESULT ID3DRenderer::OnLostDevice()
{
	HRESULT hr = S_OK;

	SAFE_RELEASE( m_pBackBufferSurf );
	SAFE_RELEASE( m_pZBufferSurf );

	m_ShadowMapRT.Release();
	m_ShadowMapColourRT.Release();

	SAFE_RELEASE( m_pSphereMesh );

	SAFE_RELEASE( m_pSphereMeshVertexDeclaration );
	SAFE_RELEASE( m_pVertexDecFVF0 );
	SAFE_RELEASE( m_pVertexDecFVF1 );
	SAFE_RELEASE( m_pVertexDecFVF4 );
	SAFE_RELEASE( m_pVertexDecFVF8 );

	// NVIDIA STUFF
	m_SceneA.Release();
	m_SceneB.Release();

	m_fpSceneF.Release(); 
	m_fp16ShrinkF.Release();
	m_fp16ShrinkF_1.Release();
	m_fp16ShrinkF_2.Release();

	m_fp16ShrinkA.Release();
	m_fp16ShrinkB.Release();

	m_fp16ShrinkA_1.Release();
	m_fp16ShrinkA_2.Release();

	for( unsigned int i = 0; i < MAX_LUM_TEX; i++ )
	{
		m_fp16Lum[i].Release();
	}
	
	for( int i = 0; i < 3; i++ )
	{
		m_fp32Lum[i].Release();			
	}

	// TEMP ################################### //
	SAFE_RELEASE( m_pQuadVB );
	SAFE_RELEASE( m_pQuadVertexDec );

	V( ClearRenderBuckets( ) );

	return hr;
}

HRESULT ID3DRenderer::CalculateLuminance( const float frameDelta, LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 channelB )
{
//	int i;
	HRESULT hr = S_OK;

	if( m_bUse2xFp )
	{
		// Downscale, blur horizontally and then vertically the first RT and then the second one

		V( DownscaleSceneAniso( m_fp16ShrinkA.pSurf, m_fp16ShrinkA_1.pSurf, channelA, m_fp16ShrinkA_1.pTex ));
		V( DownscaleSceneAniso( m_fp16ShrinkB.pSurf, m_fp16ShrinkA_1.pSurf, channelB, m_fp16ShrinkA_1.pTex ));

		V( LuminancePass( m_fp16Lum[0].pSurf, LUMINANCE_DOWNSAMPLE_LOG, m_luminanceQuadCoords[0], m_fp16ShrinkA.pTex, m_fp16ShrinkB.pTex ));
	}
	else
	{		
		V( DownscaleSceneAniso( m_fp16ShrinkF.pSurf, m_fp16ShrinkF_1.pSurf, channelA, m_fp16ShrinkF_1.pTex ));		
		V( LuminancePass( m_fp16Lum[0].pSurf, LUMINANCE_DOWNSAMPLE_LOG, m_luminanceQuadCoords[0], m_fp16ShrinkF.pTex ));
	}	
	
	// Downsample the luminance to a 4x4 texure
	for( int i = 1; i < MAX_LUM_TEX; i++ )
	{
		V( LuminancePass( m_fp16Lum[i].pSurf, LUMINANCE_DOWNSAMPLE, m_luminanceQuadCoords[i], m_fp16Lum[i-1].pTex ));
	}
	V( LuminancePass( m_fp32Lum[2].pSurf, LUMINANCE_DOWNSAMPLE_EXP, m_luminanceQuadCoords[MAX_LUM_TEX], m_fp16Lum[MAX_LUM_TEX-1].pTex ));

	// Temporal adjustment of the luminance
	m_iPreviousLuminance = !m_iPreviousLuminance;
	V( LuminanceAdaptation( m_fp32Lum[!m_iPreviousLuminance].pSurf, m_fp32Lum[m_iPreviousLuminance].pTex, m_fp32Lum[2].pTex, frameDelta ));

	return hr;
}

//-----------------------------------------------------------------------------
// Name: ToneMapping
//-----------------------------------------------------------------------------
HRESULT ID3DRenderer::ToneMapping(LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 channelB)
{
	HRESULT hr = S_OK;

	LPD3DXEFFECT pHDREffect = g_EffectManager.GetEffect( m_SharedEffectName );
	assert( pHDREffect );

	// Set exposure
	V( pHDREffect->SetValue("g_fExposure", &m_fExposure, sizeof(FLOAT) ));
	V( pHDREffect->SetTexture( "TextureC", m_fp32Lum[!m_iPreviousLuminance].pTex ));
	
	if( m_bUse2xFp )
	{
		V( pHDREffect->SetTexture  ( "TextureA", channelA ));
		V( pHDREffect->SetTexture  ( "TextureB", channelB ));	
		V( pHDREffect->SetTexture  ( "ScaledTexA", m_fp16ShrinkA.pTex ));
		V( pHDREffect->SetTexture  ( "ScaledTexB", m_fp16ShrinkB.pTex ));
		V( pHDREffect->SetTechnique( "ToneMappingx2" ));
	}
	else
	{
		V( pHDREffect->SetTexture("TextureA", channelA ));
		V( pHDREffect->SetTexture("ScaledTexA", m_fp16ShrinkF.pTex ));
		V( pHDREffect->SetTechnique("ToneMapping"));
	}
	
	// Use sRGB to produce the proper gamma correction
	V( m_pDevice->SetRenderState ( D3DRS_SRGBWRITEENABLE, TRUE ));
	V( m_pDevice->SetRenderTarget( 0, m_pBackBufferSurf ));

	UINT iPass, iPasses;
	V( pHDREffect->Begin(&iPasses, 0));
	for(iPass = 0; iPass < iPasses; iPass++)
	{
		V( pHDREffect->BeginPass(iPass));
		V( DrawNVIDIAQuad( m_fullscreenVTFTonemapPass, m_pVertexDecFVF0 , sizeof(ScreenVertex0))); // Full screen
		V( pHDREffect->EndPass());
	}

	V( pHDREffect->End());

	V( m_pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE));
	return hr;

}


//-----------------------------------------------------------------------------
// Name: glowPass
//-----------------------------------------------------------------------------
HRESULT ID3DRenderer::GlowPass( LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 channelB )
{
	HRESULT hr = S_OK;

	if( m_bUse2xFp )
	{
		// blur horizontally and then vertically the first RT and then the second one
		V( BlurTarget( m_fp16ShrinkA_2.pSurf, false, m_fp16ShrinkA.pTex ));
		V( BlurTarget( m_fp16ShrinkA.pSurf  , true,  m_fp16ShrinkA_2.pTex ));

		V( BlurTarget( m_fp16ShrinkA_2.pSurf, false, m_fp16ShrinkB.pTex ));
		V( BlurTarget( m_fp16ShrinkB.pSurf,   true,  m_fp16ShrinkA_2.pTex ));	
	}
	else
	{
		V( BlurTarget( m_fp16ShrinkF_2.pSurf, false, m_fp16ShrinkF.pTex ));
		V( BlurTarget( m_fp16ShrinkF.pSurf,   true,  m_fp16ShrinkF_2.pTex ));
	}
	
	return hr;
}

//-----------------------------------------------------------------------------
// Name: DownscaleScene()
//-----------------------------------------------------------------------------
HRESULT ID3DRenderer::DownscaleSceneAniso( LPDIRECT3DSURFACE9 RT0, LPDIRECT3DSURFACE9 RT_int, LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 Tex_int )
{	
	HRESULT hr = S_OK;

	LPD3DXEFFECT pHDREffect = g_EffectManager.GetEffect( m_SharedEffectName );
	assert( pHDREffect );

	// First pass (w,h) -> (w/4, h/2) 
	V( pHDREffect->SetTechnique("DownscaleAniso"));
	V( pHDREffect->SetTexture  ("TextureA", channelA));
	UINT uiPassCount, uiPass;       

	V( m_pDevice->SetRenderTarget( 0, RT_int ));

	V( pHDREffect->Begin(&uiPassCount, 0));
	for (uiPass = 0; uiPass < uiPassCount; uiPass++)
	{
		V( pHDREffect->BeginPass(uiPass));
		V( DrawNVIDIAQuad( m_downsampleAnisoQuadCoords[0], m_pVertexDecFVF1, sizeof(ScreenVertex1)) );
		V( pHDREffect->EndPass());
	}
	V( pHDREffect->End());

	// Second pass (w/4,h/2) -> (w/4, h/4)
	V( pHDREffect->SetTechnique("DownscaleAniso"));
	V( pHDREffect->SetTexture  ("TextureA", Tex_int));

	V( m_pDevice->SetRenderTarget( 0, RT0 ) );

	V( pHDREffect->Begin(&uiPassCount, 0));
	for (uiPass = 0; uiPass < uiPassCount; uiPass++)
	{
		V( pHDREffect->BeginPass(uiPass));
		V( DrawNVIDIAQuad(m_downsampleAnisoQuadCoords[1], m_pVertexDecFVF1, sizeof(ScreenVertex1) ));
		V( pHDREffect->EndPass());
	}
	V( pHDREffect->End());


	return hr;
}


//-----------------------------------------------------------------------------
// Name: DownscaleScene()
//-----------------------------------------------------------------------------
HRESULT ID3DRenderer::LuminancePass(LPDIRECT3DSURFACE9 RT0, int method, ScreenVertex4 *vertArray, LPDIRECT3DTEXTURE9 channelA, LPDIRECT3DTEXTURE9 channelB)
{	
	HRESULT hr = S_OK;

	LPD3DXEFFECT pHDREffect = g_EffectManager.GetEffect( m_SharedEffectName );
	assert( pHDREffect );

	switch( method )
	{
	case LUMINANCE_DOWNSAMPLE_LOG:
		if( channelB )
		{
			V( pHDREffect->SetTechnique("DownscaleLuminanceLogx2"));
		}
		else
		{
			V( pHDREffect->SetTechnique("DownscaleLuminanceLog"));
		}
		break;
	case LUMINANCE_DOWNSAMPLE:
		V( pHDREffect->SetTechnique("DownscaleLuminance"));
		break;     
	case LUMINANCE_DOWNSAMPLE_EXP:
		V( pHDREffect->SetTechnique("DownscaleLuminanceExp"));
		break; 
	}

	// Set source textures
	V( pHDREffect->SetTexture  ("TextureA", channelA));

	if(channelB)
		V( pHDREffect->SetTexture  ("TextureB", channelB));

	UINT uiPassCount, uiPass;       

	V( m_pDevice->SetRenderTarget( 0, RT0  ));

	V( pHDREffect->Begin(&uiPassCount, 0));
	for (uiPass = 0; uiPass < uiPassCount; uiPass++)
	{
		V( pHDREffect->BeginPass(uiPass));
		V( DrawNVIDIAQuad(vertArray, m_pVertexDecFVF4, sizeof(ScreenVertex4)));
		V( pHDREffect->EndPass());
	}
	V( pHDREffect->End());
	
	return hr;
}


//-----------------------------------------------------------------------------
// Name: LuminanceAdaption
//-----------------------------------------------------------------------------
HRESULT ID3DRenderer::LuminanceAdaptation(LPDIRECT3DSURFACE9 dst, LPDIRECT3DTEXTURE9 src1, LPDIRECT3DTEXTURE9 src2, const float frameDelta)
{	
	HRESULT hr = S_OK;

	LPD3DXEFFECT pHDREffect = g_EffectManager.GetEffect( m_SharedEffectName );
	assert( pHDREffect );

	UINT uiPassCount, uiPass;       

	V( pHDREffect->SetTechnique( "LuminanceAdaptation" ));

	V( pHDREffect->SetValue ( "g_fFrameDelta", &frameDelta, sizeof(FLOAT)));

	// Set source/dest textures
	V( pHDREffect->SetTexture  ("TextureA", src1));
	V( pHDREffect->SetTexture  ("TextureB", src2));

	V( m_pDevice->SetRenderTarget( 0, dst));

	V( pHDREffect->Begin(&uiPassCount, 0));
	for (uiPass = 0; uiPass < uiPassCount; uiPass++)
	{
		V( pHDREffect->BeginPass(uiPass));
		V( DrawNVIDIAQuad(m_fullscreenQuadCoords, m_pVertexDecFVF1, sizeof(ScreenVertex1)));
		V( pHDREffect->EndPass());
	}
	V( pHDREffect->End());

    return hr;
}

//-----------------------------------------------------------------------------
// Name: BlurTarget()
//-----------------------------------------------------------------------------
HRESULT ID3DRenderer::BlurTarget(LPDIRECT3DSURFACE9 RT0, bool verticalDir,  LPDIRECT3DTEXTURE9 channelA)
{	
	HRESULT hr = S_OK;

	LPD3DXEFFECT pHDREffect = g_EffectManager.GetEffect( m_SharedEffectName );
	assert( pHDREffect );

	ScreenVertex8 *coords;

	if( verticalDir )
	{
		coords = m_blurQuadCoordsV;
	}
	else
	{
		coords = m_blurQuadCoordsH;
	}

	V( pHDREffect->SetTechnique("Blur1DBilinear"));
	V( pHDREffect->SetValue    ("weights", m_gaussWeights, sizeof(m_gaussWeights)));
	V( pHDREffect->SetTexture  ("TextureA", channelA));

	V( m_pDevice->SetRenderTarget( 0, RT0 ));

	UINT uiPassCount, uiPass;       
	V( pHDREffect->Begin(&uiPassCount, 0));

	for (uiPass = 0; uiPass < uiPassCount; uiPass++)
	{
		V( pHDREffect->BeginPass(uiPass));
		V( DrawNVIDIAQuad(coords, m_pVertexDecFVF8, sizeof(ScreenVertex8)));
		V( pHDREffect->EndPass());
	}

	V( pHDREffect->End());
	return hr;

}


//-----------------------------------------------------------------------------
// Name: CalculateOffsetsAndQuadCoords
// Desc: Everytime we resize the screen we will recalculate an array of
// vertices and texture coordinates to be use to render the full screen 
// primitive for the different postprocessing effects. Note that different 
// effects use a different set of values! 
//-----------------------------------------------------------------------------
void ID3DRenderer::CalculateOffsetsAndQuadCoords()
{
	
	D3DXVECTOR2 sampleOffsets_scale[MAX_SAMPLES];
	D3DXVECTOR2 sampleOffsets_blurV[MAX_SAMPLES];
	D3DXVECTOR2 sampleOffsets_blurH[MAX_SAMPLES];

	float coordOffsets[MAX_SAMPLES];
	float dU;
	float dV;
	CoordRect tempCoords;

	// Full screen quad coords
	tempCoords.fLeftU   = 0.0f;
	tempCoords.fTopV    = 0.0f;
	tempCoords.fRightU  = 1.0f;
	tempCoords.fBottomV = 1.0f;

	ApplyTexelToPixelCorrection(m_fullscreenVTFTonemapPass, m_iWidth, m_iHeight, &tempCoords);

	ApplyTexelToPixelCorrection(m_fullscreenQuadCoords, m_iWidth, m_iHeight, &tempCoords);

	ApplyTexelToPixelCorrection(m_downsampleAnisoQuadCoords[0], m_iShrinkWidth, m_iHeight/2, &tempCoords);
	ApplyTexelToPixelCorrection(m_downsampleAnisoQuadCoords[1], m_iShrinkWidth, m_iShrinkHeight, &tempCoords);

	// Luminance quad coords
	// (w/4, h/4), (64, 64), (16, 16) and (4, 4)
	CalculateSampleOffset_4x4Bilinear( m_iShrinkWidth, m_iShrinkHeight, sampleOffsets_scale );
	ApplyTexelToPixelCorrection(m_luminanceQuadCoords[0], 64, 64, &tempCoords, sampleOffsets_scale);

	CalculateSampleOffset_4x4Bilinear( 64, 64, sampleOffsets_scale );
	ApplyTexelToPixelCorrection(m_luminanceQuadCoords[1], 16, 16, &tempCoords, sampleOffsets_scale);

	CalculateSampleOffset_4x4Bilinear( 16, 16, sampleOffsets_scale );
	ApplyTexelToPixelCorrection(m_luminanceQuadCoords[2], 4, 4, &tempCoords, sampleOffsets_scale);

	CalculateSampleOffset_4x4Bilinear( 4, 4, sampleOffsets_scale );
	ApplyTexelToPixelCorrection(m_luminanceQuadCoords[3], 1, 1, &tempCoords, sampleOffsets_scale);


	// Downsampling quad coords
	// Place the rectangle in the center of the back buffer surface
	// Get the texture coordinates for the render target
	dU = 1.0f / m_iWidth;
	dV = 1.0f / m_iHeight;

	tempCoords.fLeftU   = 0.5f * (m_iWidth  - m_iCroppedWidth)  * dU;
	tempCoords.fTopV    = 0.5f * (m_iHeight - m_iCroppedHeight) * dV;
	tempCoords.fRightU  = 1.0f - (m_iWidth  - m_iCroppedWidth)  * dU;
	tempCoords.fBottomV = 1.0f - (m_iHeight - m_iCroppedHeight) * dV;

	CalculateSampleOffset_4x4Bilinear( m_iWidth, m_iHeight, sampleOffsets_scale );
	ApplyTexelToPixelCorrection(m_downsampleQuadCoords, m_iShrinkWidth, m_iShrinkHeight, &tempCoords, sampleOffsets_scale);


	// Blur quad coords.
	tempCoords.fLeftU   = 0.0f;
	tempCoords.fTopV    = 0.0f;
	tempCoords.fRightU  = 1.0f;
	tempCoords.fBottomV = 1.0f;

	CalculateOffsets_GaussianBilinear( (float)m_iShrinkWidth, coordOffsets, m_gaussWeights, 8);
	for(int i = 0; i < MAX_SAMPLES; i++)
	{
		sampleOffsets_blurH[i].x = coordOffsets[i];
		sampleOffsets_blurH[i].y = 0;

		sampleOffsets_blurV[i].x = 0;
		sampleOffsets_blurV[i].y = coordOffsets[i] * m_iShrinkWidth / m_iShrinkHeight;
	}
	ApplyTexelToPixelCorrection(m_blurQuadCoordsV, m_iShrinkWidth, m_iShrinkHeight, &tempCoords, sampleOffsets_blurV);	
	ApplyTexelToPixelCorrection(m_blurQuadCoordsH, m_iShrinkWidth, m_iShrinkHeight, &tempCoords, sampleOffsets_blurH);	
}

//-----------------------------------------------------------------------------
// Name: applyTexelToPixelCorrection
//
// Desc: These set of function will calculate the proper coordinates for the 
// full screen scissored triangle, adding the appropiate offsets (for 
// downsampling and blur) as full coordinates in the texture values. This will
// allow to avoid the calculation of coordinates in the shaders.
//-----------------------------------------------------------------------------
void ID3DRenderer::ApplyTexelToPixelCorrection(ScreenVertex0 dst[], int srcW, int srcH, CoordRect *coords){

	// Ensure that we're directly mapping texels to pixels by offset by 0.5
	// For more info see the doc page titled "Directly Mapping Texels to Pixels"
	float tWidth   = coords->fRightU  - coords->fLeftU;
	float tHeight  = coords->fBottomV - coords->fTopV;

	float dS = 1.0f/srcW;
	float dT = 1.0f/srcH;

	dst[0].p = D3DXVECTOR4(-1.0f - dS,  1.0f + dT, coords->fLeftU, coords->fTopV);
	dst[1].p = D3DXVECTOR4( 3.0f - dS,  1.0f + dT, coords->fLeftU + (tWidth*2.f), coords->fTopV);
	dst[2].p = D3DXVECTOR4(-1.0f - dS, -3.0f + dT, coords->fLeftU, coords->fTopV + (tHeight*2.f));
}

void ID3DRenderer::ApplyTexelToPixelCorrection(ScreenVertex1 dst[], int srcW, int srcH, CoordRect *coords){

	// Ensure that we're directly mapping texels to pixels by offset by 0.5
	// For more info see the doc page titled "Directly Mapping Texels to Pixels"
	float fWidth5  = (2.f*(float)srcW) - 0.5f;
	float fHeight5 = (2.f*(float)srcH) - 0.5f;
	float tWidth   = coords->fRightU  - coords->fLeftU;
	float tHeight  = coords->fBottomV - coords->fTopV;

	dst[0].p = D3DXVECTOR4(-0.5f, -0.5f, 0.5f, 1.0f);
	dst[0].t = D3DXVECTOR2(coords->fLeftU, coords->fTopV);

	dst[1].p = D3DXVECTOR4(fWidth5, -0.5f, 0.5f, 1.f);
	dst[1].t = D3DXVECTOR2(coords->fLeftU + (tWidth*2.f), coords->fTopV);

	dst[2].p = D3DXVECTOR4(-0.5f, fHeight5, 0.5f, 1.f);
	dst[2].t = D3DXVECTOR2(coords->fLeftU, coords->fTopV + (tHeight*2.f));
}

//-----------------------------------------------------------------------------
void ID3DRenderer::ApplyTexelToPixelCorrection(ScreenVertex4 dst[], int srcW, int srcH, CoordRect *coords, D3DXVECTOR2 offsets[]){

	// Ensure that we're directly mapping texels to pixels by offset by 0.5
	// For more info see the doc page titled "Directly Mapping Texels to Pixels"
	float fWidth5  = (2.f*(float)srcW) - 0.5f;
	float fHeight5 = (2.f*(float)srcH) - 0.5f;
	float tWidth   = coords->fRightU  - coords->fLeftU;
	float tHeight  = coords->fBottomV - coords->fTopV;

	// Position
	dst[0].p  = D3DXVECTOR4(  -0.5f,    -0.5f, 0.5f, 1.0f);
	dst[1].p  = D3DXVECTOR4(fWidth5,    -0.5f, 0.5f, 1.0f);
	dst[2].p  = D3DXVECTOR4(  -0.5f, fHeight5, 0.5f, 1.0f);

	// Offsets
	float tempTexCoords[8];
	float tempX[3], tempY[3];
	int i, j;

	tempX[0] = coords->fLeftU;
	tempY[0] = coords->fTopV;

	tempX[1] = coords->fLeftU + (tWidth*2.f);
	tempY[1] = coords->fTopV;

	tempX[2] = coords->fLeftU;
	tempY[2] = coords->fTopV + (tHeight*2.f);

	for(i = 0; i < 3; i++){
		for(j = 0; j < 4; j++ ){
			tempTexCoords[2*j+0] = tempX[i] + offsets[j].x;
			tempTexCoords[2*j+1] = tempY[i] + offsets[j].y;
		}
		dst[i].t[0] = D3DXVECTOR4(&tempTexCoords[0]);
		dst[i].t[1] = D3DXVECTOR4(&tempTexCoords[4]);
	}
}


//-----------------------------------------------------------------------------
void ID3DRenderer::ApplyTexelToPixelCorrection(ScreenVertex8 dst[], int srcW, int srcH, CoordRect *coords, D3DXVECTOR2 offsets[]){

	// Ensure that we're directly mapping texels to pixels by offset by 0.5
	// For more info see the doc page titled "Directly Mapping Texels to Pixels"
	float fWidth5  = (2.f*(float)srcW) - 0.5f;
	float fHeight5 = (2.f*(float)srcH) - 0.5f;
	float tWidth   = coords->fRightU - coords->fLeftU;
	float tHeight  = coords->fBottomV - coords->fTopV;

	// Position
	dst[0].p  = D3DXVECTOR4(  -0.5f,    -0.5f, 0.5f, 1.0f);
	dst[1].p  = D3DXVECTOR4(fWidth5,    -0.5f, 0.5f, 1.0f);
	dst[2].p  = D3DXVECTOR4(  -0.5f, fHeight5, 0.5f, 1.0f);

	// Offsets
	float tempTexCoords[16];
	float tempX[3], tempY[3];
	int i, j;

	tempX[0] = coords->fLeftU;
	tempY[0] = coords->fTopV;

	tempX[1] = coords->fLeftU + (tWidth*2.f);
	tempY[1] = coords->fTopV;

	tempX[2] = coords->fLeftU;
	tempY[2] = coords->fTopV + (tHeight*2.f);

	for(i = 0; i < 3; i++){
		for(j = 0; j < 8; j++ ){
			tempTexCoords[2*j+0] = tempX[i] + offsets[j].x;
			tempTexCoords[2*j+1] = tempY[i] + offsets[j].y;
		}
		dst[i].t[0] = D3DXVECTOR4(&tempTexCoords[0]);
		dst[i].t[1] = D3DXVECTOR4(&tempTexCoords[4]);
		dst[i].t[2] = D3DXVECTOR4(&tempTexCoords[8]);
		dst[i].t[3] = D3DXVECTOR4(&tempTexCoords[12]);
	}
}


//-----------------------------------------------------------------------------
// Name: CalculateSampleOffset_4x4Bilinear
// Desc: 
//-----------------------------------------------------------------------------
void ID3DRenderer::CalculateSampleOffset_4x4Bilinear( DWORD dwWidth, DWORD dwHeight, D3DXVECTOR2 avSampleOffsets[] )
{
	float tU = 1.0f / dwWidth;
	float tV = 1.0f / dwHeight;

	// Sample from the 16 surrounding points.  Since bilinear filtering is being used, specific the coordinate
	// exactly halfway between the current texel center (k-1.5) and the neighboring texel center (k-0.5)

	int index=0;
	for( int y=0; y < 4; y+=2 )
	{
		for( int x=0; x < 4; x+=2, index++ )
		{
			avSampleOffsets[ index ].x = (x - 1.f) * tU;
			avSampleOffsets[ index ].y = (y - 1.f) * tV;
		}
	}
}

//-----------------------------------------------------------------------------
// Name: CalculateOffsets_GaussianBilinear
//
//  We want the general convolution:
//    a*f(i) + b*f(i+1)
//  Linear texture filtering gives us:
//    f(x) = (1-alpha)*f(i) + alpha*f(i+1);
//  It turns out by using the correct weight and offset we can use a linear lookup to achieve this:
//    (a+b) * f(i + b/(a+b))
//  as long as 0 <= b/(a+b) <= 1.
//
//  Given a standard deviation, we can calculate the size of the kernel and viceversa.
//
//-----------------------------------------------------------------------------
void ID3DRenderer::CalculateOffsets_GaussianBilinear( float texSize, float *coordOffset, float *gaussWeight, int kernelRadius )
{
	int i=0;
	float du = 1.0f / texSize;

	//  store all the intermediate offsets & weights, then compute the bilinear
	//  taps in a second pass
	float *tmpWeightArray = GenerateGaussianWeights(kernelRadius);
	float *tmpOffsetArray = new float[2*kernelRadius];

	// Fill the offsets
	for( i = 0; i < kernelRadius; i++ )
	{
		tmpOffsetArray[i]                = -(float)(kernelRadius - i) * du;
		tmpOffsetArray[i + kernelRadius] =  (float)(i + 1) * du;
	}

	// Bilinear filtering taps 
	// Ordering is left to right.
	for( i=0; i < kernelRadius; i++ )
	{
		float sScale   = tmpWeightArray[i*2] + tmpWeightArray[i*2+1];
		float sFrac    = tmpWeightArray[i*2] / sScale;

		coordOffset[i] = (tmpOffsetArray[i*2] + (1-sFrac))*du;
		gaussWeight[i] = sScale;
	}
}


//-----------------------------------------------------------------------------
// Calculate Gaussian weights based on kernel size
//-----------------------------------------------------------------------------
// generate array of weights for Gaussian blur
float* ID3DRenderer::GenerateGaussianWeights( int kernelRadius )
{
	int i;
	int size = kernelRadius*2;

	float x;
	float s        = kernelRadius / 3.0f;  
	float *weights = new float [size];

	float sum = 0.0;
	for(i=0; i < size; i++) 
	{
		x          = (float) i - kernelRadius + 0.5;
		weights[i] = (float)( exp( -x * x / ( 2 * s * s ) ) / ( s * sqrt( 2 * D3DX_PI )));
		sum       += weights[i];
	}

	for(i=0; i < size; i++) {
		weights[i] /= sum;
	}
	return weights;
}


HRESULT ID3DRenderer::DebugShowLightMeshWireframes( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hWorldView		= pEffect->GetParameterBySemantic( NULL, "WorldView" );
	D3DXHANDLE hWVP				= pEffect->GetParameterBySemantic( NULL, "WorldViewProjection" );
	D3DXHANDLE hWireFrameColour	= pEffect->GetParameterByName( NULL, "g_fvDebugWireframeColour" );

	D3DXMATRIX matView;
	pCamera->GetViewMatrix( &matView );

	D3DXMATRIX matProjPersp;
	pCamera->GetProjMatrix( &matProjPersp );

	D3DXHANDLE hDebugShowLights = pEffect->GetTechniqueByName( "Debug_ShowWireframe" );
	assert( hDebugShowLights );

	V( pEffect->SetTechnique( hDebugShowLights ));

	UINT uiNumPasses;

	V( pEffect->Begin( &uiNumPasses, 0 ));
	V( pEffect->BeginPass( 0 ));

	for( list< LightOmni_s* >::iterator i = pScene->m_pLightOmni.begin(); i != pScene->m_pLightOmni.end(); i++ )
	{
		LightOmni_s* pLight = *i;

		if( ! pLight->m_bActive )
			continue;

		if( ! pLight->m_bVisible )
			continue;

		// set light position in world & transform into view space then set up light colour
		D3DXVECTOR3 lightWorldPos = pLight->m_position;

		// scale light volume mesh (temp)
		D3DXMATRIX scale, translate, sphereMatWVP;
		float fScale = pLight->m_fMaxRange;
		D3DXMatrixTranslation( &translate, lightWorldPos.x, lightWorldPos.y, lightWorldPos.z );
		D3DXMatrixScaling( &scale, fScale, fScale, fScale );

		sphereMatWVP = scale * translate * matView * matProjPersp;
		V( pEffect->SetMatrix( hWVP, &sphereMatWVP ));
		D3DXVECTOR3 displayColour = D3DXVECTOR3( pLight->m_colour.x, pLight->m_colour.y, pLight->m_colour.z );
		// if our light's colour is outwith the displayable range, normalise the wireframe colour to aid in visualisation
		if( displayColour.x > 1.0f || displayColour.y > 1.0f || displayColour.z > 1.0f )
		{
			D3DXVec3Normalize( &displayColour, &displayColour )			;            			
		}

		D3DXVECTOR4 finalColour( displayColour.x, displayColour.y, displayColour.z, 1.0f );

		V( pEffect->SetVector( hWireFrameColour, &finalColour ));		

		V( m_pDevice->SetVertexDeclaration( m_pSphereMeshVertexDeclaration ));
		V( pEffect->CommitChanges( ));
		V( m_pSphereMesh->DrawSubset( 0 ));
	}						

	for( list< LightSpot_s* >::iterator i = pScene->m_pLightSpot.begin(); i != pScene->m_pLightSpot.end(); i++ )
	{
		LightSpot_s* pLight = (*i);

		if( ! pLight->m_bActive )
			continue;

		if( ! pLight->m_bVisible )
			continue;

		// set light position in world & transform into view space then set up light colour
		D3DXVECTOR3 lightWorldPos = pLight->m_position;

		D3DXMATRIX rotate, translate, matWVP;

		D3DXMatrixRotationYawPitchRoll( &rotate, pLight->m_fYaw, pLight->m_fPitch, 0.0f );
		D3DXMatrixTranslation( &translate, lightWorldPos.x, lightWorldPos.y, lightWorldPos.z );

		matWVP = rotate * translate * matView * matProjPersp;
		V( pEffect->SetMatrix( hWVP, &matWVP ));

		D3DXVECTOR3 displayColour = D3DXVECTOR3( pLight->m_colour.x, pLight->m_colour.y, pLight->m_colour.z );
		// if our light's colour is outwith the displayable range, normalise the wireframe colour to aid in visualisation
		if( displayColour.x > 1.0f || displayColour.y > 1.0f || displayColour.z > 1.0f )
		{
            D3DXVec3Normalize( &displayColour, &displayColour )			;            			
		}

		D3DXVECTOR4 finalColour( displayColour.x, displayColour.y, displayColour.z, 1.0f );

		V( pEffect->SetVector( hWireFrameColour, &finalColour ));

		V( m_pDevice->SetVertexDeclaration( pScene->m_pPosVertDec ));
		V( pEffect->CommitChanges( ));		
		V( pLight->m_pConeMesh->DrawSubset( 0 ));		
	}	

	V( pEffect->EndPass() );
	V( pEffect->End() );

	return S_OK;
}

HRESULT ID3DRenderer::DebugShowBoundingSphereWireframes( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hWorldView				= pEffect->GetParameterBySemantic( NULL, "WorldView" );
	D3DXHANDLE hWVP						= pEffect->GetParameterBySemantic( NULL, "WorldViewProjection" );
	D3DXHANDLE hDebugWireframeColour	= pEffect->GetParameterByName( NULL, "g_fvDebugWireframeColour" );
	D3DXHANDLE hDebugShowWireFrame		= pEffect->GetTechniqueByName( "Debug_ShowWireframe" );
	assert( hDebugShowWireFrame );

	D3DXMATRIX matView;
	pCamera->GetViewMatrix( &matView );

	D3DXMATRIX matProjPersp;
	pCamera->GetProjMatrix( &matProjPersp );

	V( pEffect->SetTechnique( hDebugShowWireFrame ));

	V( pEffect->SetVector( hDebugWireframeColour, &D3DXVECTOR4(1.0f, 1.0f, 0.0f, 1.0f  )));

	UINT uiNumPasses;
	V( pEffect->Begin( &uiNumPasses, 0 ));
	V( pEffect->BeginPass( 0 ));

	for( list< CNode* >::iterator i = pScene->m_pNodes.begin(); i != pScene->m_pNodes.end(); i++ )
	{
		CNode* pNode = *i;

		// set light position in world & transform into view space then set up light colour
		D3DXVECTOR3 boundingSphereCentre = pNode->m_BoundingSphere.m_Position;
		float fBoundingSphereRadius = pNode->m_BoundingSphere.m_radius;

		// scale light volume 
		D3DXMATRIX scale, translate, sphereMatWVP;
		float fScale = fBoundingSphereRadius;
		D3DXMatrixTranslation( &translate, boundingSphereCentre.x, boundingSphereCentre.y, boundingSphereCentre.z );
		D3DXMatrixScaling( &scale, fScale, fScale, fScale );

		sphereMatWVP = scale * translate * matView * matProjPersp;
		V( pEffect->SetMatrix( hWVP, &sphereMatWVP ));

		V( m_pDevice->SetVertexDeclaration( m_pSphereMeshVertexDeclaration ));
		V( pEffect->CommitChanges( ));
		V( m_pSphereMesh->DrawSubset( 0 ));
	}						

	V( pEffect->EndPass());
	V( pEffect->End());

	return S_OK;
}

HRESULT ID3DRenderer::DebugShowShadowMap( LPD3DXEFFECT pEffect )
{
	HRESULT hr = S_OK;

	D3DXHANDLE hShowShadowMap	= pEffect->GetTechniqueByName( "DisplayHardwareShadowMap" );
	assert( hShowShadowMap );

	V( pEffect->SetTechnique( hShowShadowMap ));

	UINT uiNumPasses;
	V( pEffect->Begin( &uiNumPasses, 0 ));
	V( pEffect->BeginPass( 0 ));

	V( pEffect->SetTexture( "shadowColourMapRT_Tex",	m_ShadowMapColourRT.pTex ));

	V( DrawNVIDIAQuad( m_fullscreenQuadCoords, m_pVertexDecFVF1, sizeof( ScreenVertex1 )));

	V( pEffect->EndPass() );
	V( pEffect->End() );

	return hr;
}


//--------------------------------------------------------------//
// CDummyRenderer
//--------------------------------------------------------------//

CDummyRenderer::CDummyRenderer()
{

}

CDummyRenderer::~CDummyRenderer()
{

}

HRESULT CDummyRenderer::OneTimeInit( int iWidth, int iHeight, HWND hWnd, bool bWindowed, LPDIRECT3DDEVICE9 pDevice )
{
	return S_OK;
}

HRESULT CDummyRenderer::Cleanup( )
{	
	return S_OK;
}

HRESULT CDummyRenderer::Render( Camera* pCamera, CScene* pScene, float fTimeDelta )
{
	return S_OK;
}

HRESULT CDummyRenderer::DrawObjects( Camera* pCamera, CNode* pNode, LPD3DXEFFECT pEffect )
{	
	return S_OK;
}

HRESULT CDummyRenderer::OnResetDevice( )
{
	return S_OK;
}

HRESULT CDummyRenderer::OnLostDevice( )
{
	return S_OK;
}

HRESULT CDummyRenderer::LightAmbient( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pEffect )
{
	return S_OK;
}

HRESULT CDummyRenderer::LightDirectionals( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )
{
	return S_OK;
}

HRESULT CDummyRenderer::LightOmnis( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )
{
	return S_OK;
}

HRESULT CDummyRenderer::LightSpots( Camera* pCamera, CScene* pScene, LPD3DXEFFECT pDeferred, LPD3DXEFFECT pShadow )
{
	return S_OK;
}