#line 2 "Textured_Phong.fx"
//--------------------------------------------------------------//
// Textured Phong
//--------------------------------------------------------------//

const bool	g_bCastsShadows;
const bool	g_bUseHDR;
const bool	g_bEmissive;

const float4 g_fvAmbientColour;
const float4 g_fvLightColour;
const float  g_fLightMaxRange;

const float3 g_fvViewSpaceLightPosition;
const float3 g_fvViewSpaceLightDirection;

// for normal mapping calculations in tangent space, we need these in object space ready to go from object -> tangent space
const float3 g_fvObjectSpaceEyePosition;
const float3 g_fvObjectSpaceLightPosition;
const float3 g_fvObjectSpaceLightDirection;

// spotlight

const float g_fInnerAngle;
const float g_fOuterAngle;
const float g_fSpotlightFalloff;

float g_fSpecularPower	= float( 25.00 );

const float4x4 matWorld					: World;
//float4x4 matWorldInverse		: WorldInverse;
const float4x4 matView					: View;
const float4x4 matWorldView				: WorldView;
const float4x4 matWorldViewProjection	: WorldViewProjection;

// Specialised matrices
const float4x4 matObjectToLightProjSpace;


//--------------------------------------------------------------//
// Samplers & Textures
//--------------------------------------------------------------//

// Shadow tex
texture shadowMapRT_Tex;		// hardware shadow map texture (depth stencil)
texture shadowColourMapRT_Tex;	// colour shadow map texture (debugging)

texture base_Tex;
texture normal_Tex;


sampler2D skyboxSampler = sampler_state
{
	Texture		= (base_Tex);
	AddressU	= CLAMP;
	AddressV	= CLAMP;
	MINFILTER	= LINEAR;
	MAGFILTER	= LINEAR;
	MIPFILTER	= LINEAR;	
};

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
// Ambient & Depth Pass
//--------------------------------------------------------------//
struct POSTEX
{
   float4 Position	: POSITION0;   
   float2 TexCoords : TEXCOORD0;   
};

POSTEX skybox_last_vs_main( POSTEX Input )
{
	POSTEX Out;
	float4 PosClip = mul( Input.Position, matWorldViewProjection );
	
	// set w = z, so that when the perspective divide happens, our z value is always ~1.
	// Since the z buffer is always cleared to 1 in our application, this means that we can set the depth test to be
	// less or equal
	PosClip.w = PosClip.z;
	Out.Position = PosClip;
	Out.TexCoords = Input.TexCoords;
	return Out;
}

POSTEX postex_vs_main( POSTEX Input )
{
	POSTEX Out;
	Out.Position = mul( Input.Position, matWorldViewProjection );
	Out.TexCoords = Input.TexCoords;
	
	return Out;
}

float4 ambient_ps_main( in float2 TexCoords : TEXCOORD0 ) : COLOR
{
	float4 diffuse = tex2D( baseMap, TexCoords );
	float4 colour;
	
	if( g_bEmissive )
	{
		half fEmissive = tex2D( normalMap, TexCoords ).w;
		
		if( fEmissive > abs( 0.9f ) )
		{
			colour = fEmissive * diffuse;
		}
		else
		{
			colour = diffuse * g_fvAmbientColour;				
		}
	}	
	else
	{
		colour = diffuse * g_fvAmbientColour;	
	}		
	
	return colour;
}

float4 fullbright_ps_main( in float2 TexCoords : TEXCOORD0 ) : COLOR
{
	return tex2D( skyboxSampler, TexCoords );
}


//--------------------------------------------------------------//
// Structs
//--------------------------------------------------------------//

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

struct VS_OUTPUT_POSTEXVIEWNORM
{
	float4 Position					: POSITION0;
	float2 TexCoords				: TEXCOORD0;
	float3 ViewSpaceToEye			: TEXCOORD1;
	float3 ViewSpaceNormal			: TEXCOORD2;
};

struct VS_OUTPUT_NORMALMAPPING
{
	float4 PosClip				: POSITION0;   
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;	
};

struct VS_OUTPUT_NORMALMAPPING_SHADOW
{
	float4 PosClip				: POSITION0;   
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;	
	float4 PosLightSpace		: TEXCOORD3;
};

struct VS_OUTPUT_NORMALMAPPING_ATTENUATION
{
	float4 PosClip				: POSITION0;   
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;	
	float  fAttenuation			: TEXCOORD3;
};

