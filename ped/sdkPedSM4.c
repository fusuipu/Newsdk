#include "sdkGlobal.h"

/*=======BEGIN: zhouzhihua 2013.07.25  15:2 modify===========*/
#define DC_CMD_CTRL_RES "\x11"  //dcָ���еĿ����������
//DCָ����������
#define DC_CMD_EX_DUKPT_INIT  0x1011     //dukpt��ʼ��
#define DC_CMD_EX_DUKPT_GET_PIN  0x1012  //�������
#define DC_CMD_EX_DUKPT_GET_MAC  0x1013  //���mac
#define SDK_DUKPT_KSN_LEN 10
#define SDK_DUKPT_KEY_LEN 16

/*****************************************************************************
** Descriptions:	����DUKPT��ʼ��
** Parameters: nMode����ʼ��ģʽ���ò�����δʹ�ã�Ŀǰ���ú���ʱ�ò����ɴ���0
          uInitKeyType�� 0 --- mackey
                         1 --- pinkey
                         2 --- dkey
          uKeyIndex��pinpad�б���ĳ�ʼ��Կ�����ţ���Χ0~9
          upInitKey������ĳ�ʼ��Կ����
          uInitKeyLen����ʼ��Կ���ݳ��ȣ��涨Ϊ16�ֽ�
          uKsnIndex��pinpad�б���ĳ�ʼKSN�����ţ���Χ0~9
          upInitKsn������ĳ�ʼKSN����
          uInitKsnLen����ʼKSN���ݳ��ȣ��涨Ϊ10�ֽ�
          siTimerOut����ʱʱ�䣨��λms�����ò�����δʹ�ã�Ŀǰ���ú���ʱ�ò����ɴ���0
   output����
   return�� 0 --- �ɹ�
   ����ֵ --- ʧ��
   ע�⣺ĿǰnMode��siTimerOut������δʹ�õ������ú���ʱ���ɴ���0������Ŀǰ��˵uKsnIndex��uKeyIndex����Ĳ���ֵ����һ���ġ�
** Created By:		fusuipu  2014.06.13
** Remarks:             ����DUKTP��ʼ��ʹ��FIFOָ�����ֱ�ӵ���libdev.so�Ľӿ�
*****************************************************************************/
static s32 sdkPedDukptInnerLoadInitKey(u8 nMode, u8 uInitKeyType, u8 * uKeyIndex, u8 * upInitKey, u8 uInitKeyLen, u8 * uKsnIndex, u8 * upInitKsn, u8 uInitKsnLen, s32 siTimerOut)
{
    static s32 (*p_fun)(u8 nMode, u8 uInitKeyType, u8 *uKeyIndex, u8 *upInitKey, u8 uInitKeyLen, u8 *uKsnIndex, u8 *upInitKsn, u8 uInitKsnLen, s32 siTimerOut) = NULL;
    s32 ret = 0;

    if(p_fun == NULL)
    {
        p_fun =  (s32 (*)(u8, u8, u8 *, u8 *, u8, u8 *, u8 *, u8, s32))sdk_dev_get_libdev_fun_hand("PedDukptLoadInitKey");
    }

    if(p_fun != NULL)
    {
        ret =  p_fun(nMode, uInitKeyType, uKeyIndex, upInitKey, uInitKeyLen, uKsnIndex, upInitKsn, uInitKsnLen, siTimerOut);

        if(0 == ret)
        {
            return SDK_OK;
        }
        else
        {
            return SDK_ERR;
        }
    }
    else
    {
        Trace("dev", "warming: PedDukptLoadInitKey");
        return SDK_ERR;
    }
}

