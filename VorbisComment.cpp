#include <fstream>
#include <iostream>
#include <algorithm>
#include <bitset>
#include <sstream>

#include "Utils.h"
#include "UniTag.h"

using namespace std;
using namespace UniTag;
using namespace utils;

VorbisComment::VorbisComment(const std::string path) : Tag(path), _chunks(N_CHUNCKS)
{
    parse();
}
std::string VorbisComment::parseFrameWidthIndex(const unsigned int index) const
{
    stringstream ss;
    if(_frames[index].flag == getFrameFlag(COVER_ART))
    {
        ss<<"[IMAGE (size : "<<_frames[index].data.size()<<")]";
        return ss.str();
    }
    return toStr(_frames[index].data);
}
std::string VorbisComment::get(const std::string flag) const
{
    auto framesIndexes = Tag::getFramesIndexes(flag);
    if(framesIndexes.size() == 0)
        return "";

    std::string result;
    for(int i = 0; i < framesIndexes.size(); i++)
    {
        if(i!=0)
            result += " & ";
        result += parseFrameWidthIndex(framesIndexes[i]);
    }
    return result;
}
std::vector <char> VorbisComment::getCoverArt() const
{
    auto framesIndexes = Tag::getFramesIndexes(getFrameFlag(COVER_ART));

    if(framesIndexes.size() == 0)
        return std::vector <char>();

    auto img = _frames[framesIndexes[0]].data;
    unsigned int offset = 0;
    unsigned int type = getInt(img, offset);
    offset += 4;
    unsigned int mimetypeSize = getInt(img, offset);
    offset += 4;
    if(mimetypeSize > 40)
    {
        cerr<<"ERROR : Wrong mimetype"<<endl;
        return std::vector <char>();
    }
    string mimetype = string(img.begin() + offset, img.begin() + offset + mimetypeSize);
    offset += mimetypeSize;
    unsigned int descriptionSize = getInt(img, offset);
    offset += 4 + descriptionSize + 4*4;//width in pixels//height in pixels//color depth bits-per-pixels//number of colors used
    unsigned int imgSize = getInt(img, offset);
    offset += 4;

    return vector <char>(img.begin() + offset, img.end());
}
std::string VorbisComment::getFrameFlag(const unsigned int flag) const
{
    if(flag == TITLE)
        return "TITLE";
    else if(flag == ARTIST)
        return "ARTIST";
    else if(flag == ALBUM)
        return "ALBUM";
    else if(flag == ALBUM_ARTIST)
        return "ALBUMARTIST";
    else if(flag == DATE)
        return "DATE";
    else if(flag == GENRE)
        return "GENRE";
    else if(flag == LABEL)
        return "LABEL";
    else if(flag == DESCRIPTION)
        return "DESCRIPTION";
    else if(flag == ENCODED_BY)
        return "ENCODED-BY";
    else if(flag == ENCODER)
        return "ENCODER";
    else if(flag == TRACK_NUM)
        return "TRACKNUMBER";
    else if(flag == DISC)
        return "DISCNUMBER";
    else if(flag == COMMENT)
        return "COMMENT";
    else if(flag == LYRICS)
        return "LYRICS";
    else if(flag == COVER_ART)
        return "METADATA_BLOCK_PICTURE";
    return "";
}

void VorbisComment::set(const unsigned int flag, const std::string &text)
{
    if(getFrameFlag(flag) == "")
    {
        cerr<<"VorbisComment : set : flag invalid"<<endl;
        return;
    }
    vector <char> data;
    push(data, text);
    Tag::set(getFrameFlag(flag), data);
}
void VorbisComment::setCoverArt(const std::vector <char> &img)
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

    push(data, toBytes(3, false));//picture type
    push(data, toBytes(mimetype.size(), false));//mimetype size
    push(data, mimetype);
    push(data, toBytes(0, false));//length of description
    push(data, toBytes(x, false));//width
    push(data, toBytes(y, false));//height

    if(type == 2)
        push(data, toBytes(32, false));//bits-per-pixels
    else
        push(data, toBytes(24, false));//bits-per-pixels

    push(data, toBytes(0, false));
    push(data, toBytes(img.size(), false));
    push(data, img);
    Tag::set(getFrameFlag(COVER_ART), data);
}

