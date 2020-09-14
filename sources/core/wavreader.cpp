//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#include "wavreader.h"
#include <QVariant>
#include <QVariantList>
#include <QDateTime>
#include <QDebug>

WavReader::WavReader(QObject* parent) :
    QObject(parent),
    mWavOpened(false),
    mChannel0(nullptr),
    mChannel1(nullptr)
{

}

WavReader::ErrorCodesEnum WavReader::setFileName(const QString& fileName)
{
    if (!mWavOpened) {
        mWavFile.setFileName(fileName);
        return Ok;
    }
    return AlreadyOpened;
}

WavReader::ErrorCodesEnum WavReader::open()
{
    if (!mWavOpened) {
        mWavOpened = mWavFile.open(QIODevice::ReadOnly);
        if (mWavOpened) {
            mWavOpened = false;
            QByteArray buf;

            {
                const WavHeader* riffHeader = readData<WavHeader>(buf);
                if (!riffHeader || riffHeader->chunk.chunkId != riffId || riffHeader->riffType != waveId) {
                    mWavFile.close();
                    return InvalidWavFormat;
                }
            }

            {
                const WavFmt* fmtHeader = readData<WavFmt>(buf);
                if (!fmtHeader || fmtHeader->chunk.chunkId != fmt_Id) {
                    mWavFile.close();
                    return InvalidWavFormat;
                }

                if (fmtHeader->compressionCode != 1 || (fmtHeader->numberOfChannels != 1 && fmtHeader->numberOfChannels != 2) ||
                   (fmtHeader->significantBitsPerSample != 8 && fmtHeader->significantBitsPerSample != 16)) {
                    mWavFile.close();
                    return UnsupportedWavFormat;
                }

                mWavFormatHeader = *fmtHeader;
            }

            {
                const WavChunk* dataHeader = readData<WavChunk>(buf);
                if (!dataHeader || dataHeader->chunkId != dataId) {
                    mWavFile.close();
                    return InvalidWavFormat;
                }

                mCurrentChunk = *dataHeader;
            }

            mWavOpened = true;
            return Ok;
        }

        return CantOpen;
    }

    return AlreadyOpened;
}

WavReader::QVectorBase* WavReader::createVector(size_t bytesPerSample, size_t size)
{
    WavReader::QVectorBase* r;
    if (bytesPerSample == 1) {
        r = new WavReader::QWavVector<uint8_t>(static_cast<int>(size));
    }
    else {
        r = new WavReader::QWavVector<int16_t>(static_cast<int>(size));
    }

    return r;
}

WavReader::ErrorCodesEnum WavReader::read()
{
    if (!mWavOpened) {
        return NotOpened;
    }

    QByteArray buf { mWavFile.read(mCurrentChunk.chunkDataSize) };
    if (buf.size() < static_cast<int>(mCurrentChunk.chunkDataSize)) {
        return InsufficientData;
    }

    mChannel0.reset(nullptr);
    mChannel1.reset(nullptr);

    size_t bytesPerSample = mWavFormatHeader.significantBitsPerSample / 8;
    size_t numSamples = mCurrentChunk.chunkDataSize / (bytesPerSample * mWavFormatHeader.numberOfChannels);

    mChannel0.reset(createVector(bytesPerSample, numSamples));
    if (mWavFormatHeader.numberOfChannels == 2) {
        mChannel1.reset(createVector(bytesPerSample, numSamples));
    }

    size_t bufIndex = 0;
    QVector<size_t> channelBufIndex(mWavFormatHeader.numberOfChannels, 0);
    for (size_t i = 0; i < numSamples; ++i) {
        for (int channelNum = 0; channelNum < mWavFormatHeader.numberOfChannels; ++channelNum) {
            auto& channel = channelNum == 0 ? mChannel0 : mChannel1;
            auto& cbi = channelBufIndex[channelNum];

            WavSample sample = bytesPerSample == 1 ? getSample<uint8_t>(buf, bufIndex) : getSample<int16_t>(buf, bufIndex);
            if (bytesPerSample == 1) {
                (static_cast<QWavVector<uint8_t>*>(channel.get()))->operator[](static_cast<uint>(cbi)) = *(reinterpret_cast<uint8_t*>(sample.sample));
            }
            else {
                (static_cast<QWavVector<int16_t>*>(channel.get()))->operator[](static_cast<uint>(cbi)) = *(reinterpret_cast<int16_t*>(sample.sample));
            }
            ++cbi;
        }
    }

    return Ok;
}

uint WavReader::getNumberOfChannels() const
{
    return mWavOpened ? mWavFormatHeader.numberOfChannels : 0;
}

uint32_t WavReader::getSampleRate() const
{
    return mWavOpened ? mWavFormatHeader.sampleRate : 0;
}

uint WavReader::getBytesPerSample() const
{
    return mWavOpened ? mWavFormatHeader.significantBitsPerSample / 8 : 0;
}

WavReader::QVectorBase* WavReader::getChannel0() const
{
    return mChannel0.get();
}

WavReader::QVectorBase* WavReader::getChannel1() const
{
    return mChannel1.get();
}

WavReader::ErrorCodesEnum WavReader::close()
{
    if (!mWavOpened) {
        return NotOpened;
    }

    mWavFile.close();
    return Ok;
}

void WavReader::saveWaveform() const
{
    QFile f(QString("waveform_%1.wfm").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh-mm-ss.zzz")));
    f.open(QIODevice::ReadWrite);

    const auto& ch = *getChannel0();
    QByteArray b;
    for (auto i = 0; i < ch.size(); ++i) {
        const uint16_t val = ch.get(i);
        b.append(reinterpret_cast<const char *>(&val), sizeof(val));
    }
    f.write(b);
    f.close();
}

void WavReader::repairWaveform()
{
    {
        QVectorBase* v;
        if (mWavFormatHeader.significantBitsPerSample == 8) {
            v = new QWavVector<uint8_t>(*(static_cast<QWavVector<uint8_t>*>(mChannel0.get())));
        }
        else {
            v = new QWavVector<int16_t>(*(static_cast<QWavVector<int16_t>*>(mChannel0.get())));
        }

        mStoredChannel.reset(v);
    }

    for (auto i = 0; i < mChannel0->size(); ++i) {
        mChannel0->set(i, mChannel0->get(i) - 1000);
    }
}

void WavReader::restoreWaveform()
{
    for (auto i = 0; i < mStoredChannel->size(); ++i) {
        mChannel0->set(i, mStoredChannel->get(i));
    }
}

WavReader::~WavReader()
{
    if (mWavOpened) {
        mWavFile.close();
    }
}

WavReader* WavReader::instance()
{
    static WavReader w;
    return &w;
}
