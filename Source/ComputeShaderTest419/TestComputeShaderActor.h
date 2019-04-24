// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicRHIResourceArray.h" // Core module
#include "RenderCommandFence.h" // RenderCore module
#include "TestComputeShader.h"
#include "TestComputeShaderActor.generated.h"

UCLASS()
class ATestComputeShaderActor : public AActor {

	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATestComputeShaderActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
		bool InitializeInputPositions(const TArray<FVector>& input_positions);

	UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
		bool InitializeInputScalars(const TArray<float>& input_scalars);

	UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
		void InitializeOffsetYZ(const float y, const float z);

	UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
		bool Calculate(const float x, TArray<FVector>& output);

	UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
		bool Calculate_YZ_updated(const float x, const float y, const float z, TArray<FVector>& output);


private:
	int32 num_input_ = 0;
	FVector offset_;

	FRenderCommandFence m_RenderCommandFence; // Necessary for waiting until a render command function finishes.
	const TShaderMap<FGlobalShaderType>* shader_map = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	//Note:
	// GetWorld() function cannot be called from constructor, can be called after BeginPlay() instead.
	// So I used GMaxRHIFeatureLevel, instead of GetWorld()->Scene->GetFeatureLevel().
	//const TShaderMap<FTestComputeShader::ShaderMetaType>* shader_map = GetGlobalShaderMap(GetWorld()->Scene->GetFeatureLevel());

	//// Get the actual shader instance off the ShaderMap
	//TShaderMapRef<FTestComputeShader> test_compute_shader_{ shader_map }; // Note: test_compute_shader_(shader_map) causes error.

	TResourceArray<FVector> m_InputPositionsRA;
	FRHIResourceCreateInfo m_InputPositionsResource;
	FStructuredBufferRHIRef m_InputPositionsBuffer;
	FShaderResourceViewRHIRef m_InputPositionsSRV;

	TResourceArray<float> m_InputScalarsRA;
	FRHIResourceCreateInfo m_InputScalarsResource;
	FStructuredBufferRHIRef m_InputScalarsBuffer;
	FShaderResourceViewRHIRef m_InputScalarsSRV;

	TResourceArray<FVector> m_OutputRA; // Not necessary.
	FRHIResourceCreateInfo m_OutputResource;
	FStructuredBufferRHIRef m_OutputBuffer;
	FUnorderedAccessViewRHIRef m_OutputUAV;

	void InitializeOffsetYZ_RenderThread(const float y, const float z);

	void Calculate_RenderThread(const FVector xyz, const bool yz_updated, TArray<FVector>* output);

	void PrintResult(const TArray<FVector>& output);
};
