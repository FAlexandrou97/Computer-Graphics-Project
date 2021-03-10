//--------------------------------------------------------------------------------------
//	GraphicsAssign1.cpp
//
//	Shaders Graphics Assignment
//	Add further models using different shader techniques
//	See assignment specification for details
//--------------------------------------------------------------------------------------

//***|  INFO  |****************************************************************
// Lights:
//   The initial project shows models for a couple of point lights, but the
//   initial shaders don't actually apply any lighting. Part of the assignment
//   is to add a shader that uses this information to actually light a model.
//   Refer to lab work to determine how best to do this.
// 
// Textures:
//   The models in the initial scene have textures loaded but the initial
//   technique/shaders doesn't show them. Part of the assignment is to add 
//   techniques to show a textured model
//
// Shaders:
//   The initial shaders render plain coloured objects with no lighting:
//   - The vertex shader performs basic transformation of 3D vertices to 2D
//   - The pixel shader colours every pixel the same colour
//   A few shader variables are set up to communicate between C++ and HLSL
//   You will usually need to add further variables to add new techniques
//*****************************************************************************

#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>
#include <atlbase.h>
#include "resource.h"

#include "Defines.h" // General definitions shared by all source files
#include "Model.h"   // Model class - encapsulates working with vertex/index data and world matrix
#include "Camera.h"  // Camera class - encapsulates the camera's view and projection matrix
#include "Input.h"   // Input functions - not DirectX
#include "Light.h"

//--------------------------------------------------------------------------------------
// Global Scene Variables
//--------------------------------------------------------------------------------------
float g_WiggleVar;
const int g_numLights = 3;

// Models and cameras encapsulated in classes for flexibity and convenience
// The CModel class collects together geometry and world matrix, and provides functions to control the model and render it
// The CCamera class handles the view and projections matrice, and provides functions to control the camera
CModel* Cube;
CModel* Box;
CModel* Floor;
CCamera* Camera;

// Textures - no texture class yet so using DirectX variables
ID3D10ShaderResourceView* CubeDiffuseMap = NULL;
ID3D10ShaderResourceView* LightDiffuseMap = NULL;
ID3D10ShaderResourceView* BoxDiffuseMap = NULL;
ID3D10ShaderResourceView* BoxNormalMap = NULL;
ID3D10ShaderResourceView* FloorDiffuseMap = NULL;
ID3D10ShaderResourceView* StoneDiffuseMap = NULL;

// Light data - stored manually as there is no light class
D3DXVECTOR3 AmbientColour = D3DXVECTOR3( 0.15f, 0.15f, 0.15f );
float SpecularPower = 256.0f;
// Display models where the lights are. One of the lights will follow an orbit
CLight* Light1;
CLight* Light2;
CLight* TeapotLights[g_numLights];
CLight* TeapotLight1;
CLight* TeapotLight2;
CLight* TeapotLight3;

CModel* Teapot;
// Note: There are move & rotation speed constants in Defines.h

//--------------------------------------------------------------------------------------
// Shader Variables
//--------------------------------------------------------------------------------------
// Variables to connect C++ code to HLSL shaders

// Effects / techniques
ID3D10Effect*          Effect = NULL;
ID3D10EffectTechnique* PlainColourTechnique = NULL;
ID3D10EffectTechnique* TintDiffuse = NULL;
ID3D10EffectTechnique* WiggleTechnique = NULL;
ID3D10EffectTechnique* VertexLitTechnique = NULL;
ID3D10EffectTechnique* AdditiveTexTintTechnique = NULL;
ID3D10EffectTechnique* NormalMappingTechnique = NULL;

// Matrices
ID3D10EffectMatrixVariable* WorldMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* ProjMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewProjMatrixVar = NULL;
ID3D10EffectScalarVariable* g_pCubeWiggleVar = NULL;

// Textures
ID3D10EffectShaderResourceVariable* DiffuseMapVar = NULL;
ID3D10EffectShaderResourceVariable* NormalMapVar = NULL;

// Miscellaneous
ID3D10EffectVectorVariable* ModelColourVar = NULL;

