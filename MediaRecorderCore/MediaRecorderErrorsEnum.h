#pragma once
#include "MediaRecorderErrorsEnumDef.h"

namespace Native {
    // TODO: rename to MediaRecorderMessageEnum
    enum class MediaRecorderErrorsEnum {
        MediaRecorderErrors_Enum
    };

    struct MediaRecorderEventArgs {
        MediaRecorderErrorsEnum message;
        int remainingTime = 0;
    };
}