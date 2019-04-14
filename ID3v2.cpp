#include <fstream>
#include <iostream>
#include <bitset>
#include <sstream>
#include <cstdlib>
#include <algorithm>

#include "Utils.h"
#include "UniTag.h"

using namespace std;
using namespace UniTag;
using namespace utils;

ID3v2::ID3v2(const std::string path) : Tag(path)
{
    parse();
}
std::string ID3v2::parseFrameWidthIndex(const unsigned int index) const
{
    stringstream ss;
    if(_frames[index].flag == getFrameFlag(COVER_ART))
    {
        ss<<"[IMAGE (size : "<<_frames[index].data.size()<<")]";
        return ss.str();
    }

    std::string result;
    auto f = _frames[index];

    const unsigned int dataSize = f.data.size();

    unsigned int i = 1;

    std::string language;
    if(f.flag.substr(0,3)=="UFI")
        i=0;
    else if(f.flag[0] == 'C' || f.flag[0] == 'U')
    {
        language = std::string(f.data.begin() + 1, f.data.begin() + 4);
        if(_majorVersion == 2)
            i += 3;
        else
            i += 4;
    }


    if(f.data[0] == 1)//encoding
    {
        if(language != "")
        {
            if(_majorVersion == 2)
                i = 6;
            else
                i = 10;
        }
        else
            i = 3;
        for(; i< dataSize; i++)
        {
            char c = f.data[i];
            if((uint8_t)c != 0x00)
            {
                if((uint8_t)c == 0x0D)//carriage return
                    result.push_back('\n');
                else
                    result.push_back(c);
            }
            else if((uint8_t)c == 0x00 && i+1<dataSize && (uint8_t)f.data[i+1] == 0x00)//si "\0\0"
            {
                result.push_back('\0');
                i++;
                if(i+3<dataSize && (uint8_t)f.data[i+1]==0x00 && (uint8_t)f.data[i+2]==0xFF && (uint8_t)f.data[i+3]==0xFE)
                    i+=3;
            }
        }
    }
    else
        result = std::string(f.data.begin() + i, f.data.end() );

    int null = (int)result.find('\0');
    if(f.flag.substr(0,3)!="TXX" && null!=-1)
        result.erase(result.begin()+null,result.end());
    else if(result[result.size()-1] == 0x00)
        result.erase(result.end()-1);

    if(f.flag == getFrameFlag(GENRE) && result[0]=='(' && result[result.size()-1]==')')
        return decodeGenre(std::atoi(std::string(result.begin()+1,result.end()-1).c_str()));
    return result;
}
std::string ID3v2::get(const std::string flag) const
{
    auto framesIndexes = Tag::getFramesIndexes(flag);
    if(flag == getFrameFlag(DATE) && framesIndexes.size() == 0)
        framesIndexes = Tag::getFramesIndexes("TYER");

    if(framesIndexes.size() == 0)
        return "";
    return parseFrameWidthIndex(framesIndexes[0]);
}
std::vector <char> ID3v2::getCoverArt() const
{
    auto framesIndexes = Tag::getFramesIndexes(getFrameFlag(COVER_ART));

    if(framesIndexes.size() == 0)
        return std::vector <char>();

    auto img = _frames[framesIndexes[0]].data;
    const unsigned char encoding = img[0];
    unsigned int offset = 1;
    if(_majorVersion == 2)
    {
        string mimetype = string(img.begin() + offset, img.begin() + offset + 3);
        offset += mimetype.size() + 1;
    }
    else
    {
        string mimetype = string(img.begin() + offset, find(img.begin() + offset, img.begin() + 12, (char)0x00));
        offset += mimetype.size() + 2;
    }
    if(encoding == 0x01 || encoding == 0x02)
    {
        for (;getInt(img, offset, 2) != 0x00; offset += 2);//skip UTF-16 description
        offset += 2;
    }
    else
    {
        for(;img[offset] != '\0';offset++);//skip UTF-8 or Latin-1 description
        offset += 1;
    }
    return vector <char>(img.begin() + offset, img.end());
}
std::string ID3v2::getFrameFlag(const unsigned int flag) const
{
    if(_majorVersion == 2)
    {
        if     (flag == TITLE) return "TT2";
        else if(flag == ARTIST) return "TP1";
        else if(flag == ALBUM) return "TAL";
        else if(flag == ALBUM_ARTIST) return "TP2";
        else if(flag == DATE) return "TYE";
        else if(flag == GENRE) return "TCO";
        else if(flag == LABEL) return "TPB";
        else if(flag == DESCRIPTION) return "TT3";
        else if(flag == ENCODED_BY) return "TEN";
        else if(flag == ENCODER) return "TSS";
        else if(flag == TRACK_NUM) return "TRK";
        else if(flag == DISC) return "TPA";
        else if(flag == COMMENT) return "TCM";
        else if(flag == LYRICS) return "ULT";
        else if(flag == COVER_ART) return "PIC";
    }
    else
    {
        if(flag == TITLE)
            return "TIT2";
        else if(flag == ARTIST)
            return "TPE1";
        else if(flag == ALBUM)
            return "TALB";
        else if(flag == ALBUM_ARTIST)
            return "TPE2";
        else if(flag == DATE)
            return "TDRC";
        else if(flag == GENRE)
            return "TCON";
        else if(flag == LABEL)
            return "TPUB";
        else if(flag == DESCRIPTION)
            return "TIT3";
        else if(flag == ENCODED_BY)
            return "TENC";
        else if(flag == ENCODER)
            return "TSSE";
        else if(flag == TRACK_NUM)
            return "TRCK";
        else if(flag == DISC)
            return "TPOS";
        else if(flag == COMMENT)
            return "COMM";
        else if(flag == LYRICS)
            return "USLT";
        else if(flag == COVER_ART)
            return "APIC";
    }
    return "";
}
std::string ID3v2::convertOldFrameFlag(const std::string oldId)
{
    if     (oldId == "TT2") return "TIT2";
    else if(oldId == "TP1") return "TPE1";
    else if(oldId == "TAL") return "TALB";
    else if(oldId == "TP2") return "TPE2";
    else if(oldId == "TYE") return "TDRC";
    else if(oldId == "TCO") return "TCON";
    else if(oldId == "TPB") return "TCON";
    else if(oldId == "TT3") return "TIT3";
    else if(oldId == "TEN") return "TENC";
    else if(oldId == "TSS") return "TSSE";
    else if(oldId == "TRK") return "TRCK";
    else if(oldId == "TPA") return "TPOS";
    else if(oldId == "TCM") return "COMM";
    else if(oldId == "ULT") return "USLT";
    else if(oldId == "PIC") return "APIC";
    else if(oldId == "UFI") return "UFID";
    else if(oldId == "COM") return "COMM";
    return "TXXX";
}
void ID3v2::updateFrameToNewMajorVersion()
{
    for(int i=0;(unsigned int)i<_frames.size();i++)
    {
        _majorVersion = 2;
        if(_frames[i].flag == getFrameFlag(COVER_ART))
            _frames[i].flag = convertOldFrameFlag(_frames[i].flag);
        else
        {
            string text = get(_frames[i].flag);
            _majorVersion = 4;
            _frames[i].flag = convertOldFrameFlag(_frames[i].flag);
            _frames[i].data = textToData(_frames[i].flag, text);
        }
    }
    _majorVersion = 4;
}

