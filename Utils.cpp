#include <cctype>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "Utils.h"

using namespace std;
using namespace UniTag;

unsigned int utils::bytesToInt(const std::vector <char> &bytes,const bool reverse)
{
    unsigned int result = 0x00;
    int k = 0;
    if(reverse)
        k = bytes.size() - 1;
    for(int i = 0; i<bytes.size() ; i++)
    {
        result = result << 8;
        result = result | (unsigned char) bytes[k];

        if(reverse)
            k--;
        else
            k++;
    }

    return result;
}
unsigned int utils::getInt(const std::vector <char> &bytes, const unsigned int pos, const unsigned int length, const bool reverse)
{
    if(pos + length > bytes.size())
    {
        cerr<<"ERROR : getInt"<<endl;
        return 0;
    }
    return bytesToInt(vector <char> (bytes.begin() + pos, bytes.begin() + pos + length),reverse);
}

unsigned int utils::syncIntEncode(unsigned int value)
{
    unsigned int out, mask = 0x7F;

    while (mask ^ 0x7FFFFFFF) {
        out = value & ~mask;
        out <<= 1;
        out |= value & mask;
        mask = ((mask + 1) << 8) - 1;
        value = out;
    }
    return out;
}

unsigned int utils::syncIntDecode(unsigned int value)
{
    unsigned int a, b, c, d, result = 0x0;
    a = value & 0xFF;
    b = (value >> 8) & 0xFF;
    c = (value >> 16) & 0xFF;
    d = (value >> 24) & 0xFF;

    result = result | a;
    result = result | (b << 7);
    result = result | (c << 14);
    result = result | (d << 21);

    return result;
}

std::vector <char> utils::toBytes(const int integer, const bool reverse)
{
    const unsigned int size = 4;
    vector <char> result(size);

    int k = 0;
    if(!reverse)
        k = size - 1;

    char* aux = (char*) &integer;
    for(int i = 0; i < size; i++)
    {
        result[k] = aux[i];
        if(!reverse)
            k--;
        else
            k++;
    }
    return result;
}
std::bitset<8> utils::toBits(const char &byte)
{
    return bitset<8>(byte);
}
std::bitset<8> utils::toBits(char *byte)
{
    return toBits(byte[0]);
}
void utils::push(std::vector <char> &v, const std::vector <char> &toAdd)
{
    v.insert(v.end(), toAdd.begin(), toAdd.end());
}
void utils::push(std::vector <char> &v, const std::string &toAdd)
{
    v.insert(v.end(), toAdd.begin(), toAdd.end());
}
std::string utils::toStr(const std::vector <char> &charVector)
{
    return std::string(charVector.begin(),charVector.end());
}
std::vector <std::string> utils::split(const std::string &s, const char c)
{
	vector <string> v;
    size_t prevF = 0,f = 0;
    while((f=s.find(c,prevF))!=string::npos){
        v.push_back(s.substr(prevF,f-prevF));
        prevF = f+1;
    }
    v.push_back(s.substr(prevF));
	return v;
}
bool utils::absEquals(std::string s1,std::string s2)
{
    if(s1.size() != s2.size())
        return false;

    std::transform(s1.begin(), s1.end(),s1.begin(), ::toupper);
    std::transform(s2.begin(), s2.end(),s2.begin(), ::toupper);
    return s1 == s2;
}
std::string utils::getExtension(const std::string &path)
{
    std::string ext;
    for(int i=path.size();i>=0;i--)
    {
        if(path[i]=='/')
            return "";
        else if(path[i]=='.')
            break;
        else
            ext = path[i] + ext;
    }
    if(ext[ext.size()-1] == 0)
        ext.erase(ext.size()-1);
    if(ext.size() == path.size())
        return "";
    return ext;
}

