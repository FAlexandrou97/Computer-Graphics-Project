#pragma once
#include "Model.h"

class CLight : public CModel
{

private:
    D3DXVECTOR3 m_Colour;
    float m_OrbitRadius = 15.0f;
    float m_OrbitSpeed = 0.7f;
    int m_FSM = 1;
    float m_ConeAngle = 90.0f;
    ID3D10EffectVectorVariable* m_FacingVar = NULL;
    ID3D10EffectMatrixVariable* m_ViewMatrixVar = NULL;
    ID3D10EffectMatrixVariable* m_ProjMatrixVar = NULL;
    ID3D10EffectScalarVariable* m_ConeAngleVar = NULL;
public:
    D3DXVECTOR3 GetColour();
    void SetColour(D3DXVECTOR3 colour);
    void Pulsate(D3DXVECTOR3 minColour, D3DXVECTOR3 maxColour, float colourUpdateSpeed);
    void ChangeColour(float colourUpdateSpeed);
    float GetOrbitSpeed();
    float GetOrbitRadius();
    //void CLight::FacePoint(D3DXVECTOR3 point);
    D3DXVECTOR3 CLight::GetFacing();
    float CLight::GetConeAngle();
    void CLight::SetConeAngle(float coneAngle);
    D3DXMATRIXA16 CLight::CalculateLightViewMatrix();
    D3DXMATRIXA16 CLight::CalculateLightProjMatrix();
    ID3D10EffectVectorVariable* CLight::GetFacingVar();
    void CLight::SetFacingVar(ID3D10EffectVectorVariable* facingVar);
    ID3D10EffectMatrixVariable* CLight::GetViewMatrixVar();
    void CLight::SetViewMatrixVar(ID3D10EffectMatrixVariable* viewMatrix);
    ID3D10EffectMatrixVariable* CLight::GetProjMatrixVar();
    void CLight::SetProjMatrixVar(ID3D10EffectMatrixVariable* projMatrix);
    ID3D10EffectScalarVariable* CLight::GetConeAngleVar();
    void CLight::SetConeAngleVar(ID3D10EffectScalarVariable* coneAngleVar);
};