std::vector <char> ID3v2::textToData(const std::string &flag,const std::string &text)
{
    vector <char> data;
    if(flag=="UFID")
        data = vector <char> (text.begin(),text.end());
    else{
        data.push_back(0x01);//encoding

        if(flag == getFrameFlag(COMMENT) || flag == getFrameFlag(LYRICS))
        {
            push(data,"eng");
            if(_majorVersion != 2)
            {
                data.push_back(0xFF);
                data.push_back(0xFE);
                data.push_back(0x00);
                data.push_back(0x00);
            }
        }
        data.push_back(0xFF);
        data.push_back(0xFE);

        for(char c : text)
        {
            data.push_back(c);
            data.push_back(0x00);
        }
    }
    return data;
}
void ID3v2::set(const std::string &flag, const std::string &text)
{
    if(flag == "")
    {
        cerr<<"ID3v2 : set : flag invalid"<<endl;
        return;
    }
    Tag::set(flag, textToData(flag, text));
}
void ID3v2::set(const unsigned int flag, const std::string &text)
{
    string strFlag = getFrameFlag(flag);
    set(strFlag, text);
}

void ID3v2::setCoverArt(const std::vector <char> &img)
{
    unsigned int x,y;
    unsigned int type = getImageSizeWithData(img, x, y);
    if(!type)
        return;
    vector <char> data;

    std::string mimetype = "image/jpeg";
    if(type == 2)
        mimetype = "image/png";
    else if(type == 3)
        mimetype = "image/gif";

    data.push_back(0x00);//encoding
    push(data, mimetype);
    data.push_back(0x00);
    data.push_back(0x03);//picture type (cover front)
    data.push_back(0x00);//description

    push(data, img);
    Tag::set(getFrameFlag(COVER_ART), data);
}

