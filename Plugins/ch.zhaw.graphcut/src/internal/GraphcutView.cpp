/**
 *  MITK-GEM: Graphcut Plugin
 *
 *  Copyright (c) 2016, Zurich University of Applied Sciences, School of Engineering, T. Fitze, Y. Pauchard
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved.
 */

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include <QmitkDataStorageComboBox.h>
#include "GraphcutView.h"

// MITK
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateOr.h>
#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkNodePredicateNot.h>
#include <mitkTimeGeometry.h>

// Qt
#include <QThreadPool>
#include <QMessageBox>

// Graphcut
#include "lib/GraphCut3D/ImageGraphCut3DFilter.h"
#include "GraphcutWorker.h"
#include "GraphcutWorkerMulti.h"
#include "GraphcutWorkerBin.h"


const std::string GraphcutView::VIEW_ID = "org.mitk.views.imagegraphcut3dsegmentation";

void GraphcutView::SetFocus() {
}

void GraphcutView::CreateQtPartControl(QWidget *parent) {
    // create GUI widgets from the Qt Designer's .ui file
    m_Controls.setupUi(parent);

    // init image selectors

    initializeImageSelector(m_Controls.greyscaleImageSelectorMulti);
    initializeImageSelector(m_Controls.foregroundImageSelectorMulti);

    initializeImageSelector(m_Controls.greyscaleImageSelectorBin);
    initializeImageSelector(m_Controls.foregroundImageSelectorBin);
    initializeImageSelector(m_Controls.backgroundImageSelectorBin);

    // set predicates to filter which images are selectable
    m_Controls.greyscaleImageSelectorMulti->SetPredicate(WorkbenchUtils::createIsImageTypePredicate());
    m_Controls.foregroundImageSelectorMulti->SetPredicate(WorkbenchUtils::createIsBinaryImageTypePredicate());

    m_Controls.greyscaleImageSelectorBin->SetPredicate(WorkbenchUtils::createIsImageTypePredicate());
    m_Controls.foregroundImageSelectorBin->SetPredicate(WorkbenchUtils::createIsBinaryImageTypePredicate());
    m_Controls.backgroundImageSelectorBin->SetPredicate(WorkbenchUtils::createIsBinaryImageTypePredicate());

    // setup signals
    connect(m_Controls.startButtonMulti, SIGNAL(clicked()), this, SLOT(startButtonPressedMulti()));
    connect(m_Controls.startButtonBin, SIGNAL(clicked()), this, SLOT(startButtonPressedBin()));

    connect(m_Controls.refreshTimeButtonMulti, SIGNAL(clicked()), this, SLOT(refreshButtonPressedMulti()));
    connect(m_Controls.refreshTimeButtonBin, SIGNAL(clicked()), this, SLOT(refreshButtonPressedBin()));

    connect(m_Controls.refreshMemoryButtonMulti, SIGNAL(clicked()), this, SLOT(refreshButtonPressedMulti()));
    connect(m_Controls.refreshMemoryButtonBin, SIGNAL(clicked()), this, SLOT(refreshButtonPressedBin()));

    connect(m_Controls.greyscaleImageSelectorMulti, SIGNAL(OnSelectionChanged (const mitk::DataNode *)), this, SLOT(imageSelectionChangedMulti()));
    connect(m_Controls.greyscaleImageSelectorBin, SIGNAL(OnSelectionChanged (const mitk::DataNode *)), this, SLOT(imageSelectionChangedBin()));

    connect(m_Controls.foregroundImageSelectorMulti, SIGNAL(OnSelectionChanged (const mitk::DataNode *)), this, SLOT(imageSelectionChangedMulti()));
    connect(m_Controls.foregroundImageSelectorBin, SIGNAL(OnSelectionChanged (const mitk::DataNode *)), this, SLOT(imageSelectionChangedBin()));

    connect(m_Controls.backgroundImageSelectorBin, SIGNAL(OnSelectionChanged (const mitk::DataNode *)), this, SLOT(imageSelectionChangedBin()));

    // init default state
    m_currentlyActiveWorkerCount = 0;
    lockGui(false);
}

