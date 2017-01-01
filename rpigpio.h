#ifndef RPIGPIO_H
#define RPIGPIO_H

#include "gpiointerface.h"

class QTimer;

class RpiGpio : public GpioInterface
{
    Q_OBJECT

public:
    explicit RpiGpio(int gpio, QObject* parent = nullptr);                                      // Input
    explicit RpiGpio(int gpio, PullType pullType, int pollInterval, QObject* parent = nullptr);    // Output
    ~RpiGpio();

    static void initialize();
    static void cleanUp();

    // GpioInterface interface
protected:
    void setGpioInternal(bool on) override;

private slots:
    void checkInput();

private:
    int m_gpio;
    QTimer* m_pollTimer = nullptr;
};

#endif // RPIGPIO_H