void ID3v2::saveFrames(std::string savePath)
{
    if(savePath == "")
        savePath = _path;
    if(_majorVersion == 2)
        updateFrameToNewMajorVersion();

    vector <char> buffer;
    ifstream rf(_path,ios::binary);
    if(rf)
    {
        rf.seekg(0,rf.end);
        buffer.resize(rf.tellg());
        rf.seekg(0, rf.beg);
        rf.read(&buffer[0], buffer.size());
        rf.close();
    }
    ofstream of(savePath, ios::binary);
    if(of)
    {
        of.write("ID3", 3);
        of.put(0x04);
        of.put(0x00);
        of.put(0x00);
        unsigned int finalHeaderSize = 0;
        for(Frame f : _frames)
            finalHeaderSize += 10 + f.data.size();
        of.write(&toBytes(syncIntEncode(finalHeaderSize), false)[0], 4);
        for(Frame f : _frames)
        {
            of.write(f.flag.c_str(),4);
            of.write(&toBytes(syncIntEncode(f.data.size()), false)[0],4);
            of.write("\0\0", 2);
            of.write(&f.data[0], f.data.size());
        }
        of.write(&buffer[_chunkFrames.end()], buffer.size() - _chunkFrames.end());
    }
    else
        cerr<<"ID3v2 : ERROR : Cannot save file : "<<_path<<endl;

    parse();
}

