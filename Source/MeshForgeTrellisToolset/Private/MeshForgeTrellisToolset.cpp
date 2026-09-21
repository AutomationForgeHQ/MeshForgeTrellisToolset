#include "MeshForgeTrellisToolset.h"

#include "MeshForgeTrellis.h"
#include "TrellisDocker.h"
#include "TrellisEditorSettings.h"
#include "TrellisProvider.h"

#include "Async/Async.h"
#include "ToolsetRegistry/ToolCallAsyncResultString.h"
#include "Interfaces/IPluginManager.h"

namespace TrellisToolsetPrivate
{
	/**
	 * Fill in everything Docker knows. Blocking; only ever called off the game thread.
	 */
	static FTrellisRunnerStatus DescribeDocker()
	{
		const FTrellisDockerStatus Docker = FTrellisDocker::Check();

		FTrellisRunnerStatus Status;
		Status.DockerVersion = Docker.Version;
		Status.bDockerInstalled = Docker.State != ETrellisDockerState::NotInstalled;
		Status.bDockerRunning = Docker.State != ETrellisDockerState::NotInstalled
			&& Docker.State != ETrellisDockerState::NotRunning;
		Status.bDockerGpuSupport = Docker.State == ETrellisDockerState::Ready;
		Status.bContainerExists = Docker.bContainerExists;
		Status.bContainerRunning = Docker.bContainerRunning;
		Status.Advice = Docker.Advice;

		return Status;
	}

	/**
	 * Add what the runner itself says, then complete the result.
	 *
	 * Split from DescribeDocker because it has to happen on the game thread: the provider's HTTP
	 * call completes there, and reading its cached state from a worker thread would race.
	 */
	static void CompleteWithRunner(UToolCallAsyncResultTrellisStatus* Result, FTrellisRunnerStatus Status)
	{
		AsyncTask(ENamedThreads::GameThread, [Result, Status]() mutable
		{
			Status.RunnerUrl = GetDefault<UTrellisEditorSettings>()->RunnerUrl;

			FMeshForgeTrellisModule* Module = FMeshForgeTrellisModule::GetPtr();
			TSharedPtr<FTrellisProvider> Provider = Module ? Module->GetProvider() : nullptr;

			if (!Provider.IsValid())
			{
				Status.RunnerMessage = TEXT("The TRELLIS provider is not loaded.");
				Result->SetValue(Status);
				Result->RemoveFromRoot();
				return;
			}

			Provider->TestConnection([Result, Status](bool bSuccess, const FString& Message) mutable
			{
				Status.bRunnerAnswering = bSuccess;
				Status.RunnerMessage = Message;
				Status.bReadyToGenerate = bSuccess;

				if (!bSuccess && Status.Advice.IsEmpty())
				{
					Status.Advice = Message;
				}

				// Only when something is actually wrong.
				//
				// A runner that answers is a runner that can generate, and it does not have to be
				// the container on this machine - pointing at a GPU on somebody else's desk is a
				// supported setup and the whole reason the address is a setting. Running the ladder
				// anyway told somebody with a perfectly good remote runner that "the runner has
				// never been built", directly under a line saying it was ready to generate.
				if (Status.bReadyToGenerate)
				{
					Status.Advice.Empty();
					Result->SetValue(Status);
					Result->RemoveFromRoot();
					return;
				}

				// The most useful thing this tool does: say which layer to fix, in the order the
				// layers have to be fixed in. Reporting all three states without ranking them leaves
				// the agent to guess, and it guesses badly.
				if (!Status.bDockerInstalled)
				{
					Status.Advice = TEXT("Docker is not installed or is not on PATH. Install Docker Desktop and restart the editor.");
				}
				else if (!Status.bDockerRunning)
				{
					Status.Advice = TEXT("Docker Desktop is not running. Start it, then call this tool again.");
				}
				else if (!Status.bDockerGpuSupport)
				{
					Status.Advice = TEXT("Docker cannot reach a GPU. On Windows that needs WSL2 and a current NVIDIA driver. TRELLIS.2 on a CPU is unusably slow.");
				}
				else if (!Status.bContainerExists)
				{
					Status.Advice = TEXT("The runner has never been built. Call Start Trellis Runner - the first build compiles five CUDA extensions and takes a long time.");
				}
				else if (!Status.bContainerRunning)
				{
					Status.Advice = TEXT("The runner container is stopped. Call Start Trellis Runner.");
				}

				Result->SetValue(Status);
				Result->RemoveFromRoot();
			});
		});
	}
}

