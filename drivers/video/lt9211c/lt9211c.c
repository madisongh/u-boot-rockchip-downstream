/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. *
 */
 
#include "lt9211c.h"

struct lt9211c_info {
	struct udevice *dev;
	int addr;
};

struct lvds_panel_priv {
	const char * lt9211c_mode_sel;
    LvdsOutParameter lvds_parameter;
	struct gpio_desc power_gpio;
	struct gpio_desc reset_gpio;
	StructChipRxVidTiming mipi_timing;
};

StructChipRxVidTiming mipi_timing;

struct udevice lt9211c_dev;

StructChipRx g_stChipRx;
StructChipTx g_stChipTx;
StructVidChkTiming g_stVidChk;
StructChipRxVidTiming g_stRxVidTiming;
u8 g_b1CscTodo = LONTIUM_FALSE;

void Mod_SystemTx_SetState(u8 ucState)
{
    if(ucState != g_stChipTx.ucTxState)
    {        
        g_stChipTx.ucTxState = ucState;
        g_stChipTx.b1TxStateChanged = LONTIUM_TRUE;
        pr_debug("TxState = 0x%02x", ucState);
    }
}

void Mod_SystemRx_SetState(u8 ucState)
{
    u8 ucLastState;
    if(ucState != g_stChipRx.ucRxState)
    {   
        ucLastState = g_stChipRx.ucRxState;
        g_stChipRx.ucRxState = ucState;
        g_stChipRx.b1RxStateChanged = LONTIUM_TRUE;
        pr_debug("RxState = 0x%02x", ucState);

        // other state-->STATE_HDMIRX_PLAY_BACK,need notify video on
        if(g_stChipRx.ucRxState == STATE_CHIPRX_PLAY_BACK)
        {
            g_stChipRx.pHdmiRxNotify(MIPIRX_VIDEO_ON_EVENT);
        }

        //STATE_HDMIRX_PLAY_BACK-->other state,need notify video off
        if(ucLastState == STATE_CHIPRX_PLAY_BACK)
        {
            g_stChipRx.pHdmiRxNotify(MIPIRX_VIDEO_OFF_EVENT);
        }
    }
}

void Mod_SystemRx_NotifyRegister(void (*pFunction)(EnumChipRxEvent ucEvent))
{
    g_stChipRx.pHdmiRxNotify  = pFunction;
}

void Mod_SystemTx_PowerOnInit(void)
{
    memset(&g_stChipTx, 0, sizeof(StructChipTx));
    g_stChipTx.ucTxState = STATE_CHIPTX_POWER_ON;
}

void Mod_SystemRx_PowerOnInit(void)
{
    memset(&g_stChipRx,0 ,sizeof(StructChipRx));
    memset(&g_stVidChk,0 ,sizeof(StructVidChkTiming));
    memset(&g_stRxVidTiming,0 ,sizeof(StructChipRxVidTiming));
    
    g_stChipRx.ucRxState = STATE_CHIPRX_POWER_ON;    

    Mod_SystemRx_NotifyRegister(Mod_System_RxNotifyHandle);
}

void Mod_System_RxNotifyHandle(EnumChipRxEvent ucEvent)
{
    switch (ucEvent)
    {
        case MIPIRX_VIDEO_ON_EVENT:
            g_stChipTx.b1UpstreamVideoReady = LONTIUM_TRUE;
            break;
        case MIPIRX_VIDEO_OFF_EVENT:
            g_stChipTx.b1UpstreamVideoReady = LONTIUM_FALSE;
            break;
        case MIPIRX_CSC_EVENT:
            g_b1CscTodo = LONTIUM_TRUE;
        default:
            break;
    }
}

