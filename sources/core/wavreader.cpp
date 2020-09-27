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

                if ((fmtHeader->compressionCode != 1 && fmtHeader->compressionCode != 3) || (fmtHeader->numberOfChannels != 1 && fmtHeader->numberOfChannels != 2) ||
                   (fmtHeader->significantBitsPerSample != 8 && fmtHeader->significantBitsPerSample != 16 && fmtHeader->significantBitsPerSample != 24 && fmtHeader->significantBitsPerSample != 32)
                   ) {
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

QWavVectorType WavReader::getSample(QByteArray& buf, size_t& bufIndex, uint dataSize, uint compressionCode)
{
       QWavVectorType r { };
       void* v;
       v = reinterpret_cast<void*>(getData(buf, bufIndex, dataSize));

       switch (dataSize) {
           case 1:
               r = (*reinterpret_cast<uint8_t*>(v) - 128) * 258.;
               break;

           case 2:
               r = *reinterpret_cast<int16_t*>(v);
               break;

           case 3:
               {
                   Int24* t;
                   t = reinterpret_cast<Int24*>(v);
                   r = ((t->b2 << 16) | (t->b1 << 8) | t->b0);
               }
               break;

           case 4:
               r = compressionCode == 3 ? *reinterpret_cast<float*>(v) : *reinterpret_cast<int32_t*>(v);
               break;

           default:
               qDebug() << "Unsupported data size";
               break;
       }

       return r;
}

QWavVector* WavReader::createVector(size_t bytesPerSample, size_t size)
{
    Q_UNUSED(bytesPerSample)
    return new QWavVector(static_cast<int>(size));
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

            channel->operator[](cbi) = getSample(buf, bufIndex, bytesPerSample, mWavFormatHeader.compressionCode);
            ++cbi;
        }
    }

    emit numberOfChannelsChanged();
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

QWavVector* WavReader::getChannel0() const
{
    return mChannel0.get();
}

QWavVector* WavReader::getChannel1() const
{
    return mChannel1.get();
}

WavReader::ErrorCodesEnum WavReader::close()
{
    if (!mWavOpened) {
        return NotOpened;
    }
    mWavOpened = false;

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
        const QWavVectorType val = ch[i];
        b.append(reinterpret_cast<const char *>(&val), sizeof(val));
    }
    f.write(b);
    f.close();
}

void WavReader::shiftWaveform(uint chNum)
{
    if (chNum >= mWavFormatHeader.numberOfChannels) {
        qDebug() << "Channel number exceeds number of channels";
        return;
    }

    auto& storedCh = mStoredChannels[chNum];
    auto& ch = chNum == 0 ? mChannel0 : mChannel1;
    storedCh.reset(new QWavVector(*ch.get()));
    for (auto i = 0; i < mChannel0->size(); ++i) {
        ch->operator[](i) -= 1300;
    }
}

void WavReader::storeWaveform(uint chNum)
{
    if (chNum >= mWavFormatHeader.numberOfChannels) {
        qDebug() << "Channel number exceeds number of channels";
        return;
    }

    auto& ch = chNum == 0 ? mChannel0 : mChannel1;
    mStoredChannels[chNum].reset(new QWavVector(*ch));
}

void WavReader::restoreWaveform(uint chNum)
{
    if (chNum >= mWavFormatHeader.numberOfChannels) {
        qDebug() << "Channel number exceeds number of channels";
        return;
    }

    auto& ch = chNum == 0 ? mChannel0 : mChannel1;
    auto& storedCh = mStoredChannels[chNum];
    if (storedCh.isNull()) {
        storedCh.reset(new QWavVector());
    }

    ch.reset(new QWavVector(*mStoredChannels[chNum]));
}

