# UniTag
C++ library to read and edit ID3v2, Vorbis Comment and M4 tags
- ID3v2.2, ID3v2.3, ID3v2.4 are supported by this library. These are the tags that can be found in *.mp3 files
- Vorbis Comment is the tag that is in *.flac and *.ogg files
- M4 is in *.m4a and *.m4p which are the types of files that contain AAC data

## Fonctionalities
You can read and edit any type of frame but this library is optimized for the :
- title
- artist
- album
- album artist
- year
- genre
- label
- despcription
- track number
- disc number
- lyrics
- album cover
- encoder
- ...

You can also get the bitrate, the samplingrate and the duration of the file although this is sometimes not very accurate.

## Example of use
```C++
UniTag::Tag* tag = UniTag::getTag("file.mp3");

if(tag->getState() != UniTag::Tag::OK)
    return false;
    
unsigned int duration = tag->getDuration();

std::string title = tag->get(UniTag::TITLE);
std::string artist = tag->get(UniTag::ARTIST);

tag->set(UniTag::ALBUM, "New album name");
```
