#include "debugdetector.h"

DebugDetector::DebugDetector(QObject *parent) :
    DetectorInterface(parent)
{

}

void DebugDetector::setDetected(const QString &id)
{
    setId(id);
}
