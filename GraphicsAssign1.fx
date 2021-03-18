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
float3 CubeLightPos;
float3 CubeLightColour;
float3 CarLightPos;
float3 CarLightColour;
float3 TeapotLight1Pos;
float3 TeapotLight1Colour;
float3 TeapotLight2Pos;
float3 TeapotLight2Colour;
float3 TeapotLight3Pos;
float3 TeapotLight3Colour;
float3   SpotLight1Pos;
float3   SpotLight2Pos;
float3   SpotLight1Facing;
float3   SpotLight2Facing;
float4x4 SpotLight1ViewMatrix;
float4x4 SpotLight2ViewMatrix;
float4x4 SpotLight1ProjMatrix;
float4x4 SpotLight2ProjMatrix;
float    SpotLight1CosHalfAngle;
float    SpotLight2CosHalfAngle;
float3   SpotLight1Colour;
float3   SpotLight2Colour;

float3 TintColour;
float3 AmbientColour;
float  SpecularPower;
float ParallaxDepth;

// Diffuse texture map (the main texture colour) - may contain specular map in alpha channel
Texture2D DiffuseMap;
Texture2D CellMap;
Texture2D NormalMap;
Texture2D ShadowMap1;
Texture2D ShadowMap2;

// Sampler to use with the diffuse/normal maps. Specifies texture filtering and addressing mode to use when accessing texture pixels
SamplerState TrilinearWrap
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};
SamplerState PointClamp
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
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


