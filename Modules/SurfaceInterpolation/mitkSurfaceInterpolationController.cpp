/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkSurfaceInterpolationController.h"
#include "mitkImageAccessByItk.h"
#include "mitkImageCast.h"
#include "mitkMemoryUtilities.h"

#include "mitkImageToSurfaceFilter.h"
//#include "vtkXMLPolyDataWriter.h"
#include "vtkPolyDataWriter.h"
#include "vtkIntArray.h"
#include <mitkIOUtil.h>

// Check whether the given contours are coplanar
bool ContoursCoplanar(mitk::SurfaceInterpolationController::ContourPositionInformation leftHandSide,
                      mitk::SurfaceInterpolationController::ContourPositionInformation rightHandSide)
{
  // Here we check two things:
  // 1. Whether the normals of both contours are at least parallel
  // 2. Whether both contours lie in the same plane

  // Check for coplanarity:
  // a. Span a vector between two points one from each contour
  // b. Calculate dot product for the vector and one of the normals
  // c. If the dot is zero the two vectors are orthogonal and the contours are coplanar

  double vec[3];
  vec[0] = leftHandSide.contourPoint[0] - rightHandSide.contourPoint[0];
  vec[1] = leftHandSide.contourPoint[1] - rightHandSide.contourPoint[1];
  vec[2] = leftHandSide.contourPoint[2] - rightHandSide.contourPoint[2];
  double n[3];
  n[0] = rightHandSide.contourNormal[0];
  n[1] = rightHandSide.contourNormal[1];
  n[2] = rightHandSide.contourNormal[2];
  double dot = vtkMath::Dot(n, vec);

  double n2[3];
  n2[0] = leftHandSide.contourNormal[0];
  n2[1] = leftHandSide.contourNormal[1];
  n2[2] = leftHandSide.contourNormal[2];

  // The normals of both contours have to be parallel but not of the same orientation
  double lengthLHS = leftHandSide.contourNormal.GetNorm();
  double lengthRHS = rightHandSide.contourNormal.GetNorm();
  double dot2 = vtkMath::Dot(n, n2);
  bool contoursParallel = mitk::Equal(fabs(lengthLHS * lengthRHS), fabs(dot2), 0.001);

  if (mitk::Equal(dot, 0.0, 0.001) && contoursParallel)
    return true;
  else
    return false;
}

mitk::SurfaceInterpolationController::ContourPositionInformation CreateContourPositionInformation(
  mitk::Surface::Pointer contour)
{
  mitk::SurfaceInterpolationController::ContourPositionInformation contourInfo;
  contourInfo.contour = contour;
  double n[3];
  double p[3];
  contour->GetVtkPolyData()->GetPoints()->GetPoint(0, p);
  vtkPolygon::ComputeNormal(contour->GetVtkPolyData()->GetPoints(), n);
  contourInfo.contourNormal = n;
  contourInfo.contourPoint = p;
  contourInfo.pos =-1;
  contourInfo.labelValue=vtkIntArray::SafeDownCast(contour->GetVtkPolyData()->GetFieldData()->GetAbstractArray(0))->GetValue(0);
  return contourInfo;
}

mitk::SurfaceInterpolationController::SurfaceInterpolationController()
  : m_SelectedSegmentation(nullptr), m_CurrentTimePoint(0.)
{
  m_DistanceImageSpacing = 0.0;
  m_ReduceFilter = ReduceContourSetFilter::New();
  m_NormalsFilter = ComputeContourSetNormalsFilter::New();
  m_InterpolateSurfaceFilter = CreateDistanceImageFromSurfaceFilter::New();
  // m_TimeSelector = ImageTimeSelector::New();

  m_ReduceFilter->SetUseProgressBar(false);
  //  m_ReduceFilter->SetProgressStepSize(1);
  m_NormalsFilter->SetUseProgressBar(true);
  m_NormalsFilter->SetProgressStepSize(1);
  m_InterpolateSurfaceFilter->SetUseProgressBar(true);
  m_InterpolateSurfaceFilter->SetProgressStepSize(7);

  m_Contours = Surface::New();

  m_PolyData = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  m_PolyData->SetPoints(points);

  m_InterpolationResult = nullptr;
  m_CurrentNumberOfReducedContours = 0;
  m_ContourIndex=0;
}

