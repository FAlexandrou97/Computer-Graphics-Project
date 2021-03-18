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
const int g_numTeapotLights = 3;
const int g_numSpotLights = 2;
float g_parallaxDepth = 0.05f; // Overall depth of bumpiness for parallax mapping
bool g_useParallax = false;  // Toggle for parallax 
float g_clearColour[4] = { 0.2f, 0.2f, 0.3f, 1.0f }; // Good idea to match background to ambient colour

// Models and cameras encapsulated in classes for flexibity and convenience
// The CModel class collects together geometry and world matrix, and provides functions to control the model and render it
// The CCamera class handles the view and projections matrice, and provides functions to control the camera
CModel* WiggleCube;
CModel* Box;
CModel* Floor;
CModel* Teapot;
CModel* Troll;
CLight* Sphere;
CModel* Car;
CCamera* Camera;

//**** Portal Data ****//
// Dimensions of portal texture - controls quality of rendered scene in portal
int PortalWidth = 1024;
int PortalHeight = 1024;

CModel* Portal;       // The model on which the portal appears
CCamera* PortalCamera; // The camera view shown in the portal

// The portal texture and the view of it as a render target (see code comments)
ID3D10Texture2D* PortalTexture = NULL;
ID3D10RenderTargetView* PortalRenderTarget = NULL;
ID3D10ShaderResourceView* PortalMap = NULL;

// Also need a depth/stencil buffer for the portal
// NOTE: ***Can share the depth buffer between multiple portals of the same size***
ID3D10Texture2D* PortalDepthStencil = NULL;
ID3D10DepthStencilView* PortalDepthStencilView = NULL;

//*********************//


//**** Shadow Maps ****//
// Very similar data to the render-to-texture (portal) lab

// Width and height of shadow map - controls resolution/quality of shadows
int ShadowMapSize = 1024;

// The shadow map textures and the view of it as a depth buffer and shader resource (see code comments)
ID3D10Texture2D* ShadowMap1Texture = NULL;
ID3D10DepthStencilView* ShadowMap1DepthView = NULL;
ID3D10ShaderResourceView* ShadowMap1 = NULL;
ID3D10Texture2D* ShadowMap2Texture = NULL;
ID3D10DepthStencilView* ShadowMap2DepthView = NULL;
ID3D10ShaderResourceView* ShadowMap2 = NULL;

//*********************//

// Textures - no texture class yet so using DirectX variables
ID3D10ShaderResourceView* CubeDiffuseMap = NULL;
ID3D10ShaderResourceView* LightDiffuseMap = NULL;
ID3D10ShaderResourceView* BoxDiffuseMap = NULL;
ID3D10ShaderResourceView* BoxNormalMap = NULL;
ID3D10ShaderResourceView* FloorDiffuseMap = NULL;
ID3D10ShaderResourceView* FloorNormalMap = NULL;
ID3D10ShaderResourceView* StoneDiffuseMap = NULL;
ID3D10ShaderResourceView* TrollDiffuseMap = NULL;
ID3D10ShaderResourceView* CarDiffuseMap = NULL;
ID3D10ShaderResourceView* CellMap = NULL;

// Light data - stored manually as there is no light class
D3DXVECTOR3 AmbientColour = D3DXVECTOR3( 0.2f, 0.2f, 0.2f );
float SpecularPower = 256.0f;
// Display models where the lights are. One of the lights will follow an orbit
CLight* CubeLight;
CLight* TeapotLights[g_numTeapotLights];
CLight* SpotLights[g_numSpotLights];
CLight* CarLight;
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
ID3D10EffectTechnique* ParallaxMappingTechnique = NULL;
ID3D10EffectTechnique* ShadowMappingTechnique = NULL;
ID3D10EffectTechnique* DepthOnlyTechnique = NULL;
ID3D10EffectTechnique* CellShadingTechnique = NULL;

// Matrices
ID3D10EffectMatrixVariable* WorldMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* ProjMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewProjMatrixVar = NULL;
ID3D10EffectScalarVariable* g_pCubeWiggleVar = NULL;

// Textures
ID3D10EffectShaderResourceVariable* DiffuseMapVar = NULL;
ID3D10EffectShaderResourceVariable* NormalMapVar = NULL;
ID3D10EffectShaderResourceVariable* ShadowMap1Var = NULL;
ID3D10EffectShaderResourceVariable* ShadowMap2Var = NULL; 
ID3D10EffectShaderResourceVariable* CellMapVar = NULL;


