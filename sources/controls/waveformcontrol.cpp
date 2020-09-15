//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

#include "waveformcontrol.h"
#include <cmath>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QPainter>
#include <QDebug>

WaveformControl::WaveformControl(QQuickItem* parent) :
    QQuickPaintedItem(parent),
    mWavReader(*WavReader::instance()),
    mWavParser(*WaveformParser::instance()),
    m_channelNumber(0),
    m_isWaveformRepaired(false),
    m_wavePos(0),
    m_xScaleFactor(1),
    m_yScaleFactor(80000)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setEnabled(true);
}

WavReader::QVectorBase* WaveformControl::getChannel() const
{
    return m_channelNumber == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1();
}

void WaveformControl::paint(QPainter* painter)
{
    painter->setBackground(QBrush (QColor(7, 37, 7)));
    painter->setBackgroundMode(Qt::OpaqueMode);
    painter->setPen(QColor(11, 60, 0));
    const auto& bRect = boundingRect();
    const double waveHeight = bRect.height() - 100;
    const double halfHeight = waveHeight / 2;
    painter->fillRect(bRect, painter->background());
    painter->drawLine(0, halfHeight, bRect.width(), halfHeight);
    uint32_t scale = bRect.width() * getXScaleFactor();
    uint32_t pos = getWavePos();
    const auto* channel = getChannel();
    if (channel == nullptr) {
        return;
    }

    const double maxy = getYScaleFactor();
    double px = 0;
    double py = bRect.height() / 2;
    double x = 0;
    double y = py;
    const int xinc = getXScaleFactor() > 16.0 ? getXScaleFactor() / 16 : 1;
    double dx = (bRect.width() / (double) scale) * xinc;
    const auto parsedWaveform = mWavParser.getParsedWaveform(m_channelNumber);
    bool printHint = false;
    m_allowToGrabPoint = dx > 2;
    const auto chsize = channel->size();
    for (int32_t t = pos; t < pos + scale; t += xinc) {
        if (t >= 0 && t < chsize) {
            const int val = channel->get(t);
            y = halfHeight - ((double) (val) / maxy) * waveHeight;
            painter->setPen(val >= 0 ? QColor(50, 150, 0) : QColor(200, 0 , 0));
            painter->drawLine(px, py, x, y);
            if (m_allowToGrabPoint) {
                painter->drawEllipse(QPoint(x, y), 2, 2);
            }

            const auto pwf = parsedWaveform[t];
            if (pwf & mWavParser.sequenceMiddle) {
                auto p = painter->pen();
                p.setWidth(3);
                painter->setPen(p);
                painter->drawLine(px, bRect.height() - 15, x, bRect.height() - 15);
                if (pwf & mWavParser.zeroBit || pwf & mWavParser.oneBit) {
                    p.setColor(QColor(0, 0, 250));
                    painter->setPen(p);
                    painter->drawLine(px, bRect.height() - 3, x, bRect.height() - 3);
                }

                if (printHint) {
                    QString text = pwf & mWavParser.pilotTone
                            ? "PILOT"
                            : pwf & mWavParser.synchroSignal
                              ? "SYNC"
                              : pwf & mWavParser.zeroBit
                                ? "\"0\""
                                : "\"1\"";
                    p.setWidth(1);
                    p.setColor(QColor(255, 255, 255));
                    painter->setPen(p);
                    painter->drawText(x + 3, bRect.height() - 15 - 10, text);
                    printHint = false;
                }
            }
            else if (pwf & mWavParser.sequenceBegin || pwf & mWavParser.sequenceEnd) {
                printHint = pwf & mWavParser.sequenceBegin;
                auto p = painter->pen();
                p.setWidth(3);
                painter->setPen(p);
                painter->drawLine(x, waveHeight + 2, x, bRect.height() - 15);

                if (pwf & mWavParser.byteBound) {
                    p.setColor(pwf & mWavParser.sequenceBegin ? QColor(0, 0, 250) : QColor(255, 242, 0));
                    painter->setPen(p);
                    painter->drawLine(x, bRect.height() - 10, x, bRect.height() - 3);
                }
            }
        }

        px = x;
        py = y;
        x += dx;
    }
}

int32_t WaveformControl::getWavePos() const
{
    return m_wavePos;
}

int32_t WaveformControl::getWaveLength() const
{
    return const_cast<WaveformControl*>(this)->getChannel()->size();
}

double WaveformControl::getXScaleFactor() const
{
    return m_xScaleFactor;
}

double WaveformControl::getYScaleFactor() const
{
    return m_yScaleFactor;
}

