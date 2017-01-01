#include "detectorinterface.h"

DetectorInterface::DetectorInterface(QObject *parent) : QObject(parent)
{

}

void DetectorInterface::setId(QString id)
{
    if (m_id == id)
        return;

    m_id = id;
    emit idChanged(id);
}