// Miscellaneous
ID3D10EffectVectorVariable* ModelColourVar = NULL;
ID3D10EffectScalarVariable* ParallaxDepthVar = NULL;

// Light Effect Variables
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

	delete CubeLight;
	delete Floor;
	delete WiggleCube;
	delete Box;
	delete Camera;
	delete Teapot;
	delete Portal;
	delete PortalCamera;
	delete Troll;
	delete Sphere;
	delete Car;
	delete CarLight;
	for (int i = 0; i < g_numTeapotLights; i++) {
		delete TeapotLights[i];
	}
	for (int i = 0; i < g_numSpotLights; i++) {
		delete SpotLights[i];
	}

    if( FloorDiffuseMap )  FloorDiffuseMap->Release();
    if( CubeDiffuseMap )   CubeDiffuseMap->Release();
	if( LightDiffuseMap )  LightDiffuseMap->Release();
    if( BoxDiffuseMap )    BoxDiffuseMap->Release();
    if( BoxNormalMap )     BoxNormalMap->Release();
    if( StoneDiffuseMap )  StoneDiffuseMap->Release();
	if (TrollDiffuseMap)   TrollDiffuseMap->Release();
	if (CarDiffuseMap)	   CarDiffuseMap->Release();
	if( Effect )           Effect->Release();
	if( DepthStencilView ) DepthStencilView->Release();
	if( RenderTargetView ) RenderTargetView->Release();
	if( DepthStencil )     DepthStencil->Release();
	if( SwapChain )        SwapChain->Release();
	if( g_pd3dDevice )     g_pd3dDevice->Release();
	if (PortalDepthStencilView) PortalDepthStencilView->Release();
	if (PortalDepthStencil)     PortalDepthStencil->Release();
	if (PortalMap)              PortalMap->Release();
	if (PortalRenderTarget)     PortalRenderTarget->Release();
	if (PortalTexture)          PortalTexture->Release();
	if (ShadowMap1)             ShadowMap1->Release();
	if (ShadowMap1DepthView)    ShadowMap1DepthView->Release();
	if (ShadowMap1Texture)      ShadowMap1Texture->Release();
	if (ShadowMap2)             ShadowMap2->Release();
	if (ShadowMap2DepthView)    ShadowMap2DepthView->Release();
	if (ShadowMap2Texture)      ShadowMap2Texture->Release();
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
	ParallaxMappingTechnique = Effect->GetTechniqueByName("ParallaxMappingTechnique");
	ShadowMappingTechnique = Effect->GetTechniqueByName("ShadowMappingTechnique");
	DepthOnlyTechnique = Effect->GetTechniqueByName("DepthOnlyTechnique");
	CellShadingTechnique = Effect->GetTechniqueByName("CellShadingTechnique");

	// Create special variables to allow us to access global variables in the shaders from C++
	WorldMatrixVar    = Effect->GetVariableByName( "WorldMatrix" )->AsMatrix();
	ViewMatrixVar     = Effect->GetVariableByName( "ViewMatrix"  )->AsMatrix();
	ProjMatrixVar     = Effect->GetVariableByName( "ProjMatrix"  )->AsMatrix();

	// We access the texture variable in the shader in the same way as we have before for matrices, light data etc.
	// Only difference is that this variable is a "Shader Resource"
	DiffuseMapVar = Effect->GetVariableByName( "DiffuseMap" )->AsShaderResource();
	NormalMapVar  = Effect->GetVariableByName("NormalMap")->AsShaderResource();
	ShadowMap1Var = Effect->GetVariableByName("ShadowMap1")->AsShaderResource();
	ShadowMap2Var = Effect->GetVariableByName("ShadowMap2")->AsShaderResource();
	CellMapVar = Effect->GetVariableByName("CellMap")->AsShaderResource();

	// Other shader variables
	ModelColourVar = Effect->GetVariableByName( "ModelColour"  )->AsVector();
	g_pCubeWiggleVar = Effect->GetVariableByName("Wiggle")->AsScalar();

	// Access shader variables needed for lighting
	
	// Initialize light objects
	CubeLight = new CLight;
	CarLight = new CLight;
	for (int i = 0; i < g_numTeapotLights; i++) {
		TeapotLights[i] = new CLight;
	}
	for (int i = 0; i < g_numSpotLights; i++) {
		SpotLights[i] = new CLight;
	}
	CubeLight->SetColourVar(Effect->GetVariableByName("CubeLightColour")->AsVector());
	CubeLight->SetPosVar(Effect->GetVariableByName("CubeLightPos")->AsVector());

	CarLight->SetColourVar(Effect->GetVariableByName("CarLightColour")->AsVector());
	CarLight->SetPosVar(Effect->GetVariableByName("CarLightPos")->AsVector());

	SpotLights[0]->SetColourVar(Effect->GetVariableByName("SpotLight1Colour")->AsVector());
	SpotLights[0]->SetPosVar(Effect->GetVariableByName("SpotLight1Pos")->AsVector());
	SpotLights[0]->SetFacingVar(Effect->GetVariableByName("SpotLight1Facing")->AsVector());
	SpotLights[0]->SetViewMatrixVar(Effect->GetVariableByName("SpotLight1ViewMatrix")->AsMatrix());
	SpotLights[0]->SetProjMatrixVar(Effect->GetVariableByName("SpotLight1ProjMatrix")->AsMatrix());
	SpotLights[0]->SetConeAngleVar(Effect->GetVariableByName("SpotLight1CosHalfAngle")->AsScalar());
	SpotLights[1]->SetColourVar(Effect->GetVariableByName("SpotLight2Colour")->AsVector());
	SpotLights[1]->SetPosVar(Effect->GetVariableByName("SpotLight2Pos")->AsVector());
	SpotLights[1]->SetFacingVar(Effect->GetVariableByName("SpotLight2Facing")->AsVector());
	SpotLights[1]->SetViewMatrixVar(Effect->GetVariableByName("SpotLight2ViewMatrix")->AsMatrix());
	SpotLights[1]->SetProjMatrixVar(Effect->GetVariableByName("SpotLight2ProjMatrix")->AsMatrix());
	SpotLights[1]->SetConeAngleVar(Effect->GetVariableByName("SpotLight2CosHalfAngle")->AsScalar());

	TeapotLights[0]->SetColourVar(Effect->GetVariableByName("TeapotLight1Colour")->AsVector());
	TeapotLights[0]->SetPosVar(Effect->GetVariableByName("TeapotLight1Pos")->AsVector());
	TeapotLights[1]->SetColourVar(Effect->GetVariableByName("TeapotLight2Colour")->AsVector());
	TeapotLights[1]->SetPosVar(Effect->GetVariableByName("TeapotLight2Pos")->AsVector());
	TeapotLights[2]->SetColourVar(Effect->GetVariableByName("TeapotLight3Colour")->AsVector());
	TeapotLights[2]->SetPosVar(Effect->GetVariableByName("TeapotLight3Pos")->AsVector());

	TintColourVar = Effect->GetVariableByName("TintColour")->AsVector();
	AmbientColourVar = Effect->GetVariableByName("AmbientColour")->AsVector();
	SpecularPowerVar = Effect->GetVariableByName("SpecularPower")->AsScalar();
	ParallaxDepthVar = Effect->GetVariableByName("ParallaxDepth")->AsScalar();

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

	//**** Portal camera is the view shown in the portal object's texture ****//
	PortalCamera = new CCamera();
	PortalCamera->SetPosition(D3DXVECTOR3(50, 15, 100));
	PortalCamera->SetRotation(D3DXVECTOR3(0, ToRadians(-130.0f), 0.));

	///////////////////////
	// Load/Create models

	WiggleCube = new CModel;
	Box = new CModel;
	Floor = new CModel;
	Teapot = new CModel;
	Portal = new CModel;
	Troll = new CModel;
	Sphere = new CLight;
	Car = new CModel;

	// The model class can load ".X" files. It encapsulates (i.e. hides away from this code) the file loading/parsing and creation of vertex/index buffers
	// We must pass an example technique used for each model. We can then only render models with techniques that uses matching vertex input data
	if (!WiggleCube->  Load( "Cube.x", WiggleTechnique)) return false;
	if (!Box->  Load( "CardboardBox.x", ParallaxMappingTechnique, true)) return false;
	if (!Floor-> Load( "Floor.x", ParallaxMappingTechnique, true )) return false;
	if (!CubeLight->Load( "Light.x", AdditiveTexTintTechnique )) return false;
	if (!Teapot->Load( "Teapot.x", VertexLitTechnique )) return false;
	for (int i = 0; i < g_numTeapotLights; i++) {
		if (!TeapotLights[i]->Load("Light.x", AdditiveTexTintTechnique)) return false;
	}
	for (int i = 0; i < g_numSpotLights; i++) {
		if (!SpotLights[i]->Load("Light.x", AdditiveTexTintTechnique)) return false;
	}
	if (!Portal->Load("Portal.x", AdditiveTexTintTechnique)) return false;
	if (!Troll->Load("Troll.x", ShadowMappingTechnique)) return false;
	if (!Sphere->Load("Sphere.x", PlainColourTechnique)) return false;
	if (!Car->Load("AstonMartin.x", CellShadingTechnique)) return false;
	if (!CarLight->Load("Light.x", AdditiveTexTintTechnique)) return false;

	// Initial positions
	WiggleCube->SetPosition( D3DXVECTOR3(-20, 5, 0) );
	Box->SetPosition( D3DXVECTOR3(0, 0, 30) );
	Box->SetScale( 8.0f );
	CubeLight->SetPosition( D3DXVECTOR3(30, 10, 0) );
	CubeLight->SetScale( 4.0f );
	CubeLight->SetColour(D3DXVECTOR3(1.0f, 2.0f, 0.7f) * 5);

	Troll->SetPosition(D3DXVECTOR3(-40, 0, 40));
	Troll->SetScale(5.0f);
	Troll->SetRotation(D3DXVECTOR3(0.0f, ToRadians(215.0f), 0.0f));

	SpotLights[0]->SetPosition(D3DXVECTOR3(-60, 10, 50));
	SpotLights[0]->SetScale(4.0f);
	SpotLights[0]->SetColour(D3DXVECTOR3(0.8f, 0.8f, 0.8f) * 40);
	SpotLights[0]->FacePoint(Troll->GetPosition());
	SpotLights[1]->SetPosition(D3DXVECTOR3(-20, 30, 130));
	SpotLights[1]->SetScale(4.0f);
	SpotLights[1]->SetColour(D3DXVECTOR3(1.0f, 0.8f, 0.2f) * 80);
	SpotLights[1]->FacePoint(D3DXVECTOR3(0, 0, 0));

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

	Portal->SetPosition(D3DXVECTOR3(50, 15, 100));
	Portal->SetRotation(D3DXVECTOR3(0.0f, ToRadians(-130.0f), 0.0f));

	Sphere->SetPosition(D3DXVECTOR3(20, 20, 30));
	Sphere->SetColour(D3DXVECTOR3(0.3f, 0.3f, 1.0f));
	Sphere->SetScale(0.2f);

	Car->SetPosition(D3DXVECTOR3(45, 0, 15));
	Car->SetScale(5.0f);
	CarLight->SetPosition(D3DXVECTOR3(45, 0, 15));
	CarLight->SetScale(4.0f);
	CarLight->SetColour(D3DXVECTOR3(1.0f, 1.0f, 0.2f) * 5);

	//////////////////
	// Load textures
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"StoneDiffuseSpecular.dds", NULL, NULL, &CubeDiffuseMap,  NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"CobbleDiffuseSpecular.dds",  NULL, NULL, &FloorDiffuseMap, NULL ) )) return false;
	if (FAILED( D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, L"CobbleNormalDepth.dds",      NULL, NULL, &FloorNormalMap,   NULL ) )) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"StoneDiffuseSpecular.dds", NULL, NULL, &StoneDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"PatternNormal.dds", NULL, NULL, &BoxNormalMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"PatternDiffuseSpecular.dds", NULL, NULL, &BoxDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"flare.jpg", NULL, NULL, &LightDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"TrollDiffuseSpecular.dds", NULL, NULL, &TrollDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"Red.png", NULL, NULL, &CarDiffuseMap, NULL))) return false;
	if (FAILED(D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, L"CellGradient.png", NULL, NULL, &CellMap, NULL))) return false;


	//**** Portal Texture ****//

	// Create the portal texture itself, above we used a D3DX... helper function to create a texture in one line. Here, we need to do things manually
	// as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D10_TEXTURE2D_DESC portalDesc;
	portalDesc.Width = PortalWidth;  // Size of the portal texture determines its quality
	portalDesc.Height = PortalHeight;
	portalDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
	portalDesc.ArraySize = 1;
	portalDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
	portalDesc.SampleDesc.Count = 1;
	portalDesc.SampleDesc.Quality = 0;
	portalDesc.Usage = D3D10_USAGE_DEFAULT;
	portalDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as render target, and pass it to shaders
	portalDesc.CPUAccessFlags = 0;
	portalDesc.MiscFlags = 0;
	if (FAILED(g_pd3dDevice->CreateTexture2D(&portalDesc, NULL, &PortalTexture))) return false;

	// We created the portal texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
	// we use when rendering to it (see RenderScene function below)
	if (FAILED(g_pd3dDevice->CreateRenderTargetView(PortalTexture, NULL, &PortalRenderTarget))) return false;

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	D3D10_SHADER_RESOURCE_VIEW_DESC srDesc;
	srDesc.Format = portalDesc.Format;
	srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;

	if (FAILED(g_pd3dDevice->CreateShaderResourceView(PortalTexture, &srDesc, &PortalMap))) return false;

	//**** Portal Depth Buffer ****//

	// We also need a depth buffer to go with our portal
	//**** This depth buffer can be shared with any other portals of the same size
	portalDesc.Width = PortalWidth;
	portalDesc.Height = PortalHeight;
	portalDesc.MipLevels = 1;
	portalDesc.ArraySize = 1;
	portalDesc.Format = DXGI_FORMAT_D32_FLOAT;
	portalDesc.SampleDesc.Count = 1;
	portalDesc.SampleDesc.Quality = 0;
	portalDesc.Usage = D3D10_USAGE_DEFAULT;
	portalDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	portalDesc.CPUAccessFlags = 0;
	portalDesc.MiscFlags = 0;
	if (FAILED(g_pd3dDevice->CreateTexture2D(&portalDesc, NULL, &PortalDepthStencil))) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC portalDescDSV;
	portalDescDSV.Format = portalDesc.Format;
	portalDescDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	portalDescDSV.Texture2D.MipSlice = 0;
	if (FAILED(g_pd3dDevice->CreateDepthStencilView(PortalDepthStencil, &portalDescDSV, &PortalDepthStencilView))) return false;

	////////////////////////
	//**** Shadow Maps ****//

	// Create the shadow map textures, above we used a D3DX... helper function to create basic textures in one line. Here, we need to
	// do things manually as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D10_TEXTURE2D_DESC texDesc;
	texDesc.Width = ShadowMapSize; // Size of the shadow map determines quality / resolution of shadows
	texDesc.Height = ShadowMapSize;
	texDesc.MipLevels = 1; // 1 level, means just the main texture, no additional mip-maps. Usually don't use mip-maps when rendering to textures (or we would have to render every level)
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // The shadow map contains a single 32-bit value [tech gotcha: have to say typeless because depth buffer and texture see things slightly differently]
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D10_USAGE_DEFAULT;
	texDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE; // Indicate we will use texture as render target, and will also pass it to shaders
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	if (FAILED(g_pd3dDevice->CreateTexture2D(&texDesc, NULL, &ShadowMap1Texture))) return false;
	if (FAILED(g_pd3dDevice->CreateTexture2D(&texDesc, NULL, &ShadowMap2Texture))) return false;

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = DXGI_FORMAT_D32_FLOAT; // See "tech gotcha" above
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	if (FAILED(g_pd3dDevice->CreateDepthStencilView(ShadowMap1Texture, &descDSV, &ShadowMap1DepthView))) return false;
	if (FAILED(g_pd3dDevice->CreateDepthStencilView(ShadowMap2Texture, &descDSV, &ShadowMap2DepthView))) return false;

	// We also need to send this texture (a GPU memory resource) to the shaders. To do that we must create a shader-resource "view"	
	srDesc.Format = DXGI_FORMAT_R32_FLOAT; // See "tech gotcha" above
	srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	if (FAILED(g_pd3dDevice->CreateShaderResourceView(ShadowMap1Texture, &srDesc, &ShadowMap1))) return false;
	if (FAILED(g_pd3dDevice->CreateShaderResourceView(ShadowMap2Texture, &srDesc, &ShadowMap2))) return false;

	//*****************************//

	return true;
}