struct VS_OUTPUT_NORMALMAPPING_SPOTLIGHT
{
	float4 PosClip				: POSITION0;   
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;		
	float  fAttenuation			: TEXCOORD3;	// distance based attenuation
	float  fConeStrength		: TEXCOORD4;	// dot product between the negated spotlight direction and the vector from vert to light
};


struct VS_OUTPUT_NORMALMAPPING_SPOTLIGHT_SHADOW
{
	float4 PosClip				: POSITION0;   
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;	
	float4 PosLightSpace		: TEXCOORD3;
	float  fAttenuation			: TEXCOORD4;	// distance based attenuation
	float  fConeStrength		: TEXCOORD5;	// dot product between the negated spotlight direction and the vector from vert to light
};

struct PS_INPUT_TEXVIEWNORM
{
	float2 TexCoords		: TEXCOORD0;
	float3 ViewSpaceToEye	: TEXCOORD1;
	float3 ViewSpaceNormal	: TEXCOORD2;
};

struct PS_INPUT_NORMALMAPPING
{
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;
};


struct PS_INPUT_NORMALMAPPING_SHADOW
{
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;
	float4 PosLightSpace		: TEXCOORD3;
};

struct PS_INPUT_NORMALMAPPING_ATTENUATION
{
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;
	float  fAttenuation			: TEXCOORD3;
};

struct PS_INPUT_NORMALMAPPING_SPOTLIGHT
{
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;
	float  fAttenuation			: TEXCOORD3;	// distance based attenuation
	float  fConeStrength		: TEXCOORD4;	// dot product between the negated spotlight direction and the vector from vert to light
};


struct PS_INPUT_NORMALMAPPING_SPOTLIGHT_SHADOW
{
	float2 TexCoords			: TEXCOORD0;
	float3 TangentSpaceToEye	: TEXCOORD1;
	float3 TangentSpaceToLight	: TEXCOORD2;	
	float4 PosLightSpace		: TEXCOORD3;
	float  fAttenuation			: TEXCOORD4;	// distance based attenuation
	float  fConeStrength		: TEXCOORD5;	// dot product between the negated spotlight direction and the vector from vert to light
};

//--------------------------------------------------------------//
// Directional Lighting Pass
//--------------------------------------------------------------//

/*
VS_OUTPUT_POSTEXVIEWNORM phong_directional_vs_main( VS_INPUT_POSTEXNORM Input )
{
	VS_OUTPUT_POSTEXVIEWNORM Out;
	Out.Position = mul( Input.Position, matWorldViewProjection );
	Out.TexCoords = Input.TexCoords;
	
	float3 fvViewSpaceVertexPosition = mul( Input.Position, matWorldView );
	Out.ViewSpaceToEye = -fvViewSpaceVertexPosition;
	Out.ViewSpaceNormal = mul( Input.Normal, matView );
	return Out;	
}

float4 phong_directional_ps_main( PS_INPUT_TEXVIEWNORM Input ) : COLOR
{
	half3 fvViewDirection  = normalize( Input.ViewSpaceToEye );
	half3 fvNormal         = normalize( Input.ViewSpaceNormal );
	half  fNDotL           = dot( fvNormal, -g_fvViewSpaceLightDirection ); 
   
	half3 fvReflection     = normalize( ( ( 2.0f * fvNormal ) * ( fNDotL ) ) - g_fvViewSpaceLightDirection ); 
	
	half  fRDotV           = max( 0.0f, dot( fvReflection, fvViewDirection ) );
   
	half4 fvBaseColour      = tex2D( baseMap, Input.TexCoords );
	      
	half4 fvTotalDiffuse   = fNDotL * fvBaseColour; 
	//half4 fvTotalSpecular  = pow( fRDotV, g_fSpecularPower );
	half4 fvTotalSpecular = half4( 0,0,0,0 );
	   
	//return( saturate( fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular ));
	return( saturate( g_fvLightColour * (fvTotalDiffuse + fvTotalSpecular )) );	
}
*/

//--------------------------------------------------------------//
// Directional Lighting Pass with normal mapping
//--------------------------------------------------------------//


