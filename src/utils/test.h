#ifndef TEST_H
#define TEST_H

#include "saliency_detection.h"

class TestUnit{
public:

    SaliencyDetection salient;
    void testSalient(QImage input)
    {
        salient.getSaliencyMap(input);
    }

};

#endif // TEST_H
