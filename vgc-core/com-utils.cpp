#include "pch.h"

namespace vgc
{
    void CheckResult(HRESULT hr)
    {
        if (!SUCCEEDED(hr))
        {
            throw hr;
        }
    }
}
