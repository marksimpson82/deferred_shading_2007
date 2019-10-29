#line 2 "shared.fx"

// nvidia vars
static const half  blurBlendFactor   = 0.25f;
static const half3 LUMINANCE_WEIGHTS = half3(0.27f, 0.67f, 0.06f);

const float   g_fExposure;
const float   g_fFrameDelta;

#define MAX_SAMPLES 16

// Contains weights for Gaussian blurring
const float  weights[MAX_SAMPLES];

// misc
const float4 g_fvDebugWireframeColour;

// Common matrices
const float4x4 matWorld					: World;
const float4x4 matView					: View;
const float4x4 matWorldView				: WorldView;
const float4x4 matWorldViewProjection	: WorldViewProjection;


//#############################################################################
//
// SAMPLERS
//
//#############################################################################
texture TextureA;
texture TextureB;
texture TextureC;
texture ScaledTexA;
texture ScaledTexB;

// Shadow tex
texture shadowMapRT_Tex;		// hardware shadow map texture (depth stencil)
texture shadowColourMapRT_Tex;	// colour shadow map texture (debugging)

sampler2D AnisoSampler = sampler_state
{
    Texture       = <TextureA>;
    AddressU      = CLAMP;
    AddressV      = CLAMP;
    MinFilter     = ANISOTROPIC;
    MagFilter     = LINEAR;
    MipFilter     = NONE;
    MaxAnisotropy = 16;
};

sampler2D SamplerTex = sampler_state 
{
    Texture   = <TextureA>;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
};

sampler2D SamplerTexC = sampler_state 
{
    Texture   = <TextureC>;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
};

sampler2D SamplerA = sampler_state 
{
    Texture     = <TextureA>;
    AddressU    = WRAP;
    AddressV    = WRAP;
    MinFilter   = LINEAR;
    MagFilter   = LINEAR;
    MipFilter   = LINEAR;
    SRGBTexture = TRUE;
};

sampler2D SamplerB = sampler_state 
{
    Texture     = <TextureB>;
    AddressU    = WRAP;
    AddressV    = WRAP;
    MinFilter   = LINEAR;
    MagFilter   = LINEAR;
    MipFilter   = LINEAR;
    SRGBTexture = TRUE;
};

sampler2D SamplerScaledTexA = sampler_state 
{
    Texture     = <ScaledTexA>;
    AddressU    = CLAMP;
    AddressV    = CLAMP;
    MinFilter   = LINEAR;
    MagFilter   = LINEAR;
    MipFilter   = LINEAR;
    SRGBTexture = TRUE;
};

