#include "ModelHierarchy.h"
#include "MathDX.h"
#include "Defines.h"

using namespace gen;

CModelHierarchy* CModelHierarchy::CreateNewChild()
{
	CModelHierarchy* newChild = new CModelHierarchy;
	m_Children[m_NumChildren] = newChild;
	++m_NumChildren;
	return newChild;
}

// Release resources used by model
void CModelHierarchy::ReleaseResources()
{
	// Release resources
	for (int i = 0; i < m_NumChildren; i++)
	{
		delete m_Children[i];
	}
	m_NumChildren = 0;

	CModel::ReleaseResources();
}

bool CModelHierarchy::Load(const string& fileName, ID3D10EffectTechnique* exampleTechnique, bool tangents)
{
	// Use CImportXFile class (from another application) to load the given file. The import code is wrapped in the namespace 'gen'
	gen::CImportXFile mesh;
	if (mesh.ImportFile(fileName.c_str()) != gen::kSuccess)
	{
		return false;
	}

	// If only one sub-mesh create a non-hierarchical model
	if (mesh.GetNumSubMeshes() == 1)
	{
		// Get first sub-mesh from loaded file
		gen::SSubMesh subMesh;
		if (mesh.GetSubMesh(0, &subMesh, tangents) != gen::kSuccess)
		{
			return false;
		}
		if (!CreateFromSubMesh(&subMesh, exampleTechnique))
		{
			ReleaseResources();
			return false;
		}
	}
	else
	{
		//*************************************
		// Model Hierarchy
		//*************************************
		// Multi-part model - create a hierarchy of models with this as the root

		// Array of model ptrs for each node - in the imported data the first node defines
		// the parent of the model root so is set to null (root has no parent)
		CModelHierarchy** nodeModels = new CModelHierarchy * [mesh.GetNumNodes()];
		nodeModels[0] = 0;

		// Walk through nodes (ignoring first one - root's parent) and meshes. Create a
		// model for every node, but only create geometry for a node if it is referenced
		// by a mesh. Result is a correct hierarchy of models, but with some models having
		// no geometry in them. If a node has multiple sub-meshes, only one is used
		unsigned int currMesh = 0; // First mesh
		for (unsigned int node = 1; node < mesh.GetNumNodes(); ++node) // For each node...
		{
			gen::SMeshNode meshNode;
			mesh.GetNode(node, &meshNode);

			// Create a new model for all except the first node (the root - this object - already created)
			CModelHierarchy* pNodeModel;
			if (node == 1)
			{
				pNodeModel = this;
			}
			else
			{
				// Attach new model to (already created) parent model
				CModelHierarchy* pParent = nodeModels[meshNode.parent];
				pNodeModel = pParent->CreateNewChild();
			}
			nodeModels[node] = pNodeModel;

			// Get initial position and rotation for model from node matrix
			// Using helper maths classes provided with import code
			gen::CVector3 position, rotation, scale;
			meshNode.positionMatrix.DecomposeAffineEuler(&position, &rotation, &scale);
			pNodeModel->SetPosition(ToD3DXVECTOR(position));
			pNodeModel->SetRotation(ToD3DXVECTOR(rotation));
			pNodeModel->SetScale(ToD3DXVECTOR(scale));

			// If this node is refered by next sub-mesh, then it has geometry
			if (currMesh < mesh.GetNumSubMeshes())
			{
				gen::SSubMesh subMesh;
				if (mesh.GetSubMesh(currMesh, &subMesh) != gen::kSuccess)
				{
					ReleaseResources();
					delete[] nodeModels;
					return false;
				}
				if (subMesh.node == node)
				{
					// Create the geometry for this node
					if (!pNodeModel->CreateFromSubMesh(&subMesh, exampleTechnique))
					{
						ReleaseResources();
						delete[] nodeModels;
						return false;
					}

					// Skip over any additional sub-meshes using this node - not allowing multi-material nodes
					++currMesh;
					while (currMesh < mesh.GetNumSubMeshes())
					{
						gen::SSubMesh subMesh;
						if (mesh.GetSubMesh(currMesh, &subMesh) != gen::kSuccess)
						{
							ReleaseResources();
							delete[] nodeModels;
							return false;
						}
						if (subMesh.node != node) break;
						++currMesh;
					}
				}
			}
		}
		delete[] nodeModels;
	}

	return true;
}

