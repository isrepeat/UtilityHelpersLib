#include "HelpersNew/Geometry/ContainsPoint.h"

namespace utility_helpers::new_helpers::geometry {
    bool ContainsPoint(
        float boundsX,
        float boundsY,
        float boundsWidth,
        float boundsHeight,
        float pointX,
        float pointY) {
        return pointX >= boundsX
            && pointX <= boundsX + boundsWidth
            && pointY >= boundsY
            && pointY <= boundsY + boundsHeight;
    }
}