#pragma once

extern "C" bool astar(
    const float* weights, const int h, const int w,
    const int start, const int goal, bool diag_ok,
    int* paths);