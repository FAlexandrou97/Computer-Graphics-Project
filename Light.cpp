#include "Light.h"

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


