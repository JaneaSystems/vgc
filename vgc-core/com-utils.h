#pragma once

#include "pch.h"

namespace vgc
{
    void CheckResult(HRESULT hr);

    template<class T>
    void SafeRelease(T& obj)
    {
        if (obj)
        {
            obj->Release();
            obj = nullptr;
        }
    }
}