// Update the scene - move/rotate each model and the camera, then update their matrices
void UpdateScene( float frameTime )
{
	// Control camera position and update its matrices (view matrix, projection matrix) each frame
	// Don't be deceived into thinking that this is a new method to control models - the same code we used previously is in the camera class
	Camera->Control( frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );
	Camera->UpdateMatrices();
	
	PortalCamera->Control(frameTime, Key_Numpad5, Key_Numpad0, Key_Numpad1, Key_Numpad3, Key_U, Key_O, Key_Period, Key_Comma);
	PortalCamera->UpdateMatrices();
	Portal->UpdateMatrix();

	// Control cube position and update its world matrix each frame
	WiggleCube->Control( frameTime, Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );
	WiggleCube->UpdateMatrix();

	Box->UpdateMatrix();

	// Wiggle effect
	g_WiggleVar += 6 * frameTime;
	g_pCubeWiggleVar->SetFloat(g_WiggleVar);

	// Update the orbiting lights - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float CubeLightRotate = 0.0f;
	CubeLight->SetPosition( Box->GetPosition() + D3DXVECTOR3(cos(CubeLightRotate)*CubeLight->GetOrbitRadius(), 5, sin(CubeLightRotate)*CubeLight->GetOrbitRadius()) );
	CubeLightRotate -= CubeLight->GetOrbitSpeed() * frameTime;
	CubeLight->UpdateMatrix();

	static float CarLightRotate = 0.0f;
	CarLight->SetPosition(Car->GetPosition() + D3DXVECTOR3(cos(CarLightRotate) * CarLight->GetOrbitRadius(), 5, sin(CarLightRotate) * CarLight->GetOrbitRadius()));
	CarLightRotate -= CarLight->GetOrbitSpeed() * frameTime;
	CarLight->UpdateMatrix();

	// Update teapot lights
	const float colourUpdateSpeed = frameTime * 5;

	TeapotLights[0]->Pulsate(D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(5.0f, 0.0f, 3.5f), colourUpdateSpeed);
	TeapotLights[1]->ChangeColour(colourUpdateSpeed);
	TeapotLights[2]->ChangeColour(colourUpdateSpeed * 2);

	Teapot->UpdateMatrix();
	for (int i = 0; i < g_numTeapotLights; i++) {
		TeapotLights[i]->UpdateMatrix();
	}

	// Update spot lights
	static float SpotLightRotate = 0.0f;
	SpotLights[0]->SetPosition(Troll->GetPosition() + D3DXVECTOR3(cos(SpotLightRotate) * SpotLights[0]->GetOrbitRadius(), 20, sin(SpotLightRotate) * SpotLights[0]->GetOrbitRadius()));
	SpotLightRotate -= SpotLights[0]->GetOrbitSpeed() * frameTime;
	SpotLights[0]->FacePoint(Troll->GetPosition()); // The troll is in the spotlight...

	for (int i = 0; i < g_numSpotLights; i++) {
		SpotLights[i]->UpdateMatrix();
	}

	Troll->UpdateMatrix();
	Sphere->UpdateMatrix();
	Car->UpdateMatrix();

	if (KeyHit(Key_1))
	{
		g_useParallax = !g_useParallax;
	}
}

