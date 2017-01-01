#include "debuggpio.h"

#include <QtDebug>

DebugGpio::DebugGpio(const QString &name, QObject *parent) :
    GpioInterface(GpioInterface::Out, GpioInterface::NoPull, parent)
  , m_name(name)
{
}


void DebugGpio::setGpioInternal(bool on)
{
}

void DebugGpio::setName(QString name)
{
    if (m_name == name)
        return;

    m_name = name;
    emit nameChanged(name);
}
