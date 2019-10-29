#line 2 "deferred.fx"
//--------------------------------------------------------------//
// Deferred Shading
//--------------------------------------------------------------//

// Non Tweakable 

const float2 g_fScreenSize : VIEWPORTPIXELSIZE;
const float2 g_fUVAdjust;

// Bools to turn on/off features using static branching

const bool	g_bCastsShadows;
const bool	g_bUseHDR;
const bool	g_bEmissive;

// Lights

const float4 g_fvAmbientColour;
const float4 g_fvLightColour;
const float  g_fLightMaxRange;

const float3 g_fvViewSpaceLightDirection;
const float3 g_fvViewSpaceLightPosition;

const float g_fInnerAngle;
const float g_fOuterAngle;
const float g_fSpotlightFalloff;

const float3 g_fvFogColour;
const float  g_fFogStart;
const float	 g_fFogEnd;

// spec
float g_fSpecularPower	= float( 25.00 );

// Common matrices
const float4x4 matWorld					: World;
const float4x4 matView					: View;
const float4x4 matWorldView				: WorldView;
const float4x4 matWorldViewProjection	: WorldViewProjection;

// Specialised matrices
const float4x4 matViewToLightProjSpace;


//--------------------------------------------------------------//
// Textures & Samplers
//--------------------------------------------------------------//

// Base Textures
texture base_Tex;
texture normal_Tex;

// Shadow tex
texture shadowMapRT_Tex;		// hardware shadow map texture (depth stencil)
texture shadowColourMapRT_Tex;	// colour shadow map texture (debugging)

// G-Buffer Textures
texture diffuseRT_Tex;
texture positionRT_Tex;
texture normalRT_Tex;

sampler2D skyboxSampler = sampler_state
{
	Texture		= (base_Tex);
	AddressU	= CLAMP;
	AddressV	= CLAMP;
	MINFILTER	= LINEAR;
	MAGFILTER	= LINEAR;
	MIPFILTER	= LINEAR;	
};

// Samplers for model textures etc.
sampler2D baseMap = sampler_state
{
   Texture		= (base_Tex);
   ADDRESSU		= WRAP;
   ADDRESSV		= WRAP;
   MINFILTER	= LINEAR;
   MAGFILTER	= LINEAR;
   MIPFILTER	= LINEAR;
};

sampler2D normalMap = sampler_state
{
	Texture		= (normal_Tex);
	ADDRESSU	= WRAP;
	ADDRESSV	= WRAP;
	MINFILTER	= LINEAR;
	MAGFILTER	= LINEAR;
	MIPFILTER	= LINEAR;
};

// Render Target samplers

sampler2D diffuseSampler = sampler_state
{
   Texture		= (diffuseRT_Tex);
   AddressU		= CLAMP;        
   AddressV		= CLAMP;
   MINFILTER	= POINT;
   MAGFILTER	= POINT;
   MIPFILTER	= NONE;
};

sampler2D positionSampler = sampler_state
{
   Texture		= (positionRT_Tex);
   AddressU		= CLAMP;        
   AddressV		= CLAMP;
   MINFILTER	= POINT;
   MAGFILTER	= POINT;
   MIPFILTER	= NONE;
};

sampler2D normalSampler = sampler_state
{
   Texture		= (normalRT_Tex);
   AddressU		= CLAMP;        
   AddressV		= CLAMP;
   MINFILTER	= POINT;
   MAGFILTER	= POINT;
   MIPFILTER	= NONE;
};


// Hardware shadow map (depth stencil) texture
sampler shadowMapSampler = sampler_state
{
    texture = (shadowMapRT_Tex);
    AddressU  = CLAMP;      
    AddressV  = CLAMP;
    MINFILTER = LINEAR;
    MAGFILTER = LINEAR;
    MIPFILTER = NONE;
    SRGBTEXTURE = FALSE;
};

// Colour shadow map (regular texture, used for debugging or if hardware shadow map not supported)
sampler shadowColourMapSampler = sampler_state
{
    texture = (shadowColourMapRT_Tex);
    AddressU  = CLAMP;      
    AddressV  = CLAMP;
    MINFILTER = LINEAR;
    MAGFILTER = LINEAR;
    MIPFILTER = NONE;
    SRGBTEXTURE = FALSE;
};

//--------------------------------------------------------------//
// G-Buffer Pass
//--------------------------------------------------------------//

struct POSTEX
{
	float4 Position		: POSITION0;
	float2 TexCoords	: TEXCOORD0;
};

struct VS_INPUT_POSTEXNORM
{
   float4 Position	: POSITION0;   
   float2 TexCoords	: TEXCOORD0;
   float3 Normal	: NORMAL0;
};

struct VS_INPUT_NORMALMAPPING
{
   float4 Position	: POSITION0;   
   float2 TexCoords	: TEXCOORD0;
   float3 Normal	: NORMAL0;
   float3 Binormal	: BINORMAL0;
   float3 Tangent	: TANGENT0;
};

struct VS_OUTPUT_POSPOSTEXVIEW
{
   float4 PosClip	: POSITION0;   
   float4 PosView	: TEXCOORD0;
   float2 TexCoords	: TEXCOORD1;
   float3 NormalView : TEXCOORD2;
};

struct VS_OUTPUT_NORMALMAPPING
{
	float4 PosClip		: POSITION0;   
	float4 PosView		: TEXCOORD0;
	float2 TexCoords	: TEXCOORD1;
	float3 TangentToView0 : TEXCOORD2;
	float3 TangentToView1 : TEXCOORD3;
	float3 TangentToView2 : TEXCOORD4;	
};

struct PS_INPUT_POSTEXVIEW
{
	float4 PosView		: TEXCOORD0;
	float2 TexCoords	: TEXCOORD1;
	float3 NormalView	: TEXCOORD2;
};

struct PS_INPUT_NORMALMAPPING
{
	float4 PosView		: TEXCOORD0;
	float2 TexCoords	: TEXCOORD1;
	float3 TangentToView0 : TEXCOORD2;
	float3 TangentToView1 : TEXCOORD3;
	float3 TangentToView2 : TEXCOORD4;
};

struct PS_OUTPUT_MRT	// Generalised so I don't have to mess with it if I decide to change the layout
{
	float4 MRT0			: COLOR0;	// DIFFUSE
	float4 MRT1			: COLOR1;	// POSITION (view space)
	float4 MRT2			: COLOR2;	// NORMAL	(view space)
};

