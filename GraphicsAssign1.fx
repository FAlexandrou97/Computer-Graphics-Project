//--------------------------------------------------------------------------------------
// File: GraphicsAssign1.fx
//
//	Shaders Graphics Assignment
//	Add further models using different shader techniques
//	See assignment specification for details
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// Standard input geometry data, more complex techniques (e.g. normal mapping) may need more
struct VS_BASIC_INPUT
{
    float3 Pos    : POSITION;
    float3 Normal : NORMAL;
	float2 UV     : TEXCOORD0;
};

// Data output from vertex shader to pixel shader for simple techniques. Again different techniques have different requirements
struct VS_BASIC_OUTPUT
{
    float4 ProjPos : SV_POSITION;
    float2 UV      : TEXCOORD0;
};

// ADDED
// The vertex shader processes the input geometry above and outputs data to be used by the rest of the pipeline. This is the output
// used in the lighting technique - containing a 2D position, lighting colours and texture coordinates
struct VS_LIGHTING_OUTPUT
{
	float4 ProjPos       : SV_POSITION;  // 2D "projected" position for vertex (required output for vertex shader)
	float4 WorldPos		 : POSITION;
	float4 WorldNormal   : NORMAL;
	float2 UV            : TEXCOORD0;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
// All these variables are created & manipulated in the C++ code and passed into the shader here

// The matrices (4x4 matrix of floats) for transforming from 3D model to 2D projection (used in vertex shader)
float4x4 WorldMatrix;
float4x4 ViewMatrix;
float4x4 ProjMatrix;

// A single colour for an entire model - used for light models and the intial basic shader
float3 ModelColour;
// Variable used for the wiggle effect
float Wiggle;

// Lighting variables
float3 CameraPos;
float3 TeapotLight1Pos;
float3 TeapotLight1Colour;
float3 TeapotLight2Pos;
float3 TeapotLight2Colour;
float3 TeapotLight3Pos;
float3 TeapotLight3Colour;

float3 AmbientColour;
float  SpecularPower;
// Diffuse texture map (the main texture colour) - may contain specular map in alpha channel
Texture2D DiffuseMap;

// Sampler to use with the diffuse/normal maps. Specifies texture filtering and addressing mode to use when accessing texture pixels
SamplerState TrilinearWrap
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------

// Basic vertex shader to transform 3D model vertices to 2D and pass UVs to the pixel shader
//
VS_BASIC_OUTPUT BasicTransform( VS_BASIC_INPUT vIn )
{
	VS_BASIC_OUTPUT vOut;
	
	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );
	
	// Pass texture coordinates (UVs) on to the pixel shader
	vOut.UV = vIn.UV;

	return vOut;
}

// ADDED
// The vertex shader will process each of the vertices in the model, typically transforming/projecting them into 2D at a minimum.
// This vertex shader also calculates the light colour at each vertex and also passes on UVs so the later stages can use textures
//
VS_LIGHTING_OUTPUT VertexLightingTex(VS_BASIC_INPUT vIn)
{
	VS_LIGHTING_OUTPUT vOut;

	//*********************************************************************************************
	// Transform vertices from model into world space and then into 2D
	// This is the core task for most vertex shaders

	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul(modelPos, WorldMatrix);
	vOut.WorldPos = worldPos;
	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
	float4 viewPos = mul(worldPos, ViewMatrix);
	vOut.ProjPos = mul(viewPos, ProjMatrix);



	//*********************************************************************************************
	// Transform normals from model into world space
	// Normals are used for lighting. Even if the vertex shader doesn't do lighting it's convenient
	// to do this calculation here as it's very similar to the vertex transform above

	// Transform the vertex normal from model space into world space (almost same as first lines of code above)
	float4 modelNormal = float4(vIn.Normal, 0.0f); // Set 4th element to 0.0 this time as normals are vectors
	float4 worldNormal = mul(modelNormal, WorldMatrix);


	//*********************************************************************************************
	// Calculate lighting
	// Can be done in vertex or pixel shader. Need world position and normal of vertex (or pixel)
	// as well as camera and light postition (also in world space) and light info (e.g. colour)

	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
	worldNormal = normalize(worldNormal);
	vOut.WorldNormal = worldNormal;


	//*********************************************************************************************
	// Pass data unused by vertex shader to remainder of pipeline

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
	vOut.UV = vIn.UV;


	return vOut;
}

//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------


