// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

#include "TestComputeShaderActor.h"
#include <GameFramework/Actor.h>
#include <Engine/World.h>
#include <SceneInterface.h>

// Sets default values
ATestComputeShaderActor::ATestComputeShaderActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}


// Called when the game starts or when spawned
void ATestComputeShaderActor::BeginPlay()
{
	// Has to be initialized at BeginPlay(), instead of the class's constructor.

	//ERHIFeatureLevel::Type shader_feature_level_test = GetWorld()->Scene->GetFeatureLevel();
	ERHIFeatureLevel::Type shader_feature_level_test = GMaxRHIFeatureLevel;

	UE_LOG(LogTemp, Warning, TEXT("Shader Feature Level: %d"), shader_feature_level_test);
	UE_LOG(LogTemp, Warning, TEXT("Max Shader Feature Level: %d"), GMaxRHIFeatureLevel);

	Super::BeginPlay();
}


void ATestComputeShaderActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// I'm not sure where is the appropriate place to call the following SafeRelease methods.
	// Destructor? EndPlay? BeginDestroy??
	m_InputPositionsBuffer.SafeRelease();
	m_InputPositionsSRV.SafeRelease();

	m_InputScalarsBuffer.SafeRelease();
	m_InputScalarsSRV.SafeRelease();

	m_OutputBuffer.SafeRelease();
	m_OutputUAV.SafeRelease();

	Super::EndPlay(EndPlayReason);
}


// Called every frame
void ATestComputeShaderActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ____________________________________

bool ATestComputeShaderActor::InitializeInputPositions(const TArray<FVector>& input_positions)
{
	if (input_positions.Num() == 0) {
		UE_LOG(LogTemp, Warning, TEXT("Error: input_positions is empty at ATestComputeShaderActor::InitializeInputPosition."));
		return false;
	}

	num_input_ = input_positions.Num();

	// We need to copy TArray to TResourceArray to set RHICreateStructuredBuffer.
	m_InputPositionsRA.SetNum(num_input_);
	FMemory::Memcpy(m_InputPositionsRA.GetData(), input_positions.GetData(), sizeof(FVector) * num_input_);

	m_InputPositionsResource.ResourceArray = &m_InputPositionsRA;

	// Note: In D3D11StructuredBuffer.cpp, ResourceArray->Discard() function is called, but not discarded??
	m_InputPositionsBuffer = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * num_input_, BUF_ShaderResource, m_InputPositionsResource);
	m_InputPositionsSRV = RHICreateShaderResourceView(m_InputPositionsBuffer);

	return true;
}


bool ATestComputeShaderActor::InitializeInputScalars(const TArray<float>& input_scalars)
{
	if (input_scalars.Num() == 0) {
		UE_LOG(LogTemp, Warning, TEXT("Error: input_scalars is empty at ATestComputeShaderActor::InitializeInputScalar."));
		return false;
	}
	if (input_scalars.Num() != num_input_) {
		UE_LOG(LogTemp, Warning, TEXT("Error: input_scalars and input_positions do not have the same elements at ATestComputeShaderActor::InitializeInputScalar."));
		return false;
	}

	m_InputScalarsRA.SetNum(num_input_);
	FMemory::Memcpy(m_InputScalarsRA.GetData(), input_scalars.GetData(), sizeof(float) * num_input_);

	m_InputScalarsResource.ResourceArray = &m_InputScalarsRA;
	m_InputScalarsBuffer = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * num_input_, BUF_ShaderResource, m_InputScalarsResource);
	m_InputScalarsSRV = RHICreateShaderResourceView(m_InputScalarsBuffer);
	return true;
}


void ATestComputeShaderActor::InitializeOffsetYZ(const float y, const float z)
{
	offset_.Y = y;
	offset_.Z = z;

	// ENQUEUE_RENDER_COMMAND
	ATestComputeShaderActor* compute_shader_actor = this;
	UWorld* World = GetWorld();

	ENQUEUE_RENDER_COMMAND(InitializeOffsetYZCommand)(
		[compute_shader_actor, World, y, z](FRHICommandListImmediate& RHICmdList)
	{
		compute_shader_actor->InitializeOffsetYZ_RenderThread(World, y, z);
	});

	m_RenderCommandFence.BeginFence();
	m_RenderCommandFence.Wait();
}


void ATestComputeShaderActor::InitializeOffsetYZ_RenderThread(UWorld* World, const float y, const float z)
{
	check(IsInRenderingThread());

	// Get global RHI command list
	FRHICommandListImmediate& rhi_command_list = GRHICommandList.GetImmediateCommandList();

	// Get the actual shader instance off the ShaderMap
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
	auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FTestComputeShader> test_compute_shader_(ShaderMap);

	rhi_command_list.SetComputeShader(test_compute_shader_->GetComputeShader());
	test_compute_shader_->SetOffsetYZ(rhi_command_list, y, z);
}