mitk::SurfaceInterpolationController::~SurfaceInterpolationController()
{
  // Removing all observers
  auto dataIter = m_SegmentationObserverTags.begin();
  for (; dataIter != m_SegmentationObserverTags.end(); ++dataIter)
  {
    (*dataIter).first->RemoveObserver((*dataIter).second);
  }
  m_SegmentationObserverTags.clear();
}

mitk::SurfaceInterpolationController *mitk::SurfaceInterpolationController::GetInstance()
{
  static mitk::SurfaceInterpolationController::Pointer m_Instance;

  if (m_Instance.IsNull())
  {
    m_Instance = SurfaceInterpolationController::New();
  }
  return m_Instance;
}

void mitk::SurfaceInterpolationController::AddNewContour(mitk::Surface::Pointer newContour)
{
  if (newContour->GetVtkPolyData()->GetNumberOfPoints() > 0)
  {
    ContourPositionInformation contourInfo = CreateContourPositionInformation(newContour);
    this->AddToInterpolationPipeline(contourInfo);
    this->Modified();
  }
}

void mitk::SurfaceInterpolationController::AddNewContours(std::vector<mitk::Surface::Pointer> newContours)
{
  for (unsigned int i = 0; i < newContours.size(); ++i)
  {
    if (newContours.at(i)->GetVtkPolyData()->GetNumberOfPoints() > 0)
    {
      ContourPositionInformation contourInfo = CreateContourPositionInformation(newContours.at(i));
      this->AddToInterpolationPipeline(contourInfo);
    }
  }
  this->Modified();
}

void mitk::SurfaceInterpolationController::AddToInterpolationPipeline(ContourPositionInformation contourInfo)
{
  if (!m_SelectedSegmentation)
  {
    return;
  }

  if (!m_SelectedSegmentation->GetTimeGeometry()->IsValidTimePoint(m_CurrentTimePoint))
  {
    MITK_ERROR << "Invalid time point requested for interpolation pipeline.";
    return;
  }

  const auto currentTimeStep = m_SelectedSegmentation->GetTimeGeometry()->TimePointToTimeStep(m_CurrentTimePoint);
  ContourPositionInformationVec2D currentContours = m_ListOfContours[m_SelectedSegmentation];
  ContourPositionInformationList currentContourList = currentContours[currentTimeStep];

  int replacementIndex(-1);
  int pos(-1);
  mitk::Surface *newContour = contourInfo.contour;

  for (unsigned int i = 0; i < currentContourList.size(); i++)
  {
    ContourPositionInformation contourFromList = currentContourList.at(i);
    bool contoursAreCoplanar=ContoursCoplanar(contourInfo, contourFromList);    
    bool contoursHaveSameLabel=contourInfo.labelValue==contourFromList.labelValue;

    //  Coplanar contours have the same "pos".
    if(contoursAreCoplanar)
    {
      pos=contourFromList.pos;
      if(contoursHaveSameLabel)
      {
        replacementIndex=i;
      }
    }
  }
  
  //  The current contour has the same label and position as the current slice
  if(replacementIndex!=-1)
  {
    contourInfo.pos=pos;
    m_ListOfContours[m_SelectedSegmentation][currentTimeStep][replacementIndex]=contourInfo;
    return;
  }

  //  Case that there is no contour in the current slice with the current label
  if(pos==-1)
  {
    pos=m_ContourIndex;
    m_ContourIndex++; //  Increment the contour index for the next time.
  }

  contourInfo.pos=pos;
  m_ListOfContours[m_SelectedSegmentation][currentTimeStep].push_back(contourInfo);  
  if (newContour->GetVtkPolyData()->GetNumberOfPoints() == 0)
  {
    this->RemoveContour(contourInfo);
  }
}

