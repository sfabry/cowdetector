#ifndef DEBUGDETECTOR_H
#define DEBUGDETECTOR_H

#include "detectorinterface.h"

class DebugDetector : public DetectorInterface
{
    Q_OBJECT

public:
    DebugDetector(QObject *parent = 0);

public slots:
    void setDetected(const QString &id);

};

#endif // DEBUGDETECTOR_H
