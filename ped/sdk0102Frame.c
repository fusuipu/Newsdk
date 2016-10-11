#include "sdkGlobal.h"

/********************PC LINT 过滤检测，请勿删除 start ******/
/*
lint -e415 越界(本文件中存在许多故意性越界情况，故先屏蔽掉)
lint -e416 越界(本文件中存在许多故意性越界情况，故先屏蔽掉)
lint -e420 越界(本文件中存在许多故意性越界情况，故先屏蔽掉)
*/
//lint -e751 定义的结构体类型没有被引用

//lint -e831 暂时屏蔽
//lint -e830 暂时屏蔽
/********************PC LINT 过滤检测，请勿删除 end*********/
static SDK_QUEUE_HAND g_framhand = NULL;

//由于要统一两个协议，只能按找01协议定框架
typedef struct
{
    u8 StartFlag; //起始字符0x02或0x01
    u8 Frame;     //01协议数据帧标识或02协议的序列号
#ifdef _lint //防止pclint检查越界(本文件中存在许多故意性越界情况，故先屏蔽掉)
    u8 Data[1000];
#else
    u8 Data[1];   //协议后续数据
#endif
} FRAME_HEAD, *P_FRAME_HEAD;
//协议返回数据

typedef struct
{
    s32 siBufferLen;
    u8 ucBuffer[1024];
} SDK_FRAME_DATA;