// for lighting stage
struct PS_MRTOut
{
	float4 Colour0 : COLOR0;
	float4 Colour1 : COLOR1;
};

//--------------------------------------------------------------//
// Vertex Shader
//--------------------------------------------------------------//


POSTEX skybox_last_vs_main( POSTEX Input )
{
	POSTEX Out;
	float4 PosClip = mul( Input.Position, matWorldViewProjection );
	
	// set w = z, so that when the perspective divide happens, our z value is always = ~1.
	// Since the z buffer is always cleared to 1 in our application, this means that we can set the depth test to be
	// less or equal to 1.  Any pixels that haven't been written to when rendering the scene will still have a depth of 1
	// so the skybox is only rendered for these particular pixels.  Since our skybox can't interact with anything else
	// it doesn't matter about the divide.
	
	PosClip.w = PosClip.z;
	Out.Position = PosClip;
	Out.TexCoords = Input.TexCoords;
	return Out;
}

VS_OUTPUT_POSPOSTEXVIEW fill_gbuffer_vs_main( VS_INPUT_POSTEXNORM Input )
{
	VS_OUTPUT_POSPOSTEXVIEW Out;
	Out.PosClip = mul(Input.Position, matWorldViewProjection);
	Out.PosView = mul(Input.Position, matWorldView);
	Out.TexCoords = Input.TexCoords;
	Out.NormalView = mul(Input.Normal.xyz, (float3x3)matWorldView);
	return Out;
}

VS_OUTPUT_NORMALMAPPING fill_gbuffer_normalmapping_vs_main( VS_INPUT_NORMALMAPPING Input )
{
	VS_OUTPUT_NORMALMAPPING Out;
	Out.PosClip = mul(Input.Position, matWorldViewProjection);
	Out.PosView = mul(Input.Position, matWorldView);
	Out.TexCoords = Input.TexCoords;
		
	// we need to calculate a matrix that transforms each texel in the PS from tangent to view space
	// we can do this per vertex and interpolate & normalise the result
	// so first we need to go from tangent to object space, then from object to view space
	
	// the matrix that describes the transform from tangent to object space is
	// (Tx, Ty, Tz)
	// (Bx, By, Bz)
	// (Nx, Ny, Nz)
			
	float3x3 matTBN = float3x3( Input.Tangent, Input.Binormal, Input.Normal );
	
	// object to view space
	float3x3 matTangentToViewSpace = mul( matTBN, (float3x3)matWorldView );
	Out.TangentToView0 = matTangentToViewSpace[0];
	Out.TangentToView1 = matTangentToViewSpace[1];
	Out.TangentToView2 = matTangentToViewSpace[2];
	
	return Out;
}


//--------------------------------------------------------------//
// Pixel Shader
//--------------------------------------------------------------//

/*
PS_MRTOut skybox_fill_gbuffer_ps_main( PS_INPUT_TEX Input )
{	
	PS_MRTOut OUT = (PS_MRTOut)0;
	half4 colour = half4( tex2D( baseMap, Input.TexCoords ).xyz, 1.0f );	
	
	OUT.Colour0.rg = colour.rg;
	OUT.Colour1.rg = colour.ba;
	return OUT;
}*/

float4 skybox_fill_gbuffer_ps_main( in float2 TexCoords : TEXCOORD0 ) : COLOR
{	
	half4 colour = half4( tex2D( skyboxSampler, TexCoords ).xyz, 1.0f );	
	return colour;
}

PS_OUTPUT_MRT fill_gbuffer_ps_main( PS_INPUT_POSTEXVIEW Input )
{
	PS_OUTPUT_MRT Out;
	
	Out.MRT0 = tex2D( baseMap, Input.TexCoords );	
	Out.MRT1 = half4( Input.PosView.xyz, 1.0f );
	
	half3 fvNormalView = normalize( Input.NormalView );
	Out.MRT2 = half4( fvNormalView, 0.0f );	
		
	return Out;
}

PS_OUTPUT_MRT fill_gbuffer_normalmapping_ps_main( PS_INPUT_NORMALMAPPING Input )
{
	PS_OUTPUT_MRT Out;
	Out.MRT0 = tex2D( baseMap, Input.TexCoords );     
	Out.MRT1 = Input.PosView;
	//Out.MRT1 = half4( Input.PosView.xyz, 5.0f ); // put the lighting model identifier in the w component of the position RT
			
	// texture stores normal as range of 0...1 whereas normals go from -1 to 1, so read model's normal map & convert to proper value
	half4 normalMapIn = tex2D( normalMap, Input.TexCoords );
	half3 tangentSpaceNormal = normalMapIn.xyz * 2 - 1;	
		
	float3x3 matTangentToViewSpace = float3x3( Input.TangentToView0, Input.TangentToView1, Input.TangentToView2 );
	
	// half3 = fast normalise.  Could skip this normalise to pick up additional FPS if it doesn't impact on quality too much.  It's costing me 20 fps right now
	half3 viewSpaceNormal = normalize( mul( tangentSpaceNormal, matTangentToViewSpace ));
		
	Out.MRT2 = float4( viewSpaceNormal, g_bEmissive ? normalMapIn.w : 0.0f );	
	
	return Out;
}

//--------------------------------------------------------------//
// Shader definitions
//--------------------------------------------------------------//

POSTEX posclip_texcoords_vs_main( POSTEX Input )
{
	POSTEX Out;
	Out.Position = mul( Input.Position, matWorldViewProjection );
	Out.TexCoords = Input.TexCoords;
	return Out;
}
/*
PS_MRTOut ambient_diffuse( PS_INPUT_TEX Input )
{	
	PS_MRTOut OUT = (PS_MRTOut)0;
	
	half4 diffuse = tex2D( diffuseSampler, Input.TexCoords );
	half4 colour = diffuse * g_fvAmbientColour;	
	
	colour.a = 1.0f;
	
	OUT.Colour0.rg = colour.rg;
	OUT.Colour1.rg = colour.ba;
	return OUT;
}
*/

