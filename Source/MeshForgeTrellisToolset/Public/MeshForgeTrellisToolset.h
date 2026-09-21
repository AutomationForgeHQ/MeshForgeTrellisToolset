// The TRELLIS.2 runner and its container, for agents.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "ToolsetRegistry/ToolCallAsyncResult.h"
#include "MeshForgeTrellisToolset.generated.h"

/** Everything about whether TRELLIS.2 can generate right now, in one answer. */
USTRUCT(BlueprintType)
struct FTrellisRunnerStatus
{
	GENERATED_BODY()

	/** True when a generation submitted now would run. Everything else explains why not. */
	UPROPERTY(BlueprintReadOnly, Category = "Runner")
	bool bReadyToGenerate = false;

	/** One sentence a person can act on. Empty when ready. */
	UPROPERTY(BlueprintReadOnly, Category = "Runner")
	FString Advice;

	// --- Docker ---

	UPROPERTY(BlueprintReadOnly, Category = "Docker")
	bool bDockerInstalled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Docker")
	bool bDockerRunning = false;

	/** False when Docker cannot hand a container a GPU. The runner would come up on the CPU. */
	UPROPERTY(BlueprintReadOnly, Category = "Docker")
	bool bDockerGpuSupport = false;

	UPROPERTY(BlueprintReadOnly, Category = "Docker")
	FString DockerVersion;

	// --- Container ---

	/** False before the image has ever been built. */
	UPROPERTY(BlueprintReadOnly, Category = "Container")
	bool bContainerExists = false;

	UPROPERTY(BlueprintReadOnly, Category = "Container")
	bool bContainerRunning = false;

	// --- Runner ---

	/** True when the HTTP service answers. Separate from the container being up. */
	UPROPERTY(BlueprintReadOnly, Category = "Runner")
	bool bRunnerAnswering = false;

	/** What the runner said about itself. Empty when it did not answer. */
	UPROPERTY(BlueprintReadOnly, Category = "Runner")
	FString RunnerMessage;

	/** Where the plugin is looking for it. */
	UPROPERTY(BlueprintReadOnly, Category = "Runner")
	FString RunnerUrl;
};

/** A container operation, completed. */
UCLASS(BlueprintType)
class MESHFORGETRELLISTOOLSET_API UToolCallAsyncResultTrellisStatus : public UToolCallAsyncResult
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "MeshForge")
	bool SetValue(const FTrellisRunnerStatus& InValue)
	{
		return MaybeBroadcastSuccessfulCompletion(FTrellisRunnerStatus(InValue), Value);
	}

	UPROPERTY(BlueprintReadOnly, Category = "MeshForge")
	FTrellisRunnerStatus Value;
};

/**
 * The local TRELLIS.2 runner: is it up, start it, stop it, read its log.
 *
 * Deliberately absent, and this is a design decision rather than an omission: there is no tool that
 * removes the container or its volumes. The model cache is several gigabytes that took a long time
 * to download, and an agent tidying up should not be able to cost somebody that. A person can do it
 * with `docker compose down -v`.
 */
UCLASS(BlueprintType)
class MESHFORGETRELLISTOOLSET_API UMeshForgeTrellisToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:

	/**
	 * The version an agent is told it is talking to, read from this plugin's own descriptor.
	 *
	 * Defined in the .cpp deliberately. UE_PLUGIN_NAME is a private UBT definition, correct
	 * only inside this module; a body here in a public header would resolve it to whichever
	 * plugin included the header. Nothing in this class is a second copy of the version, so
	 * there is nothing here that can drift from it.
	 */
	virtual FString GetToolsetVersion() const override;

	/**
	 * Whether TRELLIS.2 can generate right now, and what to do about it if not.
	 *
	 * Call this when a mesh generation fails on the TRELLIS provider, or before starting a batch on
	 * a machine you have not used before. It checks Docker, the container and the runner separately,
	 * because each one fails differently and each has a different fix.
	 *
	 * Takes several seconds: verifying GPU support means actually running a container.
	 *
	 * @return The state of all three layers, and one sentence saying what to fix first.
	 */
	UFUNCTION(meta = (AICallable), Category = "MeshForge|Trellis")
	static UToolCallAsyncResultTrellisStatus* GetTrellisRunnerStatus();

	/**
	 * Build the runner image if needed and start the container.
	 *
	 * **Slow, and the first time it is very slow.** The first build compiles five CUDA extensions
	 * from source and can take half an hour; the first generation afterwards downloads several
	 * gigabytes of model. Neither is a hang. Say so before starting one rather than after.
	 *
	 * @param bRebuildImage Force a rebuild. Only needed when the image exists and is wrong - a changed Dockerfile, or a TRELLIS.2 that needs updating. Leave false otherwise; a missing image is built either way.
	 * @return The runner's state once it is up.
	 */
	UFUNCTION(meta = (AICallable), Category = "MeshForge|Trellis")
	static UToolCallAsyncResultTrellisStatus* StartTrellisRunner(bool bRebuildImage);

	/**
	 * Stop the container, leaving it to be started again.
	 *
	 * Worth knowing before offering this: restarting reloads a 4B model, which takes minutes. Leave
	 * it running across a working session rather than stopping between generations.
	 *
	 * @return The runner's state once it is down.
	 */
	UFUNCTION(meta = (AICallable), Category = "MeshForge|Trellis")
	static UToolCallAsyncResultTrellisStatus* StopTrellisRunner();

	/**
	 * The tail of the runner's log.
	 *
	 * Where to look when the runner answers but generation fails: the model load error, the CUDA
	 * architecture mismatch and the sparse-convolution crash all appear here and nowhere else.
	 *
	 * @param Lines How many lines from the end. Sixty is usually enough.
	 * @return The log, most recent last.
	 */
	UFUNCTION(meta = (AICallable), Category = "MeshForge|Trellis")
	static UToolCallAsyncResultString* GetTrellisRunnerLog(int32 Lines = 60);
};
