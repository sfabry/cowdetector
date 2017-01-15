#include "cowbox.h"

#include <QSqlDatabase>
#include <QTimer>
#include <QtDebug>
#include <QtSql>

#include "cowdetector.h"

#ifdef Q_OS_WIN
#include "debuggpio.h"
#include "debugdetector.h"
#else
#include "rpigpio.h"
#include "innovationreader.h"
#endif

CowBox::CowBox(QJsonObject config, QObject *parent) :
    QObject(parent)
  , m_config(config)
  , m_cowExitTimer(new QTimer(this))
{
#ifdef Q_OS_WIN
    m_foodRelayA = new DebugGpio("gpioFoodA", this);
    m_foodRelayB = new DebugGpio("gpioFoodB", this);
    m_foodRelayPhysA = new DebugGpio("gpioFoodPhysA", this);
    m_foodRelayPhysB = new DebugGpio("gpioFoodPhysB", this);
    m_calibrationButtonA = new DebugGpio("gpioCalibButtonA", this);
    m_calibrationButtonB = new DebugGpio("gpioCalibButtonB", this);
    m_reader = new DebugDetector(this);
#else
    m_foodRelayA = new RpiGpio(m_config.value("gpioFoodA").toInt(22), this);
    m_foodRelayB = new RpiGpio(m_config.value("gpioFoodB").toInt(23), this);
    m_foodRelayPhysA = new RpiGpio(m_config.value("gpioFoodPhysA").toInt(7), this);
    m_foodRelayPhysB = new RpiGpio(m_config.value("gpioFoodPhysB").toInt(8), this);
    m_calibrationButtonA = new RpiGpio(m_config.value("gpioCalibButtonA").toInt(25), GpioInterface::PullUp, 250, this);
    m_calibrationButtonB = new RpiGpio(m_config.value("gpioCalibButtonB").toInt(24), GpioInterface::PullUp, 250, this);
    m_reader = new InnovationReader(config.value("detector").toObject(), this);
#endif

    // Revert box signal
    m_foodRelayA->setOn(false);
    m_foodRelayB->setOn(false);
    m_foodRelayPhysA->setOn(false);
    m_foodRelayPhysB->setOn(false);

    // Load settings from DB box table
    readBoxParameters();
    m_readParameterTimer = new QTimer(this);
    m_readParameterTimer->setInterval(60 * 1000);
    m_readParameterTimer->setSingleShot(false);
    connect(m_readParameterTimer, &QTimer::timeout, this, &CowBox::readBoxParameters);
    m_readParameterTimer->start();
    qDebug() << "[box initialized] " << name() << m_newDayTime << m_idleStart1 << m_idleStop1 << m_idleStart2 << m_idleStop2 << m_foodSpeedA << m_foodSpeedB;

    // Cow detection delay
    m_cowExitTimer->setSingleShot(true);
    m_cowExitTimer->setInterval(m_detectionDelay * 1000);
    connect(m_cowExitTimer, &QTimer::timeout, this, &CowBox::cowExit);

    // Manual calibration
    connect(m_calibrationButtonA, &GpioInterface::onChanged, this, [this](bool on) {
        if (on && !m_foodRelayA->on() && !m_foodRelayB->on()) {
            qDebug() << "Box " << name() << "calibration for food A";
            m_foodRelayA->setOn(true);
            m_foodRelayPhysA->setOn(true);
            QTimer::singleShot(m_calibrationTime * 1000, this, SLOT(stopFoodA()));
        }
    });
    connect(m_calibrationButtonB, &GpioInterface::onChanged, this, [this](bool on) {
        if (on && !m_foodRelayA->on() && !m_foodRelayB->on()) {
            qDebug() << "Box " << name() << "calibration for food B";
            m_foodRelayB->setOn(true);
            m_foodRelayPhysB->setOn(true);
            QTimer::singleShot(m_calibrationTime * 1000, this, SLOT(stopFoodB()));
        }
    });

    connect(m_reader, &DetectorInterface::idChanged, this, &CowBox::detectedIdChanged);
}

CowBox::~CowBox()
{
    m_foodRelayA->setOn(false);
    m_foodRelayB->setOn(false);
    m_foodRelayPhysA->setOn(false);
    m_foodRelayPhysB->setOn(false);
}

QString CowBox::name() const
{
    return m_config.value("name").toString();
}