// Light Effect Variables
ID3D10EffectVectorVariable* Light1PosVar = NULL;
ID3D10EffectVectorVariable* Light1ColourVar = NULL;
ID3D10EffectVectorVariable* Light2PosVar = NULL;
ID3D10EffectVectorVariable* Light2ColourVar = NULL;
ID3D10EffectVectorVariable* TeapotLight1PosVar = NULL;
ID3D10EffectVectorVariable* TeapotLight1ColourVar = NULL;
ID3D10EffectVectorVariable* TeapotLight2PosVar = NULL;
ID3D10EffectVectorVariable* TeapotLight2ColourVar = NULL;
ID3D10EffectVectorVariable* TeapotLight3PosVar = NULL;
ID3D10EffectVectorVariable* TeapotLight3ColourVar = NULL;

ID3D10EffectVectorVariable* TintColourVar = NULL;
ID3D10EffectVectorVariable* AmbientColourVar = NULL;
ID3D10EffectScalarVariable* SpecularPowerVar = NULL;


//--------------------------------------------------------------------------------------
// DirectX Variables
//--------------------------------------------------------------------------------------

// The main D3D interface, this pointer is used to access most D3D functions (and is shared across all cpp files through Defines.h)
ID3D10Device* g_pd3dDevice = NULL;

// Width and height of the window viewport
int g_ViewportWidth;
int g_ViewportHeight;

// Variables used to setup D3D
IDXGISwapChain*         SwapChain = NULL;
ID3D10Texture2D*        DepthStencil = NULL;
ID3D10DepthStencilView* DepthStencilView = NULL;
ID3D10RenderTargetView* RenderTargetView = NULL;


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
bool InitDevice(HWND hWnd)
{
	// Many DirectX functions return a "HRESULT" variable to indicate success or failure. Microsoft code often uses
	// the FAILED macro to test this variable, you'll see it throughout the code - it's fairly self explanatory.
	HRESULT hr = S_OK;


	////////////////////////////////
	// Initialise Direct3D

	// Calculate the visible area the window we are using - the "client rectangle" refered to in the first function is the 
	// size of the interior of the window, i.e. excluding the frame and title
	RECT rc;
	GetClientRect(hWnd, &rc);
	g_ViewportWidth = rc.right - rc.left;
	g_ViewportHeight = rc.bottom - rc.top;


	// Create a Direct3D device (i.e. initialise D3D), and create a swap-chain (create a back buffer to render to)
	DXGI_SWAP_CHAIN_DESC sd;         // Structure to contain all the information needed
	ZeroMemory( &sd, sizeof( sd ) ); // Clear the structure to 0 - common Microsoft practice, not really good style
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_ViewportWidth;             // Target window size
	sd.BufferDesc.Height = g_ViewportHeight;           // --"--
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Pixel format of target window
	sd.BufferDesc.RefreshRate.Numerator = 60;          // Refresh rate of monitor
	sd.BufferDesc.RefreshRate.Denominator = 1;         // --"--
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.OutputWindow = hWnd;                            // Target window
	sd.Windowed = TRUE;                                // Whether to render in a window (TRUE) or go fullscreen (FALSE)
	hr = D3D10CreateDeviceAndSwapChain( NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
										D3D10_SDK_VERSION, &sd, &SwapChain, &g_pd3dDevice );
	if( FAILED( hr ) ) return false;

	// Specify the render target as the back-buffer - this is an advanced topic. This code almost always occurs in the standard D3D setup
	ID3D10Texture2D* pBackBuffer;
	hr = SwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer );
	if( FAILED( hr ) ) return false;
	hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &RenderTargetView );
	pBackBuffer->Release();
	if( FAILED( hr ) ) return false;


	// Create a texture (bitmap) to use for a depth buffer
	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width = g_ViewportWidth;
	descDepth.Height = g_ViewportHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &DepthStencil );
	if( FAILED( hr ) ) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView( DepthStencil, &descDSV, &DepthStencilView );
	if( FAILED( hr ) ) return false;

	// Select the back buffer and depth buffer to use for rendering now
	g_pd3dDevice->OMSetRenderTargets( 1, &RenderTargetView, DepthStencilView );


	// Setup the viewport - defines which part of the window we will render to, almost always the whole window
	D3D10_VIEWPORT vp;
	vp.Width  = g_ViewportWidth;
	vp.Height = g_ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	return true;
}


