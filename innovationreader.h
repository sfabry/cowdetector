#ifndef INNOVATIONREADER_H
#define INNOVATIONREADER_H

#include <QJsonObject>
#include "detectorinterface.h"

class QSerialPort;
class GpioInterface;

class InnovationReader : public DetectorInterface
{
    Q_OBJECT

public:
    InnovationReader(const QJsonObject& config, QObject *parent = 0);


private slots:
    void readData();

private:
    QJsonObject m_config;
    QSerialPort* m_serial;
    GpioInterface* m_tagInRange;
    QByteArray m_buffer;
};

#endif // INNOVATIONREADER_H
