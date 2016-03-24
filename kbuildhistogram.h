#ifndef KBUILDHISTOGRAM_H
#define KBUILDHISTOGRAM_H

#include <QDebug>
#include <QString>

#include <vector>
#include <map>
#include <utility>
#include <algorithm>

using std::vector;
using std::map;
using std::make_pair;

// just use for single band picture, be prepared for pymarid match
class KBuildHistogram
{
public:
    KBuildHistogram(QString);
    void addFile(QString file){ m_vFileInput.push_back(file); }
    void build();
    void save();
    void save_unittest();
private:
    vector<QString> m_vFileInput;
    QString m_sOutput;
    map<vector<int>,long> m_PointPair;

};

#endif // KBUILDHISTOGRAM_H