VS_OUTPUT_NORMALMAPPING phong_directional_normal_mapping_vs_main( VS_INPUT_NORMALMAPPING Input )
{
	VS_OUTPUT_NORMALMAPPING Out;
	Out.PosClip = mul( Input.Position, matWorldViewProjection );
	Out.TexCoords = Input.TexCoords;
	
	float3x3 TBN = float3x3( Input.Tangent, Input.Binormal, Input.Normal );	
	
	half3 fvObjectSpaceVertexToEye		= g_fvObjectSpaceEyePosition - Input.Position.xyz;	// V
	half3 fvObjectSpaceVertexToLight	= -g_fvObjectSpaceLightDirection;									// L
	
	// doing vert * TBN would move a vert from tangent to object space
	// so we want to do the opposite (i.e. move from object to tangent space) 
	// and do vert * transpose(TBN) which is the equivalent of TBN * vert
	
	Out.TangentSpaceToEye = mul( TBN, fvObjectSpaceVertexToEye );
	Out.TangentSpaceToLight = mul( TBN, fvObjectSpaceVertexToLight );
	
	return Out;	
}

float4 phong_directional_normal_mapping_ps_main( PS_INPUT_NORMALMAPPING Input ) : COLOR
{
	// decompress normal from texture.  Go from range [0:1] to [-1:1]
	half3 fvTangentSpaceNormal = tex2D( normalMap, Input.TexCoords ).xyz * 2 - 1;
	half4 fvBaseColour			= tex2D( baseMap, Input.TexCoords );
	
	half3 fvTangentSpacePixelToLight	= normalize( Input.TangentSpaceToLight );	// L
	half fNDotL				= max( 0, dot( fvTangentSpaceNormal, fvTangentSpacePixelToLight ));	// N.L
	
	half4 fvTempColour = half4( 0.0f, 0.0f, 0.0f, 0.0f );
	
	if( fNDotL > 0.0f )
	{
		half3 fvTangentSpacePixelToEye		= normalize( Input.TangentSpaceToEye );		// V
		half4 fvTotalDiffuse		= fNDotL * fvBaseColour; 
		
		half3 fvReflection			= normalize( 2.0f * fNDotL * fvTangentSpaceNormal - fvTangentSpacePixelToLight );
		half fRDotV				= max( 0.0f, dot( fvReflection, fvTangentSpacePixelToEye  ));									// R.V
		half4 fvTotalSpecular		= fvBaseColour.w * pow( fRDotV, g_fSpecularPower );
		
		if( g_bUseHDR )
		{
			fvTempColour = g_fvLightColour * (fvTotalDiffuse + fvTotalSpecular );		
		}
		else
		{
			fvTempColour = saturate( g_fvLightColour * (fvTotalDiffuse + fvTotalSpecular ));		
		}		
	}	
	return fvTempColour;
}


VS_OUTPUT_NORMALMAPPING_SHADOW phong_directional_normal_mapping_shadowed_vs_main( VS_INPUT_NORMALMAPPING Input )
{
	VS_OUTPUT_NORMALMAPPING_SHADOW Out;
	Out.PosClip			= mul( Input.Position, matWorldViewProjection );	
	Out.TexCoords		= Input.TexCoords;
	
	// unlike deferred shading, we can take advantage of doing this per-vertex and interpolating linearly
	Out.PosLightSpace	= mul( Input.Position, matObjectToLightProjSpace );
	
	float3x3 TBN = float3x3( Input.Tangent, Input.Binormal, Input.Normal );	
	
	half3 fvObjectSpaceVertexToEye		= g_fvObjectSpaceEyePosition - Input.Position.xyz;	// V
	half3 fvObjectSpaceVertexToLight	= -g_fvObjectSpaceLightDirection;									// L
	
	// doing vert * TBN would move a vert from tangent to object space
	// so we want to do the opposite (i.e. move from object to tangent space) 
	// and do vert * transpose(TBN) which is the equivalent of TBN * vert
	
	Out.TangentSpaceToEye = mul( TBN, fvObjectSpaceVertexToEye );
	Out.TangentSpaceToLight = mul( TBN, fvObjectSpaceVertexToLight );
	
	return Out;	
}

