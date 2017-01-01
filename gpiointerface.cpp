#include "gpiointerface.h"

#include <QTimer>

GpioInterface::GpioInterface(GpioType type, PullType pullType, QObject *parent) :
    QObject(parent)
  , m_type(type)
  , m_pullType(pullType)
{
}

GpioInterface::~GpioInterface()
{

}

void GpioInterface::setOn(bool on)
{
    if (m_on == on)
        return;

    m_on = on;
    setGpioInternal(m_on);
    emit onChanged(on);
}

void GpioInterface::toggle()
{
    setOn(!m_on);
}

void GpioInterface::pulse(int ms)
{
    setOn(true);
    QTimer::singleShot(ms, this, SLOT(reset()));
}

