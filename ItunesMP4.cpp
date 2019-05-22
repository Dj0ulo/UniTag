#include <fstream>
#include <iostream>
#include <bitset>
#include <sstream>
#include <cstdlib>

#include "Utils.h"
#include "UniTag.h"

using namespace std;
using namespace UniTag;
using namespace utils;

M4::M4(const std::string path) : Tag(path), _chunks(N_CHUNCKS)
{
    parse();
}
std::string M4::parseFrameWidthIndex(const unsigned int index) const
{
    std::string result;
    auto f = _frames[index];

    unsigned int offset = 0;

    const unsigned int dataType = getInt(f.data, offset);
    const unsigned int dataSize = f.data.size();

    offset += 8;

    stringstream ss;
    if(f.flag == getFrameFlag(COVER_ART))
    {
        ss<<"[IMAGE ";
        if(dataType == 14)
            ss<<"PNG";
        else
            ss<<"JPEG";
        ss<<" (size : "<<dataSize - 8<<")]";

        result = ss.str();
    }
    else if(dataType == 0)
    {
        if(f.flag == "trkn")
            ss<<getInt(f.data,offset+3,1)<<"/"<<getInt(f.data,offset+5,1);
        else if(f.flag == "disk")
            ss<<getInt(f.data,offset+3,1)<<"/"<<getInt(f.data,f.data.size()-1,1);
        else if(f.flag == "gnre")
            ss<<getInt(f.data,f.data.size()-1,1);
        else
        {
            for(unsigned int k = offset; k < f.data.size(); k ++)
            {
                if(k > offset)
                    ss<<"/";
                ss<<getInt(f.data,k,1);
            }
        }
        result = ss.str();
    }
    else if(dataType == 1)
    {
        wstring wres = utils::ainsiToUTF32(string(f.data.begin() + offset, f.data.end()));
        for(auto w:wres)
            result.push_back((char)w);
    }

    else if(dataType == 21)
    {
        ss<<std::hex;
        for(unsigned int k = offset; k < f.data.size(); k++)
            ss<<"0x"<<(int)f.data[k]<<" ";
        result = ss.str();
    }
    else
        result = "[UNKNOWN DATA TYPE]";

    return result;
}
std::string M4::get(const std::string flag) const
{
    auto framesIndexes = Tag::getFramesIndexes(flag);
    if(flag == getFrameFlag(GENRE) && framesIndexes.size() == 0)
    {
        framesIndexes = Tag::getFramesIndexes("gnre");
        if(framesIndexes.size() == 0)
            return "";
        return decodeGenre(std::atoi(parseFrameWidthIndex(framesIndexes[0]).c_str()) - 1);
    }


    if(framesIndexes.size() == 0)
        return "";
    return parseFrameWidthIndex(framesIndexes[0]);
}
std::vector <char> M4::getCoverArt() const
{
    auto imgsIndexes = Tag::getFramesIndexes(getFrameFlag(COVER_ART));
    if(imgsIndexes.size() == 0)
        return std::vector <char>();
    auto img = _frames[imgsIndexes[0]].data;
    return vector <char> (img.begin() + 8, img.end());
}
std::string M4::getFrameFlag(const unsigned int flag) const
{
    std::string cr;
    cr.push_back(0xc2a9);
    if(flag == TITLE)
        return cr+"nam";
    else if(flag == ARTIST)
        return cr+"ART";
    else if(flag == ALBUM)
        return cr+"alb";
    else if(flag == ALBUM_ARTIST)
        return "aART";
    else if(flag == DATE)
        return cr+"day";
    else if(flag == GENRE)
        return cr+"gen";
    else if(flag == LABEL)
        return "----:com.apple.iTunes:LABEL";
    else if(flag == DESCRIPTION)
        return "desc";
    else if(flag == ENCODED_BY)
        return cr+"too";
    else if(flag == ENCODER)
        return cr+"too";
    else if(flag == TRACK_NUM)
        return "trkn";
    else if(flag == DISC)
        return "disk";
    else if(flag == COMMENT)
        return cr+"cmt";
    else if(flag == LYRICS)
        return cr+"lyr";
    else if(flag == COVER_ART)
        return "covr";
    return "";
}

void M4::set(const unsigned int flag, const std::string &text)
{
    if(getFrameFlag(flag) == "")
    {
        cerr<<"M4 : set : flag invalid"<<endl;
        return;
    }
    vector <char> data;
    push(data, toBytes(1, false));
    push(data, toBytes(0, false));
    wstring wstr;
    for(auto c:text)
        if(c<0)
            wstr.push_back(0x100 + c);
        else
            wstr.push_back(c);

    push(data, utils::UTF32ToAinsi(wstr));
    Tag::set(getFrameFlag(flag), data);
}

