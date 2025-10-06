#include <codecvt>
#include <string>
#include <locale>


int main () {
    std::string utf8_str = "Привет, мир!";
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring utf16_str = converter.from_bytes(utf8_str);

    std::string back_to_utf8 = converter.to_bytes(utf16_str);
    return 0;
}