void CowBox::detectedIdChanged(const QString &id)
{
    if (!QSqlDatabase::database().isOpen()) {
        CowDetector::instance()->reconnectDatabase();
        return;
    }

    // The cow is going out of the box
    if (id.isEmpty()) {
        // Start timer, maybe the cow will reconnect soon.
        if (m_cow >= 0) m_cowExitTimer->start();
        return;
    }

    // Search identification table for corresponding cow
    int cow = -1;
    int card = -1;
    QSqlQuery query(QSqlDatabase::database());
    query.prepare("SELECT cownumber, cardnumber FROM identification WHERE rfid = :rfid");
    query.bindValue(":rfid", id);
    if (!query.exec()) qWarning() << "[CowBox] identification table query error : " << query.lastError().text();
    else if (query.next()) {
        cow = query.value(0).toInt();
        card = query.value(1).toInt();
    }
    else {
        // Create the row with the detected Id, so we could fill it with cow numbers
        QSqlQuery query(QSqlDatabase::database());
        query.prepare("INSERT INTO identification (rfid) VALUES (:rfid)");
        query.bindValue(":rfid",  id);
        if (!query.exec()) qWarning() << "[CowBox] identification insert query error : " << query.lastError().text();
    }

    // Check cow number
    if (cow <= 0) {
        qWarning() << name() << " No cow for RFID : " << id << " on card " << card;
        return;
    }
    m_cowExitTimer->stop();         // Cow is connected, stop the timer until we loose the connection.

    // Fetch food allocation of this cow
    if (m_cow != cow) {
        if (m_cow > 0) {
            // Another cow is entered before exit timer trigger (possible of course !)
            cowExit();
        }
        m_cow = cow;
        m_entryTime = QDateTime::currentDateTime();
        m_foodAllocation_A = 0;
        m_foodAllocation_B = 0;
        m_mealCount = -1;
        m_mealDelay = -1;
        m_eatSpeed = 6;
        m_currentMealId = -1;
        m_foodMealA = 0.0;
        m_foodMealB = 0.0;
        QSqlQuery query(QSqlDatabase::database());
        query.prepare("SELECT fooda, foodb, mealcount, mealdelay, eatspeed FROM foodallocation WHERE cow = :cow ORDER BY id DESC LIMIT 1");
        query.bindValue(":cow", m_cow);
        if (!query.exec()) qWarning() << "[CowBox] foodallocation table query error : " << query.lastError().text();
        else if (query.next()) {
            m_foodAllocation_A = query.value(0).toInt();
            m_foodAllocation_B = query.value(1).toInt();
            m_mealCount = query.value(2).toInt();
            m_mealDelay = query.value(3).toInt();
            m_eatSpeed = query.value(4).toInt();
            if (m_eatSpeed <= 0) m_eatSpeed = 6;
        }
    }
//    qDebug() << name() << ": Cow entry detected : " << cow << " - " << m_foodAllocation_A << ", " << m_foodAllocation_B;

    checkFoodDistribution();
}

void CowBox::cowExit()
{
    if (!QSqlDatabase::database().isOpen()) {
        CowDetector::instance()->reconnectDatabase();
        return;
    }
    if (m_cow <= 0) return;

    // Save the empty meal into database to keep track of cow entry/exit between meals
    if (m_currentMealId < 0) {
        QSqlQuery query(QSqlDatabase::database());
        query.prepare("INSERT INTO meals (cow, box, fooda, foodb, entry, exit) VALUES (:cow, :box, :fooda, :foodb, :entry, :exit)");
        query.bindValue(":cow"  ,  m_cow);
        query.bindValue(":box"  ,  m_config.value("id").toInt());
        query.bindValue(":fooda",  m_foodMealA);
        query.bindValue(":foodb",  m_foodMealB);
        query.bindValue(":entry",  m_entryTime);
        query.bindValue(":exit" ,  QDateTime::currentDateTime());
        if (!query.exec()) qWarning() << "[CowBox] meals insert query error : " << query.lastError().text();
    }

    m_currentMealId = -1;
    m_cow = -1;

}