float4 ambient_diffuse( in float2 TexCoords : TEXCOORD0 ) : COLOR
{	
	half4 diffuse = tex2D( diffuseSampler, TexCoords );
	half fEmissive = tex2D( normalSampler, TexCoords ).w;
	
	half4 colour;	
			
	if( fEmissive > abs( 0.5f ) )
	{
		colour = fEmissive * diffuse;
	}
	else
	{
		colour = diffuse * g_fvAmbientColour;				
	}		
	
	return colour;	
}

//--------------------------------------------------------------//
// Diffuse Lighting Pass (Directional)
//--------------------------------------------------------------//

//--------------------------------------------------------------//
// Shader definitions
//--------------------------------------------------------------//

	// we must negate the view space light dir as it gives the direction in which the light is facing
	// however, we need to do a dot between a vector leaving the surface and the surface's normal to get the intensity
	
	// N = normal of surf, L = light direction
	//
	// N  L				N  -L
	// ^  /				^  ^
	// | /		versus	| / 
	// |V				|/
	
	/*
PS_MRTOut directional_shadowed_phong_ps_main ( PS_INPUT_TEX Input )
{		
	PS_MRTOut OUT = (PS_MRTOut)0;
	
	half3 fvToLight				= -g_fvViewSpaceLightDirection;
		
	half4 fvDiffuse				= tex2D( diffuseSampler, Input.TexCoords );
	half3 fvViewSpacePos		= tex2D( positionSampler, Input.TexCoords ).xyz;
	half3 fvViewSpaceNormal		= tex2D( normalSampler, Input.TexCoords ).xyz;								// N
	
	// get viewspace pos
	half4 fvPos					= float4( fvViewSpacePos.xyz, 1.0f );
	
	// transform from view to world space, then from world to light projection space
	half4 fvLightSpacePos		= mul( fvPos, matViewToLightProjSpace );
	
	// Hardware will figure out if it's in shadow or not automatically (since we're using a depth stencil texture).
	// If we weren't using hardware to do the test for us, we'd need to a manual check.  
	
	// To do the manual check, we transform the pixel's position into light space and retrieve a depth value for it.
	// We then compares the depth of the corresponding value in the shadow map (the depth).  The depth in the shadow map
	// represents the distance from the light, so if the projected co-ord is greater than this depth, it is in shadow
	// this value will be roughly 1.0f if the pixel is visible to the light.  Big suit optional.
	
	// also, we first check whether the point we're considering is BEHIND the light in light space.
	// projective texturing inherently causes back projection, so we must test for it as a separate case.
	// if something is behind the light, it is obviously not lit, so set fRemainInLight to 0.
	half fRemainInLight;
	
	if( g_bCastsShadows )
	{
		fRemainInLight			= fvLightSpacePos.w < 0 ? 0 : tex2Dproj( shadowMapSampler, fvLightSpacePos ).x;
	}
	else
	{
		fRemainInLight			= 1.0f;
	}
		
	half4 fvTempColour			= half4( 0.0f, 0.0f, 0.0f, 1.0f );
		
	half fNDotL					= max( 0, dot( fvViewSpaceNormal, fvToLight ));							// N.L
	
	if( fNDotL > 0.0f )
	{
		half4 fvTotalDiffuse		= fvDiffuse * fNDotL;
		
		half3 fvPixelToViewer		= normalize( -fvViewSpacePos );												// V
		half3 fvReflection			= normalize( 2.0f * fNDotL * fvViewSpaceNormal - fvToLight );				// R
		half fRDotV					= max( 0.0f, dot( fvReflection, fvPixelToViewer ));							// R.V
		
		// alpha = specular map.  Modulate specularity by alpha channel of diffuse texture map.
		half4 fvTotalSpecular		= fvDiffuse.w * pow( fRDotV, g_fSpecularPower );
				
		if( g_bUseHDR )
		{
			if( g_bCastsShadows )
			{
				fvTempColour = fRemainInLight * g_fvLightColour * ( fvTotalDiffuse + fvTotalSpecular );		
			}
			else
			{
				fvTempColour = g_fvLightColour * ( fvTotalDiffuse + fvTotalSpecular );		
			}
			
		}
		else
		{
			if( g_bCastsShadows )
			{
				fvTempColour = saturate( fRemainInLight * g_fvLightColour * ( fvTotalDiffuse + fvTotalSpecular ));		
			}
			else
			{
				fvTempColour = saturate( g_fvLightColour * ( fvTotalDiffuse + fvTotalSpecular ));		
			}			
		}		
	}	
	
	fvTempColour.a = 1.0f;
	
	OUT.Colour0.rg = fvTempColour.rg;
	OUT.Colour1.rg = fvTempColour.ba;

	return OUT;
}
*/

