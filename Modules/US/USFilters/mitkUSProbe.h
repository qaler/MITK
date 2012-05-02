/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#ifndef MITKUSProbe_H_HEADER_INCLUDED_
#define MITKUSProbe_H_HEADER_INCLUDED_

#include <MitkUSExports.h>
#include <mitkCommon.h>
#include <itkObject.h>
#include <itkObjectFactory.h>

namespace mitk {

    /**Documentation
    * \brief Right now, the US Probe is only a fancy name for a string. Later, it could handle probe specific parameters
    *  like the current frequency etc. It is able to compare itself to other probes for device managment thoiugh.
    *  Be sure to check the isEqualTo() method if you expand this class to see if it needs work.
    * \ingroup US
    */
  class MitkUS_EXPORT USProbe : public itk::Object
    {
    public:
      mitkClassMacro(USProbe,itk::Object);
      itkNewMacro(Self);

      /**
      * \brief Compares this probe to another probe and returns true if they are equal in terms of name AND NAME ONLY
      *  be sure to sufficiently extend this method along with further capabilities probes
      */
      bool IsEqualToProbe(mitk::USProbe::Pointer probe);

    
      //## getter and setter ##

      itkGetMacro(Name, std::string);
      itkSetMacro(Name, std::string);

    protected:
      USProbe();
      virtual ~USProbe();

      std::string m_Name;

    

    };
} // namespace mitk
#endif