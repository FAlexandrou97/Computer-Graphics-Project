
#include "Light.h"
#include "CImportXFile.h"    // Class to load meshes (taken from a full graphics engine)
using namespace gen;
D3DXVECTOR3 CLight::GetColour()
{
    return m_Colour;
}

void CLight::SetColour(D3DXVECTOR3 colour)
{
    m_Colour = colour;
}

void CLight::Pulsate(D3DXVECTOR3 minColour, D3DXVECTOR3 maxColour, float colourUpdateSpeed)
{
	switch (m_FSM)
	{
	case 0:
		m_Colour.operator+=(D3DXVECTOR3(0.5f * colourUpdateSpeed, 0.5f * colourUpdateSpeed, 0.5f * colourUpdateSpeed));
		if (m_Colour.x > maxColour.x && m_Colour.y > maxColour.y && m_Colour.z > maxColour.z)
			m_FSM = 1;
		break;
	case 1:
		m_Colour.operator-=(D3DXVECTOR3(0.5f * colourUpdateSpeed, 0.5f * colourUpdateSpeed, 0.5f * colourUpdateSpeed));
		if (m_Colour.x < minColour.x && m_Colour.y < minColour.y && m_Colour.z < minColour.z)
			m_FSM = 0;
		break;
	default:
		break;
	}
}

void CLight::ChangeColour(float colourUpdateSpeed)
{
	switch (m_FSM)
	{
	case 0:
		m_Colour.x -= 0.5f * colourUpdateSpeed;
		if (m_Colour.x < 0.0f)
			m_FSM = 1;
		break;
	case 1:
		m_Colour.x += 0.5f * colourUpdateSpeed;
		if (m_Colour.x > 5.0f)
			m_FSM = 2;
		break;
	case 2:
		m_Colour.y -= 0.5f * colourUpdateSpeed;
		if (m_Colour.y < 0.0f)
			m_FSM = 3;
		break;
	case 3:
		m_Colour.x -= 0.25f * colourUpdateSpeed;
		m_Colour.y += 0.5f * colourUpdateSpeed;
		if (m_Colour.y > 5.0f)
			m_FSM = 4;
		break;
	case 4:
		m_Colour.z -= 0.5f * colourUpdateSpeed;
		if (m_Colour.z < 0.0f)
			m_FSM = 5;
		break;
	case 5:
		m_Colour.y -= 0.25f * colourUpdateSpeed;
		m_Colour.z += 0.5f * colourUpdateSpeed;
		if (m_Colour.z > 5.0f)
			m_FSM = 0;
		break;
	default:
		break;
	}
}

float CLight::GetOrbitSpeed()
{
	return m_OrbitSpeed;
}

float CLight::GetOrbitRadius()
{
	return m_OrbitRadius;
}

// Get the direction the model is facing
D3DXVECTOR3 CLight::GetFacing()
{
	D3DXVECTOR3 facing;
	D3DXVec3Normalize(&facing, &D3DXVECTOR3(&this->GetWorldMatrix()(2, 0)));
	return facing;
}

float CLight::GetConeAngle()
{
	return m_ConeAngle;
}

void CLight::SetConeAngle(float coneAngle)
{
	m_ConeAngle = coneAngle;
}

// Make the model face a given point
//void CLight::FacePoint(D3DXVECTOR3 point)
//{
//	CMatrix4x4 facingMatrix = MatrixFaceTarget(CVector3(this->GetPosition()), CVector3(point));
//	facingMatrix.DecomposeAffineEuler((CVector3*)&this->GetPosition(), (CVector3*)&this->GetRotation(), 0);
//}

// Get "camera-like" view matrix for a spot light
D3DXMATRIXA16 CLight::CalculateLightViewMatrix()
{
	D3DXMATRIXA16 viewMatrix;

	// Get the world matrix of the light model and invert it to get the view matrix (that is more-or-less the definition of a view matrix)
	// We don't always have a physical model for a light, in which case we would need to store this data along with the light colour etc.
	D3DXMATRIXA16 worldMatrix = this->GetWorldMatrix();
	D3DXMatrixInverse(&viewMatrix, NULL, &worldMatrix);

	return viewMatrix;
}

// Get "camera-like" projection matrix for a spot light
D3DXMATRIXA16 CLight::CalculateLightProjMatrix()
{
	D3DXMATRIXA16 projMatrix;

	// Create a projection matrix for the light. Use the spotlight cone angle as an FOV, just set default values for everything else.
	D3DXMatrixPerspectiveFovLH(&projMatrix, ToRadians(this->m_ConeAngle), 1, 0.1f, 1000.0f);

	return projMatrix;
}

ID3D10EffectVectorVariable* CLight::GetFacingVar()
{
	return m_FacingVar;
}

void CLight::SetFacingVar(ID3D10EffectVectorVariable* facingVar)
{
	m_FacingVar = facingVar;
}

ID3D10EffectMatrixVariable* CLight::GetViewMatrixVar()
{
	return m_ViewMatrixVar;
}

void CLight::SetViewMatrixVar(ID3D10EffectMatrixVariable* viewMatrix)
{
	m_ViewMatrixVar = viewMatrix;
}

ID3D10EffectMatrixVariable* CLight::GetProjMatrixVar()
{
	return m_ProjMatrixVar;
}

void CLight::SetProjMatrixVar(ID3D10EffectMatrixVariable* projMatrix)
{
	m_ProjMatrixVar = projMatrix;
}

ID3D10EffectScalarVariable* CLight::GetConeAngleVar()
{
	return m_ConeAngleVar;
}

void CLight::SetConeAngleVar(ID3D10EffectScalarVariable* coneAngleVar)
{
	m_ConeAngleVar = coneAngleVar;
}

void CLight::OrbitAround(CModel* model, float frameTime)
{
	SetPosition(model->GetPosition() + D3DXVECTOR3(cos(m_CubeLightRotate) * GetOrbitRadius(), 5, sin(m_CubeLightRotate) * GetOrbitRadius()));
	m_CubeLightRotate -= GetOrbitSpeed() * frameTime;
}