void WavReader::normalizeWaveform(uint chNum)
{
    if (chNum >= mWavFormatHeader.numberOfChannels) {
        qDebug() << "Channel number exceeds number of channels";
        return;
    }

    auto& ch = *(chNum == 0 ? mChannel0 : mChannel1).get();
    auto haveSameSign = [](QWavVectorType o1, QWavVectorType o2) {
        return lessThanZero(o1) == lessThanZero(o2);
    };

    //Trying to find a sine
    auto bIt = ch.begin();

    while (bIt != ch.end()) {
        //qDebug() << "bIt: " << std::distance(ch.begin(), bIt) << "; end: " << std::distance(ch.begin(), ch.end());
//        auto itPos = std::distance(ch.begin(), bIt);
//        auto prc = ((float) itPos / ch.size()) * 100;
//        qDebug() << "Total: " << (int) prc;

        auto prevIt = bIt;
        auto it = std::next(prevIt);
        QMap<int, QVector<QWavVectorType>::iterator> peaks {{0, bIt}};

        for (int i = 1; i < 4; ++i) {
            //down-to-up part when i == 1, 3
            //up-to-down part when i == 2
            bool finished = true;
            for (; it != ch.end();) {
                if (haveSameSign(*prevIt, *it)) {
                    if ((i == 2 ? std::abs(*prevIt) >= std::abs(*it) : std::abs(*prevIt) <= std::abs(*it))) {
                        prevIt = it;
                        it = std::next(it);
                    }
                    else {
                        peaks[i] = it;
                        break;
                    }
                }
                else {
                    bIt = it;
                    finished = false;
                    it = ch.end();
                }
            }

            //Signal crosses zero level - not ours case
            if (it == ch.end()) {
                if (finished) {
                    bIt = it;
                }
                break;
            }
        }

        //Looks like we've found a sine, normalizing it
        if (it != ch.end()) {
            bIt = it;
            for (auto i = 0; i < 3; ++i) {
                auto middlePoint = std::distance(peaks[i], peaks[i + 1]) / 2;
                auto middleIt = std::next(peaks[i], middlePoint);
                auto middleVal = *middleIt;
                auto incVal = QWavVectorType(-1) * middleVal;
                std::for_each(i == 1 ? middleIt : peaks[i], i == 1 ? peaks[i + 1] : middleIt, [incVal](QWavVectorType& i) {
//                    qDebug() << "Old: " << i << "; new: " << (i+incVal);
                    i += incVal;
                });
            }
        }
    }
}

void WavReader::normalizeWaveform2(uint chNum)
{
    if (chNum >= mWavFormatHeader.numberOfChannels) {
        qDebug() << "Channel number exceeds number of channels";
        return;
    }

    auto& ch = *(chNum == 0 ? mChannel0 : mChannel1).get();
    auto haveSameSign = [](QWavVectorType o1, QWavVectorType o2) {
        return lessThanZero(o1) == lessThanZero(o2);
    };

    //Trying to find a sine
    auto bIt = ch.begin();

    while (bIt != ch.end()) {
        auto prevIt = bIt;
        auto it = std::next(prevIt);
        QMap<int, QVector<QWavVectorType>::iterator> peaks {{0, bIt}};

        for (int i = 1; i < 4; ++i) {
            //down-to-up part when i == 1, 3
            //up-to-down part when i == 2
            bool finished = true;
            for (; it != ch.end();) {
                if (haveSameSign(*prevIt, *it)) {
                    if ((i == 2 ? std::abs(*prevIt) >= std::abs(*it) : std::abs(*prevIt) <= std::abs(*it))) {
                        prevIt = it;
                        it = std::next(it);
                    }
                    else {
                        auto itNext = std::next(it);
                        if (itNext != ch.end() && ((i == 2 ? std::abs(*prevIt) >= std::abs(*itNext) : std::abs(*prevIt) <= std::abs(*itNext)))) {
                            prevIt = it;
                            it = itNext;
                        }
                        else {
                            peaks[i] = it;
                            break;
                        }
                    }
                }
                else {
                    bIt = it;
                    finished = false;
                    it = ch.end();
                }
            }

            //Signal crosses zero level - not ours case
            if (it == ch.end()) {
                if (finished) {
                    bIt = it;
                }
                break;
            }
        }

        //Looks like we've found a sine, normalizing it
        if (it != ch.end()) {
            bIt = it;
            double freq = getSampleRate() / std::distance(peaks[0], peaks[3]);
            if (freq <= ZERO_HALF_FREQ) {
                auto it = peaks[2];
                for (int i = 0; i < 2 && it != ch.end(); ++i, ++it) {
                    auto val = *it;
                    *it = val >= 0 ? -1000 : 1000;
                }
//                for (auto i = 0; i < 3; ++i) {
//                    auto middlePoint = std::distance(peaks[i], peaks[i + 1]) / 2;
//                    auto middleIt = std::next(peaks[i], middlePoint);
//                    auto middleVal = *middleIt;
//                    auto incVal = QWavVectorType(-1) * middleVal;
//                    std::for_each(i == 1 ? middleIt : peaks[i], i == 1 ? peaks[i + 1] : middleIt, [incVal](QWavVectorType& i) {
//                        //qDebug() << "Old: " << i << "; new: " << (i+incVal);
//                        i += incVal;
//                    });
//                }
            }
        }
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