void Drv_SystemActRx_Sel(u8 ucSrc)
{
    lt9211c_write(0xff,0x85);
    lt9211c_write(0x30,(lt9211c_read(0x30) & 0xf8));

    switch(ucSrc)
    {
        case LVDSRX:
            lt9211c_write(0x30,(lt9211c_read(0x30) | LVDSRX));
            lt9211c_write(0x30,(lt9211c_read(0x30) & 0xcf)); //[5:4]00: LVDSRX
        break;
        case MIPIRX:
            lt9211c_write(0x30,(lt9211c_read(0x30) | MIPIRX));
            lt9211c_write(0x30,(lt9211c_read(0x30) | BIT4_1)); //[5:4]01: MIPIRX
        break;
        case TTLRX:
            lt9211c_write(0x30,(lt9211c_read(0x30) | TTLRX));
        break;
        case PATTERN:
            lt9211c_write(0x30,(lt9211c_read(0x30) | PATTERN));
        break;
        default:
            lt9211c_write(0x30,(lt9211c_read(0x30) | LVDSRX));
        break;
        
    }
}

void Drv_SystemTxSram_Sel(u8 ucSrc)
{
    //[7:6]2'b00: TX Sram sel MIPITX; others sel LVDSTX
    lt9211c_write(0xff,0x85);
    lt9211c_write(0x30,(lt9211c_read(0x30) & 0x3f)); 

    switch(ucSrc)
    {
        case LVDSTX:
            lt9211c_write(0x30,(lt9211c_read(0x30) | BIT6_1));
        break;
        case MIPITX:
            lt9211c_write(0x30,(lt9211c_read(0x30) & BIT6_0));
        break;
    }
}

u8 Drv_System_GetPixelEncoding(void)
{
    return g_stChipRx.ucRxFormat;
}


void Drv_System_VidChkClk_SrcSel(u8 ucSrc)
{
    lt9211c_write(0xff,0x81);
    lt9211c_write(0x80,(lt9211c_read(0x80) & 0xfc));

    switch (ucSrc)
    {
        case RXPLL_PIX_CLK:
            lt9211c_write(0x80,(lt9211c_read(0x80) | RXPLL_PIX_CLK));
        break;
        case DESSCPLL_PIX_CLK:
            lt9211c_write(0x80,(lt9211c_read(0x80) | DESSCPLL_PIX_CLK));
        break;
        case RXPLL_DEC_DDR_CLK:
            lt9211c_write(0x80,(lt9211c_read(0x80) | RXPLL_DEC_DDR_CLK));
        break;
        case MLRX_BYTE_CLK:
            lt9211c_write(0x80,(lt9211c_read(0x80) | MLRX_BYTE_CLK));
        break;    
        
    }

}

void Drv_System_VidChk_SrcSel(u8 ucSrc)
{
    lt9211c_write(0xff,0x86);
    lt9211c_write(0x3f,(lt9211c_read(0x80) & 0xf8));

    switch (ucSrc)
    {
        case LVDSRX:
            lt9211c_write(0x3f,LVDSRX);
        break;
        case MIPIRX:
            lt9211c_write(0x3f,MIPIRX);
        break;
        case TTLRX:
            lt9211c_write(0x3f,TTLRX);
        break;
        case PATTERN:
            lt9211c_write(0x3f,PATTERN);
        break;
        case LVDSDEBUG:
            lt9211c_write(0x3f,LVDSDEBUG);
        case MIPIDEBUG:
            lt9211c_write(0x3f,MIPIDEBUG);
        break;
        case TTLDEBUG:
            lt9211c_write(0x3f,TTLDEBUG);
        break;    
        
    }

}