sampler2D SamplerScaledTexB = sampler_state 
{
    Texture     = <ScaledTexB>;
    AddressU    = CLAMP;
    AddressV    = CLAMP;
    MinFilter   = LINEAR;
    MagFilter   = LINEAR;
    MipFilter   = LINEAR;
    SRGBTexture = TRUE;
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

//#############################################################################
struct PS_Input_4 {
  float4 texCoord[2] : TEXCOORD0;
};

struct PS_Input_8 {
  float4 texCoord[4] : TEXCOORD0;
};

struct VS_OutputVTF
{
    float4 Pos : POSITION;
    float3 Tex : TEXCOORD0;
};

//#############################################################################
//
// VIGNETTING
//
//#############################################################################
void vignette(inout float3 c, const float2 win_bias)
{
	// convert window coord to [-1, 1] range
	float2 wpos = 2*(win_bias - (float2)(0.5, 0.5)); 

	// calculate distance from origin
	float r = length(wpos.xy);
	r = 1.0 - smoothstep(0.8, 1.5, r);
	c = c * r;
}

//#############################################################################
//
// TONE MAPPING
//
//#############################################################################
VS_OutputVTF VS_toneMapping( in float4 position: POSITION )
{
	VS_OutputVTF OUT;
	
	OUT = (VS_OutputVTF)0;

	OUT.Tex.z  = g_fExposure / max(0.001f, tex2Dlod( SamplerTexC, float4(0.5f, 0.5f, 0.0f, 0.0f)).x);
		
	OUT.Pos    = float4(position.x, position.y, 0.5f, 1.0f);
	OUT.Tex.xy = position.zw;
	
	return OUT;
}

float4 PS_toneMapping(in float3 Tex: TEXCOORD0) : COLOR
{
    // sum original and blurred image
    half3 c = (1.0f - blurBlendFactor) * tex2D(SamplerA, Tex.xy).rgb +  blurBlendFactor * tex2D(SamplerScaledTexA, Tex.xy).rgb;
    
    half3 L = c * Tex.z;
    
    // exposure
    c = L / (1 + L); 
    
    // vignette effect (makes brightness drop off with distance from center)
    vignette(c, Tex.xy); 
      
	return float4(c, 1.0);
}

float4 PS_toneMappingx2(in float3 Tex: TEXCOORD0) : COLOR
{
    // sum original and blurred image
    half3 c;
  
  	c.rg = (1.0f - blurBlendFactor) * tex2D(SamplerA, Tex.xy).rg +  blurBlendFactor * tex2D(SamplerScaledTexA, Tex.xy).rg;
	c.b  = (1.0f - blurBlendFactor) * tex2D(SamplerB, Tex.xy).r  +  blurBlendFactor * tex2D(SamplerScaledTexB, Tex.xy).r;

    half3 L = c * Tex.z;
    
    // exposure
    c = L / (1 + L); 
    
    // vignette effect (makes brightness drop off with distance from center)
 	vignette(c, Tex.xy); 

    return float4(c, 1.0);
}

//===================================================================
technique ToneMapping
{
    Pass Main
    {
		VertexShader = compile vs_3_0 VS_toneMapping();
		PixelShader  = compile ps_3_0 PS_toneMapping();
    }
}

//===================================================================
technique ToneMappingx2
{
    Pass Main
    {  
		VertexShader = compile vs_3_0 VS_toneMapping();
        PixelShader  = compile ps_3_0 PS_toneMappingx2();
    }
}

//#############################################################################
//
// BLUR
//
//#############################################################################
float4 PS_downScale4x4(in PS_Input_4 IN) : COLOR
{
	half4 sample = 0.0f;

	for( int i = 0; i < 2; i++ )
	{
		sample += tex2D( SamplerTex, IN.texCoord[i].xy);
		sample += tex2D( SamplerTex, IN.texCoord[i].zw);
	}
	
	sample *= 0.25f;
	
	return sample;
	
}

float4 PS_downScaleAniso(in float2 Tex: TEXCOORD0) : COLOR
{
	
	return tex2D( AnisoSampler, Tex.xy);
	
}

float4 PS_blurBilinear(in PS_Input_8 IN) : COLOR
{
    
    half4 sample = 0.0f;

	for( int i = 0; i < 4; i++ )
	{
		sample += weights[2*i + 0] * tex2D( SamplerTex, IN.texCoord[i].xy);
		sample += weights[2*i + 1] * tex2D( SamplerTex, IN.texCoord[i].zw);
	}
	
	return sample;
}

//=============================================================
technique Downscale4x4Bilinear
{
    pass Main
    {
        PixelShader  = compile ps_2_0 PS_downScale4x4();
    }
}

//=============================================================
technique DownscaleAniso
{
    pass Main
    {
        PixelShader  = compile ps_2_0 PS_downScaleAniso();
    }
}

//=============================================================
technique Blur1DBilinear
{
    Pass Main
    {
       PixelShader = compile ps_2_0 PS_blurBilinear();
    }
}

//#############################################################################
//
// LUMINANCE
//
//#############################################################################
float4 PS_downscaleLuminanceLog(in float4 Tex[2]: TEXCOORD0) : COLOR
{
	half luminance = 0; 
		
	for( int i = 0; i < 2; i++ )
	{
		luminance += log(dot(tex2D( SamplerA, Tex[i].xy), LUMINANCE_WEIGHTS)+ 0.0001);
		luminance += log(dot(tex2D( SamplerA, Tex[i].zw), LUMINANCE_WEIGHTS)+ 0.0001);
	}
	
	luminance *= 0.25f;
	
	return float4(luminance, 0.0f, 0.0f, 0.0f);
}

float4 PS_downscaleLuminanceLogx2(in float4 Tex[2]: TEXCOORD0) : COLOR
{
    half3 color;
    half luminance = 0; 
    
    for( int i = 0; i < 2; i++ )
	{
		color.xy = tex2D(SamplerA, Tex[i].xy).xy;
		color.z  = tex2D(SamplerB, Tex[i].xy).x;
    
		luminance += log(dot(color, LUMINANCE_WEIGHTS) + 0.0001);
		
		color.xy = tex2D(SamplerA, Tex[i].zw).xy;
		color.z  = tex2D(SamplerB, Tex[i].zw).x;
    
		luminance += log(dot(color, LUMINANCE_WEIGHTS) + 0.0001);
	
	}
	luminance *= 0.25f;
		
	return float4(luminance, 0.0f, 0.0f, 0.0f);
}

float4 PS_downscaleLuminance(in PS_Input_4 IN) : COLOR
{
	half sample = 0.0f;

	for( int i = 0; i < 2; i++ )
	{
		sample += tex2D( SamplerA, IN.texCoord[i].xy).x;
		sample += tex2D( SamplerA, IN.texCoord[i].zw).x;
	}
	
	sample *= 0.25f;
	
	return sample;
	
}

float4 PS_downscaleLuminanceExp(in PS_Input_4 IN) : COLOR
{
	half sample = 0.0f;

	for( int i = 0; i < 2; i++ )
	{
		sample += tex2D( SamplerA, IN.texCoord[i].xy).x;
		sample += tex2D( SamplerA, IN.texCoord[i].zw).x;
	}
	
	sample *= 0.25f;
	
	return exp(sample);
}

float4 PS_LuminanceAdaptation(in float2 Tex: TEXCOORD0) : COLOR
{
	half adaptedLum = tex2D(SamplerA, float2(0.5f, 0.5f));
    half currentLum = tex2D(SamplerB, float2(0.5f, 0.5f));
   
    return (adaptedLum + (currentLum - adaptedLum) * ( 1 - pow( 0.98f, 50 * g_fFrameDelta ) ));
}

float4 PS_PassThrough(in float2 Tex : TEXCOORD0 ) : COLOR
{
	return( tex2D( SamplerTex, Tex ));
}

//=============================================================
technique DownscaleLuminanceLog
{
    Pass Main
    {
       PixelShader = compile ps_2_0 PS_downscaleLuminanceLog();
    }
}

//=============================================================
technique DownscaleLuminanceLogx2
{
    Pass Main
    {
       PixelShader = compile ps_2_0 PS_downscaleLuminanceLogx2();
    }
}

//=============================================================
technique DownscaleLuminance
{
    Pass Main
    {
       PixelShader = compile ps_2_0 PS_downscaleLuminance();
    }
}

//=============================================================
technique DownscaleLuminanceExp
{
    Pass Main
    {
       PixelShader = compile ps_2_0 PS_downscaleLuminanceExp();
    }
}

//=============================================================
technique LuminanceAdaptation
{
    Pass Main
    {
       PixelShader = compile ps_2_0 PS_LuminanceAdaptation();
    }
}

technique PassThrough
{
	Pass Main
	{
		PixelShader = compile ps_2_0 PS_PassThrough();
	}
}

//--------------------------------------------------------------//
// Shader definitions
//--------------------------------------------------------------//

float4 pos_vs_main( in float4 inPos : POSITION ) : POSITION
{	
	float4 clipPos = mul( inPos, matWorldViewProjection );
	return clipPos;
}

//--------------------------------------------------------------//
// DEBUG shader pass -- draw point light wireframe
//--------------------------------------------------------------//
//--------------------------------------------------------------//
// Shader definitions
//--------------------------------------------------------------//

struct POSTEX
{
	float4 Position		: POSITION0;
	float2 TexCoords	: TEXCOORD0;
};

// uses existing VS 
float4 fullbright_colour_ps( ) : COLOR
{
	return g_fvDebugWireframeColour;
}

technique Debug_ShowWireframe
{
	pass Pass0_Wireframe
	{
		VertexShader	= compile vs_1_1 pos_vs_main();
		PixelShader		= compile ps_2_0 fullbright_colour_ps();
		
		CullMode		= CCW;
		
		ZEnable			= true;
		ZWriteEnable	= false;
				
		FillMode		= WIREFRAME;
	}
}

POSTEX RenderToShadowMapVS( POSTEX Input )
{
    POSTEX Out;

    // transform position to light projection space.  matWorldViewProjection should be world * lightView * lightProj.
    float4 position = mul( Input.Position, matWorldViewProjection );  
        
    Out.Position = position;	// for rasteriser
    Out.TexCoords.xy = position.zw;	// for outputting depth as a greyscale value in the pixel shader (debugging)

	return Out;
}

float4 RenderToShadowMapPS( in float2 TexCoords : TEXCOORD0 ) : COLOR
{
	float depth = TexCoords.x / TexCoords.y;	
    return float4( depth.xxx, 1.0f );        
}

float4 draw_hardware_shadow_map_ps_main( in float2 TexCoords : TEXCOORD0 ) : COLOR
{
	return float4( tex2D( shadowColourMapSampler, TexCoords ).rgb, 1.0 );
}

float4 ClipSpaceVS( in float4 Position : POSITION ) : POSITION
{
	return( mul( Position, matWorldViewProjection ));
}

technique DepthOnly
{
	pass P0
	{
		VertexShader		= compile vs_1_1 ClipSpaceVS();
		PixelShader			= null;
		
		AlphaTestEnable		= false;
		AlphaBlendEnable	= false;

		CullMode			= CCW;
		      
		ZEnable				= true;
		ZWriteEnable		= true;		
						
		ColorWriteEnable	= 0;
	}
}

technique RenderToShadowMap
{
	pass P0
	{
		VertexShader		= compile vs_1_1 RenderToShadowMapVS();
		PixelShader			= compile ps_2_0 RenderToShadowMapPS();
		//PixelShader			= null;
		
		AlphaTestEnable		= false;
		AlphaBlendEnable	= false;

		// could use front face culling to (slightly) improve the biasing issues?
		//CullMode			= CW;
		
		// commented out, as we need colour writes enabled to write to the PS when debugging.
		// If we are not debugging, disable it using the device call instead:
		// pDevice->SetRenderState( D3DRS_COLORWRITEENABLE, ( m_bDisplayShadowMap ) ? 0xf : 0 );
		
		//ColorWriteEnable	= 0;
		
		FillMode			= Solid;
	}
}

// Debug technique used to display the contents of a shadow map
technique DisplayHardwareShadowMap
{
	pass P0
	{
//		VertexShader		= compile vs_2_0 posclip_texcoords_vs_main();
		PixelShader			= compile ps_2_0 draw_hardware_shadow_map_ps_main();
		
		CullMode			= None;	
		ZEnable				= false;	
		ZWriteEnable		= false;
	}	
}

