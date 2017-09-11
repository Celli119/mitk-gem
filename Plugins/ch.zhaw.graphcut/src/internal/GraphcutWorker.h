/**
 *  MITK-GEM: Graphcut Plugin
 *
 *  Copyright (c) 2016, Zurich University of Applied Sciences, School of Engineering, T. Fitze, Y. Pauchard
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved.
 */

#ifndef __GraphcutWorker_h__
#define __GraphcutWorker_h__

// ITK
#include <itkImage.h>
#include <itkCommand.h>
#include <itkBinaryThresholdImageFilter.h>

#include <thread>

#include "Worker.h"
#include "WorkbenchUtils.h"

#include <internal/lib/GraphCut3D/GraphCut.h>
#include <internal/lib/GraphCut3D/MultiLabelGraphCut.h>

class ProgressObserverCommand : public itk::Command {
public:
    itkNewMacro(ProgressObserverCommand);

    void Execute(itk::Object *caller, const itk::EventObject &event) override {
        Execute(const_cast<const itk::Object *>(caller), event);
    }

    void Execute(const itk::Object *caller, const itk::EventObject &event) override{
        itk::ProcessObject *processObject = (itk::ProcessObject*)caller;
        if (typeid(event) == typeid(itk::ProgressEvent)) {
            if(m_worker){
                m_worker->itkProgressCommandCallback(processObject->GetProgress());
            } else{
                std::cout << "ITK Progress event received from "
                        << processObject->GetNameOfClass() << ". Progress is "
                        << 100.0 * processObject->GetProgress() << " %."
                        << std::endl;
            }
        }
    }

    void SetCallbackWorker(Worker* worker){
        m_worker = worker;
    }
private:
    Worker *m_worker;
};

class GraphcutWorker : public Worker {

public:
    enum BoundaryDirection{
        BIDIRECTIONAL = 0,
        BRIGHT_TO_DARK = 1,
        DARK_TO_BRIGHT = 2,
        BoundaryDirection_MAX_VALUE = DARK_TO_BRIGHT
    };

    ~GraphcutWorker(){
    }

    // image typedefs
    typedef itk::Image<short, 3> InputImageType;
    typedef unsigned char BinaryPixelType;
    typedef itk::Image<BinaryPixelType, 3> MaskImageType;
    typedef itk::Image<BinaryPixelType, 3> OutputImageType;

    GraphcutWorker();

    // inherited slots
    virtual void process() = 0;

    // inherited signals
    void started(unsigned int workerId);
    void progress(float progress, unsigned int workerId);
    void finished(itk::DataObject::Pointer ptr, unsigned int workerId);

    // callback for the progress command
    void itkProgressCommandCallback(float progress);

    // setters
    void setInputImage(InputImageType::Pointer img){
        m_input = img;
    }

    void setForegroundMask(MaskImageType::Pointer mask){
        m_foreground = mask;
    }


    void setSigma(double d){
        m_Sigma = d;
    }

    void setBoundaryDirection(BoundaryDirection i){
        m_boundaryDirection = i;
    }

    void setForegroundPixelValue(BinaryPixelType u){
        m_ForegroundPixelValue = u;
    }

    unsigned int id;

protected:

    virtual void preparePipeline()= 0;

    BinaryPixelType m_ForegroundPixelValue;

    OutputImageType::Pointer m_output;
    BoundaryDirection m_boundaryDirection;
// member variables
    InputImageType::Pointer m_input;
// parameters
    double m_Sigma;
    MaskImageType::Pointer m_foreground;
    ProgressObserverCommand::Pointer m_progressCommand;

    MaskImageType::Pointer rescaleMask(MaskImageType::Pointer, MaskImageType::ValueType);
};

#endif // __GraphcutWorker_h__

