#ifndef COWDETECTOR_H
#define COWDETECTOR_H

#include <QObject>
#include <QJsonObject>
#include <QElapsedTimer>

class QTimer;
class GpioInterface;

class CowDetector : public QObject
{
    Q_OBJECT
public:
    explicit CowDetector(const QJsonObject &config, QObject *parent = 0);
    ~CowDetector();

    static CowDetector* firstInstance(const QJsonObject &config);
    static CowDetector* instance();
    static void deleteCowDetector();

    GpioInterface* runningGpio() { return m_runningGpio; }

signals:

public slots:
    void reconnectDatabase();

private:
    static CowDetector* m_instance;
    QJsonObject m_config;
    GpioInterface* m_runningGpio = nullptr;
    QTimer *m_runningTimer = nullptr;
    QElapsedTimer m_timer;
};

#endif // COWDETECTOR_H