VS_BASIC_OUTPUT ExpandOutline(VS_BASIC_INPUT vIn)
{
	VS_BASIC_OUTPUT vOut;

	// Transform model-space vertex position to world-space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul(modelPos, WorldMatrix);

	// Next the usual transform from world space to camera space - but we don't go any further here - this will be used to help expand the outline
	// The result "viewPos" is the xyz position of the vertex as seen from the camera. The z component is the distance from the camera - that's useful...
	float4 viewPos = mul(worldPos, ViewMatrix);

	// Transform model normal to world space, using the normal to expand the geometry, not for lighting
	float4 modelNormal = float4(vIn.Normal, 0.0f); // Set 4th element to 0.0 this time as normals are vectors
	float4 worldNormal = normalize(mul(modelNormal, WorldMatrix)); // Normalise in case of world matrix scaling

	// Now we return to the world position of this vertex and expand it along the world normal - that will expand the geometry outwards.
	// Use the distance from the camera to decide how much to expand. Use this distance together with a sqrt to creates an outline that
	// gets thinner in the distance, but always remains clear. Overall thickness is also controlled by the constant "OutlineThickness"
	worldPos += 0.015 * sqrt(viewPos.z) * worldNormal;

	// Transform new expanded world-space vertex position to viewport-space and output
	viewPos = mul(worldPos, ViewMatrix);
	vOut.ProjPos = mul(viewPos, ProjMatrix);

	return vOut;
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------


// A pixel shader that just outputs a single fixed colour
//
float4 OneColour( VS_BASIC_OUTPUT vOut ) : SV_Target
{
	vOut.UV.x += 0.1f;
	vOut.UV.y += 0.1f;
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

	//// CUBE LIGHT
	float3 Light1Dir = normalize(CubeLightPos - vOut.WorldPos.xyz);   // Direction for each light is different
	float3 Light1Dist = length(CubeLightPos - vOut.WorldPos.xyz);
	float3 DiffuseLight1 = CubeLightColour * max(dot(worldNormal.xyz, Light1Dir), 0) / Light1Dist;
	float3 halfway = normalize(Light1Dir + CameraDir);
	float3 SpecularLight1 = DiffuseLight1 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);

	//// CAR LIGHT
	Light1Dir = normalize(CarLightPos - vOut.WorldPos.xyz);   // Direction for each light is different
	Light1Dist = length(CarLightPos - vOut.WorldPos.xyz);
	float3 CarDiffuseLight = CarLightColour * saturate(dot(worldNormal.xyz, Light1Dir)) / Light1Dist;
	halfway = normalize(Light1Dir + CameraDir);
	float3 CarSpecularLight = CarDiffuseLight * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);

	//// TEAPOT LIGHT 1
	Light1Dir = normalize(TeapotLight1Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	Light1Dist = length(TeapotLight1Pos - vOut.WorldPos.xyz);
	float3 TeapotDiffuseLight1 = TeapotLight1Colour * saturate(dot(worldNormal.xyz, Light1Dir)) / Light1Dist;
	halfway = normalize(Light1Dir + CameraDir);
	float3 TeapotSpecularLight1 = TeapotDiffuseLight1 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);

	//// TEAPOT LIGHT 2
	float3 Light2Dir = normalize(TeapotLight2Pos - vOut.WorldPos.xyz);
	float3 Light2Dist = length(TeapotLight2Pos - vOut.WorldPos.xyz);
	float3 TeapotDiffuseLight2 = TeapotLight2Colour * saturate(dot(worldNormal.xyz, Light2Dir)) / Light2Dist;
	halfway = normalize(Light2Dir + CameraDir);
	float3 TeapotSpecularLight2 = TeapotDiffuseLight2 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);


	//// TEAPOT LIGHT 3
	float3 Light3Dir = normalize(TeapotLight3Pos - vOut.WorldPos.xyz);   // Direction for each light is different
	float3 Light3Dist = length(TeapotLight3Pos - vOut.WorldPos.xyz);
	float3 TeapotDiffuseLight3 = TeapotLight3Colour * saturate(dot(worldNormal.xyz, Light1Dir)) / Light3Dist;
	halfway = normalize(Light3Dir + CameraDir);
	float3 TeapotSpecularLight3 = TeapotDiffuseLight3 * pow(saturate(dot(worldNormal.xyz, halfway)), SpecularPower);
	

	//// SPOT LIGHT 1
	// Slight adjustment to calculated depth of pixels so they don't shadow themselves
	const float DepthAdjust = 0.0005f;

	// Start with no light contribution from this light
	float3 DiffuseSpotLight1 = 0;
	float3 SpecularSpotLight1 = 0;

	// Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	// pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	// The usual view / projection matrix multiplies as we would see in a vertex shader (can improve performance by putting these lines in vertex shader)
	float4 light1ViewPos = mul(float4(vOut.WorldPos, 1.0f), SpotLight1ViewMatrix);
	float4 light1ProjPos = mul(light1ViewPos, SpotLight1ProjMatrix);

	// Get direction from pixel to light
	Light1Dir = normalize(SpotLight1Pos - vOut.WorldPos.xyz);

	// Check if pixel is within light cone
	if (dot(SpotLight1Facing, -Light1Dir) > SpotLight1CosHalfAngle) //**** This condition needs to be written as the first exercise to get spotlights working
	{
		// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
		float2 shadowUV = 0.5f * light1ProjPos.xy / light1ProjPos.w + float2(0.5f, 0.5f);
		shadowUV.y = 1.0f - shadowUV.y;

		// Get depth of this pixel if it were visible from the light (another advanced projection step)
		float depthFromLight = light1ProjPos.z / light1ProjPos.w - DepthAdjust; //*** Adjustment so polygons don't shadow themselves

		// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		// to the light than this pixel - so the pixel gets no effect from this light
		if (depthFromLight < ShadowMap1.Sample(PointClamp, shadowUV).r)
		{
			// Remainder of standard per-pixel lighting code is unchanged
			float3 light1Dist = length(SpotLight1Pos - vOut.WorldPos.xyz);
			DiffuseSpotLight1 = SpotLight1Colour * max(dot(worldNormal.xyz, Light1Dir), 0) / light1Dist;
			float3 halfway = normalize(Light1Dir + CameraDir);
			SpecularSpotLight1 = DiffuseSpotLight1 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
		}
	}


	//// SPOT LIGHT 2

	float3 DiffuseSpotLight2 = 0;
	float3 SpecularSpotLight2 = 0;

	// Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	// pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	// The usual view / projection matrix multiplies as we would see in a vertex shader (can improve performance by putting these lines in vertex shader)
	float4 Light2ViewPos = mul(float4(vOut.WorldPos, 1.0f), SpotLight2ViewMatrix);
	float4 Light2ProjPos = mul(Light2ViewPos, SpotLight2ProjMatrix);

	// Get direction from pixel to light
	Light2Dir = normalize(SpotLight2Pos - vOut.WorldPos.xyz);


	// Check if pixel is within light cone
	if (dot(SpotLight2Facing, -Light2Dir) > SpotLight2CosHalfAngle) //**** This condition needs to be written as the first exercise to get spotlights working
	{
		// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
		float2 shadowUV = 0.5f * Light2ProjPos.xy / Light2ProjPos.w + float2(0.5f, 0.5f);
		shadowUV.y = 1.0f - shadowUV.y;

		// Get depth of this pixel if it were visible from the light (another advanced projection step)
		float depthFromLight = Light2ProjPos.z / Light2ProjPos.w - DepthAdjust; //*** Adjustment so polygons don't shadow themselves

		// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		// to the light than this pixel - so the pixel gets no effect from this light
		if (depthFromLight < ShadowMap2.Sample(PointClamp, shadowUV).r)
		{
			// Remainder of standard per-pixel lighting code is unchanged
			float3 Light2Dist = length(SpotLight2Pos - vOut.WorldPos.xyz);
			DiffuseSpotLight2 = SpotLight2Colour * max(dot(worldNormal.xyz, Light2Dir), 0) / Light2Dist;
			float3 halfway = normalize(Light2Dir + CameraDir);
			SpecularSpotLight2 = DiffuseSpotLight2 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
		}
	}

	// Sum the effect of all lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 DiffuseLight = AmbientColour + DiffuseLight1 + CarDiffuseLight + TeapotDiffuseLight1 + TeapotDiffuseLight2 + TeapotDiffuseLight3 + DiffuseSpotLight1 + DiffuseSpotLight2;
	float3 SpecularLight = SpecularLight1 + CarSpecularLight + TeapotSpecularLight1 + TeapotSpecularLight2 + TeapotSpecularLight3 + SpecularSpotLight1 + SpecularSpotLight2;


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

