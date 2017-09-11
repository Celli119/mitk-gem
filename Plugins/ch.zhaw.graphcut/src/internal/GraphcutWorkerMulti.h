/**
 *  MITK-GEM: Graphcut Plugin
 *
 *  Copyright (c) 2016, Zurich University of Applied Sciences, School of Engineering, T. Fitze, Y. Pauchard
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved.
 */


#ifndef _GraphcutWorkerMulti_h_
#define _GraphcutWorkerMulti_h_

#include "GraphcutWorker.h"
#include <internal/lib/GraphCut3D/MultiLabelGraphCut.h>

class GraphcutWorkerMulti : public GraphcutWorker{

public:
    // typedef for pipeline
    typedef MultiLabelGraphCut::FilterType <InputImageType, MaskImageType, OutputImageType> GraphCutFilterType;

private:
    virtual void preparePipeline();
    GraphCutFilterType::Pointer m_graphCut;

    virtual void process();
};


#endif //_GraphcutWorkerMulti_h_
