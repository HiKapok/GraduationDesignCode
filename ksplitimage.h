#ifndef KSPLITIMAGE_H
#define KSPLITIMAGE_H

#include "common.h"
#include <QString>
#include <QDebug>

class KSplitImage
{
public:
    KSplitImage(GDALDataset *,QString="");
    GUInt16 split(int,int);
private:
    GDALDataset *m_piDataSet;
    QString m_sPreFileName;
    QString m_sAftFileName;
    GDALDriver *m_pDriver;
};

#endif // KSPLITIMAGE_H
