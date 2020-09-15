//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#ifndef WAVREADER_H
#define WAVREADER_H

#include <QObject>
#include <QFile>

class WavReader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint numberOfChannels READ getNumberOfChannels NOTIFY numberOfChannelsChanged)

public:
    class QVectorBase {
    public:
        QVectorBase() = default;
        virtual ~QVectorBase() = default;

        virtual void insert(uint idx, int16_t val) = 0;
        virtual void remove(uint idx) = 0;
        virtual void set(uint idx, int16_t val) = 0;
        virtual int16_t get(uint idx) const = 0;
        virtual int32_t size() const = 0;
    };

    template <typename T>
    class QWavVector : public QVector<T>, public QVectorBase {
    public:
        QWavVector(int size) : QVector<T>(size) { }
        QWavVector(const QWavVector<T>& v) : QVector<T>(v) { }
        ~QWavVector() = default;

        virtual void insert(uint idx, int16_t val) {
            QVector<T>::insert(idx, val);
        }

        virtual void remove(uint idx) {
            QVector<T>::removeAt(idx);
        }

        virtual void set(uint idx, int16_t val) {
            QVector<T>::operator [](idx) = val;
        }

        virtual int16_t get(uint idx) const {
            return QVector<T>::operator [](idx);
        }

        virtual int32_t size() const {
            return QVector<T>::size();
        }
    };

private:
//Disable struct alignment
#pragma pack(push, 1)
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

    struct WavSample {
        uint8_t* sample;
    };

    const uint32_t riffId = 0x46464952; //"RIFF"
    const uint32_t waveId = 0x45564157; //"WAVE"
    const uint32_t fmt_Id = 0x20746D66; //"fmt "
    const uint32_t dataId = 0x61746164; //"data"

    template <typename T>
    const T* readData(QByteArray& buf) {
        buf = mWavFile.read(sizeof(T));
        if (buf.size() < sizeof(T)) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(buf.data());
    }

    template <typename T>
    const T* getData(QByteArray& buf, size_t& bufIndex) {
        const T* res = reinterpret_cast<const T*>(buf.data() + bufIndex);
        bufIndex += sizeof(T);
        return res;
    }

    template <typename T>
    WavSample getSample(QByteArray& buf, size_t& bufIndex) {
        WavSample r;
        r.sample = reinterpret_cast<uint8_t*>(const_cast<WavMonoSample<T>*>(getData<WavMonoSample<T>>(buf, bufIndex)));

        return r;
    }

    template <typename T>
    void restore() {
        QVector<T>& v = *(static_cast<QWavVector<T>*>(mChannel0.get()));

    }

    QVectorBase* createVector(size_t bytesPerSample, size_t size);

    WavFmt mWavFormatHeader;
    WavChunk mCurrentChunk;
    bool mWavOpened;
    QFile mWavFile;
    QScopedPointer<QVectorBase> mChannel0;
    QScopedPointer<QVectorBase> mChannel1;
    QScopedPointer<QVectorBase> mStoredChannel;

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
    QVectorBase* /*const*/ getChannel0() const;
    QVectorBase* /*const*/ getChannel1() const;

    ErrorCodesEnum setFileName(const QString& fileName);
    ErrorCodesEnum open();
    ErrorCodesEnum read();
    ErrorCodesEnum close();

    void saveWaveform() const;
    void repairWaveform();
    void restoreWaveform();

    static WavReader* instance();

signals:
    void numberOfChannelsChanged();
};

#endif // WAVREADER_H
