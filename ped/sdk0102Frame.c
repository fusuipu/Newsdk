#include "sdkGlobal.h"

/********************PC LINT ���˼�⣬����ɾ�� start ******/
/*
lint -e415 Խ��(���ļ��д�����������Խ��������������ε�)
lint -e416 Խ��(���ļ��д�����������Խ��������������ε�)
lint -e420 Խ��(���ļ��д�����������Խ��������������ε�)
*/
//lint -e751 ����Ľṹ������û�б�����

//lint -e831 ��ʱ����
//lint -e830 ��ʱ����
/********************PC LINT ���˼�⣬����ɾ�� end*********/
static SDK_QUEUE_HAND g_framhand = NULL;

//����Ҫͳһ����Э�飬ֻ�ܰ���01Э�鶨���
typedef struct
{
    u8 StartFlag; //��ʼ�ַ�0x02��0x01
    u8 Frame;     //01Э������֡��ʶ��02Э������к�
#ifdef _lint //��ֹpclint���Խ��(���ļ��д�����������Խ��������������ε�)
    u8 Data[1000];
#else
    u8 Data[1];   //Э���������
#endif
} FRAME_HEAD, *P_FRAME_HEAD;
//Э�鷵������

typedef struct
{
    s32 siBufferLen;
    u8 ucBuffer[1024];
} SDK_FRAME_DATA;



