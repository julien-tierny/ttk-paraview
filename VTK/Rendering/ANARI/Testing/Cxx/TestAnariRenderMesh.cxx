// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that we can do simple mesh rendering with ANARI
// and that VTK's many standard rendering modes (points, lines, surface, with
// a variety of color controls (actor, point, cell, texture) etc work as
// they should.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit.
//              In interactive mode it responds to the keys listed
//              vtkAnariTestInteractor.h
// -GL       => users OpenGL instead of ANARI to render
// -type N   => where N is one of 0,1,2, or 3 makes meshes consisting of
//              points, wireframes, triangles (=the default) or triangle strips
// -rep N    => where N is one of 0,1 or 2 draws the meshes as points, lines
//              or surfaces

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExtractEdges.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"
#include "vtkTexture.h"
#include "vtkTextureMapToSphere.h"
#include "vtkTransformTextureCoords.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVertexGlyphFilter.h"

#include <string>
#include <vector>

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariTestInteractor.h"

class renderable
{
public:
  vtkNew<vtkSphereSource> s;
  vtkNew<vtkPolyDataMapper> m;
  vtkNew<vtkActor> a;
};

std::unique_ptr<renderable> MakeSphereAt(
  double x, double y, double z, int res, int type, int rep, const char* name)
{
  vtkAnariTestInteractor::AddName(name);
  std::unique_ptr<renderable> ret(new renderable);
  ret->s->SetEndTheta(180); // half spheres better show variation and f and back
  ret->s->SetStartPhi(30);
  ret->s->SetEndPhi(150);
  ret->s->SetPhiResolution(res);
  ret->s->SetThetaResolution(res);
  ret->s->SetCenter(x, y, z);
  // make texture coordinate
  vtkNew<vtkTextureMapToSphere> tc;
  tc->SetCenter(x, y, z);
  tc->PreventSeamOn();
  tc->AutomaticSphereGenerationOff();
  tc->SetInputConnection(ret->s->GetOutputPort());
  vtkNew<vtkTransformTextureCoords> tt;
  tt->SetInputConnection(tc->GetOutputPort());
  // tt->SetScale(1,0.5,1);
  // make normals
  vtkNew<vtkPolyDataNormals> nl;
  nl->SetInputConnection(tt->GetOutputPort());
  nl->Update();
  // make more attribute arrays
  vtkPolyData* pd = nl->GetOutput();
  // point aligned
  vtkNew<vtkDoubleArray> da1;
  da1->SetName("testarray1");
  da1->SetNumberOfComponents(1);
  pd->GetPointData()->AddArray(da1);
  int np = pd->GetNumberOfPoints();
  int nc = pd->GetNumberOfCells();
  for (int i = 0; i < np; i++)
  {
    da1->InsertNextValue((double)i / (double)np);
  }
  vtkNew<vtkDoubleArray> da2;
  da2->SetName("testarray2");
  da2->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(da2);
  for (int i = 0; i < np; i++)
  {
    double vals[3] = { (double)i / (double)np, (double)(i * 4) / (double)np - 2.0, 42.0 };
    da2->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }

  vtkNew<vtkUnsignedCharArray> pac;
  pac->SetName("testarrayc1");
  pac->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(pac);
  for (int i = 0; i < np; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>(255 * ((double)i / (double)np)),
      static_cast<unsigned char>(255 * ((double)(i * 4) / (double)np - 2.0)), 42 };
    pac->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }

  vtkNew<vtkUnsignedCharArray> ca1;
  ca1->SetName("testarray3");
  ca1->SetNumberOfComponents(3);
  pd->GetPointData()->AddArray(ca1);
  for (int i = 0; i < np; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>((double)i / (double)np * 255),
      static_cast<unsigned char>((double)(1 - i) / (double)np), 42 };
    ca1->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  // cell aligned
  vtkNew<vtkDoubleArray> da4;
  da4->SetName("testarray4");
  da4->SetNumberOfComponents(1);
  pd->GetCellData()->AddArray(da4);
  for (int i = 0; i < pd->GetNumberOfCells(); i++)
  {
    da4->InsertNextValue((double)i / (double)pd->GetNumberOfCells());
  }
  vtkNew<vtkDoubleArray> da5;
  da5->SetName("testarray5");
  da5->SetNumberOfComponents(3);
  pd->GetCellData()->AddArray(da5);
  for (int i = 0; i < nc; i++)
  {
    double vals[3] = { (double)i / (double)nc, (double)(i * 2) / (double)nc, 42.0 };
    da5->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  vtkNew<vtkUnsignedCharArray> ca2;
  ca2->SetName("testarray6");
  ca2->SetNumberOfComponents(3);
  pd->GetCellData()->AddArray(ca2);
  for (int i = 0; i < nc; i++)
  {
    unsigned char vals[3] = { static_cast<unsigned char>((double)i / (double)np * 255),
      static_cast<unsigned char>((double)(1 - i) / (double)np), 42 };
    ca2->InsertNextTuple3(vals[0], vals[1], vals[2]);
  }
  ret->m->SetInputData(pd);

  switch (type)
  {
    case 0: // points
    {
      vtkNew<vtkVertexGlyphFilter> filter;
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
    case 1: // lines
    {
      vtkNew<vtkExtractEdges> filter;
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
    case 2: // polys
      break;
    case 3: // strips
    {
      vtkNew<vtkStripper> filter;
      filter->SetInputData(pd);
      filter->Update();
      ret->m->SetInputData(filter->GetOutput());
      break;
    }
  }
  ret->a->SetMapper(ret->m);
  ret->a->GetProperty()->SetPointSize(20.0f);
  ret->a->GetProperty()->SetLineWidth(1.0f);
  if (rep != -1)
  {
    ret->a->GetProperty()->SetRepresentation(rep);
  }
  return ret;
}

int TestAnariRenderMesh(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;
  int type = 2;
  int rep = -1;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-type"))
    {
      type = atoi(argv[i + 1]);
    }
    else if (!strcmp(argv[i], "-rep"))
    {
      rep = atoi(argv[i + 1]);
    }
    else if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  renderer->AutomaticLightCreationOn();
  renderer->SetBackground(0.75, 0.75, 0.75);
  renWin->SetSize(600, 550);
  vtkNew<vtkCamera> camera;
  camera->SetPosition(2.5, 11, -3);
  camera->SetFocalPoint(2.5, 0, -3);
  camera->SetViewUp(0, 0, 1);
  renderer->SetActiveCamera(camera);
  renWin->Render();

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, renderer);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariRenderMesh";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), renderer);
  }

  vtkAnariRendererNode::SetLibraryName("environment", renderer);
  vtkAnariRendererNode::SetSamplesPerPixel(6, renderer);
  vtkAnariRendererNode::SetLightFalloff(.5, renderer);
  vtkAnariRendererNode::SetUseDenoiser(1, renderer);
  vtkAnariRendererNode::SetCompositeOnGL(1, renderer);
  // Now, vary most of the many parameters that rendering can vary by.

  // representations points, wireframe, surface
  std::unique_ptr<renderable> ren = MakeSphereAt(5, 0, -5, 10, type, 0, "points");
  renderer->AddActor(ren->a);

  ren = MakeSphereAt(5, 0, -4, 10, 1, 1, "wireframe");
  ren->a->GetProperty()->SetColor(1, 0, 0);
  renderer->AddActor(ren->a);

  ren = MakeSphereAt(5, 0, -3, 10, type, rep, "surface");
  ren->a->GetProperty()->SetRepresentationToSurface();
  renderer->AddActor(ren->a);

  // actor color
  ren = MakeSphereAt(4, 0, -5, 10, type, rep, "actor_color");
  ren->a->GetProperty()->SetColor(0, 1, 0);
  renderer->AddActor(ren->a);

  // ambient, diffuse, and specular components
  ren = MakeSphereAt(4, 0, -4, 7, type, rep, "amb/diff/spec");
  ren->a->GetProperty()->SetAmbient(0.5);
  ren->a->GetProperty()->SetAmbientColor(0.1, 0.1, 0.3);
  ren->a->GetProperty()->SetDiffuse(0.4);
  ren->a->GetProperty()->SetDiffuseColor(0.5, 0.1, 0.1);
  ren->a->GetProperty()->SetSpecular(0.2);
  ren->a->GetProperty()->SetSpecularColor(1, 1, 1);
  ren->a->GetProperty()->SetSpecularPower(100);
  ren->a->GetProperty()->SetInterpolationToPhong();
  renderer->AddActor(ren->a);

  // opacity
  ren = MakeSphereAt(4, 0, -3, 10, type, rep, "opacity");
  ren->a->GetProperty()->SetOpacity(0.2);
  renderer->AddActor(ren->a);

  // color map cell values
  ren = MakeSphereAt(3, 0, -5, 10, type, rep, "cell_value");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(0);
  renderer->AddActor(ren->a);

  // default color component
  ren = MakeSphereAt(3, 0, -4, 10, type, rep, "cell_default_comp");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(1);
  renderer->AddActor(ren->a);

  // choose color component
  ren = MakeSphereAt(3, 0, -3, 10, type, rep, "cell_comp_1");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(1);
  ren->m->ColorByArrayComponent(1, 1); // todo, use lut since this is deprecated
  renderer->AddActor(ren->a);

  // RGB direct
  ren = MakeSphereAt(3, 0, -2, 10, type, rep, "cell_rgb");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(2);
  renderer->AddActor(ren->a);

  // RGB through LUT
  ren = MakeSphereAt(3, 0, -1, 10, type, rep, "cell_rgb_through_LUT");
  ren->m->SetScalarModeToUseCellFieldData();
  ren->m->SelectColorArray(2);
  ren->m->SetColorModeToMapScalars();
  renderer->AddActor(ren->a);

  // color map point values
  ren = MakeSphereAt(2, 0, -5, 6, type, rep, "point_value");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SelectColorArray("testarray1");
  renderer->AddActor(ren->a);

  // interpolate scalars before mapping
  ren = MakeSphereAt(2, 0, -4, 6, type, rep, "point_interp");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SelectColorArray("testarray1");
  ren->m->InterpolateScalarsBeforeMappingOn();
  renderer->AddActor(ren->a);

  // RGB direct
  ren = MakeSphereAt(2, 0, -3, 10, type, rep, "point_rgb");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SetColorModeToDefault();
  ren->m->SelectColorArray("testarrayc1");
  renderer->AddActor(ren->a);

  // RGB mapped
  ren = MakeSphereAt(2, 0, -2, 10, type, rep, "point_rgb_through_LUT");
  ren->m->SetScalarModeToUsePointFieldData();
  ren->m->SetColorModeToMapScalars();
  ren->m->SelectColorArray("testarrayc1");
  renderer->AddActor(ren->a);

  // unlit, flat, and gouraud lighting
  ren = MakeSphereAt(1, 0, -5, 7, type, rep, "not_lit");
  ren->a->GetProperty()->LightingOff();
  renderer->AddActor(ren->a);

  ren = MakeSphereAt(1, 0, -4, 7, type, rep, "flat");
  ren->a->GetProperty()->SetInterpolationToFlat();
  renderer->AddActor(ren->a);

  ren = MakeSphereAt(1, 0, -3, 7, type, rep, "gouraud");
  ren->a->GetProperty()->SetInterpolationToGouraud();
  renderer->AddActor(ren->a);

  // texture
  int maxi = 100;
  int maxj = 100;
  vtkNew<vtkImageData> texin;
  texin->SetExtent(0, maxi, 0, maxj, 0, 0);
  texin->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  vtkUnsignedCharArray* aa =
    vtkArrayDownCast<vtkUnsignedCharArray>(texin->GetPointData()->GetScalars());
  int idx = 0;
  for (int i = 0; i <= maxi; i++)
  {
    for (int j = 0; j <= maxj; j++)
    {
      bool ival = (i / 10) % 2 == 1;
      bool jval = (j / 10) % 2 == 1;
      unsigned char val = (ival ^ jval) ? 255 : 0;
      aa->SetTuple3(idx, val, val, val);
      if (j <= 3 || j >= maxj - 3)
      {
        aa->SetTuple3(idx, 255, 255, 0);
      }
      if (i <= 20 || i >= maxi - 20)
      {
        aa->SetTuple3(idx, 255, 0, 0);
      }
      idx = idx + 1;
    }
  }
  ren = MakeSphereAt(0, 0, -5, 20, type, rep, "texture");
  renderer->AddActor(ren->a);
  vtkNew<vtkTexture> texture;
  texture->SetInputData(texin);
  ren->a->SetTexture(texture);

  // imagespace positional transformations
  ren = MakeSphereAt(0, 0, -4, 10, type, rep, "transform");
  ren->a->SetScale(1.2, 1.0, 0.87);
  renderer->AddActor(ren->a);

  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkAnariTestInteractor> style;
    style->SetPipelineControlPoints(renderer, anariPass, nullptr);
    style->SetCurrentRenderer(renderer);

    iren->SetInteractorStyle(style);
    iren->Start();
  }

  return !retVal;
}
