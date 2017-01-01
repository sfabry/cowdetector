#include <QCoreApplication>

#include <iostream>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QJsonParseError>
#include <QJsonArray>
#include <QtSql>

#include "gpiointerface.h"
#include "cowbox.h"
#include "cowdetector.h"

#ifdef Q_OS_WIN
#include <QGuiApplication>
#include <QtQml>
#include "debuggpio.h"
#else
#include <signal.h>
#include "rpigpio.h"
#endif

void cowLogMsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

void interruptHandler(int sig)
{
    Q_UNUSED(sig);
    qDebug() << "[CowDetector] Exit application.";
    qApp->quit();
}

int main(int argc, char *argv[])
{
#ifndef Q_OS_WIN
    signal(SIGINT, interruptHandler);
#endif

#ifdef Q_OS_WIN
    QGuiApplication app(argc, argv);
#else
    QCoreApplication app(argc, argv);
#endif
    qDebug() << "[CowDetector] Application starting...";

    // Read on JSON file
    QList<QObject*> cowboxes;
    QFile tagsFile("cowdetector.json");
    if (!tagsFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Error opening cowdetector tags file : " << tagsFile.fileName();
        return -1;
    }
    QByteArray json = tagsFile.readAll();
    QJsonParseError error;
    QJsonDocument tags = QJsonDocument::fromJson(json, &error);
    if (tags.isNull() || !tags.isObject()) {
        qWarning() << "Error parsing Maestro tags file: " << tagsFile.fileName() << " - " << error.errorString();
        return -1;
    }
    QJsonObject jsonObject = tags.object();

    // Initialize GPIO
#ifdef Q_OS_WIN
#else
    RpiGpio::initialize();
#endif

    // Connect to database
    CowDetector::firstInstance(jsonObject);

    // Install debug handler to write logs to database
    qInstallMessageHandler(cowLogMsgHandler);

    // Create cow boxes on demand
    auto boxArray = jsonObject.value("boxes").toArray();
    for (QJsonArray::const_iterator i = boxArray.constBegin(); i != boxArray.constEnd(); i++) {
        CowBox* box = new CowBox((*i).toObject(), nullptr);
        cowboxes.append(box);
    }

#ifdef Q_OS_WIN
    QQmlApplicationEngine engine(QUrl("qrc:/qml/main.qml"));
    engine.rootContext()->setContextProperty("runningGpio", CowDetector::instance()->runningGpio());
    engine.rootContext()->setContextProperty("box", cowboxes.first());
#endif

    qDebug() << "[CowDetector] Started.";
    app.exec();
    qDebug() << "[CowDetector] Stopped.";

    // Close database
    QSqlDatabase::database().close();

    // Delete objects before end of program
    CowDetector::deleteCowDetector();
    for (auto box : cowboxes) delete box;

#ifdef Q_OS_WIN
#else
    RpiGpio::cleanUp();
#endif

    return 0;
}

void cowLogMsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        std::cout << msg.toLatin1().data();
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO logevents (level, message) VALUES (:level, :message)");
    if (type == QtDebugMsg) query.bindValue(":level", "DEBUG");
    else if (type == QtWarningMsg) query.bindValue(":level", "WARNING");
    else query.bindValue(":level", "OTHER");
    query.bindValue(":message", msg);
    if (!query.exec()) {
        std::cerr << "[cowLogMsgHandler] Log error : " << query.lastError().text().toLatin1().data();
        std::cerr << query.lastQuery().toLatin1().data();
    }
}

