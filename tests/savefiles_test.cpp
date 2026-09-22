#include <QtTest>
#include <QDataStream>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopeGuard>
#include "sources/core/waveformparser.h"
#include "sources/models/fileworkermodel.h"
#include "sources/models/parsersettingsmodel.h"
#include "sources/models/suspiciouspointsmodel.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

class SaveFilesTest : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    QString path(const QString& name) const { return m_dir.filePath(name); }

    static QByteArray readFile(const QString& name) {
        QFile file(name);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }

    static bool writeFile(const QString& name, const QByteArray& data) {
        QFile file(name);
        return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
    }

    void setBlocks(std::initializer_list<qsizetype> sizes) {
        auto blocks = WaveformParser::instance()->getParsedDataSharedPtr(0);
        blocks->clear();
        for (const auto size : sizes) {
            auto block = QSharedPointer<ParsedData::DataBlock>::create();
            block->data.fill(0xA5, size);
            blocks->append(block);
        }
        WaveformParser::instance()->clearBlockSelection(0);
    }

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
        qmlRegisterUncreatableType<WavReader>("SaveTest", 1, 0, "WavReader", "Enums only");
        qmlRegisterUncreatableType<WaveformParser>("SaveTest", 1, 0, "WaveformParser", "Enums only");
    }

    void init() {
        // A small valid PCM16 WAV is enough to initialize the real models.
        QByteArray wav;
        QDataStream stream(&wav, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF", 4);
        stream << quint32(36 + 128);
        stream.writeRawData("WAVEfmt ", 8);
        stream << quint32(16) << quint16(1) << quint16(1) << quint32(48000)
               << quint32(96000) << quint16(2) << quint16(16);
        stream.writeRawData("data", 4);
        stream << quint32(128);
        for (int i = 0; i < 64; ++i) {
            stream << qint16(i % 2 ? 100 : -100);
        }
        QVERIFY(writeFile(path("source.wav"), wav));
        auto reader = WavReader::instance();
        reader->close();
        QCOMPARE(reader->setFileName(path("source.wav")), WavReader::Ok);
        QCOMPARE(reader->open(), WavReader::Ok);
        QCOMPARE(reader->read(), WavReader::Ok);
        SuspiciousPointsModel::instance()->clearSuspiciousPoints();
        ParserSettingsModel::instance()->restoreDefaultSettings();
        WaveformParser::instance()->clearParsingCancellation();
        WaveformParser::instance()->parse(0);
    }

    void tapRoundTripAndSelection() {
        setBlocks({ 2, 3 });
        const auto name = path("selected.tap");
        QVERIFY(writeFile(name, "old contents that must disappear"));
        QVERIFY(WaveformParser::instance()->saveTap(0, name).succeeded());
        QCOMPARE(readFile(name), QByteArray::fromHex("0200a5a50300a5a5a5"));
        WaveformParser::instance()->setBlockSelected(0, 1, true);
        QVERIFY(WaveformParser::instance()->saveTap(0, name).succeeded());
        QCOMPARE(readFile(name), QByteArray::fromHex("0300a5a5a5"));
    }

    void tapSizeLimits() {
        const auto name = path("limit.tap");
        setBlocks({ 65535 });
        QVERIFY(WaveformParser::instance()->saveTap(0, name).succeeded());
        const auto valid = readFile(name);
        QCOMPARE(valid.size(), 65537);
        QCOMPARE(valid.first(2), QByteArray::fromHex("ffff"));
        setBlocks({ 2, 65536 });
        const auto result = WaveformParser::instance()->saveTap(0, name);
        QCOMPARE(result.code, WaveformParser::SaveTapResultCode::BlockTooLarge);
        QCOMPARE(result.details, QString("2"));
        QCOMPARE(readFile(name), valid);
        // An unselected oversized block does not prevent a valid export.
        WaveformParser::instance()->setBlockSelected(0, 0, true);
        QVERIFY(WaveformParser::instance()->saveTap(0, name).succeeded());
        QCOMPARE(readFile(name), QByteArray::fromHex("0200a5a5"));
    }

    void invalidTapPreservesDestination() {
        const auto name = path("invalid.tap");
        QVERIFY(writeFile(name, "original"));
        setBlocks({});
        QCOMPARE(WaveformParser::instance()->saveTap(0, name).code,
                 WaveformParser::SaveTapResultCode::NoParsedData);
        QCOMPARE(readFile(name), QByteArray("original"));
        setBlocks({ 2, 0 });
        QCOMPARE(WaveformParser::instance()->saveTap(0, name).code,
                 WaveformParser::SaveTapResultCode::InvalidBlock);
        QCOMPARE(readFile(name), QByteArray("original"));
        WaveformParser::instance()->getParsedDataSharedPtr(0)->last().clear();
        QCOMPARE(WaveformParser::instance()->saveTap(0, name).code,
                 WaveformParser::SaveTapResultCode::InvalidBlock);
        QCOMPARE(readFile(name), QByteArray("original"));
    }

    void openFailures() {
        setBlocks({ 2 });
        const auto tap = WaveformParser::instance()->saveTap(0, path("missing/out.tap"));
        QCOMPARE(tap.code, WaveformParser::SaveTapResultCode::CannotOpenFile);
        QVERIFY(!tap.details.isEmpty());
        const auto wfm = WavReader::instance()->saveWaveform(path("missing/out.wfm"));
        QCOMPARE(wfm.code, WavReader::SaveWaveformResultCode::CannotOpenFile);
        QVERIFY(!wfm.details.isEmpty());
    }

    void waveformChunkedRoundTrip() {
        const auto name = path("roundtrip.wfm");
        auto reader = WavReader::instance();
        auto channel = reader->getChannel0();
        channel->resize(1024 * 1024 + 17);
        for (qsizetype i = 0; i < channel->size(); ++i) {
            (*channel)[i] = static_cast<float>(i % 65536) - 32768.25f;
        }
        const auto expected = *channel;
        SuspiciousPointsModel::instance()->addSuspiciousPoint(42);
        QCOMPARE(SuspiciousPointsModel::instance()->getSuspiciousPoint(0), 42u);
        const auto points = SuspiciousPointsModel::instance()->getSuspiciousPoints();
        QCOMPARE(points.first().toUInt(), 42u);
        QVERIFY(writeFile(name, "old waveform"));
        QVERIFY(reader->saveWaveform(name).succeeded());
        const auto saved = readFile(name);
        // Existing WFM layout: packed fmt (24), count (4), floats, point count and point.
        QCOMPARE(saved.size(), 24 + 4 + expected.size() * sizeof(float) + 4 + 4);
        QCOMPARE(saved.last(8), QByteArray::fromHex("010000002a000000"));
        QCOMPARE(saved.mid(28, expected.size() * sizeof(float)),
                 QByteArray(reinterpret_cast<const char*>(expected.constData()), expected.size() * sizeof(float)));
        reader->close();
        reader->loadWaveform(name);
        QCOMPARE(*reader->getChannel0(), expected);
        QCOMPARE(SuspiciousPointsModel::instance()->getSuspiciousPoint(0), 42u);
    }

    void invalidWaveformPreservesDestination() {
        const auto name = path("invalid.wfm");
        QVERIFY(writeFile(name, "original"));
        auto reader = WavReader::instance();
        reader->getChannel0()->clear();
        QCOMPARE(reader->saveWaveform(name).code, WavReader::SaveWaveformResultCode::InvalidChannelData);
        QCOMPARE(readFile(name), QByteArray("original"));
        reader->close();
        QCOMPARE(reader->saveWaveform(name).code, WavReader::SaveWaveformResultCode::NoWaveform);
        QCOMPARE(readFile(name), QByteArray("original"));
    }

    void fileWorkerReportsFailure() {
        FileWorkerModel model;
        QSignalSpy errors(&model, &FileWorkerModel::saveWaveformFailed);
        QCOMPARE(model.saveWaveformFileByUrl(QUrl::fromLocalFile(path("missing/out.wfm")).toString()),
                 int(FileWorkerModel::FW_ERR));
        QCOMPARE(errors.size(), 1);
        QCOMPARE(qvariant_cast<WavReader::SaveWaveformResultCode>(errors.first().at(1)),
                 WavReader::SaveWaveformResultCode::CannotOpenFile);
        QCOMPARE(model.saveWaveformFile(path("worker.wfm")), int(FileWorkerModel::FW_OK));
        QCOMPARE(errors.size(), 1);
    }

    void qmlErrorCodes() {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQml
            import SaveTest 1.0
            QtObject {
                property int wfmOpen: WavReader.CannotOpenFile
                property int wfmCommit: WavReader.CannotCommitFile
                property int tapSize: WaveformParser.BlockTooLarge
                property int tapWrite: WaveformParser.CannotWriteFile
            }
        )", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QCOMPARE(object->property("wfmOpen").toInt(), int(WavReader::SaveWaveformResultCode::CannotOpenFile));
        QCOMPARE(object->property("wfmCommit").toInt(), int(WavReader::SaveWaveformResultCode::CannotCommitFile));
        QCOMPARE(object->property("tapSize").toInt(), int(WaveformParser::SaveTapResultCode::BlockTooLarge));
        QCOMPARE(object->property("tapWrite").toInt(), int(WaveformParser::SaveTapResultCode::CannotWriteFile));
    }

    void lockedDestinationPreserved() {
#ifdef Q_OS_WIN
        const auto name = path("locked.file");
        QVERIFY(writeFile(name, "original"));
        // Deny rename/delete while permitting reads: commit must not destroy the old file.
        const auto handle = CreateFileW(reinterpret_cast<LPCWSTR>(name.utf16()), GENERIC_READ,
                                       FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        const auto guard = qScopeGuard([handle] { CloseHandle(handle); });
        setBlocks({ 2 });
        const auto tap = WaveformParser::instance()->saveTap(0, name);
        QCOMPARE(tap.code, WaveformParser::SaveTapResultCode::CannotCommitFile);
        QCOMPARE(readFile(name), QByteArray("original"));
        const auto wfm = WavReader::instance()->saveWaveform(name);
        QCOMPARE(wfm.code, WavReader::SaveWaveformResultCode::CannotCommitFile);
        QCOMPARE(readFile(name), QByteArray("original"));
#else
        QSKIP("Windows rename-sharing regression test");
#endif
    }

    void cleanup() {
        WavReader::instance()->close();
    }
};

QTEST_MAIN(SaveFilesTest)
#include "savefiles_test.moc"
