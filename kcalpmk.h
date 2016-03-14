#ifndef KCALPMK_H
#define KCALPMK_H

#include "common.h"
#include <QString>
#include <QDebug>

/**
 * deploy The Pyramid Match Kernel
*/

class KCalPMK
{
public:
    KCalPMK();
    // get the specified level Histogram
 //   bool getHistogram(GByte *,GUInt32 *,int);
private:
    int m_iMaxLevel;


};

#endif // KCALPMK_H