// Render all the models from the point of view of the given camera
void RenderModels(CCamera* camera)
{
	// Clear the back buffer - before drawing the geometry clear the entire window to a fixed colour
	g_pd3dDevice->ClearRenderTargetView(RenderTargetView, g_clearColour);
	g_pd3dDevice->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0); // Clear the depth buffer too


	// Pass the camera's matrices to the vertex shader
	ViewMatrixVar->SetMatrix((float*)&camera->GetViewMatrix());
	ProjMatrixVar->SetMatrix((float*)&camera->GetProjectionMatrix());

	// Send the shadow maps rendered in the function below to the shader
	ShadowMap1Var->SetResource(ShadowMap1);
	ShadowMap2Var->SetResource(ShadowMap2);


	//---------------------------
	// Render each model

	// Constant colours used for models in initial shaders
	D3DXVECTOR3 Black(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 Blue(0.0f, 0.0f, 1.0f);

	// Portal
	WorldMatrixVar->SetMatrix((float*)Portal->GetWorldMatrix());
	DiffuseMapVar->SetResource(PortalMap);
	Portal->Render(VertexLitTechnique);

	// WiggleCube
	WorldMatrixVar->SetMatrix((float*)WiggleCube->GetWorldMatrix());  // Send the cube's world matrix to the shader
	DiffuseMapVar->SetResource(CubeDiffuseMap);                 // Send the cube's diffuse/specular map to the shader
	WiggleCube->Render(WiggleTechnique);                         // Pass rendering technique to the model class

	// Box
	WorldMatrixVar->SetMatrix((float*)Box->GetWorldMatrix());
	DiffuseMapVar->SetResource(BoxDiffuseMap);
	NormalMapVar->SetResource(BoxNormalMap);
	Box->Render(ParallaxMappingTechnique);

	// Floor
	WorldMatrixVar->SetMatrix((float*)Floor->GetWorldMatrix());
	DiffuseMapVar->SetResource(FloorDiffuseMap);
	NormalMapVar->SetResource(FloorNormalMap);
	Floor->Render(ParallaxMappingTechnique);

	// Teapot
	WorldMatrixVar->SetMatrix((float*)Teapot->GetWorldMatrix());
	DiffuseMapVar->SetResource(StoneDiffuseMap);
	Teapot->Render(VertexLitTechnique);

	// Troll
	WorldMatrixVar->SetMatrix(Troll->GetWorldMatrix());
	DiffuseMapVar->SetResource(TrollDiffuseMap);
	Troll->Render(ShadowMappingTechnique);

	// Shere
	WorldMatrixVar->SetMatrix(Sphere->GetWorldMatrix());
	ModelColourVar->SetRawValue(Blue, 0, 12);
	Sphere->Render(PlainColourTechnique);

	// Car
	WorldMatrixVar->SetMatrix(Car->GetWorldMatrix());
	DiffuseMapVar->SetResource(CarDiffuseMap);
	ModelColourVar->SetRawValue(Black, 0, 12);
	Car->Render(CellShadingTechnique);

	// CubeLight
	WorldMatrixVar->SetMatrix((float*)CubeLight->GetWorldMatrix());
	DiffuseMapVar->SetResource(LightDiffuseMap);
	TintColourVar->SetRawValue(CubeLight->GetColour(), 0, 12);
	CubeLight->Render(AdditiveTexTintTechnique);

	// CarLight
	WorldMatrixVar->SetMatrix((float*)CarLight->GetWorldMatrix());
	DiffuseMapVar->SetResource(LightDiffuseMap);
	TintColourVar->SetRawValue(CarLight->GetColour(), 0, 12);
	CarLight->Render(AdditiveTexTintTechnique);

	// Teapot Lights (3)
	for (int i = 0; i < g_numTeapotLights; i++) {
		WorldMatrixVar->SetMatrix((float*)TeapotLights[i]->GetWorldMatrix());
		DiffuseMapVar->SetResource(LightDiffuseMap);
		TintColourVar->SetRawValue(TeapotLights[i]->GetColour(), 0, 12);
		TeapotLights[i]->Render(AdditiveTexTintTechnique);
	}

	// Spot Lights (2)
	for (int i = 0; i < g_numSpotLights; i++) {
		WorldMatrixVar->SetMatrix((float*)SpotLights[i]->GetWorldMatrix());
		DiffuseMapVar->SetResource(LightDiffuseMap);
		TintColourVar->SetRawValue(SpotLights[i]->GetColour(), 0, 12);
		SpotLights[i]->Render(AdditiveTexTintTechnique);
	}
}