float4 phong_directional_normal_mapping_shadowed_ps_main( PS_INPUT_NORMALMAPPING_SHADOW Input ) : COLOR
{
	// decompress normal from texture.  Go from range [0:1] to [-1:1]
	half3 fvTangentSpaceNormal = tex2D( normalMap, Input.TexCoords ).xyz * 2 - 1;
	half4 fvBaseColour			= tex2D( baseMap, Input.TexCoords );
	
	// get shadowing term.  
	half fRemainInLight		= Input.PosLightSpace.w < 0 ? 0 : tex2Dproj( shadowMapSampler, Input.PosLightSpace ).x;
	
	half3 fvTangentSpacePixelToLight	= normalize( Input.TangentSpaceToLight );	// L
	half fNDotL							= max( 0, dot( fvTangentSpaceNormal, fvTangentSpacePixelToLight ));	// N.L
	
	half4 fvTempColour = half4( 0.0f, 0.0f, 0.0f, 0.0f );
	
	if( fNDotL > 0.0f )
	{
		half3 fvTangentSpacePixelToEye		= normalize( Input.TangentSpaceToEye );		// V
		half4 fvTotalDiffuse		= fNDotL * fvBaseColour; 
		
		half3 fvReflection			= normalize( 2.0f * fNDotL * fvTangentSpaceNormal - fvTangentSpacePixelToLight );
		half fRDotV					= max( 0.0f, dot( fvReflection, fvTangentSpacePixelToEye  ));									// R.V
		half4 fvTotalSpecular		= fvBaseColour.w * pow( fRDotV, g_fSpecularPower );
		
		if( g_bUseHDR )
		{
			fvTempColour = g_fvLightColour * fRemainInLight * (fvTotalDiffuse + fvTotalSpecular );		
		}
		else
		{
			fvTempColour = saturate( g_fvLightColour * fRemainInLight * (fvTotalDiffuse + fvTotalSpecular ));		
		}		
	}	
	return fvTempColour;
}

//--------------------------------------------------------------//
// Point Lighting Pass
//--------------------------------------------------------------//

struct VS_OUTPUT_POSTEXVIEWLIGHTNORM
{
   float4 Position					: POSITION0;
   float3 ViewSpacePosition			: TEXCOORD0;
   float2 TexCoords					: TEXCOORD1;
   float3 ViewSpaceToEye			: TEXCOORD2;
   float3 ViewSpaceToLight			: TEXCOORD3;
   float3 ViewSpaceNormal			: TEXCOORD4;   
};

struct PS_INPUT_POINTLIGHT
{
   float3 ViewSpacePosition			: TEXCOORD0;
   float2 TexCoords					: TEXCOORD1;
   float3 ViewSpaceToEye			: TEXCOORD2;
   float3 ViewSpaceToLight			: TEXCOORD3;
   float3 ViewSpaceNormal			: TEXCOORD4;  
};

/*
VS_OUTPUT_POSTEXVIEWLIGHTNORM phong_pointlight_vs_main( VS_INPUT_POSTEXNORM Input )
{
   VS_OUTPUT_POSTEXVIEWLIGHTNORM Output;

   Output.Position			= mul( Input.Position, matWorldViewProjection );
   Output.ViewSpacePosition	= mul( Input.Position, matWorldView );
   Output.TexCoords			= Input.TexCoords;
   
   half3 fvViewSpaceVertexPosition = mul( Input.Position, matWorldView );
   
   // doing this in view space so we just negatve the vertex position since the camera is at (0,0,0)
   Output.ViewSpaceToEye    = -fvViewSpaceVertexPosition;
   Output.ViewSpaceToLight   = g_fvViewSpaceLightPosition - fvViewSpaceVertexPosition;
   Output.ViewSpaceNormal           = mul( Input.Normal, (float3x3)matWorldView );
      
   return( Output );   
}


float4 phong_pointlight_ps_main( PS_INPUT_POINTLIGHT Input ) : COLOR0
{      
   float3 fvLightDirection = normalize( Input.ViewSpaceToLight );
   float3 fvNormal         = normalize( Input.ViewSpaceNormal );
   float  fNDotL           = dot( fvNormal, fvLightDirection ); 
   
   float3 fvReflection     = normalize( ( ( 2.0f * fvNormal ) * ( fNDotL ) ) - fvLightDirection ); 
   float3 fvViewDirection  = normalize( Input.ViewSpaceToEye );
   float  fRDotV           = max( 0.0f, dot( fvReflection, fvViewDirection ) );
   
   float4 fvBaseColour      = tex2D( baseMap, Input.TexCoords );
      
   float4 fvTotalDiffuse   = fNDotL * fvBaseColour; 
   
   //float4 fvTotalSpecular  = pow( fRDotV, g_fSpecularPower );
   float4 fvTotalSpecular = float4( 0,0,0,0 );
   
   // linear falloff.  Translate distance from light and maxdist into the range 0...1.
   // e.g. if a light has a range of 100 and the object is 20 units away from it:
   // 100 - 20 / 100 = 80 / 100 = 0.8 of max brightness. 
   
   half attenuation = ( g_fLightMaxRange - distance( g_fvViewSpaceLightPosition, Input.ViewSpacePosition )) / g_fLightMaxRange;
   
   // if attenuation factor is negative, clamp to 0.
   attenuation = saturate( attenuation );
   
   //return( saturate( fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular ));
   return( saturate( g_fvLightColour * attenuation * (fvTotalDiffuse + fvTotalSpecular )) );
      
}

*/

