// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h" //ShaderCore module
#include "UniformBuffer.h" // RenderCore module
#include "RHICommandList.h" // RHI module

// The first "F", like "FStruct" is not necessary for uniform buffer struct??
// The struct has to be implemented in .cpp using IMPLEMENT_UNIFORM_BUFFER_STRUCT
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(OffsetYZ, )
SHADER_PARAMETER(float, y)
SHADER_PARAMETER(float, z)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

class FTestComputeShader : public FGlobalShader {
  DECLARE_EXPORTED_SHADER_TYPE(FTestComputeShader, Global, );

public:
  /* Essential functions */

  FTestComputeShader() {}
  explicit FTestComputeShader(const ShaderMetaType::CompiledShaderInitializerType& initializer);

  static bool ShouldCache(EShaderPlatform platform) {
    return true;
    //// Could skip compiling if the platform does not support DirectX Shader Model 4, for example, like the following.
    //return IsFeatureLevelSupported(platform, ERHIFeatureLevel::SM4);
  }

  static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& PermutationParams, FShaderCompilerEnvironment& OutEnvironment) {
    FGlobalShader::ModifyCompilationEnvironment(PermutationParams, OutEnvironment);
    OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
  }

  static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& PermutationParams) {
    // Useful when adding a permutation of a particular shader
    return true;
  }

  virtual bool Serialize(FArchive& Ar) override;

  /* Custom functions */

  void SetOffsetX(FRHICommandList& rhi_command_list, const float offset_x);
  void SetOffsetYZ(FRHICommandList& rhi_command_list, const float offset_y, const float offset_z);

  // FShaderResourceViewRHIRef for StructuredBuffer, FUnorderedAccessViewRHIParamRef for RWStructuredBuffer.
  void SetInputPosition(FRHICommandList& rhi_command_list, FShaderResourceViewRHIRef input_position);
  void SetInputScalar(FRHICommandList& rhi_command_list, FShaderResourceViewRHIRef input_scalar);
  void SetOutput(FRHICommandList& rhi_command_list, FUnorderedAccessViewRHIParamRef output);
  void ClearParameters(FRHICommandList& rhi_command_list); // for StructuredBuffer.
  void ClearOutput(FRHICommandList& rhi_command_list); // for RWStructuredBuffer.

private:
  FShaderParameter offset_x_; // float test_offset_x;

  FShaderResourceParameter input_position_; // StructuredBuffer<float3> test_input_position;
  FShaderResourceParameter input_scalar_; // StructuredBuffer<float> test_input_scalar;
  FShaderResourceParameter output_; // RWStructuredBuffer<float3> test_output;
};