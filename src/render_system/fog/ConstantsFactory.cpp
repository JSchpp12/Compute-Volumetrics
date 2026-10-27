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
constexpr int kSizeLocalXId = 0;
constexpr int kSizeLocalYId = 1;
constexpr int kSizeLocalZId = 2;
constexpr int kNumRaySamplesId = 3;

constexpr std::uint32_t kSizeLocalX = 8u;
constexpr std::uint32_t kSizeLocalY = 8u;
constexpr std::uint32_t kSizeLocalZ = 1u;
constexpr std::uint32_t kNumRaySamples = 1u;

constexpr int kEnableTrilinearInterpolationGridLookupsId = 0;
constexpr bool kEnableTrilinearInterpolationGridLookups = false;
} // namespace

template <typename T>
static void AddConstantOrDefault(star::StarPipelineConstants &constants, int id, const std::optional<T> &overrideValue,
                                 const T &defaultValue)
{
    constants.addConstant(id, overrideValue.value_or(defaultValue));
}

void GetPipelineConstantsForPrecomputeTransmittance(star::command::pipeline::CreatePipeline &cmd,
                                                    const TransmittanceConstants &overrideDefaults)
{
    star::StarPipelineConstants pipelineConstants{};

    AddConstantOrDefault(pipelineConstants, kSizeLocalXId, overrideDefaults.sizeLocalX, kSizeLocalX);
    AddConstantOrDefault(pipelineConstants, kSizeLocalYId, overrideDefaults.sizeLocalY, kSizeLocalY);
    AddConstantOrDefault(pipelineConstants, kSizeLocalZId, overrideDefaults.sizeLocalZ, kSizeLocalZ);
    AddConstantOrDefault(pipelineConstants, kNumRaySamplesId, overrideDefaults.numRaySamples, kNumRaySamples);

    cmd.setPipelineConstantForStage(star::Shader_Stage::compute, std::move(pipelineConstants));
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