bool mitk::SurfaceInterpolationController::RemoveContour(ContourPositionInformation contourInfo)
{
  if (!m_SelectedSegmentation)
  {
    return false;
  }

  if (!m_SelectedSegmentation->GetTimeGeometry()->IsValidTimePoint(m_CurrentTimePoint))
  {
    return false;
  }

  const auto currentTimeStep = m_SelectedSegmentation->GetTimeGeometry()->TimePointToTimeStep(m_CurrentTimePoint);

  auto it = m_ListOfContours[m_SelectedSegmentation][currentTimeStep].begin();
  while (it != m_ListOfContours[m_SelectedSegmentation][currentTimeStep].end())
  {
    ContourPositionInformation currentContour = (*it);
    if (ContoursCoplanar(currentContour, contourInfo))
    {
      m_ListOfContours[m_SelectedSegmentation][currentTimeStep].erase(it);
      this->ReinitializeInterpolation();
      return true;
    }
    ++it;
  }
  return false;
}

const mitk::Surface *mitk::SurfaceInterpolationController::GetContour(ContourPositionInformation contourInfo)
{
  if (!m_SelectedSegmentation)
  {
    return nullptr;
  }

  if (!m_SelectedSegmentation->GetTimeGeometry()->IsValidTimePoint(m_CurrentTimePoint))
  {
    return nullptr;
  }
  const auto currentTimeStep = m_SelectedSegmentation->GetTimeGeometry()->TimePointToTimeStep(m_CurrentTimePoint);

  ContourPositionInformationList contourList = m_ListOfInterpolationSessions[m_SelectedSegmentation][currentTimeStep];
  for (unsigned int i = 0; i < contourList.size(); ++i)
  {
    ContourPositionInformation currentContour = contourList.at(i);
    if (ContoursCoplanar(contourInfo, currentContour))
      return currentContour.contour;
  }
  return nullptr;
}

unsigned int mitk::SurfaceInterpolationController::GetNumberOfContours()
{
  if (!m_SelectedSegmentation)
  {
    return -1;
  }

  if (!m_SelectedSegmentation->GetTimeGeometry()->IsValidTimePoint(m_CurrentTimePoint))
  {
    return -1;
  }
  const auto currentTimeStep = m_SelectedSegmentation->GetTimeGeometry()->TimePointToTimeStep(m_CurrentTimePoint);

  return m_ListOfInterpolationSessions[m_SelectedSegmentation][currentTimeStep].size();
}

void mitk::SurfaceInterpolationController::AddActiveLabelContoursForInterpolation(const mitk::Label::PixelType& activeLabel)
{
  m_ReduceFilter->Reset();
  this->ReinitializeInterpolation();
  
  if (!m_SelectedSegmentation->GetTimeGeometry()->IsValidTimePoint(m_CurrentTimePoint))
  {
    MITK_ERROR << "Invalid time point requested for interpolation pipeline.";
    return;
  }
  const auto currentTimeStep = m_SelectedSegmentation->GetTimeGeometry()->TimePointToTimeStep(m_CurrentTimePoint);
  auto currentContours=m_ListOfContours[m_SelectedSegmentation][currentTimeStep];

  for(size_t i=0;i<currentContours.size();++i)
  {
    if(currentContours[i].labelValue==activeLabel)
    { 
      m_ListOfInterpolationSessions[m_SelectedSegmentation][currentTimeStep].push_back(currentContours[i]);
      m_ReduceFilter->SetInput(m_ListOfInterpolationSessions[m_SelectedSegmentation][currentTimeStep].size()-1, currentContours[i].contour);
    }
  }
}
 

