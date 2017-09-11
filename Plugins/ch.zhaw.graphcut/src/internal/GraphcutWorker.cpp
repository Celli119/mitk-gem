/**
 *  MITK-GEM: Graphcut Plugin
 *
 *  Copyright (c) 2016, Zurich University of Applied Sciences, School of Engineering, T. Fitze, Y. Pauchard
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved.
 */

#include <thread>
#include <itkBinaryThresholdImageFilter.h>

#include "GraphcutWorker.h"

GraphcutWorker::GraphcutWorker()
        : id(WorkbenchUtils::getId())
        , m_Sigma(50)
        , m_ForegroundPixelValue(255)
{
}

void GraphcutWorker::itkProgressCommandCallback(float progress){
    emit Worker::progress(progress, id);
}

GraphcutWorker::MaskImageType::Pointer GraphcutWorker::rescaleMask(MaskImageType::Pointer _mask, MaskImageType::ValueType _insideValue) {
    auto min = itk::NumericTraits<MaskImageType::ValueType>::min();
    auto max = itk::NumericTraits<MaskImageType::ValueType>::max();

    auto thresholdFilter = itk::BinaryThresholdImageFilter<MaskImageType, MaskImageType>::New();
    thresholdFilter->SetInput(_mask);
    thresholdFilter->SetLowerThreshold(min + 1);
    thresholdFilter->SetUpperThreshold(max);
    thresholdFilter->SetInsideValue(_insideValue);
    thresholdFilter->Update();
    return thresholdFilter->GetOutput();
}