/*****************************************************************************
** Descriptions:            得到协议数据的长度
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameDataLen(const FRAME_HEAD * const pData)
{
    s32 len = 0;

    //01协议分扩展帧和一般帧，长度有区别
    if(pData->Frame == 0x15)     //扩展帧
    {
        len = ((u16)(pData->Data[1]) << 8) | (pData->Data[1 + 1]);
    }
    else if(pData->Frame == 0x13)     //命令帧格式  //zhouzhihua 2013.07.26 16:9
    {
        len = 0;     //命令帧没有数据
    }
    else
    {
        len = pData->Data[1];
    }
    return len;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameDataLen(const FRAME_HEAD * const pData)
{
    //s32 len = 0;

    s32 len = ((u16)(pData->Data[4 + 1]) << 8) | (pData->Data[4 + 1 + 1]);

    return len;
}

/*****************************************************************************
** Descriptions:            得到协议尾的位置
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameEndLocate(const FRAME_HEAD * const pData)
{
    s32 len = 0;

    if(pData->Frame == 0x15)
    {
        len = sdkGet01FrameDataLen(pData) + 2 + 1;     //加上两个长度
    }
    else if(pData->Frame == 0x13)     //没有结束符
    {
        len = 0;
    }
    else
    {
        len = sdkGet01FrameDataLen(pData) + 1 + 1;     //加上一个长度
    }
    return len;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameEndLocate(const FRAME_HEAD * const pData)
{
    s32 len = sdkGet02FrameDataLen(pData) + 4 + 1 + 2 + 2;

    return len;
}

/*****************************************************************************
** Descriptions:            得到02协议保留数据结束位置
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.17
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameRFUDataEndLocate(const FRAME_HEAD * const pData)
{
    //s32 len = 0;

    s32 len = 1 + 1 + 4;

    return len;
}

/*****************************************************************************
** Descriptions:            得到协议的长度数据的位置(在数据帧中的位置)
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameLenLocate(const FRAME_HEAD * const pData)
{
    s32 len = 0;

    if(pData->Frame == 0x13)     //命令帧没有长度
    {
        len = 0;
    }
    else if(pData->Frame == 0x15)
    {
        len = 1 + 1 + 1 + 2;
    }
    else
    {
        len = 1 + 1 + 1 + 1;
    }
    return len;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameLenLocate(const FRAME_HEAD * const pData)
{
    //s32 len = 0;

    s32 len = 1 + 1 + 4 + 1 + 2;

    return len;
}

/*****************************************************************************
** Descriptions:            得到整个协议的长度
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameLen(const FRAME_HEAD * const pData)
{
    s32 len  = 0;

    if(pData->Frame == 0x15)
    {
        len = sdkGet01FrameDataLen(pData) + 1 + 1 + 1 + 2 + 1 + 1;
    }
    else if(pData->Frame == 0x13)
    {
        len = sdkGet01FrameDataLen(pData) + 1 + 1 + 1 + 1;
    }
    else
    {
        len = sdkGet01FrameDataLen(pData) + 1 + 1 + 1 + 1 + 1 + 1;
    }
    return len;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameLen(const FRAME_HEAD * const pData)
{
    //s32 len  = 0;

    s32 len = sdkGet02FrameDataLen(pData) + 1 + 1 + 4 + 1 + 2 + 2 + 1;

    return len;
}

/*****************************************************************************
** Descriptions:            得到协议校验数据的位置
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameCheckLocate(const FRAME_HEAD * const pData)
{
    s32 len  = 0;

    if(pData->Frame == 0x13)
    {
        len = 1;
    }
    else
    {
        len = sdkGet01FrameEndLocate(pData) + 1;
    }
    return len;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameCheckLocate(const FRAME_HEAD * const pData)
{
    s32 len  = 0;

    if(sdkGet02FrameEndLocate(pData) > 2)
    {
        len = sdkGet02FrameEndLocate(pData) - 2;
    }
    return len;
}

/*****************************************************************************
** Descriptions:            得到协议参加计算校验数据的长度
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameCheckDataLen(const FRAME_HEAD * const pData)
{
    s32 len = 0;

    if(pData->Frame == 0x13)     //命令帧特殊
    {
        len = 3;
    }
    else if(pData->Frame == 0x15)
    {
        len = sdkGet01FrameDataLen(pData) + 1 + 1 + 1 + 2 + 1;
    }
    else
    {
        len = sdkGet01FrameDataLen(pData) + 1 + 1 + 1 + 1 + 1;
    }
    return len;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameCheckDataLen(const FRAME_HEAD * const pData)
{
    //s32 len = 0;

    s32 len = sdkGet02FrameDataLen(pData) + 1 + 4 + 1 + 2;

    return len;
}

/*****************************************************************************
** Descriptions:            计算校验和
** Parameters:          u8 const * const pata
                               u16 Len
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static u8    sdkGet01FrameCheckSum(u8 const * const pata, u16 Len)
{
    u8 RxBBC = 0;

    while (Len--)
    {
        RxBBC += pata[Len];
    }

    return RxBBC;
}

/*****************************************************************************
** Descriptions:            判断协议校验数据是否正确
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkIs01FrameCheckOK(const FRAME_HEAD * const pData)
{
    if(sdkGet01FrameCheckSum(&(pData->StartFlag), (u8)sdkGet01FrameCheckDataLen(pData)) == pData->Data[sdkGet01FrameCheckLocate(pData)])
    {
        return SDK_OK;
    }
    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkIs02FrameCheckOK(const FRAME_HEAD * const pData)
{
    //u16 temp = 0;

    u16 temp = (u16)sdkCalcCrc16(&(pData->Frame), sdkGet02FrameCheckDataLen(pData));

    if((((temp >> 8) & 0x00ff) == pData->Data[sdkGet02FrameCheckLocate(pData)]) && ((temp & 0xff) == pData->Data[sdkGet02FrameCheckLocate(pData) + 1]))
    {
        return SDK_OK;
    }
    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:            得到协议的最小长度
** Parameters:          void
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameMinSize(void)
{
    return 4 + 3; //最少命令帧长度
}

/*****************************************************************************
** Descriptions:
** Parameters:          void
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameMinSize(void)
{
    return 12; //最少命令帧长度
}

/*****************************************************************************
** Descriptions:
** Parameters:          Input: None
** Returned value:
** Created By:      黄少波  2013.04.14
** Remarks:
*****************************************************************************/
static s32 sdkRemoveTrashData(SDK_QUEUE_HAND hand)
{
    s32 locate;
    u8 temp[4] = {0};

    //1、求取队列长度
    s32 len = sdkQueueGetDataLen(hand);
    //Trace("zhouzhihua", "sdkRemoveTrashData len = %d \r\n", len);

    //2、定位数据头
    for(locate = 0; locate < len; locate++)
    {
        if(sdkQueueGetPosValue(hand, locate) == 0x02) //02协议
        {
            if(locate > 0)
            {
                sdkQueueRemoveRange(hand, 0, locate); //删除非协议数据
            }
            return sdkQueueGetDataLen(hand);
        }
        else if(sdkQueueGetPosValue(hand, locate) == 0x01) //01协议
        {
            if(locate > 0)
            {
                sdkQueueRemoveRange(hand, 0, locate); //删除非协议数据
            }

            if(sdkQueueGetData(hand, temp, 4) < 4) //说明数据还不够
            {
                TraceHex("zhouzhihua", "sdkRemoveTrashData ok \r\n", temp, 4);
                return SDK_ESC;
            }
            else
            {
                TraceHex("zhouzhihua", "sdkRemoveTrashData1 ok \r\n", temp, 4);

				//如果找到同步头，就返回
                if(memcmp("\x01\x01\x01", temp, 3) == 0)
                {
                    return sdkQueueGetDataLen(hand);
                }
                else
                {
                	//如果不是协议头就将此0x01要去掉 防止双循环
                    sdkQueueRemoveRange(hand, 0, 1); 
                    return SDK_ESC;
                }
            }
        }
    }
	
	//否则清空队列，没有满足的数据
    sdkQueueEmpey(hand);
    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:            协议数据解析
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkMis01FrameDataParse(const FRAME_HEAD * const pData)
{
    /*****1、判断数据块是不是符合协议框架*****/

    if(pData->Frame == 0x13)     //命令帧直接判读校验
    {
        if(sdkIs01FrameCheckOK(pData) == SDK_OK)     //解析正确
        {
            return SDK_OK;
        }
    }
    else
    {
        if(pData->Data[sdkGet01FrameEndLocate(pData)] == 0x03)
        {
            if(sdkIs01FrameCheckOK(pData) == SDK_OK)     //解析正确
            {
                Trace("zhouzhihua", "数据解析完成++++++++++++\r\n");
                return SDK_OK;
            }
        }
    }
    return SDK_ERR; //解析错误
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkMis02FrameDataParse(const FRAME_HEAD * const pData)
{
    /*****1、判断数据块是不是符合协议框架*****/
    if(pData->Data[sdkGet02FrameEndLocate(pData)] == 0x03)
    {
        if(sdkIs02FrameCheckOK(pData) == SDK_OK)        //解析正确
        {
            return SDK_OK;
        }
    }
    return SDK_ERR; //解析错误
}

/*****************************************************************************
** Descriptions:
** Parameters:          SDK_QUEUE_HAND hand
                               SDK_FRAME_DATA *pFrameData
** Returned value:
** Created By:      黄少波  2013.04.19
** Remarks:
*****************************************************************************/
static s32 Get02FramData(SDK_QUEUE_HAND hand, SDK_FRAME_DATA *pFrameData)
{
    u8 data1;
    s32 len = 0;

    if( sdkQueueGetDataLen(hand) < sdkGet02FrameMinSize())
    {
        return SDK_ESC;
    }
    data1 = sdkQueueGetPosValue(hand, 0);

    if(data1 != 0x02)
    {
        return SDK_ESC;
    }
    else
    {
        u8 buffer[1024];
        P_FRAME_HEAD pData = (P_FRAME_HEAD)&buffer[0];
		memset(buffer, 0x00, sizeof(buffer));
		
        len = sdkQueueGetData(hand, buffer, sizeof(buffer));
        Trace("ped", "sdkQueueGetData len = %d\r\n buffer[0] = %x\r\n", len, buffer[0]);

        if(len > sdkGet02FrameRFUDataEndLocate(pData)) //判断保留数据
        {
            if(memcmp("\x0E\x01\x0B\x01", &(pData->Data[0]), 4) != 0) //数据帧错误
            {
                sdkQueueRemoveRange(hand, 0, 1); //去掉数据头
            }
        }

        if(len < sdkGet02FrameLenLocate(pData) ) //是否到长度位置
        {
            return SDK_ESC;
        }

        if(len < sdkGet02FrameLen(pData)) //是否满足整个框架长度
        {
            return SDK_ESC;
        }

        if(sdkMis02FrameDataParse(pData) == SDK_OK) //解析数据
        {
            //取出数据帧，原样返回
            sdkQueueGetData(hand, pFrameData->ucBuffer, sdkGet02FrameLen(pData));

            //移除数据帧
            if(sdkGet02FrameLen(pData) > 0)
            {
                //len = sdkQueueRemoveRange(hand, 0, sdkGet02FrameLen(pData));
                Verify( sdkQueueRemoveRange(hand, 0, sdkGet02FrameLen(pData)) >= 0 );
            }
            //Assert(len >= 0);
            //Trace("ped", "final queue len = %d\r\n", len);
            Trace("ped", "FrameLen = %d\r\n", sdkGet02FrameLen(pData));
            pFrameData->siBufferLen = sdkGet02FrameLen(pData);
            Trace("ped", "final ret len = %d\r\n", pFrameData->siBufferLen);
            return SDK_OK;
        }
        else //解析错了，只去掉协议头，继续解析
        {
            sdkQueueRemoveRange(hand, 0, 1);
            return SDK_ERR;
        }
    }
}

/*****************************************************************************
** Descriptions:		获取01协议数据
** Parameters:          SDK_QUEUE_HAND hand
                        SDK_FRAME_DATA *pFrameData
** Returned value:
** Created By:      	黄少波  2013.04.19
** Remarks:         	lzl正确返回数据的格式:0x01+framId+命令+长度+有效数据+帧尾+累加和       
*****************************************************************************/
static s32 Get01FramData(SDK_QUEUE_HAND hand, SDK_FRAME_DATA *pFrameData)
{
    u8 data1;
    s32 len = 0;
    s32 ii = 0;

	//如果数据长度不够，就退出
    if( sdkQueueGetDataLen(hand)< sdkGet01FrameMinSize())
    {
        Trace("ped", "data 1 \r\n");
        return SDK_ESC;
    }

	//获取第一个数据的值
    data1 = sdkQueueGetPosValue(hand, 0);

	//如果第一个数据是0x01就继续解析，不是就退出
    if(data1 != 0x01)
    {
        Trace("ped", "data 2 \r\n");
        return SDK_ESC;
    }
    else
    {
        u8 buffer[1024];
        P_FRAME_HEAD pFHead = (P_FRAME_HEAD)&buffer[0];
		memset(buffer, 0x00, sizeof(buffer));
        
		//取出队列中的数据
        len = sdkQueueGetData(hand, buffer, sizeof(buffer));
        Trace("ped", "sdkQueueGetData len = %d\r\n buffer[0] = %02X\r\n", len, buffer[0]);

     	//如果第一个数据是0x01,就依次找出第一个不是0x01是数据位置,然后将从帧头的最后一个01开始拷贝
        if(buffer[0] == 0x01)
        {
            for(ii = 1; ii < len; ii++)
            {
                if(buffer[ii] != 0x01)
                {
                	//数据映射(帧头的最后一个01开始)
                    pFHead = (P_FRAME_HEAD)&buffer[ii - 1]; 
                    break;
                }
            }
        }

        if(pFHead->StartFlag == 0x01)
        {
            Assert(len >= ii - 1);
            len = len - (ii - 1);
        }

        if(pFHead->Frame == 0x13)
        {
        }
        else
        {
        	//根据数据头判断数据够不够
            if(len < sdkGet01FrameLenLocate(pFHead) ) 
            {
                Trace("ped", "data 3 \r\n");
                return SDK_ESC;
            }
        }
		
		//根据整个数据判断够不够
        if(len < sdkGet01FrameLen(pFHead)) 
        {
            Trace("ped", "data 4 \r\n");
            return SDK_ESC;
        }
		
        /*****3、开始解析数据*****/
        if(sdkMis01FrameDataParse(pFHead) == SDK_OK)
        {
            if(pFHead->StartFlag == 0x01)
            {
                if( ii > 1)
                {
                	//去掉多余的01
                    sdkQueueRemoveRange(hand, 0, ii - 1);
                }
            }
            //取出数据帧，原样返回
            sdkQueueGetData(hand, pFrameData->ucBuffer, sdkGet01FrameLen(pFHead));

            //移除数据帧
            if(sdkGet01FrameLen(pFHead) > 0)
            {
                Verify( sdkQueueRemoveRange(hand, 0, sdkGet01FrameLen(pFHead)) >= 0 );
            }
            Trace("ped", "FrameLen = %d\r\n", sdkGet01FrameLen(pFHead));

			//返回的数据长度
            pFrameData->siBufferLen = sdkGet01FrameLen(pFHead);
			
            Trace("ped", "final ret len = %d\r\n", pFrameData->siBufferLen);
            return SDK_OK;
        }
        else //解析错了，只去掉协议头，继续解析
        {
            if(ii > 0)
            {
                sdkQueueRemoveRange(hand, 0, ii);
            }
            Trace("ped", "data 6 \r\n");
            return SDK_ERR;
        }
    }
}

static s32 Get01And02Fram(SDK_QUEUE_HAND hand, SDK_FRAME_DATA *pFrameData)
{
    s32 rslt = SDK_ERR;
    s32 usleftLen;    /*****3、处理数据*****/

    if(hand == NULL || pFrameData == NULL)
    {
        return SDK_PARA_ERR;
    }
    usleftLen = sdkRemoveTrashData(hand);

    if((usleftLen + 1 >(sdkGet02FrameMinSize())) || (usleftLen + 1 > (sdkGet01FrameMinSize())))
    {
        Trace("ped", "usleftLen =%d \r\n",usleftLen );
        if(Get02FramData(hand, pFrameData) == SDK_OK || Get01FramData(hand, pFrameData) == SDK_OK)
        {
            TraceHex("ped", "resv ok \r\n", pFrameData->ucBuffer, pFrameData->siBufferLen);
            rslt = SDK_OK;
        }
    }
    return rslt;
}

/*****************************************************************************
** Descriptions: 接收串口协议数据
** Parameters:
** Returned value:  返回接收到的数据的长度
** Created By:      twz  2013.04.15
** Remarks:			主要是找到协议的头，并判断接收到数据的有效性
*****************************************************************************/
s32 sdkCommRev01And02Fram(enum SDK_SYS_COM_TYPE com, u8* pbuffer, s32 siMaxLen, s32 siTimerOut, bool IsCancel)
{
#define QueueSize()    1024

    SDK_FRAME_DATA FrameData;
    u8 ucRecvBuf[QueueSize()];
    s32 usRecvLen = 0;
    s32 rslt = 0;
    u32 timerid;
    u8 comid;

    if(pbuffer == NULL || siTimerOut < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(g_framhand == NULL)
    {
        //创建队列
        g_framhand =  sdkQueueCreate(QueueSize());

        if(g_framhand == NULL)
        {
            Assert(0);
            return SDK_ERR;
        }
    }

    //获取串口号
    comid = (u8)sdkSysGetComNo(com);

    //获取定时时间点
    timerid = sdkTimerGetId();
    while(1)
    {
        if(IsCancel)
        {
            if(sdkKbGetKey() == SDK_KEY_ESC)
            {
                rslt = SDK_ESC;
                break;
            }
        }
        /*****1、接收串口数据*****/
        memset(ucRecvBuf, 0, sizeof(ucRecvBuf));
        usRecvLen = sdkCommUartRecvData(comid, ucRecvBuf, sizeof(ucRecvBuf), 10);

		//如果接收到有数据
        if(usRecvLen > 0)
        {
            TraceHex("ped", "sdkCommUartRecvData ok \r\n", ucRecvBuf, usRecvLen);

			//判断队列是否已经满了
            if(usRecvLen + sdkQueueGetDataLen(g_framhand)> QueueSize())
            {
                Verify(sdkQueueEmpey(g_framhand) >= 0);
            }

			/*****2、压入队列数据*****/
            Verify(sdkQueueInsertData(g_framhand, ucRecvBuf, usRecvLen) > 0); 
        }

        /*=======BEGIN: 黄少波 2013.11.25  10:6 modify===========*/
        //获取有效数据
        memset(&FrameData,0x00,sizeof(SDK_FRAME_DATA));//lzl20150712
        if(Get01And02Fram(g_framhand, &FrameData) == SDK_OK)
        {
            TraceHex("zhouzhihua", "Get01And02Fram ok \r\n", FrameData.ucBuffer, FrameData.siBufferLen);

            if (FrameData.siBufferLen < siMaxLen)
            {
                memcpy(pbuffer, FrameData.ucBuffer, FrameData.siBufferLen);
                rslt = FrameData.siBufferLen;
                break;
            }
            else
            {
                Assert(0);
                /*=======BEGIN: 黄少波 2013.12.05  11:36 modify===========*/
                if(sdkTimerIsEnd(timerid, (u32)siTimerOut))
                {
                    rslt = SDK_TIME_OUT;
                    break;
                }
                /*====================== END======================== */
            }
        }
#if 0 /*Modify by 黄少波 at 2013.12.05  11:36 */
        else //解析失败。//黄少波 2013.08.19 11:23
        {
//          Assert(0);
            rslt = SDK_ERR;
            break;
        }
#endif /* if 0 */
        if(sdkTimerIsEnd(timerid, (u32)siTimerOut))
        {
            rslt = SDK_TIME_OUT;
            break;
        }
        /*====================== END======================== */
    }

    return rslt;
}