u16 Drv_VidChkSingle_Get(u8 ucPara)
{ 
    u16 usRtn = 0;

    lt9211c_write(0xff,0x81);
    lt9211c_write(0x0b,0x7f);
    lt9211c_write(0x0b,0xff);
    mdelay(80);
    lt9211c_write(0xff,0x86);
    switch(ucPara)
    {
        case HTOTAL_POS:
            usRtn = (lt9211c_read(0x60) << 8) + lt9211c_read(0x61);
        break;
        case HACTIVE_POS:
            usRtn = (lt9211c_read(0x5c) << 8) + lt9211c_read(0x5d);  
        break;
        case HFP_POS:
            usRtn = (lt9211c_read(0x58) << 8) + lt9211c_read(0x59);
        break;
        case HSW_POS:
            usRtn = (lt9211c_read(0x50) << 8) + lt9211c_read(0x51);
        break;    
        case HBP_POS:
            usRtn = (lt9211c_read(0x54) << 8) + lt9211c_read(0x55);
        break;
        case VTOTAL_POS:
            usRtn = (lt9211c_read(0x62) << 8) + lt9211c_read(0x63);
        break;
        case VACTIVE_POS:
            usRtn = (lt9211c_read(0x5e) << 8) + lt9211c_read(0x5f);
        break;
        case VFP_POS:
            usRtn = (lt9211c_read(0x5a) << 8) + lt9211c_read(0x5b);
        break;
        case VSW_POS:
            usRtn = (lt9211c_read(0x52) << 8) + lt9211c_read(0x53);
        break;
        case VBP_POS:
            usRtn = (lt9211c_read(0x56) << 8) + lt9211c_read(0x57);
        break;
        case HSPOL_POS:
            usRtn = (lt9211c_read(0x4f) & 0x01);
        break;
        case VSPOL_POS:
            usRtn = (lt9211c_read(0x4f) & 0x02);
        break;
        default:
        break;
    }
    return usRtn;
}

void Drv_VidChkAll_Get(StructVidChkTiming *video_time)
{
    video_time->usHtotal    =     Drv_VidChkSingle_Get(HTOTAL_POS);
    video_time->usHact      =     Drv_VidChkSingle_Get(HACTIVE_POS);
    video_time->usHfp       =     Drv_VidChkSingle_Get(HFP_POS);
    video_time->usHs        =     Drv_VidChkSingle_Get(HSW_POS);
    video_time->usHbp       =     Drv_VidChkSingle_Get(HBP_POS);
    
    video_time->usVtotal    =     Drv_VidChkSingle_Get(VTOTAL_POS);
    video_time->usVact      =     Drv_VidChkSingle_Get(VACTIVE_POS);
    video_time->usVfp       =     Drv_VidChkSingle_Get(VFP_POS);
    video_time->usVs        =     Drv_VidChkSingle_Get(VSW_POS);
    video_time->usVbp       =     Drv_VidChkSingle_Get(VBP_POS);
    
    video_time->ucHspol     =     Drv_VidChkSingle_Get(HSPOL_POS);
    video_time->ucVspol     =     Drv_VidChkSingle_Get(VSPOL_POS);        
    video_time->ucFrameRate =     Drv_VidChk_FrmRt_Get(); 
}

u8 Drv_VidChk_FrmRt_Get(void)
{
    u8 ucframerate = 0; 
    u32 ulframetime = 0;

    lt9211c_write(0xff,0x86);
    ulframetime = lt9211c_read(0x43);
    ulframetime = (ulframetime << 8) + lt9211c_read(0x44);
    ulframetime = (ulframetime << 8) + lt9211c_read(0x45);
    ucframerate = (u8)((250000000 / (ulframetime*10)) + 1); //2500000/ulframetime
    return ucframerate;
}

u32 Drv_System_FmClkGet(u8 ucSrc)
{
    u32 ulRtn = 0;
    lt9211c_write(0xff,0x86);
    lt9211c_write(0X90,ucSrc);
    mdelay(5);
    lt9211c_write(0x90,(ucSrc | BIT7_1));
    ulRtn = (lt9211c_read(0x98) & 0x0f);
    ulRtn = (ulRtn << 8) + lt9211c_read(0x99);
    ulRtn = (ulRtn << 8) + lt9211c_read(0x9a);
    lt9211c_write(0x90,(lt9211c_read(0x90) & BIT7_0));
    return ulRtn;
}