// According to some result, UniformBuffer does not seem to be kept saved even if UniformBuffer_MultiFrame flag is set...
bool ATestComputeShaderActor::Calculate( const float x, TArray<FVector>& output)
{
	if ((num_input_ == 0) || (m_InputPositionsRA.Num() != m_InputScalarsRA.Num())) {
		UE_LOG(LogTemp, Warning, TEXT("Error: input_positions or input_scalars have not been set correctly at ATestComputeShaderActor::Calculate."));
		return false;
	}

	// In this sample code, output_buffer_ has not input values, so what we need here is just the pointer to output_resource_.
	//output_RA_.SetNum(num_input_);
	//output_resource_.ResourceArray = &output_RA_;
	m_OutputBuffer = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * num_input_, BUF_ShaderResource | BUF_UnorderedAccess, m_OutputResource);
	m_OutputUAV = RHICreateUnorderedAccessView(m_OutputBuffer, /* bool bUseUAVCounter */ false, /* bool bAppendBuffer */ false);

	output.SetNum(num_input_);
	offset_.X = x;

	// ENQUEUE_RENDER_COMMAND
	ATestComputeShaderActor* compute_shader_actor = this;
	const FVector offset = offset_;
	const bool yz_updated = false;
	TArray<FVector>* outputCmd = &output;
	UWorld* World = GetWorld();

	ENQUEUE_RENDER_COMMAND(CalculateCommand)(
		[compute_shader_actor, World, offset, yz_updated, outputCmd](FRHICommandListImmediate& RHICmdList)
	{
		compute_shader_actor->Calculate_RenderThread(World, offset, yz_updated, outputCmd);
	});

	m_RenderCommandFence.BeginFence();
	m_RenderCommandFence.Wait(); // Waits for pending fence commands to retire.

	UE_LOG(LogTemp, Warning, TEXT("===== Calculate ====="));

	return true;
}


bool ATestComputeShaderActor::Calculate_YZ_updated(const float x, const float y, const float z, TArray<FVector>& output)
{
	if ((num_input_ == 0) || (m_InputPositionsRA.Num() != m_InputScalarsRA.Num())) {
		UE_LOG(LogTemp, Warning, TEXT("Error: input_positions or input_scalars have not been set correctly at ATestComputeShaderActor::Calculate."));
		return false;
	}

	m_OutputBuffer = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * num_input_, BUF_ShaderResource | BUF_UnorderedAccess, m_OutputResource);
	m_OutputUAV = RHICreateUnorderedAccessView(m_OutputBuffer, /* bool bUseUAVCounter */ false, /* bool bAppendBuffer */ false);

	output.SetNum(num_input_);
	offset_.Set(x, y, z);

	// ENQUEUE_RENDER_COMMAND
	ATestComputeShaderActor* compute_shader_actor = this;
	const FVector offset = offset_;
	const bool yz_updated = true;
	TArray<FVector>* outputCmd = &output;
	UWorld* World = GetWorld();

	ENQUEUE_RENDER_COMMAND(CalculateCommand)(
		[compute_shader_actor, World, offset, yz_updated, outputCmd](FRHICommandListImmediate& RHICmdList)
	{
		compute_shader_actor->Calculate_RenderThread(World, offset, yz_updated, outputCmd);
	});

	m_RenderCommandFence.BeginFence();
	m_RenderCommandFence.Wait();

	UE_LOG(LogTemp, Warning, TEXT("===== Calculate_YZ_Updated ====="));

	return true;
}


void ATestComputeShaderActor::Calculate_RenderThread(UWorld* World, const FVector xyz, const bool yz_updated,TArray<FVector>* output)
{
	check(IsInRenderingThread());

	// Get global RHI command list
	FRHICommandListImmediate& rhi_command_list = GRHICommandList.GetImmediateCommandList();

	// Get the actual shader instance off the ShaderMap
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
	auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FTestComputeShader> test_compute_shader_(ShaderMap);

	rhi_command_list.SetComputeShader(test_compute_shader_->GetComputeShader());
	test_compute_shader_->SetOffsetX(rhi_command_list, xyz.X);
	if (yz_updated) {
		test_compute_shader_->SetOffsetYZ(rhi_command_list, xyz.Y, xyz.Z);
	}
	test_compute_shader_->SetInputPosition(rhi_command_list, m_InputPositionsSRV);
	test_compute_shader_->SetInputScalar(rhi_command_list, m_InputScalarsSRV);
	test_compute_shader_->SetOutput(rhi_command_list, m_OutputUAV);

	DispatchComputeShader(rhi_command_list, *test_compute_shader_, num_input_, 1, 1);

	test_compute_shader_->ClearOutput(rhi_command_list);
	test_compute_shader_->ClearParameters(rhi_command_list);

	const FVector* shader_data = (const FVector*)rhi_command_list.LockStructuredBuffer(m_OutputBuffer, 0, sizeof(FVector) * num_input_, EResourceLockMode::RLM_ReadOnly);
	FMemory::Memcpy(output->GetData(), shader_data, sizeof(FVector) * num_input_);
	// If you would like to get the partial data, (*output)[index] = *(shader_data + index) is more efficient??

	//for (int32 index = 0; index < num_input_; ++index) {
	//  (*output)[index] = *(shader_data + index);
	//}

	rhi_command_list.UnlockStructuredBuffer(m_OutputBuffer);
}