void mitk::SurfaceInterpolationController::Interpolate()
{
  if (!m_SelectedSegmentation->GetTimeGeometry()->IsValidTimePoint(m_CurrentTimePoint))
  {
    MITK_WARN << "No interpolation possible, currently selected timepoint is not in the time bounds of currently selected segmentation. Time point: " << m_CurrentTimePoint;
    m_InterpolationResult = nullptr;
    return;
  }
  const auto currentTimeStep = m_SelectedSegmentation->GetTimeGeometry()->TimePointToTimeStep(m_CurrentTimePoint);
  m_ReduceFilter->Update();
  m_CurrentNumberOfReducedContours = m_ReduceFilter->GetNumberOfOutputs();

  if (m_CurrentNumberOfReducedContours == 1)
  {
    vtkPolyData *tmp = m_ReduceFilter->GetOutput(0)->GetVtkPolyData();
    if (tmp == nullptr)
    {
      m_CurrentNumberOfReducedContours = 0;
    }
  }

  //  We use the timeSelector to get the segmentation image for the current segmentation.
  mitk::ImageTimeSelector::Pointer timeSelector = mitk::ImageTimeSelector::New();
  timeSelector->SetInput(m_SelectedSegmentation);   
  timeSelector->SetTimeNr(currentTimeStep);
  timeSelector->SetChannelNr(0);
  timeSelector->Update();

  mitk::Image::Pointer refSegImage = timeSelector->GetOutput();
  itk::ImageBase<3>::Pointer itkImage = itk::ImageBase<3>::New();
  AccessFixedDimensionByItk_1(refSegImage, GetImageBase, 3, itkImage);

  m_NormalsFilter->SetSegmentationBinaryImage(m_SelectedSegmentation);
  m_NormalsFilter->SetSegmentationBinaryImage(refSegImage);

  for (unsigned int i = 0; i < m_CurrentNumberOfReducedContours; i++)
  {
    mitk::Surface::Pointer reducedContour = m_ReduceFilter->GetOutput(i);
    reducedContour->DisconnectPipeline();
    m_NormalsFilter->SetInput(i, reducedContour);
    m_InterpolateSurfaceFilter->SetInput(i, m_NormalsFilter->GetOutput(i));
  }

  if (m_CurrentNumberOfReducedContours < 2)
  {
    // If no interpolation is possible reset the interpolation result
    m_InterpolationResult = nullptr;
    return;
  }
  

  // Setting up progress bar
  mitk::ProgressBar::GetInstance()->AddStepsToDo(10);
  mitk::Image::Pointer m_InterpolatedSurfaceOutput=m_InterpolateSurfaceFilter->GetOutput();

  // create a surface from the distance-image
  mitk::ImageToSurfaceFilter::Pointer imageToSurfaceFilter = mitk::ImageToSurfaceFilter::New();
  imageToSurfaceFilter->SetInput(m_InterpolateSurfaceFilter->GetOutput());
  imageToSurfaceFilter->SetThreshold(0);
  imageToSurfaceFilter->SetSmooth(true);
  imageToSurfaceFilter->SetSmoothIteration(1);
  imageToSurfaceFilter->Update();

  mitk::Surface::Pointer interpolationResult = mitk::Surface::New();
  interpolationResult->Expand(m_SelectedSegmentation->GetTimeSteps());

  auto geometry = m_SelectedSegmentation->GetTimeGeometry()->Clone();
  geometry->ReplaceTimeStepGeometries(mitk::Geometry3D::New());
  interpolationResult->SetTimeGeometry(geometry);

  interpolationResult->SetVtkPolyData(imageToSurfaceFilter->GetOutput()->GetVtkPolyData(), currentTimeStep);
  m_InterpolationResult = interpolationResult;

  m_DistanceImageSpacing = m_InterpolateSurfaceFilter->GetDistanceImageSpacing();

  vtkSmartPointer<vtkAppendPolyData> polyDataAppender = vtkSmartPointer<vtkAppendPolyData>::New();
  auto contours=m_ListOfInterpolationSessions[m_SelectedSegmentation][currentTimeStep];
  for (unsigned int i = 0; i < m_ListOfInterpolationSessions[m_SelectedSegmentation][currentTimeStep].size(); i++)
  {
    // std::string lbl=contours.at(i).contour->GetVtkPolyData()->GetFieldData()->GetAbstractArray(0)->GetName();
    polyDataAppender->AddInputData(m_ListOfInterpolationSessions[m_SelectedSegmentation][currentTimeStep].at(i).contour->GetVtkPolyData());
  }
  polyDataAppender->Update();
  m_Contours->SetVtkPolyData(polyDataAppender->GetOutput());

  auto* contoursGeometry = static_cast<mitk::ProportionalTimeGeometry*>(m_Contours->GetTimeGeometry());
  auto timeBounds = geometry->GetTimeBounds(currentTimeStep);
  contoursGeometry->SetFirstTimePoint(timeBounds[0]);
  contoursGeometry->SetStepDuration(timeBounds[1] - timeBounds[0]);

  // Last progress step
  mitk::ProgressBar::GetInstance()->Progress(20);

  m_InterpolationResult->DisconnectPipeline();
}
 