void GraphcutView::OnSelectionChanged(berry::IWorkbenchPart::Pointer, const QList <mitk::DataNode::Pointer> &) {
    MITK_DEBUG("ch.zhaw.graphcut") << "selection changed";
}

void GraphcutView::startButtonPressedMulti() {
    MITK_INFO("ch.zhaw.graphcut") << "start button pressed";

    if (isValidSelectionMulti()) {
        MITK_INFO("ch.zhaw.graphcut") << "processing input";

        // get the nodes
        mitk::DataNode *greyscaleImageNode = m_Controls.greyscaleImageSelectorMulti->GetSelectedNode();
        mitk::DataNode *foregroundMaskNode = m_Controls.foregroundImageSelectorMulti->GetSelectedNode();

        // gather input images
        mitk::Image::Pointer greyscaleImage = dynamic_cast<mitk::Image *>(greyscaleImageNode->GetData());
        mitk::Image::Pointer foregroundMask = dynamic_cast<mitk::Image *>(foregroundMaskNode->GetData());

        // create worker. QThreadPool will take care of the deconstruction of the worker once it has finished
        MITK_INFO("ch.zhaw.graphcut") << "create the worker";
        GraphcutWorker *worker = new GraphcutWorkerMulti();

        // cast the images to ITK
        MITK_INFO("ch.zhaw.graphcut") << "cast the images to ITK";
        GraphcutWorker::InputImageType::Pointer greyscaleImageItk;
        GraphcutWorker::MaskImageType::Pointer foregroundMaskItk;
        mitk::CastToItkImage(greyscaleImage, greyscaleImageItk);
        mitk::CastToItkImage(foregroundMask, foregroundMaskItk);

        // set images in worker
        MITK_INFO("ch.zhaw.graphcut") << "init worker";
        worker->setInputImage(greyscaleImageItk);
        worker->setForegroundMask(foregroundMaskItk);

        // set parameters
        worker->setSigma(m_Controls.paramSigmaSpinBoxMulti->value());

        // set up signals
        MITK_INFO("ch.zhaw.graphcut") << "register signals";
        qRegisterMetaType<itk::DataObject::Pointer>("itk::DataObject::Pointer");
        QObject::connect(worker, SIGNAL(started(unsigned int)), this, SLOT(workerHasStarted(unsigned int)));
        QObject::connect(worker, SIGNAL(finished(itk::DataObject::Pointer, unsigned int)), this, SLOT(workerIsDone(itk::DataObject::Pointer, unsigned int)));
        QObject::connect(worker, SIGNAL(progress(float, unsigned int)), this, SLOT(workerProgressUpdateMulti(float, unsigned int)));

        // prepare the progress bar
        MITK_INFO("ch.zhaw.graphcut") << "prepare GUI";
        m_Controls.progressBarMulti->setValue(0);
        m_Controls.progressBarMulti->setMinimum(0);
        m_Controls.progressBarMulti->setMaximum(100);

        MITK_INFO("ch.zhaw.graphcut") << "start the worker";
        QThreadPool::globalInstance()->start(worker, QThread::HighestPriority);
    }
}