void VorbisComment::saveFrames(std::string savePath)
{
    if(savePath == "")
        savePath = _path;
    if(_type == "OggS")
    {
        cout<<"Cannot edit .ogg file :/"<<endl;
    }
    else if(_type == "fLaC")
    {
        auto buffer = getDataFromFile(_path);
        vector <char> newCOMMENT, newIMAGES;

        unsigned int nF = 0, nImg = 0;
        for(Frame f : _frames)
            if(f.flag == getFrameFlag(COVER_ART))
                nImg++;
            else
                nF++;

        push(newCOMMENT, toBytes(nF, true));
        for(Frame f : _frames)
        {
            if(f.flag != getFrameFlag(COVER_ART))
            {
                push(newCOMMENT, toBytes(f.flag.size() + 1 + f.data.size(), true));
                push(newCOMMENT, f.flag + "=");
                push(newCOMMENT, f.data);
            }
        }

        bool wasLast = bitset<8>(buffer[_chunks[_chunks.size()-1].pos - 4])[7];//if the last image was the last chunck

        const int diffSize = newCOMMENT.size() - _chunks[CHUNK_COMMENT_LIST].size;
        ofstream of(savePath, ios::binary);
        if(of)
        {
            of.write(&buffer[0], _chunks[CHUNK_COMMENT].pos - 4);
            if(nImg == 0 && wasLast)
                of.write(&toBytes(bitset<8>(std::string("10000100")).to_ulong(), true)[0], 1);
            else
                of.write(&toBytes(bitset<8>(std::string("00000100")).to_ulong(), true)[0], 1);
            of.write(&toBytes(_chunks[CHUNK_COMMENT].size + diffSize, false)[1], 3);

            of.write(&buffer[_chunks[CHUNK_COMMENT].pos], _chunks[CHUNK_COMMENT].size - _chunks[CHUNK_COMMENT_LIST].size);
            of.write(&newCOMMENT[0], newCOMMENT.size());

            unsigned int i = 0;
            for(Frame f : _frames)
            {
                if(f.flag == getFrameFlag(COVER_ART))
                {
                    i++;
                    if(i == nImg && wasLast)
                        of.write(&toBytes(bitset<8>(std::string("10000110")).to_ulong(), true)[0], 1);
                    else
                        of.write(&toBytes(bitset<8>(std::string("00000110")).to_ulong(), true)[0], 1);

                    of.write(&toBytes(f.data.size(), false)[1], 3);
                    of.write(&f.data[0], f.data.size());
                }
            }

            of.write(&buffer[_chunks[_chunks.size()-1].end()], buffer.size() - _chunks[_chunks.size()-1].end());
            of.close();
        }
    }
    parse();
}

unsigned int VorbisComment::getDuration()
{
    if(_duration != (unsigned int)-1)
        return _duration;
    else
        return 0;
}

