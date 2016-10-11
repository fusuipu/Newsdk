#include "sdkGlobal.h"
/*Ϊ��ʵ�ָ����ĵ绰POS����Կ���ط�����sdkped.c�����Ҫ��Ӻ��޸ĵĽӿ����£�

��ע��Ϊ��ʵ��01Э��Ľ�����SDKʹ�õ���sdk0102Frame.c�еĽӿ�sdkCommRev01And02Fram��


һ����Ҫ��ӵĽӿ�
*/
 
//=================================================================//
/****************************�绰POS��������Կ���******************/
//=================================================================//

#define USERDATEMAXNUM     88 //�û��Զ�������С
#define USERDATEFILE       "SafeModeNoFile"  //�û��Զ������ļ���

/********************************************************************
��������: �����û��Զ���������--���ü���
�������:
�������:
�� �� ֵ:
�޸���Ϣ:
********************************************************************/
s32 sdk_ped_save_pinpad_user_date(u8 *pasIn, s32 uiInLen)
{
    u8 fn[64],gasCurAppDir[64];
    u8 FileData[128];

    if(NULL == pasIn || uiInLen > USERDATEMAXNUM || uiInLen<=0)
    {
        return SDK_ERR;
    }

    memset(fn,0,sizeof(fn));
    memset(gasCurAppDir,0,sizeof(fn));
    sdkSysGetCurAppDir(gasCurAppDir);
    strcpy(fn,gasCurAppDir);
    strcat(fn,USERDATEFILE);
    memset(FileData,0,sizeof(FileData));
    memcpy(FileData,pasIn,uiInLen);

    if(sdkWriteFile(fn,FileData,uiInLen) == SDK_FILE_OK)
    {
        return SDK_OK;
    }
    return SDK_ERR;
}

/********************************************************************
��������: ��ȡ�Զ���������--���ü���
�������:
�������:
�� �� ֵ:
�޸���Ϣ:
********************************************************************/
s32 sdk_ped_read_pinpad_user_date(u8 *pasOut,s32 *uiOutLen)
{
    u8 fn[64],gasCurAppDir[64];

    if(NULL == pasOut || *uiOutLen<=0 ||*uiOutLen>USERDATEMAXNUM)
    {
        return SDK_ERR;
    }

    memset(fn,0,sizeof(fn));
    memset(gasCurAppDir,0,sizeof(fn));
    sdkSysGetCurAppDir(gasCurAppDir);
    strcpy(fn,gasCurAppDir);
    strcat(fn,USERDATEFILE);

    if(sdkReadFile(fn, pasOut,0,uiOutLen) == SDK_FILE_OK)
    {
        TraceHex("AppDebug", "��ȡ�����Զ�����������", pasOut, *uiOutLen);
        return SDK_OK;
    }
    else
    {
        *uiOutLen=0;
        return SDK_ERR;
    }
}

/********************************************************************
��������: OK֡
�������:
�������:
�� �� ֵ:
�޸���Ϣ: 2015-08-21  ¬ԣ��  ����
********************************************************************/
static void SendDataOK(void)
{
    sdkCommUartSendData(sdkSysGetComNo(SDK_SYS_COM_PIN_PAD),"\x01\x01\x01\x01\x01\x13\x20\x34",8);
}

/********************************************************************
��������: ʧ��֡
�������:
�������:
�� �� ֵ:
�޸���Ϣ: 2015-08-21  ¬ԣ��  ����
********************************************************************/
static void SendDataErr(void)
{
    sdkCommUartSendData(sdkSysGetComNo(SDK_SYS_COM_PIN_PAD),"\x01\x01\x01\x01\x01\x13\x2f\x43",8);
}

/********************************************************************
��������:�������ݵ���ԿPOS
�������:
�������:
�� �� ֵ:
�޸���Ϣ: 2015-04-14  ������  ����
********************************************************************/
static s32 SendDataToKeyPos(u8 *heBuf,u16 unLen)
{
    u8 heRecvBuf[128]= {0};
    u8 len=0;

    memset(heRecvBuf,0x00,sizeof(heRecvBuf));
    memcpy(&heRecvBuf[len],"\x01\x01\x01\x01",4);//ͬ��ͷ
    len+=4;
    heRecvBuf[len++] = 0x01;//01Э��
    memcpy(&heRecvBuf[len],heBuf,unLen);
    len+=unLen;
    heRecvBuf[len++] = 0x03;//������־�ֽ�
    heRecvBuf[len] = sdkCalcCUSUM(&heRecvBuf[4],len-4);
    len+=1;

    sdkCommUartSendData(sdkSysGetComNo(SDK_SYS_COM_PIN_PAD),heRecvBuf,len);
    return SDK_OK;
}

