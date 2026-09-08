# MeshForge TRELLIS.2 Toolset

Exposes the [TRELLIS.2 runner](https://kovati.dev/plugins/meshforge/) to agents through the Unreal toolset registry.

**Version 0.0.2. Experimental.**

An adapter and nothing more; deleting it changes nothing about the provider.

## What is here, and what is not

**Generation is not here.** A mesh definition pointed at the TRELLIS provider goes through
MeshForge's own tools like any other — that is the point of the provider abstraction. An agent that
knows how to generate a mesh should not have to learn a second way to do it because the model
happens to be running on this machine.

What is here is everything specific to *hosting* a model yourself: the container, the device it got,
and the reason it is not working. That last one matters most. Local inference fails for a handful of
mundane reasons — Docker closed, no GPU passthrough, kernels built for the wrong card — and every
one is fixable in seconds by somebody who is told which it is.

| | |
|---|---|
| `GetTrellisRunnerStatus` | Docker, the container and the runner, checked separately, with one sentence saying which to fix first. |
| `StartTrellisRunner` | Build if needed, and start. Slow the first time; say so before starting one. |
| `StopTrellisRunner` | Stop it. |
| `GetTrellisRunnerLog` | Where the model-load error, the CUDA architecture mismatch and the sparse-convolution crash all appear. |

## Deliberately absent

**No tool removes the container or its volumes.** The model cache is several gigabytes that took a
long time to download, and an agent tidying up should not be able to cost somebody that. A person
can do it with `docker compose down -v`.
