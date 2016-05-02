#include "kregion.h"

//#include <algorithm>
#include <limits>

KRegion &KRegion::operator+=(const KRegion &rhs)
{
    list<KSLE> tempList=rhs.getLists();
    for(list<KSLE>::iterator it = m_regLists.begin();it != m_regLists.end();++it){
        tempList.push_back(*it);
    }
    m_regLists.clear();
    tempList.sort();
    while(!tempList.empty()){
        KSLE temp = tempList.front();
        tempList.pop_front();
        KSLE tempTop = tempList.front();
        if(temp.getLine() == tempTop.getLine() && temp.getEndCol()+1 == tempTop.getStartCol()){//||(tempTop.getStartCol()>temp.getStartCol()+1 && temp.getEndCol()+1 > tempTop.getStartCol()))){
            temp+=tempTop;
            tempList.pop_front();
        }
        m_regLists.push_back(temp);
    }
    return *this;
}

int KRegion::getMinLine()
{
    int minValue = (std::numeric_limits<int>::max)();
    for(list<KSLE>::iterator it = m_regLists.begin();it != m_regLists.end();++it){
        int temp = it->getLine();
        if(temp<minValue){
            minValue = temp;
        }
    }
    return minValue;
}

int KRegion::getMaxLine()
{
    int maxValue = (std::numeric_limits<int>::min)();
    for(list<KSLE>::iterator it = m_regLists.begin();it != m_regLists.end();++it){
        int temp = it->getLine();
        if(temp>maxValue){
            maxValue = temp;
        }
    }
    return maxValue;
}

bool KRegion::isSinglePixel()
{
    if(m_regLists.size()==1){
        KSLE temp =  m_regLists.front();
        if(temp.getStartCol() == temp.getEndCol()) return true;
        else return false;
    }else return false;
}

int KRegion::totalPixels()
{
    int total=0;
    for(list<KSLE>::iterator it = m_regLists.begin();it != m_regLists.end();++it){
        total+=(it->getEndCol()-it->getStartCol()+1);
    }
    return total;
}

bool KRegion::hasPoint(int _x, int _y)
{
    for(list<KSLE>::iterator it = m_regLists.begin();it != m_regLists.end();++it){
        if(it->getLine() == _x && it->getStartCol()<=_y && it->getEndCol()>=_y){
            return true;
        }
    }
    return false;
}


