// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUComputePipeline.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputeRenderBuffer);

//------------------------------------------------------------------------------
vtkWebGPUComputeRenderBuffer::vtkWebGPUComputeRenderBuffer() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputeRenderBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "wgpuBuffer: " << this->wgpuBuffer.Get() << std::endl;

  os << indent << "Point buffer attribute: " << this->PointBufferAttribute << std::endl;
  os << indent << "Cell buffer attribute: " << this->CellBufferAttribute << std::endl;

  os << indent << "RenderUniformsGroup: " << this->RenderUniformsGroup << std::endl;
  os << indent << "RenderUniformsBinding: " << this->RenderUniformsBinding << std::endl;
  os << indent << "RenderBufferOffset: " << this->RenderBufferOffset << std::endl;
  os << indent << "RenderBufferElementCount: " << this->RenderBufferElementCount << std::endl;

  os << indent << "Associated pipeline: ";
  this->AssociatedPipeline->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeRenderBuffer::SetAssociatedPipeline(
  vtkWeakPointer<vtkWebGPUComputePipeline> pipeline)
{
  if (this->AssociatedPipeline != nullptr)
  {
    vtkLog(ERROR,
      "This buffer is already associated to a pipeline. A buffer cannot be used in two "
      "pipelines.");
  }

  this->AssociatedPipeline = pipeline;
}

VTK_ABI_NAMESPACE_END