void CowBox::checkFoodDistribution()
{
    if (!QSqlDatabase::database().isOpen()) {
        CowDetector::instance()->reconnectDatabase();
        return;
    }
    if (m_cow <= 0) return;
    if (!this->isActive()) return;

    // We don't resend food if we don't know if the cow is still there or not, we will give again at next connexion
    if (m_cowExitTimer->isActive()) return;

    // Don't send food if some is already given
    if (m_foodRelayA->on() || m_foodRelayB->on()) return;
    if ((m_foodAllocation_A == 0) && (m_foodAllocation_B == 0)) {
        qWarning() << name() <<" [checkFoodDistribution] Cow " << m_cow << " has zero allocation for food : " << m_foodAllocation_A << m_foodAllocation_B;
        return;
    }
    if (m_foodSpeedA == 0 || m_foodSpeedB == 0) {
        qWarning() << name() << " [checkFoodDistribution] Box has zero speed for food : " << m_foodSpeedA << m_foodSpeedB;
        return;
    }

    // Check the 6gr per second limit
    int mealDuration = m_entryTime.msecsTo(QDateTime::currentDateTime());
    if (mealDuration > 500 && mealDuration < (m_foodMealA + m_foodMealB) * 1000.0 / m_eatSpeed) {
        QTimer::singleShot(2000, this, SLOT(checkFoodDistribution()));
        return;
    }

    // Check already allocated meal for the day
    QSqlQuery query(QSqlDatabase::database());
    query.prepare("SELECT SUM(fooda), SUM(foodb) FROM meals WHERE cow = :cow AND entry > :starttime");
    query.bindValue(":cow", m_cow);
    QDate startDate = QDate::currentDate();
    if (QTime::currentTime() < m_newDayTime) startDate = startDate.addDays(-1);     // When the new day time is not yet passed, we have to check against previous day.
    query.bindValue(":starttime", QDateTime(startDate, m_newDayTime));
    if (!query.exec() || !query.next()) {
        qWarning() << "[CowBox] meals day query error : " << query.lastError().text();
        return;
    }
    qreal eatenTodayA = query.value(0).toReal();
    qreal eatenTodayB = query.value(1).toReal();

    // Check already allocated meal for the mealInterval
    if (m_mealCount <= 0) {
        m_mealCount = 2;
        if (m_foodAllocation_A + m_foodAllocation_B > 3000) m_mealCount = 4;
        if (m_foodAllocation_A + m_foodAllocation_B > 6000) m_mealCount = 6;
    }
    int mealInterval = m_mealDelay * 60;
    if (mealInterval <= 0) mealInterval = 18 * 3600 / m_mealCount;     // spread on 18 hours
    query.prepare("SELECT SUM(fooda), SUM(foodb) FROM meals WHERE cow = :cow AND entry > :starttime");
    query.bindValue(":cow", m_cow);
    query.bindValue(":starttime", QDateTime::currentDateTime().addSecs(-mealInterval));
    if (!query.exec() || !query.next()) {
        qWarning() << "[CowBox] meals day query error : " << query.lastError().text();
        return;
    }
    qreal eatenMealA = query.value(0).toReal();
    qreal eatenMealB = query.value(1).toReal();

    // Compute food to give
    qreal giveFoodA = qMin(m_foodAllocation_A - eatenTodayA, (qreal)m_foodAllocation_A / m_mealCount - eatenMealA);
    qreal giveFoodB = qMin(m_foodAllocation_B - eatenTodayB, (qreal)m_foodAllocation_B / m_mealCount - eatenMealB);
    giveFoodA = qMin(giveFoodA, (qreal)m_foodAllocation_A / (m_foodAllocation_A + m_foodAllocation_B) * m_mealMinimum);
    giveFoodB = qMin(giveFoodB, (qreal)m_foodAllocation_B / (m_foodAllocation_A + m_foodAllocation_B) * m_mealMinimum);
    if (giveFoodA + giveFoodB < 10) {
        // In the case the cow is still there without moving in 5 minutes
        QTimer::singleShot(5 * 60 * 1000, this, SLOT(checkFoodDistribution()));
        return;
    }

    // Give the food
    m_foodMealA += giveFoodA;
    m_foodMealB += giveFoodB;
    m_foodRelayA->setOn(true);
    m_foodRelayB->setOn(true);
    m_foodRelayPhysA->setOn(true);
    m_foodRelayPhysB->setOn(true);
    QTimer::singleShot(giveFoodA / m_foodSpeedA * 1000, this, SLOT(stopFoodA()));
    QTimer::singleShot(giveFoodB / m_foodSpeedB * 1000, this, SLOT(stopFoodB()));
    qDebug() << name() << " : Start food distribution to cow " << m_cow << " : today, meal, given: " << eatenTodayA << eatenTodayB << " -- " << eatenMealA << eatenMealB << " -- " << giveFoodA << giveFoodB;

    // Schedule next check for food
    QTimer::singleShot(1000 + qMax(giveFoodA / m_foodSpeedA * 1000, giveFoodB / m_foodSpeedB * 1000), this, SLOT(checkFoodDistribution()));

    // Save the meal into database
    if (m_currentMealId < 0) {
        QSqlQuery query(QSqlDatabase::database());
        query.prepare("INSERT INTO meals (cow, box, fooda, foodb, entry, exit) VALUES (:cow, :box, :fooda, :foodb, :entry, :exit) RETURNING id");
        query.bindValue(":cow"  ,  m_cow);
        query.bindValue(":box"  ,  m_config.value("id").toInt());
        query.bindValue(":fooda",  m_foodMealA);
        query.bindValue(":foodb",  m_foodMealB);
        query.bindValue(":entry",  m_entryTime);
        query.bindValue(":exit" ,  QDateTime::currentDateTime());
        if (!query.exec()) qWarning() << "[CowBox] meals insert query error : " << query.lastError().text();

        // Get id of the row just inserted
        if (query.next()) m_currentMealId = query.value(0).toInt();
    }
    else {
        // Update current meal
        QSqlQuery query(QSqlDatabase::database());
        query.prepare("UPDATE meals SET fooda = :fooda, foodb = :foodb, exit = :exit WHERE id = :id");
        query.bindValue(":fooda",  m_foodMealA);
        query.bindValue(":foodb",  m_foodMealB);
        query.bindValue(":exit" ,  QDateTime::currentDateTime());
        query.bindValue(":id",  m_currentMealId);
        if (!query.exec()) qWarning() << "[CowBox] meal update query error : " << query.lastError().text();
    }
}