mitk::Surface::Pointer mitk::SurfaceInterpolationController::GetInterpolationResult()
{
  return m_InterpolationResult;
}

mitk::Surface *mitk::SurfaceInterpolationController::GetContoursAsSurface()
{
  return m_Contours;
}

void mitk::SurfaceInterpolationController::SetDataStorage(DataStorage::Pointer ds)
{
  m_DataStorage = ds;
}

void mitk::SurfaceInterpolationController::SetMinSpacing(double minSpacing)
{
  m_ReduceFilter->SetMinSpacing(minSpacing);
}

void mitk::SurfaceInterpolationController::SetMaxSpacing(double maxSpacing)
{
  m_ReduceFilter->SetMaxSpacing(maxSpacing);
  m_NormalsFilter->SetMaxSpacing(maxSpacing);
}

void mitk::SurfaceInterpolationController::SetDistanceImageVolume(unsigned int distImgVolume)
{
  m_InterpolateSurfaceFilter->SetDistanceImageVolume(distImgVolume);
}

mitk::Image::Pointer mitk::SurfaceInterpolationController::GetCurrentSegmentation()
{
  return m_SelectedSegmentation;
}

mitk::Image *mitk::SurfaceInterpolationController::GetImage()
{
  return m_InterpolateSurfaceFilter->GetOutput();
}

double mitk::SurfaceInterpolationController::EstimatePortionOfNeededMemory()
{
  double numberOfPointsAfterReduction = m_ReduceFilter->GetNumberOfPointsAfterReduction() * 3;
  double sizeOfPoints = pow(numberOfPointsAfterReduction, 2) * sizeof(double);
  double totalMem = mitk::MemoryUtilities::GetTotalSizeOfPhysicalRam();
  double percentage = sizeOfPoints / totalMem;
  return percentage;
}

unsigned int mitk::SurfaceInterpolationController::GetNumberOfInterpolationSessions()
{
  return m_ListOfInterpolationSessions.size();
}

template <typename TPixel, unsigned int VImageDimension>
void mitk::SurfaceInterpolationController::GetImageBase(itk::Image<TPixel, VImageDimension> *input,
                                                        itk::ImageBase<3>::Pointer &result)
{
  result->Graft(input);
}

void mitk::SurfaceInterpolationController::SetCurrentSegmentationInterpolationList(mitk::Image::Pointer segmentation)
{
  this->SetCurrentInterpolationSession(segmentation);
}