//--------------------------------------------------------------//
// Point Lighting Pass with normal mapping
//--------------------------------------------------------------//

VS_OUTPUT_NORMALMAPPING phong_pointlight_normal_mapping_vs_main( VS_INPUT_NORMALMAPPING Input )
{		
	VS_OUTPUT_NORMALMAPPING Out;
	Out.PosClip = mul( Input.Position, matWorldViewProjection );
	Out.TexCoords = Input.TexCoords;
	
	float3x3 TBN = float3x3( Input.Tangent, Input.Binormal, Input.Normal );	
	
	half3 fvObjectSpaceVertexToEye		= g_fvObjectSpaceEyePosition - Input.Position.xyz;		// V
	half3 fvObjectSpaceVertexToLight	= g_fvObjectSpaceLightPosition - Input.Position.xyz;	// L
	
	// doing vert * TBN would move a vert from tangent to object space
	// so we want to do the opposite (i.e. move from object to tangent space) 
	// and do vert * transpose(TBN) which is the equivalent of TBN * vert
	
	Out.TangentSpaceToEye	= mul( TBN, fvObjectSpaceVertexToEye );
	Out.TangentSpaceToLight	= mul( TBN, fvObjectSpaceVertexToLight );	// normalize in pixel shader	
	
	return Out;	
}


float4 phong_pointlight_normal_mapping_ps_main( PS_INPUT_NORMALMAPPING_ATTENUATION Input ) : COLOR0
{
	// decompress normal from texture.  Go from range [0:1] to [-1:1]
	half3 fvTangentSpaceNormal = tex2D( normalMap, Input.TexCoords ).xyz * 2 - 1;
	half4 fvBaseColour			= tex2D( baseMap, Input.TexCoords );
	
	half fAttenuation = max( 0, 1 - length( Input.TangentSpaceToLight ) / g_fLightMaxRange );
	half3 fvTangentSpacePixelToLight	= normalize( Input.TangentSpaceToLight );		// L
		
	half fNDotL = max( 0, dot( fvTangentSpaceNormal, fvTangentSpacePixelToLight ));	// N.L
	
	half4 fvTempColour = half4( 0.0f, 0.0f, 0.0f, 0.0f );
	if( fNDotL > 0.0f )
	{
		half3 fvTangentSpacePixelToEye		= normalize( Input.TangentSpaceToEye );			// V
		
		half4 fvTotalDiffuse		= fNDotL * fvBaseColour; 
		half3 fvReflection			= normalize( 2.0f * fNDotL * fvTangentSpaceNormal - fvTangentSpacePixelToLight) ;
		half fRDotV				= max( 0.0f, dot( fvReflection, fvTangentSpacePixelToEye  ));									// R.V
			
		half4 fvTotalSpecular		= fvBaseColour.w * pow( fRDotV, g_fSpecularPower );
		//half4 fvTotalSpecular		= half4( 0.0f, 0.0f, 0.0f, 0.0f );
			
		half attenuation = Input.fAttenuation;
		
		if( g_bUseHDR )
		{
			fvTempColour = g_fvLightColour * fAttenuation * ( fvTotalDiffuse + fvTotalSpecular );
		}
		else
		{
			fvTempColour = saturate( g_fvLightColour * fAttenuation * ( fvTotalDiffuse + fvTotalSpecular ));
		}		
		
	}
	return fvTempColour;	
}

//--------------------------------------------------------------//
// Spot Lighting Pass with normal mapping
//--------------------------------------------------------------//

