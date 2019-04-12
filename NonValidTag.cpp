#include "UniTag.h"

using namespace std;
using namespace UniTag;


NonValidTag::NonValidTag(const std::string path) : Tag(path)
{
    _state = Tag::NO_TAG;
}
std::string NonValidTag::parseFrameWidthIndex(const unsigned int index) const
{
    return "";
}
std::string NonValidTag::get(const std::string flag) const
{
    return "";
}
std::vector <char> NonValidTag::getCoverArt() const
{
    return std::vector <char>();
}
std::string NonValidTag::getFrameFlag(const unsigned int flag) const
{
    return "";
}
void NonValidTag::set(const unsigned int flag, const std::string &text)
{

}
void NonValidTag::setCoverArt(const std::vector <char> &img)
{

}
void NonValidTag::saveFrames(std::string savePath)
{

}
unsigned int NonValidTag::getDuration()
{
    return 0;
}

void NonValidTag::parse()
{

}