bool CModelHierarchy::CreateFromSubMesh(const SSubMesh* subMesh, ID3D10EffectTechnique* exampleTechnique)
{
	// Release any existing geometry in this object
	ReleaseResources();

	// Create vertex element list & layout. We need a vertex layout to say what data we have per vertex in this model (e.g. position, normal, uv, etc.)
	// In previous projects the element list was a manually typed in array as we knew what data we would provide. However, as we can load models with
	// different vertex data this time we need flexible code. The array is built up one element at a time: ask the import class if it loaded normals, 
	// if so then add a normal line to the array, then ask if it loaded UVS...etc
	unsigned int numElts = 0;
	unsigned int offset = 0;
	// Position is always required
	m_VertexElts[numElts].SemanticName = "POSITION";   // Semantic in HLSL (what is this data for)
	m_VertexElts[numElts].SemanticIndex = 0;           // Index to add to semantic (a count for this kind of data, when using multiple of the same type, e.g. TEXCOORD0, TEXCOORD1)
	m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT; // Type of data - this one will be a float3 in the shader. Most data communicated as though it were colours
	m_VertexElts[numElts].AlignedByteOffset = offset;  // Offset of element from start of vertex data (e.g. if we have position (float3), uv (float2) then normal, the normal's offset is 5 floats = 5*4 = 20)
	m_VertexElts[numElts].InputSlot = 0;               // For when using multiple vertex buffers (e.g. instancing - an advanced topic)
	m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA; // Use this value for most cases (only changed for instancing)
	m_VertexElts[numElts].InstanceDataStepRate = 0;                     // --"--
	offset += 12;
	++numElts;
	// Repeat for each kind of vertex data
	if (subMesh->hasNormals)
	{
		m_VertexElts[numElts].SemanticName = "NORMAL";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 12;
		++numElts;
	}
	if (subMesh->hasTangents)
	{
		m_VertexElts[numElts].SemanticName = "TANGENT";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 12;
		++numElts;
	}
	if (subMesh->hasTextureCoords)
	{
		m_VertexElts[numElts].SemanticName = "TEXCOORD";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 8;
		++numElts;
	}
	if (subMesh->hasVertexColours)
	{
		m_VertexElts[numElts].SemanticName = "COLOR";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R8G8B8A8_UNORM; // A RGBA colour with 1 byte (0-255) per component
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 4;
		++numElts;
	}
	m_VertexSize = offset;

	// Given the vertex element list, pass it to DirectX to create a vertex layout. We also need to pass an example of a technique that will
	// render this model. We will only be able to render this model with techniques that have the same vertex input as the example we use here
	D3D10_PASS_DESC PassDesc;
	exampleTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	g_pd3dDevice->CreateInputLayout(m_VertexElts, numElts, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_VertexLayout);


	// Create the vertex buffer and fill it with the loaded vertex data
	m_NumVertices = subMesh->numVertices;
	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D10_USAGE_DEFAULT; // Not a dynamic buffer
	bufferDesc.ByteWidth = m_NumVertices * m_VertexSize; // Buffer size
	bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
	bufferDesc.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA initData; // Initial data
	initData.pSysMem = subMesh->vertices;
	if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, &initData, &m_VertexBuffer)))
	{
		return false;
	}


	// Create the index buffer - assuming 2-byte (WORD) index data
	m_NumIndices = static_cast<unsigned int>(subMesh->numFaces) * 3;
	bufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D10_USAGE_DEFAULT;
	bufferDesc.ByteWidth = m_NumIndices * sizeof(WORD);
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	initData.pSysMem = subMesh->faces;
	if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, &initData, &m_IndexBuffer)))
	{
		return false;
	}

	m_HasGeometry = true;
	return true;
}

// Control the model's position and rotation using keys provided. Amount of motion performed depends on frame time
void CModelHierarchy::Control(float frameTime, EKeyCode moveForward, EKeyCode moveBackward, EKeyCode turnLeft, EKeyCode turnRight)
{
	float maxRotation = 0.60;
	float rotationY = m_Children[0]->GetRotation().y;

	// Local Z movement - move in the direction of the Z axis, get axis from world matrix
	if (KeyHeld(moveForward))
	{
		m_Position.x += m_WorldMatrix._31 * (MoveSpeed / 2)* frameTime;
		m_Position.y +=m_WorldMatrix._32 * (MoveSpeed / 2) * frameTime;
		m_Position.z += m_WorldMatrix._33 * (MoveSpeed / 2) * frameTime;

		m_Rotation.y += RotSpeed * (rotationY * 2) * frameTime;
		
		// Rotate wheels
		m_Children[0]->m_Rotation.x += (RotSpeed * 2) * frameTime;
		m_Children[1]->m_Rotation.x += (RotSpeed * 2) * frameTime;

	}
	if (KeyHeld(moveBackward))
	{
		m_Position.x -= m_WorldMatrix._31 * (MoveSpeed / 2) * frameTime;
		m_Position.y -= m_WorldMatrix._32 * (MoveSpeed / 2) * frameTime;
		m_Position.z -= m_WorldMatrix._33 * (MoveSpeed / 2) * frameTime;

		m_Rotation.y -= RotSpeed * (rotationY * 2) * frameTime;
		// Rotate wheels
		m_Children[0]->m_Rotation.x -= (RotSpeed * 2) * frameTime;
		m_Children[1]->m_Rotation.x -= (RotSpeed * 2) * frameTime;


	}
	if (KeyHeld(turnRight) && m_Children[0]->m_Rotation.y < maxRotation)
	{
		m_Children[0]->m_Rotation.y += RotSpeed * frameTime;

	}
	if (KeyHeld(turnLeft) && m_Children[0]->m_Rotation.y > -maxRotation)
	{
		m_Children[0]->m_Rotation.y -= RotSpeed * frameTime;
	}

}


CModelHierarchy::~CModelHierarchy()
{
	ReleaseResources();
	CModel::ReleaseResources();
}
