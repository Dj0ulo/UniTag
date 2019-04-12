#ifndef VORBISCOMMENT_H_INCLUDED
#define VORBISCOMMENT_H_INCLUDED

#include <string>
#include <vector>

#include "Utils.h"

namespace UniTag
{
    enum{
        TITLE,
        ARTIST,
        ALBUM,
        ALBUM_ARTIST,
        DATE,
        GENRE,
        LABEL,
        DESCRIPTION,
        ENCODED_BY,
        ENCODER,
        TRACK_NUM,
        DISC,
        COMMENT,
        LYRICS,
        COVER_ART
    };

    struct Frame
    {
        std::string flag;
        std::vector <char> data;

        Frame();
        Frame(const std::string &flag_, const std::vector <char> &data_);
        bool operator == (const std::string& flag_) const;
    };

    struct Chunk
    {
        unsigned int pos;
        unsigned int size;

        Chunk();
        Chunk(const unsigned int pos_, const unsigned int size_);
        unsigned int end() const;
    };

    class Tag
    {
    public:
        Tag(const std::string path);

        std::string getPath() const;
        std::vector <unsigned int> getFramesIndexes(const std::string flag) const;
        unsigned int getFramesNumber() const;

        virtual std::string parseFrameWidthIndex(const unsigned int index) const = 0;
        virtual std::string get(const std::string flag) const = 0;
        std::string get(const unsigned int flag) const;

        std::string getType() const;
        unsigned int getBitRate() const;
        unsigned int getSamplingRate() const;
        unsigned int getState() const;
        size_t getSize() const;
        size_t getFileSize() const;

        virtual std::vector <char> getCoverArt() const = 0;

        virtual std::string getFrameFlag(const unsigned int flag) const = 0;

        virtual void set(const unsigned int flag, const std::string &text) = 0;
        virtual void setCoverArt(const std::vector <char> &img) = 0;

        void deleteFrame(const unsigned int flag);

        virtual void saveFrames(std::string savePath = "") = 0;

        void printAllFrames() const;

        virtual unsigned int getDuration() = 0;

        enum{INIT, OK, NO_TAG, NO_FILE};

    protected:
        void set(const std::string &flag, const std::vector <char> &data);

        virtual void parse() = 0;

        std::string _path, _type;

        unsigned int _samplerate;
        unsigned int _bitrate;
        unsigned int _state;
        unsigned int _duration;
        size_t _posEndTag, _fileSize;

        std::vector <Frame> _frames;
    };

    class NonValidTag : public Tag
    {
    public:
        NonValidTag(const std::string path);
        std::string parseFrameWidthIndex(const unsigned int index) const;
        std::string get(const std::string flag) const;

        std::vector <char> getCoverArt() const;

        std::string getFrameFlag(const unsigned int flag) const;

        void set(const unsigned int flag, const std::string &text);
        void setCoverArt(const std::vector <char> &img);

        void saveFrames(std::string savePath = "");

        unsigned int getDuration();
    private:
        void parse();

    };

    class VorbisComment : public Tag
    {
    public:
        VorbisComment(const std::string path);

        std::string parseFrameWidthIndex(const unsigned int index) const;
        std::string get(const std::string flag) const;

        std::vector <char> getCoverArt() const;
        std::string getFrameFlag(const unsigned int flag) const;

        void set(const unsigned int flag, const std::string &text);
        void setCoverArt(const std::vector <char> &img);

        void saveFrames(std::string savePath = "");
        unsigned int getDuration();
    private:
        void parse();

        enum{
            CHUNK_COMMENT_LIST,
            CHUNK_COMMENT,
            N_CHUNCKS
        };
        std::vector <Chunk> _chunks;
    };

    class M4 : public Tag
    {
    public:
        M4(const std::string path);

        std::string parseFrameWidthIndex(const unsigned int index) const;
        std::string get(const std::string flag) const;

        std::vector <char> getCoverArt() const;
        std::string getFrameFlag(const unsigned int flag) const;

        void set(const unsigned int flag, const std::string &text);
        void setCoverArt(const std::vector <char> &img);

        void saveFrames(std::string savePath = "");
        unsigned int getDuration();
    private:
        void parse();

        enum{
            CHUNK_MOOV,
            CHUNK_UDTA,
            CHUNK_META,
            CHUNK_ILST,
            CHUNK_STCO,
            N_CHUNCKS
        };
        std::vector <Chunk> _chunks;
    };

    class ID3v2 : public Tag
    {
    public:
        ID3v2(const std::string path);

        std::string parseFrameWidthIndex(const unsigned int index) const;
        std::string get(const std::string flag) const;

        std::vector <char> getCoverArt() const;
        std::string getFrameFlag(const unsigned int flag) const;

        void set(const unsigned int flag, const std::string &text);
        void setCoverArt(const std::vector <char> &img);

        void saveFrames(std::string savePath = "");
        unsigned int getDuration();
        unsigned int getMajorVersion();
    private:
        static unsigned int toInt(const std::vector <char>& bytes);
        static std::string convertOldFrameFlag(const std::string oldId);
        void updateFrameToNewMajorVersion();
        std::vector <char> textToData(const std::string &flag,const std::string &text);
        void set(const std::string &flag, const std::string &text);
        void parse();

        unsigned int _majorVersion;
        Chunk _chunkFrames;
    };

    std::vector <std::string> getReadableFileType();
    bool isFileTypeReadable(const std::string &path);
    Tag* getTag(const std::string &path);
    std::string decodeGenre(unsigned int code);
}



#endif // VORBISCOMMENT_H_INCLUDED
