#pragma once

template<typename T>
inline T _max(T a, T b)
{
    if(a > b)
        return a;
    else
        return b;
}

template<typename T>
inline T _min(T a, T b)
{
    if(a < b)
        return a;
    else
        return b;
}

template<typename T>
inline T _clamp(T v, T min, T max)
{
    if(v < min)
        return min;
    if(v > max)
        return max;
    return v;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
