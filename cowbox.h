#ifndef COWBOX_H
#define COWBOX_H

#include <QJsonObject>
#include <QObject>
#include <QDateTime>
#include <QTimer>

#include "gpiointerface.h"
#include "detectorinterface.h"

class CowBox : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(GpioInterface* food_A READ food_A CONSTANT)
    Q_PROPERTY(GpioInterface* food_B READ food_B CONSTANT)
    Q_PROPERTY(GpioInterface* calibrationButtonA READ calibrationButtonA CONSTANT)
    Q_PROPERTY(GpioInterface* calibrationButtonB READ calibrationButtonB CONSTANT)
    Q_PROPERTY(DetectorInterface* rfid READ rfid CONSTANT)

public:
    explicit CowBox(QJsonObject config, QObject *parent = 0);
    ~CowBox();

    GpioInterface* food_A() const { return m_foodRelayA; }
    GpioInterface* food_B() const { return m_foodRelayB; }
    GpioInterface* calibrationButtonA() const { return m_calibrationButtonA; }
    GpioInterface* calibrationButtonB() const { return m_calibrationButtonB; }
    DetectorInterface* rfid() const { return m_reader; }

    QString name() const;

private slots:
    void detectedIdChanged(const QString &id);
    void cowExit();
    void checkFoodDistribution();
    void stopFoodA();
    void stopFoodB();

private:
    bool isActive() const;
    void readBoxParameters();

private:
    QJsonObject m_config;

    GpioInterface* m_foodRelayA;
    GpioInterface* m_foodRelayB;
    GpioInterface* m_foodRelayPhysA;
    GpioInterface* m_foodRelayPhysB;
    GpioInterface* m_calibrationButtonA;
    GpioInterface* m_calibrationButtonB;
    DetectorInterface* m_reader;
    QTimer* m_cowExitTimer;
    QTimer* m_readParameterTimer;

    // From box table, updated only at boot
    QTime m_newDayTime;
    QTime m_idleStart1;
    QTime m_idleStop1;
    QTime m_idleStart2;
    QTime m_idleStop2;
    qreal m_foodSpeedA = 0.0;
    qreal m_foodSpeedB = 0.0;
    int m_calibrationTime = 120;            // 120s for box food calibration
    int m_mealMinimum = 100;                // 100gr food distribution at each detection
    int m_detectionDelay = 30;              // 30s between connections of the cow

    // From foodallocation table once a cow is in the box
    int m_cow = -1;
    int m_foodAllocation_A = 0;
    int m_foodAllocation_B = 0;
    int m_mealCount = -1;
    int m_mealDelay = -1;
    int m_eatSpeed = 6;
    QDateTime m_entryTime;

    // Current meal distribution
    int m_currentMealId = -1;
    qreal m_foodMealA = 0.0;
    qreal m_foodMealB = 0.0;
};

#endif // COWBOX_H
