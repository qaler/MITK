/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#ifndef QMITKXNATTREEMODEL_H
#define QMITKXNATTREEMODEL_H

// CTK includes
#include <ctkXnatTreeModel.h>

// MITK includes
#include "MitkXNATExports.h"

class MITKXNAT_EXPORT QmitkXnatTreeModel : public ctkXnatTreeModel
{
  Q_OBJECT

public:
  QmitkXnatTreeModel();

  virtual QVariant data(const QModelIndex& index, int role) const;

};

#endif // QMITKXNATTREEMODEL_H
