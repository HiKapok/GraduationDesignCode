#ifndef KMAKESVMTABLE_H
#define KMAKESVMTABLE_H

#include <QString>
#include <QDebug>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>

using std::vector;
using std::map;
using std::pair;

class KMakeSVMTable
{
public:
    KMakeSVMTable(map<QString,int>,QString,bool=true);
    KMakeSVMTable(QString,QString,bool=true);
    void makeTable();
private:
    map<QString,int> m_vecInput;
    map<QString,int> m_vecPyramid;
    QString m_sOutput;
    bool m_beTraining;
    QString m_sOutRoot;
    QString getDirRoot(QString);
    bool checkDirName(QString &);
    void buildInputList(map<QString,int>&,QString);
    void buildAllPyramid();
};

#endif // KMAKESVMTABLE_H
