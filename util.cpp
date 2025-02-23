#include "include/util.h"

namespace yhl_util {

bool
check_collision(const Rectangle& rect1, const Rectangle& rect2)
{
    // Check if one rectangle is to the left of the other
    if (rect1.x + rect1.width < rect2.x || rect2.x + rect2.width < rect1.x) {
        return false; // No collision on the x-axis
    }

    // Check if one rectangle is above the other
    if (rect1.y + rect1.height < rect2.y || rect2.y + rect2.height < rect1.y) {
        return false; // No collision on the y-axis
    }

    // If neither condition is true, the rectangles overlap
    return true;
}
};