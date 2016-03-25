#include "kprogressbar.h"

#include <QDebug>
#include <cstdlib>
//#include <QCoreApplication>

int KProgressBar::m_siHisWidth=0;
bool KProgressBar::sBeRunning = false;

KProgressBar::KProgressBar(QString tip, unsigned long int totalSteps, int totalItems)
     :m_tips(tip),
      m_iTotalSteps(totalSteps),
      m_iTotalItems(totalItems>100?100:totalItems),
      m_iNowPos(0),
      m_iHisNowPos(-1),
      m_fNowPos(0.),
      m_fUpdateFreq(1.*totalItems/totalSteps),
      m_tRunning(Progress_Run)
{
//    redirect the stdout to the specified file

//    QString tempName=QCoreApplication::applicationDirPath()+"/tempConsole.txt";

//    std::streambuf *psbuf;
//    std::streambuf *pPrebuf;
//    std::ofstream filestr;
//    filestr.open (tempName.toUtf8().data(),std::ios_base::out | std::ios_base::app);

//    psbuf = filestr.rdbuf();
//    pPrebuf = std::cout.rdbuf(psbuf);

//    std::cout << "This is written to the file";

//    filestr.close();
//    std::cout.rdbuf(pPrebuf);
    int cols = tip.length() + 25 + m_iTotalItems;
    // ensure the progress bar can be showed fullly,but this will cause screen to be clear
    if(m_siHisWidth+5<cols){
        // uncoment this before use
        //system(QString("mode con cols=%1 lines=%2").arg(cols).arg(cols*5/16).toUtf8().data());
        m_siHisWidth=cols;
    }
}

void KProgressBar::autoRun()
{
    if(true != sBeRunning){
        sBeRunning = true;
        while(1)
        {
            if(m_iHisNowPos != m_iNowPos)
            {
                m_iHisNowPos = m_iNowPos;

                if(m_iNowPos == m_iTotalItems) { m_tRunning=Progress_Finish; }
                std::cout<<"\r"<<m_tips.toStdString()<<"("<<100*m_iNowPos/m_iTotalItems<<"%):[";
                int index = 0;
                for(;index < m_iNowPos;++index)
                {
                    std::cout<<"#";
                }
                for(;index < m_iTotalItems;++index){
                    std::cout<<" ";
                }
                std::cout<<"]";
                if(m_tRunning==Progress_Finish){ std::cout<<"-Finished"<<std::endl; break;}
                else if(m_tRunning==Progress_Cancel){ std::cout<<"-Canceled"<<std::endl; break;}
                else { std::cout<<"-Running"; }
                std::cout.flush();
            }
        }
        sBeRunning = false;
    }
}


