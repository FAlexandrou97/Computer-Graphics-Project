#pragma once
#include "Model.h"
#include "CImportXFile.h"

class CModelHierarchy : public CModel
{
private:
	// Child models of this model
	static const int MaxChildren = 32;
	int              m_NumChildren = 0;
	CModelHierarchy* m_Children[MaxChildren];

public:
	// Number of child models of this model
	int GetNumChildren()
	{
		return m_NumChildren;
	}

	// Get pointer to child model given index (range 0 -> NumChildren - 1)
	CModelHierarchy* GetChild(int child)
	{
		return m_Children[child];
	}

	// Create a new child model for this model (add it to current list of children)
	// Returns a pointer to the new child model
	CModelHierarchy* CModelHierarchy::CreateNewChild();
	
	bool CModelHierarchy::Load(const string& fileName, ID3D10EffectTechnique* exampleTechnique, bool tangents = false);

	// Create this model using a CMesh class sub-mesh. Helper function for LoadModel above
	bool CModelHierarchy::CreateFromSubMesh(const gen::SSubMesh* subMesh, ID3D10EffectTechnique* exampleTechnique);
	void CModelHierarchy::ReleaseResources();
	void CModelHierarchy::Control(float frameTime, EKeyCode moveForward, EKeyCode moveBackward, EKeyCode turnLeft, EKeyCode turnRight);
	CModelHierarchy::~CModelHierarchy();
};

