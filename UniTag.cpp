#include <algorithm>
#include <iostream>

#include "Utils.h"
#include "UniTag.h"


using namespace std;
using namespace UniTag;
using namespace utils;

UniTag::Frame::Frame()
{

}
UniTag::Frame::Frame(const std::string &flag_, const std::vector <char> &data_)
{
    data = data_;
    flag = flag_;
}
bool UniTag::Frame::operator == (const std::string& flag_) const
{
    return utils::absEquals(flag, flag_);
}

UniTag::Chunk::Chunk()
{
    pos = 0;
    size = 0;
}
UniTag::Chunk::Chunk(const unsigned int pos_, const unsigned int size_)
{
    pos = pos_;
    size = size_;
}
unsigned int UniTag::Chunk::end() const
{
    return pos + size;
}

Tag::Tag(const std::string path)
{
    _path = path;
    _state = Tag::INIT;
    _posEndTag = 0;
    _fileSize = 0;
    _bitrate = 0;
    _samplerate = 0;
    _duration = (unsigned int)-1;
}
std::string Tag::getPath() const
{
    return _path;
}
std::vector <unsigned int> Tag::getFramesIndexes(const std::string flag) const
{
    std::vector <unsigned int>  result;
    auto it = _frames.begin();
    while( (it = find(it,_frames.end(),flag)) != _frames.end() )
    {
        result.push_back(it - _frames.begin());
        it++;
    }
    return result;
}
unsigned int Tag::getFramesNumber() const
{
    return _frames.size();
}
std::string Tag::get(const unsigned int flag) const
{
    return get(getFrameFlag(flag));
}
std::string Tag::getType() const {return _type;}
unsigned int Tag::getBitRate() const {return _bitrate;}
unsigned int Tag::getSamplingRate() const {return _samplerate;}
unsigned int Tag::getState() const {return _state;}
size_t Tag::getSize() const {return _posEndTag;}
size_t Tag::getFileSize() const {return _fileSize;}

void Tag::deleteFrame(const unsigned int flag)
{
    for(unsigned int i=0;i<_frames.size();i++)
        if(_frames[i].flag == getFrameFlag(flag))
        {
            _frames.erase(_frames.begin() + i);
            i--;
        }
}
void Tag::set(const std::string &flag, const std::vector <char> &data)
{
    auto it = find(_frames.begin(), _frames.end(), flag);
    if(it == _frames.end())
        _frames.push_back(Frame(flag, data));
    else
        it->data = data;
}
void Tag::printAllFrames() const
{
    cout<<"--------------------"<<endl;
    cout<<"> Path : "<<_path<<endl;
    cout<<"> Tag type : "<<getType();
    if(getType() == "ID3")
        cout<<" 2."<<((UniTag::ID3v2*)this)->getMajorVersion();
    cout<<endl;
    cout<<"There is "<<_frames.size()<<" frame(s)"<<endl;
    for(int i=0;i<_frames.size();i++)
        cout<<"> "<<_frames[i].flag<<" : "<<parseFrameWidthIndex(i)<<endl;
    cout<<"--------------------"<<endl;
}
///////////////////////////////////////////////////////////////////////////////////////////
std::vector <std::string> UniTag::getReadableFileType()
{
    return {"ogg", "flac", "m4a", "m4p", "3gp", "mp3"};
}
bool UniTag::isFileTypeReadable(const std::string &path)
{
    auto types = getReadableFileType();
    std::string ext = getExtension(path);
    std::transform(ext.begin(), ext.end(),ext.begin(), ::tolower);
    if (find(types.begin(),types.end(),ext)!=types.end())
        return true;
    return false;
}

UniTag::Tag *UniTag::getTag(const std::string &path)
{
    std::string ext = getExtension(path);
    std::transform(ext.begin(), ext.end(),ext.begin(), ::tolower);
    if(ext=="ogg" || ext=="flac")
        return new VorbisComment(path);
    else if(ext=="m4a" || ext=="m4p" || ext=="3gp")
        return new M4(path);
    else if(ext=="mp3")
        return new ID3v2(path);
    cerr<<"UniTag : ERROR : "<<path<<" : INVALID file type"<<endl;
    return new NonValidTag(path);
}
