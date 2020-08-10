/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "TractSegTool3D.h"
#include <usGetModuleContext.h>
#include <usModuleResource.h>

namespace mitk
{
  MITK_TOOL_MACRO(MITKDEEPLEARNINGSEGMENTATION_EXPORT, TractSegTool3D, "TractSeg tool");
}

mitk::TractSegTool3D::TractSegTool3D()
    : DeepLearningSegmentationTool("tract_seg", "in_image", "tractseg.py","segList", DeepLearningSegmentationTool::MITKImage, true)
{
}

mitk::TractSegTool3D::~TractSegTool3D() {
}

us::ModuleResource mitk::TractSegTool3D::GetIconResource() const {
  auto moduleContext = us::GetModuleContext();
  auto module = moduleContext->GetModule();
  auto resource = module->GetResource("tract_seg.svg");
  return resource;
}

const char *mitk::TractSegTool3D::GetName() const
{
  return "Tract Segmentation";
}