int lt9211c_write(u8 reg, u8 val)
{
    int ret = 0;
    ret =  dm_i2c_reg_write(&lt9211c_dev, reg, val);
    if(ret){
        //dev_err(dev,"write address failed 0x%x\n",reg);
    }
    return ret;
}

int lt9211c_read(u8 reg)
{
    int ret = 0;
    ret = dm_i2c_reg_read(&lt9211c_dev, reg);
    return ret;
}

static int lt9211c_parse_dt(struct udevice *dev,struct lvds_panel_priv *priv)
{
    int ret = 0;
    unsigned int ulPclk;

    ret = gpio_request_by_name(dev, "power-gpio", 0,
				   &priv->power_gpio, GPIOD_IS_OUT);
	if (ret && ret != -ENOENT) {
		dev_err(dev,"%s: Cannot get pwr GPIO: %d\n", __func__, ret);
		return ret;
	}

    ret = gpio_request_by_name(dev, "reset-gpio", 0,
				   &priv->reset_gpio, GPIOD_IS_OUT);
	if (ret && ret != -ENOENT) {
		dev_err(dev,"%s: Cannot get pwr GPIO: %d\n", __func__, ret);
		return ret;
	}

    priv->lt9211c_mode_sel = dev_read_string(dev, "mod");

    priv->lvds_parameter.lvdstx_port_sel = dev_read_string(dev, "lvdstx_port_sel");

    dev_dbg(dev,"lvdstx_port_sel=%s\n",priv->lvds_parameter.lvdstx_port_sel);

    priv->lvds_parameter.lvdstx_port_swap =  dev_read_string(dev, "lvdstx_port_swap");
    dev_dbg(dev,"lvdstx_port_swap=%s\n",priv->lvds_parameter.lvdstx_port_swap);

    ulPclk = dev_read_u32_default(dev, "clock-frequency", 0);

    priv->mipi_timing.ulPclk_Khz = ulPclk/1000;
    
    priv->mipi_timing.usHfp = dev_read_u32_default(dev, "hfront-porch", 0);
    
    priv->mipi_timing.usHs = dev_read_u32_default(dev, "hsync-len", 0);

    priv->mipi_timing.usHbp = dev_read_u32_default(dev, "hback-porch", 0);

    priv->mipi_timing.usHact = dev_read_u32_default(dev, "hactive", 0);
    
    priv->mipi_timing.usVfp = dev_read_u32_default(dev, "vfront-porch", 0);

    priv->mipi_timing.usVs = dev_read_u32_default(dev, "vsync-len", 0);
    
    priv->mipi_timing.usVbp = dev_read_u32_default(dev, "vback-porch", 0);

    priv->mipi_timing.usVact = dev_read_u32_default(dev, "vactive", 0);

    priv->mipi_timing.ucFrameRate = dev_read_u32_default(dev, "frameRate", 0);

    dev_dbg(dev,"ulPclk_Khz=%d\n", priv->mipi_timing.ulPclk_Khz);
    dev_dbg(dev,"ucFrameRate=%d\n", priv->mipi_timing.ucFrameRate);
    dev_dbg(dev,"hfront-porch=%d,hsync-len=%d,hback-porch=%d,hactive=%d\n",
    priv->mipi_timing.usHfp,priv->mipi_timing.usHs,priv->mipi_timing.usHbp,priv->mipi_timing.usHact);

    dev_dbg(dev,"vfront-porch=%d,vsync-len=%d,vback-porch=%d,vactive=%d\n",
    priv->mipi_timing.usVfp,priv->mipi_timing.usVs,priv->mipi_timing.usVbp,priv->mipi_timing.usVact);

    priv->mipi_timing.usHtotal = priv->mipi_timing.usHfp + priv->mipi_timing.usHs + priv->mipi_timing.usHbp + priv->mipi_timing.usHact;
    priv->mipi_timing.usVtotal = priv->mipi_timing.usVfp + priv->mipi_timing.usVs + priv->mipi_timing.usVbp + priv->mipi_timing.usVact;

    return ret;
}

