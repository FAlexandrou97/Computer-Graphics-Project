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

// ADDED
struct VS_NORMALMAP_INPUT
{
	float3 Pos     : POSITION;
	float3 Normal  : NORMAL;
	float3 Tangent : TANGENT;
	float2 UV      : TEXCOORD0;
};

// ADDED
struct VS_NORMALMAP_OUTPUT
{
	float4 ProjPos      : SV_POSITION;
	float3 WorldPos     : POSITION;
	float3 ModelNormal  : NORMAL;
	float3 ModelTangent : TANGENT;
	float2 UV           : TEXCOORD0;
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
float3 Light1Pos;
float3 Light1Colour;
float3 Light2Pos;
float3 Light2Colour;
float3 TeapotLight1Pos;
float3 TeapotLight1Colour;
float3 TeapotLight2Pos;
float3 TeapotLight2Colour;
float3 TeapotLight3Pos;
float3 TeapotLight3Colour;

float3 TintColour;
float3 AmbientColour;
float  SpecularPower;
float ParallaxDepth;

// Diffuse texture map (the main texture colour) - may contain specular map in alpha channel
Texture2D DiffuseMap;

Texture2D NormalMap;

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

	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul(modelPos, WorldMatrix);
	vOut.WorldPos = worldPos;
	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
	float4 viewPos = mul(worldPos, ViewMatrix);
	vOut.ProjPos = mul(viewPos, ProjMatrix);

	// Transform the vertex normal from model space into world space (almost same as first lines of code above)
	float4 modelNormal = float4(vIn.Normal, 0.0f); // Set 4th element to 0.0 this time as normals are vectors
	float4 worldNormal = mul(modelNormal, WorldMatrix);

	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
	worldNormal = normalize(worldNormal);
	vOut.WorldNormal = worldNormal;

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
	vOut.UV = vIn.UV;

	return vOut;
}

// ADDED
VS_NORMALMAP_OUTPUT NormalMapTransform(VS_NORMALMAP_INPUT vIn)
{
	VS_NORMALMAP_OUTPUT vOut;

	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul(modelPos, WorldMatrix);
	vOut.WorldPos = worldPos.xyz;

	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
	float4 viewPos = mul(worldPos, ViewMatrix);
	vOut.ProjPos = mul(viewPos, ProjMatrix);

	// Just send the model's normal and tangent untransformed (in model space). The pixel shader will do the matrix work on normals
	vOut.ModelNormal = vIn.Normal;
	vOut.ModelTangent = vIn.Tangent;

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
	diffuseMapColour.rgb *= TintColour / 10;

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
	float3 worldNormal = normalize(vOut.WorldNormal);


	///////////////////////
	// Calculate lighting

	// Calculate direction of camera
	float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)

	//// LIGHT 1
	float3 Light1Dir = normalize(TeapotLight1Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	float3 Light1Dist = length(TeapotLight1Pos - vOut.WorldPos.xyz);
	float3 DiffuseLight1 = TeapotLight1Colour * saturate(dot(worldNormal.xyz, Light1Dir)) / Light1Dist;
	float3 halfway = normalize(Light1Dir + CameraDir);
	float3 SpecularLight1 = DiffuseLight1 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);

	//// LIGHT 2
	float3 Light2Dir = normalize(TeapotLight2Pos - vOut.WorldPos.xyz);
	float3 Light2Dist = length(TeapotLight2Pos - vOut.WorldPos.xyz);
	float3 DiffuseLight2 = TeapotLight2Colour * saturate(dot(worldNormal.xyz, Light2Dir)) / Light2Dist;
	halfway = normalize(Light2Dir + CameraDir);
	float3 SpecularLight2 = DiffuseLight2 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);


	//// LIGHT 3
	float3 Light3Dir = normalize(TeapotLight3Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	float3 Light3Dist = length(TeapotLight3Pos - vOut.WorldPos.xyz);
	float3 DiffuseLight3 = TeapotLight3Colour * saturate(dot(worldNormal.xyz, Light1Dir)) / Light3Dist;
	halfway = normalize(Light3Dir + CameraDir);
	float3 SpecularLight3 = DiffuseLight3 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);

	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 DiffuseLight = AmbientColour + DiffuseLight1 + DiffuseLight2 + DiffuseLight3;
	float3 SpecularLight = SpecularLight1 + SpecularLight2 + SpecularLight3;


	////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture (using float3, so we get RGB - i.e. ignore any alpha in the texture)
	float4 DiffuseMaterial = DiffuseMap.Sample(TrilinearWrap, vOut.UV);

	// Assume specular material colour is white (i.e. highlights are a full, untinted reflection of light)
	float3 SpecularMaterial = DiffuseMaterial.a;


	////////////////////
	// Combine colours 

	// Combine maps and lighting for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = DiffuseMaterial * DiffuseLight + SpecularMaterial * SpecularLight;
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}