void mitk::SurfaceInterpolationController::SetCurrentInterpolationSession(mitk::Image::Pointer currentSegmentationImage)
{
  if (currentSegmentationImage.GetPointer() == m_SelectedSegmentation)
    return;

  if (currentSegmentationImage.IsNull())
  {
    m_SelectedSegmentation = nullptr;
    return;
  }

  m_SelectedSegmentation = currentSegmentationImage.GetPointer();

  auto it = m_ListOfContours.find(currentSegmentationImage.GetPointer());
  // If the session does not exist yet create a new ContourPositionPairList otherwise reinitialize the interpolation
  // pipeline
  if (it == m_ListOfContours.end())
  {
    ContourPositionInformationVec2D newList;
    m_ListOfContours.insert(
      std::pair<mitk::Image *, ContourPositionInformationVec2D>(m_SelectedSegmentation, newList));
    m_InterpolationResult = nullptr;
    m_CurrentNumberOfReducedContours = 0;

    itk::MemberCommand<SurfaceInterpolationController>::Pointer command =
      itk::MemberCommand<SurfaceInterpolationController>::New();
    command->SetCallbackFunction(this, &SurfaceInterpolationController::OnSegmentationDeleted);
    m_SegmentationObserverTags.insert(std::pair<mitk::Image *, unsigned long>(
      m_SelectedSegmentation, m_SelectedSegmentation->AddObserver(itk::DeleteEvent(), command)));
  }

  it=m_ListOfInterpolationSessions.find(currentSegmentationImage.GetPointer());
  if (it == m_ListOfInterpolationSessions.end())
  {
    ContourPositionInformationVec2D newList;
    m_ListOfInterpolationSessions.insert(
      std::pair<mitk::Image *, ContourPositionInformationVec2D>(m_SelectedSegmentation, newList));
    m_InterpolationResult = nullptr;
    m_CurrentNumberOfReducedContours = 0;

    itk::MemberCommand<SurfaceInterpolationController>::Pointer command =
      itk::MemberCommand<SurfaceInterpolationController>::New();
    command->SetCallbackFunction(this, &SurfaceInterpolationController::OnSegmentationDeleted);
    m_SegmentationObserverTags.insert(std::pair<mitk::Image *, unsigned long>(
      m_SelectedSegmentation, m_SelectedSegmentation->AddObserver(itk::DeleteEvent(), command)));
  }  


  this->ReinitializeInterpolation();
}

bool mitk::SurfaceInterpolationController::ReplaceInterpolationSession(mitk::Image::Pointer oldSession,
                                                                       mitk::Image::Pointer newSession)
{
  if (oldSession.IsNull() || newSession.IsNull())
    return false;

  if (oldSession.GetPointer() == newSession.GetPointer())
    return false;

  if (!mitk::Equal(*(oldSession->GetGeometry()), *(newSession->GetGeometry()), mitk::eps, false))
    return false;

  auto it = m_ListOfInterpolationSessions.find(oldSession.GetPointer());

  if (it == m_ListOfInterpolationSessions.end())
    return false;

  if (!newSession->GetTimeGeometry()->IsValidTimePoint(m_CurrentTimePoint))
  {
    MITK_WARN << "Interpolation session cannot be replaced. Currently selected timepoint is not in the time bounds of the new session. Time point: " << m_CurrentTimePoint;
    return false;
  }

  ContourPositionInformationVec2D oldList = (*it).second;
  m_ListOfInterpolationSessions.insert(
    std::pair<mitk::Image *, ContourPositionInformationVec2D>(newSession.GetPointer(), oldList));
  itk::MemberCommand<SurfaceInterpolationController>::Pointer command =
    itk::MemberCommand<SurfaceInterpolationController>::New();
  command->SetCallbackFunction(this, &SurfaceInterpolationController::OnSegmentationDeleted);
  m_SegmentationObserverTags.insert(
    std::pair<mitk::Image *, unsigned long>(newSession, newSession->AddObserver(itk::DeleteEvent(), command)));

  if (m_SelectedSegmentation == oldSession)
    m_SelectedSegmentation = newSession;

  const auto currentTimeStep = m_SelectedSegmentation->GetTimeGeometry()->TimePointToTimeStep(m_CurrentTimePoint);

  mitk::ImageTimeSelector::Pointer timeSelector = mitk::ImageTimeSelector::New();
  timeSelector->SetInput(m_SelectedSegmentation);
  timeSelector->SetTimeNr(currentTimeStep);
  timeSelector->SetChannelNr(0);
  timeSelector->Update();
  mitk::Image::Pointer refSegImage = timeSelector->GetOutput();
  m_NormalsFilter->SetSegmentationBinaryImage(refSegImage);

  this->RemoveInterpolationSession(oldSession);
  return true;
}

void mitk::SurfaceInterpolationController::RemoveSegmentationFromContourList(mitk::Image *segmentation)
{
  this->RemoveInterpolationSession(segmentation);
}

