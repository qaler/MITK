/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkUSTelemedImageSource.h"
#include "mitkUSTelemedSDKHeader.h"
#include "MITKUSTelemedScanConverterPlugin.h"
#include "mitkImageReadAccessor.h"

mitk::USTelemedImageSource::USTelemedImageSource()
  : m_Image(mitk::Image::New()),
  m_ImageMutex(new std::mutex()),
  m_Plugin(0),
  m_PluginCallback(0),
  m_UsgDataView(0),
  m_ImageProperties(0),
  m_DepthProperties(0),
  m_OldnXPelsPerUnit(0),
  m_OldnYPelsPerUnit(0)

{

}

mitk::USTelemedImageSource::~USTelemedImageSource( )
{
  SAFE_RELEASE(m_PluginCallback);
  SAFE_RELEASE(m_Plugin);
  SAFE_RELEASE(m_ImageProperties);
  SAFE_RELEASE(m_DepthProperties);
  delete m_ImageMutex;
}

void mitk::USTelemedImageSource::GetNextRawImage(std::vector<mitk::Image::Pointer>& imageVector)
{
  if (imageVector.empty() ) { imageVector.push_back( mitk::Image::New()); }

  //get the actual resolution to check if it changed. We have to do this every time because the geometry takes a few frames to adapt
  Usgfw2Lib::tagImageResolution resolutionInMetersActual;
  m_ImageProperties->GetResolution(&resolutionInMetersActual, 0);
  if (m_OldnXPelsPerUnit != resolutionInMetersActual.nXPelsPerUnit || m_OldnYPelsPerUnit != resolutionInMetersActual.nYPelsPerUnit)
    {
      //we can only update if the image exists and has a geometry
      if (m_Image.IsNotNull() && m_Image->GetGeometry() != nullptr)
     {
        m_OldnXPelsPerUnit = resolutionInMetersActual.nXPelsPerUnit;
        m_OldnYPelsPerUnit = resolutionInMetersActual.nYPelsPerUnit;
        UpdateImageGeometry();
      }

    }
  //now update image
  if ( m_Image->IsInitialized() )
  {
    m_ImageMutex->lock();

    // copy contents of the given image into the member variable
    imageVector.at(0)->Initialize(m_Image->GetPixelType(), m_Image->GetDimension(), m_Image->GetDimensions());
    mitk::ImageReadAccessor inputReadAccessor(m_Image, m_Image->GetSliceData(0,0,0));
    imageVector.at(0)->SetSlice(inputReadAccessor.GetData());
    imageVector.at(0)->SetGeometry(m_Image->GetGeometry());
    m_ImageMutex->unlock();
  }
}

void mitk::USTelemedImageSource::UpdateImageGeometry()
{
  Usgfw2Lib::tagPixelsOrigin origin = Usgfw2Lib::tagPixelsOrigin();
  Usgfw2Lib::tagImageResolution resolutionInMeters;
  m_ImageProperties->GetResolution(&resolutionInMeters,0);

  mitk::Vector3D spacing;
  spacing[0] = ((double)1 / resolutionInMeters.nXPelsPerUnit) * 1000; //conversion: meters to millimeters
  spacing[1] = ((double)1 / resolutionInMeters.nXPelsPerUnit) * 1000; //conversion: meters to millimeters
  spacing[2] = 1;

  m_ImageMutex->lock();
  if(m_Image.IsNotNull() && (m_Image->GetGeometry()!=nullptr))
    {
    m_Image->GetGeometry()->SetSpacing(spacing);
    m_Image->GetGeometry()->Modified();
    }
  else
    {MITK_WARN << "image or geometry was nullptr, can't adapt geometry";}
  m_ImageMutex->unlock();

  MITK_DEBUG << "UpdateImageGeometry called!";
  MITK_DEBUG << "depth: " << m_DepthProperties->GetCurrent();
  MITK_DEBUG << "new spacing: " << spacing;
}

bool mitk::USTelemedImageSource::CreateAndConnectConverterPlugin(Usgfw2Lib::IUsgDataView* usgDataView, Usgfw2Lib::tagScanMode scanMode)
{
  IUnknown* tmp_obj = nullptr;
  // create control object from Telemed API
  mitk::telemed::CreateUsgControl( usgDataView, Usgfw2Lib::IID_IUsgScanConverterPlugin, scanMode, 0, (void**)&tmp_obj );
  if ( ! tmp_obj )
  {
    MITK_ERROR("USImageSource")("USTelemedImageSource") << "Could not create scan converter plugin.";
    return false;
  }

  // create the callback object for the scan conversion
  if ( ! m_PluginCallback )
  {
    m_PluginCallback = new USTelemedScanConverterPlugin();

    // current image buffer should be copied to m_Image at every callback
    m_PluginCallback->SetOutputImage(m_Image.GetPointer(), m_ImageMutex);
  }
  else
  {
    // make sure that the scan converter plugin is not set
    // to the plugin callback any longer
    m_PluginCallback->SetScanConverterPlugin(0);
  }

  // now the ScanConverterPlugin can be created and set as plugin
  SAFE_RELEASE(m_Plugin);
  m_Plugin = (Usgfw2Lib::IUsgScanConverterPlugin*)tmp_obj;
  m_PluginCallback->SetScanConverterPlugin(m_Plugin);

  //last: create some connections which are needed inside this class for communication with the telemed device
  m_UsgDataView = usgDataView;

  // create telemed controls
  if (!m_DepthProperties) {CREATE_TelemedControl(m_DepthProperties, m_UsgDataView, Usgfw2Lib::IID_IUsgDepth, Usgfw2Lib::IUsgDepth, Usgfw2Lib::SCAN_MODE_B);}
  if (!m_ImageProperties) {CREATE_TelemedControl(m_ImageProperties, m_UsgDataView, Usgfw2Lib::IID_IUsgImageProperties, Usgfw2Lib::IUsgImageProperties, Usgfw2Lib::SCAN_MODE_B);}

  return true;
}