float4 directional_shadowed_phong_ps_main ( in float2 TexCoords : TEXCOORD0 ) : COLOR
{		
	half3 fvToLight				= -g_fvViewSpaceLightDirection;
		
	half4 fvDiffuse				= tex2D( diffuseSampler, TexCoords );
	half4 fvViewSpacePos		= tex2D( positionSampler, TexCoords );
	half3 fvViewSpaceNormal		= tex2D( normalSampler, TexCoords ).xyz;	// N
	
	// get viewspace pos
	half4 fvPos					= float4( fvViewSpacePos.xyz, 1.0f );
	
	// transform from view to world space, then from world to light projection space
	half4 fvLightSpacePos		= mul( fvPos, matViewToLightProjSpace );
	
	// Hardware will figure out if it's in shadow or not automatically (since we're using a depth stencil texture).
	// If we weren't using hardware to do the test for us, we'd need to a manual check.  
	
	// To do the manual check, we transform the pixel's position into light space and retrieve a depth value for it.
	// We then compares the depth of the corresponding value in the shadow map (the depth).  The depth in the shadow map
	// represents the distance from the light, so if the projected co-ord is greater than this depth, it is in shadow
	// this value will be roughly 1.0f if the pixel is visible to the light.  Big suit optional.
	
	// also, we first check whether the point we're considering is BEHIND the light in light space.
	// projective texturing inherently causes back projection, so we must test for it as a separate case.
	// if something is behind the light, it is obviously not lit, so set fRemainInLight to 0.
	half fRemainInLight;
	
	if( g_bCastsShadows )
	{
		fRemainInLight			= fvLightSpacePos.w < 0 ? 0 : tex2Dproj( shadowMapSampler, fvLightSpacePos ).x;
	}
	else
	{
		fRemainInLight			= 1.0f;
	}
		
	half4 fvTempColour			= half4( 0.0f, 0.0f, 0.0f, 1.0f );
		
	half fNDotL					= max( 0, dot( fvViewSpaceNormal, fvToLight ));							// N.L
	
	if( fNDotL > 0.0f )
	{
		half4 fvTotalDiffuse		= fvDiffuse * fNDotL;
		
		half3 fvPixelToViewer		= normalize( -fvViewSpacePos );												// V
		half3 fvReflection			= normalize( 2.0f * fNDotL * fvViewSpaceNormal - fvToLight );				// R
		half fRDotV					= max( 0.0f, dot( fvReflection, fvPixelToViewer ));							// R.V
		
		// alpha = specular map.  Modulate specularity by alpha channel of diffuse texture map.
		half4 fvTotalSpecular		= fvDiffuse.w * pow( fRDotV, g_fSpecularPower );
				
		if( g_bUseHDR )
		{
			if( g_bCastsShadows )
			{
				fvTempColour = fRemainInLight * g_fvLightColour * ( fvTotalDiffuse + fvTotalSpecular );		
			}
			else
			{
				fvTempColour = g_fvLightColour * ( fvTotalDiffuse + fvTotalSpecular );		
			}
			
		}
		else
		{
			if( g_bCastsShadows )
			{
				fvTempColour = saturate( fRemainInLight * g_fvLightColour * ( fvTotalDiffuse + fvTotalSpecular ));		
			}
			else
			{
				fvTempColour = saturate( g_fvLightColour * ( fvTotalDiffuse + fvTotalSpecular ));		
			}			
		}		
	}	
	
	// demonstrates how to change shading model based on contents of a g-buffer channel
	// could use texture lookups or something for this
	/*
	if( abs( fvViewSpacePos.w - 5.0f ) < 0.1f )
	{
		fvTempColour = half4(1.0f, 0.0f, 0.0f, 1.0f );	
	}
	*/

	return fvTempColour;
}

//--------------------------------------------------------------//
// Diffuse Lighting Pass (Point Light [omni])
//--------------------------------------------------------------//

struct PS_INPUT_TEXVPOS
{
	float2 TexCoords : TEXCOORD0;
	float2 vPos : VPOS;
};


//--------------------------------------------------------------//
// Shader definitions
//--------------------------------------------------------------//

// uses existing VS

/*
PS_MRTOut phong_point_ps_main( PS_INPUT_TEXVPOS Input )
{	
	PS_MRTOut OUT = (PS_MRTOut)0;
	
	// Co-ords for texture lookups = pixel rasterisation pos / screen dimensions (e.g. 512/1024 = 0.5f)
	float2 coords = Input.vPos.xy / g_fScreenSize.xy;
	
	// retrieve data from g-buffer
	half3 fvViewSpacePos	= tex2D( positionSampler, coords ).xyz;
	half3 fvViewSpaceNormal	= tex2D( normalSampler, coords ).xyz;
	half4 fvDiffuse			= tex2D( diffuseSampler, coords ); 
	
	// light calcs	
	half3 fvPixelToLight	= g_fvViewSpaceLightPosition - fvViewSpacePos;				
	half3 fvPixelToLightNormalised = normalize( fvPixelToLight );													// L
	
	half4 fvTempColour		= half4( 0.0f, 0.0f, 0.0f, 0.0f );
	half fNDotL				= max( 0.0f, dot( fvViewSpaceNormal, fvPixelToLightNormalised ));						// N.L
	
	if( fNDotL > 0.0f )
	{
		half4 fvTotalDiffuse	= fvDiffuse * fNDotL;
		
		half3 fvPixelToViewer	= normalize( -fvViewSpacePos );															// V
		half3 fvReflection		= normalize( 2.0f * fNDotL * fvViewSpaceNormal - fvPixelToLightNormalised);				// R
		half fRDotV				= max( 0.0f, dot( fvReflection, fvPixelToViewer ));										// R.V

		half4 fvTotalSpecular	= fvDiffuse.w * pow( fRDotV, g_fSpecularPower );
			
		// crude attenuation
		half attenuation = max( 0, 1 - length( fvPixelToLight) / g_fLightMaxRange );
			
		if( g_bUseHDR )
		{
			fvTempColour = g_fvLightColour * attenuation * (fvTotalDiffuse + fvTotalSpecular );	
		}
		else
		{
			fvTempColour = saturate( g_fvLightColour * attenuation * (fvTotalDiffuse + fvTotalSpecular ));	
		}		
	}
		
	fvTempColour.a = 1.0f;
	
	OUT.Colour0.rg = fvTempColour.rg;
	OUT.Colour1.rg = fvTempColour.ba;
	return OUT;
}
*/

float4 phong_point_ps_main( PS_INPUT_TEXVPOS Input ) : COLOR
{		
	// Co-ords for texture lookups = pixel rasterisation pos / screen dimensions (e.g. 512/1024 = 0.5f)
	// But due to D3D's weird sampling rules, we have to correct the texture co-ordinate by offsetting it by a predefined amount
	float2 coords = Input.vPos.xy / g_fScreenSize.xy;		
	coords += g_fUVAdjust;
	
	// retrieve data from g-buffer
	half3 fvViewSpacePos	= tex2D( positionSampler, coords ).xyz;
	half3 fvViewSpaceNormal	= tex2D( normalSampler, coords ).xyz;
	half4 fvDiffuse			= tex2D( diffuseSampler, coords ); 
	
	// light calcs	
	half3 fvPixelToLight	= g_fvViewSpaceLightPosition - fvViewSpacePos;				
	half3 fvPixelToLightNormalised = normalize( fvPixelToLight );													// L
	
	half4 fvTempColour		= half4( 0.0f, 0.0f, 0.0f, 0.0f );
	half fNDotL				= max( 0.0f, dot( fvViewSpaceNormal, fvPixelToLightNormalised ));						// N.L
	
	if( fNDotL > 0.0f )
	{
		half4 fvTotalDiffuse	= fvDiffuse * fNDotL;
		
		half3 fvPixelToViewer	= normalize( -fvViewSpacePos );															// V
		half3 fvReflection		= normalize( 2.0f * fNDotL * fvViewSpaceNormal - fvPixelToLightNormalised );			// R
		half fRDotV				= max( 0.0f, dot( fvReflection, fvPixelToViewer ));										// R.V

		half4 fvTotalSpecular	= fvDiffuse.w * pow( fRDotV, g_fSpecularPower );
			
		// crude attenuation
		half fDistanceToLight = length( fvPixelToLight );
		half fAttenuation = max( 0, 1 - ( fDistanceToLight / g_fLightMaxRange ));	
						
		if( g_bUseHDR )
		{
			fvTempColour = g_fvLightColour * fAttenuation * (fvTotalDiffuse + fvTotalSpecular );	
		}
		else
		{
			fvTempColour = saturate( g_fvLightColour * fAttenuation * (fvTotalDiffuse + fvTotalSpecular ));	
		}		
	}
		
	return fvTempColour;
}


