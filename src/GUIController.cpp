#include "GUIController.hpp"

#ifdef STAR_ENABLE_PRESENTATION

#include "FogType.hpp"
#include "Volume.hpp"
#include "render_system/fog/struct/ShaderFlags.hpp"
#include "structs/FogInfo.hpp"

#include <starlight/core/helper/star_object/ObjectHelpers.hpp>
#include <starlight/virtual/StarScene.hpp>

#include <imgui.h>

#include <cfloat>

template <typename FlagType> static void DrawShaderFlagToggle(Volume &volume, const FlagType flag)
{
    bool state = volume.isShaderDebugEnabled(flag);
    if (ImGui::Checkbox(render_system::fog::to_string(flag).data(), &state))
    {
        volume.setShaderDebug(flag, state);
    }
}

struct FogTypeOption
{
    const char *label;
    Fog::Type type;
};

static constexpr FogTypeOption kFogTypeOptions[] = {
    {"Linear", Fog::Type::sLinear},
    {"Exponential", Fog::Type::sExponential},
    {"Marched", Fog::Type::sMarched},
    {"Nano Bounding Box", Fog::Type::sNanoBoundingBox},
    {"Nano Surface", Fog::Type::sNanoSurface},
};

static void DrawFogTypeSelector(Volume &volume)
{
    const Fog::Type current = volume.getRenderer().getFogType();

    const char *preview = nullptr;
    for (const auto &option : kFogTypeOptions)
    {
        if (option.type == current)
        {
            preview = option.label;
        }
    }

    ImGui::SeparatorText("Fog Type");
    if (ImGui::BeginCombo("##fogType", preview != nullptr ? preview : "Unknown"))
    {
        for (const auto &option : kFogTypeOptions)
        {
            const bool selected = option.type == current;
            if (ImGui::Selectable(option.label, selected))
            {
                volume.setFogType(option.type);
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

static void DrawLinearFogControls(LinearFogInfo &linear)
{
    ImGui::SeparatorText("Linear Fog");
    ImGui::DragFloat("Near Dist", &linear.nearDist, 0.1f, 0.0f, FLT_MAX, "%.3f");
    ImGui::DragFloat("Far Dist", &linear.farDist, 10.0f, 0.0f, FLT_MAX, "%.3f");
}

static void DrawExpFogControls(ExpFogInfo &exp)
{
    ImGui::SeparatorText("Exponential Fog");
    ImGui::DragFloat("Density", &exp.density, 0.01f, 0.0f, FLT_MAX, "%.4f");
}

/// Values held privately by MarchedFogInfo are written back through their
/// setters so their clamping still applies.
static void DrawMarchedFogControls(MarchedFogInfo &marched)
{
    ImGui::SeparatorText("Marched Fog");

    ImGui::DragFloat("Default Density", &marched.defaultDensity, 0.001f, 0.0f, FLT_MAX, "%.6f");

    float densityMultiplier = marched.getDensityMultiplier();
    if (ImGui::DragFloat("Density Multiplier", &densityMultiplier, 0.01f, 0.0f, FLT_MAX, "%.4f"))
    {
        marched.setDensityMultiplier(densityMultiplier);
    }

    float sigmaAbsorption = marched.getSigmaAbsorption();
    if (ImGui::DragFloat("Sigma Absorption", &sigmaAbsorption, 0.0001f, 0.0f, FLT_MAX, "%.6f"))
    {
        marched.setSigmaAbsorption(sigmaAbsorption);
    }

    float sigmaScattering = marched.getSigmaScattering();
    if (ImGui::DragFloat("Sigma Scattering", &sigmaScattering, 0.0001f, 0.0f, FLT_MAX, "%.6f"))
    {
        marched.setSigmaScattering(sigmaScattering);
    }

    float lightPropertyDirG = marched.getLightPropertyDirG();
    if (ImGui::DragFloat("Light Property Dir G", &lightPropertyDirG, 0.01f, -1.0f, 1.0f, "%.3f"))
    {
        marched.setLightPropertyDirG(lightPropertyDirG);
    }

    ImGui::DragFloat("Step Size Dist", &marched.stepSizeDist, 1.0f, 0.0f, FLT_MAX, "%.3f");
    ImGui::DragFloat("Step Size Light", &marched.stepSizeDist_light, 1.0f, 0.0f, FLT_MAX, "%.3f");

    float colorTransparencyCutoff = marched.getColorTransparencyCutoff();
    if (ImGui::DragFloat("Color Transparency Cutoff", &colorTransparencyCutoff, 0.000001f, 0.0f, 1.0f, "%.6f"))
    {
        marched.setColorTransparencyCutoff(colorTransparencyCutoff);
    }

    float distanceTransparencyCutoff = marched.getDistanceTransparencyCutoff();
    if (ImGui::DragFloat("Distance Transparency Cutoff", &distanceTransparencyCutoff, 0.000001f, 0.0f, 1.0f, "%.6f"))
    {
        marched.setDistanceTransparencyCutoff(distanceTransparencyCutoff);
    }

    float lightExtinctionScale = marched.getLightExtinctionScale();
    if (ImGui::DragFloat("Light Extinction Scale", &lightExtinctionScale, 0.01f, 0.0f, FLT_MAX, "%.4f"))
    {
        marched.setLightExtinctionScale(lightExtinctionScale);
    }

    float shadowBias = marched.getShadowBias();
    if (ImGui::DragFloat("Shadow Bias", &shadowBias, 0.001f, 0.0f, FLT_MAX, "%.5f"))
    {
        marched.setShadowBias(shadowBias);
    }
}

static void DrawFogSection(Volume &volume)
{
    if (!ImGui::CollapsingHeader("Fog"))
    {
        return;
    }

    ImGui::Indent();

    DrawFogTypeSelector(volume);

    auto &fog = volume.getRenderer().getFogInfo();
    DrawLinearFogControls(fog.linearInfo);
    DrawExpFogControls(fog.expFogInfo);
    DrawMarchedFogControls(fog.marchedInfo);

    ImGui::Unindent();
}

static void DrawVolumeTransform(const Volume &volume)
{
    const auto &instance = volume.getInstance(0);
    const auto position = instance.getPosition();
    const auto rotation = star::core::helper::star_object::ExtractRotationDegrees(instance.getRotationMat());
    const auto scale = instance.getScale();

    ImGui::SeparatorText("Volume Transform");
    ImGui::Text("Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
    ImGui::Text("Rotation (deg): %.2f, %.2f, %.2f", rotation.x, rotation.y, rotation.z);
    ImGui::Text("Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
}

static void DrawShaderFlagControls(Volume &volume)
{
    using render_system::fog::MarchShaderFlags;
    using render_system::fog::PrecomputeLightTransmittanceShaderFlags;

    if (!ImGui::CollapsingHeader("Debug Shader Flags"))
    {
        return;
    }

    ImGui::Indent();

    ImGui::SeparatorText("March Shader Flags");
    DrawShaderFlagToggle(volume, MarchShaderFlags::EnableDebugHighlightCutoffValue);
    DrawShaderFlagToggle(volume, MarchShaderFlags::EnableDebugHighlightShadows);
    DrawShaderFlagToggle(volume, MarchShaderFlags::EnableDebugTransmittanceMap);
    DrawShaderFlagToggle(volume, MarchShaderFlags::EnableDebugForceMarchCalculateTransmittance);

    ImGui::SeparatorText("Precompute Light Transmittance Shader Flags");
    DrawShaderFlagToggle(volume, PrecomputeLightTransmittanceShaderFlags::EnableDebugSetAreasInShadow);
    DrawShaderFlagToggle(volume, PrecomputeLightTransmittanceShaderFlags::EnableDebugDisableAabbRayTest);
    DrawShaderFlagToggle(volume, PrecomputeLightTransmittanceShaderFlags::EnableDebugSetTexelsOfInterest);

    ImGui::Unindent();
}

void GUIController::setScene(star::StarScene *scene) noexcept
{
    m_scene = scene;
}

void GUIController::setVolume(Volume *volume) noexcept
{
    m_volume = volume;
}

void GUIController::draw()
{
    // Begin() reports false when the window is collapsed, but End() is still
    // required for every Begin().
    if (ImGui::Begin("Volumetrics"))
    {
        if (m_scene != nullptr && m_scene->getCamera())
        {
            const auto position = m_scene->getCamera()->getPosition();
            ImGui::Text("Camera: %.2f, %.2f, %.2f", position.x, position.y, position.z);
        }

        if (m_volume != nullptr)
        {
            DrawVolumeTransform(*m_volume);
            DrawFogSection(*m_volume);
            DrawShaderFlagControls(*m_volume);
        }
    }
    ImGui::End();
}

#endif