VS_OUTPUT_NORMALMAPPING_SPOTLIGHT phong_spotlight_normal_mapping_vs_main( VS_INPUT_NORMALMAPPING Input )
{
	VS_OUTPUT_NORMALMAPPING_SPOTLIGHT Out;
	Out.PosClip = mul( Input.Position, matWorldViewProjection );
	Out.TexCoords = Input.TexCoords;
	
	float3x3 TBN = float3x3( Input.Tangent, Input.Binormal, Input.Normal );	
	
	half3 fvObjectSpaceVertexToEye		= g_fvObjectSpaceEyePosition - Input.Position.xyz;		// V
	
	// get vector describing the translation between Vert & Light as we need this to calculate the attenuation
	half3 fvObjectSpaceVertexToLight	= g_fvObjectSpaceLightPosition - Input.Position.xyz;				
	
	// calc distance-based attenuation
	Out.fAttenuation = max( 0, 1 - length( fvObjectSpaceVertexToLight ) / g_fLightMaxRange );
	
	// normalise previous result to save having to calculate the first part again
	half3 fvObjectSpaceVertexToLightNormalised = normalize( fvObjectSpaceVertexToLight );									// L
		
	// calc the attenuation cone factor.  This is the negated spotlight direction dotted with the vector pointing from the vertex to the light source
	Out.fConeStrength	= max( 0, dot( fvObjectSpaceVertexToLightNormalised, -g_fvObjectSpaceLightDirection ));
	
	// doing vert * TBN would move a vert from tangent to object space
	// so we want to do the opposite (i.e. move from object to tangent space) 
	// and do vert * transpose(TBN) which is the equivalent of TBN * vert
	
	Out.TangentSpaceToEye	= mul( TBN, fvObjectSpaceVertexToEye );
	Out.TangentSpaceToLight	= mul( TBN, fvObjectSpaceVertexToLightNormalised );	
	
	return Out;	
}		

float4 phong_spotlight_normal_mapping_ps_main( PS_INPUT_NORMALMAPPING_SPOTLIGHT Input ) : COLOR0
{	
	// decompress normal from texture.  Go from range [0:1] to [-1:1]
	half3 fvTangentSpaceNormal = tex2D( normalMap, Input.TexCoords ).xyz * 2 - 1;
	half4 fvDiffuse			= tex2D( baseMap, Input.TexCoords );
	
	half3 fvTangentSpacePixelToLight	= normalize( Input.TangentSpaceToLight );		// L
	half fDistanceAttenuation			= Input.fAttenuation;
	half fConeStrength					= Input.fConeStrength;
		
	half4 fvTempColour = half4( 0.0f, 0.0f, 0.0f, 0.0f );
	half fNDotL = max( 0, dot( fvTangentSpaceNormal, fvTangentSpacePixelToLight ));
		
	if( fNDotL > 0.0f )
	{		
		// alter the angular falloff.  Larger values give sharper declines (much more intense towards the centre of the spotlight)
		// whereas smaller values make a 'hill' shaped curve where the intensity is stronger for a wider range of angles near the centre
				
		if( fConeStrength > g_fOuterAngle )
		{
			half fConeAttenuation		= smoothstep( g_fOuterAngle, g_fInnerAngle, fConeStrength );
			//half fConeAttenuation = pow( Input.fConeStrength, g_fSpotlightFalloff );			
					
			// specular term
			half3 fvPixelToEye = normalize( Input.TangentSpaceToEye );												// V
			half3 fvReflection = normalize( 2.0f * fNDotL * fvTangentSpaceNormal - fvTangentSpacePixelToLight );	// R
			half fRDotV = max( 0.0f, dot( fvReflection, fvPixelToEye ) );											// R.V
					
			half4 fvTotalDiffuse =  fvDiffuse * fNDotL;
			half4 fvTotalSpecular = fvDiffuse.w * pow( fRDotV, g_fSpecularPower );		
			
			if( g_bUseHDR )
			{
				fvTempColour = g_fvLightColour * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular );					
			}
			else
			{
				fvTempColour = saturate( g_fvLightColour * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular ) );					
			}		
			
		}				
	}
		
	return fvTempColour;		
}

//--------------------------------------------------------------//
// Spot Lighting Pass with normal mapping & shadow mapping
//--------------------------------------------------------------//

