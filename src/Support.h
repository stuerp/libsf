
/** $VER: Support.h (2025.09.01) P. Stuer **/

#pragma once

#include <sdkddkver.h>
#include <Windows.h>

#include <math.h>

/// <summary>
/// Returns the input value clamped between min and max.
/// </summary>
template <class T>
inline static T Clamp(T value, T minValue, T maxValue)
{
    return std::min(std::max(value, minValue), maxValue);
}

/// <summary>
/// Returns true of the input value is in the interval between min and max.
/// </summary>
template <class T>
inline static T InRange(T value, T minValue, T maxValue)
{
    return (minValue <= value) && (value <= maxValue);
}

/// <summary>
/// Maps a value from one range (srcMin, srcMax) to another (dstMin, dstMax).
/// </summary>
template<class T, class U>
inline static U Map(T value, T srcMin, T srcMax, U dstMin, U dstMax)
{
    return dstMin + (U) (((double) (value - srcMin) * (double) (dstMax - dstMin)) / (double) (srcMax - srcMin));
}
