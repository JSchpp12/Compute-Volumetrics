#include "render_system/fog/ConstantsFactory.hpp"

#include <starlight/command/pipeline/CreatePipeline.hpp>
#include <starlight/enums/Enums.hpp>
#include <starlight/wrappers/graphics/StarPipelineConstants.hpp>

#include <cstdint>
#include <optional>

namespace render_system::fog
{
namespace
{
// Precompute Transmittance Constants
constexpr int kNumRaySamplesId = 0;

constexpr std::uint32_t kNumRaySamples = 1u;

constexpr int kRandTextureBreathId = 1;

constexpr int kEnableTrilinearInterpolationGridLookupsId = 0;
constexpr bool kEnableTrilinearInterpolationGridLookups = false;
} // namespace

template <typename T>
static void AddConstantOrDefault(star::StarPipelineConstants &constants, int id, const std::optional<T> &overrideValue,
                                 const T &defaultValue)
{
    constants.addConstant(id, overrideValue.value_or(defaultValue));
}

star::StarPipelineConstants MakePrecomputeTransmittanceConstants(const TransmittanceConstants &overrides)
{
    star::StarPipelineConstants pipelineConstants{};

    AddConstantOrDefault(pipelineConstants, kNumRaySamplesId, overrides.numRaySamples, kNumRaySamples);
    pipelineConstants.addConstant(kRandTextureBreathId, overrides.randTextureBreath);

    return pipelineConstants;
}

void GetPipelineConstantsForColorMarch(star::command::pipeline::CreatePipeline &cmd,
                                       const ColorConstants &overrideDefaults)
{

    star::StarPipelineConstants pipelineConstants{};
    AddConstantOrDefault(pipelineConstants, kEnableTrilinearInterpolationGridLookupsId,
                         overrideDefaults.enableTrilinearInterpolationGridLookups,
                         kEnableTrilinearInterpolationGridLookups);

    cmd.setPipelineConstantForStage(star::Shader_Stage::compute, std::move(pipelineConstants));
}
} // namespace render_system::fog
