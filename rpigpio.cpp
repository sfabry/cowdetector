#include "rpigpio.h"

#include <QtDebug>
#include <QTimer>

extern "C" {
#include "gertboard/gb_common.h"
}

// Output GPIO
RpiGpio::RpiGpio(int gpio, QObject *parent) :
    GpioInterface(GpioInterface::Out, GpioInterface::NoPull, parent)
  , m_gpio(gpio)
{
    inputGpioConfig(m_gpio);
    outputGpioConfig(m_gpio);

    setGpioInternal(on());
}

// INPUT GPIO
RpiGpio::RpiGpio(int gpio, GpioInterface::PullType pullType, int pollInterval, QObject* parent) :
    GpioInterface(GpioInterface::In, pullType, parent)
  , m_gpio(gpio)
  , m_pollTimer(new QTimer(this))
{
    inputGpioConfig(m_gpio);
    setPullType(m_gpio, pullType);

    m_pollTimer->setInterval(pollInterval);
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, &RpiGpio::checkInput);
    m_pollTimer->start();
}

RpiGpio::~RpiGpio()
{
    if (type() == GpioInterface::In) {
        setPullType(m_gpio, NoPull);
    }
}

void RpiGpio::initialize()
{
    qDebug() << "[RpiGpio] Initialize gpio.";
    setup_io();
}

void RpiGpio::cleanUp()
{
    qDebug() << "[RpiGpio] Restore gpio.";
    restore_io();
}


void RpiGpio::setGpioInternal(bool on)
{
    if (type() == GpioInterface::Out) {
        if (on) setGpio(m_gpio);
        else    clearGpio(m_gpio);
    }
//    else qDebug() << "Gpio " << m_gpio << " : " << on;
}

// Input low is considered activated (pull high case)
void RpiGpio::checkInput()
{
    if (pullType() == GpioInterface::PullUp) setOn(readGpio(m_gpio) == 0);
    else setOn(readGpio(m_gpio) != 0);
}
