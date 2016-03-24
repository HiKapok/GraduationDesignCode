#ifndef KPYRIMADMATCH_H
#define KPYRIMADMATCH_H

#include "common.h"
#include <QString>
#include <QDebug>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>

using std::vector;
using std::map;
using std::make_pair;

class KPyrimadMatch
{
public:
    KPyrimadMatch(QString,QString,bool=true);
    double doMatch();
private:
    QString m_sInput;
    QString m_sInputAnother;
    bool m_beToNormarlized;
    int m_iTotalLevel;

    vector<map<long,long> > m_vecPyrimad;
    vector<map<long,long> > m_vecPyrimadAnother;
    void readPyrimad(QString &,vector<map<long,long> > &);

};

#endif // KPYRIMADMATCH_H