//--------------------------------------------------------------//
// Phong spot lighting
//--------------------------------------------------------------//

	// All of the spotlight explanations & mathematical models I've read don't bother including normal calculations.  
	// This is the best article I've read: http://www.delphi3d.net/articles/viewarticle.php?article=phong.htm
	// I = A + spot*att*(D + g*S)
	// where A = Ambient.  spot = dot( -LightDir, PosToLight ).  att = Attenuation.  D = Diffuse. G = Gloss.  S = Specular
/*
PS_MRTOut phong_spot_shadowed_ps_main( PS_INPUT_TEXVPOS Input )
{
	PS_MRTOut OUT = (PS_MRTOut)0;
	
	float2 coords = Input.vPos.xy / g_fScreenSize.xy;
	
	// get attributes
	half3 fvViewSpacePos	= tex2D( positionSampler, coords ).xyz;
	half3 fvViewSpaceNormal	= tex2D( normalSampler, coords ).xyz;
	half4 fvDiffuse			= tex2D( diffuseSampler, coords ); 
	
	// need these two vectors for lighting calcs
	half3 fvPixelToLightSource				= g_fvViewSpaceLightPosition - fvViewSpacePos;
	half3 fvPixelToLightSourceNormalised	= normalize( fvPixelToLightSource );
			
	half4 fvTempColour = half4( 0.0f, 0.0f, 0.0f, 1.0f );
	half fNDotL			= max( 0, dot( fvViewSpaceNormal, fvPixelToLightSourceNormalised ) );
	
	// get viewspace pos
	half4 fvPos					= half4( fvViewSpacePos.xyz, 1.0f );
	
	// transform from view to world space, then from world to light projection space
	half4 fvLightSpacePos		= mul( fvPos, matViewToLightProjSpace );
	
	half fRemainInLight;
	
	if( g_bCastsShadows )
	{
		fRemainInLight			= fvLightSpacePos.w < 0 ? 0 : tex2Dproj( shadowMapSampler, fvLightSpacePos ).x;
	}
	else
	{
		fRemainInLight			= 1.0f;
	}	
		
	if( fNDotL > 0.0f )
	{
		// the more alligned the vectors, the brighter the light as the pixel considered will be closer to the centre of the spotlight
		// circle when the dot product value tends towards 1.  When it is 0 or below, the pixel receives no light.
		
		// alter the angular falloff.  Larger values give sharper declines (much more intense towards the centre of the spotlight)
		// whereas smaller values make a 'hill' shaped curve where the intensity is stronger for a wider range of angles near the centre
		half fConeStrength = max( 0.0f, dot( fvPixelToLightSourceNormalised, -g_fvViewSpaceLightDirection ));
		
		//if( fConeStrength > g_fInnerAngle )
		//{
		//	fvTempColour = float4( 1.0f, 0.0f, 0.0f, 1.0f );
		//}
		
		if( fConeStrength > g_fOuterAngle )
		{
			half fConeAttenuation		= pow( fConeStrength, g_fSpotlightFalloff );
			half fDistanceAttenuation	= 1.0f;//max( 0, 1 - length( fvPixelToLightSource ) / g_fLightMaxRange );
							
			// specular term
			half3 fvPixelToEye = normalize( -fvViewSpacePos );											// V
			half3 fvReflection = normalize( 2.0f * fNDotL * fvViewSpaceNormal - fvPixelToLightSourceNormalised ); // R
			half fRDotV = max( 0.0f, dot( fvReflection, fvPixelToEye ) );								// R.V
					
			half4 fvTotalDiffuse =  fvDiffuse * fNDotL;
			half4 fvTotalSpecular = fvDiffuse.w * pow( fRDotV, g_fSpecularPower );		
			
			if( g_bUseHDR )
			{
				if( g_bCastsShadows )
				{
					fvTempColour = g_fvLightColour * fRemainInLight * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular );					
				}
				else
				{
					fvTempColour = g_fvLightColour * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular );					
				}				
			}
			else
			{
				if( g_bCastsShadows )
				{
					fvTempColour = saturate( g_fvLightColour * fRemainInLight * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular ) );					
				}
				else
				{
					fvTempColour = saturate( g_fvLightColour * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular ) );					
				}				
			}					
		}				
	}	
		
	fvTempColour.a = 1.0f;
		
	OUT.Colour0.rg = fvTempColour.rg;
	OUT.Colour1.rg = fvTempColour.ba;
	
	return OUT;
}
*/

