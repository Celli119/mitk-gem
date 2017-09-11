/**
 *  MITK-GEM: Graphcut Plugin
 *
 *  Copyright (c) 2016, Zurich University of Applied Sciences, School of Engineering, T. Fitze, Y. Pauchard
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved.
 */

#ifndef GraphcutView_h
#define GraphcutView_h

// MITK
#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include "ui_GraphcutViewControls.h"
#include <mitkLabelSetImageConverter.h>

// Utils
#include "WorkbenchUtils.h"

class GraphcutView : public QmitkAbstractView {
    Q_OBJECT

public:

    static const std::string VIEW_ID;

protected slots:
    void startButtonPressedMulti();
    void startButtonPressedBin();
    void refreshButtonPressedMulti();
    void refreshButtonPressedBin();
    void imageSelectionChangedMulti();
    void imageSelectionChangedBin();

    void workerHasStarted(unsigned int);
    void workerIsDone(itk::DataObject::Pointer, unsigned int);
    void workerProgressUpdateMulti(float progress, unsigned int);
    void workerProgressUpdateBin(float progress, unsigned int);

protected:
    virtual void CreateQtPartControl(QWidget *parent);
    virtual void SetFocus();

    // called by QmitkFunctionality when DataManager's selection has changed
    virtual void OnSelectionChanged(berry::IWorkbenchPart::Pointer source,
            const QList <mitk::DataNode::Pointer> &nodes);

    Ui::GraphcutViewControls m_Controls;

private:
    void updateMemoryRequirementsBin(double memoryRequiredInBytes);
    void updateMemoryRequirementsMulti(double memoryRequiredInBytes);
    void updateTimeEstimateMulti(long long int numberOfEdges);
    void updateTimeEstimateBin(long long int numberOfEdges);
    void initializeImageSelector(QmitkDataStorageComboBox *);
    void setMandatoryField(QWidget *, bool);
    void setWarningField(QWidget *, bool);
    void setErrorField(QWidget *, bool);
    void setQStyleSheetField(QWidget *, const char *, bool);
    bool isValidSelectionMulti();
    bool isValidSelectionBin();
    void lockGui(bool);

    unsigned int m_currentlyActiveWorkerCount;


};

#endif // GraphcutView_h