float4 ShadowMapTex( VS_LIGHTING_OUTPUT vOut ) : SV_Target  // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	// Slight adjustment to calculated depth of pixels so they don't shadow themselves
	const float DepthAdjust = 0.0005f;

	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
	float3 worldNormal = normalize(vOut.WorldNormal); 

	///////////////////////
	// Calculate lighting

	// Calculate direction of camera
	float3 cameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)

	//----------
	// LIGHT 1

	// Start with no light contribution from this light
	float3 diffuseLight1 = 0;
	float3 specularLight1 = 0;

	// Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	// pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	// The usual view / projection matrix multiplies as we would see in a vertex shader (can improve performance by putting these lines in vertex shader)
	float4 light1ViewPos = mul( float4(vOut.WorldPos), SpotLight1ViewMatrix ); 
	float4 light1ProjPos = mul( light1ViewPos, SpotLight1ProjMatrix );

	// Get direction from pixel to light
	float3 light1Dir = normalize(SpotLight1Pos - vOut.WorldPos.xyz);


	// Check if pixel is within light cone
	if ( dot(SpotLight1Facing, -light1Dir ) > SpotLight1CosHalfAngle) //**** This condition needs to be written as the first exercise to get spotlights working
	{
		// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
		float2 shadowUV = 0.5f * light1ProjPos.xy / light1ProjPos.w + float2( 0.5f, 0.5f );
		shadowUV.y = 1.0f - shadowUV.y;

		// Get depth of this pixel if it were visible from the light (another advanced projection step)
		float depthFromLight = light1ProjPos.z / light1ProjPos.w - DepthAdjust; //*** Adjustment so polygons don't shadow themselves
		
		// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		// to the light than this pixel - so the pixel gets no effect from this light
		if (depthFromLight < ShadowMap1.Sample( PointClamp, shadowUV ).r)
		{
			// Remainder of standard per-pixel lighting code is unchanged
			float3 light1Dist = length(SpotLight1Pos - vOut.WorldPos.xyz); 
			diffuseLight1 = SpotLight1Colour * max( dot(worldNormal.xyz, light1Dir), 0 ) / light1Dist;
			float3 halfway = normalize(light1Dir + cameraDir);
			specularLight1 = diffuseLight1 * pow( max( dot(worldNormal.xyz, halfway), 0 ), SpecularPower );
		}
	}


	//----------
	// LIGHT 2

	// Start with no light contribution from this light
	float3 diffuseLight2 = 0;
	float3 specularLight2 = 0;

	// Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	// pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	// The usual view / projection matrix multiplies as we would see in a vertex shader (can improve performance by putting these lines in vertex shader)
	float4 Light2ViewPos = mul(float4(vOut.WorldPos), SpotLight2ViewMatrix);
	float4 Light2ProjPos = mul(Light2ViewPos, SpotLight2ProjMatrix);

	// Get direction from pixel to light
	float3 Light2Dir = normalize(SpotLight2Pos - vOut.WorldPos.xyz);


	// Check if pixel is within light cone
	if (dot(SpotLight2Facing, -Light2Dir) > SpotLight2CosHalfAngle) //**** This condition needs to be written as the first exercise to get spotlights working
	{
		// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
		float2 shadowUV = 0.5f * Light2ProjPos.xy / Light2ProjPos.w + float2(0.5f, 0.5f);
		shadowUV.y = 1.0f - shadowUV.y;

		// Get depth of this pixel if it were visible from the light (another advanced projection step)
		float depthFromLight = Light2ProjPos.z / Light2ProjPos.w - DepthAdjust; //*** Adjustment so polygons don't shadow themselves

		// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		// to the light than this pixel - so the pixel gets no effect from this light
		if (depthFromLight < ShadowMap2.Sample(PointClamp, shadowUV).r)
		{
			// Remainder of standard per-pixel lighting code is unchanged
			float3 Light2Dist = length(SpotLight2Pos - vOut.WorldPos.xyz);
			diffuseLight2 = SpotLight2Colour * max(dot(worldNormal.xyz, Light2Dir), 0) / Light2Dist;
			float3 halfway = normalize(Light2Dir + cameraDir);
			specularLight2 = diffuseLight2 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
		}
	}

	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 diffuseLight = AmbientColour + diffuseLight1 + diffuseLight2;
	float3 specularLight = specularLight1 + specularLight2;


	////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture
	float4 diffuseMaterial = DiffuseMap.Sample( TrilinearWrap, vOut.UV );
	
	// Get specular material colour from texture alpha
	float3 specularMaterial = diffuseMaterial.a;

	
	////////////////////
	// Combine colours 
	
	// Combine maps and lighting for final pixel colour
	float4 combinedColour;
	combinedColour.rgb = diffuseMaterial * (diffuseLight + diffuseLight2)+ specularMaterial * (specularLight + specularLight2);
	combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

	return combinedColour;
}

