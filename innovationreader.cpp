#include "innovationreader.h"

#include <QtSerialPort/QSerialPort>
#include <QtDebug>

#include "gpiointerface.h"
#ifdef Q_OS_WIN
#include "debuggpio.h"
#else
#include "rpigpio.h"
#endif

/*
 * Once a tag is identified, it will be set as the id detected,
 * It will be reset to empty string when the tag in range is falling back to zero.
 */

InnovationReader::InnovationReader(const QJsonObject &config, QObject *parent) :
    DetectorInterface(parent),
    m_config(config),
    m_serial(new QSerialPort(this))
{
#ifdef Q_OS_WIN
    m_tagInRange = new DebugGpio("CardPresent", this);
#else
    // TagInRange is pull down so there is no tag in range when not connected
    m_tagInRange = new RpiGpio(m_config.value("gpioTagInRange").toInt(17), GpioInterface::NoPull, 250, this);
#endif
    connect(m_tagInRange, &GpioInterface::onChanged, this, [this](bool on) {
        if (!on) setId(QString());
    });

    m_serial->setPortName(m_config.value("port").toString());
    m_serial->setBaudRate(QSerialPort::Baud9600);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    connect(m_serial, &QSerialPort::readyRead, this, &InnovationReader::readData);
    if (!m_serial->open(QIODevice::ReadOnly)) qWarning() << "Impossible to open serial port : " << m_serial->portName();
    qDebug() << "Innovation reader connected on port : " << m_serial->portName();
}

void InnovationReader::readData()
{
    m_buffer.append(m_serial->readAll());
//    qDebug() << "Innovation reader read data : " << m_buffer << m_buffer.toHex();

    int endText = m_buffer.indexOf(0x03);           // ETX
    if (endText == -1) return;
    QByteArray data = m_buffer.left(endText);
    m_buffer = m_buffer.mid(endText + 1);           // Remove message from buffer

    // Process data
    int startText = data.indexOf(0x02);         // STX
    if (startText == -1) {
        qWarning() << "Start TEXT not found;";
        return;
    }
    data = data.mid(startText + 1);
    if (data.count() < 12) {
        qWarning() << "No enough data read from the Innovation RFID reader : " << data.count() << data;
        return;
    }
    QByteArray id = data.left(10);
    setId(id);
}