static void mod_lt9211c_reset(struct lvds_panel_priv *priv){

    dm_gpio_set_value(&priv->power_gpio,1);
    mdelay(100);
    dm_gpio_set_value(&priv->reset_gpio,0);
    mdelay(100);
    dm_gpio_set_value(&priv->reset_gpio,1);
}

static int mod_chipid_read(struct udevice *dev)
{
    u8 retry = 0;
    u8 chip_id_h = 0, chip_id_m = 0, chip_id_l = 0;
    int ret = -EAGAIN;

    while(retry++ < 3) {
        ret = lt9211c_write(0xff, 0x81);

        if(ret < 0) {
            //dev_err(dev, "LT9211C i2c test write addr:0xff failed\n");
            continue;
        }

        ret = lt9211c_write(0x08, 0x7f);

        if(ret < 0) {
            dev_err(dev,"LT9211C i2c test write addr:0x08 failed\n");
            continue;
        }
        chip_id_l = lt9211c_read(0x00);
        chip_id_m = lt9211c_read(0x01);
        chip_id_h = lt9211c_read(0x02);

        printf( "LT9211C i2c test success chipid: 0x%x%x%x\n", chip_id_l, chip_id_m, chip_id_h);

        if (chip_id_h == 0 && chip_id_l == 0 && chip_id_m == 0) {
            dev_err(dev,"LT9211C i2c test failed time %d\n", retry);
            continue;
        }

        ret = 0;
        break;
    }

    return ret;
}
static int lt9211c_config(struct udevice *dev,struct lvds_panel_priv *priv)
{
    int count = 0;
    mod_lt9211c_reset(priv);
    mod_chipid_read(dev);
    if(!strstr(priv->lt9211c_mode_sel, "pattern_out")){
        Mod_SystemRx_PowerOnInit();
        Mod_SystemTx_PowerOnInit();
        while(1) {
            count++;
            if(strstr(priv->lt9211c_mode_sel, "mipi_in_lvds_out")){
                Mod_MipiRx_Handler(&priv->mipi_timing);
                Mod_LvdsTx_Handler(&priv->lvds_parameter);
            }
            if((g_stChipRx.ucRxState == STATE_CHIPRX_PLAY_BACK) && (g_stChipTx.ucTxState == STATE_CHIPTX_PLAY_BACK))
                break;
            if (count>30) {
                dev_err(dev,"No video source access!\n");
                break;
            }
        }
    }
    return 0;
}

static int lt9211c_probe(struct udevice *dev)
{
    int ret = 0;

    struct lt9211c_info *info = dev_get_platdata(dev);
    struct lvds_panel_priv *priv = dev_get_priv(dev);
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);
    int addr;
    lt9211c_dev = *dev;
    if (!info) {
		dev_err(dev, "platdata not ready\n");
		return -ENOMEM;
	}

	if (!chip) {
		dev_err(dev, "i2c not ready\n");
		return -ENODEV;
	}

	addr = ofnode_read_s32_default(dev->node, "reg", 0);
    if (addr == 0)
		return -ENODEV;
	dev_dbg(dev,"device i2c addr is 0x%x\n",addr);

    info->addr = addr;
	ret = lt9211c_parse_dt(dev,priv);
    if (ret) {
        dev_err(dev, "Lontium Failed to parse device tree\n");
        return -ENOMEM; 
    }
    lt9211c_config(dev,priv);
    return ret;
}


static const struct udevice_id lt9211c_ids[] = {
	{ .compatible = "loutium, LT9211C" },
	{ }
};

U_BOOT_DRIVER(lt9211c) = {
	.name		= "lt9211c",
	.id	= UCLASS_VIDEO,
	.of_match	= lt9211c_ids,
	.probe		= lt9211c_probe,
    .priv_auto_alloc_size = sizeof(struct lvds_panel_priv),
	.platdata_auto_alloc_size	= sizeof(struct lt9211c_info),
};