VS_OUTPUT_NORMALMAPPING_SPOTLIGHT_SHADOW phong_spotlight_normal_mapping_shadowed_vs_main( VS_INPUT_NORMALMAPPING Input )
{
	VS_OUTPUT_NORMALMAPPING_SPOTLIGHT_SHADOW Out;
	Out.PosClip = mul( Input.Position, matWorldViewProjection );
	Out.TexCoords = Input.TexCoords;
	
	// unlike deferred shading, we can take advantage of doing this per-vertex and interpolating linearly
	Out.PosLightSpace	= mul( Input.Position, matObjectToLightProjSpace );
	
	float3x3 TBN = float3x3( Input.Tangent, Input.Binormal, Input.Normal );	
	
	half3 fvObjectSpaceVertexToEye		= g_fvObjectSpaceEyePosition - Input.Position.xyz;		// V
	
	// get vector describing the translation between Vert & Light as we need this to calculate the attenuation
	half3 fvObjectSpaceVertexToLight	= g_fvObjectSpaceLightPosition - Input.Position.xyz;				
	
	// calc distance-based attenuation
	Out.fAttenuation = max( 0, 1 - length( fvObjectSpaceVertexToLight ) / g_fLightMaxRange );
	
	// normalise previous result to save having to calculate the first part again
	half3 fvObjectSpaceVertexToLightNormalised = normalize( fvObjectSpaceVertexToLight );									// L
		
	// calc the attenuation cone factor.  This is the negated spotlight direction dotted with the vector pointing from the vertex to the light source
	Out.fConeStrength	= max( 0, dot( fvObjectSpaceVertexToLightNormalised, -g_fvObjectSpaceLightDirection ));
	
	// doing vert * TBN would move a vert from tangent to object space
	// so we want to do the opposite (i.e. move from object to tangent space) 
	// and do vert * transpose(TBN) which is the equivalent of TBN * vert
	
	Out.TangentSpaceToEye	= mul( TBN, fvObjectSpaceVertexToEye );
	Out.TangentSpaceToLight	= mul( TBN, fvObjectSpaceVertexToLight );	
	
	return Out;	
}		

float4 phong_spotlight_normal_mapping_shadowed_ps_main( PS_INPUT_NORMALMAPPING_SPOTLIGHT_SHADOW Input ) : COLOR0
{	
	// decompress normal from texture.  Go from range [0:1] to [-1:1]
	half3 fvTangentSpaceNormal = tex2D( normalMap, Input.TexCoords ).xyz * 2 - 1;
	half4 fvDiffuse			= tex2D( baseMap, Input.TexCoords );
	
	half fDistanceAttenuation			= max( 0, 1 - length( Input.TangentSpaceToLight ) / g_fLightMaxRange );
	half3 fvTangentSpacePixelToLight	= normalize( Input.TangentSpaceToLight );		// L
	half fConeStrength					= Input.fConeStrength;
	
	// get shadowing term.  
	half fRemainInLight		= Input.PosLightSpace.w < 0 ? 0 : tex2Dproj( shadowMapSampler, Input.PosLightSpace ).x;
		
	half4 fvTempColour = half4( 0.0f, 0.0f, 0.0f, 0.0f );
	half fNDotL = max( 0, dot( fvTangentSpaceNormal, fvTangentSpacePixelToLight ));
		
	if( fNDotL > 0.0f )
	{		
		// alter the angular falloff.  Larger values give sharper declines (much more intense towards the centre of the spotlight)
		// whereas smaller values make a 'hill' shaped curve where the intensity is stronger for a wider range of angles near the centre
				
		if( fConeStrength > g_fOuterAngle )
		{
			half fConeAttenuation		= smoothstep( g_fOuterAngle, g_fInnerAngle, fConeStrength );
			//half fConeAttenuation = pow( Input.fConeStrength, g_fSpotlightFalloff );			
					
			// specular term
			half3 fvPixelToEye = normalize( Input.TangentSpaceToEye );												// V
			half3 fvReflection = normalize( 2.0f * fNDotL * fvTangentSpaceNormal - fvTangentSpacePixelToLight );	// R
			half fRDotV = max( 0.0f, dot( fvReflection, fvPixelToEye ) );											// R.V
					
			half4 fvTotalDiffuse =  fvDiffuse * fNDotL;
			half4 fvTotalSpecular = fvDiffuse.w * pow( fRDotV, g_fSpecularPower );		
			
			if( g_bUseHDR )
			{
				fvTempColour = g_fvLightColour * fRemainInLight * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular );					
			}
			else
			{
				fvTempColour = saturate( g_fvLightColour * fRemainInLight * fConeAttenuation * fDistanceAttenuation * ( fvTotalDiffuse + fvTotalSpecular ) );					
			}		
			
		}				
	}
		
	return fvTempColour;		
}