// ADDED
float4 NormalMapLighting(VS_NORMALMAP_OUTPUT vOut) : SV_Target
{
	float3 modelNormal = normalize(vOut.ModelNormal);
	float3 modelTangent = normalize(vOut.ModelTangent);

	float3 modelBiTangent = cross(modelNormal, modelTangent);
	float3x3 invTangentMatrix = float3x3(modelTangent, modelBiTangent, modelNormal);

	// Calculate direction of camera
	float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)
	
	float3x3 invWorldMatrix = transpose(WorldMatrix);
	float3 cameraModelDir = normalize(mul(CameraDir, invWorldMatrix));

	float3x3 tangentMatrix = transpose(invTangentMatrix);
	float2 textureOffsetDir = mul(cameraModelDir, tangentMatrix);

	float texDepth = ParallaxDepth * (NormalMap.Sample(TrilinearWrap, vOut.UV).a - 0.5f);
	float2 offsetTexCoord = vOut.UV + texDepth * textureOffsetDir;

	// Get the texture normal from the normal map + parallax map
	float3 textureNormal = 2.0f * NormalMap.Sample(TrilinearWrap, offsetTexCoord) - 1.0f; // Scale from 0->1 to -1->1

	// Now convert the texture normal into model space using the inverse tangent matrix, and then convert into world space using the world
	// matrix. Normalise, because of the effects of texture filtering and in case the world matrix contains scaling
	float3 worldNormal = normalize(mul(mul(textureNormal, invTangentMatrix), WorldMatrix));

	///////////////////////
	// Calculate lighting

	//// LIGHT 1
	float3 Light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	float3 Light1Dist = length(Light1Pos - vOut.WorldPos.xyz);
	float3 DiffuseLight1 = Light1Colour * max(dot(worldNormal.xyz, Light1Dir), 0) / Light1Dist;
	float3 halfway = normalize(Light1Dir + CameraDir);
	float3 SpecularLight1 = DiffuseLight1 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);

	//// LIGHT 2
	float3 Light2Dir = normalize(Light2Pos - vOut.WorldPos.xyz);
	float3 Light2Dist = length(Light2Pos - vOut.WorldPos.xyz);
	float3 DiffuseLight2 = Light2Colour * max(dot(worldNormal.xyz, Light2Dir), 0) / Light2Dist;
	halfway = normalize(Light2Dir + CameraDir);
	float3 SpecularLight2 = DiffuseLight2 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);


	//// TEAPOT LIGHT 1
	Light1Dir = normalize(TeapotLight1Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	Light1Dist = length(TeapotLight1Pos - vOut.WorldPos.xyz);
	float3 TeapotDiffuseLight1 = TeapotLight1Colour * saturate(dot(worldNormal.xyz, Light1Dir)) / Light1Dist;
	halfway = normalize(Light1Dir + CameraDir);
	float3 TeapotSpecularLight1 = TeapotDiffuseLight1 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);

	//// TEAPOT LIGHT 2
	Light2Dir = normalize(TeapotLight2Pos - vOut.WorldPos.xyz);
	Light2Dist = length(TeapotLight2Pos - vOut.WorldPos.xyz);
	float3 TeapotDiffuseLight2 = TeapotLight2Colour * saturate(dot(worldNormal.xyz, Light2Dir)) / Light2Dist;
	halfway = normalize(Light2Dir + CameraDir);
	float3 TeapotSpecularLight2 = TeapotDiffuseLight2 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);


	//// TEAPOT LIGHT 3
	float3 Light3Dir = normalize(TeapotLight3Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	float3 Light3Dist = length(TeapotLight3Pos - vOut.WorldPos.xyz);
	float3 TeapotDiffuseLight3 = TeapotLight3Colour * saturate(dot(worldNormal.xyz, Light1Dir)) / Light3Dist;
	halfway = normalize(Light3Dir + CameraDir);
	float3 TeapotSpecularLight3 = TeapotDiffuseLight3 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);
	
	// Sum the effect of all lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 DiffuseLight = AmbientColour + DiffuseLight1 + DiffuseLight2 + TeapotDiffuseLight1 + TeapotDiffuseLight2 + TeapotDiffuseLight3;
	float3 SpecularLight = SpecularLight1 + SpecularLight2 + TeapotSpecularLight1 + TeapotSpecularLight2 + TeapotSpecularLight3;


	////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture (using float3, so we get RGB - i.e. ignore any alpha in the texture)
	float4 DiffuseMaterial = DiffuseMap.Sample(TrilinearWrap, offsetTexCoord);

	// Assume specular material colour is white (i.e. highlights are a full, untinted reflection of light)
	float3 SpecularMaterial = DiffuseMaterial.a;


	////////////////////
	// Combine colours 

	// Combine maps and lighting for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = DiffuseMaterial * DiffuseLight + SpecularMaterial * SpecularLight;
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}

//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

// States are needed to switch between additive blending for the lights and no blending for other models
RasterizerState CullNone  // Cull none of the polygons, i.e. show both sides
{
	CullMode = None;
};

RasterizerState CullBack  // Cull back side of polygon - normal behaviour, only show front of polygons
{
	CullMode = Back;
};

DepthStencilState DepthWritesOff // Don't write to the depth buffer - polygons rendered will not obscure other polygons
{
	DepthWriteMask = ZERO;
};

DepthStencilState DepthWritesOn  // Write to the depth buffer - normal behaviour 
{
	DepthWriteMask = ALL;
};

BlendState NoBlending // Switch off blending - pixels will be opaque
{
	BlendEnable[0] = FALSE;
};

BlendState AdditiveBlending // Additive blending is used for lighting effects
{
	BlendEnable[0] = TRUE;
	SrcBlend = ONE;
	DestBlend = ONE;
	BlendOp = ADD;
};

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
technique10 AdditiveTexTint
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, BasicTransform()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, TintDiffuseMap()));

		SetBlendState(AdditiveBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullNone);
		SetDepthStencilState(DepthWritesOff, 0);
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

		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullBack);
		SetDepthStencilState(DepthWritesOn, 0);
	}
}

// Vertex lighting with diffuse map
technique10 VertexLitTechnique
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, VertexLightingTex()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, VertexLitDiffuseMap()));

		// Switch off blending states
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullBack);
		SetDepthStencilState(DepthWritesOn, 0);
	}
}

technique10 NormalMapping
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, NormalMapTransform()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, NormalMapLighting()));

		// Switch off blending states
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullBack);
		SetDepthStencilState(DepthWritesOn, 0);
	}
}