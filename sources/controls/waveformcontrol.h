//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#ifndef WAVEFORMCONTROL_H
#define WAVEFORMCONTROL_H

#include <QQuickPaintedItem>
#include <QDateTime>
#include "sources/core/wavreader.h"
#include "sources/core/waveformparser.h"

class WaveformControl : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(uint channelNumber READ getChannelNumber WRITE setChannelNumber NOTIFY channelNumberChanged)
    Q_PROPERTY(int wavePos READ getWavePos WRITE setWavePos NOTIFY wavePosChanged)
    Q_PROPERTY(int waveLength READ getWaveLength NOTIFY waveLengthChanged)
    Q_PROPERTY(double xScaleFactor READ getXScaleFactor WRITE setXScaleFactor NOTIFY xScaleFactorChanged)
    Q_PROPERTY(double yScaleFactor READ getYScaleFactor WRITE setYScaleFactor NOTIFY yScaleFactorChanged)
    Q_PROPERTY(bool isWaveformRepaired READ getIsWaveformRepaired NOTIFY isWaveformRepairedChanged)
    Q_PROPERTY(bool selectionMode READ getSelectionMode WRITE setSelectionMode NOTIFY selectionModeChanged)

    WavReader& mWavReader;
    WaveformParser& mWavParser;

public:
    explicit WaveformControl(QQuickItem* parent = nullptr);

    uint getChannelNumber() const;
    int32_t getWavePos() const;
    int32_t getWaveLength() const;
    double getXScaleFactor() const;
    double getYScaleFactor() const;
    bool getIsWaveformRepaired() const;
    bool getSelectionMode() const;

    void setChannelNumber(uint chNum);
    void setWavePos(int wavPos);
    void setXScaleFactor(double xScaleFactor);
    void setYScaleFactor(double yScaleFactor);
    void setSelectionMode(bool mode);

    virtual void paint(QPainter* painter) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    Q_INVOKABLE void reparse();
    Q_INVOKABLE void saveTap(const QString& fileUrl = QString());
    Q_INVOKABLE void saveWaveform();
    Q_INVOKABLE void repairWaveform();
    Q_INVOKABLE void restoreWaveform();
    Q_INVOKABLE void copySelectedToAnotherChannel();

signals:
    void channelNumberChanged();
    void wavePosChanged();
    void waveLengthChanged();
    void xScaleFactorChanged();
    void yScaleFactorChanged();
    void isWaveformRepairedChanged();
    void selectionModeChanged();

    void doubleClick(int idx);

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
    bool m_selectionMode;
    bool m_rangeSelected;
    QPair<int, int> m_selectionRange;

    QWavVector* getChannel(uint* chNum = nullptr) const;
    int getWavPositionByMouseX(int x, int* point = nullptr, double* dx = nullptr) const;
};

#endif // WAVEFORMCONTROL_H