void mitk::SurfaceInterpolationController::RemoveInterpolationSession(mitk::Image::Pointer segmentationImage)
{
  if (segmentationImage)
  {
    if (m_SelectedSegmentation == segmentationImage)
    {
      m_NormalsFilter->SetSegmentationBinaryImage(nullptr);
      m_SelectedSegmentation = nullptr;
    }
    m_ListOfInterpolationSessions.erase(segmentationImage);
    // Remove observer
    auto pos = m_SegmentationObserverTags.find(segmentationImage);
    if (pos != m_SegmentationObserverTags.end())
    {
      segmentationImage->RemoveObserver((*pos).second);
      m_SegmentationObserverTags.erase(pos);
    }
  }
}

void mitk::SurfaceInterpolationController::RemoveAllInterpolationSessions()
{
  // Removing all observers
  auto dataIter = m_SegmentationObserverTags.begin();
  while (dataIter != m_SegmentationObserverTags.end())
  {
    mitk::Image *image = (*dataIter).first;
    image->RemoveObserver((*dataIter).second);
    ++dataIter;
  }

  m_SegmentationObserverTags.clear();
  m_SelectedSegmentation = nullptr;
  m_ListOfInterpolationSessions.clear();
}

void mitk::SurfaceInterpolationController::ReinitializeInterpolation(mitk::Surface::Pointer contours)
{
  // 1. detect coplanar contours
  // 2. merge coplanar contours into a single surface
  // 4. add contour to pipeline

  // Split the surface into separate polygons
  vtkSmartPointer<vtkCellArray> existingPolys;
  vtkSmartPointer<vtkPoints> existingPoints;
  existingPolys = contours->GetVtkPolyData()->GetPolys();
  existingPoints = contours->GetVtkPolyData()->GetPoints();
  existingPolys->InitTraversal();

  vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();

  typedef std::pair<mitk::Vector3D, mitk::Point3D> PointNormalPair;
  std::vector<ContourPositionInformation> list;
  std::vector<vtkSmartPointer<vtkPoints>> pointsList;
  int count(0);

  //  existingPolys->GetNextCell(ids) --> ids contains the ids for the points of this poly
  //  
  for (existingPolys->InitTraversal(); existingPolys->GetNextCell(ids);)
  {
    // Get the points
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    //  points from the current polygon are extracted
    existingPoints->GetPoints(ids, points);
    ++count;
    pointsList.push_back(points);

    //  Compute the normal
    PointNormalPair p_n;
    double n[3];
    vtkPolygon::ComputeNormal(points, n);
    p_n.first = n;
    double p[3];

    //  p contains point information
    existingPoints->GetPoint(ids->GetId(0), p);
    p_n.second = p;

    ContourPositionInformation p_info;
    p_info.contourNormal = n;
    p_info.contourPoint = p;
    list.push_back(p_info);
    continue;
  }

  // Detect and sort coplanar polygons
  auto outer = list.begin();
  std::vector<std::vector<vtkSmartPointer<vtkPoints>>> relatedPoints;
  while (outer != list.end())
  {
    auto inner = outer;
    ++inner;
    std::vector<vtkSmartPointer<vtkPoints>> rel;
    auto pointsIter = pointsList.begin();
    rel.push_back((*pointsIter));
    pointsIter = pointsList.erase(pointsIter);

    while (inner != list.end())
    {
      if (ContoursCoplanar((*outer), (*inner)))
      {
        inner = list.erase(inner);
        rel.push_back((*pointsIter));
        pointsIter = pointsList.erase(pointsIter);
      }
      else
      {
        ++inner;
        ++pointsIter;
      }
    }
    relatedPoints.push_back(rel);
    ++outer;
  }

  // Build the separate surfaces again
  std::vector<mitk::Surface::Pointer> finalSurfaces;
  for (unsigned int i = 0; i < relatedPoints.size(); ++i)
  {
    vtkSmartPointer<vtkPolyData> contourSurface = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> polygons = vtkSmartPointer<vtkCellArray>::New();
    unsigned int pointId(0);
    for (unsigned int j = 0; j < relatedPoints.at(i).size(); ++j)
    {
      unsigned int numPoints = relatedPoints.at(i).at(j)->GetNumberOfPoints();
      vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();
      polygon->GetPointIds()->SetNumberOfIds(numPoints);
      polygon->GetPoints()->SetNumberOfPoints(numPoints);
      vtkSmartPointer<vtkPoints> currentPoints = relatedPoints.at(i).at(j);
      for (unsigned k = 0; k < numPoints; ++k)
      {
        points->InsertPoint(pointId, currentPoints->GetPoint(k));
        polygon->GetPointIds()->SetId(k, pointId);
        ++pointId;
      }
      polygons->InsertNextCell(polygon);
    }
    contourSurface->SetPoints(points);
    contourSurface->SetPolys(polygons);
    contourSurface->BuildLinks();
    mitk::Surface::Pointer surface = mitk::Surface::New();
    surface->SetVtkPolyData(contourSurface);
    finalSurfaces.push_back(surface);
  }

  // Add detected contours to interpolation pipeline
  this->AddNewContours(finalSurfaces);
}