void M4::setCoverArt(const std::vector <char> &img)
{
    unsigned int x,y;
    unsigned int type = getImageSizeWithData(img, x, y);
    if(!type)
        return;
    vector <char> data;

    if(type == 2)//PNG
        push(data, toBytes(14, false));
    else//JPEG
        push(data, toBytes(13, false));
    push(data, toBytes(0, false));
    push(data, img);
    Tag::set(getFrameFlag(COVER_ART), data);
}

void writeSizeInFront(std::vector <char> &atom)
{
    vector <char> bytesForSize = toBytes(4 + atom.size(), false);
    for(int i = bytesForSize.size() - 1; i >= 0; i--)
        atom.insert(atom.begin(), bytesForSize[i]);
}
void M4::saveFrames(std::string savePath)
{
    if(savePath == "")
        savePath = _path;
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

    vector <Chunk> newChunks = _chunks;
    vector <char> newILST;

    push(newILST, "ilst");
    for(Frame f : _frames)
    {
        vector <char> frame;
        if(f.flag.size() == 4)
            push(frame, f.flag);
        else if(f.flag.size() > 0)
        {
            auto v = split(f.flag,':');
            push(frame, v[0]);

            push(frame, toBytes(12 + v[1].size(), false));
            push(frame, "mean");
            push(frame, toBytes(0,false));
            push(frame, v[1]);

            push(frame, toBytes(12 + v[2].size(), false));
            push(frame, "name");
            push(frame, toBytes(0,false));
            push(frame, v[2]);

        }
        push(frame, toBytes(8 + f.data.size(), false));
        push(frame, "data");
        push(frame, f.data);

        writeSizeInFront(frame);
        push(newILST, frame);
    }
    writeSizeInFront(newILST);

    const int diffSize = newILST.size() - _chunks[CHUNK_ILST].size;

    vector <char> newSTCO(buffer.begin() + newChunks[CHUNK_STCO].pos, buffer.begin() + newChunks[CHUNK_STCO].pos + 16);
    const unsigned int nEntries = getInt(buffer, newChunks[CHUNK_STCO].pos + 12, 4);
    for(unsigned int i = 0; i < nEntries; i++)
        push(newSTCO, toBytes(getInt(buffer, newChunks[CHUNK_STCO].pos + 16 + i*4, 4) + diffSize, false));

    newChunks[CHUNK_STCO].size = newSTCO.size();
    newChunks[CHUNK_ILST].size = newILST.size();
    newChunks[CHUNK_META].size += diffSize;
    newChunks[CHUNK_UDTA].size += diffSize;
    newChunks[CHUNK_MOOV].size += diffSize;

    ofstream of(savePath,ios::binary);
    if(of)
    {
//        of.write(&buffer[0], _chunks[CHUNK_ILST].pos);
//        of.write(&newILST[0], newILST.size());
//
//        const unsigned int oldFreeSize = getInt(buffer, _chunks[CHUNK_ILST].end(),4);
//        int newFreeSize = oldFreeSize + _chunks[CHUNK_ILST].size - newILST.size() - 8;
//
//        if(newFreeSize < 0)
//            newFreeSize = 0;
//
//        vector <char> freeSpace(newFreeSize, 0);
//
//        of.write(&toBytes(freeSpace.size(), false)[0], 4);
//        of.write("free", 4);
//        of.write(&freeSpace[0], freeSpace.size());
//
//        const unsigned int pos = _chunks[CHUNK_ILST].end() +  oldFreeSize;
//        of.write(&buffer[pos], buffer.size() - pos);
//
        of.write(&buffer[0], newChunks[CHUNK_MOOV].pos);
        of.write(&toBytes(newChunks[CHUNK_MOOV].size, false)[0], 4);

        of.write(&buffer[newChunks[CHUNK_MOOV].pos + 4], newChunks[CHUNK_STCO].pos - (newChunks[CHUNK_MOOV].pos + 4));
        of.write(&newSTCO[0], newSTCO.size());
        of.write(&buffer[newChunks[CHUNK_STCO].end()], newChunks[CHUNK_UDTA].pos - (newChunks[CHUNK_STCO].end()));

        for(int i = CHUNK_UDTA; i <= CHUNK_META; i++)
        {
            of.write(&toBytes(newChunks[i].size, false)[0], 4);
            of.write(&buffer[newChunks[i].pos + 4], newChunks[i+1].pos - (newChunks[i].pos + 4));
        }

        of.write(&newILST[0], newILST.size());

        of.write(&buffer[_chunks[CHUNK_ILST].end()], buffer.size() - _chunks[CHUNK_ILST].end());
        of.close();
    }
    parse();
}