void VorbisComment::parse()
{
    _frames.clear();
    ifstream f(_path,ios::binary);
    if(f)
    {
        f.seekg(0,f.end);
        _fileSize = f.tellg();
        f.seekg(0, f.beg);

        vector <char> bytes(4);
        char byte;

        f.read(&bytes[0],4);
        _type = toStr(bytes);

        string flagStart;
        vector <char> testFlagStart;

        unsigned int startVC = 0, endVC = 0;

        if(_type == "OggS")
        {
            flagStart = "vorbis";
            testFlagStart = vector <char>(flagStart.size());

            for(int i=f.tellg();i<600;i++)
            {
                f.seekg(i, f.beg);
                /*f.read(&bytes[0],4);
                if(bytesToInt(bytes)<600)
                    cout<<"pos : "<<f.tellg()-4<<", size : "<<bytesToInt(bytes)<<", end : "<<f.tellg() + bytesToInt(bytes)<<endl;*/

                f.read(&testFlagStart[0],testFlagStart.size());
                if(toStr(testFlagStart) == flagStart)
                {
                    if(_type == "OggS")
                    {
                        f.seekg((unsigned int)f.tellg() - (unsigned int)testFlagStart.size() - 1, f.beg);
                        f.read(&byte,1);
                        f.seekg((unsigned int)f.tellg() + (unsigned int)testFlagStart.size(), f.beg);
                        if(byte == 0x01)
                        {
                            f.read(&bytes[0],4);
                            f.seekg(bytesToInt(bytes, true) + 1, f.cur);

                            f.read(&bytes[0],4);
                            f.seekg(4, f.cur);
                            _samplerate = bytesToInt(bytes, true);

                            f.read(&bytes[0],4);
                            f.seekg(4, f.cur);
                            _bitrate = bytesToInt(bytes, true);
                        }
                        else
                        {
                            f.read(&bytes[0],4);
                            f.seekg(bytesToInt(bytes, true) + 4, f.cur);
                            break;
                        }
                    }
                }
            }
        }
        else if(_type == "fLaC")
        {
            while(1)
            {
                f.read(&byte,1);
                string bitsStr = bitset<8>(byte).to_string();
                bitset<7> otherBits(bitsStr.substr(1));
                unsigned int blockType = otherBits.to_ulong();

                bytes.resize(3);
                f.read(&bytes[0],bytes.size());

                unsigned int sizeBlock = bytesToInt(bytes,false);
                unsigned int endBlock = (unsigned int)f.tellg() + sizeBlock;
                //cout<<"Pos : "<<f.tellg()<<", size : "<<sizeBlock<<", type : "<<blockType<<endl;
                if(blockType == 127)
                    break;
                else if(blockType == 0)//STREAMINFO
                {
                    f.seekg(2 + 2 + 3 + 3,f.cur);

                    bytes.resize(3);
                    f.read(&bytes[0], bytes.size());
                    _samplerate = bitset<20>(bitset<8>(bytes[0]).to_string() +
                                             bitset<8>(bytes[1]).to_string() +
                                             bitset<8>(bytes[2]).to_string().substr(0,4)).to_ulong();

                    string temp = bitset<8>(bytes[2]).to_string().substr(7);

                    bytes.resize(5);
                    f.read(&bytes[0], bytes.size());

                    unsigned long long bitsPerSample = bitset<5>(temp + bitset<8>(bytes[0]).to_string().substr(0,4)).to_ulong() + 1;
                    unsigned long long nSamples = bitset<36>(  bitset<8>(bytes[0]).to_string().substr(4) +
                                                               bitset<8>(bytes[1]).to_string() +
                                                               bitset<8>(bytes[2]).to_string() +
                                                               bitset<8>(bytes[3]).to_string() +
                                                               bitset<8>(bytes[4]).to_string()).to_ulong();
                    _duration = nSamples/_samplerate;
                    _bitrate = bitsPerSample*_samplerate;

                }
                else if(blockType == 1);//PADDING
                else if(blockType == 4)//VORBIS_COMMENT
                {
                    _chunks[CHUNK_COMMENT] = Chunk(f.tellg(), sizeBlock);

                    bytes.resize(4);
                    f.read(&bytes[0], bytes.size());
                    f.seekg(bytesToInt(bytes,true), f.cur);//ref

                    _chunks[CHUNK_COMMENT_LIST] = Chunk(f.tellg(), _chunks[CHUNK_COMMENT].end() - f.tellg());

                    f.read(&bytes[0], bytes.size());
                    //const unsigned int nFrames = bytesToInt(bytes,true);//useless

                    startVC = f.tellg();
                    endVC = endBlock;
                }
                else if(blockType == 6)//PICTURE
                {
                    _chunks.push_back(Chunk(f.tellg(), sizeBlock));
                    bytes.resize(sizeBlock);
                    f.read(&bytes[0], bytes.size());
                    _frames.push_back(Frame(getFrameFlag(COVER_ART), bytes));
                }
                f.seekg(endBlock, f.beg);

                if(bitsStr[0]=='1')//last block
                    break;
            }
            _posEndTag = f.tellg();
            if(_duration == 0)
                _duration = (_fileSize-_posEndTag)/2.0 * 1/1.22/_samplerate;//un peu au pif
        }
        else
        {
            cerr<<"Vorbis Comment : ERROR : No tag in : "<<_path<<endl;
            _state = Tag::NO_TAG;
            return;
        }

        if(startVC != 0)
            f.seekg(startVC, f.beg);
        bytes.resize(4);

        while(1)
        {
            if(endVC != 0 && f.tellg()>=endVC)
                break;

            f.read(&bytes[0],4);
            const unsigned int sizeFrame = bytesToInt(bytes, true);

            if(sizeFrame == 1)//bit de fin
                break;
            else if(sizeFrame > 1000000)
            {
                cerr<<"Vorbis Comment : ERROR : frame : invalid size : (pos : "<<f.tellg()<<")"<<endl;
                break;
            }
            else if(sizeFrame == 0)
            {
                cerr<<"Vorbis Comment : ERROR : framing bit : not set : (pos : "<<f.tellg()<<")"<<endl;
                break;
            }

            vector <char> frame(sizeFrame);
            f.read(&frame[0],sizeFrame);

            auto it = find(frame.begin(),frame.end(),'=');
            _frames.push_back(Frame(std::string(frame.begin(), it), vector <char>(it+1, frame.end())));
        }
        f.close();
        _state = Tag::OK;
    }
    else
    {
        cerr<<"Vorbis Comment : ERROR : Cannot read file : "<<_path<<endl;
        _state = Tag::NO_FILE;
    }
}
