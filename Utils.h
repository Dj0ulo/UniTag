#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <string>
#include <vector>
#include <bitset>

namespace UniTag
{
    namespace utils
    {
        unsigned int bytesToInt(const std::vector <char> &bytes,const bool reverse);
        unsigned int getInt(const std::vector <char> &bytes, const unsigned int pos = 0, const unsigned int length = 4, const bool reverse = false);
        unsigned int syncIntEncode(unsigned int value);
        unsigned int syncIntDecode(unsigned int value);
        std::vector <char> toBytes(const int integer, const bool reverse);
        std::bitset<8> toBits(const char &byte);
        std::bitset<8> toBits(char *byte);
        void push(std::vector <char> &v, const std::vector <char> &toAdd);
        void push(std::vector <char> &v, const std::string &toAdd);
        std::string toStr(const std::vector <char> &charVector);
        std::vector <std::string> split(const std::string &s, const char c);
        bool absEquals(std::string s1,std::string s2);
        std::string getExtension(const std::string &path);
        std::vector <char> getDataFromFile(const std::string &path);
        unsigned int getImageSizeWithData(const std::vector <char> &img, unsigned int &w, unsigned int &h);

        std::vector <uint8_t> UTF32ToAinsi(wchar_t c);
        std::string UTF32ToAinsi(std::wstring str);
        std::wstring ainsiToUTF32(std::string str);
    }
}

#endif // UTILS_H_INCLUDED