// Release the memory held by all objects created
void ReleaseResources()
{
	// The D3D setup and preparation of the geometry created several objects that use up memory (e.g. textures, vertex/index buffers etc.)
	// Each object that allocates memory (or hardware resources) needs to be "released" when we exit the program
	// There is similar code in every D3D program, but the list of objects that need to be released depends on what was created
	// Test each variable to see if it exists before deletion
	if( g_pd3dDevice )     g_pd3dDevice->ClearState();

	delete Light2;
	delete Light1;
	delete Floor;
	delete Cube;
	delete Box;
	delete Camera;
	delete Teapot;
	delete TeapotLight1;
	delete TeapotLight2;
	delete TeapotLight3;

	for (int i = 0; i < g_numLights; i++) {
		delete TeapotLights[i];
	}

    if( FloorDiffuseMap )  FloorDiffuseMap->Release();
    if( CubeDiffuseMap )   CubeDiffuseMap->Release();
	if( LightDiffuseMap )  LightDiffuseMap->Release();
    if( BoxDiffuseMap )    BoxDiffuseMap->Release();
    if( BoxNormalMap )     BoxNormalMap->Release();
    if( StoneDiffuseMap )  StoneDiffuseMap->Release();
	if( Effect )           Effect->Release();
	if( DepthStencilView ) DepthStencilView->Release();
	if( RenderTargetView ) RenderTargetView->Release();
	if( DepthStencil )     DepthStencil->Release();
	if( SwapChain )        SwapChain->Release();
	if( g_pd3dDevice )     g_pd3dDevice->Release();

}