// A pixel shader that just outputs a single fixed colour
//
float4 OneColour( VS_BASIC_OUTPUT vOut ) : SV_Target
{
	return float4( ModelColour, 1.0 ); // Set alpha channel to 1.0 (opaque)
}

// ADDED
// A pixel shader that just tints a (diffuse) texture
//
float4 TintDiffuseMap(VS_BASIC_OUTPUT vOut) : SV_Target
{
	// Extract diffuse material colour for this pixel from a texture
	float4 diffuseMapColour = DiffuseMap.Sample(TrilinearWrap, vOut.UV);

	return diffuseMapColour;
}

// ADDED
float3 WigglePixelShader(VS_BASIC_OUTPUT vOut) : SV_Target  // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	float SinY = sin(vOut.UV.y * radians(360.0f) + Wiggle);
	float SinX = sin(vOut.UV.x * radians(360.0f) + Wiggle);
	vOut.UV.x += 0.1f * SinY;
	vOut.UV.y += 0.1f * SinX;

	float4 colour;
	float3 TexColour = DiffuseMap.Sample(TrilinearWrap, vOut.UV);
	colour.rgb = TexColour;
	
	return TexColour;
}

// ADDED
// The pixel shader determines colour for each pixel in the rendered polygons, given the data passed on from the vertex shader
// This shader expects lighting colours and UVs from the vertex shader. It combines a diffuse map with this calculated lighting to create output pixel colour
//
float4 VertexLitDiffuseMap(VS_LIGHTING_OUTPUT vOut) : SV_Target  // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	// Calculate direction of light and camera
	float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)
	float3 LightDir = normalize(TeapotLight1Pos - vOut.WorldPos.xyz);   // Same for light
	float3 Light2Dir = normalize(TeapotLight2Pos - vOut.WorldPos.xyz);
	float3 Light3Dir = normalize(TeapotLight3Pos - vOut.WorldPos.xyz);

	// Calculate lighting on this vertex (or pixel) - equations from lecture
	float3 DiffuseLight = AmbientColour + TeapotLight1Colour * max(dot(vOut.WorldNormal.xyz, LightDir), 0);
	float3 Diffuse2Light = AmbientColour + TeapotLight2Colour * max(dot(vOut.WorldNormal.xyz, Light2Dir), 0);
	float3 Diffuse3Light = AmbientColour + TeapotLight3Colour * max(dot(vOut.WorldNormal.xyz, Light3Dir), 0);

	float3 halfway = normalize(LightDir + CameraDir);
	float3 halfway2 = normalize(Light2Dir + CameraDir);
	float3 halfway3 = normalize(Light3Dir + CameraDir);
	float3 SpecularLight = TeapotLight1Colour * pow(max(dot(vOut.WorldNormal.xyz, halfway), 0), SpecularPower);
	float3 Specular2Light = TeapotLight2Colour * pow(max(dot(vOut.WorldNormal.xyz, halfway2), 0), SpecularPower);
	float3 Specular3Light = TeapotLight3Colour * pow(max(dot(vOut.WorldNormal.xyz, halfway3), 0), SpecularPower);


	//*********************************************************************************************
	// Get colours from texture maps (only diffuse map supported at start of lab)

	// Extract diffuse material colour for this pixel from a texture (using float3, so we get RGB - i.e. ignore any alpha in the texture)
	float3 DiffuseMaterial = DiffuseMap.Sample(TrilinearWrap, vOut.UV).rgb;

	// Assume specular material colour is white (i.e. highlights are a full, untinted reflection of light)
	float3 SpecularMaterial = 1.0f;



	//*********************************************************************************************
	// Combine colours (lighting, textures) for final pixel colour

	float4 combinedColour;
	combinedColour.rgb = DiffuseMaterial * (DiffuseLight + Diffuse2Light + Diffuse3Light) + SpecularMaterial * (SpecularLight + Specular2Light + Specular3Light);
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

// Techniques are used to render models in our scene. They select a combination of vertex, geometry and pixel shader from those provided above. Can also set states.

// Render models unlit in a single colour
technique10 PlainColour
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, BasicTransform() ) );
        SetGeometryShader( NULL );                                   
        SetPixelShader( CompileShader( ps_4_0, OneColour() ) );
	}
}

// ADDED
technique10 TintDiffuse
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, BasicTransform()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, TintDiffuseMap()));
	}
}

// ADDED
technique10 WiggleTechnique
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, BasicTransform()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, WigglePixelShader()));
	}
}