bool WaveformControl::getIsWaveformRepaired() const
{
    return m_isWaveformRepaired;
}

void WaveformControl::setWavePos(int32_t wavePos)
{
    if (m_wavePos != wavePos) {
        m_wavePos = wavePos;
        update();

        emit wavePosChanged();
    }
}

void WaveformControl::setXScaleFactor(double xScaleFactor)
{
    if (m_xScaleFactor != xScaleFactor) {
        m_xScaleFactor = xScaleFactor;
        update();

        emit xScaleFactorChanged();
    }
}

void WaveformControl::setYScaleFactor(double yScaleFactor)
{
    if (m_yScaleFactor != yScaleFactor) {
        m_yScaleFactor = yScaleFactor;
        update();

        emit yScaleFactorChanged();
    }
}

void WaveformControl::mousePressEvent(QMouseEvent* event)
{
    if (!event) {
        return;
    }

    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton || event->button() == Qt::MiddleButton) {
        event->accept();
        const int xinc = getXScaleFactor() > 16.0 ? getXScaleFactor() / 16 : 1;
        uint32_t scale = boundingRect().width() * getXScaleFactor();
        double dx = (boundingRect().width() / (double) scale) * xinc;
        if (dx <= 2.0) {
            return;
        }
        uint32_t point = std::round(event->x() / dx);
        double dpoint = point * dx;
        const double waveHeight = boundingRect().height() - 100;
        const double halfHeight = waveHeight / 2;

        if (event->button() == Qt::MiddleButton) {
            const auto p = point + getWavePos();
            const auto d = getChannel()->get(p);
            qDebug() << "Inserting point: " << p;
            getChannel()->insert(p + (dpoint > event->x() ? 1 : -1), d);
            update();
        }
        else {
            if (dpoint >= event->x() - 2.0 && dpoint <= event->x() + 2) {
                const double maxy = getYScaleFactor();
                double y = halfHeight - ((double) (getChannel()->get(point + getWavePos())) / maxy) * waveHeight;
                if (y >= event->y() - 2 && y <= event->y() + 2) {
                    if (event->button() == Qt::LeftButton) {
                        m_pointIndex = point;
                        m_pointGrabbed = true;
                        qDebug() << "Grabbed point: " << getChannel()->get(point + getWavePos());
                    }
                    else {
                        m_pointGrabbed = false;
                        getChannel()->remove(point + getWavePos());
                        update();
                    }
                }
            }
        }
    }
}

void WaveformControl::mouseReleaseEvent(QMouseEvent* event)
{
    if (!event) {
        return;
    }

    switch (event->button()) {
    case Qt::LeftButton:
        {
            m_pointGrabbed = false;
            event->accept();
        }
        break;

    default:
        event->ignore();
        break;
    }
}

void WaveformControl::mouseMoveEvent(QMouseEvent* event)
{
    if (!event) {
        return;
    }

    switch (event->buttons()) {
    case Qt::LeftButton:
        {
            const auto* ch = getChannel();
            if (!ch) {
                return;
            }

            const double waveHeight = boundingRect().height() - 100;
            const double halfHeight = waveHeight / 2;
            double val = halfHeight - (0b1111111111111111 / halfHeight * event->y());
            if (m_pointIndex + getWavePos() >= 0 && m_pointIndex + getWavePos() < ch->size()) {
                getChannel()->set(m_pointIndex + getWavePos(), val);
            }
            qDebug() << "Setting point: " << m_pointIndex + getWavePos();
            event->accept();
            update();
        }
        break;

    default:
        event->ignore();
        break;
    }
}

uint WaveformControl::getChannelNumber() const
{
    return m_channelNumber;
}

void WaveformControl::setChannelNumber(uint chNum)
{
    if (chNum != m_channelNumber) {
        m_channelNumber = chNum;
        emit channelNumberChanged();
    }
}

void WaveformControl::reparse()
{
    mWavParser.parse(m_channelNumber);
    update();
}

void WaveformControl::saveTap()
{
    mWavParser.saveTap(m_channelNumber);
}

void WaveformControl::saveWaveform()
{
    mWavReader.saveWaveform();
}

void WaveformControl::repairWaveform()
{
    if (!m_isWaveformRepaired) {
        mWavReader.repairWaveform();
        update();
        m_isWaveformRepaired = true;
        emit isWaveformRepairedChanged();
    }
}

void WaveformControl::restoreWaveform()
{
    if (m_isWaveformRepaired) {
        mWavReader.restoreWaveform();
        update();
        m_isWaveformRepaired = false;
        emit isWaveformRepairedChanged();
    }
}