// Shader used when rendering the shadow map depths. In fact a pixel shader isn't needed, we are
// only writing to the depth buffer. However, needed to display what's in a shadow map (which we
// do as one of the exercises).
float4 PixelDepth(VS_BASIC_OUTPUT vOut) : SV_Target
{
	// Output the value that would go in the depth puffer to the pixel colour (greyscale)
	return vOut.ProjPos.z / vOut.ProjPos.w;
}

float4 CellShadingVertexLitDiffuseMap(VS_LIGHTING_OUTPUT vOut) : SV_Target  // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
	float3 worldNormal = normalize(vOut.WorldNormal);
	// Calculate direction of camera
	float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)

	///////////////////////
	// Calculate lighting

	//// SPOT LIGHT 1
	// Slight adjustment to calculated depth of pixels so they don't shadow themselves
	const float DepthAdjust = 0.0005f;

	// Start with no light contribution from this light
	float3 DiffuseSpotLight1 = 0;
	float3 SpecularSpotLight1 = 0;

	// Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	// pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	// The usual view / projection matrix multiplies as we would see in a vertex shader (can improve performance by putting these lines in vertex shader)
	float4 light1ViewPos = mul(float4(vOut.WorldPos), SpotLight1ViewMatrix);
	float4 light1ProjPos = mul(light1ViewPos, SpotLight1ProjMatrix);

	// Get direction from pixel to light
	float3 Light1Dir = normalize(SpotLight1Pos - vOut.WorldPos.xyz);

	// Check if pixel is within light cone
	if (dot(SpotLight1Facing, -Light1Dir) > SpotLight1CosHalfAngle) //**** This condition needs to be written as the first exercise to get spotlights working
	{
		// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
		float2 shadowUV = 0.5f * light1ProjPos.xy / light1ProjPos.w + float2(0.5f, 0.5f);
		shadowUV.y = 1.0f - shadowUV.y;

		// Get depth of this pixel if it were visible from the light (another advanced projection step)
		float depthFromLight = light1ProjPos.z / light1ProjPos.w - DepthAdjust; //*** Adjustment so polygons don't shadow themselves

		// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		// to the light than this pixel - so the pixel gets no effect from this light
		if (depthFromLight < ShadowMap1.Sample(PointClamp, shadowUV).r)
		{
			// Remainder of standard per-pixel lighting code is unchanged
			float3 light1Dist = length(SpotLight1Pos - vOut.WorldPos.xyz);
			DiffuseSpotLight1 = SpotLight1Colour * max(dot(worldNormal.xyz, Light1Dir), 0) / light1Dist;
			float3 halfway = normalize(Light1Dir + CameraDir);
			SpecularSpotLight1 = DiffuseSpotLight1 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
		}
	}

	//// SPOT LIGHT 2
	float3 DiffuseSpotLight2 = 0;
	float3 SpecularSpotLight2 = 0;

	// Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	// pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	// The usual view / projection matrix multiplies as we would see in a vertex shader (can improve performance by putting these lines in vertex shader)
	float4 Light2ViewPos = mul(float4(vOut.WorldPos), SpotLight2ViewMatrix);
	float4 Light2ProjPos = mul(Light2ViewPos, SpotLight2ProjMatrix);

	// Get direction from pixel to light
	float3 Light2Dir = normalize(SpotLight2Pos - vOut.WorldPos.xyz);


	// Check if pixel is within light cone
	if (dot(SpotLight2Facing, -Light2Dir) > SpotLight2CosHalfAngle) //**** This condition needs to be written as the first exercise to get spotlights working
	{
		// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
		// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
		float2 shadowUV = 0.5f * Light2ProjPos.xy / Light2ProjPos.w + float2(0.5f, 0.5f);
		shadowUV.y = 1.0f - shadowUV.y;

		// Get depth of this pixel if it were visible from the light (another advanced projection step)
		float depthFromLight = Light2ProjPos.z / Light2ProjPos.w - DepthAdjust; //*** Adjustment so polygons don't shadow themselves

		// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
		// to the light than this pixel - so the pixel gets no effect from this light
		if (depthFromLight < ShadowMap2.Sample(PointClamp, shadowUV).r)
		{
			// Remainder of standard per-pixel lighting code is unchanged
			float3 Light2Dist = length(SpotLight2Pos - vOut.WorldPos.xyz);
			DiffuseSpotLight2 = SpotLight2Colour * max(dot(worldNormal.xyz, Light2Dir), 0) / Light2Dist;
			float3 halfway = normalize(Light2Dir + CameraDir);
			SpecularSpotLight2 = DiffuseSpotLight2 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
		}
	}

	//// CAR LIGHT 
	float3 CarLightDir = normalize(CarLightPos - vOut.WorldPos.xyz);  
	float3 CarLightDist = length(CarLightPos - vOut.WorldPos.xyz);

	//****| INFO |*************************************************************************************//
	// To make a cartoon look to the lighting, we clamp the basic light level to just a small range of
	// colours. This is done by using the light level itself as the U texture coordinate to look up
	// a colour in a special 1D texture (a single line). This could be done with if statements, but
	// GPUs are much faster at looking up small textures than if statements
	//*************************************************************************************************//
	float DiffuseLevel = max(dot(worldNormal.xyz, CarLightDir), 0);
	float CellDiffuseLevel = CellMap.Sample(PointClamp, DiffuseLevel).r;
	float3 CarDiffuseLight = CarLightColour * CellDiffuseLevel / CarLightDist;

	// Do same for specular light and further lights
	float3 halfway = normalize(CarLightDir + CameraDir);
	float SpecularLevel = pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
	float CellSpecularLevel = CellMap.Sample(PointClamp, SpecularLevel).r;
	float3 CarSpecularLight = CarDiffuseLight * CellSpecularLevel;


	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
	float3 DiffuseLight = AmbientColour + CarDiffuseLight + DiffuseSpotLight1 + DiffuseSpotLight2;
	float3 SpecularLight = CarSpecularLight + CarSpecularLight + SpecularSpotLight1 + SpecularSpotLight2;


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

RasterizerState CullFront  // Cull back side of polygon - unsusual behaviour for special techniques
{
	CullMode = Front;
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

technique10 ParallaxMappingTechnique
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

technique10 ShadowMappingTechnique
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, VertexLightingTex()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ShadowMapTex()));

		// Switch off blending states
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullBack);
		SetDepthStencilState(DepthWritesOn, 0);
	}
}

// Rendering a shadow map. Only outputs the depth of each pixel
technique10 DepthOnlyTechnique
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, BasicTransform()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PixelDepth()));

		// Switch off blending states
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullBack);
		SetDepthStencilState(DepthWritesOn, 0);
	}
}

technique10 CellShadingTechnique
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, ExpandOutline()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, OneColour()));

		// Cull (remove) the polygons facing us - i.e. draw the inside of the model
		SetRasterizerState(CullFront);

		// Switch off other blending states
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetDepthStencilState(DepthWritesOn, 0);
	}
	pass P1
	{
		SetVertexShader(CompileShader(vs_4_0, VertexLightingTex()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, CellShadingVertexLitDiffuseMap()));

		// Return to standard culling (remove polygons facing away from us)
		SetRasterizerState(CullBack);
	}
}