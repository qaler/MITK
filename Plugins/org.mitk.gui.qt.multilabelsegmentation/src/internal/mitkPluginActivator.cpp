/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkPluginActivator.h"

#include "QmitkThresholdAction.h"
#include "QmitkConvertSurfaceToLabelAction.h"
#include "QmitkConvertMaskToLabelAction.h"
#include "QmitkConvertToMultiLabelSegmentationAction.h"
#include "QmitkCreateMultiLabelSegmentationAction.h"

#include "SegmentationUtilities/QmitkMultiLabelSegmentationUtilitiesView.h"

#include <usModuleInitialization.h>

ctkPluginContext* mitk::PluginActivator::m_Context = nullptr;

//MLI TODO
US_INITIALIZE_MODULE //("MultiLabelSegmentation", "liborg_mitk_gui_qt_multilabelsegmentation")

void mitk::PluginActivator::start(ctkPluginContext *context)
{
  BERRY_REGISTER_EXTENSION_CLASS(QmitkThresholdAction, context)
  BERRY_REGISTER_EXTENSION_CLASS(QmitkConvertSurfaceToLabelAction, context)
  BERRY_REGISTER_EXTENSION_CLASS(QmitkConvertMaskToLabelAction, context)
  BERRY_REGISTER_EXTENSION_CLASS(QmitkConvertToMultiLabelSegmentationAction, context)
  BERRY_REGISTER_EXTENSION_CLASS(QmitkCreateMultiLabelSegmentationAction, context)
  BERRY_REGISTER_EXTENSION_CLASS(QmitkMultiLabelSegmentationUtilitiesView, context)

  m_Context = context;
}

void mitk::PluginActivator::stop(ctkPluginContext*)
{
}

ctkPluginContext* mitk::PluginActivator::getContext()
{
  return m_Context;
}
