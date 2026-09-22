#include <QtTest>
#include <QAbstractItemModelTester>
#include <QDataStream>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "sources/actions/centerwaveformaction.h"
#include "sources/actions/editsampleaction.h"
#include "sources/actions/shiftwaveformaction.h"
#include "sources/core/wavreader.h"
#include "sources/models/actionsmodel.h"
#include "sources/models/fileworkermodel.h"
#include "sources/models/waveformmodel.h"

class ActionsTest : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    QString path(const QString& name) const { return m_dir.filePath(name); }

    static bool writeFile(const QString& name, const QByteArray& data) {
        QFile file(name);
        return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
    }

    static QSharedPointer<EditSampleAction> edit(int channel = 0, int sample = 0) {
        return QSharedPointer<EditSampleAction>::create(channel, EditSampleActionParams { 100.0f, 200.0f, sample });
    }

    static QSharedPointer<QWavVector> channel(int index = 0) {
        return WaveFormModel::instance()->getChannel(index);
    }

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
        new QAbstractItemModelTester(ActionsModel::instance(),
                                    QAbstractItemModelTester::FailureReportingMode::QtTest, this);
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
            stream << qint16(100);
        }
        QVERIFY(writeFile(path("source.wav"), wav));
        QVERIFY(writeFile(path("other.wav"), wav));
        QVERIFY(writeFile(path("other.tap"), QByteArray::fromHex("0200ffff")));
        FileWorkerModel worker;
        QCOMPARE(worker.openWavFile(path("source.wav")), int(FileWorkerModel::FW_OK));
        QVERIFY(WavReader::instance()->saveWaveform(path("other.wfm")).succeeded());
    }

    void init() {
        FileWorkerModel worker;
        QCOMPARE(worker.openWavFile(path("source.wav")), int(FileWorkerModel::FW_OK));
        QCOMPARE(ActionsModel::instance()->rowCount(), 0);
        QVERIFY(!ActionsModel::instance()->getCanRedo());
    }

    void undoRedoSameDocument() {
        auto model = ActionsModel::instance();
        model->addAction(edit());
        QCOMPARE(channel()->at(0), 200.0f);
        QCOMPARE(model->rowCount(), 1);
        QVERIFY(model->getCanUndo());
        model->removeAction();
        QCOMPARE(channel()->at(0), 100.0f);
        QVERIFY(!model->getCanUndo());
        QVERIFY(model->getCanRedo());
        model->redoAction();
        QCOMPARE(channel()->at(0), 200.0f);
        QVERIFY(model->getCanUndo());
        QVERIFY(!model->getCanRedo());
        model->removeAction();
        model->addAction(edit(0, 1));
        QVERIFY(!model->getCanRedo());
    }

    void newDocumentClearsHistory_data() {
        QTest::addColumn<QString>("file");
        QTest::addColumn<int>("undone");
        for (const auto& file : { "source.wav", "other.wav", "other.tap", "other.wfm" }) {
            for (int undone = 0; undone <= 2; ++undone) {
                QTest::newRow(qPrintable(QString("%1-undo-%2").arg(file).arg(undone))) << QString(file) << undone;
            }
        }
    }

    void newDocumentClearsHistory() {
        QFETCH(QString, file);
        QFETCH(int, undone);
        auto model = ActionsModel::instance();
        model->addAction(edit());
        model->addAction(edit(0, 1));
        for (int i = 0; i < undone; ++i) {
            model->removeAction();
        }
        const auto revision = WaveFormModel::instance()->documentRevision();
        QSignalSpy changed(model, &ActionsModel::actionsChanged);
        QSignalSpy reset(model, &QAbstractItemModel::modelReset);
        FileWorkerModel worker;
        if (file.endsWith(".wav")) {
            QCOMPARE(worker.openWavFile(path(file)), int(FileWorkerModel::FW_OK));
        } else if (file.endsWith(".tap")) {
            worker.openTapFile(path(file));
        } else {
            worker.openWaveformFile(path(file));
        }
        QCOMPARE(WaveFormModel::instance()->documentRevision(), revision + 1);
        QCOMPARE(changed.size(), 1);
        QCOMPARE(reset.size(), 1);
        QCOMPARE(model->rowCount(), 0);
        QVERIFY(model->getActions().isEmpty());
        QVERIFY(!model->getCanUndo());
        QVERIFY(!model->getCanRedo());
        const auto samples = *channel();
        model->removeAction();
        model->redoAction();
        QCOMPARE(*channel(), samples);
    }

    void failedOpenKeepsHistory_data() {
        QTest::addColumn<QString>("extension");
        QTest::newRow("wav") << QString("wav");
        QTest::newRow("tap") << QString("tap");
        QTest::newRow("wfm") << QString("wfm");
    }

    void failedOpenKeepsHistory() {
        QFETCH(QString, extension);
        auto model = ActionsModel::instance();
        model->addAction(edit());
        model->addAction(edit(0, 1));
        model->removeAction();
        const auto revision = WaveFormModel::instance()->documentRevision();
        const auto originalChannel = channel();
        QSignalSpy changed(model, &ActionsModel::actionsChanged);
        FileWorkerModel worker;
        const auto missing = path("missing." + extension);
        if (extension == "wav") {
            QCOMPARE(worker.openWavFile(missing), int(FileWorkerModel::FW_ERR));
        } else if (extension == "tap") {
            worker.openTapFile(missing);
        } else {
            worker.openWaveformFile(missing);
        }
        QCOMPARE(WaveFormModel::instance()->documentRevision(), revision);
        QCOMPARE(channel(), originalChannel);
        QCOMPARE(changed.size(), 0);
        QVERIFY(model->getCanUndo());
        QVERIFY(model->getCanRedo());
        model->removeAction();
        QCOMPARE(channel()->at(0), 100.0f);
        model->redoAction();
        QCOMPARE(channel()->at(0), 200.0f);
    }

    void retainedActionsRejectNewDocument() {
        const QList<QSharedPointer<ActionBase>> actions {
            edit(),
            QSharedPointer<ShiftWaveFormAction>::create(0, ShiftWaveFormActionParams { 10.0f }),
            QSharedPointer<CenterWaveformAction>::create(0, CenterWaveformActionParams {
                48000, ParserSettingsModel::AdaptiveVirtualAxisMedianWindow })
        };
        for (const auto& action : actions) {
            QVERIFY(action->apply());
        }
        FileWorkerModel worker;
        QCOMPARE(worker.openWavFile(path("other.wav")), int(FileWorkerModel::FW_OK));
        const auto samples = *channel();
        for (const auto& action : actions) {
            action->undo();
            QVERIFY(!action->apply());
            ActionsModel::instance()->addAction(action);
            QCOMPARE(*channel(), samples);
        }
        QCOMPARE(ActionsModel::instance()->rowCount(), 0);
    }

    void editBounds_data() {
        QTest::addColumn<int>("channelIndex");
        QTest::addColumn<int>("sample");
        QTest::newRow("negative-channel") << -1 << 0;
        QTest::newRow("missing-channel") << 1 << 0;
        QTest::newRow("large-channel") << 2 << 0;
        QTest::newRow("negative-sample") << 0 << -1;
        QTest::newRow("past-end") << 0 << 64;
    }

    void editBounds() {
        QFETCH(int, channelIndex);
        QFETCH(int, sample);
        const auto samples = *channel();
        const auto action = edit(channelIndex, sample);
        QVERIFY(!action->apply());
        action->undo();
        QCOMPARE(*channel(), samples);
    }

    void undoAfterChannelShrinks() {
        const auto action = edit(0, 63);
        QVERIFY(action->apply());
        channel()->resize(1);
        action->undo();
        QCOMPARE(channel()->size(), 1);
        QCOMPARE(channel()->at(0), 100.0f);
        QVERIFY(!action->apply());
    }

    void independentChannels() {
        auto left = QSharedPointer<QWavVector>::create(4, 100.0f);
        auto right = QSharedPointer<QWavVector>::create(4, 100.0f);
        WaveFormModel::instance()->initialize({ left, right });
        auto model = ActionsModel::instance();
        model->addAction(edit(0));
        model->addAction(edit(1));
        model->removeAction();
        QCOMPARE(left->at(0), 200.0f);
        QCOMPARE(right->at(0), 100.0f);
        model->removeAction();
        QCOMPARE(left->at(0), 100.0f);
        model->redoAction();
        model->redoAction();
        QCOMPARE(left->at(0), 200.0f);
        QCOMPARE(right->at(0), 200.0f);
    }

    void cleanup() {
        ActionsModel::instance()->clear();
        WavReader::instance()->close();
    }
};

QTEST_MAIN(ActionsTest)
#include "actions_test.moc"