/*****************************************************************************
** Descriptions:            �õ�Э�����ݵĳ���
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameDataLen(const FRAME_HEAD * const pData)
{
    s32 len = 0;

    //01Э�����չ֡��һ��֡������������
    if(pData->Frame == 0x15)     //��չ֡
    {
        len = ((u16)(pData->Data[1]) << 8) | (pData->Data[1 + 1]);
    }
    else if(pData->Frame == 0x13)     //����֡��ʽ  //zhouzhihua 2013.07.26 16:9
    {
        len = 0;     //����֡û������
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
** Created By:      ���ٲ�  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameDataLen(const FRAME_HEAD * const pData)
{
    //s32 len = 0;

    s32 len = ((u16)(pData->Data[4 + 1]) << 8) | (pData->Data[4 + 1 + 1]);

    return len;
}

/*****************************************************************************
** Descriptions:            �õ�Э��β��λ��
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameEndLocate(const FRAME_HEAD * const pData)
{
    s32 len = 0;

    if(pData->Frame == 0x15)
    {
        len = sdkGet01FrameDataLen(pData) + 2 + 1;     //������������
    }
    else if(pData->Frame == 0x13)     //û�н�����
    {
        len = 0;
    }
    else
    {
        len = sdkGet01FrameDataLen(pData) + 1 + 1;     //����һ������
    }
    return len;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameEndLocate(const FRAME_HEAD * const pData)
{
    s32 len = sdkGet02FrameDataLen(pData) + 4 + 1 + 2 + 2;

    return len;
}

/*****************************************************************************
** Descriptions:            �õ�02Э�鱣�����ݽ���λ��
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.17
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameRFUDataEndLocate(const FRAME_HEAD * const pData)
{
    //s32 len = 0;

    s32 len = 1 + 1 + 4;

    return len;
}

/*****************************************************************************
** Descriptions:            �õ�Э��ĳ������ݵ�λ��(������֡�е�λ��)
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameLenLocate(const FRAME_HEAD * const pData)
{
    s32 len = 0;

    if(pData->Frame == 0x13)     //����֡û�г���
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
** Created By:      ���ٲ�  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameLenLocate(const FRAME_HEAD * const pData)
{
    //s32 len = 0;

    s32 len = 1 + 1 + 4 + 1 + 2;

    return len;
}

/*****************************************************************************
** Descriptions:            �õ�����Э��ĳ���
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.15
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
** Created By:      ���ٲ�  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameLen(const FRAME_HEAD * const pData)
{
    //s32 len  = 0;

    s32 len = sdkGet02FrameDataLen(pData) + 1 + 1 + 4 + 1 + 2 + 2 + 1;

    return len;
}

/*****************************************************************************
** Descriptions:            �õ�Э��У�����ݵ�λ��
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.15
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
** Created By:      ���ٲ�  2013.04.19
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
** Descriptions:            �õ�Э��μӼ���У�����ݵĳ���
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameCheckDataLen(const FRAME_HEAD * const pData)
{
    s32 len = 0;

    if(pData->Frame == 0x13)     //����֡����
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
** Created By:      ���ٲ�  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameCheckDataLen(const FRAME_HEAD * const pData)
{
    //s32 len = 0;

    s32 len = sdkGet02FrameDataLen(pData) + 1 + 4 + 1 + 2;

    return len;
}

/*****************************************************************************
** Descriptions:            ����У���
** Parameters:          u8 const * const pata
                               u16 Len
** Returned value:
** Created By:      ���ٲ�  2013.04.15
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
** Descriptions:            �ж�Э��У�������Ƿ���ȷ
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.15
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
** Created By:      ���ٲ�  2013.04.19
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
** Descriptions:            �õ�Э�����С����
** Parameters:          void
** Returned value:
** Created By:      ���ٲ�  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkGet01FrameMinSize(void)
{
    return 4 + 3; //��������֡����
}

/*****************************************************************************
** Descriptions:
** Parameters:          void
** Returned value:
** Created By:      ���ٲ�  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkGet02FrameMinSize(void)
{
    return 12; //��������֡����
}

/*****************************************************************************
** Descriptions:
** Parameters:          Input: None
** Returned value:
** Created By:      ���ٲ�  2013.04.14
** Remarks:
*****************************************************************************/
static s32 sdkRemoveTrashData(SDK_QUEUE_HAND hand)
{
    s32 locate;
    u8 temp[4] = {0};

    //1����ȡ���г���
    s32 len = sdkQueueGetDataLen(hand);
    //Trace("zhouzhihua", "sdkRemoveTrashData len = %d \r\n", len);

    //2����λ����ͷ
    for(locate = 0; locate < len; locate++)
    {
        if(sdkQueueGetPosValue(hand, locate) == 0x02) //02Э��
        {
            if(locate > 0)
            {
                sdkQueueRemoveRange(hand, 0, locate); //ɾ����Э������
            }
            return sdkQueueGetDataLen(hand);
        }
        else if(sdkQueueGetPosValue(hand, locate) == 0x01) //01Э��
        {
            if(locate > 0)
            {
                sdkQueueRemoveRange(hand, 0, locate); //ɾ����Э������
            }

            if(sdkQueueGetData(hand, temp, 4) < 4) //˵�����ݻ�����
            {
                TraceHex("zhouzhihua", "sdkRemoveTrashData ok \r\n", temp, 4);
                return SDK_ESC;
            }
            else
            {
                TraceHex("zhouzhihua", "sdkRemoveTrashData1 ok \r\n", temp, 4);

				//����ҵ�ͬ��ͷ���ͷ���
                if(memcmp("\x01\x01\x01", temp, 3) == 0)
                {
                    return sdkQueueGetDataLen(hand);
                }
                else
                {
                	//�������Э��ͷ�ͽ���0x01Ҫȥ�� ��ֹ˫ѭ��
                    sdkQueueRemoveRange(hand, 0, 1); 
                    return SDK_ESC;
                }
            }
        }
    }
	
	//������ն��У�û�����������
    sdkQueueEmpey(hand);
    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:            Э�����ݽ���
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.15
** Remarks:
*****************************************************************************/
static s32 sdkMis01FrameDataParse(const FRAME_HEAD * const pData)
{
    /*****1���ж����ݿ��ǲ��Ƿ���Э����*****/

    if(pData->Frame == 0x13)     //����ֱ֡���ж�У��
    {
        if(sdkIs01FrameCheckOK(pData) == SDK_OK)     //������ȷ
        {
            return SDK_OK;
        }
    }
    else
    {
        if(pData->Data[sdkGet01FrameEndLocate(pData)] == 0x03)
        {
            if(sdkIs01FrameCheckOK(pData) == SDK_OK)     //������ȷ
            {
                Trace("zhouzhihua", "���ݽ������++++++++++++\r\n");
                return SDK_OK;
            }
        }
    }
    return SDK_ERR; //��������
}

/*****************************************************************************
** Descriptions:
** Parameters:          const P_FRAME_HEAD pData
** Returned value:
** Created By:      ���ٲ�  2013.04.19
** Remarks:
*****************************************************************************/
static s32 sdkMis02FrameDataParse(const FRAME_HEAD * const pData)
{
    /*****1���ж����ݿ��ǲ��Ƿ���Э����*****/
    if(pData->Data[sdkGet02FrameEndLocate(pData)] == 0x03)
    {
        if(sdkIs02FrameCheckOK(pData) == SDK_OK)        //������ȷ
        {
            return SDK_OK;
        }
    }
    return SDK_ERR; //��������
}

/*****************************************************************************
** Descriptions:
** Parameters:          SDK_QUEUE_HAND hand
                               SDK_FRAME_DATA *pFrameData
** Returned value:
** Created By:      ���ٲ�  2013.04.19
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

        if(len > sdkGet02FrameRFUDataEndLocate(pData)) //�жϱ�������
        {
            if(memcmp("\x0E\x01\x0B\x01", &(pData->Data[0]), 4) != 0) //����֡����
            {
                sdkQueueRemoveRange(hand, 0, 1); //ȥ������ͷ
            }
        }

        if(len < sdkGet02FrameLenLocate(pData) ) //�Ƿ񵽳���λ��
        {
            return SDK_ESC;
        }

        if(len < sdkGet02FrameLen(pData)) //�Ƿ�����������ܳ���
        {
            return SDK_ESC;
        }

        if(sdkMis02FrameDataParse(pData) == SDK_OK) //��������
        {
            //ȡ������֡��ԭ������
            sdkQueueGetData(hand, pFrameData->ucBuffer, sdkGet02FrameLen(pData));

            //�Ƴ�����֡
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
        else //�������ˣ�ֻȥ��Э��ͷ����������
        {
            sdkQueueRemoveRange(hand, 0, 1);
            return SDK_ERR;
        }
    }
}

/*****************************************************************************
** Descriptions:		��ȡ01Э������
** Parameters:          SDK_QUEUE_HAND hand
                        SDK_FRAME_DATA *pFrameData
** Returned value:
** Created By:      	���ٲ�  2013.04.19
** Remarks:         	lzl��ȷ�������ݵĸ�ʽ:0x01+framId+����+����+��Ч����+֡β+�ۼӺ�       
*****************************************************************************/
static s32 Get01FramData(SDK_QUEUE_HAND hand, SDK_FRAME_DATA *pFrameData)
{
    u8 data1;
    s32 len = 0;
    s32 ii = 0;

	//������ݳ��Ȳ��������˳�
    if( sdkQueueGetDataLen(hand)< sdkGet01FrameMinSize())
    {
        Trace("ped", "data 1 \r\n");
        return SDK_ESC;
    }

	//��ȡ��һ�����ݵ�ֵ
    data1 = sdkQueueGetPosValue(hand, 0);

	//�����һ��������0x01�ͼ������������Ǿ��˳�
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
        
		//ȡ�������е�����
        len = sdkQueueGetData(hand, buffer, sizeof(buffer));
        Trace("ped", "sdkQueueGetData len = %d\r\n buffer[0] = %02X\r\n", len, buffer[0]);

     	//�����һ��������0x01,�������ҳ���һ������0x01������λ��,Ȼ�󽫴�֡ͷ�����һ��01��ʼ����
        if(buffer[0] == 0x01)
        {
            for(ii = 1; ii < len; ii++)
            {
                if(buffer[ii] != 0x01)
                {
                	//����ӳ��(֡ͷ�����һ��01��ʼ)
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
        	//��������ͷ�ж����ݹ�����
            if(len < sdkGet01FrameLenLocate(pFHead) ) 
            {
                Trace("ped", "data 3 \r\n");
                return SDK_ESC;
            }
        }
		
		//�������������жϹ�����
        if(len < sdkGet01FrameLen(pFHead)) 
        {
            Trace("ped", "data 4 \r\n");
            return SDK_ESC;
        }
		
        /*****3����ʼ��������*****/
        if(sdkMis01FrameDataParse(pFHead) == SDK_OK)
        {
            if(pFHead->StartFlag == 0x01)
            {
                if( ii > 1)
                {
                	//ȥ�������01
                    sdkQueueRemoveRange(hand, 0, ii - 1);
                }
            }
            //ȡ������֡��ԭ������
            sdkQueueGetData(hand, pFrameData->ucBuffer, sdkGet01FrameLen(pFHead));

            //�Ƴ�����֡
            if(sdkGet01FrameLen(pFHead) > 0)
            {
                Verify( sdkQueueRemoveRange(hand, 0, sdkGet01FrameLen(pFHead)) >= 0 );
            }
            Trace("ped", "FrameLen = %d\r\n", sdkGet01FrameLen(pFHead));

			//���ص����ݳ���
            pFrameData->siBufferLen = sdkGet01FrameLen(pFHead);
			
            Trace("ped", "final ret len = %d\r\n", pFrameData->siBufferLen);
            return SDK_OK;
        }
        else //�������ˣ�ֻȥ��Э��ͷ����������
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
    s32 usleftLen;    /*****3����������*****/

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
** Descriptions: ���մ���Э������
** Parameters:
** Returned value:  ���ؽ��յ������ݵĳ���
** Created By:      twz  2013.04.15
** Remarks:			��Ҫ���ҵ�Э���ͷ�����жϽ��յ����ݵ���Ч��
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
        //��������
        g_framhand =  sdkQueueCreate(QueueSize());

        if(g_framhand == NULL)
        {
            Assert(0);
            return SDK_ERR;
        }
    }

    //��ȡ���ں�
    comid = (u8)sdkSysGetComNo(com);

    //��ȡ��ʱʱ���
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
        /*****1�����մ�������*****/
        memset(ucRecvBuf, 0, sizeof(ucRecvBuf));
        usRecvLen = sdkCommUartRecvData(comid, ucRecvBuf, sizeof(ucRecvBuf), 10);

		//������յ�������
        if(usRecvLen > 0)
        {
            TraceHex("ped", "sdkCommUartRecvData ok \r\n", ucRecvBuf, usRecvLen);

			//�ж϶����Ƿ��Ѿ�����
            if(usRecvLen + sdkQueueGetDataLen(g_framhand)> QueueSize())
            {
                Verify(sdkQueueEmpey(g_framhand) >= 0);
            }

			/*****2��ѹ���������*****/
            Verify(sdkQueueInsertData(g_framhand, ucRecvBuf, usRecvLen) > 0); 
        }

        /*=======BEGIN: ���ٲ� 2013.11.25  10:6 modify===========*/
        //��ȡ��Ч����
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
                /*=======BEGIN: ���ٲ� 2013.12.05  11:36 modify===========*/
                if(sdkTimerIsEnd(timerid, (u32)siTimerOut))
                {
                    rslt = SDK_TIME_OUT;
                    break;
                }
                /*====================== END======================== */
            }
        }
#if 0 /*Modify by ���ٲ� at 2013.12.05  11:36 */
        else //����ʧ�ܡ�//���ٲ� 2013.08.19 11:23
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

