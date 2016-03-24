#ifndef KCALPMK_H
#define KCALPMK_H

#include <QString>
#include <QDebug>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>

using std::vector;
using std::map;
using std::make_pair;

/**
 * deploy The Pyramid Match Kernel
 */

class KCalPMK
{
public:
    KCalPMK(QString,QString,bool=false,double=1.);
    void savePyramid_unittest();
    void savePtramid();
private:
    int m_iMaxLevel;
    long m_dDiameter;

    int m_dMax;
    int m_dMin;
    QString m_sInput;
    QString m_sOutput;
    double m_dScale;
    long m_lPointPair;
    int m_iDims;
    vector<vector<int> > m_transTable;
    map<vector<int>,long> m_PointPair;
    vector<map<long,long> > m_pyramid;
    void calMinMax();
    void prepare();
    void readPrimaryHist();
    void buildPyramid();
private:
    static vector<map<vector<int>,long> > sIndex_map;
    static vector<long> sNowIndex;
};

#endif // KCALPMK_H
