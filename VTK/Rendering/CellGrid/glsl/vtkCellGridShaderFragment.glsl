//VTK::System::Dec
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//VTK::Camera::Dec

//VTK::Output::Dec

// Position of vertex in view coordinates.
//VTK::PositionVC::Dec

// The normal of the output primitive in view coordinates.
//VTK::Normal::Dec

// Depth Peeling Support
//VTK::DepthPeeling::Dec

// handle coincident offsets
//VTK::Coincident::Dec

// Value raster
//VTK::ValuePass::Dec

// Lights
//VTK::Light::Dec

smooth in vec3 pcoordVSOutput;
#if {UsesGeometryShaders}
// distance of fragment from origin in patch coordinate system.
smooth in vec3 patchDistanceGSOutput;
// distance of fragment from origin in parametric coordinate system
// of the tessellated output primitive.
smooth in vec3 primDistanceGSOutput;
#endif

flat in int cellIdVSOutput;

uniform sampler2D color_map;
uniform vec3 color_range;
uniform int color_component;

// The indices of all DOFs for all cells of this shape.
uniform highp isamplerBuffer shape_conn;
// The (x,y,z) points of each DOF in the entire mesh.
uniform samplerBuffer shape_vals;
// We may need this if the color and shape have different interpolation orders
// **and** the color attribute is continuous (i.e., shares coefficients at cell boundaries).
uniform highp isamplerBuffer color_conn;
// The coefficient of each basis function at each integration point in each cell.
uniform samplerBuffer color_vals;

// basic material shading capabilities
uniform vec3 color_ambient;
uniform vec3 color_diffuse;
uniform vec3 color_specular;
uniform vec3 color_ambient_bf; // backface
uniform vec3 color_diffuse_bf; // backface
uniform vec3 color_specular_bf; // backface
uniform float intensity_ambient;
uniform float intensity_diffuse;
uniform float intensity_specular;
uniform float intensity_opacity;
uniform float intensity_ambient_bf; // backface
uniform float intensity_diffuse_bf; // backface
uniform float intensity_specular_bf; // backface
uniform float intensity_opacity_bf; // backface
uniform float power_specular;
uniform float power_specular_bf; // backface
uniform int enable_specular;

// debugging
uniform int color_override_type;

{commonDefs}
{cellEval}
{cellUtil}

#if {UsesTessellationShaders}
float amplify(float d, float scale, float offset)
{{
  d = scale * d + offset;
  d = 1 - exp2(-2*d*d);
  d = clamp(d, 0, 1);
  return d;
}}
#endif

