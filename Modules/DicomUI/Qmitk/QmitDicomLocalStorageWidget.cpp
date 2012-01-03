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

// Qmitk
#include "QmitkDicomLocalStorageWidget.h"

// Qt
#include <QMessageBox>



const std::string QmitkDicomLocalStorageWidget::Widget_ID = "org.mitk.Widgets.QmitkDicomLocalStorageWidget";

QmitkDicomLocalStorageWidget::QmitkDicomLocalStorageWidget(QWidget *parent)
:  m_Controls( 0 )
{
   CreateQtPartControl(this);
}

QmitkDicomLocalStorageWidget::~QmitkDicomLocalStorageWidget()
{
}


void QmitkDicomLocalStorageWidget::CreateQtPartControl( QWidget *parent )
{
   // build up qt Widget, unless already done
   if ( !m_Controls )
   {
      // create GUI widgets from the Qt Designer's .ui file
       m_Controls = new Ui::QmitkDicomLocalStorage;
      m_Controls->setupUi( parent );

   }
}

void QmitkDicomLocalStorageWidget::Initialize()
{

}

