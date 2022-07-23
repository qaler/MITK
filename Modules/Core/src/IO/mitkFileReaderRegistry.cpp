/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkFileReaderRegistry.h"

#include "mitkCoreServices.h"
#include "mitkIMimeTypeProvider.h"

// Microservices
#include <usGetModuleContext.h>
#include <usLDAPProp.h>
#include <usModuleContext.h>
#include <usServiceProperties.h>

#include "itksys/SystemTools.hxx"

mitk::FileReaderRegistry::FileReaderRegistry()
{
}

mitk::FileReaderRegistry::~FileReaderRegistry()
{
  for (auto &elem : m_ServiceObjects)
  {
    elem.second.UngetService(elem.first);
  }
}

mitk::MimeType mitk::FileReaderRegistry::GetMimeTypeForFile(const std::string &path, us::ModuleContext *context)
{
  if (path.empty())
  {
    mitkThrow() << "FileReaderRegistry::GetMimeTypeForFile was called with empty path. Returning empty MimeType, "
                   "please report this error to the developers.";
  }

  mitk::CoreServicePointer<mitk::IMimeTypeProvider> mimeTypeProvider(mitk::CoreServices::GetMimeTypeProvider(context));
  std::vector<MimeType> mimeTypes = mimeTypeProvider->GetMimeTypesForFile(path);
  if (mimeTypes.empty())
  {
    return MimeType();
  }
  else
  {
    return mimeTypes.front();
  }
}

std::vector<mitk::FileReaderRegistry::ReaderReference> mitk::FileReaderRegistry::GetReferences(
  const MimeType &mimeType, us::ModuleContext *context)
{
  if (context == nullptr)
    context = us::GetModuleContext();

  std::string filter = us::LDAPProp(us::ServiceConstants::OBJECTCLASS()) == us_service_interface_iid<IFileReader>() &&
                       us::LDAPProp(IFileReader::PROP_MIMETYPE()) == mimeType.GetName();
  std::cout << "File Type: " << filter << std::endl;
  if (1 && filter == "(&(objectclass=org.mitk.IFileReader)(org.mitk.IFileIO.mimetype=application/vnd.mitk.image.dicom))")
  {
    std::cout << "LabMRI: Default loader for image.dicom would crash! Using default dcm service now..." << std::endl;
    filter = "(&(objectclass=org.mitk.IFileReader)(org.mitk.IFileIO.mimetype=application/vnd.mitk.image.dcm))";
  }
  auto result = context->GetServiceReferences<IFileReader>(filter);
  std::cout <<  "GetServiceReferences result: " << result.size() << std::endl;
  if (result.size() == 0)
  {
    std::cout << "LabMRI: File reader service not found! Using default dcm service now..." << std::endl;
    filter = "(&(objectclass=org.mitk.IFileReader)(org.mitk.IFileIO.mimetype=application/vnd.mitk.image.dcm))";
    return context->GetServiceReferences<IFileReader>(filter);
  }
  return context->GetServiceReferences<IFileReader>(filter);
}

mitk::IFileReader *mitk::FileReaderRegistry::GetReader(const mitk::FileReaderRegistry::ReaderReference &ref,
                                                       us::ModuleContext *context)
{
  if (context == nullptr)
    context = us::GetModuleContext();

  us::ServiceObjects<mitk::IFileReader> serviceObjects = context->GetServiceObjects(ref);
  mitk::IFileReader *reader = serviceObjects.GetService();
  m_ServiceObjects.insert(std::make_pair(reader, serviceObjects));
  return reader;
}

std::vector<mitk::IFileReader *> mitk::FileReaderRegistry::GetReaders(const MimeType &mimeType,
                                                                      us::ModuleContext *context)
{
  if (context == nullptr)
    context = us::GetModuleContext();

  std::vector<mitk::IFileReader *> result;

  if (!mimeType.IsValid())
    return result;

  std::vector<us::ServiceReference<IFileReader>> refs = GetReferences(mimeType, context);
  std::sort(refs.begin(), refs.end());

  result.reserve(refs.size());

  // Translate List of ServiceRefs to List of Pointers
  for (std::vector<us::ServiceReference<IFileReader>>::const_reverse_iterator iter = refs.rbegin(), end = refs.rend();
       iter != end;
       ++iter)
  {
    us::ServiceObjects<mitk::IFileReader> serviceObjects = context->GetServiceObjects(*iter);
    mitk::IFileReader *reader = serviceObjects.GetService();
    m_ServiceObjects.insert(std::make_pair(reader, serviceObjects));
    result.push_back(reader);
  }

  return result;
}

void mitk::FileReaderRegistry::UngetReader(mitk::IFileReader *reader)
{
  auto readerIter =
    m_ServiceObjects.find(reader);
  if (readerIter != m_ServiceObjects.end())
  {
    readerIter->second.UngetService(reader);
    m_ServiceObjects.erase(readerIter);
  }
}

void mitk::FileReaderRegistry::UngetReaders(const std::vector<mitk::IFileReader *> &readers)
{
  for (const auto &reader : readers)
  {
    this->UngetReader(reader);
  }
}