//--------------------------------------------------------------//
// Technique Section for Phong
//--------------------------------------------------------------//

technique SkyboxLast
{
	pass P0
	{
		VertexShader		= compile vs_2_0 skybox_last_vs_main();
		PixelShader			= compile ps_2_0 fullbright_ps_main();
		
		CullMode			= CCW;
		      
		ZEnable				= true;
		ZWriteEnable		= false;
		ZFunc				= lessEqual;		
	}	   	
}

technique DepthAmbient
{
	pass P0
	{
		VertexShader		= compile vs_2_0 postex_vs_main();
		PixelShader			= compile ps_2_0 ambient_ps_main();
		
		CullMode			= CCW;
		      
		ZEnable				= true;
		ZWriteEnable		= true;
		AlphaBlendEnable	= false;
	}	   	
}

technique PhongDirectionalNormalMapping
{	
	pass P0
	{
		VertexShader		= compile vs_2_0 phong_directional_normal_mapping_vs_main();
		PixelShader			= compile ps_2_0 phong_directional_normal_mapping_ps_main();
	
		CullMode			= CCW;
		
		ZEnable				= true;
		ZWriteEnable		= false;
		ZFunc				= equal;	
		
		AlphaBlendEnable	= true;
		
		SrcBlend = One;
        DestBlend = One;	
	}
}


technique PhongDirectionalNormalMappingShadowed
{	
	pass P0
	{
		VertexShader		= compile vs_2_0 phong_directional_normal_mapping_shadowed_vs_main();
		PixelShader			= compile ps_2_0 phong_directional_normal_mapping_shadowed_ps_main();
	
		CullMode			= CCW;
		
		ZEnable				= true;
		ZWriteEnable		= false;
		ZFunc				= equal;	
		
		AlphaBlendEnable	= true;
		
		SrcBlend = One;
        DestBlend = One;	
	}
}

technique PhongPointLightNormalMapping
{
	pass P0
	{
		VertexShader		= compile vs_2_0 phong_pointlight_normal_mapping_vs_main();
		PixelShader			= compile ps_2_0 phong_pointlight_normal_mapping_ps_main();
		
		CullMode			= CCW;
		
		ZEnable				= true;
		ZWriteEnable		= false;
		ZFunc				= equal;
		
		AlphaBlendEnable	= true;
		
		SrcBlend			= One;
        DestBlend			= One;
	}
}

technique PhongSpotLightNormalMapping
{
	pass P0
	{
		VertexShader		= compile vs_2_0 phong_spotlight_normal_mapping_vs_main();
		PixelShader			= compile ps_2_0 phong_spotlight_normal_mapping_ps_main();
		
		CullMode			= CCW;
		
		ZEnable				= true;
		ZWriteEnable		= false;
		ZFunc				= equal;
		
		AlphaBlendEnable	= true;
		SrcBlend			= One;
		DestBlend			= One;	
	}
}


technique PhongSpotLightNormalMappingShadowed
{
	pass P0
	{
		VertexShader		= compile vs_2_0 phong_spotlight_normal_mapping_shadowed_vs_main();
		PixelShader			= compile ps_2_0 phong_spotlight_normal_mapping_shadowed_ps_main();
		
		CullMode			= CCW;
		
		ZEnable				= true;
		ZWriteEnable		= false;
		ZFunc				= equal;
		
		AlphaBlendEnable	= true;
		SrcBlend			= One;
		DestBlend			= One;	
	}
}

/*
technique PhongDirectional
{	
	pass PhongDirectional
	{
		VertexShader		= compile vs_2_0 phong_directional_vs_main();
		PixelShader			= compile ps_2_0 phong_directional_ps_main();
	
		CullMode			= CCW;
		
		ZEnable				= true;
		ZWriteEnable		= false;
		ZFunc				= equal;	
		
		AlphaBlendEnable	= true;
		
		SrcBlend = One;
        DestBlend = One;	
	}
}

technique PhongPointLight
{
	pass PhongPointLight
	{
		VertexShader		= compile vs_2_0 phong_pointlight_vs_main();
		PixelShader			= compile ps_2_0 phong_pointlight_ps_main();
		
		CullMode			= CCW;
		
		ZEnable				= true;
		ZWriteEnable		= false;
		ZFunc				= equal;
		
		AlphaBlendEnable	= true;
		
		SrcBlend = One;
        DestBlend = One;
	}
}
*/