void CowBox::stopFoodA()
{
    m_foodRelayA->setOn(false);
    m_foodRelayPhysA->setOn(false);
}

void CowBox::stopFoodB()
{
    m_foodRelayB->setOn(false);
    m_foodRelayPhysB->setOn(false);
}

bool CowBox::isActive() const
{
    if (!m_idleStart1.isNull() && !m_idleStop1.isNull()) {
        QTime current = QTime::currentTime();
        if (current > m_idleStart1 && current < m_idleStop1) return false;
    }

    if (!m_idleStart2.isNull() && !m_idleStop2.isNull()) {
        QTime current = QTime::currentTime();
        if (current > m_idleStart2 && current < m_idleStop2) return false;
    }

    return true;
}

void CowBox::readBoxParameters()
{
    if (!QSqlDatabase::database().isOpen()) {
        CowDetector::instance()->reconnectDatabase();
        return;
    }

    QSqlQuery query(QSqlDatabase::database());
    query.prepare("SELECT newdaytime, idlestart1, idlestop1, idlestart2, idlestop2, foodspeeda, foodspeedb, calibrationtime, mealminimum, detectiondelay FROM box WHERE boxnumber = :boxnumber");
    query.bindValue(":boxnumber", m_config.value("id").toInt());
    if (!query.exec()) qWarning() << "[CowBox] box table query error : " << query.lastError().text();
    else {
        if (query.next()) {
            m_newDayTime = query.value(0).toTime();
            m_idleStart1 = query.value(1).toTime();
            m_idleStop1  = query.value(2).toTime();
            m_idleStart2 = query.value(3).toTime();
            m_idleStop2  = query.value(4).toTime();
            m_foodSpeedA = query.value(5).toReal();
            m_foodSpeedB = query.value(6).toReal();
            m_calibrationTime = query.value(7).toInt();
            m_mealMinimum = query.value(8).toInt();
            m_detectionDelay = query.value(9).toInt();
        }
        else {
            // Create the row with default values
            m_newDayTime = QTime(5, 0);     // 5 AM
            m_foodSpeedA = 7;
            m_foodSpeedB = 7;
            QSqlQuery query(QSqlDatabase::database());
            query.prepare("INSERT INTO box (boxnumber, boxname, newdaytime, foodspeeda, foodspeedb) "
                          "VALUES (:boxNumber, :boxName, :newDayTime, :foodSpeedA, :foodSpeedB)");
            query.bindValue(":boxNumber",   m_config.value("id").toInt());
            query.bindValue(":boxName",     m_config.value("name").toString());
            query.bindValue(":newDayTime",  m_newDayTime);
            query.bindValue(":foodSpeedA",  m_foodSpeedA);
            query.bindValue(":foodSpeedB",  m_foodSpeedB);
            if (!query.exec()) qWarning() << "[CowBox] box insert query error : " << query.lastError().text();
        }
    }

    // Fill last connected column
    query.prepare("UPDATE box SET lastconnected = :lastconnected WHERE boxnumber = :boxnumber");
    query.bindValue(":boxnumber",   m_config.value("id").toInt());
    query.bindValue(":lastconnected", QDateTime::currentDateTime());
    if (!query.exec()) qWarning() << "[CowBox] box update last connected error : " << query.lastError().text();

}