unsigned int M4::getDuration()
{
    if(_bitrate==0)
        return 0;
    return (_fileSize - _posEndTag)/_bitrate;
}

void M4::parse()
{
    _frames.clear();
    ifstream f(_path,ios::binary);
    if(f)
    {
        f.seekg(0,f.end);
        _fileSize = f.tellg();
        f.seekg(0, f.beg);

        vector <char> bytes(4);

        f.seekg(0, f.beg);

        f.read(&bytes[0],bytes.size());

        unsigned int endHeader = (unsigned int)f.tellg() + (size_t)bytesToInt(bytes, false) - 4;

        f.read(&bytes[0],bytes.size());
        if(toStr(bytes) == "ftyp")
        {
            f.read(&bytes[0],bytes.size());
            _type = toStr(bytes);
        }
        else
        {
            cerr<<"M4 : ERROR : No tag in : "<<_path<<endl;
            _state = Tag::NO_TAG;
            return;
        }

        f.seekg(endHeader);

        do
        {
            f.read(&bytes[0],bytes.size());
            const size_t sizeBlock = (size_t)bytesToInt(bytes,false);
            const size_t endBlock = (unsigned int)f.tellg() + sizeBlock - 4;

            if(sizeBlock != 0)
            {
                f.read(&bytes[0],bytes.size());
                const std::string atomName = toStr(bytes);
                if(atomName == "moov")
                    _chunks[CHUNK_MOOV] = Chunk((unsigned int)f.tellg() - 8, sizeBlock);
                else if(atomName == "trak" || atomName == "mdia" || atomName == "minf" || atomName == "stbl");//get into these atoms
                else if(atomName == "stco")
                {
                    _chunks[CHUNK_STCO] = Chunk((unsigned int)f.tellg() - 8, sizeBlock);
                    f.seekg(endBlock);
                }
                else if(atomName == "udta")
                    _chunks[CHUNK_UDTA] = Chunk((unsigned int)f.tellg() - 8, sizeBlock);
                else if(atomName == "meta")
                    _chunks[CHUNK_META] = Chunk((unsigned int)f.tellg() - 8, sizeBlock);
                else if(atomName == "ilst")
                {
                    _chunks[CHUNK_ILST] = Chunk((unsigned int)f.tellg() - 8, sizeBlock);
                    break;
                }
                else
                    f.seekg(endBlock);//pass these atoms
            }
        }while(f.tellg() < _chunks[CHUNK_MOOV].end());

        ///---start of interesting frames
        while(true)
        {
            if(f.tellg() >= _chunks[CHUNK_ILST].end())
                break;

            bytes.resize(4);

            f.read(&bytes[0],bytes.size());//size of the current frame
            const size_t sizeFrame = bytesToInt(bytes,false);
            const size_t endFrame = (unsigned int)f.tellg() - 4 + sizeFrame;

            if(endFrame >= _fileSize)
                break;

            f.read(&bytes[0],bytes.size());//type
            std::string flag = toStr(bytes);

            for(unsigned int i = f.tellg();i<endFrame;i = f.tellg())
            {
                bytes.resize(4);
                f.read(&bytes[0],bytes.size());
                const size_t chunkSize = bytesToInt(bytes,false);//size of the chunk
                const size_t chunkEnd = (unsigned int)f.tellg() + chunkSize - 4;//end of the chunk

                f.read(&bytes[0],bytes.size());
                const std::string chunkType = toStr(bytes);

                if(chunkType == "data" &&  chunkSize  >= 8)
                {
                    bytes.resize(chunkSize - 8);
                    f.read(&bytes[0],bytes.size());
                    _frames.push_back(Frame(flag, bytes));
                }
                else if(chunkSize >= 12)
                {
                    f.read(&bytes[0],bytes.size());//0000

                    bytes.resize(chunkSize - 12);
                    f.read(&bytes[0],bytes.size());
                    flag += ":" + toStr(bytes);
                }
                f.seekg(chunkEnd);
            }

            f.seekg(endFrame);
            _posEndTag = f.tellg();
        }
        f.close();
        _state = Tag::OK;
    }
    else
    {
        cerr<<"M4 : ERROR : Cannot read file : "<<_path<<endl;
        _state = Tag::NO_FILE;
    }
}
