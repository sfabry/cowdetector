#ifndef DEBUGGPIO_H
#define DEBUGGPIO_H

#include "gpiointerface.h"

class DebugGpio : public GpioInterface
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    explicit DebugGpio(const QString &name, QObject* parent = nullptr);
    QString name() const { return m_name; }

public slots:
    void setName(QString name);

signals:
    void nameChanged(QString name);

    // GpioInterface interface
protected:
    void setGpioInternal(bool on) override;

private:
    QString m_name;

};

#endif // DEBUGGPIO_H