void GraphcutView::startButtonPressedBin() {
    MITK_INFO("ch.zhaw.graphcut") << "start button pressed";

    if (isValidSelectionBin()) {
        MITK_INFO("ch.zhaw.graphcut") << "processing input";

        // get the nodes
        mitk::DataNode *greyscaleImageNode = m_Controls.greyscaleImageSelectorBin->GetSelectedNode();
        mitk::DataNode *foregroundMaskNode = m_Controls.foregroundImageSelectorBin->GetSelectedNode();
        mitk::DataNode *backgroundMaskNode = m_Controls.backgroundImageSelectorBin->GetSelectedNode();

        // gather input images
        mitk::Image::Pointer greyscaleImage = dynamic_cast<mitk::Image *>(greyscaleImageNode->GetData());
        mitk::Image::Pointer foregroundMask = dynamic_cast<mitk::Image *>(foregroundMaskNode->GetData());
        mitk::Image::Pointer backgroundMask = dynamic_cast<mitk::Image *>(backgroundMaskNode->GetData());

        // create worker. QThreadPool will take care of the deconstruction of the worker once it has finished
        MITK_INFO("ch.zhaw.graphcut") << "create the worker";
        GraphcutWorkerBin *worker = new GraphcutWorkerBin();

        // cast the images to ITK
        MITK_INFO("ch.zhaw.graphcut") << "cast the images to ITK";
        GraphcutWorker::InputImageType::Pointer greyscaleImageItk;
        GraphcutWorker::MaskImageType::Pointer foregroundMaskItk;
        GraphcutWorker::MaskImageType::Pointer backgroundMaskItk;
        mitk::CastToItkImage(greyscaleImage, greyscaleImageItk);
        mitk::CastToItkImage(foregroundMask, foregroundMaskItk);
        mitk::CastToItkImage(backgroundMask, backgroundMaskItk);

        // set images in worker
        MITK_INFO("ch.zhaw.graphcut") << "init worker";
        worker->setInputImage(greyscaleImageItk);
        worker->setForegroundMask(foregroundMaskItk);
        worker->setBackgroundMask(backgroundMaskItk);

        // set parameters
        worker->setSigma(m_Controls.paramSigmaSpinBoxBin->value());
        worker->setBoundaryDirection((GraphcutWorker::BoundaryDirection) m_Controls.paramBoundaryDirectionComboBox->currentIndex());
        worker->setForegroundPixelValue(m_Controls.paramLabelValueSpinBox->value());

        // set up signals
        MITK_INFO("ch.zhaw.graphcut") << "register signals";
        qRegisterMetaType<itk::DataObject::Pointer>("itk::DataObject::Pointer");
        QObject::connect(worker, SIGNAL(started(unsigned int)), this, SLOT(workerHasStarted(unsigned int)));
        QObject::connect(worker, SIGNAL(finished(itk::DataObject::Pointer, unsigned int)), this, SLOT(workerIsDone(itk::DataObject::Pointer, unsigned int)));
        QObject::connect(worker, SIGNAL(progress(float, unsigned int)), this, SLOT(workerProgressUpdateBin(float, unsigned int)));

        // prepare the progress bar
        MITK_INFO("ch.zhaw.graphcut") << "prepare GUI";
        m_Controls.progressBarBin->setValue(0);
        m_Controls.progressBarBin->setMinimum(0);
        m_Controls.progressBarBin->setMaximum(100);

        MITK_INFO("ch.zhaw.graphcut") << "start the worker";
        QThreadPool::globalInstance()->start(worker, QThread::HighestPriority);
    }
}

void GraphcutView::workerHasStarted(unsigned int workerId) {
    MITK_DEBUG("ch.zhaw.graphcut") << "worker " << workerId << " started";
    m_currentlyActiveWorkerCount++;
    lockGui(true);
}

