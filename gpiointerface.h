#ifndef GPIOINTERFACE_H
#define GPIOINTERFACE_H

#include <QObject>

class GpioInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool on READ on WRITE setOn NOTIFY onChanged)
    Q_PROPERTY(GpioType type READ type CONSTANT)
    Q_PROPERTY(PullType pullType READ pullType CONSTANT)

public:
    enum GpioType {
        In,
        Out
    };
    Q_ENUMS(GpioType)

    enum PullType {
        NoPull = 0,
        PullUp = 2,
        PullDown = 1
    };
    Q_ENUMS(PullType)

public:
    explicit GpioInterface(GpioType type, PullType pullType = NoPull, QObject *parent = 0);
    virtual ~GpioInterface();

    bool on() const { return m_on; }
    GpioType type() const { return m_type; }
    PullType pullType() const { return m_pullType; }

signals:
    void onChanged(bool on);

public slots:
    void setOn(bool on);
    void toggle();
    void pulse(int ms);
    void set() { setOn(true); }
    void reset() { setOn(false); }

protected:
    virtual void setGpioInternal(bool on) = 0;

private:
    bool m_on = false;
    GpioType m_type;
    PullType m_pullType;
};

#endif // GPIOINTERFACE_H
