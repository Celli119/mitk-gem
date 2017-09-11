/**
 *  MITK-GEM: Graphcut Plugin
 *
 *  Copyright (c) 2016, Zurich University of Applied Sciences, School of Engineering, T. Fitze, Y. Pauchard
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved.
 */

#include "GraphcutWorkerBin.h"

void GraphcutWorkerBin::preparePipeline() {
    MITK_INFO("ch.zhaw.graphcut") << "prepare pipeline...";

    m_graphCut = GraphCutFilterType::New();
    m_graphCut->SetInputImage(m_input);
    m_graphCut->SetForegroundImage(rescaleMask(m_foreground, m_ForegroundPixelValue));
    m_graphCut->SetBackgroundImage(rescaleMask(m_background, m_ForegroundPixelValue));
    m_graphCut->SetForegroundImage(m_foreground);
    m_graphCut->SetBackgroundImage(m_background);
    m_graphCut->SetForegroundPixelValue(m_ForegroundPixelValue);
    const uint32_t uiNumberOfThreads = std::thread::hardware_concurrency();
    m_graphCut->SetNumberOfThreads(uiNumberOfThreads > 0 ? uiNumberOfThreads : 1);

    m_graphCut->SetSigma(m_Sigma);
    switch (m_boundaryDirection) {
        case 0:
            m_graphCut->SetBoundaryDirectionTypeToNoDirection();
            break;
        case 1:
            m_graphCut->SetBoundaryDirectionTypeToBrightDark();
            break;
        case 2:
            m_graphCut->SetBoundaryDirectionTypeToDarkBright();
            break;
    }

    // add progress observer
    m_progressCommand = ProgressObserverCommand::New();
    static_cast<ProgressObserverCommand*>(m_progressCommand.GetPointer())->SetCallbackWorker(this);
    m_graphCut->AddObserver(itk::ProgressEvent(), m_progressCommand);

    MITK_INFO("ch.zhaw.graphcut") << "... pipeline prepared";
}

void GraphcutWorkerBin::process() {
    MITK_INFO("ch.zhaw.graphcut") << "worker started";
    emit Worker::started(id);

    try{
        preparePipeline();
        m_graphCut->Update();
        m_output = m_graphCut->GetOutput();
    } catch (itk::ExceptionObject &e){
        MITK_ERROR("ch.zhaw.graphcut") << "Exception caught during execution of pipeline 'GraphcutWorker'.";
        MITK_ERROR("ch.zhaw.graphcut") << e;
    }

    MITK_INFO("ch.zhaw.graphcut") << "worker done";
    emit Worker::finished((itk::DataObject::Pointer) m_output, id);
}

GraphcutWorkerBin::GraphcutWorkerBin()
        : m_Sigma(50)
        , m_ForegroundPixelValue(255){

}