void RenderShadowMap(CLight* light)
{
	//---------------------------------
	// Set "camera" matrices in shader

	// Pass the light's "camera" matrices to the vertex shader - use helper functions above to turn spotlight settings into "camera" matrices
	ViewMatrixVar->SetMatrix(light->CalculateLightViewMatrix());
	ProjMatrixVar->SetMatrix(light->CalculateLightProjMatrix());


	//-----------------------------------
	// Render each model into shadow map

	// Render troll - no need to set its texture as shadow maps just render to the depth buffer
	WorldMatrixVar->SetMatrix(Troll->GetWorldMatrix());
	Troll->Render(DepthOnlyTechnique);  // Use special rendering technique to render depths only

	// Same for the other models in the scene
	WorldMatrixVar->SetMatrix(WiggleCube->GetWorldMatrix());
	WiggleCube->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix(Floor->GetWorldMatrix());
	Floor->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix(Teapot->GetWorldMatrix());
	Teapot->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix(Box->GetWorldMatrix());
	Box->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix(Sphere->GetWorldMatrix());
	Sphere->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix(Car->GetWorldMatrix());
	Car->Render(DepthOnlyTechnique);

	WorldMatrixVar->SetMatrix(Portal->GetWorldMatrix());
	Portal->Render(DepthOnlyTechnique);
}