float4 phong_spot_shadowed_ps_main( PS_INPUT_TEXVPOS Input ) : COLOR
{	
	// Co-ords for texture lookups = pixel rasterisation pos / screen dimensions (e.g. 512/1024 = 0.5f)
	// But due to D3D's weird sampling rules, we have to correct the texture co-ordinate by offsetting it by a predefined amount
	float2 coords = Input.vPos.xy / g_fScreenSize.xy;		
	coords += g_fUVAdjust;
	
	// get attributes
	half3 fvViewSpacePos	= tex2D( positionSampler, coords ).xyz;
	half3 fvViewSpaceNormal	= tex2D( normalSampler, coords ).xyz;
	half4 fvDiffuse			= tex2D( diffuseSampler, coords ); 
	
	// need these two vectors for lighting calcs
	half3 fvPixelToLightSource				= g_fvViewSpaceLightPosition - fvViewSpacePos;
	half3 fvPixelToLightSourceNormalised	= normalize( fvPixelToLightSource );
			
	half4 fvTempColour = half4( 0.0f, 0.0f, 0.0f, 1.0f );
	half fNDotL			= max( 0, dot( fvViewSpaceNormal, fvPixelToLightSourceNormalised ) );
	
	// get viewspace pos
	half4 fvPos					= half4( fvViewSpacePos.xyz, 1.0f );
	
	// transform from view to world space, then from world to light projection space
	half4 fvLightSpacePos		= mul( fvPos, matViewToLightProjSpace );
	
	half fRemainInLight;
	
	if( g_bCastsShadows )
	{
		fRemainInLight			= fvLightSpacePos.w < 0 ? 0 : tex2Dproj( shadowMapSampler, fvLightSpacePos ).x;
	}
	else
	{
		fRemainInLight			= 1.0f;
	}	
		
	if( fNDotL > 0.0f )
	{
		// the more alligned the vectors, the brighter the light as the pixel considered will be closer to the centre of the spotlight
		// circle when the dot product value tends towards 1.  When it is 0 or below, the pixel receives no light.
		
		// alter the angular falloff.  Larger values give sharper declines (much more intense towards the centre of the spotlight)
		// whereas smaller values make a 'hill' shaped curve where the intensity is stronger for a wider range of angles near the centre
		half fConeStrength = max( 0.0f, dot( fvPixelToLightSourceNormalised, -g_fvViewSpaceLightDirection ));
		
		// currently we've got an ugly spotlight because if fConeStrength > g_fOuterAngle then it means it passes the cone strength test
		// but then it gets lit at a constant intensity inside the cone.  Instead, we've got to treat the outer edge of the cone
		// as being the point at which intensity is 0.  If the point is inside the inner cone, then it is lit at full intensity.
		// If it is somewhere between the two, lerp between full & zero intensity using the smoothstep function or a texture lookup.
		
		if( fConeStrength > g_fOuterAngle )
		{						
			half fConeAttenuation		= smoothstep( g_fOuterAngle, g_fInnerAngle, fConeStrength );
			//half fConeAttenuation		= pow( fConeStrength, g_fSpotlightFalloff );
			
			// alternative attenuation equation is something along the lines of
			// att = 1 / (quadratic * distance^2 + linear * distance + constant )

			half fDistanceAttenuation	= max( 0, 1 - length( fvPixelToLightSource ) / g_fLightMaxRange );
							
			// specular term
			half3 fvPixelToEye = normalize( -fvViewSpacePos );											// V
			half3 fvReflection = normalize( 2.0f * fNDotL * fvViewSpaceNormal - fvPixelToLightSourceNormalised ); // R
			half fRDotV = max( 0.0f, dot( fvReflection, fvPixelToEye ) );								// R.V
					
			half4 fvTotalDiffuse =  fvDiffuse * fNDotL;
			half4 fvTotalSpecular = fvDiffuse.w * pow( fRDotV, g_fSpecularPower );		
			
			if( g_bUseHDR )
			{
				if( g_bCastsShadows )
				{
					fvTempColour = g_fvLightColour * fRemainInLight * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular );					
				}
				else
				{
					fvTempColour = g_fvLightColour * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular );					
				}				
			}
			else
			{
				if( g_bCastsShadows )
				{
					fvTempColour = saturate( g_fvLightColour * fRemainInLight * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular ) );					
				}
				else
				{
					fvTempColour = saturate( g_fvLightColour * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular ) );					
				}				
			}					
		}				
	}	
	return fvTempColour;
}

float4 show_stencil_ps_main( PS_INPUT_TEXVPOS Input ) : COLOR
{
	// Co-ords for texture lookups = pixel rasterisation pos / screen dimensions (e.g. 512/1024 = 0.5f)
	// But due to D3D's weird sampling rules, we have to correct the texture co-ordinate by offsetting it by a predefined amount
	float2 coords = Input.vPos.xy / g_fScreenSize.xy;		
	coords += g_fUVAdjust;
	
	half4 pixelDiffuse = tex2D( diffuseSampler, coords ); 
	half4 colour = pixelDiffuse * half4(1.0f, 1.0f, 1.0f, 1.0f);
	return colour;	
}

// simple distance based fog.  Set the blending values so that this output linearly blends using the alpha value
float4 fog_ps_main( POSTEX Input ) : COLOR
{	
	half fDepth = tex2D( positionSampler, Input.TexCoords ).z;
		
	half fFog = g_fFogEnd - fDepth;
	half fFogDif = g_fFogEnd - g_fFogStart;
	half fFogVal = clamp( (fFog / fFogDif), 0, 1 );
	half fOneMinusFogVal = 1 - fFogVal;

	return half4( g_fvFogColour, fOneMinusFogVal );
}

//--------------------------------------------------------------//
// Stencil Convex Light Pass
//--------------------------------------------------------------//

float4 pos_vs_main( in float4 Position : POSITION ) : POSITION
{
	return( mul( Position, matWorldViewProjection ));	
}

//--------------------------------------------------------------//
// DEBUG shader pass -- Output RT0
//--------------------------------------------------------------//
float4 show_RT0_ps_main( in float2 TexCoords : TEXCOORD0 ) : COLOR
{
	// get it into a decent range so that it actually makes sense to visualise
	
	half4 pos = tex2D( positionSampler, TexCoords );
	pos /= 256.0f;
	return pos;
	//return( tex2D( positionSampler, TexCoords ));		
}

//--------------------------------------------------------------//
// DEBUG shader pass -- Output RT1
//--------------------------------------------------------------//
float4 show_RT1_ps_main( in float2 TexCoords : TEXCOORD0 ) : COLOR
{	
	return( tex2D( normalSampler, TexCoords ));	
}

//--------------------------------------------------------------//
// DEBUG shader pass -- Output RT2
//--------------------------------------------------------------//
float4 show_RT2_ps_main( in float2 TexCoords : TEXCOORD0 ) : COLOR
{
	return(  tex2D( diffuseSampler, TexCoords ) );	
}

//--------------------------------------------------------------//
// Shadow mapping
//--------------------------------------------------------------//



//--------------------------------------------------------------//
// Technique Section for Deferred Shading
//--------------------------------------------------------------//

// technique to do skybox last 
technique SkyboxLast
{
	pass Pass0
	{
		VertexShader	= compile vs_2_0 skybox_last_vs_main();
		PixelShader		= compile ps_2_0 skybox_fill_gbuffer_ps_main();
		
		CullMode		= CCW;
						      
		ZEnable			= true;
		ZWriteEnable	= false;
		ZFunc			= LessEqual;
	}	   	   
}


