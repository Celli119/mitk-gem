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


// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <mitkLabelSetImage.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
// Qmitk
#include "Voxel2MeshView.h"

// Qt
#include <QMessageBox>

//mitk image
#include <mitkImage.h>
#include <mitkNodePredicateProperty.h>
#include <mitkColorProperty.h>
#include <vtkMitkRectangleProp.h>

#include "mitkGraphcutSegmentationToSurfaceFilter.h"

const std::string Voxel2MeshView::VIEW_ID = "org.mitk.views.voxelmasktopolygonmesh";

void Voxel2MeshView::SetFocus() {
}

void Voxel2MeshView::CreateQtPartControl(QWidget * parent) {
    m_Controls.setupUi(parent);

    // connect slots
    connect(m_Controls.generateSurfaceButton, SIGNAL(clicked()), this, SLOT(generateSurfaceButtonPressed()));
}

void Voxel2MeshView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/, const QList <mitk::DataNode::Pointer> &nodes) {
    if(nodes.count() == 0) {
        m_Controls.selectedImages->setText("no segmentation selected");
        return;
    }
    else {
        setMandatoryField(m_Controls.selectedImages, false);
        QString selectedImageNames;

        bool firstImage = true;
        foreach(mitk::DataNode::Pointer node, nodes){
            if(!firstImage) {
                selectedImageNames.append(", ");
            } else {
                firstImage = false;
            }
            mitk::StringProperty* nameProperty= (mitk::StringProperty*)(node->GetProperty("name"));
            selectedImageNames.append(nameProperty->GetValue());
        }
        m_Controls.selectedImages->setText(selectedImageNames);
    }
}

void Voxel2MeshView::generateSurfaceButtonPressed() {
    // get data
    QList <mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
    SurfaceGeneratorParameters params = getParameters();

    if(nodes.empty()){
        setMandatoryField(m_Controls.selectedImages, true);
    }

    foreach(mitk::DataNode::Pointer node, nodes) {
            mitk::LabelSetImage::Pointer img = dynamic_cast<mitk::LabelSetImage *>(node->GetData());
            std::cout<<img->GetNumberOfLabels()<<std::endl;
            auto nLabels = img->GetNumberOfLabels();
            if (img->GetNumberOfLabels() < 3) {
                mitk::Image::Pointer imgBin = dynamic_cast<mitk::Image *>(node->GetData());
                mitk::Surface::Pointer surface = createSurface(imgBin, params);
                mitk::DataNode::Pointer surfaceNode = mitk::DataNode::New();
                QString name("Surface");
                surfaceNode->SetProperty("name", mitk::StringProperty::New(name.toUtf8().constData()));
                surfaceNode->SetData(surface);
                this->GetDataStorage()->Add( surfaceNode );
            }
            else {
                for (unsigned int iLabel = 1; iLabel < nLabels; ++iLabel) {
                    mitk::Image::Pointer imageLabel = mitk::Image::New();
                    imageLabel = img->CreateLabelMask(iLabel);
                    mitk::Color color = img->GetLabel(iLabel, img->GetActiveLayer())->GetColor();
                    mitk::Surface::Pointer surface = createSurface(imageLabel, params);
                    vtkSmartPointer<vtkPolyDataNormals> normalsGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
                    normalsGenerator->SetInputData(surface->GetVtkPolyData());
                    normalsGenerator->FlipNormalsOn();
                    normalsGenerator->Update();
                    surface->SetVtkPolyData(normalsGenerator->GetOutput());
                    surface->Update();
                    mitk::DataNode::Pointer surfaceNode = mitk::DataNode::New();
                    auto labelName = img->GetLabel(iLabel, img->GetActiveLayer())->GetName();
                    std::string labelNameTmp = labelName + "-Surface";
                    QString name(labelNameTmp.c_str());
                    surfaceNode->SetProperty("name", mitk::StringProperty::New(name.toUtf8().constData()));
                    surfaceNode->SetData(surface);
                    surfaceNode->SetColor(color);
                    this->GetDataStorage()->Add(surfaceNode);
            }
        }
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

mitk::Surface::Pointer Voxel2MeshView::createSurface(mitk::Image::Pointer img, SurfaceGeneratorParameters params){
    mitk::GraphcutSegmentationToSurfaceFilter::Pointer surfaceFilter = mitk::GraphcutSegmentationToSurfaceFilter::New();

    // set params
    surfaceFilter->SetUseMedian(params.doMedian);
    surfaceFilter->SetMedianKernelSize(params.kernelX, params.kernelY, params.kernelZ);

    surfaceFilter->SetUseGaussianSmoothing(params.doGaussian);
    surfaceFilter->SetGaussianStandardDeviation(params.deviation);
    surfaceFilter->SetGaussianRadius(params.radius);

    surfaceFilter->SetThreshold(params.threshold);

    surfaceFilter->SetSmooth(params.doSmoothing);
    surfaceFilter->SetSmoothIteration(params.iterations);
    surfaceFilter->SetSmoothRelaxation(params.relaxation);

    surfaceFilter->SetInput(img);
    surfaceFilter->Update();
    return surfaceFilter->GetOutput();
}

Voxel2MeshView::SurfaceGeneratorParameters Voxel2MeshView::getParameters() {
    SurfaceGeneratorParameters ret;
    ret.doMedian = m_Controls.medianGroup->isChecked();
    ret.kernelX = m_Controls.kernelXSpinBox->value();
    ret.kernelY = m_Controls.kernelYSpinBox->value();
    ret.kernelZ = m_Controls.kernelZSpinBox->value();

    ret.doGaussian = m_Controls.gaussGroup->isChecked();
    ret.deviation = m_Controls.deviationSpinBox->value();
    ret.radius = m_Controls.radiusSpinBox->value();

    auto thresholdPercentage = m_Controls.thresholdSpinBox->value();
    ret.threshold = thresholdPercentage * 255.0 / 100.0;

    ret.doSmoothing = m_Controls.smoothingGroup->isChecked();
    ret.iterations = m_Controls.iterationSpinBox->value();
    ret.relaxation = m_Controls.relaxationSpinBox->value();

    return ret;
}

void Voxel2MeshView::setMandatoryField(QWidget *widget, bool bEnabled){
    setQStyleSheetField(widget, "mandatoryField", bEnabled);
}

void Voxel2MeshView::setQStyleSheetField(QWidget *widget, const char *fieldName, bool bEnabled){
    widget->setProperty(fieldName, bEnabled);
    widget->style()->unpolish(widget); // need to do this since we changed the stylesheet
    widget->style()->polish(widget);
    widget->update();
}