#pragma once
#include <string>

namespace IBR_Font
{
    std::wstring SearchFontName(std::wstring FontName);
    std::wstring SearchFont(std::wstring_view FontName);
    void BuildFontQuery();
}