/*****************************************************************************
** Descriptions:ʹ��dukpt��Կgetpin
** input��      ucMode��0 --- ����getpinģʽ
                        1 --- ����ģʽ��ֻ������pinһ��
                upPan��panblock������6�ֽ�
                upPwdLen������pin���ȷ�Χ�����ֽ�upPwdLen[0] --- ��С����
                upPwdLen[1] --- ��󳤶�
                uKsnIndx��KSN������
                siTimerOut����ʱʱ�䣬��λms
** output��     upPinBlock��getpin���������8�ֽ�
                upKsn��KSN������10�ֽ�
                upKsnLen��KSN����
** return��     0 --- �ɹ�
                ����ֵ --- ʧ��
   ע�⣺Ŀǰ����siTimerOut��δʹ�ã����ú���ʱ�ò����ɴ���0��

** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/
static s32 sdkPedDukptInnerGetPinFun(u8 ucMode, u8 * upPan, u8 * upPwdLen, u8 * upPinBlock, u8 uKsnIndx, u8 * upKsn, u8 * upKsnLen, s32 siTimerOut)
{
    static s32 (*p_fun)(u8 ucMode, u8 *upPan, u8 *upPwdLen, u8 *upPinBlock, u8 uKsnIndx, u8 *upKsn, u8 *upKsnLen, s32 siTimerOut) = NULL;

    if(p_fun == NULL)
    {
        p_fun =  (s32 (*)(u8, u8 *, u8 *, u8 *, u8, u8 *, u8 *, s32))sdk_dev_get_libdev_fun_hand("PedDukptGetPin");
    }

    if(p_fun != NULL)
    {
        return p_fun(ucMode, upPan, upPwdLen, upPinBlock, uKsnIndx, upKsn, upKsnLen, siTimerOut);
    }
    else
    {
        Trace("dev", "warming: PedDukptGetPin");
        return SDK_ERR;
    }
}

/*****************************************************************************

** Descriptions:        ����ʹ��dukpt��Կgetmac
** Parameters:          upData��Ҫ���ܵ�����
                        uspDatalen��Ҫ���ܵ����ݳ���
                        upDukptMode��   0 --- ������øýӿ�getmac֮ǰ���й���dukpt��Կgetpin����ʹ��֮ǰgetpin��dukpt��Կ��getmac�����û�У���ʹ���µ�dukpt��Կ��getmac��
                                        1 --- ʹ���µ�dukpt��Կgetmac
                        uKsnIndx�� KSN������
                        siTimerOut����ʱʱ�䣬��λms
   output��                upMac��getmac���������8�ֽ�
                        upKsn��KSN������10�ֽ�
                        upKsnLen��KSN����
   return:                 0 --- �ɹ�
                        ����ֵ --- ʧ��
   ע�⣺Ŀǰ����siTimerOut��δʹ�ã����ú���ʱ�ò����ɴ���0��

** Returned value:
** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/
static s32 sdkPedDukptInnerGetMac(u8 * upData, u16 uspDatalen, u8 * upDukptMode, u8 * upMac, u8 uKsnIndx, u8 * upKsn, u8 * upKsnLen, s32 siTimerOut)
{
    static s32 (*p_fun)(u8 *, u16, u8 *, u8 *, u8, u8 *, u8 *, s32) = NULL;

    if(p_fun == NULL)
    {
        p_fun =  (s32 (*)(u8 *, u16, u8 *, u8 *, u8, u8 *, u8 *, s32))sdk_dev_get_libdev_fun_hand("PedDukptGetMac");
    }

    if(p_fun != NULL)
    {
        return p_fun(upData, uspDatalen, upDukptMode, upMac, uKsnIndx, upKsn, upKsnLen, siTimerOut);
    }
    else
    {
        Trace("dev", "warming: PedDukptGetPin");
        return SDK_ERR;
    }
}
/*****************************************************************************

** Descriptions:        ����ʹ��dukpt��Կgetmac
** Parameters:          upData��Ҫ���ܵ�����
                        uspDatalen��Ҫ���ܵ����ݳ���
                        upDukptMode��   0 --- ������øýӿ�getmac֮ǰ���й���dukpt��Կgetpin����ʹ��֮ǰgetpin��dukpt��Կ��getmac�����û�У���ʹ���µ�dukpt��Կ��getmac��
                                        1 --- ʹ���µ�dukpt��Կgetmac
                        ucAlgo		��չ�㷨����
                        uKsnIndx�� KSN������
                        siTimerOut����ʱʱ�䣬��λms
   output��                upMac��getmac���������8�ֽ�
                        upKsn��KSN������10�ֽ�
                        upKsnLen��KSN����
   return:                 0 --- �ɹ�
                        ����ֵ --- ʧ��
   ע�⣺Ŀǰ����siTimerOut��δʹ�ã����ú���ʱ�ò����ɴ���0��

** Returned value:
** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/

static s32 sdkPedDukptInnerGetMacEx(u8 * upData, u16 uspDatalen, u8 * upDukptMode, u8 ucAlgo, u8 * upMac, u8 uKsnIndx, u8 * upKsn, u8 * upKsnLen, s32 siTimerOut)
{
    static s32 (*p_fun)(u8 *, u16, u8 *, u8, u8 *, u8, u8 *, u8 *, s32) = NULL;

    if(p_fun == NULL)
    {
        p_fun =  (s32 (*)(u8 *, u16, u8 *, u8, u8 *, u8, u8 *, u8 *, s32))sdk_dev_get_libdev_fun_hand("PedDukptGetMacExt");
    }

    if(p_fun != NULL)
    {
        return p_fun(upData, uspDatalen, upDukptMode, ucAlgo, upMac, uKsnIndx, upKsn, upKsnLen, siTimerOut);
    }
    else
    {
        Trace("dev", "warming: PedDukptGetMacExt");
        return SDK_ERR;
    }
}

/*****************************************************************************
** Descriptions:	��ȡDUKPT�����������
** Parameters:          const SDK_PED_PIN_CFG * pstPinCfg
** Returned value:
** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/
extern s32 sdkKbTransKey(s32 siKey);
static s32 sdkPEDDukptGetInnerKey(const SDK_PED_PIN_CFG * pstPinCfg)
{
    s32 i = 0, key = 0, pinlen = 0;
    u32 timer_id = 0, flash_id = 0;
    u8 disppin[32] = {0};
    u8 flash_flag = 0;                //���һ����˸��ͷ��������ʾ

    memset(disppin, 0, sizeof(disppin));
    timer_id = sdkTimerGetId();
    flash_id = timer_id;
    pinlen = (pstPinCfg->ePinMode == SDK_PED_MAG_PIN) ? 6 : 12;

//    sdkDispClearScreen();
//    sdkDispFillRowRam(SDK_DISP_LINE1, 0, "����������", SDK_DISP_DEFAULT);
//    sdkDispBrushScreen();

    while(1)
    {
        if(sdkTimerIsEnd(timer_id, (u32)pstPinCfg->iTimeOut))
        {
            return SDK_TIME_OUT;
        }

//���ַ���ʱ�����˸��ʾ
        if(0 == i)
        {
            if(true == sdkTimerIsEnd(flash_id, 500))
            {
                flash_flag = (flash_flag + 1) % 2;
                flash_id = sdkTimerGetId();
            }

            if(flash_flag == 0)
            {
                sdkDispRow(pstPinCfg->ucRow, 0, " ", SDK_DISP_RIGHT_DEFAULT);
            }
            else
            {
                sdkDispRow(pstPinCfg->ucRow, 0, "_", SDK_DISP_RIGHT_DEFAULT);
            }
        }
//������
        key = Private_sdkGetKeyValue();

        if(key <= 0)
        {
            continue;
        }
        timer_id = sdkTimerGetId();

        if( key == '*')
        {
        	if(i >= pinlen)
        	{
				sdkSysBeep(SDK_SYS_BEEP_ERR);
				continue;				
			}
            sdkSysBeep(SDK_SYS_BEEP_OK);
            disppin[i++] = (u8)key;
            sdkDispRow(pstPinCfg->ucRow, 0, disppin, SDK_DISP_RIGHT_DEFAULT);
        }
        else
        {
            key = sdkKbTransKey(key);

            if(key == SDK_KEY_CLEAR )
            {
                if(i != 0)
                {
                    sdkSysBeep(SDK_SYS_BEEP_OK);;
                }
                else
                {
                    sdkSysBeep(SDK_SYS_BEEP_ERR);
                }
                i = 0;
                memset(disppin, 0, sizeof(disppin));
            }
            else if(key == SDK_KEY_BACKSPACE)
            {
                if(i == 0)
                {
                    sdkSysBeep(SDK_SYS_BEEP_ERR);
                    memset(disppin, 0, sizeof(disppin));
                }
                else
                {
                    i--;
                    sdkSysBeep(SDK_SYS_BEEP_OK);
                    i = (0 >= i) ? (0) : (i);                                    //��ֹ����
                    disppin[i] = 0;
                    sdkDispRow(pstPinCfg->ucRow, 0, disppin, SDK_DISP_RIGHT_DEFAULT);
                }
            }
            else if(SDK_KEY_ESC == key)
            {
                sdkSysBeep(SDK_SYS_BEEP_OK);
                return SDK_ESC;
            }
            else if(key == SDK_KEY_ENTER )
            {
                sdkSysBeep(SDK_SYS_BEEP_OK);
                break;
            }
        }
    }

    if (i > 0 && i < 4)                                        // ���볤��С��4
    {
        return SDK_ERR;
    }
    else
    {
        if (!i)
        {
            return SDK_ERR;
        }
        else
        {
            return SDK_OK;
        }
    }
}

/*****************************************************************************
** Descriptions:
** Descriptions:    �������������������
** Parameters:      u8 ucMode  ���ΪDUKPT��ģʽ SDK_PED_DUKPT_GET_PIN_MODE
                    const SDK_PED_PIN_CFG *pstPinCfg
                    u8 *pucOutData ���ܺ������
                    u8 *pucKsn ksn��ֵ
** Returned value:
                    ����0	�����ɹ���������KSN�ĳ��ȣ����DUKPT��
                    SDK_PED_PIN_FORMAT_ERR	�����ʽ��
                    SDK_TIME_OUT	���������Ӧ��ʱ(��λ: ms)
                    SDK_ERR	����ʧ��
                    SDK_ESC	�û�ȡ������
                    SDK_PED_NOPIN	����������
** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/
static s32 sdkPEDDukptInsideInput(u8 ucMode,  const SDK_PED_PIN_CFG * pstPinCfg, u8 * upPwdLen, u8 * pucOutData, u8 ucIndex, u8 * pucKsn, u8 * pucKsnLen, s32 siTimer)
{
    s32 ret = 0;

    if(SDK_PED_DUKPT_GET_PIN_RELEASE == ucMode)
    {
        sdk_dev_key_set_filitermode(1, upPwdLen[0], upPwdLen[1], NULL, 0);

        if((ret = sdkPEDDukptGetInnerKey(pstPinCfg)) != SDK_OK)
        {
            sdk_dev_key_set_filitermode(0, upPwdLen[0], upPwdLen[1], NULL, 0);
            return ret;
        }
    }

    if(pstPinCfg->eKeyType == SDK_PED_DES_DUKPT)
    {
        ret =  sdkPedDukptInnerGetPinFun(ucMode, &pstPinCfg->hePan[2], upPwdLen, pucOutData, ucIndex, pucKsn, pucKsnLen, siTimer);
        sdk_dev_key_set_filitermode(0, upPwdLen[0], upPwdLen[1], NULL, 0);

        if(0 == ret)
        {
            return SDK_OK;
        }
        else
        {
            Assert(0);
            Trace("libsdkped", "ret = %d\r\n", ret);
            return SDK_ERR;
        }
    }
    else                                        //������ʱû������
    {
        sdk_dev_key_set_filitermode(0, upPwdLen[0], upPwdLen[1], NULL, 0);
        Trace("libsdkped", "pstPinCfg->eKeyType != SDK_PED_DES_DUKPT quite\r\n");
        return SDK_ERR;
    }
}

/*****************************************************************************
** Descriptions:	������DC��������������
** Parameters:                      u16 uiLen �����򳤶�(���ֽ����ڵ����ݳ��Ȳ��ܳ���256,����Ŀ��Բο��ĵ�)
                               u8 *pucData ����������
                               u16 uiCmd
                               u8 *pucOutData
** Returned value:	������ݳ���,����ֵ�Ǵ���0 �ġ�
** Created By:		zhouzhihua  2013.07.26
** Remarks:
*****************************************************************************/
static s32 sdkPedFormDCReserved(u16 uiLen, const u8 * pucData, u16 uiCmd, u8 * pucOutData)
{
    u8 temp[512]  = {0};
    u16 i;

    if(pucData == NULL || NULL == pucOutData)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(uiLen > 0xFF)
    {
        Assert(0);
    }
    i = 0;
    memset(temp, 0, sizeof(temp) );
    temp[i++] = (u8)uiLen;
    memcpy(&temp[i], pucData, (u8)uiLen);
    i += (u8)uiLen;
    temp[i++] = (uiCmd & 0xFF00) >> 8;
    temp[i++] = (u8)uiCmd;
    memcpy(pucOutData, temp, i);

    return i;
}