//--------------------------------------------------------------------------------------
// Load and compile Effect file (.fx file containing shaders)
//--------------------------------------------------------------------------------------
// An effect file contains a set of "Techniques". A technique is a combination of vertex, geometry and pixel shaders (and some states) used for
// rendering in a particular way. We load the effect file at runtime (it's written in HLSL and has the extension ".fx"). The effect code is compiled
// *at runtime* into low-level GPU language. When rendering a particular model we specify which technique from the effect file that it will use
//
bool LoadEffectFile()
{
	ID3D10Blob* pErrors; // This strangely typed variable collects any errors when compiling the effect file
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS; // These "flags" are used to set the compiler options

	// Load and compile the effect file
	HRESULT hr = D3DX10CreateEffectFromFile( L"GraphicsAssign1.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL, NULL, &Effect, &pErrors, NULL );
	if( FAILED( hr ) )
	{
		if (pErrors != 0)  MessageBox( NULL, CA2CT(reinterpret_cast<char*>(pErrors->GetBufferPointer())), L"Error", MB_OK ); // Compiler error: display error message
		else               MessageBox( NULL, L"Error loading FX file. Ensure your FX file is in the same folder as this executable.", L"Error", MB_OK );  // No error message - probably file not found
		return false;
	}

	// Now we can select techniques from the compiled effect file
	PlainColourTechnique = Effect->GetTechniqueByName("PlainColour");
	TintDiffuse = Effect->GetTechniqueByName("TintDiffuse");
	WiggleTechnique = Effect->GetTechniqueByName("WiggleTechnique");
	VertexLitTechnique = Effect->GetTechniqueByName("VertexLitTechnique");
	AdditiveTexTintTechnique = Effect->GetTechniqueByName("AdditiveTexTint");
	NormalMappingTechnique = Effect->GetTechniqueByName("NormalMapping");

	// Create special variables to allow us to access global variables in the shaders from C++
	WorldMatrixVar    = Effect->GetVariableByName( "WorldMatrix" )->AsMatrix();
	ViewMatrixVar     = Effect->GetVariableByName( "ViewMatrix"  )->AsMatrix();
	ProjMatrixVar     = Effect->GetVariableByName( "ProjMatrix"  )->AsMatrix();

	// We access the texture variable in the shader in the same way as we have before for matrices, light data etc.
	// Only difference is that this variable is a "Shader Resource"
	DiffuseMapVar = Effect->GetVariableByName( "DiffuseMap" )->AsShaderResource();
	NormalMapVar  = Effect->GetVariableByName("NormalMap")->AsShaderResource();

	// Other shader variables
	ModelColourVar = Effect->GetVariableByName( "ModelColour"  )->AsVector();
	g_pCubeWiggleVar = Effect->GetVariableByName("Wiggle")->AsScalar();

	// Access shader variables needed for lighting
	Light1ColourVar = Effect->GetVariableByName("Light1Colour")->AsVector();
	Light1PosVar = Effect->GetVariableByName("Light1Pos")->AsVector();
	Light2ColourVar = Effect->GetVariableByName("Light2Colour")->AsVector();
	Light2PosVar = Effect->GetVariableByName("Light2Pos")->AsVector();
	TeapotLight1ColourVar = Effect->GetVariableByName("TeapotLight1Colour")->AsVector();
	TeapotLight1PosVar = Effect->GetVariableByName("TeapotLight1Pos")->AsVector();
	TeapotLight2ColourVar = Effect->GetVariableByName("TeapotLight2Colour")->AsVector();
	TeapotLight2PosVar = Effect->GetVariableByName("TeapotLight2Pos")->AsVector();
	TeapotLight3ColourVar = Effect->GetVariableByName("TeapotLight3Colour")->AsVector();
	TeapotLight3PosVar = Effect->GetVariableByName("TeapotLight3Pos")->AsVector();

	TintColourVar = Effect->GetVariableByName("TintColour")->AsVector();
	AmbientColourVar = Effect->GetVariableByName("AmbientColour")->AsVector();
	SpecularPowerVar = Effect->GetVariableByName("SpecularPower")->AsScalar();

	return true;
}



//--------------------------------------------------------------------------------------
// Scene Setup / Update / Rendering
//--------------------------------------------------------------------------------------

// Create / load the camera, models and textures for the scene
bool InitScene()
{
	//////////////////
	// Create camera

	Camera = new CCamera();
	Camera->SetPosition( D3DXVECTOR3(-15, 20,-40) );
	Camera->SetRotation( D3DXVECTOR3(ToRadians(13.0f), ToRadians(18.0f), 0.0f) ); // ToRadians is a new helper function to convert degrees to radians


	///////////////////////
	// Load/Create models

	Cube = new CModel;
	Box = new CModel;
	Floor = new CModel;
	Light1 = new CLight;
	Light2 = new CLight;
	Teapot = new CModel;
	TeapotLight1 = new CLight;
	TeapotLight2 = new CLight;
	TeapotLight3 = new CLight;

	// The model class can load ".X" files. It encapsulates (i.e. hides away from this code) the file loading/parsing and creation of vertex/index buffers
	// We must pass an example technique used for each model. We can then only render models with techniques that uses matching vertex input data
	if (!Cube->  Load( "Cube.x",  PlainColourTechnique )) return false;
	if (!Box->  Load( "CardboardBox.x", NormalMappingTechnique, true)) return false;
	if (!Floor-> Load( "Floor.x", PlainColourTechnique )) return false;
	if (!Light1->Load( "Light.x", AdditiveTexTintTechnique )) return false;
	if (!Light2->Load( "Light.x", AdditiveTexTintTechnique)) return false;
	if (!Teapot->Load( "Teapot.x", VertexLitTechnique )) return false;
	if (!TeapotLight1->Load("Light.x", AdditiveTexTintTechnique)) return false;
	if (!TeapotLight2->Load("Light.x", AdditiveTexTintTechnique)) return false;
	if (!TeapotLight3->Load("Light.x", AdditiveTexTintTechnique)) return false;
	for (int i = 0; i < g_numLights; i++) {
		TeapotLights[i] = new CLight;
		if (!TeapotLights[i]->Load("Light.x", AdditiveTexTintTechnique)) return false;
	}
	// Initial positions
	Cube->SetPosition( D3DXVECTOR3(-20, 5, 0) );
	Box->SetPosition( D3DXVECTOR3(30, 10, 0) );
	Box->SetScale( 8.0f );
	Light1->SetPosition( D3DXVECTOR3(30, 10, 0) );
	Light1->SetScale( 4.0f ); // Nice if size of light reflects its brightness
	Light1->SetColour(D3DXVECTOR3(1.0f, 2.0f, 0.7f) * 5);
	Light2->SetPosition( D3DXVECTOR3(-20, 30, 50) );
	Light2->SetScale(4.0f );
	Light2->SetColour(D3DXVECTOR3(1.0f, 0.8f, 0.2f) * 5);

	Teapot->SetPosition(D3DXVECTOR3(0, 0, 80));
	TeapotLights[0]->SetPosition(D3DXVECTOR3(0, 15, 80));
	TeapotLights[0]->SetScale(4.0f);
	TeapotLights[0]->SetColour(D3DXVECTOR3(1.0f, 0.0f, 0.7f) * 5);
	TeapotLights[1]->SetPosition(D3DXVECTOR3(20, 5, 80));
	TeapotLights[1]->SetScale(4.0f);
	TeapotLights[1]->SetColour(D3DXVECTOR3(1.0f, 0.4f, 0.2f) * 5);
	TeapotLights[2]->SetPosition(D3DXVECTOR3(-20, 5, 80));
	TeapotLights[2]->SetScale(4.0f);
	TeapotLights[2]->SetColour(D3DXVECTOR3(0.4f, 0.4f, 0.1f) * 5);

	//////////////////
	// Load textures

	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"StoneDiffuseSpecular.dds", NULL, NULL, &CubeDiffuseMap,  NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"WoodDiffuseSpecular.dds",  NULL, NULL, &FloorDiffuseMap, NULL ) )) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"StoneDiffuseSpecular.dds", NULL, NULL, &StoneDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"PatternNormal.dds", NULL, NULL, &BoxNormalMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"PatternDiffuseSpecular.dds", NULL, NULL, &BoxDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"flare.jpg", NULL, NULL, &LightDiffuseMap, NULL))) return false;

	return true;
}