void GraphcutView::workerIsDone(itk::DataObject::Pointer data, unsigned int workerId){
    MITK_DEBUG("ch.zhaw.graphcut") << "worker " << workerId << " finished";

    // cast the image back to mitk
    GraphcutWorker::OutputImageType *resultImageItk = dynamic_cast<GraphcutWorker::OutputImageType *>(data.GetPointer());
    mitk::Image::Pointer resultImage = mitk::GrabItkImageMemory(resultImageItk, nullptr, nullptr, false);

    // create the node and store the result
    mitk::DataNode::Pointer newNode = mitk::DataNode::New();
    mitk::LabelSetImage::Pointer lsImage = mitk::LabelSetImage::New();
    lsImage->InitializeByLabeledImage(resultImage);
    newNode->SetData(lsImage);

    // set some node properties
    //newNode->SetProperty("boolean", mitk::BoolProperty::New(true));

    newNode->SetProperty("labelset.contour.active", mitk::BoolProperty::New(true));
    newNode->SetProperty("name", mitk::StringProperty::New("graphcut segmentation"));
    newNode->SetProperty("volumerendering", mitk::BoolProperty::New(true));
    newNode->SetProperty("layer", mitk::IntProperty::New(1));
    newNode->SetProperty("opacity", mitk::FloatProperty::New(0.5));
    // add result to the storage
    this->GetDataStorage()->Add( newNode );

    // update gui
    if(--m_currentlyActiveWorkerCount == 0){ // no more active workers
        lockGui(false);
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void GraphcutView::imageSelectionChangedMulti() {
    MITK_DEBUG("ch.zhaw.graphcut") << "selector changed image";

    // estimate required memory and computation time
    mitk::DataNode *greyscaleImageNode = m_Controls.greyscaleImageSelectorMulti->GetSelectedNode();
    if(greyscaleImageNode){
        // numberOfVertices is straightforward
        mitk::Image::Pointer greyscaleImage = dynamic_cast<mitk::Image *>(greyscaleImageNode->GetData());
        auto x = greyscaleImage->GetDimension(0);
        auto y = greyscaleImage->GetDimension(1);
        auto z = greyscaleImage->GetDimension(2);
        auto numberOfVertices = x*y*z;

        // numberOfEdges are a bit more tricky
        auto numberOfEdges = 3; // 3 because we're using a 6-connected neighborhood which gives us 3 edges / pixel
        numberOfEdges = (numberOfEdges * x) - 1;
        numberOfEdges = (numberOfEdges * y) - x;
        numberOfEdges = (numberOfEdges * z) - x * y;
        numberOfEdges *= 2; // because kolmogorov adds 2 directed edges instead of 1 bidirectional

        // the input image will be cast to short
        auto itkImageSizeInMemory = numberOfVertices * sizeof(short);

        // both mask are cast to unsigned chars
        itkImageSizeInMemory += (2 * numberOfVertices * sizeof(unsigned char));

        // node struct is 48byte, arc is 28byte as defined by Kolmogorov max flow v3.0.03
        auto memoryRequiredInBytes = numberOfVertices * 48 + numberOfEdges * 28 + itkImageSizeInMemory;

        MITK_INFO("ch.zhaw.graphcut") << "Image has " << numberOfVertices << " vertices and " <<  numberOfEdges << " edges";

        updateMemoryRequirementsMulti(memoryRequiredInBytes);
        updateTimeEstimateMulti(numberOfEdges);
    }
}

void GraphcutView::imageSelectionChangedBin() {
    MITK_DEBUG("ch.zhaw.graphcut") << "selector changed image";

    // estimate required memory and computation time
    mitk::DataNode *greyscaleImageNode = m_Controls.greyscaleImageSelectorBin->GetSelectedNode();
    if(greyscaleImageNode){
        // numberOfVertices is straightforward
        mitk::Image::Pointer greyscaleImage = dynamic_cast<mitk::Image *>(greyscaleImageNode->GetData());
        auto x = greyscaleImage->GetDimension(0);
        auto y = greyscaleImage->GetDimension(1);
        auto z = greyscaleImage->GetDimension(2);
        auto numberOfVertices = x*y*z;

        // numberOfEdges are a bit more tricky
        auto numberOfEdges = 3; // 3 because we're using a 6-connected neighborhood which gives us 3 edges / pixel
        numberOfEdges = (numberOfEdges * x) - 1;
        numberOfEdges = (numberOfEdges * y) - x;
        numberOfEdges = (numberOfEdges * z) - x * y;
        numberOfEdges *= 2; // because kolmogorov adds 2 directed edges instead of 1 bidirectional

        // the input image will be cast to short
        auto itkImageSizeInMemory = numberOfVertices * sizeof(short);

        // both mask are cast to unsigned chars
        itkImageSizeInMemory += (2 * numberOfVertices * sizeof(unsigned char));

        // node struct is 48byte, arc is 28byte as defined by Kolmogorov max flow v3.0.03
        auto memoryRequiredInBytes = numberOfVertices * 48 + numberOfEdges * 28 + itkImageSizeInMemory;

        MITK_INFO("ch.zhaw.graphcut") << "Image has " << numberOfVertices << " vertices and " <<  numberOfEdges << " edges";

        updateMemoryRequirementsBin(memoryRequiredInBytes);
        updateTimeEstimateBin(numberOfEdges);
    }
}

void GraphcutView::updateMemoryRequirementsMulti(double memoryRequiredInBytes){
    QString memory = QString::number(memoryRequiredInBytes / 1024.0 / 1024.0, 'f', 0);
    memory.append("MB");
    m_Controls.estimatedMemoryMulti->setText(memory);
    if(memoryRequiredInBytes > 4096000000){
        setErrorField(m_Controls.estimatedMemoryMulti, true);
    } else if(memoryRequiredInBytes > 2048000000){
        setErrorField(m_Controls.estimatedMemoryMulti, false);
        setWarningField(m_Controls.estimatedMemoryMulti, true);
    } else{
        setErrorField(m_Controls.estimatedMemoryMulti, false);
        setWarningField(m_Controls.estimatedMemoryMulti, false);
    }
    MITK_INFO("ch.zhaw.graphcut") <<  "Representing the full graph will require " << memoryRequiredInBytes << " Bytes of memory to compute.";
}

void GraphcutView::updateMemoryRequirementsBin(double memoryRequiredInBytes){
    QString memory = QString::number(memoryRequiredInBytes / 1024.0 / 1024.0, 'f', 0);
    memory.append("MB");
    m_Controls.estimatedMemoryBin->setText(memory);
    if(memoryRequiredInBytes > 4096000000){
        setErrorField(m_Controls.estimatedMemoryBin, true);
    } else if(memoryRequiredInBytes > 2048000000){
        setErrorField(m_Controls.estimatedMemoryBin, false);
        setWarningField(m_Controls.estimatedMemoryBin, true);
    } else{
        setErrorField(m_Controls.estimatedMemoryBin, false);
        setWarningField(m_Controls.estimatedMemoryBin, false);
    }
    MITK_INFO("ch.zhaw.graphcut") <<  "Representing the full graph will require " << memoryRequiredInBytes << " Bytes of memory to compute.";
}

void GraphcutView::updateTimeEstimateMulti(long long numberOfEdges){
    // trendlines based on dataset of 50 images with incremental sizes calculated on a 32GB machine

    // graph init / reading results. linear
    // y = c0*x + c1
    double c0 = 2.0e-07;
    double c1 = 0.1148;
    double x = numberOfEdges;
    double estimatedSetupAndBreakdownTimeInSeconds = c0*x + c1;

    // the max flow computation.
    // c0*x^(c1)
    c0 = 2.0e-18;
    c1 = 2.4;
    x = numberOfEdges;

    double estimatedComputeTimeInSeconds = c0*pow(x, c1);
    double estimateInSeconds;

    // max flow on < 30mega edges has a irregular time complexity and is thus excluded from the trendline
    if(numberOfEdges < 30000000){
        // max flow is very (<0.03s) fast in this range
        estimateInSeconds = estimatedSetupAndBreakdownTimeInSeconds;
    } else{
        estimateInSeconds = estimatedSetupAndBreakdownTimeInSeconds + estimatedComputeTimeInSeconds * 2; // * 2 because the estimation is off anyways. better estimate pessimistically
    }

    QString time = QString::number(estimateInSeconds, 'f', 2);
    time.append("s");
    m_Controls.estimatedTimeMulti->setText(time);
    if(estimateInSeconds > 60){
        setErrorField(m_Controls.estimatedTimeMulti, true);
    } else if (estimateInSeconds > 30) {
        setErrorField(m_Controls.estimatedTimeMulti, false);
        setWarningField(m_Controls.estimatedTimeMulti, true);
    }else {
        setErrorField(m_Controls.estimatedTimeMulti, false);
        setWarningField(m_Controls.estimatedTimeMulti, false);
    }

    MITK_INFO("ch.zhaw.graphcut") << "Graphcut computation will take about " << estimateInSeconds << " seconds.";
}


void GraphcutView::updateTimeEstimateBin(long long numberOfEdges){
    // trendlines based on dataset of 50 images with incremental sizes calculated on a 32GB machine

    // graph init / reading results. linear
    // y = c0*x + c1
    double c0 = 2.0e-07;
    double c1 = 0.1148;
    double x = numberOfEdges;
    double estimatedSetupAndBreakdownTimeInSeconds = c0*x + c1;

    // the max flow computation.
    // c0*x^(c1)
    c0 = 2.0e-18;
    c1 = 2.4;
    x = numberOfEdges;

    double estimatedComputeTimeInSeconds = c0*pow(x, c1);
    double estimateInSeconds;

    // max flow on < 30mega edges has a irregular time complexity and is thus excluded from the trendline
    if(numberOfEdges < 30000000){
        // max flow is very (<0.03s) fast in this range
        estimateInSeconds = estimatedSetupAndBreakdownTimeInSeconds;
    } else{
        estimateInSeconds = estimatedSetupAndBreakdownTimeInSeconds + estimatedComputeTimeInSeconds * 2; // * 2 because the estimation is off anyways. better estimate pessimistically
    }

    QString time = QString::number(estimateInSeconds, 'f', 2);
    time.append("s");
    m_Controls.estimatedTimeBin->setText(time);
    if(estimateInSeconds > 60){
        setErrorField(m_Controls.estimatedTimeBin, true);
    } else if (estimateInSeconds > 30) {
        setErrorField(m_Controls.estimatedTimeBin, false);
        setWarningField(m_Controls.estimatedTimeBin, true);
    }else {
        setErrorField(m_Controls.estimatedTimeBin, false);
        setWarningField(m_Controls.estimatedTimeBin, false);
    }

    MITK_INFO("ch.zhaw.graphcut") << "Graphcut computation will take about " << estimateInSeconds << " seconds.";
}

void GraphcutView::initializeImageSelector(QmitkDataStorageComboBox *selector){
    selector->SetDataStorage(this->GetDataStorage());
    selector->SetAutoSelectNewItems(false);
}

void GraphcutView::setMandatoryField(QWidget *widget, bool bEnabled){
    setQStyleSheetField(widget, "mandatoryField", bEnabled);
}

void GraphcutView::setWarningField(QWidget *widget, bool bEnabled){
    setQStyleSheetField(widget, "warningField", bEnabled);
}

void GraphcutView::setErrorField(QWidget *widget, bool bEnabled){
    setQStyleSheetField(widget, "errorField", bEnabled);
}

void GraphcutView::setQStyleSheetField(QWidget *widget, const char *fieldName, bool bEnabled){
    widget->setProperty(fieldName, bEnabled);
    widget->style()->unpolish(widget); // need to do this since we changed the stylesheet
    widget->style()->polish(widget);
    widget->update();
}


bool GraphcutView::isValidSelectionMulti() {
    // get the nodes selected
    mitk::DataNode *greyscaleImageNode = m_Controls.greyscaleImageSelectorMulti->GetSelectedNode();
    mitk::DataNode *foregroundMaskNode = m_Controls.foregroundImageSelectorMulti->GetSelectedNode();

    // set the mandatory field based on whether or not the nodes are NULL
    setMandatoryField(m_Controls.greyscaleSelectorMulti, (greyscaleImageNode==NULL));
    setMandatoryField(m_Controls.foregroundSelectorMulti, (foregroundMaskNode==NULL));

    if(greyscaleImageNode && foregroundMaskNode){
        // gather input images
        mitk::Image::Pointer grey = dynamic_cast<mitk::Image *>(greyscaleImageNode->GetData());
        mitk::Image::Pointer fg = dynamic_cast<mitk::Image *>(foregroundMaskNode->GetData());

        MITK_INFO << grey->GetDimension() << fg->GetDimension();
        if((grey->GetDimension() == fg->GetDimension())){
            for(int i = 0, max = grey->GetDimension(); i < max ; ++i){
                if((grey->GetDimensions()[i] == fg->GetDimensions()[i])){
                    continue;
                } else{
                    QString msg("Image dimension mismatch in dimension ");
                    msg.append(QString::number(i));
                    msg.append(". Please resample the images.");
                    QMessageBox::warning ( NULL, "Error", msg);
                    return false;
                }
            }
        } else{
            QMessageBox::warning ( NULL, "Error", "Image dimensions do not match.");
            return false;
        }

        MITK_DEBUG("ch.zhaw.graphcut") << "valid selection";
        return true;
    } else{
        MITK_ERROR("ch.zhaw.graphcut") << "invalid selection: missing input.";
        return false;
    }
}

bool GraphcutView::isValidSelectionBin() {
    // get the nodes selected
    mitk::DataNode *greyscaleImageNode = m_Controls.greyscaleImageSelectorBin->GetSelectedNode();
    mitk::DataNode *foregroundMaskNode = m_Controls.foregroundImageSelectorBin->GetSelectedNode();
    mitk::DataNode *backgroundMaskNode = m_Controls.backgroundImageSelectorBin->GetSelectedNode();

    // set the mandatory field based on whether or not the nodes are NULL
    setMandatoryField(m_Controls.greyscaleSelectorBin, (greyscaleImageNode==NULL));
    setMandatoryField(m_Controls.foregroundSelectorBin, (foregroundMaskNode==NULL));
    setMandatoryField(m_Controls.backgroundSelectorBin, (backgroundMaskNode==NULL));

    if(greyscaleImageNode && foregroundMaskNode && backgroundMaskNode){
        if(foregroundMaskNode->GetName() == backgroundMaskNode->GetName()){
            setMandatoryField(m_Controls.foregroundSelectorBin, true);
            setMandatoryField(m_Controls.backgroundSelectorBin, true);
            QMessageBox::warning ( NULL, "Error", "foreground and background seem to be the same image.");
            return false;
        }

        // gather input images
        mitk::Image::Pointer grey = dynamic_cast<mitk::Image *>(greyscaleImageNode->GetData());
        mitk::Image::Pointer fg = dynamic_cast<mitk::Image *>(foregroundMaskNode->GetData());
        mitk::Image::Pointer bg = dynamic_cast<mitk::Image *>(backgroundMaskNode->GetData());

        MITK_INFO << grey->GetDimension() << fg->GetDimension() << bg->GetDimension();
        if((grey->GetDimension() == fg->GetDimension()) && (fg->GetDimension() == bg->GetDimension())){
            for(int i = 0, max = grey->GetDimension(); i < max ; ++i){
                if((grey->GetDimensions()[i] == fg->GetDimensions()[i]) && (fg->GetDimensions()[i] == bg->GetDimensions()[i])){
                    continue;
                } else{
                    QString msg("Image dimension mismatch in dimension ");
                    msg.append(QString::number(i));
                    msg.append(". Please resample the images.");
                    QMessageBox::warning ( NULL, "Error", msg);
                    return false;
                }
            }
        } else{
            QMessageBox::warning ( NULL, "Error", "Image dimensions do not match.");
            return false;
        }

        MITK_DEBUG("ch.zhaw.graphcut") << "valid selection";
        return true;
    } else{
        MITK_ERROR("ch.zhaw.graphcut") << "invalid selection: missing input.";
        return false;
    }
}

void GraphcutView::lockGui(bool b) {
    m_Controls.parentWidget->setEnabled(!b);
    m_Controls.progressBarBin->setVisible(b);
    m_Controls.startButtonBin->setDisabled(b);
    m_Controls.progressBarMulti->setVisible(b);
    m_Controls.startButtonMulti->setDisabled(b);
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void GraphcutView::workerProgressUpdateMulti(float progress, unsigned int){
    int progressInt = (int) (progress * 100.0f);
    m_Controls.progressBarMulti->setValue(progressInt);
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void GraphcutView::workerProgressUpdateBin(float progress, unsigned int){
    int progressInt = (int) (progress * 100.0f);
    m_Controls.progressBarBin->setValue(progressInt);
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void GraphcutView::refreshButtonPressedMulti(){
    imageSelectionChangedMulti();
}
void GraphcutView::refreshButtonPressedBin(){
    imageSelectionChangedBin();
}


