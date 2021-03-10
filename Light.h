#pragma once
#include "Model.h"

class CLight : public CModel
{

private:
    D3DXVECTOR3 m_Colour;
    float m_OrbitRadius = 15.0f;
    float m_OrbitSpeed = 0.7f;
    int m_FSM = 1;
public:
    D3DXVECTOR3 GetColour();
    void SetColour(D3DXVECTOR3 colour);
    void Pulsate(D3DXVECTOR3 minColour, D3DXVECTOR3 maxColour, float colourUpdateSpeed);
    void ChangeColour(float colourUpdateSpeed);
    float GetOrbitSpeed();
    float GetOrbitRadius();
};

