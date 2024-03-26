#pragma once
#include <Helpers/common.h>
#include <Helpers/Time.h>

namespace MEDIA_FOUNDATION_NS {
    struct SampleInfo {
        const HELPERS_NS::Chrono::Hns pts;
        const HELPERS_NS::Chrono::Hns duration;
        const HELPERS_NS::Chrono::Hns nextSamplePts;

        SampleInfo()
            : pts{ 0 }
            , duration{ 0 }
            , nextSamplePts{ 0 }
        {}

        SampleInfo(int64_t pts, int64_t duration)
            : pts{ pts }
            , duration{ duration }
            , nextSamplePts{ pts + duration }
        {}
    };
}