// Render everything in the scene
void RenderScene()
{
	// Pass light information to the vertex shader
	CubeLight->GetPosVar()->SetRawValue(CubeLight->GetPosition(), 0, 12);
	CubeLight->GetColourVar()->SetRawValue(CubeLight->GetColour(), 0, 12);
	
	CarLight->GetPosVar()->SetRawValue(CarLight->GetPosition(), 0, 12);
	CarLight->GetColourVar()->SetRawValue(CarLight->GetColour(), 0, 12);

	for (int i = 0; i < g_numTeapotLights; i++) {
		TeapotLights[i]->GetPosVar()->SetRawValue(TeapotLights[i]->GetPosition(), 0, 12);
		TeapotLights[i]->GetColourVar()->SetRawValue(TeapotLights[i]->GetColour(), 0, 12);
	}
	
	for (int i = 0; i < g_numSpotLights; i++) {
		SpotLights[i]->GetPosVar()->SetRawValue(SpotLights[i]->GetPosition(), 0, 12);
		SpotLights[i]->GetColourVar()->SetRawValue(SpotLights[i]->GetColour(), 0, 12);
		SpotLights[i]->GetFacingVar()->SetRawValue(SpotLights[i]->GetFacing(), 0, 12);
		SpotLights[i]->GetViewMatrixVar()->SetMatrix(SpotLights[i]->CalculateLightViewMatrix());
		SpotLights[i]->GetProjMatrixVar()->SetMatrix(SpotLights[i]->CalculateLightProjMatrix());
		SpotLights[i]->GetConeAngleVar()->SetFloat(cos(ToRadians(SpotLights[i]->GetConeAngle() * 0.5f)));
	}

	AmbientColourVar->SetRawValue(AmbientColour, 0, 12);
	SpecularPowerVar->SetFloat(SpecularPower);
	// Parallax mapping depth
	ParallaxDepthVar->SetFloat(g_useParallax ? g_parallaxDepth : 0.0f);
	CellMapVar->SetResource(CellMap);

	//---------------------------
	// Render portal scene

	// Setup the viewport - defines which part of the texture we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width = PortalWidth;
	vp.Height = PortalHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports(1, &vp);

	// Select the portal texture to use for rendering, will share the depth/stencil buffer with the backbuffer though
	g_pd3dDevice->OMSetRenderTargets(1, &PortalRenderTarget, PortalDepthStencilView);

	// Clear the back buffer - before drawing the geometry clear the entire window to a fixed colour
	g_pd3dDevice->ClearRenderTargetView(PortalRenderTarget, g_clearColour);
	g_pd3dDevice->ClearDepthStencilView(PortalDepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0); // Clear the depth buffer too

	// Render everything from the portal camera's point of view (into the portal render target [texture] set above)
	RenderModels(PortalCamera);


	//---------------------------
	// Render shadow maps

	// Setup the viewport - defines which part of the shadow map we will render to (usually all of it)
	vp.Width = ShadowMapSize;
	vp.Height = ShadowMapSize;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports(1, &vp);

	// Rendering a single shadow map for a light
	// 1. Select the shadow map texture as the current depth buffer. We will not be rendering any pixel colours
	// 2. Clear the shadow map texture (as a depth buffer)
	// 3. Render everything from point of view of light 0
	g_pd3dDevice->OMSetRenderTargets(0, 0, ShadowMap1DepthView);
	g_pd3dDevice->ClearDepthStencilView(ShadowMap1DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0);
	RenderShadowMap(SpotLights[0]);

	g_pd3dDevice->OMSetRenderTargets(0, 0, ShadowMap2DepthView);
	g_pd3dDevice->ClearDepthStencilView(ShadowMap2DepthView, D3D10_CLEAR_DEPTH, 1.0f, 0);
	RenderShadowMap(SpotLights[1]);


	//---------------------------
	// Render main scene

	// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	vp.Width = g_ViewportWidth;
	vp.Height = g_ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports(1, &vp);

	// Select the back buffer and depth buffer to use for rendering
	g_pd3dDevice->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);

	// Clear the back buffer  and its depth buffer
	g_pd3dDevice->ClearRenderTargetView(RenderTargetView, g_clearColour);
	g_pd3dDevice->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);

	// Render everything from the main camera's point of view (into the portal render target [texture] set above)
	RenderModels(Camera);

	//---------------------------
	// Display the Scene

	// After we've finished drawing to the off-screen back buffer, we "present" it to the front buffer (the screen)
	SwapChain->Present(0, 0);
}
