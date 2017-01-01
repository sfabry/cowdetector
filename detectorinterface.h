#ifndef DETECTORINTERFACE_H
#define DETECTORINTERFACE_H

#include <QObject>

class DetectorInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id NOTIFY idChanged)

public:
    explicit DetectorInterface(QObject *parent = 0);
    virtual ~DetectorInterface() {}

    QString id() const { return m_id; }

signals:
    void idChanged(QString id);

protected slots:
    void setId(QString id);

private:
    QString m_id;

};

#endif // DETECTORINTERFACE_H