/********************************************************************
��������: ������ԿPOS��������
�������:
�������:
�� �� ֵ:
��    ע:��Ҫ����8C��EE��FB��DB01��DB03��DB04ָ��
�޸���Ϣ: 2015-04-08  ������  ����
********************************************************************/
static s32 PraseKeyPosData(u8 ucFrameId,u16 ucCmd,u8 *heRecvBuf,s32 unRecvLen)
{
    const u8 StaticKey[8]= {0x36,0x38,0x34,0x38,0x32,0x35,0x36,0x39};//68482569 �̶���Կ,��������Կ���ܴ�����ܵ����
    u8 Bufferkey[8]= {0};
    static u8 EncryMode = 0xff;
    u8 temp[128]= {0},SData[128]= {0};
    u8 heTmk[16];
    s32 ret=0;
    u32 uiTmkIndex = 0;
    s32 rlen = 0;

    memset(heTmk,0x00,sizeof(heTmk));
    switch(ucCmd)
    {
        case 0x8C: //ȡ���������ָ��
        {
            Trace("AppDebug","PraseKeyPosData() case 0x8C\r\n");
            sdkSysBeep(SDK_SYS_BEEP_OK);
            SendDataOK();
            break;
        }
        case 0xEE: //����Կ���·�ʽ
        {
            Trace("AppDebug","PraseKeyPosData() case 0xEE\r\n����ģʽ:%x\r\n",heRecvBuf[0]);
            TraceHex("AppDebug","������Կ\r\n",&heRecvBuf[1],8);
            memcpy(Bufferkey,&heRecvBuf[1],8);
            EncryMode = heRecvBuf[0];
            sdkSysBeep(SDK_SYS_BEEP_OK);
            SendDataOK();
            break;
        }
        case 0xFB://���µ�������Կ
        {
            Trace("AppDebug","PraseKeyPosData() case 0xFB\r\n");
            Trace("AppDebug","TMK ����:%d\r\n",heRecvBuf[0]);
            TraceHex("AppDebug","TMK ����:%d\r\n",&heRecvBuf[1],unRecvLen-1);

            uiTmkIndex = heRecvBuf[0];
            if(uiTmkIndex > 101)
            {
                //ĸPOS�·�����3DESƫ���������ȥ��ƫ����
                uiTmkIndex = uiTmkIndex - 102;
            }
            memset(heTmk,0,sizeof(heTmk));
            if(unRecvLen == 0x09)
            {
                memcpy(heTmk, &heRecvBuf[1],8);

                //�������Կ�Ǽ��ܴ�ͳ
                if(EncryMode == 0x03)
                {
                    sdkDesS(0, heTmk, StaticKey);
                }
            }
            else if(unRecvLen == 0x11)
            {
                memcpy(heTmk, &heRecvBuf[1],16);

                //�������Կ�Ǽ��ܴ���
                if(EncryMode == 0x03)
                {
                    sdkDesS(0, heTmk, StaticKey);
                    sdkDesS(0, &heTmk[8], StaticKey);
                }
            }
            Trace("AppDebug","TMK uiTmkIndex:%d\r\n",uiTmkIndex);
            TraceHex("AppDebug","TMK ����:%d\r\n",heTmk,unRecvLen-1);

            //��������Կ
            ret = sdkPEDUpdateTmk(uiTmkIndex, SDK_PED_DES_TRIPLE, heTmk, SDK_PED_TIMEOUT);
            if(ret == SDK_OK)
            {
                SendDataOK();
                sdkSysBeep(SDK_SYS_BEEP_OK);
                Trace("AppDebug","����TMK�ɹ�!\r\n");
                return SDK_OK;
            }
            else
            {
                SendDataErr();
                sdkSysBeep(SDK_SYS_BEEP_ERR);
                Trace("AppDebug","����TMKʧ��!\r\n");
                return SDK_ERR;
            }
            break;
        }

        //====================DBָ��======================/
        case 0x01: //��ȡ���к�,���ڶ�ȡ�û��Զ�����������
        {
            SendDataOK();
            memset(temp,0,sizeof(temp));
            memset(SData,0,sizeof(SData));
            memcpy(SData,"\x12\xDB\x12\x01",4);

            //��ȡ��ȡ�Զ������ĳ���
            rlen = heRecvBuf[1];

            //��ȡ�û��Զ�����������
            if(sdk_ped_read_pinpad_user_date(temp,&rlen)==SDK_OK)
            {
                SData[4]=0x00;
            }
            else
            {
                SData[4]=0x01; //ʧ��
                memset(temp,0,sizeof(temp));
            }
            memcpy(&SData[5],temp,rlen);
            SendDataToKeyPos(SData, 5+rlen);
            break;
        }
        case 0x03://�������кţ�����д�û��Զ�����
        {
            SendDataOK();
            memset(SData,0,sizeof(SData));
            memcpy(SData,"\x12\xDB\x02\x03",4);
            if(sdk_ped_save_pinpad_user_date(&heRecvBuf[1], unRecvLen-1)==SDK_OK)
            {
                SData[4]=0x00;
            }
            else
            {
                SData[4]=0x01;
            }
            SendDataToKeyPos(SData, 5);
            break;
        }
        case 0x04://��ȡ�����������к�(SN)
        {
            SendDataOK();
            memset(temp,0,sizeof(temp));
            memset(SData,0,sizeof(SData));
            memcpy(SData,"\x12\xDB\x12\x04",4);
            if(sdkReadPosSn(temp)==SDK_OK)
            {
                SData[4]=0x00;
            }
            else
            {
                SData[4]=0x01;
            }
            memcpy(&SData[5],temp,11);
            SendDataToKeyPos(SData, 16);
            break;
        }
        default:
             break;
    }
    return SDK_OK;
}