void mitk::SurfaceInterpolationController::OnSegmentationDeleted(const itk::Object *caller,
                                                                 const itk::EventObject & /*event*/)
{
  auto *tempImage = dynamic_cast<mitk::Image *>(const_cast<itk::Object *>(caller));
  if (tempImage)
  {
    if (m_SelectedSegmentation == tempImage)
    {
      m_NormalsFilter->SetSegmentationBinaryImage(nullptr);
      m_SelectedSegmentation = nullptr;
    }
    m_SegmentationObserverTags.erase(tempImage);
    m_ListOfInterpolationSessions.erase(tempImage);
  }
}

void mitk::SurfaceInterpolationController::ReinitializeInterpolation()
{
  // If session has changed reset the pipeline
  m_ReduceFilter->Reset();
  m_NormalsFilter->Reset();
  m_InterpolateSurfaceFilter->Reset();
  // m_ContourIndex=0;
  //  Empty out the listOfInterpolationSessions
  m_ListOfInterpolationSessions[m_SelectedSegmentation].clear();

  itk::ImageBase<3>::Pointer itkImage = itk::ImageBase<3>::New();

  if (m_SelectedSegmentation)
  {
    if (!m_SelectedSegmentation->GetTimeGeometry()->IsValidTimePoint(m_CurrentTimePoint))
    {
      MITK_WARN << "Interpolation cannot be reinitialized. Currently selected timepoint is not in the time bounds of the currently selected segmentation. Time point: " << m_CurrentTimePoint;
      return;
    }
    const auto currentTimeStep = m_SelectedSegmentation->GetTimeGeometry()->TimePointToTimeStep(m_CurrentTimePoint);

    //  Set reference image for interpolation surface filter
    mitk::ImageTimeSelector::Pointer timeSelector = mitk::ImageTimeSelector::New();
    timeSelector->SetInput(m_SelectedSegmentation);
    timeSelector->SetTimeNr(currentTimeStep);
    timeSelector->SetChannelNr(0);
    timeSelector->Update();
    mitk::Image::Pointer refSegImage = timeSelector->GetOutput();
    AccessFixedDimensionByItk_1(refSegImage, GetImageBase, 3, itkImage);
    m_InterpolateSurfaceFilter->SetReferenceImage(itkImage.GetPointer());

    //  Resize listofinterpolationsessions and listofcontours to numTimeSteps
    unsigned int numTimeSteps = m_SelectedSegmentation->GetTimeSteps();
    unsigned int size = m_ListOfInterpolationSessions[m_SelectedSegmentation].size();
    if (size != numTimeSteps)
    {
      m_ListOfInterpolationSessions[m_SelectedSegmentation].resize(numTimeSteps);
      m_ListOfContours[m_SelectedSegmentation].resize(numTimeSteps);
    }
  }
}
