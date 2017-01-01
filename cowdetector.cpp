#include "cowdetector.h"

#include <QtSql>
#include <QtDebug>
#include <QTimer>

#include "gpiointerface.h"

#ifdef Q_OS_WIN
#include "debuggpio.h"
#else
#include "rpigpio.h"
#endif

CowDetector* CowDetector::m_instance = nullptr;

CowDetector::CowDetector(const QJsonObject &config, QObject *parent) :
    QObject(parent)
  , m_config(config)
  , m_runningTimer(new QTimer(this))
{
    // Launch a timer to blink a led showing application is running
#ifdef Q_OS_WIN
    m_runningGpio = new DebugGpio("Running", this);
#else
    m_runningGpio = new RpiGpio(m_config.value("runningGpio").toInt(27), nullptr);
#endif
    m_runningTimer->setSingleShot(false);
    m_runningTimer->setInterval(3000);
    QObject::connect(m_runningTimer, &QTimer::timeout, m_runningGpio, [this]() { m_runningGpio->pulse(1500); });

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName      (m_config.value("databaseHost").toString());
    db.setPort          (m_config.value("databasePort").toInt());
    db.setDatabaseName  (m_config.value("databaseName").toString());
    db.setUserName      (m_config.value("databaseUser").toString());
    db.setPassword      (m_config.value("databasePwd").toString());
    reconnectDatabase();
}

CowDetector::~CowDetector()
{
    delete m_runningGpio;
    m_runningGpio = nullptr;
}

CowDetector *CowDetector::firstInstance(const QJsonObject &config)
{
    if (m_instance == nullptr) m_instance = new CowDetector(config);
    return m_instance;
}

CowDetector *CowDetector::instance()
{
    if (m_instance == nullptr) return nullptr;
    return m_instance;
}

void CowDetector::deleteCowDetector()
{
    delete m_instance;
    m_instance = nullptr;
}

void CowDetector::reconnectDatabase()
{
    if (m_timer.isValid() && m_timer.elapsed() < 20000) return;
    m_timer.start();

    if (!QSqlDatabase::database().open()) {
        qWarning() << "Database connection error : " << QSqlDatabase::database().lastError().text();
        QTimer::singleShot(30 * 1000, this, SLOT(reconnectDatabase()));
        m_runningTimer->stop();
    }
    else m_runningTimer->start();
}
