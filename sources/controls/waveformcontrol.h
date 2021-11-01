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

#ifndef WAVEFORMCONTROL_H
#define WAVEFORMCONTROL_H

#include <QQuickPaintedItem>
#include <QDateTime>
#include "sources/core/wavreader.h"
#include "sources/core/waveformparser.h"
#include "sources/configuration/configurationmanager.h"
#include "sources/util/enummetainfo.h"

class WaveformControl : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(uint channelNumber READ getChannelNumber WRITE setChannelNumber NOTIFY channelNumberChanged)
    Q_PROPERTY(int wavePos READ getWavePos WRITE setWavePos NOTIFY wavePosChanged)
    Q_PROPERTY(int waveLength READ getWaveLength NOTIFY waveLengthChanged)
    Q_PROPERTY(double xScaleFactor READ getXScaleFactor WRITE setXScaleFactor NOTIFY xScaleFactorChanged)
    Q_PROPERTY(double yScaleFactor READ getYScaleFactor WRITE setYScaleFactor NOTIFY yScaleFactorChanged)
    Q_PROPERTY(bool isWaveformRepaired READ getIsWaveformRepaired NOTIFY isWaveformRepairedChanged)
    Q_PROPERTY(WaveformControlOperationModes operationMode READ getOperationMode WRITE setOperationMode NOTIFY operationModeChanged)

    WavReader& mWavReader;
    WaveformParser& mWavParser;
    ConfigurationManager::WaveformCustomization& m_customData;

    QColor getBackgroundColor() const;

public:
    enum WaveformControlOperationModes {
        WaveformRepairMode,
        WaveformSelectionMode,
        WaveformMeasurementMode
    };
    Q_ENUM(WaveformControlOperationModes)

    explicit WaveformControl(QQuickItem* parent = nullptr);

    uint getChannelNumber() const;
    int32_t getWavePos() const;
    int32_t getWaveLength() const;
    double getXScaleFactor() const;
    double getYScaleFactor() const;
    bool getIsWaveformRepaired() const;
    WaveformControlOperationModes getOperationMode() const;

    void setChannelNumber(uint chNum);
    void setWavePos(int wavPos);
    void setXScaleFactor(double xScaleFactor);
    void setYScaleFactor(double yScaleFactor);
    void setOperationMode(WaveformControlOperationModes mode);

    virtual void paint(QPainter* painter) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    Q_INVOKABLE void reparse();
    Q_INVOKABLE void saveTap(const QString& fileUrl = QString());
    Q_INVOKABLE void saveWaveform();
    Q_INVOKABLE void repairWaveform();
    Q_INVOKABLE void restoreWaveform();
    Q_INVOKABLE void shiftWaveform();
    Q_INVOKABLE void copySelectedToAnotherChannel();

signals:
    void channelNumberChanged();
    void wavePosChanged();
    void waveLengthChanged();
    void xScaleFactorChanged();
    void yScaleFactorChanged();
    void isWaveformRepairedChanged();
    void operationModeChanged();

    void doubleClick(int idx);
    void cannotSetMeasurementPoint();
    void frequency(int freq);

private:
    enum ClickStates {
        WaitForFirstPress,
        WaitForSecondPress,
        WaitForFirstRelease,
        WaitForSecondRelease
    };

    uint m_channelNumber;
    bool m_isWaveformRepaired;
    bool m_allowToGrabPoint;
    bool m_pointGrabbed;
    int m_pointIndex;
    int m_wavePos;
    double m_xScaleFactor;
    double m_yScaleFactor;
    ClickStates m_clickState;
    QDateTime m_clickTime;
    int m_clickPosition;
    WaveformControlOperationModes m_operationMode;
    bool m_rangeSelected;
    QPair<int, int> m_selectionRange;
    int m_clickCount;

    QWavVector* getChannel(uint* chNum = nullptr) const;
    int getWavPositionByMouseX(int x, int* point = nullptr, double* dx = nullptr) const;
};

#endif // WAVEFORMCONTROL_H