void main()
{{
  //VTK::PositionVC::Impl

  // Place any calls that require uniform flow (e.g. dFdx) here.
  //VTK::UniformFlow::Impl

  // Set gl_FragDepth here (gl_FragCoord.z by default)
  //VTK::Depth::Impl

  // Early depth peeling abort:
  //VTK::DepthPeeling::PreColor

  float colorValues[{ColorCoeffPerCell}];
  float fieldValue[{ColorNumValPP}];
  float scalar; // The non-normalized scalar value computed from a fieldValue tuple.
  vec2 texCoord; // Used for color lookup.

  if ({HaveColors})
  {{
    float shapeValues[{ShapeCoeffPerCell}];
    shapeValuesForCell(cellIdVSOutput, shapeValues);

    if ({ColorContinuous})
    {{
      // Continuous (shared) field values
      for (int ii = 0; ii < {ColorNumBasisFun}; ++ii)
      {{
        int dofId = texelFetchBuffer(color_conn, cellIdVSOutput * {ColorNumBasisFun} + ii).s;
        for (int jj = 0; jj < {ColorMultiplicity}; ++jj)
        {{
          colorValues[ii * {ColorMultiplicity} + jj] = texelFetchBuffer(color_vals, dofId * {ColorMultiplicity} + jj).x;
        }}
      }}
    }}
    else
    {{
      // Discontinuous field values
      for (int ii = 0; ii < {ColorNumBasisFun}; ++ii)
      {{
        for (int jj = 0; jj < {ColorMultiplicity}; ++jj)
        {{
          int colorValTBIdx = (cellIdVSOutput * {ColorNumBasisFun} + ii) * {ColorMultiplicity} + jj;
          int colorValVSIdx = ii * {ColorMultiplicity} + jj;
          colorValues[colorValVSIdx] = texelFetchBuffer(color_vals, colorValTBIdx).x;
        }}
      }}
    }}

    // Evaluate the basis function at this fragment's parametric coords (pcoordVSOutput),
    // yielding the \a fieldValue tuple.
    colorEvaluateAt(pcoordVSOutput, shapeValues, colorValues, fieldValue);

    // Choose how we map the \a fieldValue tuple into a texture-map coordinate.
    switch (color_component)
    {{
    case -2:
      // L₂ norm:
      {{
        float mag = 0.0;
        for (int cc = 0; cc < {ColorNumValPP}; ++cc)
        {{
          mag += fieldValue[cc] * fieldValue[cc];
        }}
        scalar = sqrt(mag);
      }}
      break;
    case -1:
      // L₁ norm (choose the maximum across components):
      {{
        scalar = fieldValue[0];
        for (int cc = 1; cc < {ColorNumValPP}; ++cc)
        {{
          if (fieldValue[cc] > scalar)
          {{
            scalar = fieldValue[cc];
          }}
        }}
      }}
      break;
    default:
      // Choose a single component.
      scalar = fieldValue[color_component];
    }}
    if (color_range[2] > 0.0)
    {{
      texCoord = vec2((scalar - color_range[0]) / color_range[2], 0.0);
    }}
    else
    {{
      texCoord = vec2((scalar - color_range[0]), 0.0);
    }}
    // texCoord = vec2(colorValues[3], 0.);
  }}

  vec4 color;
  vec3 ambientColor;
  vec3 diffuseColor;
  vec3 specularColor;
  float opacity;
  float specularPower = 0.0f;
  if ({HaveColors})
  {{
    vec3 pcoord = pcoordVSOutput;
    switch (color_override_type)
    {{
      case ScalarVisualizationOverride_R:
        color = texture(color_map, vec2(pcoord.x * 0.5 + 0.5, 0.0));
        break;
      case ScalarVisualizationOverride_S:
        color = texture(color_map, vec2(pcoord.y * 0.5 + 0.5, 0.0));
        break;
      case ScalarVisualizationOverride_T:
        color = texture(color_map, vec2(pcoord.z * 0.5 + 0.5, 0.0));
        break;
      case ScalarVisualizationOverride_L2_NORM_R_S:
        color = texture(color_map, vec2(length(pcoord.xy) * 0.707106f, 0.0));
        break;
      case ScalarVisualizationOverride_L2_NORM_S_T:
        color = texture(color_map, vec2(length(pcoord.yz) * 0.707106f, 0.0));
        break;
      case ScalarVisualizationOverride_L2_NORM_T_R:
        color = texture(color_map, vec2(length(pcoord.zx) * 0.707106f, 0.0));
        break;
      case ScalarVisualizationOverride_NONE:
      default:
        color = texture(color_map, texCoord);
        break;
    }}
    // color = vec4((colorValues[0] - color_range[0])/color_range[2], (colorValues[1] - color_range[0])/color_range[2], (colorValues[2] - color_range[0])/color_range[2], 1.0);
    // color = vec4(texCoord.s, texCoord.s, texCoord.s, 1.0);
    ambientColor = intensity_ambient * color.rgb;
    diffuseColor = intensity_diffuse * color.rgb;
    specularColor = intensity_specular * color.rgb;
    opacity = intensity_opacity * color.a;
  }}
  else
  {{
    // FIXME: gl_FrontFacing cannot be relied upon for some reason.
    //        1. Dual depth peeling works and sometimes doesn't.
    //        2. Backfaces are not rendered.
    // if (gl_FrontFacing == false)
    // {{
    //   ambientColor = intensity_ambient_bf * color_ambient_bf;
    //   diffuseColor = intensity_diffuse_bf * color_diffuse_bf;
    //   specularColor = intensity_specular_bf * color_specular_bf;
    //   opacity = intensity_opacity_bf;
    //   specularPower = power_specular_bf;
    // }}
    // else
    {{
      ambientColor = intensity_ambient * color_ambient;
      diffuseColor = intensity_diffuse * color_diffuse;
      specularColor = intensity_specular * color_specular;
      opacity = intensity_opacity;
      specularPower = power_specular;
    }}
  }}

  //VTK::Normal::Impl

  //VTK::Light::Impl

  // Discard fully transparent pixels
  if (gl_FragData[0].a <= 0.0) discard;

#if {UsesGeometryShaders}
#if {GSOutputMaxVertices} == 3
  // minimum distance from all primitive edges.
  float d1 = min(min(primDistanceGSOutput.x, primDistanceGSOutput.y), primDistanceGSOutput.z);
#if {PatchSize} == 3
  // minimum distance from all patch edges.
  float d2 = min(min(patchDistanceGSOutput.x, patchDistanceGSOutput.y), patchDistanceGSOutput.z);
#elif {PatchSize} == 4
  // minimum distance from all patch edges.
  float d2 = min(patchDistanceGSOutput.x, patchDistanceGSOutput.y);
#endif
  gl_FragData[0].rgb *= amplify(d1, 40, -0.5) * amplify(d2, 60, -0.5);
#elif {GSOutputMaxVertices} == 2
  // minimum distance from two end points of primitive.
  float d1 = primDistanceGSOutput.x;
  // minimum distance from two end points of patch.
  float d2 = patchDistanceGSOutput.x;
  gl_FragData[0].rgb *= amplify(d1, 40, -0.5) * amplify(d2, 60, -0.5);
#endif
#endif
  //VTK::DepthPeeling::Impl
}}
