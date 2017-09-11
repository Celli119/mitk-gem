//
// Created by girinon on 10/09/17.
//

#ifndef _GraphcutWorkerBin_h
#define _GraphcutWorkerBin_h

#include "GraphcutWorker.h"
//#include <internal/lib/GraphCut3D/GraphCut.h>

class GraphcutWorkerBin : public GraphcutWorker{

public:
    GraphcutWorkerBin();
    // typedef for pipeline

    typedef GraphCut::FilterType<InputImageType, MaskImageType, MaskImageType, OutputImageType> GraphCutFilterType;

    void setBackgroundMask(MaskImageType::Pointer mask){
        m_background = mask;
    }
private:
    virtual void preparePipeline();
    GraphCutFilterType::Pointer m_graphCut;
    virtual void process();

protected:
    MaskImageType::Pointer m_background;
    BinaryPixelType m_ForegroundPixelValue;
    double m_Sigma;

};


#endif //_GraphcutWorkerBin_h