UToolCallAsyncResultTrellisStatus* UMeshForgeTrellisToolset::GetTrellisRunnerStatus()
{
	UToolCallAsyncResultTrellisStatus* Result = NewObject<UToolCallAsyncResultTrellisStatus>();

	// Held against the GC while worker threads own it; released on every path in CompleteWithRunner.
	Result->AddToRoot();

	Async(EAsyncExecution::Thread, [Result]
	{
		TrellisToolsetPrivate::CompleteWithRunner(Result, TrellisToolsetPrivate::DescribeDocker());
	});

	return Result;
}

UToolCallAsyncResultTrellisStatus* UMeshForgeTrellisToolset::StartTrellisRunner(bool bRebuildImage)
{
	UToolCallAsyncResultTrellisStatus* Result = NewObject<UToolCallAsyncResultTrellisStatus>();
	Result->AddToRoot();

	Async(EAsyncExecution::Thread, [Result, bRebuildImage]
	{
		FString Error;

		// Two hours. A first build compiles nvdiffrast, nvdiffrec, CuMesh, FlexGEMM and o-voxel from
		// source; on a machine with few cores that is genuinely this slow, and a shorter deadline
		// would kill a build that was going to succeed.
		if (!FTrellisDocker::ComposeUp(7200, bRebuildImage, Error))
		{
			AsyncTask(ENamedThreads::GameThread, [Result, Error]
			{
				Result->SetError(FString::Printf(
					TEXT("Could not start the runner: %s Call Get Trellis Runner Status to see which "
						 "layer is at fault, or Get Trellis Runner Log for the build output."),
					*Error));
				Result->RemoveFromRoot();
			});
			return;
		}

		TrellisToolsetPrivate::CompleteWithRunner(Result, TrellisToolsetPrivate::DescribeDocker());
	});

	return Result;
}

UToolCallAsyncResultTrellisStatus* UMeshForgeTrellisToolset::StopTrellisRunner()
{
	UToolCallAsyncResultTrellisStatus* Result = NewObject<UToolCallAsyncResultTrellisStatus>();
	Result->AddToRoot();

	Async(EAsyncExecution::Thread, [Result]
	{
		FString Error;

		if (!FTrellisDocker::ComposeStop(180, Error))
		{
			AsyncTask(ENamedThreads::GameThread, [Result, Error]
			{
				Result->SetError(FString::Printf(TEXT("Could not stop the runner: %s"), *Error));
				Result->RemoveFromRoot();
			});
			return;
		}

		TrellisToolsetPrivate::CompleteWithRunner(Result, TrellisToolsetPrivate::DescribeDocker());
	});

	return Result;
}

UToolCallAsyncResultString* UMeshForgeTrellisToolset::GetTrellisRunnerLog(int32 Lines)
{
	UToolCallAsyncResultString* Result = NewObject<UToolCallAsyncResultString>();
	Result->AddToRoot();

	const int32 Wanted = FMath::Clamp(Lines, 1, 2000);

	Async(EAsyncExecution::Thread, [Result, Wanted]
	{
		const FString Log = FTrellisDocker::ComposeLogs(Wanted);

		AsyncTask(ENamedThreads::GameThread, [Result, Log]
		{
			if (Log.IsEmpty())
			{
				Result->SetError(
					TEXT("The runner has no log. If it has never been built, call Start Trellis Runner."));
			}
			else
			{
				Result->SetValue(Log);
			}

			Result->RemoveFromRoot();
		});
	});

	return Result;
}

FString UMeshForgeTrellisToolset::GetToolsetVersion() const
{
	// The descriptor is the version. Reading it here rather than repeating it means there is no
	// second copy to keep true - and no window, between a bump and a fix, where an agent is told
	// a number the package does not carry.
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT(UE_PLUGIN_NAME));
	return Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : FString();
}
