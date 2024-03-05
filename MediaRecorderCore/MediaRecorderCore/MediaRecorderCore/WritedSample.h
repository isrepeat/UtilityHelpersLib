#pragma once
#include <Helpers/Time.h>

//namespace MediaRecorderCore {
struct WritedSample {
    const HH::Chrono::Hns pts;
    const HH::Chrono::Hns duration;
    const HH::Chrono::Hns nextSamplePts;

    WritedSample()
        : pts{ 0 }
        , duration{ 0 }
        , nextSamplePts{ 0 }
    {}

    WritedSample(int64_t pts, int64_t duration)
        : pts{ pts }
        , duration{ duration }
        , nextSamplePts{ pts + duration }
    {}
};
//}