#ifndef TEST_H
#define TEST_H

#include "saliency_detection.h"

class TestUnit{
public:

    SaliencyDetection salient;
    QImage testSalient(QImage input)
    {
        return salient.getSaliencyDetection(input);
    }

};

#endif // TEST_H