// technique to do skybox last 
technique SkyboxLastStencil
{
	pass P0
	{
		VertexShader	= compile vs_2_0 skybox_last_vs_main();
		PixelShader		= null;
		
		CullMode		= CCW;
						      
		ZEnable			= true;
		ZWriteEnable	= false;
		ZFunc			= LessEqual;
				
        ColorWriteEnable	= 0x0;        
        
        // Disable writing to the frame buffer
        AlphaBlendEnable	= true;
        SrcBlend			= Zero;
        DestBlend			= One;
              
        // Setup stencil states
        StencilEnable		= true;
        TwoSidedStencilMode = false;
        
        StencilRef			= 1;
        StencilMask			= 0xFFFFFFFF;
        StencilWriteMask	= 0xFFFFFFFF;
        
        // stencil settings for front facing triangles
        StencilFunc			= Always;
        StencilZFail		= Keep;
        StencilPass			= Incr;        
	}	   
	
	pass P1
	{
		VertexShader	= compile vs_2_0 skybox_last_vs_main();
		PixelShader		= compile ps_2_0 skybox_fill_gbuffer_ps_main();
		
		CullMode		= CCW;
								      
		ZEnable			= false;
		ZWriteEnable	= false;
		ZFunc			= Always;	
		
		ColorWriteEnable = 0xFFFFFFFF;       
		
		 // Disable writing to the frame buffer
        AlphaBlendEnable	= false;
        SrcBlend			= One;
        DestBlend			= One;
		
		// Setup stencil states
        StencilEnable	= true;
		StencilFunc		= Equal;

		StencilFail		= Keep;
		StencilZFail	= Keep;
		StencilPass		= Keep;

		StencilRef		= 1;
		StencilMask		= 0xFFFFFFFF;
        StencilWriteMask = 0xFFFFFFFF;        
	}	   
}


technique FillGBuffer
{
	pass Pass0
	{
		VertexShader	= compile vs_2_0 fill_gbuffer_vs_main();
		PixelShader		= compile ps_2_0 fill_gbuffer_ps_main();
		
		CullMode		= CCW;
		      
		ZEnable			= true;
		ZWriteEnable	= true;		
	}	   	   
}

technique FillGBufferNormalMapping
{
	pass Pass0
	{
		VertexShader	= compile vs_2_0 fill_gbuffer_normalmapping_vs_main();
		PixelShader		= compile ps_2_0 fill_gbuffer_normalmapping_ps_main();
		
		CullMode		= CCW;
		      
		ZEnable			= true;
		ZWriteEnable	= true;		
	}	   	   
}

technique AmbientLighting
{
	pass Pass0
	{
		VertexShader	= compile vs_2_0 posclip_texcoords_vs_main();
		PixelShader		= compile ps_2_0 ambient_diffuse();		
		
		CullMode		= CCW;
		
		ZEnable			= false;
		ZWriteEnable	= false;		
	}	

}

technique PhongDirectionalShadowed
{
	pass Pass0
	{
		VertexShader	= compile vs_2_0 posclip_texcoords_vs_main();
		PixelShader		= compile ps_2_0 directional_shadowed_phong_ps_main();
		
		CullMode		= CCW;
		
		ZEnable			= false;
		ZWriteEnable	= false;
		
		AlphaBlendEnable = true;
		
		SrcBlend		= One;
        DestBlend		= One;       
	}	
}

technique PhongDirectionalShadowedStencil
{
	pass Pass0
	{
		VertexShader	= compile vs_2_0 posclip_texcoords_vs_main();
		PixelShader		= compile ps_2_0 directional_shadowed_phong_ps_main();
		
		CullMode		= none;
		
		ZEnable			= false;
		ZWriteEnable	= false;
		ZFunc			= Always;
		
		AlphaBlendEnable = true;
				
		SrcBlend		= One;
        DestBlend		= One;       
        
        CullMode		= CCW;
				
		StencilFunc		= Equal;

		StencilFail		= Keep;
		StencilZFail	= Keep;
		StencilPass		= Keep;

		StencilRef		= 0;
		StencilMask		= 0xFFFFFFFF;
        StencilWriteMask = 0xFFFFFFFF;     
	}	
}



	// PhongPointStencil is the pass 1/2 that handles the stencil volumes and avoids doing wasted shading.
	// 
	// Based on a two-sided stencil implementation as found in shadow volume algorithms.
	// Renders the light volume into the stencil buffer in one pass. 
	// * Initial stencil buffer value = 1
	// * Front facing polys drawn first.  If the depth test fails, increment the stencil value
	// * Back facing polys drawn second.  If the depth test fails, decrement the stencil value
	//
	// So we have several cases:	
	
technique PhongPointStencil
{
	pass Pass0_DoubleSidedStencil
	{
		VertexShader		= compile vs_2_0 pos_vs_main();
        PixelShader			= null;
        
        CullMode			= none;
        ColorWriteEnable	= 0x0;        
        
        // Disable writing to the frame buffer
        AlphaBlendEnable	= true;
        SrcBlend			= Zero;
        DestBlend			= One;
        
        // Disable writing to depth buffer
        ZWriteEnable		= false;
        ZEnable				= true;
        ZFunc				= Less;
       
        // Setup stencil states
        StencilEnable		= true;
        TwoSidedStencilMode = true;
        
        StencilRef			= 1;
        StencilMask			= 0xFFFFFFFF;
        StencilWriteMask	= 0xFFFFFFFF;
        
        // stencil settings for front facing triangles
        StencilFunc			= Always;
        StencilZFail		= Incr;
        StencilPass			= Keep;
        
        // stencil settings for back facing triangles
        Ccw_StencilFunc		= Always;
        Ccw_StencilZFail	= Decr;
        Ccw_StencilPass		= Keep;
	}
	
	pass Pass1_PointLightPhong
	{
		VertexShader	= compile vs_3_0 pos_vs_main();
		PixelShader		= compile ps_3_0 phong_point_ps_main();
		
		ZEnable			= false;
		ZWriteEnable	= false;
							
		AlphaBlendEnable = true;		
		SrcBlend		= One;
        DestBlend		= One;
        
        // draw backfaces so that we're always guaranteed to get the right behaviour when inside the light volume
        CullMode		= CW;	        
        
        ColorWriteEnable = 0xFFFFFFFF;

  		StencilEnable	= true;
  		TwoSidedStencilMode = false;
        StencilFunc		= Equal;

		StencilFail		= Keep;
		StencilZFail	= Keep;
		StencilPass		= Keep;

		StencilRef		= 0;
		StencilMask		= 0xFFFFFFFF;
        StencilWriteMask = 0xFFFFFFFF;
	}
	/*
	pass Pass2_ShowStencilResult
	{
		VertexShader	= compile vs_3_0 pos_vs_main();
		PixelShader		= compile ps_3_0 show_stencil_ps_main();
		
		ZEnable			= false;
		ZWriteEnable	= false;
							
		AlphaBlendEnable = true;		
		SrcBlend		= One;
        DestBlend		= One;
        
        // draw backfaces so that we're always guaranteed to get the right behaviour when inside the light volume
        CullMode		= CW;	        
        
        ColorWriteEnable = 0xFFFFFFFF;

  		StencilEnable	= true;
  		TwoSidedStencilMode = false;
        StencilFunc		= Equal;

		StencilFail		= Keep;
		StencilZFail	= Keep;
		StencilPass		= Keep;

		StencilRef		= 0;
		StencilMask		= 0xFFFFFFFF;
        StencilWriteMask = 0xFFFFFFFF;        
	}
	  */
}


