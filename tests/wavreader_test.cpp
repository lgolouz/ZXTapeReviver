#include <QtTest>
#include <QDataStream>
#include <QTemporaryDir>
#include <bit>
#include <limits>
#include "sources/core/wavreader.h"
#include "sources/models/waveformmodel.h"

class WavReaderTest : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    static QByteArray wav(int bits, int code, int channels, const QByteArray& payload) {
        QByteArray result;
        QDataStream stream(&result, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF", 4);
        stream << quint32(36 + payload.size() + payload.size() % 2);
        stream.writeRawData("WAVEfmt ", 8);
        stream << quint32(16) << quint16(code) << quint16(channels) << quint32(48000)
               << quint32(48000 * channels * bits / 8) << quint16(channels * bits / 8) << quint16(bits);
        stream.writeRawData("data", 4);
        stream << quint32(payload.size());
        stream.writeRawData(payload.constData(), payload.size());
        if (payload.size() % 2) {
            stream << quint8(0);
        }
        return result;
    }

    WavReader::ErrorCodesEnum load(const QByteArray& bytes) {
        auto reader = WavReader::instance();
        reader->close();
        const auto name = m_dir.filePath("input.wav");
        QFile file(name);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) {
            return WavReader::CantOpen;
        }
        file.close();
        reader->setFileName(name);
        const auto result = reader->open();
        return result == WavReader::Ok ? reader->read() : result;
    }

    static void appendLittleEndian(QByteArray& bytes, quint32 value, int count) {
        for (int i = 0; i < count; ++i) {
            bytes.append(char((value >> (8 * i)) & 0xff));
        }
    }

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
    }

    void commonAmplitude_data() {
        QTest::addColumn<int>("bits");
        QTest::addColumn<int>("code");
        QTest::addColumn<int>("channels");
        for (int channels : { 1, 2 }) {
            for (int bits : { 8, 16, 24, 32 }) {
                QTest::newRow(qPrintable(QString("pcm%1-%2ch").arg(bits).arg(channels))) << bits << 1 << channels;
            }
            QTest::newRow(qPrintable(QString("float32-%1ch").arg(channels))) << 32 << 3 << channels;
        }
    }

    void commonAmplitude() {
        QFETCH(int, bits);
        QFETCH(int, code);
        QFETCH(int, channels);
        const QWavVector expected { -32768, -16384, -256, 0, 256, 16384, 32512 };
        QByteArray payload;
        for (qsizetype i = 0; i < expected.size(); ++i) {
            for (int ch = 0; ch < channels; ++ch) {
                const int sample = int(expected.at(ch == 0 ? i : expected.size() - 1 - i));
                quint32 encoded;
                if (code == 3) {
                    encoded = std::bit_cast<quint32>(sample / 32768.0f);
                } else if (bits == 8) {
                    encoded = sample / 256 + 128;
                } else {
                    encoded = quint32(qint64(sample) * (qint64(1) << (bits - 16)));
                }
                appendLittleEndian(payload, encoded, bits / 8);
            }
        }
        QCOMPARE(load(wav(bits, code, channels, payload)), WavReader::Ok);
        auto reader = WavReader::instance();
        QCOMPARE(*reader->getChannel0(), expected);
        QCOMPARE(reader->getSampleRate(), 48000u);
        QCOMPARE(reader->getNumberOfChannels(), uint(channels));
        if (channels == 2) {
            auto reverse = expected;
            std::reverse(reverse.begin(), reverse.end());
            QCOMPARE(*reader->getChannel1(), reverse);
        } else {
            QVERIFY(reader->getChannel1().isNull());
        }
        // WFM stores internal floats, not a second copy of source-format PCM.
        const auto name = m_dir.filePath("roundtrip.wfm");
        QVERIFY(reader->saveWaveform(name).succeeded());
        reader->close();
        reader->loadWaveform(name);
        QCOMPARE(*reader->getChannel0(), expected);
    }

    void signed24BitAndFractions() {
        const auto bytes = QByteArray::fromHex("000080 ffff7f ffffff 010000 000000 fffeff 010100");
        QCOMPARE(load(wav(24, 1, 1, bytes)), WavReader::Ok);
        const QWavVector expected { -32768, 32767.99609375f, -1.0f / 256, 1.0f / 256,
                                   0, -257.0f / 256, 257.0f / 256 };
        QCOMPARE(*WavReader::instance()->getChannel0(), expected);
    }

    void signed32BitAndFractions() {
        const auto bytes = QByteArray::fromHex("00000080 ffffff7f ffffffff 01000000 00000000");
        QCOMPARE(load(wav(32, 1, 1, bytes)), WavReader::Ok);
        // Float storage rounds INT32_MAX to 32768; output clipping belongs to playback.
        const QWavVector expected { -32768, 32768, -1.0f / 65536, 1.0f / 65536, 0 };
        QCOMPARE(*WavReader::instance()->getChannel0(), expected);
    }

    void floatHeadroomAndQuietSamples() {
        QByteArray bytes;
        const QWavVector source { -1.5f, -1.0f, -0.5f, -1.0f / 65536, 0, 1.0f / 65536, 0.5f, 1, 1.5f };
        for (float sample : source) {
            appendLittleEndian(bytes, std::bit_cast<quint32>(sample), 4);
        }
        QCOMPARE(load(wav(32, 3, 1, bytes)), WavReader::Ok);
        auto expected = source;
        for (auto& sample : expected) {
            sample *= 32768;
        }
        QCOMPARE(*WavReader::instance()->getChannel0(), expected);
    }

    void invalidFloatDepth_data() {
        QTest::addColumn<int>("bits");
        for (int bits : { 8, 16, 24, 64 }) {
            QTest::newRow(qPrintable(QString::number(bits))) << bits;
        }
    }

    void invalidFloatDepth() {
        QFETCH(int, bits);
        QCOMPARE(load(wav(bits, 3, 1, QByteArray(bits / 8, '\0'))), WavReader::UnsupportedWavFormat);
    }

    void nonFiniteFloat_data() {
        QTest::addColumn<quint32>("encoded");
        QTest::newRow("nan") << quint32(0x7fc00000);
        QTest::newRow("positive-inf") << quint32(0x7f800000);
        QTest::newRow("negative-inf") << quint32(0xff800000);
        QTest::newRow("scale-overflow") << quint32(0x7f7fffff);
    }

    void nonFiniteFloat() {
        QFETCH(quint32, encoded);
        QCOMPARE(load(wav(16, 1, 1, QByteArray::fromHex("0100"))), WavReader::Ok);
        const auto original = WavReader::instance()->getChannel0();
        const auto revision = WaveFormModel::instance()->documentRevision();
        QByteArray bytes;
        appendLittleEndian(bytes, 0, 4);
        appendLittleEndian(bytes, encoded, 4);
        QCOMPARE(load(wav(32, 3, 1, bytes)), WavReader::InvalidWavFormat);
        QCOMPARE(WavReader::instance()->getChannel0(), original);
        QCOMPARE(WaveFormModel::instance()->documentRevision(), revision);
    }

    void incompleteFrame() {
        QCOMPARE(load(wav(24, 1, 2, QByteArray::fromHex("000000 ffff"))), WavReader::InsufficientData);
    }

    void cleanup() {
        WavReader::instance()->close();
    }
};

QTEST_MAIN(WavReaderTest)
#include "wavreader_test.moc"
