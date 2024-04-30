#pragma once
#include <Helpers/common.h>
#include <Helpers/Time.h>

namespace MEDIA_FOUNDATION_NS {
    struct SampleInfo {
        const HELPERS_NS::Chrono::Hns pts;
        const HELPERS_NS::Chrono::Hns duration;
        const HELPERS_NS::Chrono::Hns nextSamplePts;

        SampleInfo()
            : pts{ 0_hns }
            , duration{ 0_hns }
            , nextSamplePts{ 0_hns }
        {}

        SampleInfo(HH::Chrono::Hns pts, HH::Chrono::Hns duration)
            : pts{ pts }
            , duration{ duration }
            , nextSamplePts{ pts + duration }
        {}
    };
}