#ifndef KUTILITY_H
#define KUTILITY_H

#include <QString>
#include <map>
#include <cassert>
#include <iostream>

class KUtility
{
private:
    // no instance of this class
    KUtility();
    KUtility(const KUtility &);
    KUtility & operator=(const KUtility &);
    void *operator new(size_t);
    void *operator new[](size_t);
    void operator delete(void*);
    void operator delete[](void*);
public:
    static long getRegionIndex(long=-1);
    static QString getDirRoot(QString);
    static bool checkDirName(QString &);
    static void buildInputList(std::map<QString,int>&,QString);
    template<typename T> static bool reflectExtend(float * inBuff, T * outBuff, int iXSize, int iYSize, int iKernelSize)
    {
        assert(NULL != outBuff);
        assert(NULL != inBuff);

        long width = iXSize + 2*iKernelSize;
        long height = iYSize + 2*iKernelSize;

        if(width < 3 * iKernelSize || height < 3 * iKernelSize)
        {
            std::cout<<QString("KTamura:each line of the image cannot be shorter than %1!").arg(iKernelSize).toStdString()<<std::endl;
            exit( 1 );
        }
        // copy the source to the outbuff
        for(int iYDes = iKernelSize,iYSrc = 0;iYDes < height - iKernelSize;++iYDes,++iYSrc)
        {
            for(int iXDes = iKernelSize,iXSrc = 0;iXDes < width - iKernelSize;++iXDes,++iXSrc)
            {
                outBuff[iYDes*width+iXDes] = inBuff[iYSrc*(width-2*iKernelSize)+iXSrc];
            }
        }

        // fill horizontal
        for(int iYDes = iKernelSize;iYDes < height - iKernelSize;++iYDes)
        {
            // fill the head of each line
            int iXDes = iKernelSize-1,iXSrc = iKernelSize+1;
            for(;iXDes >= 0 && iXSrc < width - iKernelSize;--iXDes,++iXSrc)
            {
                outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc];
            }
            for(;iXDes >= 0;--iXDes)
            {
                outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc-1];
            }
            // fill the end of each line
            iXDes = width - iKernelSize,iXSrc = width - iKernelSize - 2;
            for(;iXDes < width && iXSrc >= iKernelSize;++iXDes,--iXSrc)
            {
                outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc];
            }
            for(;iXDes < width;++iXDes)
            {
                outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc+1];
            }
        }

        // fill vertical -- think rotate 90 degrees
        for(int iYDes = iKernelSize;iYDes < width - iKernelSize;++iYDes)
        {
            // fill the head of each line
            int iXDes = iKernelSize-1,iXSrc = iKernelSize+1;
            for(;iXDes >= 0 && iXSrc < height - iKernelSize;--iXDes,++iXSrc)
            {
                outBuff[iXDes*width+iYDes] = outBuff[iXSrc*width+iYDes];
            }
            for(;iXDes >= 0;--iXDes)
            {
                outBuff[iXDes*width+iYDes] = outBuff[(iXSrc-1)*width+iYDes];
            }
            // fill the end of each line
            iXDes = height - iKernelSize,iXSrc = height - iKernelSize - 2;
            for(;iXDes < height && iXSrc >= iKernelSize;++iXDes,--iXSrc)
            {
                outBuff[iXDes*width+iYDes] = outBuff[iXSrc*width+iYDes];
            }
            for(;iXDes < height;++iXDes)
            {
                outBuff[iXDes*width+iYDes] = outBuff[(iXSrc+1)*width+iYDes];
            }
        }

        // fill four corners, please find the axis of symmetry carefully
        for(int iYDes = 0;iYDes < iKernelSize;++iYDes)
        {
            for(int iXDes = 0;iXDes < iKernelSize;++iXDes)
            {
                outBuff[iYDes*width+iXDes] = (outBuff[(2*iKernelSize-iYDes)*width+iXDes]+outBuff[iYDes*width+2*iKernelSize-iXDes])/2;
            }
        }
        for(int iYDes = 0;iYDes < iKernelSize;++iYDes)
        {
            for(int iXDes = width-iKernelSize;iXDes < width;++iXDes)
            {
                outBuff[iYDes*width+iXDes] = (outBuff[(2*iKernelSize-iYDes)*width+iXDes]+outBuff[iYDes*width+2*(width-iKernelSize-1)-iXDes])/2;
            }
        }
        for(int iYDes = height - iKernelSize;iYDes < height;++iYDes)
        {
            for(int iXDes = 0;iXDes < iKernelSize;++iXDes)
            {
                outBuff[iYDes*width+iXDes] = (outBuff[(2*(height-iKernelSize-1)-iYDes)*width+iXDes]+outBuff[iYDes*width+2*iKernelSize-iXDes])/2;
            }
        }
        for(int iYDes = height - iKernelSize;iYDes < height;++iYDes)
        {
            for(int iXDes = width - iKernelSize;iXDes < width;++iXDes)
            {
                outBuff[iYDes*width+iXDes] = (outBuff[(2*(height-iKernelSize-1)-iYDes)*width+iXDes]+outBuff[iYDes*width+2*(width-iKernelSize-1)-iXDes])/2;
            }
        }
        return true;
    }
};

#endif // KUTILITY_H
