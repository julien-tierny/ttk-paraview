// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test multiplies element-wise two vectors and adds a multiplier given as a uniform in the
 * process.
 */

#include <numeric> // For std::iota
#include <vector>

#include "TestComputePipelineUniformShader.h"
#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePipeline.h"

namespace
{
constexpr int DATA_SIZE = 128;
}

using OutputDataType = float;

int TestComputePipelineUniform(int argc, char** argv)
{
  // This first vector will be using a vtkDataArray as its data source
  vtkNew<vtkIntArray> inputVector1Values;
  inputVector1Values->SetNumberOfComponents(1);
  inputVector1Values->Allocate(::DATA_SIZE);
  for (int i = 0; i < ::DATA_SIZE; i++)
  {
    inputVector1Values->InsertNextValue(i);
  }

  // The second vector uses a std::vector. The two vectors use different types (they are not both
  // std::vector / vtkDataArray) for testing purposes as both std::vector and vtkDataArray are
  // supposed to be supported by the compute pipeline
  std::vector<float> inputVector2Values(::DATA_SIZE);
  std::iota(inputVector2Values.begin(), inputVector2Values.end(), 0.0f);

  // Creating the input buffer to the compute shader
  vtkNew<vtkWebGPUComputeBuffer> inputValues1Buffer;
  inputValues1Buffer->SetLabel("First input buffer");
  inputValues1Buffer->SetGroup(0);
  inputValues1Buffer->SetBinding(0);
  inputValues1Buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputValues1Buffer->SetData(inputVector1Values);
  inputValues1Buffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

  // Creating the second input buffer for the compute shader
  vtkNew<vtkWebGPUComputeBuffer> inputValues2Buffer;
  inputValues2Buffer->SetLabel("Second input buffer");
  inputValues2Buffer->SetGroup(0);
  inputValues2Buffer->SetBinding(1);
  inputValues2Buffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  inputValues2Buffer->SetData(inputVector2Values);
  inputValues2Buffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  // Creating a buffer for the additional uniform
  float myUniform = 2.5f;
  std::vector<float> multiplierUniform = { myUniform };
  vtkNew<vtkWebGPUComputeBuffer> uniformBuffer;
  uniformBuffer->SetLabel("Uniform buffer");
  uniformBuffer->SetGroup(0);
  uniformBuffer->SetBinding(2);
  uniformBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
  uniformBuffer->SetData(multiplierUniform);
  uniformBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  // Creating the output buffer to the compute shader
  vtkNew<vtkWebGPUComputeBuffer> outputBuffer;
  outputBuffer->SetLabel("Output buffer");
  outputBuffer->SetGroup(0);
  outputBuffer->SetBinding(3);
  outputBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputBuffer->SetByteSize(::DATA_SIZE * sizeof(OutputDataType));

  // Creating the compute pipeline
  vtkNew<vtkWebGPUComputePipeline> multiplyCompute;
  multiplyCompute->SetLabel("Multiply vectors compute pipeline");
  multiplyCompute->SetShaderSource(TestComputePipelineUniformShader);
  multiplyCompute->SetShaderEntryPoint("computeFunction");
  multiplyCompute->AddBuffer(inputValues1Buffer);
  multiplyCompute->AddBuffer(inputValues2Buffer);
  multiplyCompute->AddBuffer(uniformBuffer);
  // Getting the index of the output buffer for later mapping with ReadBufferFromGPU()
  int outputBufferIndex = multiplyCompute->AddBuffer(outputBuffer);

  // Dispatching the compute with
  int nbXGroups = std::ceil(::DATA_SIZE / 32.0f);
  multiplyCompute->SetWorkgroups(nbXGroups, 1, 1);
  multiplyCompute->Dispatch();

  // Output buffer for the result data
  std::vector<OutputDataType> outputData;
  outputData.resize(::DATA_SIZE);

  auto onBufferMapped = [](const void* mappedData, void* userdata) {
    std::vector<OutputDataType>* out = reinterpret_cast<std::vector<OutputDataType>*>(userdata);
    vtkIdType elementCount = out->size();

    const OutputDataType* mappedDataTyped = static_cast<const OutputDataType*>(mappedData);
    for (int i = 0; i < elementCount; i++)
    {
      (*out)[i] = mappedDataTyped[i];
    }
  };

  // Mapping the buffer on the CPU to get the results from the GPU
  multiplyCompute->ReadBufferFromGPU(outputBufferIndex, onBufferMapped, &outputData);
  // Update() to actually execute WebGPU commands. Without this, the compute shader won't execute
  // and the data that we try to map here may not be available yet
  multiplyCompute->Update();

  for (int i = 0; i < ::DATA_SIZE; i++)
  {
    auto input1Value = inputVector1Values->GetValue(i);
    auto input2Value = inputVector2Values[i];

    // The compute shader is expected to multiply both inputs
    if (input1Value * input2Value * myUniform != outputData[i])
    {
      vtkLog(ERROR, "Incorrect result from the mapped buffer");

      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