std::vector <char> utils::getDataFromFile(const std::string &path)
{
    vector <char> buffer;
    ifstream rf(path,ios::binary);
    if(rf)
    {
        rf.seekg(0,rf.end);
        buffer.resize(rf.tellg());
        rf.seekg(0, rf.beg);
        rf.read(&buffer[0], buffer.size());
        rf.close();
    }
    else
        cerr<<"Error : utils : getDataFromFile : "<<path<<" : not found";
    return buffer;
}
unsigned int utils::getImageSizeWithData(const std::vector <char> &img, unsigned int &w, unsigned int &h)
{
    if(img.size()<24)
        return 0;

    std::vector <uint8_t> buf(img.begin(),img.begin()+24);
    // Strategy:
    // reading GIF dimensions requires the first 10 bytes of the file
    // reading PNG dimensions requires the first 24 bytes of the file
    // reading JPEG dimensions requires scanning through jpeg chunks
    // In all formats, the file is at least 24 bytes big, so we'll read that always

    // For JPEGs, we need to read the first 12 bytes of each chunk.
    // We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
    if(buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF && buf[3]==0xE0
       && buf[6]=='J' && buf[7]=='F' && buf[8]=='I' && buf[9]=='F')
    {
        unsigned int pos = 2;
        while (buf[2]==0xFF)
        {
            if (buf[3]==0xC0 || buf[3]==0xC1 || buf[3]==0xC2 || buf[3]==0xC3 || buf[3]==0xC9 || buf[3]==0xCA || buf[3]==0xCB)
                break;
            pos += 2 + (buf[4]<<8) + buf[5];
            if(pos + 12 > img.size())
                break;
            for(int i=0;i<12;i++)
                buf[2 + i] = img.at(pos + i);
        }
    }

    // JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
    if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF)
    {
        h = (buf[7]<<8) + buf[8];
        w = (buf[9]<<8) + buf[10];
        return 1;
    }

    // GIF: first three bytes say "GIF", next three give version number. Then dimensions
    if (buf[0]=='G' && buf[1]=='I' && buf[2]=='F')
    {
        w = buf[6] + (buf[7]<<8);
        h = buf[8] + (buf[9]<<8);
        return 2;
    }

    // PNG: the first frame is by definition an IHDR frame, which gives dimensions
    if ( buf[0]==0x89 && buf[1]=='P' && buf[2]=='N' && buf[3]=='G' && buf[4]==0x0D && buf[5]==0x0A && buf[6]==0x1A && buf[7]==0x0A
    && buf[12]=='I' && buf[13]=='H' && buf[14]=='D' && buf[15]=='R')
    {
        w = (buf[16]<<24) + (buf[17]<<16) + (buf[18]<<8) + (buf[19]<<0);
        h = (buf[20]<<24) + (buf[21]<<16) + (buf[22]<<8) + (buf[23]<<0);
        return 3;
    }
    return 0;
}

std::vector <uint8_t> utils::UTF32ToAinsi(wchar_t c)
{
    vector <uint8_t> result;
    if(c<0x80)
    {
        result.push_back(c);
        return result;
    }
    else if(c<0x800)
    {
        result.push_back(0xc0+c/0x40);
    }
    else if(c<0x10000)
    {
        result.push_back(0xe0+c/0x1000);
        result.push_back(0x80+(c%0x1000)/0x40);
    }
    else
    {
        result.push_back(0xf0+c/0x40000);
        result.push_back(0x80+(c%0x40000)/0x1000);
        result.push_back(0x80+(c%0x1000)/0x40);
    }

    result.push_back(0x80+c%0x40);
    return result;
}

std::string utils::UTF32ToAinsi(std::wstring str)
{
    std::string newStr;
    for(auto c : str)
    {
        auto res = UTF32ToAinsi(c);
        for(auto k : res)
            newStr.push_back(k);
    }
    return newStr;
}
std::wstring utils::ainsiToUTF32(std::string str)
{
    std::wstring newStr;
    for(unsigned int i=0;i<str.size();i++)
    {
        wchar_t c[4];
        for(unsigned int k=0;k<4 && i+k<str.size();k++)
        {
            if(str[i+k]<0)
                c[k]=(wchar_t)((int)str[i+k]+256);
            else
            {
                c[k]=(wchar_t)str[i+k];
                break;
            }
        }

        if(c[0]<0xc0)
            newStr.push_back(c[0]);
        else if(c[0]<0xe0 && c[1]>=0x80 && i<str.size()-1)
        {
            newStr.push_back(0x40*(c[0]-0xc0)+(c[1]-0x80));
            i++;
        }
        else if(c[0]<0xf0 && c[1]>=0x80 && c[2]>=0x80 &&  i<str.size()-2)
        {
            newStr.push_back(0x1000*(c[0]-0xe0) + 0x40*(c[1]-0x80) + (c[2]-0x80));
            i+=2;
        }
        else if(c[0]>=0x80 && c[1]>=0x80 && c[2]>=0x80 && c[3]>=0x80 && i<str.size()-3)
        {
            newStr.push_back(0x40000*(c[0]-0xf0) + 0x1000*(c[1]-0x80) + 0x40*(c[2]-0x80) + (c[3]-0x80));
            i+=3;
        }
        else
            newStr.push_back(str[i]);
    }
    return newStr;
}