technique PhongSpotStencilShadowed
{
	pass Pass0_DoubleSidedStencil
	{
		VertexShader		= compile vs_2_0 pos_vs_main();
        PixelShader			= null;
        
        CullMode			= none;
        ColorWriteEnable	= 0x0;        
        
        // Disable writing to the frame buffer
        AlphaBlendEnable	= true;
        SrcBlend			= Zero;
        DestBlend			= One;
        
        // Disable writing to depth buffer
        ZWriteEnable		= false;
        ZEnable				= true;
        ZFunc				= Less;
       
        // Setup stencil states
        StencilEnable		= true;
        TwoSidedStencilMode = true;
        
        StencilRef			= 1;
        StencilMask			= 0xFFFFFFFF;
        StencilWriteMask	= 0xFFFFFFFF;
        
        // stencil settings for front facing triangles
        StencilFunc			= Always;
        StencilZFail		= Incr;
        StencilPass			= Keep;
        
        // stencil settings for back facing triangles
        Ccw_StencilFunc		= Always;
        Ccw_StencilZFail	= Decr;
        Ccw_StencilPass		= Keep;
	}
	
	pass Pass1_SpotLightPhongShadowed
	{
		VertexShader	= compile vs_3_0 pos_vs_main();
		PixelShader		= compile ps_3_0 phong_spot_shadowed_ps_main();
		
		ZEnable			= false;
		ZWriteEnable	= false;
							
		AlphaBlendEnable = true;		
		SrcBlend		= One;
        DestBlend		= One;
        
        // draw backfaces so that we're always guaranteed to get the right behaviour when inside the light volume
        CullMode		= CW;	        
        
        ColorWriteEnable = 0xFFFFFFFF;

  		StencilEnable	= true;
  		TwoSidedStencilMode = false;
        StencilFunc		= Equal;

		StencilFail		= Keep;
		StencilZFail	= Keep;
		StencilPass		= Keep;

		StencilRef		= 0;
		StencilMask		= 0xFFFFFFFF;
        StencilWriteMask = 0xFFFFFFFF;
	}
	
	/*
	
	pass Pass2_ShowStencilResult
	{
		VertexShader	= compile vs_3_0 pos_vs_main();
		PixelShader		= compile ps_3_0 show_stencil_ps_main();
		
		ZEnable			= false;
		ZWriteEnable	= false;
							
		AlphaBlendEnable = true;		
		SrcBlend		= One;
        DestBlend		= One;
        
        // draw backfaces so that we're always guaranteed to get the right behaviour when inside the light volume
        CullMode		= CW;	        
        
        ColorWriteEnable = 0xFFFFFFFF;

  		StencilEnable	= true;
  		TwoSidedStencilMode = false;
        StencilFunc		= Equal;

		StencilFail		= Keep;
		StencilZFail	= Keep;
		StencilPass		= Keep;

		StencilRef		= 0;
		StencilMask		= 0xFFFFFFFF;
        StencilWriteMask = 0xFFFFFFFF;
        
	}
	*/
	
}
	
Technique PointLightDiffuse
{
	pass Pass0_PointLightDiffuse
	{
		VertexShader	= compile vs_3_0 posclip_texcoords_vs_main();
		PixelShader		= compile ps_3_0 phong_point_ps_main();
		
		ZEnable			= true;
		ZWriteEnable	= false;		
	
		AlphaBlendEnable = true;
		
		SrcBlend		= One;
        DestBlend		= One;
            
        CullMode		= none;		       			
	}
}

Technique Fogging
{
	Pass P0
	{
		VertexShader	= compile vs_2_0 posclip_texcoords_vs_main();
		PixelShader		= compile ps_2_0 fog_ps_main();
		
		ZEnable			= false;
		ZWriteEnable	= false;
		
		AlphaBlendEnable = true;
		
		//SrcBlend		= One;
		//DestBlend		= One;
		SrcBlend		= SRCALPHA; 
		DestBlend		= INVSRCALPHA;
				
		CullMode		= none;
	}
	
}

// DEBUG PASSES
	
Technique Debug_ShowRT
{	
	pass Pass0_ShowRT0
	{
		VertexShader	= compile vs_2_0 posclip_texcoords_vs_main();
		PixelShader		= compile ps_2_0 show_RT0_ps_main();
			
		ZEnable			= false;
		ZWriteEnable	= false;				
	}
	
	pass Pass1_ShowRT1
	{
		VertexShader	= compile vs_2_0 posclip_texcoords_vs_main();
		PixelShader		= compile ps_2_0 show_RT1_ps_main();
			
		ZEnable			= false;
		ZWriteEnable	= false;				
	}
	
	pass Pass2_ShowRT2
	{
		VertexShader	= compile vs_2_0 posclip_texcoords_vs_main();
		PixelShader		= compile ps_2_0 show_RT2_ps_main();
			
		ZEnable			= false;
		ZWriteEnable	= false;				
	}
}

	