// Update the scene - move/rotate each model and the camera, then update their matrices
void UpdateScene( float frameTime )
{
	// Control camera position and update its matrices (view matrix, projection matrix) each frame
	// Don't be deceived into thinking that this is a new method to control models - the same code we used previously is in the camera class
	Camera->Control( frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
	Camera->UpdateMatrices();
	
	// Control cube position and update its world matrix each frame
	Cube->Control( frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );
	Cube->UpdateMatrix();

	Box->UpdateMatrix();

	// Wiggle effect
	g_WiggleVar += 6 * frameTime;
	g_pCubeWiggleVar->SetFloat(g_WiggleVar);

	// Update the orbiting light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float Rotate = 0.0f;
	Light1->SetPosition( Box->GetPosition() + D3DXVECTOR3(cos(Rotate)*Light1->GetOrbitRadius(), 5, sin(Rotate)*Light1->GetOrbitRadius()) );
	Rotate -= Light1->GetOrbitSpeed() * frameTime;
	Light1->UpdateMatrix();
	// Second light doesn't move, but do need to make sure its matrix has been calculated - could do this in InitScene instead
	Light2->UpdateMatrix();

	// Update teapot lights
	const float colourUpdateSpeed = frameTime * 5;

	TeapotLights[0]->Pulsate(D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(5.0f, 0.0f, 3.5f), colourUpdateSpeed);
	TeapotLights[1]->ChangeColour(colourUpdateSpeed);
	TeapotLights[2]->ChangeColour(colourUpdateSpeed * 2);


	Teapot->UpdateMatrix();
	for (int i = 0; i < g_numLights; i++) {
		TeapotLights[i]->UpdateMatrix();
	}
}


// Render everything in the scene
void RenderScene()
{
	// Clear the back buffer - before drawing the geometry clear the entire window to a fixed colour
	float ClearColor[4] = { 0.2f, 0.2f, 0.3f, 1.0f }; // Good idea to match background to ambient colour
	g_pd3dDevice->ClearRenderTargetView( RenderTargetView, ClearColor );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 ); // Clear the depth buffer too


	//---------------------------
	// Common rendering settings

	// Common features for all models, set these once only

	// Pass the camera's matrices to the vertex shader
	ViewMatrixVar->SetMatrix( (float*)&Camera->GetViewMatrix() );
	ProjMatrixVar->SetMatrix( (float*)&Camera->GetProjectionMatrix() );

	// Pass light information to the vertex shader
	Light1PosVar->SetRawValue(Light1->GetPosition(), 0, 12);  // Send 3 floats (12 bytes) from C++ LightPos variable (x,y,z) to shader counterpart (middle parameter is unused) 
	Light1ColourVar->SetRawValue(Light1->GetColour(), 0, 12);
	Light2PosVar->SetRawValue(Light2->GetPosition(), 0, 12);
	Light2ColourVar->SetRawValue(Light2->GetColour(), 0, 12);
	TeapotLight1PosVar->SetRawValue(TeapotLights[0]->GetPosition(), 0, 12);
	TeapotLight1ColourVar->SetRawValue(TeapotLights[0]->GetColour(), 0, 12);
	TeapotLight2PosVar->SetRawValue(TeapotLights[1]->GetPosition(), 0, 12);
	TeapotLight2ColourVar->SetRawValue(TeapotLights[1]->GetColour(), 0, 12);
	TeapotLight3PosVar->SetRawValue(TeapotLights[2]->GetPosition(), 0, 12);
	TeapotLight3ColourVar->SetRawValue(TeapotLights[2]->GetColour(), 0, 12);
	AmbientColourVar->SetRawValue(AmbientColour, 0, 12);
	SpecularPowerVar->SetFloat(SpecularPower);

	//---------------------------
	// Render each model
	
	// Constant colours used for models in initial shaders
	D3DXVECTOR3 Black( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 Blue( 0.0f, 0.0f, 1.0f );

	// Cube
	WorldMatrixVar->SetMatrix( (float*)Cube->GetWorldMatrix() );  // Send the cube's world matrix to the shader
    DiffuseMapVar->SetResource( CubeDiffuseMap );                 // Send the cube's diffuse/specular map to the shader
	Cube->Render( WiggleTechnique );                         // Pass rendering technique to the model class

	// Box
	WorldMatrixVar->SetMatrix((float*)Box->GetWorldMatrix());
	DiffuseMapVar->SetResource(BoxDiffuseMap);
	NormalMapVar->SetResource(BoxNormalMap);
	Box->Render( NormalMappingTechnique );

	WorldMatrixVar->SetMatrix((float*)Floor->GetWorldMatrix());
	DiffuseMapVar->SetResource(FloorDiffuseMap);
	Floor->Render(VertexLitTechnique);

	WorldMatrixVar->SetMatrix( (float*)Light1->GetWorldMatrix() );
	DiffuseMapVar->SetResource(LightDiffuseMap);
	TintColourVar->SetRawValue(Light1->GetColour(), 0, 12);
	Light1->Render( AdditiveTexTintTechnique );

	WorldMatrixVar->SetMatrix( (float*)Light2->GetWorldMatrix() );
	DiffuseMapVar->SetResource(LightDiffuseMap);
	TintColourVar->SetRawValue(Light2->GetColour(), 0, 12);
	Light2->Render( AdditiveTexTintTechnique );

	//---------------------------
	// Teapot with 3 lights
	WorldMatrixVar->SetMatrix((float*)Teapot->GetWorldMatrix());
	DiffuseMapVar->SetResource(StoneDiffuseMap);
	Teapot->Render(VertexLitTechnique);

	for (int i = 0; i < g_numLights; i++) {
		WorldMatrixVar->SetMatrix((float*)TeapotLights[i]->GetWorldMatrix());
		DiffuseMapVar->SetResource(LightDiffuseMap);
		TintColourVar->SetRawValue(TeapotLights[i]->GetColour(), 0, 12);
		TeapotLights[i]->Render(AdditiveTexTintTechnique);
	}

	//---------------------------
	// Display the Scene

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present( 0, 0 );
}