/*****************************************************************************
** Descriptions:	 DUKPT���� �������������
** Parameters:          u16 usCmd����������
                               u8 *pucData��������
                               u16 usDataLen���ݳ���
** Returned value:
** Created By:
** Remarks:         DUKPTָ��������ú�����ָ����Щ��һ��
                    ����ϲ㷽��Ϊ0����������̣�����������������̣�
                    ����ϲ㷽����1��û��������̣��������ⲿ�������
*****************************************************************************/
static s32 sdkPedDCSend(u16 usCmd, const u8 * pucSend, u16 usSendLen)
{
    FIFO fifo;
    u8 dest = 0;

    memset(&fifo, 0, sizeof(FIFO));
    fifo.Cmd = usCmd;

    /*=======BEGIN: fusuipu 2014.10.13  16:48 modify����0x11ָ����������0����ʾ
       ��������������̣����������1ʱ����ʾ��������������̡�����0x12ָ��,���������0��
       ��ʾ�������÷ǽӣ����������1����ʾ�������÷ǽӡ�����0x10ָ����������0ʱ����
       ʾ����������̣�����Ϊ1ʱ�������ڲ���===========*/
    dest = ((true == sdkPEDIsWithPinpad()) ? TOPINPAD : TOAUX51);

    if(FIFO_POSTDCCMD == usCmd && 0X11 == pucSend[1])
    {
        dest = (dest == TOAUX51 ? TOPINPAD : TOAUX51);
    }
    /*====================== END======================== */

    fifo.Data[0] = dest;                        //���û�����

    Trace("sdkped", ">>>>>>>>>>send to %s\r\n", (sdkPEDIsWithPinpad()) ? "TOPINPAD " : " TOAUX51");

    if (pucSend != NULL && usSendLen > 0)
    {
        memcpy(&fifo.Data[1], pucSend, usSendLen);
    }
    fifo.Len = usSendLen + 1;
    TraceHex("sdkped", "ped send fifo data", fifo.Data, fifo.Len);
    sdk_dev_clear_app_read_fifo();                        //zxx 20130131 16://shijianglong 2013.07.12 17:9
    sdk_dev_write_fifo(&fifo);
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:   �����ʼ����
** Parameters:
                   u8 ucIndex ����
                   u8 *pucKey ��ʼkey DK
                   u16 uiKeyLen ���Գ���
                   u8 *pucKsn ksn
                   u16 uiKsnLen ksn�ĳ���
                   s32 siTimer  ��ʱʱ��
** Returned value:
                    ����0	�����ɹ�
                    SDK_ESC	�û�ȡ��
                    SDK_PARA_ERR	��������
                    SDK_ESC	�û�ȡ��
                    SDK_TIME_OUT	��ʱ�˳�
                    SDK_ERR	����ʧ��
** Created By:		zhouzhihua  2013.07.24
** Remarks:
*****************************************************************************/
s32 sdkPEDDukptLoadAndInit(u8 ucIndex, const u8 * pucKey, u16 uiKeyLen, const u8 * pucKsn, u16 uiKsnLen, s32 siTimer)
{
    u8 ucksn[20] = {0}, ucbuf[512] = {0}, temp[1024] = {0};
    s32 rslt, i;
    u16 ucCmd[1] = {0}, len = 0;

    //UARTCFG stUartCfg;

    siTimer  = (siTimer < 500) ? 500 : siTimer;

    if(pucKey == NULL || NULL == pucKsn || uiKsnLen > SDK_DUKPT_KSN_LEN )
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(!sdkPEDIsWithPinpad())
    {
        rslt = sdkPedDukptInnerLoadInitKey(0, 2, &ucIndex, pucKey, uiKeyLen, &ucIndex, pucKsn, uiKsnLen, 0);

        if(SDK_OK == rslt)
        {
            return SDK_OK;
        }
        else
        {
            return SDK_ERR;
        }
    }
    else
    {
        memset(ucksn, 0xFF, sizeof(ucksn));

        if( uiKsnLen <= SDK_DUKPT_KSN_LEN )
        {
            memcpy(&ucksn[SDK_DUKPT_KSN_LEN - uiKsnLen], pucKsn, uiKsnLen);
        }

        if( uiKeyLen != SDK_DUKPT_KEY_LEN )                                    //Ŀǰkey�ĳ��ȱ�����16�ֽ�
        {
            Assert(0);
            return SDK_PARA_ERR;
        }
        memset(ucbuf, 0, sizeof(ucbuf));
        i = sdkPedFormDCReserved(1, DC_CMD_CTRL_RES, DC_CMD_EX_DUKPT_INIT, ucbuf);

        if( i < 0 )
        {
            Assert(0);
            return SDK_PARA_ERR;
        }
        ucbuf[i++] = SDK_PED_DUKPT_UPDATE_ALL;                                    //����ģʽ 0:ͬ�Ǹ���ksn��key;1:ֻ����key��2:ֻ����ksn
        ucbuf[i++] = ucIndex;
        ucbuf[i++] = SDK_DUKPT_KEY_LEN;
        memcpy(&ucbuf[i], pucKey, SDK_DUKPT_KEY_LEN);
        i += SDK_DUKPT_KEY_LEN;
        ucbuf[i++] = SDK_DUKPT_KSN_LEN;
        memcpy(&ucbuf[i], ucksn, SDK_DUKPT_KSN_LEN);
        i += SDK_DUKPT_KSN_LEN;
        ucCmd[0] = (u16)FIFO_POSTDCCMD;
        TraceHex("zhouzhihua", "ucbuf  ", ucbuf, i);
#if 0 /*Modify by zhouzhihua at 2014.05.08  11:4 */
        sSetUartCfg(&stUartCfg, POS_DC_COMMAND, ucbuf, (u16)i, ucCmd, 1, siTimer);
        rslt = sSendDataToPed(&stUartCfg);
#else
        sdkPedDCSend(FIFO_POSTDCCMD, ucbuf, (u16)i);
        memset(temp, 0, sizeof(temp));
        rslt = sdk_ped_receive(ucCmd, 1, temp, &len, SDK_PED_TIMEOUT, false);
        TraceHex("zhouzhihua", "temp  ", temp, len);
#endif /* if 0 */

//�õ���������ɿ����򳤶�1Byte + ����������(nBytes)+cmd(2Bytes)+ret(1Byte)
        if( rslt == SDK_OK )
        {
            memset(ucbuf, 0, sizeof(ucbuf));
            rslt = SDK_ERR;

            if( (temp[2] == ((DC_CMD_EX_DUKPT_INIT >> 8) & 0xFF)) && (temp[3] == (u8)DC_CMD_EX_DUKPT_INIT) && (temp[4] == 0))
            {
                Trace("zhouzhihua", "chenggong+++++++++ \r\n");
                rslt = SDK_OK;
            }
        }
        return rslt;
    }
}

/*****************************************************************************
** Descriptions:

** Parameters:          u8 ucMode 00 ����ģʽ��0x01 ����ģʽ
                        u8 ucIndex ����
                        u8 *pucPan 6Bytes BCD �����ʺż���
                        u16 uiMinLen ������С����
                        u16 uiMaxLen ������󳤶�
                        u8 *pucPinBlock ���ܺ������
                        s32 siTimer ��ʱ��
** Returned value:	����0 ksn�ĳ��ȣ�С�ڵ���0 ʧ��
** Created By:		zhouzhihua  2013.07.29
** Remarks:             Ŀǰ��������ʺż���
*****************************************************************************/
s32 sdkPEDDukptGetPin(u8 ucMode, u8 ucIndex, const SDK_PED_PIN_CFG * pstPinCfg, u8 uiMinLen, u8 uiMaxLen, u8 * pucPinBlock, u8 * pucKsn, u8 * pucKsnLen, s32 siTimer)
{
    u8 ucbuf[1024] = {0}, temp[1024] = {0};
    s32 rslt = 0, i = 0;
    u16 len = 0;
    u16 ucCmd[1];
    u8 pucPan[128] = {0};
    u8 len_buf[3] = {0};

    //UARTCFG stUartCfg;

    if( (uiMinLen < 4) || (uiMaxLen > 12) || (uiMinLen > uiMaxLen) || (NULL == pucPinBlock) || (pucKsn == NULL))
    {
        return SDK_PARA_ERR;
    }

    if( siTimer < 1000 )
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(!sdkPEDIsWithPinpad())
    {
        len_buf[0] = uiMinLen;
        len_buf[1] = uiMaxLen;
        return sdkPEDDukptInsideInput(ucMode, pstPinCfg, len_buf, pucPinBlock, ucIndex, pucKsn, pucKsnLen, siTimer);
    }
    else
    {
        siTimer -= 500;                                        //ȥ��500ms
        memset(ucbuf, 0, sizeof(ucbuf));
        memset(pucPan, 0, sizeof(pucPan));
        memcpy(pucPan, &pstPinCfg->hePan[2], 6);
        i = sdkPedFormDCReserved(1, DC_CMD_CTRL_RES, DC_CMD_EX_DUKPT_GET_PIN, ucbuf);

        if( i < 0 )
        {
            Assert(0);
            return SDK_PARA_ERR;
        }
        ucbuf[i++] = ucMode;
        ucbuf[i++] = ucIndex;
        ucbuf[i++] = uiMinLen;
        ucbuf[i++] = uiMaxLen;

        if( pucPan == NULL )                                    //�����Ҫ�������ʺŵļ��ܿ�����˴���
        {
            memset(&ucbuf[i], 0, 6);
        }
        else
        {
            memcpy(&ucbuf[i], pucPan, 6);
        }
        i += 6;
        ucbuf[i++] = (u8)(((u32)siTimer & 0xFF000000) >> 24);
        ucbuf[i++] = (u8)((siTimer & 0x00FF0000) >> 16);
        ucbuf[i++] = (u8)((siTimer & 0x0000FF00) >> 8);
        ucbuf[i++] = (u8)((siTimer & 0x000000FF));
        ucCmd[0] = FIFO_POSTDCCMD;

        TraceHex("zhouzhihua", "ucbuf  ", ucbuf, i);

        // sSetUartCfg(&stUartCfg, POS_DC_COMMAND, ucbuf, (u16)i, ucCmd, 1, siTimer);
        // rslt = sSendDataToPed(&stUartCfg);

        sdkPedDCSend(FIFO_POSTDCCMD, ucbuf, (u16)i);
        memset(temp, 0, sizeof(temp));
        rslt = sdk_ped_receive(ucCmd, 1, temp, &pucKsnLen, SDK_PED_TIMEOUT, false);
        len = *pucKsnLen;

        //�õ�������������򳤶�1Byte + ��������(nBytes)+cmd(2Bytes)+ret(1Byte)+����
        //data ����pinblock����(1Byte)+pinblock����(nByte)+ksn����(1Byte)+ksn����(nByte)
        if(rslt == SDK_OK)
        {
            memset(ucbuf, 0, sizeof(ucbuf));
            rslt = SDK_ERR;
            i = 1 + temp[0] + 2;                                    //���ؽ����λ��

            if( temp[i] == 0x00 )
            {
                len = temp[i + 1];                                    //pinblock ����
                memcpy(pucPinBlock, &temp[i + 1 + 1], 8);
                memcpy(pucKsn, &temp[i + 1 + 1 + len + 1], SDK_DUKPT_KSN_LEN);
                TraceHex("zhouzhihua", "pucKsn  ", pucKsn, 10);
                TraceHex("zhouzhihua", "pucPinBlock  ", pucPinBlock, 8);
                return SDK_OK;
            }
        }
        return rslt;
    }
}

/*****************************************************************************
** Descriptions:    DUKPT��ȡmac
** Parameters:          u8 ucMode 0x00 ��key �� 0x01��key, 0x10 ��key �� 0x01��key
                               u8 ucIndex  ����
                               u8 *pucInData ���ݱ���Ϊ ASCII
                               u16 uiDataLen ���ݳ���
                               u8 *pucMac �����mac
                               u8 *pucKsn
                               s32 siTimer
** Returned value:	����0 ksn�ĳ��� ��ȷ��С��0:ʧ�ܣ�ʧ�ܣ���ʱ���û�ȡ������������
** Created By:		zhouzhihua  2013.07.30
** Remarks:
*****************************************************************************/
s32 sdkPEDDukptGetMac(u8 ucMode, u8 ucIndex, const u8 * puascInData, u16 uiDataLen, u8 * pucMac, u8 * pucKsn, u8 * pucKsnLen, s32 siTimer)
{
    u8 ucbuf[512 + 512 + 64] = {0}, temp[1024] = {0};
    s32 rslt, i;
    u16 len = 0;
    u16 ucCmd[1];
	u8 alog = 0;
    //UARTCFG stUartCfg;

    if( (NULL == puascInData) || (NULL == pucMac) )
    {
        return SDK_PARA_ERR;
    }

    if(uiDataLen > 1024)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(!sdkPEDIsWithPinpad())
    {
    	if(1 == (ucMode >> 4 & 0x0F) || 2 == (ucMode >> 4 & 0x0F))
    	{
			alog = (ucMode >> 4 & 0x0F);	
			ucMode = ucMode & 0x0F;
			rslt = sdkPedDukptInnerGetMacEx(puascInData, uiDataLen, &ucMode, alog, pucMac, ucIndex, pucKsn, pucKsnLen, 0);
		}
		else
		{			
			rslt = sdkPedDukptInnerGetMac(puascInData, uiDataLen, &ucMode, pucMac, ucIndex, pucKsn, pucKsnLen, 0);
		}
        
        if(0 == rslt)
        {
            return SDK_OK;
        }
        else
        {
            return SDK_ERR;
        }
    }
    else
    {
        memset(ucbuf, 0, sizeof(ucbuf));
        i = sdkPedFormDCReserved(1, DC_CMD_CTRL_RES, DC_CMD_EX_DUKPT_GET_MAC, ucbuf);

        if( i < 0 )
        {
            Assert(0);
            return SDK_PARA_ERR;
        }
        ucbuf[i++] = ucMode;
        ucbuf[i++] = ucIndex;
        ucbuf[i++] = (uiDataLen & 0xFF00) >> 8;
        ucbuf[i++] = (u8)uiDataLen;
        memcpy(&ucbuf[i], puascInData, uiDataLen);
        i += uiDataLen;
        ucbuf[i++] = (u8)(((u32)siTimer & 0xFF000000) >> 24);
        ucbuf[i++] = (u8)((siTimer & 0x00FF0000) >> 16);
        ucbuf[i++] = (u8)((siTimer & 0x0000FF00) >> 8);
        ucbuf[i++] = (u8)((siTimer & 0x000000FF));
        ucCmd[0] = FIFO_POSTDCCMD;

        TraceHex("zhouzhihua", "ucbuf  ", ucbuf, i);

        //sSetUartCfg(&stUartCfg, POS_DC_COMMAND, ucbuf, (u16)i, ucCmd, 1, siTimer);
        // rslt = sSendDataToPed(&stUartCfg);

        sdkPedDCSend(FIFO_POSTDCCMD, ucbuf, (u16)i);
        memset(temp, 0, sizeof(temp));
        rslt = sdk_ped_receive(ucCmd, 1, temp, pucKsnLen, SDK_PED_TIMEOUT, false);
        len = *pucKsnLen;

        //�õ�������������򳤶�1Byte + ��������(nBytes)+cmd(2Bytes)+ret(1Byte)+data
        //data ����mac����(1Byte)+mac����(nByte)+ksn����(1Byte)+ksn����(nByte)
        if(rslt == SDK_OK)
        {
            memset(ucbuf, 0, sizeof(ucbuf));
            rslt = SDK_ERR;
            i = 1 + temp[0] + 2;                                    //���ؽ����λ��

            if( temp[i] == 0x00 )
            {
                len = temp[i + 1];
                memcpy(pucMac, &temp[i + 1 + 1], 8);
                memcpy(pucKsn, &temp[i + 1 + 1 + len + 1], SDK_DUKPT_KSN_LEN);
                TraceHex("zhouzhihua", "pucKsn  ", pucKsn, 10);
                TraceHex("zhouzhihua", "pucMac  ", pucMac, 8);
                return SDK_DUKPT_KSN_LEN;
            }
        }
        return rslt;
    }
}

/*******************************************************************
   ��    ��: ������
   ��    Ȩ: Xinguodu Ltd,Co.
   ��������: void XOR8(u8 *Dest, u8 *Scr1, u8 *Scr2)
   ��������: 8���ֽ����
   ��ڲ���: 1.Ŀ�ĵ�ַ; 2.Դ��ַ; 3.Դ��ַ2
   �� �� ֵ: ��
   ��ص���:
   ��    ע: 8���ֽڵ����
   �޸���Ϣ:
 ********************************************************************/
static void XORN(u8 *Dest, u8 *Scr1, u8 *Scr2, u32 Len)
{
    u8 i;

    for (i = 0; i < Len; i++)
    {
        *(Dest + i) = *(Scr1 + i) ^ *(Scr2 + i);
    }
}

/*****************************************************************************
** Descriptions:	ʹ��SM4�㷨���ܹ�����Կ
** Parameters:          Input: None
** Returned value:
** Created By:		fusuipu  2014.09.22
** Remarks:
*****************************************************************************/
static s32 sdkPedDCSendDateToPad(u8 *psrDate, u32 uiLen, u8 *pCmd, u8 uiCmdLen)
{
    u8 *pbuf = NULL;
    u32 i = 0;

    if(NULL == psrDate || NULL == pCmd)
    {
        return SDK_PARA_ERR;
    }

    if(NULL == (pbuf = sdkGetMem(uiLen + uiCmdLen + 1))) //1byte �Ƿ��������̣�Ԥ��1byte��ֹ���
    {
        return SDK_ERR;
    }
    memset(pbuf, 0, uiLen + uiCmdLen + 1);
    memcpy(pbuf, pCmd, uiCmdLen);
    i += uiCmdLen;

    memcpy(pbuf + i, psrDate, uiLen);
    i += uiLen;

    sdkPedDCSend(FIFO_POSTDCCMD, pbuf, i);
    sdkFreeMem(pbuf);
    pbuf = NULL;
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	��DCָ���ֵ���ж�
** Parameters:          void
** Returned value:
** Created By:		fusuipu  2015.05.05
** Remarks:             �����������ݵĿ�����
*****************************************************************************/
static s32 sdkPedDCReceviceJudge(bool bIsRetDate, u8 *lpOut, u8 *lpCmd, s32 siCmdLen, s32 siTimeOut)
{
    FIFO fifo;
    u32 time_id = 0;
    s32 key = 0;

    time_id = sdkTimerGetId();
    memset(&fifo, 0, sizeof(FIFO));

    while(1)
    {
        if(sdk_dev_read_fifo(&fifo, 100))
        {
            if(fifo.Cmd == FIFO_POSTDCCMD)
            {
                if((0 == memcmp(fifo.Data, lpCmd, siCmdLen)) && (0 == fifo.Data[4])) //0X00��ʾ�ɹ���0X01��ʾʧ��
                {
                    if(true == bIsRetDate)
                    {
                        memcpy(lpOut, &fifo.Data[5], fifo.Len - 5);
                    }
                    return SDK_OK;
                }
            }
        }

        if (0 != siTimeOut && sdkTimerIsEnd(time_id, siTimeOut)) //��ѯʱ���Ƿ�
        {
            return SDK_TIME_OUT;
        }
        key = sdkKbGetKey(); //shijianglong 2012.12.27 14:47�޸�ʹ�÷�ʽ

        if (key == SDK_KEY_ESC)
        {
            sdkSysBeep(SDK_SYS_BEEP_OK);
            return SDK_ERR;
        }
        else if (key != 0)
        {
            sdkSysBeep(SDK_SYS_BEEP_ERR);
        }
    }

    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:	ʹ��SM4�㷨���¹�����Կ
** Parameters:          u8 *pChectOutDate   ���ܹ�����Կ�󷵻ص�У������
                                        1B	                16B	            1B	                16 B
                                        ��һ���ݴ�������	��һ��У������	�ڶ����ݴ�������	�ڶ���У������

                    u8 *pucWkDate       ��Ҫ���µ�ԭʼ������Կ����
                                        ��һ������:1B	        1B	    16 B	1B	        16 B	1 B
                                               ����Կ������	���ĳ���	����	���ĳ���	����	�ݴ�������
                                        �ڶ�������:1B	        1B	    16 B	1B	        16 B	1 B
                                               ����Կ������	���ĳ���	����	���ĳ���	����	�ݴ�������

                    u32 uiLen           ��Ҫ���µĹ�����Կ���ݳ���
                    u32 uiTimeOut       ����ʱʱ��Ϊ0ʱ��ʹ��Ĭ�����õĵȴ�����������̵ĳ�ʱʱ��
                                        ����0ʱ����ʱʱ��ΪuiTimeOut��
                    VTable *pVTable
** Returned value:	SDK_OK             �����ɹ�
                    SDK_ESC            ����ʧ��
                    RSLT_OVERTIME       ����ʱʱ��Ϊ0ʱ��ʹ��Ĭ�����õĵȴ�����������̵ĳ�ʱʱ��
                                        ����0ʱ����ʱʱ��ΪuiTimeOut
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_derypt_wk_sm4(u8 *pChectOutDate, u8 *pucWkDate, u32 uiLen, u32 uiTimeOut)
{
    u8 cmd[5] = {0X01, 0X11, 0X10, 0x20, 0x00};
    u8 rst = 0;

    if(SDK_OK != (rst = sdkPedDCSendDateToPad(pucWkDate, uiLen, cmd, strlen(cmd))))
    {
        return rst;
    }
    return sdkPedDCReceviceJudge(true, pChectOutDate, cmd, 4,  uiTimeOut);
}

/*****************************************************************************
** Descriptions:	ʹ��SM4�㷨���¹�����Կ
** Parameters:          bool bEchoTak       �Ƿ񱣴���µĹ�����Կ��Ŀǰ����
                    u8 nTempTpkSNo      ��һ�鹤����Կ�ݴ�������
                    u8 nTpkSNo          ��һ�鹤����Կ����
                    u8 nTempTakSNo      �ڶ����ݴ�������
                    u8 nTakSNo          �ڶ��鹤����Կ����
                    VTable *pVTable
** Returned value:	SDK_OK             �����ɹ�
                    SDK_ESC            ����ʧ��
                    RSLT_OVERTIME       ��ʱʱ��ʹ��Ĭ�ϵ�������̵�fifo���ճ�ʱʱ��
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_flush_wk_sm4(bool bEchoTak, u8 nTempTpkSNo, u8 nTpkSNo, u8 nTempTakSNo, u8 nTakSNo, u32 uiTimeOut)
{
    u8 temp[10] = {0};
    FIFO fifo;
    u8 cmd[5] = {0X01, 0X11, 0X10, 0x21, 0x00};
    u8 rst = 0;
    u32 i = 0;
    u32 time_id = 0;

    memset(temp, 0, sizeof(temp));
    i = 0;
    temp[i++] = nTempTpkSNo;
    temp[i++] =  nTpkSNo;
    temp[i++] = nTempTakSNo;
    temp[i++] =  nTakSNo;

    if(SDK_OK != (rst = sdkPedDCSendDateToPad(temp, i, cmd, strlen(cmd))))
    {
        return rst;
    }
    memset(&fifo, 0, sizeof(FIFO));
    time_id = sdkTimerGetId();

    while(1)
    {
        if(sdk_dev_read_fifo(&fifo, 100))
        {
            if(fifo.Cmd == FIFO_POSTDCCMD)
            {
                if((0 == memcmp(fifo.Data, cmd, 4)) && (0 == fifo.Data[4]))
                {
                    return SDK_OK;
                }
            }
        }

        if (0 != uiTimeOut && sdkTimerIsEnd(time_id, uiTimeOut)) //��ѯʱ���Ƿ�
        {
            return SDK_TIME_OUT;
        }
    }

    return SDK_ERR;
}

/*******************************************************************
   ��	  ��: ������
   ��	  Ȩ: �����¹����ɷ����޹�˾
   ��������: ����SM4�㷨��ECB�㷨
   ��ڲ���: 1:��ŵĵ�ַ;2.��Mac����ָ��;3.��Mac���ݳ���
   �� �� ֵ: ���ص������16���ֽ�ASCII����
   ��	  ע:
 ********************************************************************/
static void sdkEcbSM4(u8 *lpOut, u8 *lpIn, u16 nLen)
{
    u16 i;
    u8 temp[256], A1[256];

    memset(A1, 0, sizeof(A1));

    for(i = 0; i < (nLen / 16); i++)
    {
        XORN(A1, A1, &lpIn[i * 16], 16);
    }

    if((nLen % 16) != 0)
    {
        memset(temp, 0, sizeof(temp));
        memcpy(temp, &lpIn[(nLen / 16) * 16], nLen - (nLen / 16) * 16);
        XORN(A1, A1, temp, 16);
    }
    sdkBcdToAsc(lpOut, A1, 16);
}

typedef enum
{
    SDK_SM4_ENCRYPT = 0x00,                                     //����
    SDK_SM4_DECRYPT = 0x01,                                     //����
}SDK_SM4_DEC_ENC_MODE;
/*****************************************************************************
** Descriptions:	����SM4�ļӽ����㷨
** Parameters:          u8 uiAlogMode
                                        0x00 SM4���ܣ�ֻ��ʹ��˫������Կ��128bit��
                                        0x01 SM4���ܣ�ֻ��ʹ��˫������Կ��128bit��
                                        0x02 �ų����ṩɽ��ũ���㷨
                    u8 uiKeyMode
                                        0x00 ����Կ
                                        0x01 ������Կ
                    u8 *lpOut           ���ܺ������
                    u8 *lpIn            �㷨����(1B)	��Կ����(1B)	��Կ������(1B)	����������(NB)
                    u16 nLen            ���������ݳ���
                    u32 uiTimeOut       ����ʱʱ��Ϊ0ʱ��ʹ��Ĭ�����õĵȴ�����������̵ĳ�ʱʱ��
                                        ����0ʱ����ʱʱ��ΪuiTimeOut��
                    VTable *pVTable
** Returned value:	SDK_OK             �����ɹ�
                    SDK_ESC            ����ʧ��
                    RSLT_OVERTIME       ����ʱʱ��Ϊ0ʱ��ʹ��Ĭ�����õĵȴ�����������̵ĳ�ʱʱ��
                                        ����0ʱ����ʱʱ��ΪuiTimeOut
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_wk_des_sm4(u8 uiAlogMode, u8 uiKeyMode, u8 nWkIndex, u8 *lpOut, u8 *lpIn, u16 nLen, u32 uiTimeOut)
{
    u8 *temp = NULL;
    u16 i = 0;
    u8 cmd[5] = {0X01, 0X11, 0X10, 0x22, 0x00};

    if(NULL == lpOut || NULL == lpIn)
    {
        return SDK_PARA_ERR;
    }

    if(NULL == (temp = sdkGetMem(3 + nLen + 1)))
    {
        return SDK_ERR;
    }
    memset(temp, 0, 3 + nLen + 1);
    i = 0;
    temp[i++] = uiAlogMode;                             //�㷨����
    temp[i++] = uiKeyMode;                              //��Կ����
    temp[i++] = nWkIndex;
    memcpy(&temp[i], lpIn, nLen);
    i += nLen;
    sdkPedDCSendDateToPad(temp, i, cmd, strlen(cmd));
    sdkFreeMem(temp);
    temp = NULL;

    return sdkPedDCReceviceJudge(true, lpOut, cmd, 4, uiTimeOut);
}

/*****************************************************************************
** Descriptions:	ʵ�ֻ���SM4�㷨��E90�㷨Mac
** Parameters:          u8 *lpOut           ���ܺ󷵻صĽ������
                    u8 *lpIn            ��Ҫ�����MAC���ݵ�PAN
                    u16 nLen            ��Ҫ�����MAC����PAN����
                    u8 nTakSno          ��Կ������
                    VTable *pVTable
** Returned value:	SDK_OK             �����ɹ�
                    SDK_ESC            ����ʧ��
                    RSLT_OVERTIME       ����ʱʱ��Ϊ0ʱ��ʹ��Ĭ�����õĵȴ�����������̵ĳ�ʱʱ��
                                        ����0ʱ����ʱʱ��ΪuiTimeOut
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_calc_mac_sm4(u8 *lpOut, u8 *lpIn, u16 nLen, u8 nTakSno)
{
    u8 mac[256] = {0}, temp[256] = {0};
    u8 rslt = 0;

    memset(mac, 0, sizeof(mac));
    sdkEcbSM4(mac, lpIn, nLen);
    memset(temp, 0, sizeof(temp));

    rslt = sdk_ped_wk_des_sm4(SDK_SM4_ENCRYPT, 0x01, nTakSno, temp, mac, 16, 10000);

    if(rslt != SDK_OK)
    {
        return rslt;
    }
    XORN(mac, temp, &mac[16], 16);

    rslt = sdk_ped_wk_des_sm4(SDK_SM4_ENCRYPT, 0x01, nTakSno, temp, mac, 16, 10000);

    if(rslt != SDK_OK)
    {
        return rslt;
    }
    sdkBcdToAsc(lpOut, temp, 8);

    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	ʵ�ֻ���SM4�㷨��CBC�㷨Mac
** Parameters:          u8 *lpOut           ���ܺ󷵻صĽ������
                    u8 *lpIn            ��Ҫ�����MAC���ݵ�PAN
                    u16 nLen            ��Ҫ�����MAC����PAN����
                    u8 nTakSno          ��Կ������
                    VTable *pVTable
** Returned value:	SDK_OK             �����ɹ�
                    SDK_ESC            ����ʧ��
                    RSLT_OVERTIME       ����ʱʱ��Ϊ0ʱ��ʹ��Ĭ�����õĵȴ�����������̵ĳ�ʱʱ��
                                        ����0ʱ����ʱʱ��ΪuiTimeOut
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_calc_mac_cbc_sm4(u8 *lpOut, u8 *lpIn, u16 nLen, u8 nTakSno)
{
    u8 temp[256] = {0}, A1[256] = {0};
    u8 rslt = 0;
    u16 i;
       
    memset(A1, 0, sizeof(A1));

    for(i = 0; i < (nLen / 16); i++)
    {
        XORN(A1, A1, &lpIn[i * 16], 16);
        if(SDK_OK != (rslt = sdk_ped_wk_des_sm4(SDK_SM4_ENCRYPT, 0x01, nTakSno, temp, A1, 16, 10000)))
        {
            return SDK_ERR;
        }
        memcpy(A1, temp, 16);
    }

    if((nLen % 16) != 0)
    {
        memset(temp, 0, sizeof(temp));
        memcpy(temp, &lpIn[(nLen / 16) * 16], nLen - (nLen / 16) * 16);
        XORN(A1, A1, temp, 16);
        if(SDK_OK != (rslt = sdk_ped_wk_des_sm4(SDK_SM4_ENCRYPT, 0x01, nTakSno, temp, A1, 16, 10000)))
        {
            return SDK_ERR;
        }
    }
    memcpy(lpOut, temp, 16);
    return SDK_OK;
}
/*****************************************************************************
** Descriptions:	��������ַ�������
** Parameters:      u8 uiMode           :Ŀǰ��֧��0x01
                    u8 *pVoiceStr       :{0, "0"},
                                         {1, "1"},
                                         {2, "2"},
                                         {3, "3"},
                                         {4, "4"},
                                         {5, "5"},
                                         {6, "6"},
                                         {7, "7"},
                                         {8, "8"},
                                         {9, "9"},
                                         {10, "."},
                                         {11, "*"},
                                         {12, "#"},
                                         {13, "Ԫ��"},
                                         {14, "��"},//���������⣬��������������ʹ��
                                         {15, "ˢ��"},////���������⣬��������������ʹ��
                                         {16, "�ٴ�"},////���������⣬��������������ʹ��
                                         {17, "ˢ��"},////���������⣬��������������ʹ��
                                         {18, "�����"},////���������⣬��������������ʹ��
                                         {19, "����"},////���������⣬��������������ʹ��
                                         {20, "������"},////���������⣬��������������ʹ��
                                         {21, "����"},////���������⣬��������������ʹ��
                                         {22, "������"},////���������⣬��������������ʹ��
                                         {23, "���»ӿ�"},
                                         {24, "��"},
                                         {25, "ˢ��"},
                                         {26, "�ٴ�ˢ��"},
                                         {27, "������"},
                                         {28, "�ٴ�������"},
                                         {29, "��������"},
                                         {30, "ȷ��"},
                                         {31, "ѡ��"},
                                         {32, "�忨"},
                                         {33, "BEEP(OK)"},
                                         {34, "BEEP(ERR)"},
                                         {35, "�����ظ�"},
                                         {36, "����"},
                                         {37, "�ɹ�"},
                                         {38, "ʧ��"},
                                         {39, "�ӿ�"},


                    s32 siStrLen         :�����ַ�������
                    s32 siTimeOut        :��ʱʱ��
** Returned value:  SDK_OK               :���ųɹ�
** Created By:		fusuipu  2015.05.05
** Remarks:         ��"�����ظ�ˢ��": 0x23,0x19
                      "�ӿ�ʧ��"    : 0x27,0x26
*****************************************************************************/
s32 sdkPEDVoicePlay(u8 uiMode, u8 *pVoiceStr, s32 siStrLen, s32 siTimeOut)
{
    s32 mode = 0x01, len = 0;
    u8 cmd[5] = {0X01, 0X10, 0X10, 0x12, 0x00};     //��������
    u8 date[512] = {0};
    s32 i = 0;

    if(NULL == pVoiceStr || siStrLen <= 0 || siTimeOut < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    date[i++] = mode;
    len = (siStrLen > 480 ? 480 : siStrLen);        //��ֹ���ݹ�������������̳�����
    date[i++] = len;
    memcpy(&date[i], pVoiceStr, len);
    i += len;
    sdkPedDCSendDateToPad(date, i, cmd, 4);
    return sdkPedDCReceviceJudge(false, NULL, cmd, 4, siTimeOut);
}

/*****************************************************************************
** Descriptions:	�������������С����
** Parameters:          u8 uiVolume
                               s32 siTimeOut
** Returned value:
** Created By:		fusuipu  2015.05.05
** Remarks:
*****************************************************************************/
s32 sdk_ped_volume_set(u8 uiVolume, s32 siTimeOut)
{
    u8 cmd[5] = {0X01, 0X10, 0X10, 0x13, 0x00};     //��������
    u8 date[128] = {0};
    s32 i = 0;
    s32 volume = 0;

    if(siTimeOut < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    volume = (uiVolume > 7 ? 7 : uiVolume);
    date[i++] = volume;
    sdkPedDCSendDateToPad(date, i, cmd, 4);
    return sdkPedDCReceviceJudge(false, NULL, cmd, 4, siTimeOut);
}

/*****************************************************************************
** Descriptions:	��ʾ������̻�ӭ����
** Parameters:          SDK_PED_WELCOME_STR *pWelStr
                               s32 siTimeOut
** Returned value:
** Created By:		fusuipu  2015.05.06
** Remarks:
*****************************************************************************/
s32 sdkPEDDispWelcomStr(SDK_PED_WELCOME_STR *pWelStr, s32 siTimeOut)
{
    u8 cmd[5] = {0X01, 0X10, 0X10, 0x11, 0x00};     //��������
    u8 date[512] = {0};
    s32 i = 0;
    s32 len = 0;

    if(NULL == pWelStr || siTimeOut < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    date[i++] = 0;                                  //����λԤ��
    date[i++] = 0x02;                               //��ʾ�ڵ����
    date[i++] = 0x34;                               //��һ���ڵ����tag
    len = strlen(pWelStr->ucLine1Str);
    date[i++] = len;

    if(len > 0)
    {
        strcpy(&date[i], pWelStr->ucLine1Str);
        i += len;
    }
    date[i++] = 0x35;                               //��һ���ڵ����tag
    len = strlen(pWelStr->ucLine2Str);
    date[i++] = len;

    if(len > 0)
    {
        strcpy(&date[i], pWelStr->ucLine2Str);
        i += len;
    }
    date[0] = i;
    sdkPedDCSendDateToPad(date, i, cmd, 4);
    return sdkPedDCReceviceJudge(false, NULL, cmd, 4, siTimeOut);
}
/*****************************************************************************
** Descriptions:	mode ��չ��������DCָ���·�
** Parameters:          SDK_PED_WELCOME_STR *pWelStr
                               s32 siTimeOut
** Returned value:
** Created By:		fusuipu  2015.05.06
** Remarks:
*****************************************************************************/
s32 sdk_ped_mode_ex(u8 *pucWkDate, u32 uiLen)
{
    u8 cmd[5] = {0X01, 0X14, 0x00, 0X10};
    u8 rst = 0;
	TraceHex("sdkExtLib", "pucWkDate", pucWkDate, uiLen);
    return sdkPedDCSendDateToPad(pucWkDate, uiLen, cmd, 4);
}

