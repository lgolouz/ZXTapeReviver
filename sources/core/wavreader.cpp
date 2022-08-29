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

#include "wavreader.h"
#include "sources/models/suspiciouspointsmodel.h"
#include "sources/models/parsersettingsmodel.h"
#include "sources/models/waveformmodel.h"
#include <QVariant>
#include <QVariantList>
#include <QDateTime>
#include <QDebug>
#include <QScopeGuard>

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

                //File pos aligning according to header data
                mWavFile.seek(mWavFile.pos() + (fmtHeader->chunk.chunkDataSize - (sizeof (WavFmt) - sizeof (WavChunk))));
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

QWavVectorType WavReader::getSample(QByteArray& buf, size_t& bufIndex, uint dataSize, uint compressionCode) const
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

    WaveFormModel::instance()->initialize({ getChannel0(), getChannel1() });
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

QSharedPointer<QWavVector> WavReader::getChannel0() const
{
    return mChannel0;
}

QSharedPointer<QWavVector> WavReader::getChannel1() const
{
    return mChannel1;
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

void WavReader::loadWaveform(const QString& fname)
{
    QFile f(fname);
    f.open(QIODevice::ReadOnly);
    QByteArray b(f.read(f.size()));
    size_t idx = 0;
    //Get header
    mWavFormatHeader = *getData<WavFmt>(b, idx);
    for (auto i = 0; i < mWavFormatHeader.numberOfChannels; ++i) {
        //Get channel length
        const int32_t l { *getData<int32_t>(b, idx) };
        auto& ch = i == 0 ? mChannel0 : mChannel1;
        ch.reset(new QWavVector(l));
        //Fill channel data
        for (auto& v: *ch.get()) {
            v = *getData<QWavVectorType>(b, idx);
        }
    }

    //Restore suspicious points
    const int32_t l { *getData<int32_t>(b, idx) };
    QVariantList sp;
    for (auto i = 0; i < l; ++i) {
        sp.append(*getData<uint32_t>(b, idx));
    }
    SuspiciousPointsModel::instance()->setSuspiciousPoints(sp);

    f.close();
    WaveFormModel::instance()->initialize({ getChannel0(), getChannel1() });
    mWavOpened = true;
}

unsigned WavReader::calculateOnesInByte (uint8_t n) {
  n = ((n>>1) & 0x55) + (n & 0x55);
  n = ((n>>2) & 0x33) + (n & 0x33);
  n = ((n>>4) & 0x0F) + (n & 0x0F);
  return n;
}

void WavReader::loadTap(const QString& fname) {
    QFile f(fname);
    f.open(QIODevice::ReadOnly);
    const auto guard = qScopeGuard([&f](){ f.close(); });

    const size_t fSize = f.size();
    QByteArray b(f.read(fSize));
    //Create header
    mWavFormatHeader = WavFmt {
            WavChunk { 0, 0 }, //Doesn't matter for TAP
            1, //Compression code
            2, //Number of channels
            48000, //Sample Rate
            192000, //Avg bytes per second
            4, //Block align
            16 //Significant bits per sample
    };

    const auto& parserSettings = ParserSettingsModel::instance()->getParserSettings();

    const auto oneFreq { parserSettings.oneHalfFreq / 2 };
    const auto oneHalfFreq { oneFreq * 2 };
    const auto zeroFreq { parserSettings.zeroHalfFreq / 2 };
    const auto zeroHalfFreq { zeroFreq * 2 };
    const auto synchroFirstHalf { parserSettings.synchroFirstHalfFreq };
    const auto synchroSecondHalf { parserSettings.synchroSecondHalfFreq };
    const auto pilotFreq { parserSettings.pilotHalfFreq / 2 };
    const auto pilotHalfFreq { pilotFreq * 2 };
    const auto silence { 0.5 };
    const auto pilotLen { 3 };

    //Check TAP file for correctness
    size_t pos { 0 };
    bool err { false };
    size_t wavlen { 0 };
    while (!err && pos < fSize) {
        err = fSize <= pos + sizeof(uint16_t);
        if (err) {
            continue;
        }

        const uint16_t blockSize { *getData<uint16_t>(b, pos) };
        err = fSize < pos + blockSize;
        if (err) {
            continue;
        }

        //Pilot
        wavlen += mWavFormatHeader.sampleRate * pilotLen;
        //Synchro
        wavlen += mWavFormatHeader.sampleRate / synchroFirstHalf;
        wavlen += mWavFormatHeader.sampleRate / synchroSecondHalf;

        for (size_t i { 0 }; i < blockSize; ++i) {
            const uint8_t byte { *getData<uint8_t>(b, pos) };
            const auto ones { calculateOnesInByte(byte) };
            const size_t byteLen { ones * (mWavFormatHeader.sampleRate / oneFreq) + (8 - ones) * (mWavFormatHeader.sampleRate / zeroFreq) };

            wavlen += byteLen;
        }

        //Silence
        wavlen += mWavFormatHeader.sampleRate * silence;
    }
    if (err) {
        return;
    }

    QWavVector v(wavlen, -1.);
    size_t wavpos { 0 };
    pos ^= pos;
    while (pos < fSize) {
        const uint16_t blockSize { *getData<uint16_t>(b, pos) };

        //Pilot
        wavlen = mWavFormatHeader.sampleRate / pilotHalfFreq;
        auto threshold { mWavFormatHeader.sampleRate * pilotLen / (wavlen * 2) };
        for (size_t i { 0 }; i < threshold; ++i) {
            for (auto p { 0 }; p <= 1; ++p) {
                const QWavVectorType val { QWavVectorType(32767 * (p ? 1 : -1)) };
                for (size_t c { 0 }; c < wavlen; ++c) {
                    v[wavpos++] = val;
                }
            }
        }
        //Synchro
        for (auto w { 0 }; w <= 1; ++w) {
            wavlen = mWavFormatHeader.sampleRate / (w ? synchroSecondHalf : synchroFirstHalf);
            const QWavVectorType val { QWavVectorType(32767 * (w ? 1 : -1)) };
            for (size_t c { 0 }; c < wavlen; ++c) {
                v[wavpos++] = val;
            }
        }

        for (size_t i { 0 }; i < blockSize; ++i) {
            const uint8_t byte { *getData<uint8_t>(b, pos) };
            //const auto ones { calculateOnesInByte(byte) };
            //const size_t byteLen { ones * (mWavFormatHeader.sampleRate / oneFreq) + (8 - ones) * (mWavFormatHeader.sampleRate / zeroFreq) };
            for (int i { 7 }; i >= 0; --i) {
                const uint8_t bit8 = 1 << i;
                const auto bit { byte & bit8 };
                wavlen = mWavFormatHeader.sampleRate / (bit == 0 ? zeroHalfFreq : oneHalfFreq);
                for (auto b { 0 }; b <= 1; ++b) {
                    const QWavVectorType val { QWavVectorType(32767 * (b ? 1 : -1)) };
                    for (size_t c { 0 }; c < wavlen; ++c) {
                        v[wavpos++] = val;
                    }
                }
            }
        }

        //Silence
        threshold = mWavFormatHeader.sampleRate * silence;
        for (size_t s { 0 }; s < threshold; ++s) {
            v[wavpos++] = s == 1 ? 0. : -1.;
        }
    }


    for (auto i = 0; i < mWavFormatHeader.numberOfChannels; ++i) {
        auto& ch = i == 0 ? mChannel0 : mChannel1;
        ch.reset(new QWavVector(v));
    }

    WaveFormModel::instance()->initialize({ getChannel0(), getChannel1() });
    mWavOpened = true;
}

void WavReader::saveWaveform(const QString& fname) const
{
    QFile f(fname.isEmpty() ? QString("waveform_%1.wfm").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh-mm-ss.zzz")) : fname);
    f.open(QIODevice::ReadWrite);

    //Store header
    QByteArray b;
    appendData(b, mWavFormatHeader);

    for (auto i = 0; i < mWavFormatHeader.numberOfChannels; ++i) {
        const auto& ch = i == 0 ? *getChannel0() : *getChannel1();
        //Store channel length
        const int32_t l = ch.length();
        appendData(b, l);
        //Store channel
        for (const auto& v: ch) {
            appendData(b, v);
        }
    }

    //Store suspicious points
    const auto s = SuspiciousPointsModel::instance()->getSuspiciousPoints();
    const int32_t l = s.length();
    appendData(b, l);
    for (const auto& p: s) {
        uint32_t sp = p.toUInt();
        appendData(b, sp);
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

void WavReader::repairWaveform(uint chNum) {
    if (chNum >= mWavFormatHeader.numberOfChannels) {
        qDebug() << "Channel number exceeds number of channels";
        return;
    }

    auto& ch = *(chNum == 0 ? mChannel0 : mChannel1).get();
    auto& ch2 =  *(chNum == 0 ? mChannel1 : mChannel0).get();
    const auto size { ch.size() };
    const auto size2 { ch2.size() };
    if (size == 0) {
        qDebug() << "Empty channel data";
        return;
    }
    for (auto i = 0; i < size; ++i) {
        const auto u = i < size2 ? (ch[i] + ch2[i]) / 2 : ch[i];
        ch[i] = u;
    }
    return;

    auto siz1 { ch.size() };

    int    du,siz0,_siz0;		// last amplitude value, size of last buff
    int a0,u0,du0;				// peak (index,amplitude,delta)
    int a1,u1,du1;				// peak (index,amplitude,delta)
    int a2,u2,du2;				// peak (index,amplitude,delta)

    du=0x7FFFFFFF; siz0=0; _siz0=siz1;
    a0=0; u0=0x7FFFFFFF; du0=0;
    a1=0; u1=0x7FFFFFFF; du1=0;
    a2=0; u2=0x7FFFFFFF; du2=0;

    int64_t adr,/*i,*/u,thr,A0,A1,U0,U1,uu;
    //                        noise      amplitude
    //                      threshold  min        max
    //if (wav.fmt.bits== 8){ thr=  32; U0=     0; U1=   255; }
    { thr=6000; U0=-30000; U1=+30000; }
    /*
    for (adr=0;adr<siz1;++adr)
        {
        // sum chanels into mono
//        for (u=0,i=0;i<wav.fmt.chanels;i++)
//            {
//            if (wav.fmt.bits== 8) u+=((unsigned __int8* )(dat1+adr))[i];
//            if (wav.fmt.bits==16) u+=((         __int16*)(dat1+adr))[i];
//            } u/=i;
//        // write back
//        for (i=0;i<wav.fmt.chanels;i++)
//            {
//            if (wav.fmt.bits== 8) ((unsigned __int8* )(dat1+adr))[i]=u;
//            if (wav.fmt.bits==16) ((         __int16*)(dat1+adr))[i]=u;
//            }
        u = adr < size2 ? (ch.at(adr) + ch2.at(adr)) / 2 : ch.at(adr);
        // detect peaks
        if (du==0x7FFFFFFF) du=u; 					// first value
        if (u2==0x7FFFFFFF){ a2=adr; u2=u; du2=0; }	// first value
        du=u-du;									// delta
        if (du*du2>=0) du2+=du;						// no peak
        else{
            if ((abs(du1)>thr)&&(abs(du2)>thr))		// 2 valid peaks
                {
                uu=u;
                // center and normalize amplitude
                if (u1<u2){ A0=u1; A1=u2; }
                 else     { A0=u2; A1=u1; }
                if (A1-A0>thr)
                 for (;a1<a2;++a1)
                    {
//                    for (u=0,i=0;i<wav.fmt.chanels;i++)
//                        {
//                        if (a1<0)
//                            {
//                            if (wav.fmt.bits== 8) u+=((unsigned __int8* )(dat0+a1+_siz0))[i];
//                            if (wav.fmt.bits==16) u+=((         __int16*)(dat0+a1+_siz0))[i];
//                            }
//                        else{
//                            if (wav.fmt.bits== 8) u+=((unsigned __int8* )(dat1+a1))[i];
//                            if (wav.fmt.bits==16) u+=((         __int16*)(dat1+a1))[i];
//                            }
//                        } u/=i;
                    u = ch.at(a1);
                    u=U0+(((U1-U0)*(u-A0))/(A1-A0));
                    if (u<U0) u=U0;
                    if (u>U1) u=U1;
                    ch[a1] = u;
//                    for (i=0;i<wav.fmt.chanels;i++)
//                        {
//                        if (a1<0)
//                            {
//                            if (wav.fmt.bits== 8) ((unsigned __int8* )(dat0+a1+_siz0))[i]=u;
//                            if (wav.fmt.bits==16) ((         __int16*)(dat0+a1+_siz0))[i]=u;
//                            }
//                        else{
//                            if (wav.fmt.bits== 8) ((unsigned __int8* )(dat1+a1))[i]=u;
//                            if (wav.fmt.bits==16) ((         __int16*)(dat1+a1))[i]=u;
//                            }
//                        }
                    }
                // shift peaks forward
                a0=a1;  u0=u1; du0=du1;
                a1=a2;  u1=u2; du1=du2;
                a2=adr; u2=uu; du2=du; u=uu;
                }
            else if (u1==0x7FFFFFFF)					// first peak
                {
                // shift peaks forward
                a0=a1;  u0=u1; du0=du1;
                a1=a2;  u1=u2; du1=du2;
                a2=adr; u2=u;  du2=du;
                }
            else{										// noise
                // shift peaks backward
                a2=a1; u2=u1; du2=u-u2;
                a1=a0; u1=u0; du1=du0;
                a0= 0; u0=0x7FFFFFFF; du0=0;
                }
            }
        du=u;
        }
//    if ((a0>=0)&&(u0!=0x7FFFFFFF)) a0-=siz1;
//    if ((a1>=0)&&(u1!=0x7FFFFFFF)) a1-=siz1;
//    if ((a2>=0)&&(u2!=0x7FFFFFFF)) a2-=siz1;
//    siz0=_siz0;	_siz0=siz1;
*/
    const auto thrhold = thr;
    auto v { ch.first() };
    auto max { v };
    //auto it { ch.begin() };
    auto i { size - size };
    while (i < size) {
        v = ch[i];
        while (i < size) {
        //auto tit = std::find_if(it, ch.end(), [&max, v, threshold](auto& val) {
            auto& val = ch[i];
                bool signEqual { lessThanZero(v) == lessThanZero(val) };
                if (signEqual) {
                    if (abs(val) > abs(max)) {
                        max = val;
                    }
                } else if (abs(val) <= abs(thrhold)) {
                    //if (i >= 553180) {
                        val = (max + val) / 2;
                        signEqual = lessThanZero(v) == lessThanZero(val);
                    //}
                }
                if (!signEqual) {
                    break;
                }
                ++i;
            //});
        }
        if (i < size) {
            max = ch.at(i);
            ++i;
        }
//        uint64_t d = std::distance(it, ch.end());
//        qDebug() << "Distance: " << d;
    }

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

    const auto& parserSettings = ParserSettingsModel::instance()->getParserSettings();

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
            if (freq <= parserSettings.zeroHalfFreq) {
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
    static QScopedPointer<WavReader> w { new WavReader() };
    return w.get();
}