//void tocsv(vector <float> x,vector <float> y,std::string f) {ofstream of(f); for(int i=0;i<x.size() && i<y.size();i++) of<<x[i]<<" "<<y[i]<<endl;}
unsigned int ID3v2::getDuration()
{
//    vector <float> pos;
//    vector <float> srs;

    ifstream f(_path,ios::binary);
    if(f)
    {
        f.seekg(_posEndTag, f.beg);//on se place apres les tags

        vector <char> buffer(10000);

        float seconds = 0;

        size_t totBitrate = 0;
        size_t totSamplingRate=0;
        int sr32=0,sr48=0,sr44=0;

        int nbFrame=0;
        size_t posFrame = 0;

        const vector <char> a(1,0xff);
        const char xing[] = "Xing";

        bool vbr = false;
        unsigned int vbrNbFrames=0;
        do{
            bitset <8> b0,b1;
            bool condition=false;
            size_t found = string::npos;
            while(!f.eof() && !condition)
            {
                f.read(&buffer[0],buffer.size());//on lit les [sizeBuffer] octets
                condition=false;
                do
                {
                    auto it = buffer.begin();
                    if(!vbr && (it = search(buffer.begin(), buffer.end(), begin(xing), end(xing))) != buffer.end())
                    {
                        vbr=true;

                        const size_t b = distance(buffer.begin(), it) - 36;
                        if(buffer[b]==-1 && getInt(buffer, b+40, 4, false)%2==1)
                            vbrNbFrames = getInt(buffer, b+44, 4, false);
                        else
                            found = b + 156;
                    }

                    it = search(buffer.begin() + found + 1, buffer.end(), a.begin(), a.end());
                    if(it == buffer.end())
                        break;

                    found = distance(buffer.begin(), it);

                    b0 = toBits(buffer[found + 1]);
                    b1 = toBits(buffer[found + 2]);
                    condition = (b0[7]==1 && b0[6]==1 && b0[5]==1)
                            && !(b1[7]==1 && b1[6]==1 && b1[5]==1 && b1[4]==1)
                            && !(b1[3]==1 && b1[2]==1);
                }while(!condition);// || vbr == Vbr::FALSE_FIRST);
            }///check where the first frame header is

            posFrame = ( f.tellg() - buffer.size() ) + found;

            ///---------------------------------------------BITRATE
            string strBitRate = b1.to_string().substr(0,4);
            unsigned long lBR = bitset<4>(strBitRate).to_ulong() - 1;

            size_t bitrate = 320;
            if(lBR <= 4)
                bitrate = 32 + lBR*8;
            else if(lBR <= 8)
                bitrate = 64 + (lBR-4)*16;
            else if(lBR <= 12)
                bitrate = 128 + (lBR-8)*32;
            totBitrate += bitrate;//pour la moyenne

            ///-------------------------------------------SAMPLINGRATE
            string strSamplingRate = b1.to_string().substr(4,2);

            size_t samplingRate = 44100;
            if( strSamplingRate == "01") samplingRate = 48000;
            else if( strSamplingRate == "10") samplingRate = 32000;
            totSamplingRate += samplingRate;

            ///---------------------------------------------PADDING
            int CRC = 0;
            if(b0[0]==0)
                CRC = 2;

            int padding = 0;
            if(b1[1]==1)
                padding = 1;

            ///-----------------------------------------------SIZEFRAME
            size_t sizeFrame = (144 * bitrate * 1000/samplingRate + CRC + padding);
//            if(nbFrame%100 == 0)
//                cout<<nbFrame<<" "<<vbr<< " "<<posFrame<<" - "<<bitrate<<" - "<<samplingRate<<" - "<<sizeFrame<<endl;
//            pos.push_back(posFrame);
//            srs.push_back(samplingRate);

            if(!vbr && nbFrame==1)
            {
                _bitrate = bitrate;
                _samplerate = samplingRate;
                seconds = 1152.0 * (_fileSize - posFrame)/sizeFrame/samplingRate;
            }
            else if(!vbr)
                f.seekg(posFrame+sizeFrame, f.beg);
            else
            {
                if(vbrNbFrames>0)
                {
                    if(samplingRate==48000)
                        sr48++;
                    else if(samplingRate==32000)
                        sr32++;
                    else
                        sr44++;
                    f.seekg(posFrame+_fileSize/100, f.beg);
                }
                else
                {
                    seconds += 1152.0/samplingRate;
                    f.seekg(posFrame+sizeFrame, f.beg);
                }
            }
            nbFrame++;
        }while(posFrame < _fileSize && !f.eof() && (vbr || nbFrame==1));

        if(vbr)
        {
            //cout<<"Nb: "<<nbFrame<<" "<<seconds<<endl;
            if(vbrNbFrames>0)
            {
                seconds = 0;
                seconds += 1152.0* ((double)sr48/nbFrame) *(double)vbrNbFrames/48000;
                seconds += 1152.0* ((double)sr32/nbFrame) *(double)vbrNbFrames/32000;
                seconds += 1152.0* ((double)sr44/nbFrame) *(double)vbrNbFrames/44100;
                if(seconds>60)
                    seconds -= 5;
            }
            _bitrate = totBitrate/nbFrame;
            _samplerate = totSamplingRate/nbFrame;
        }
        //tocsv(pos,srs,"samplingrate.csv");
        return round(seconds);
    }
    cerr<<"ID3v2 : ERROR : getDuration : Cannot read file : "<<_path<<endl;
    return 0;
}
unsigned int ID3v2::getMajorVersion()
{
    return _majorVersion;
}
unsigned int ID3v2::toInt(const std::vector <char>& bytes)
{
    return syncIntDecode(bytesToInt(bytes, false));
}
void ID3v2::parse()
{
    _frames.clear();
    ifstream f(_path,ios::binary);
    if(f)
    {
        f.seekg(0,f.end);
        _fileSize = f.tellg();
        f.seekg(0, f.beg);

        vector <char> bytes(3);

        f.read(&bytes[0],bytes.size());
        string testType = toStr(bytes);
        if(testType != "ID3")
        {
            cerr<<"ID3v2 : ERROR : No tag in : "<<_path<<endl;
            _state = Tag::NO_TAG;
            return;
        }
        _type = testType;

        bytes.resize(1);
        f.read(&bytes[0],bytes.size());
        _majorVersion = (unsigned int)bytes[0];

        f.seekg(6);

        bytes.resize(4);
        f.read(&bytes[0],bytes.size());

        unsigned int tagSize = bytesToInt(bytes, false);
        tagSize = syncIntDecode(tagSize);
        _chunkFrames = Chunk(f.tellg(), tagSize);

        if(_majorVersion == 2)
            bytes.resize(3);
        else
            bytes.resize(4);
        while(f.tellg() < tagSize)
        {
            f.read(&bytes[0],bytes.size());//type
            if(bytes[0] == 0)
                break;

            Frame newFrame;
            newFrame.flag = toStr(bytes);

            f.read(&bytes[0],bytes.size());
            unsigned int sizeData = bytesToInt(bytes, false);
            if(_majorVersion >= 4)
                sizeData = syncIntDecode(sizeData);
            if(_majorVersion > 2)
                f.seekg(2, f.cur);
            newFrame.data.resize(sizeData);
            f.read(&newFrame.data[0],newFrame.data.size());
            _frames.push_back(newFrame);
        }
        _posEndTag = f.tellg();

        f.close();
        _state = Tag::OK;
    }
    else
    {
        cerr<<"ID3v2 : ERROR : Cannot read file : "<<_path<<endl;
        _state = Tag::NO_FILE;
    }
}
