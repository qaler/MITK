file(GLOB_RECURSE H_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")

set(CPP_FILES
  qclickablelabel.cpp
  QmitkAboutDialog.cpp
  QmitkBasePropertyView.cpp
  QmitkBoolPropertyWidget.cpp
  QmitkBoundingObjectWidget.cpp
  QmitkCallbackFromGUIThread.cpp
  QmitkColorPropertyEditor.cpp
  QmitkColorPropertyView.cpp
  QmitkColorTransferFunctionCanvas.cpp
  QmitkCrossWidget.cpp
  QmitkEditPointDialog.cpp
  QmitkEnumerationPropertyWidget.cpp
  QmitkFFmpegWriter.cpp
  QmitkFileChooser.cpp
  QmitkGnuplotWidget.cpp
  QmitkHistogram.cpp
  QmitkHotkeyLineEdit.cpp
  QmitkModulesDialog.cpp
  QmitkModuleTableModel.cpp
  QmitkNumberPropertyEditor.cpp
  QmitkNumberPropertySlider.cpp
  QmitkNumberPropertyView.cpp
  QmitkPiecewiseFunctionCanvas.cpp
  QmitkPlotDialog.cpp
  QmitkPlotWidget.cpp
  QmitkPointListModel.cpp
  QmitkPointListView.cpp
  QmitkPointListWidget.cpp
  QmitkPrimitiveMovieNavigatorWidget.cpp
  QmitkPropertyViewFactory.cpp
  QmitkSelectableGLWidget.cpp
  QmitkSliceWidget.cpp
  QmitkSliderNavigatorWidget.cpp
  QmitkStandardViews.cpp
  QmitkStepperAdapter.cpp
  QmitkStringPropertyEditor.cpp
  QmitkStringPropertyOnDemandEdit.cpp
  QmitkStringPropertyView.cpp
  QmitkTransferFunctionCanvas.cpp
  QmitkTransferFunctionGeneratorWidget.cpp
  QmitkTransferFunctionWidget.cpp
  QmitkUGCombinedRepresentationPropertyWidget.cpp
  QmitkVideoBackground.cpp
  QtWidgetsExtRegisterClasses.cpp
)

set(MOC_H_FILES
  include/qclickablelabel.h
  include/QmitkAboutDialog.h
  include/QmitkBasePropertyView.h
  include/QmitkBoolPropertyWidget.h
  include/QmitkBoundingObjectWidget.h
  include/QmitkCallbackFromGUIThread.h
  include/QmitkColorPropertyEditor.h
  include/QmitkColorPropertyView.h
  include/QmitkColorTransferFunctionCanvas.h
  include/QmitkCrossWidget.h
  include/QmitkEditPointDialog.h
  include/QmitkEnumerationPropertyWidget.h
  include/QmitkFFmpegWriter.h
  include/QmitkFileChooser.h
  include/QmitkGnuplotWidget.h
  include/QmitkHotkeyLineEdit.h
  include/QmitkNumberPropertyEditor.h
  include/QmitkNumberPropertySlider.h
  include/QmitkNumberPropertyView.h
  include/QmitkPiecewiseFunctionCanvas.h
  include/QmitkPlotWidget.h
  include/QmitkPointListModel.h
  include/QmitkPointListView.h
  include/QmitkPointListWidget.h
  include/QmitkPrimitiveMovieNavigatorWidget.h
  include/QmitkSelectableGLWidget.h
  include/QmitkSliceWidget.h
  include/QmitkSliderNavigatorWidget.h
  include/QmitkStandardViews.h
  include/QmitkStepperAdapter.h
  include/QmitkStringPropertyEditor.h
  include/QmitkStringPropertyOnDemandEdit.h
  include/QmitkStringPropertyView.h
  include/QmitkTransferFunctionCanvas.h
  include/QmitkTransferFunctionGeneratorWidget.h
  include/QmitkTransferFunctionWidget.h
  include/QmitkUGCombinedRepresentationPropertyWidget.h
  include/QmitkVideoBackground.h
)

set(UI_FILES
  src/QmitkAboutDialogGUI.ui
  src/QmitkGnuplotWidget.ui
  src/QmitkPrimitiveMovieNavigatorWidget.ui
  src/QmitkSelectableGLWidget.ui
  src/QmitkSliceWidget.ui
  src/QmitkSliderNavigator.ui
  src/QmitkTransferFunctionGeneratorWidget.ui
  src/QmitkTransferFunctionWidget.ui
)

set(QRC_FILES
  resource/QtWidgetsExt.qrc
)