/********************************************************************
��������:����ĸPOS��ָ��--01Э��
�������:
            u8 *ucFrameId   ֡ID
            u8 *uiCmd       ָ��
            u32 iTimeOut    ��ʱʱ��
�������:
            u8 *heRecvBuf   ��Ч����
            s32 *usLen      ��Ч���ݳ���
�� �� ֵ:
�޸���Ϣ: 2014-11-26  ������  ����
********************************************************************/
static s32 ResvDataFromKeyPos(u8 *ucFrameId,u16 *ucCmd,u8 *heRecvBuf,u16 *usLen)
{
    u8 rbuf[1024] = {0};
    s32 rlen = 0;
    u16 dccmdaddress = 0;

    *ucFrameId = 0;
    *ucCmd = 0;

    //����Ϊtrue��Ե�����
    rlen = sdkCommRev01And02Fram(SDK_SYS_COM_PIN_PAD, rbuf, sizeof(rbuf), 1, false);

    if(rlen > 0)
    {
        TraceHex("zwh", "resv ok \r\n", rbuf, rlen);
        if(rbuf[0] == 0x01)     //�ж���01Э��
        {
            *ucFrameId = rbuf[1];
            *ucCmd = rbuf[2];

            if(*ucFrameId == 0x15)//��չ֡
            {
                *usLen = rbuf[3] * 256 + rbuf[4];

                if( *ucCmd == 0xDC)
                {
                    //Э��ͷ1���ֽ�+֡��ʶ1���ֽ� +DC������1���ֽ� +����2���ֽ�+ �����򳤶�1���ֽ�+����������n�ֽ�+1
                    dccmdaddress = 5 + rbuf[5] + 1;
                    *ucCmd = ((rbuf[dccmdaddress] & 0xFF) << 8) | (rbuf[dccmdaddress + 1]);
                }
                else
                {
                    if(*ucCmd == 0xDD || *ucCmd == 0xDB )
                    {
                        *ucCmd = rbuf[5];        //����Ext_cmd����
                    }
                }
                memcpy(heRecvBuf, &rbuf[5], *usLen);
            }
            else if(*ucFrameId == 0x13) //��֡����ִ����ȷ����������ʱ�����Ҫ�������
            {
                //����֡
            }
            else
            {
                //����֡
                *usLen = rbuf[3];

                if(*ucCmd == 0xDC )
                {
                    //Э��ͷ1���ֽ�+֡��ʶ1���ֽ� +DC������1���ֽ� +����1���ֽ�+ �����򳤶�n���ֽ�+1
                    dccmdaddress = 4 + rbuf[4] + 1;
                    *ucCmd = ((rbuf[dccmdaddress] & 0xFF) << 8) | (rbuf[dccmdaddress + 1]);
                }
                else
                {
                    if(*ucCmd == 0xDD || *ucCmd == 0xDB ) //
                    {
                        *ucCmd = rbuf[4];      //����Ext_cmd����
                    }
                }
                memcpy(heRecvBuf, &rbuf[4], *usLen);//��2�������ȥ
            }
            TraceHex("zwh", "heRecvBuf", heRecvBuf, *usLen);
            Trace("zwh", "*usLen = %d\r\n", *usLen);
            Trace("zwh", "*uiCmd = %02x\r\n", *ucCmd);
            Trace("zwh", "������ȷ--0101010101132034\r\n");
            return SDK_OK;
        }
    }

    return SDK_ESC;
}

/*******************************************************************
��	  ��: �ӻ�
��	  Ȩ: �����¹����ɷ����޹�˾
��������: ��������Կ
��ڲ���:
�� �� ֵ:
��	  ע:
********************************************************************/
s32 sdkPEDImportTMKData(void)
{
    u8 rbuf[1024] = {0};
    s32 rlen = 0;
    u16 ucCmd=0;
    u8 ucFrameId = 0;
    s32 ret=0;

    memset(rbuf,0x00,sizeof(rbuf));
    ret = ResvDataFromKeyPos(&ucFrameId, &ucCmd, rbuf, &rlen);
    if(ret == SDK_OK)
    {
        TraceHex("AppDebug","����������н��յ�����:\r\n",rbuf,rlen);
        Trace("AppDebug","����uiCmd:%x\r\n",ucCmd);

        ret=PraseKeyPosData(ucFrameId,ucCmd, rbuf, rlen);
        Trace("AppDebug","PraseData--i_rslt:%d\r\n",ret);
    }
    return ret;
}
 