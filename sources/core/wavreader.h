//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
// YouTube channel: https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg
// YouTube channel e-mail: computerenthusiasttips@mail.ru
//
// Code modification and distribution of any kind is not allowed without direct
// permission of the Author.
//*******************************************************************************

#ifndef WAVREADER_H
#define WAVREADER_H

#include <QObject>
#include <QFile>
#include <QMap>
#include <QSharedPointer>
#include "sources/defines.h"

class WavReader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint numberOfChannels READ getNumberOfChannels NOTIFY numberOfChannelsChanged)

private:
//Disable struct alignment
#pragma pack(push, 1)
    struct Int24 {
        uint8_t b0;
        uint8_t b1;
        uint8_t b2;
    };

    struct WavChunk
    {
        uint32_t chunkId;
        uint32_t chunkDataSize;
    };

    struct WavHeader
    {
        WavChunk chunk;
        uint32_t riffType;
    };

    struct WavFmt
    {
        WavChunk chunk;
        uint16_t compressionCode;
        uint16_t numberOfChannels;
        uint32_t sampleRate;
        uint32_t avgBytesPerSecond;
        uint16_t blockAlign;
        uint16_t significantBitsPerSample;
    };

    template <typename T>
    struct WavMonoSample
    {
        T channel;
    };

    template <typename T>
    struct WavStereoSample
    {
        T leftChannel;
        T rightChannel;
    };
#pragma pack(pop)

    const uint32_t riffId = 0x46464952; //"RIFF"
    const uint32_t waveId = 0x45564157; //"WAVE"
    const uint32_t fmt_Id = 0x20746D66; //"fmt "
    const uint32_t dataId = 0x61746164; //"data"

    template <typename T>
    const T* readData(QByteArray& buf) {
        buf = mWavFile.read(sizeof(T));
        if ((unsigned) buf.size() < sizeof(T)) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(buf.data());
    }

    template <typename T>
    T* getData(QByteArray& buf, size_t& bufIndex) const {
        T* res = reinterpret_cast<T*>(buf.data() + bufIndex);
        bufIndex += sizeof(T);
        return res;
    }

    template <typename T>
    void appendData(QByteArray& buf, const T& data) const {
        buf.append(reinterpret_cast<const char*>(&data), sizeof(T));
    }

    uint8_t* getData(QByteArray& buf, size_t& bufIndex, uint dataSize) const {
        if (dataSize == 0) {
            return nullptr;
        }

        auto t = getData<uint8_t>(buf, bufIndex);
        bufIndex += dataSize - 1;
        return t;
    }

    QWavVectorType getSample(QByteArray& buf, size_t& bufIndex, uint dataSize, uint compressionCode) const;
    QWavVector* createVector(size_t bytesPerSample, size_t size);
    unsigned calculateOnesInByte(uint8_t n);

    WavFmt mWavFormatHeader;
    WavChunk mCurrentChunk;
    bool mWavOpened;
    QFile mWavFile;
    QSharedPointer<QWavVector> mChannel0;
    QSharedPointer<QWavVector> mChannel1;
    QMap<uint, QSharedPointer<QWavVector>> mStoredChannels;

protected:
    explicit WavReader(QObject* parent = nullptr);

public:
    enum ErrorCodesEnum {
        Ok,
        AlreadyOpened,
        CantOpen,
        NotOpened,
        InvalidWavFormat,
        UnsupportedWavFormat,
        InsufficientData,
        EndOfBuffer
    };
    Q_ENUM(ErrorCodesEnum)

    virtual ~WavReader() override;

    uint getNumberOfChannels() const;
    uint32_t getSampleRate() const;
    uint getBytesPerSample() const;
    QSharedPointer<QWavVector> getChannel0() const;
    QSharedPointer<QWavVector> getChannel1() const;

    ErrorCodesEnum setFileName(const QString& fileName);
    ErrorCodesEnum open();
    ErrorCodesEnum read();
    ErrorCodesEnum close();

    void loadTap(const QString& fname);
    void loadWaveform(const QString& fname);
    void saveWaveform(const QString& fname = QString()) const;
    void shiftWaveform(uint chNum);
    void storeWaveform(uint chNum);
    void restoreWaveform(uint chNum);
    void repairWaveform(uint chNum);
    void normalizeWaveform(uint chNum);
    void normalizeWaveform2(uint chNum);

    static WavReader* instance();

signals:
    void numberOfChannelsChanged();
};

#endif // WAVREADER_H
