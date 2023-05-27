#include "mipirx-lvdstx.h"

StructPcrPara g_stPcrPara;
SrtuctMipiRx_VidTiming_Get g_stMipiRxVidTiming_Get;
 
#define MIPIRX_FORMAT_CNT   0x0f
char* g_szStrRxFormat[MIPIRX_FORMAT_CNT] = 
{
    "",
    "DSI YUV422 10bit",
    "DSI YUV422 12bit",
    "YUV422 8bit",
    "RGB 10bit",
    "RGB 12Bit",
    "YUV420 8bit",
    "RGB 565",
    "RGB 666",
    "DSI RGB 6L",
    "RGB 8Bit",
    "RAW8",
    "RAW10",
    "RAW12",
    "CSI YUV422 10",
};

void DRV_DesscPll_SdmCal(void)
{
    lt9211c_write(0xff,0xd0);//      
    lt9211c_write(0x08,0x00);//sel mipi rx sdm  

    lt9211c_write(0x26,0x80 | ((u8)g_stPcrPara.Pcr_M)); //m
    lt9211c_write(0x2d,g_stPcrPara.Pcr_UpLimit); //PCR M overflow limit setting.
    lt9211c_write(0x31,g_stPcrPara.Pcr_DownLimit); //PCR M underflow limit setting. 
    
    lt9211c_write(0x27,(u8)(g_stPcrPara.Pcr_K >> 16)); //k
    lt9211c_write(0x28,(u8)(g_stPcrPara.Pcr_K >> 8)); //k
    lt9211c_write(0x29,(u8)(g_stPcrPara.Pcr_K)); //k  
    lt9211c_write(0x26,(lt9211c_read(0x26) & 0x7f));
   
}

void Drv_MipiRx_DesscPll_Set(void)
{
    u8 ucdesscpll_pixck_div = 0;

    lt9211c_write(0xff,0x82);
    lt9211c_write(0x26,0x20); //[7:6]desscpll reference select Xtal clock as reference
                                   //[4]1'b0: dessc-pll power down
    lt9211c_write(0x27,0x40); //prediv = 0;

    //dev_dbg(dev,"Mipi Rx PixClk: %ld",g_stRxVidTiming.ulPclk_Khz);
    if (g_stRxVidTiming.ulPclk_Khz >= 352000)
    {
        lt9211c_write(0x2f,0x04);
        ucdesscpll_pixck_div = 2;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 176000 && g_stRxVidTiming.ulPclk_Khz < 352000)
    {
        lt9211c_write(0x2f,0x04);
        ucdesscpll_pixck_div = 2;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 88000 && g_stRxVidTiming.ulPclk_Khz < 176000)
    {
        lt9211c_write(0x2f,0x05);
        ucdesscpll_pixck_div = 4;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 44000 && g_stRxVidTiming.ulPclk_Khz < 88000)
    {
        lt9211c_write(0x2f,0x06);
        ucdesscpll_pixck_div = 8;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 22000 && g_stRxVidTiming.ulPclk_Khz < 44000)
    {
        lt9211c_write(0x2f,0x07);
        ucdesscpll_pixck_div = 16;
    }
    else 
    {
        lt9211c_write(0x2f,0x07);
        ucdesscpll_pixck_div = 16;
    }

    g_stPcrPara.Pcr_M = (g_stRxVidTiming.ulPclk_Khz * ucdesscpll_pixck_div) / 25;
    g_stPcrPara.Pcr_K = g_stPcrPara.Pcr_M % 1000;
    g_stPcrPara.Pcr_M = g_stPcrPara.Pcr_M / 1000;
    
    g_stPcrPara.Pcr_UpLimit   = g_stPcrPara.Pcr_M + 1;
    g_stPcrPara.Pcr_DownLimit = g_stPcrPara.Pcr_M - 1;

    g_stPcrPara.Pcr_K <<= 14;

    DRV_DesscPll_SdmCal();
    lt9211c_write(0xff,0x81);     
    lt9211c_write(0x03,0xfe); //desscpll rst
    mdelay(1);
    lt9211c_write(0x03,0xff); //desscpll rst   
}

u8 Drv_MipiRx_PcrCali(void)
{    
    u8 ucRtn = LONTIUM_TRUE;
    u8 ucPcr_Cal_Cnt;

    lt9211c_write(0xff,0xd0);
    #if MIPIRX_INPUT_SEL == MIPI_DSI 
    lt9211c_write(0x0a,(lt9211c_read(0x0a) & 0x0f));
    lt9211c_write(0x1e,0x51); //0x51[7:4]pcr diff first step,[3:0]pcr diff second step
    lt9211c_write(0x23,0x80); //0x05 MIPIRX PCR capital data set for PCR second step
    lt9211c_write(0x24,0x70);
    lt9211c_write(0x25,0x80);
    lt9211c_write(0x2a,0x10); 
    lt9211c_write(0x21,0x4f);
    lt9211c_write(0x22,0xf0);
    
    lt9211c_write(0x38,0x04); //MIPIRX PCR de mode delay offset0
    lt9211c_write(0x39,0x08);
    lt9211c_write(0x3a,0x10);
    lt9211c_write(0x3b,0x20); //MIPIRX PCR de mode delay offset2

    lt9211c_write(0x3f,0x04); //PCR de mode step0 setting
    lt9211c_write(0x40,0x08);
    lt9211c_write(0x41,0x10);
    lt9211c_write(0x42,0x20);
    lt9211c_write(0x2b,0xA0);
//    lt9211c_write(0x4d,0x00); //0x00
    #elif MIPIRX_INPUT_SEL == MIPI_CSI
    lt9211c_write(0x0a,(lt9211c_read(0x0a) & 0x1f));
    lt9211c_write(0x1e,0x51); //0x51[7:4]pcr diff first step,[3:0]pcr diff second step
    lt9211c_write(0x23,0x10); //0x05 MIPIRX PCR capital data set for PCR second step
    lt9211c_write(0x24,0x70);
    lt9211c_write(0x25,0x80);
    lt9211c_write(0x2a,0x10); 
    lt9211c_write(0x21,0xc6); //[7]1'b1:CSI sel de sync mode;
    lt9211c_write(0x22,0x00);
    
    lt9211c_write(0x38,0x04); //MIPIRX PCR de mode delay offset0
    lt9211c_write(0x39,0x08);
    lt9211c_write(0x3a,0x10);
    lt9211c_write(0x3b,0x20); //MIPIRX PCR de mode delay offset2

    lt9211c_write(0x3f,0x04); //PCR de mode step0 setting
    lt9211c_write(0x40,0x08);
    lt9211c_write(0x41,0x10);
    lt9211c_write(0x42,0x20);
    lt9211c_write(0x2b,0xA0);
    #endif
    
    if (g_stRxVidTiming.ulPclk_Khz < 44000)
    {
        #if MIPIRX_CLK_BURST == ENABLED
//        lt9211c_write(0x0a,0x42);
        lt9211c_write(0x0c,0x40); //[7:0]rgd_vsync_dly(sram rd delay)
        lt9211c_write(0x1b,0x00); //pcr wr dly[15:0]
        lt9211c_write(0x1c,0x40); //pcr wr dly[7:0]
        #else
        lt9211c_write(0x0c,0x60); //[7:0]rgd_vsync_dly(sram rd delay)
        lt9211c_write(0x1b,0x00); //pcr wr dly[15:0]
        lt9211c_write(0x1c,0x60); //pcr wr dly[7:0]
        #endif
    }
    else 
    {
        lt9211c_write(0x0c,0x40); //[7:0]rgd_vsync_dly(sram rd delay)
        lt9211c_write(0x1b,0x00); //pcr wr dly[15:0]
        lt9211c_write(0x1c,0x40); //pcr wr dly[7:0]
    }
    
    lt9211c_write(0xff,0x81);     
    lt9211c_write(0x09,0xdb);
    lt9211c_write(0x09,0xdf); //pcr rst

    lt9211c_write(0xff,0xd0);
    lt9211c_write(0x08,0x80);
    lt9211c_write(0x08,0x00);
    mdelay(10);
    ucPcr_Cal_Cnt = 0;
    
    do
    {
        mdelay(100);
        ucPcr_Cal_Cnt++;
        pr_debug("PCR unstable M = 0x%02x",(lt9211c_read(0x94)&0x7F));
    }while((ucPcr_Cal_Cnt < 200) && ((lt9211c_read(0x87) & 0x18) != 0x18));

    if((lt9211c_read(0x87) & 0x18) != 0x18)
    {
        dev_dbg(dev,"LT9211C pcr unstable");
        ucRtn = FAIL;
    }
    
    return ucRtn;
}

u8 Drv_MipiRx_VidFmtUpdate(void)
{
    u8 ucRxFmt;
    
    ucRxFmt = g_stMipiRxVidTiming_Get.ucFmt;
    lt9211c_write(0xff,0xd0);
    g_stMipiRxVidTiming_Get.ucFmt = (lt9211c_read(0x84) & 0x0f);
    
    if (ucRxFmt != g_stMipiRxVidTiming_Get.ucFmt)
    {
        return LONTIUM_TRUE;
    }
    else
    {
        return LONTIUM_FALSE;
    }
}


void Drv_MipiRx_HsSettleSet(void)
{
    if((g_stMipiRxVidTiming_Get.ucLane0SetNum > 0x10) && (g_stMipiRxVidTiming_Get.ucLane0SetNum < 0x50))
    {
        pr_debug( "Set Mipi Rx Settle: 0x%02x", (g_stMipiRxVidTiming_Get.ucLane0SetNum - 5));
        lt9211c_write(0xff,0xd0);
        lt9211c_write(0x02,(g_stMipiRxVidTiming_Get.ucLane0SetNum - 5));
    }
    else
    {
        pr_debug( "Set Mipi Rx Settle: 0x0e"); //mipi rx cts test need settle 0x0e
        lt9211c_write(0xff,0xd0);
        lt9211c_write(0x02,0x08);
    }
}

void Drv_MipiRx_SotGet(void)
{
    lt9211c_write(0xff,0xd0);
    g_stMipiRxVidTiming_Get.ucLane0SetNum  = lt9211c_read(0x88);
    g_stMipiRxVidTiming_Get.ucLane0SotData = lt9211c_read(0x89);
    g_stMipiRxVidTiming_Get.ucLane1SetNum  = lt9211c_read(0x8a);
    g_stMipiRxVidTiming_Get.ucLane1SotData = lt9211c_read(0x8b);
    g_stMipiRxVidTiming_Get.ucLane2SetNum  = lt9211c_read(0x8c);
    g_stMipiRxVidTiming_Get.ucLane2SotData = lt9211c_read(0x8d);
    g_stMipiRxVidTiming_Get.ucLane3SetNum  = lt9211c_read(0x8e);
    g_stMipiRxVidTiming_Get.ucLane3SotData = lt9211c_read(0x8f);

    pr_debug("Sot Num = 0x%02x, 0x%02x, 0x%02x, 0x%02x", g_stMipiRxVidTiming_Get.ucLane0SetNum,g_stMipiRxVidTiming_Get.ucLane1SetNum,
                                                                    g_stMipiRxVidTiming_Get.ucLane2SetNum,g_stMipiRxVidTiming_Get.ucLane3SetNum);
    pr_debug("Sot Dta = 0x%02x, 0x%02x, 0x%02x, 0x%02x", g_stMipiRxVidTiming_Get.ucLane0SotData,g_stMipiRxVidTiming_Get.ucLane1SotData,
                                                                    g_stMipiRxVidTiming_Get.ucLane2SotData,g_stMipiRxVidTiming_Get.ucLane3SotData); 
}

void Drv_MipiRx_HactGet(void)
{
    lt9211c_write(0xff,0xd0);
    g_stMipiRxVidTiming_Get.usVact = (lt9211c_read(0x85) << 8) +lt9211c_read(0x86);
    g_stMipiRxVidTiming_Get.ucFmt  = (lt9211c_read(0x84) & 0x0f);
    g_stMipiRxVidTiming_Get.ucPa_Lpn = lt9211c_read(0x9c);
    g_stMipiRxVidTiming_Get.uswc = (lt9211c_read(0x82) << 8) + lt9211c_read(0x83); //

    switch (g_stMipiRxVidTiming_Get.ucFmt)
    {
        case 0x01: //DSI-YUV422-10bpc
        case 0x0e: //CSI-YUV422-10bpc
            g_stMipiRxVidTiming_Get.usHact = ((g_stMipiRxVidTiming_Get.uswc*10) / 25)/10; //wc = hact * 20bpp/8
        break;
        case 0x02: //DSI-YUV422-12bpc
            g_stMipiRxVidTiming_Get.usHact = g_stMipiRxVidTiming_Get.uswc / 3; //wc = hact * 24bpp/8
        break;
        case 0x03: //YUV422-8bpc
            g_stMipiRxVidTiming_Get.usHact = g_stMipiRxVidTiming_Get.uswc / 2; //wc = hact * 16bpp/8
        break; 
        case 0x04: //RGB10bpc
            g_stMipiRxVidTiming_Get.usHact = ((g_stMipiRxVidTiming_Get.uswc*100) / 375)/100; //wc = hact * 30bpp/8
        break;
        case 0x05: //RGB12bpc
            g_stMipiRxVidTiming_Get.usHact = ((g_stMipiRxVidTiming_Get.uswc*10) / 45)/10; //wc = hact * 36bpp/8
        break;
        case 0x06: //YUV420-8bpc
        case 0x0a: //RGB8bpc
            g_stMipiRxVidTiming_Get.usHact = g_stMipiRxVidTiming_Get.uswc / 3; //wc = hact * 24bpp/8
        break;
        case 0x07: //RGB565
            g_stMipiRxVidTiming_Get.usHact = g_stMipiRxVidTiming_Get.uswc / 2; //wc = hact * 16bpp/8
        break;
        case 0x08: //RGB6bpc
        case 0x09: //RGB6bpc_losely
            g_stMipiRxVidTiming_Get.usHact = ((g_stMipiRxVidTiming_Get.uswc*100) / 225)/100; //wc = hact * 18bpp/8
        break;
        case 0x0b: //RAW8
            g_stMipiRxVidTiming_Get.usHact = g_stMipiRxVidTiming_Get.uswc / 1; //wc = hact * 8bpp/8
        break;
        case 0x0c: //RAW10
            g_stMipiRxVidTiming_Get.usHact = ((g_stMipiRxVidTiming_Get.uswc*100) / 125)/10; //wc = hact * 10bpp/8
        break;
        case 0x0d: //RAW12
            g_stMipiRxVidTiming_Get.usHact = ((g_stMipiRxVidTiming_Get.uswc*10) / 15)/10; //wc = hact * 12bpp/8
        break;
        default: 
            g_stMipiRxVidTiming_Get.usHact = g_stMipiRxVidTiming_Get.uswc / 3; //wc = hact * 24bpp/8
        break;
    }
    
}

u8 Drv_MipiRx_VidTiming_Get(void)
{
    Drv_MipiRx_SotGet();
    Drv_MipiRx_HsSettleSet();
    Drv_MipiRx_HactGet();
    if((g_stMipiRxVidTiming_Get.usHact < 400) || (g_stMipiRxVidTiming_Get.usVact < 400))
    {
        //dev_err(dev,"RX No Video Get");
        return FAIL;
    }
    else
    {
        dev_dbg(dev,"hact = %d",g_stMipiRxVidTiming_Get.usHact);
        dev_dbg(dev,"vact = %d",g_stMipiRxVidTiming_Get.usVact);    
        dev_dbg(dev,"fmt = 0x%02x", g_stMipiRxVidTiming_Get.ucFmt);
        dev_dbg(dev,"pa_lpn = 0x%02x", g_stMipiRxVidTiming_Get.ucPa_Lpn);
        return SUCCESS;
    }
    return SUCCESS;
}
void Drv_MipiRx_VidTiming_Set(void)
{
    lt9211c_write(0xff,0xd0);
    lt9211c_write(0x0d,(u8)(g_stRxVidTiming.usVtotal >> 8));     //vtotal[15:8]
    lt9211c_write(0x0e,(u8)(g_stRxVidTiming.usVtotal));          //vtotal[7:0]
    lt9211c_write(0x0f,(u8)(g_stRxVidTiming.usVact >> 8));       //vactive[15:8]
    lt9211c_write(0x10,(u8)(g_stRxVidTiming.usVact));            //vactive[7:0]
    lt9211c_write(0x15,(u8)(g_stRxVidTiming.usVs));              //vs[7:0]
    lt9211c_write(0x17,(u8)(g_stRxVidTiming.usVfp >> 8));        //vfp[15:8]
    lt9211c_write(0x18,(u8)(g_stRxVidTiming.usVfp));             //vfp[7:0]    

    lt9211c_write(0x11,(u8)(g_stRxVidTiming.usHtotal >> 8));     //htotal[15:8]
    lt9211c_write(0x12,(u8)(g_stRxVidTiming.usHtotal));          //htotal[7:0]
    lt9211c_write(0x13,(u8)(g_stRxVidTiming.usHact >> 8));       //hactive[15:8]
    lt9211c_write(0x14,(u8)(g_stRxVidTiming.usHact));            //hactive[7:0]
    lt9211c_write(0x4c,(u8)(g_stRxVidTiming.usHs >> 8));         //hs[15:8]
    lt9211c_write(0x16,(u8)(g_stRxVidTiming.usHs));              //hs[7:0]
    lt9211c_write(0x19,(u8)(g_stRxVidTiming.usHfp >> 8));        //hfp[15:8]
    lt9211c_write(0x1a,(u8)(g_stRxVidTiming.usHfp));             //hfp[7:0]
}

u8 Drv_MipiRx_VidTiming_Sel(StructChipRxVidTiming *ptn_timing)
{   
    u8 rtn = LONTIUM_FALSE;
    if ((g_stMipiRxVidTiming_Get.usHact == ptn_timing->usHact ) && ( g_stMipiRxVidTiming_Get.usVact == ptn_timing->usVact )){
        g_stMipiRxVidTiming_Get.ucFrameRate = Drv_VidChk_FrmRt_Get();
        dev_dbg(dev,"FrameRate = %d", g_stMipiRxVidTiming_Get.ucFrameRate);

        if ((g_stMipiRxVidTiming_Get.ucFrameRate > (ptn_timing->ucFrameRate - 3)) && 
               (g_stMipiRxVidTiming_Get.ucFrameRate < (ptn_timing->ucFrameRate + 3))){
                    g_stRxVidTiming.usVtotal = ptn_timing->usVtotal;
                    g_stRxVidTiming.usVact   = ptn_timing->usVact;
                    g_stRxVidTiming.usVs     = ptn_timing->usVs;
                    g_stRxVidTiming.usVfp    = ptn_timing->usVfp;
                    g_stRxVidTiming.usVbp    = ptn_timing->usVbp;

                    g_stRxVidTiming.usHtotal = ptn_timing->usHtotal;
                    g_stRxVidTiming.usHact   = ptn_timing->usHact;
                    g_stRxVidTiming.usHs     = ptn_timing->usHs;
                    g_stRxVidTiming.usHfp    = ptn_timing->usHfp;
                    g_stRxVidTiming.usHbp    = ptn_timing->usHbp;

                    g_stRxVidTiming.ulPclk_Khz = (u32)((u32)(ptn_timing->usHtotal) * (ptn_timing->usVtotal) * (ptn_timing->ucFrameRate) / 1000);
                    Drv_MipiRx_VidTiming_Set();
                    dev_dbg(dev,"Video Timing Set %dx%d_%d",g_stRxVidTiming.usHact,g_stRxVidTiming.usVact,g_stMipiRxVidTiming_Get.ucFrameRate);
                    rtn = LONTIUM_TRUE;
               }
 
        
        
    }
    return rtn;
    
}



u8 Drv_MipiRx_VidFmt_Get(u8 VidFmt)
{
    u8 ucRxVidFmt = 0;
    
    switch (VidFmt)
    {
        case 0x01: //DSI-YUV422-10bpc
            ucRxVidFmt = YUV422_10bit;
        break;
        case 0x02: //DSI-YUV422-12bpc
            ucRxVidFmt = YUV422_12bit;
        break;
        case 0x03: //YUV422-8bpc
            ucRxVidFmt = YUV422_8bit;
        break;
        case 0x04: //RGB30bpp
            ucRxVidFmt = RGB_10Bit;
        break;
        case 0x05: //RGB36bpp
            ucRxVidFmt = RGB_12Bit;
        break;
        case 0x06: //YUV420-8bpc
            ucRxVidFmt = YUV420_8bit;
        break;
        case 0x07: //RGB565
        break;
        case 0x08: //RGB666
            ucRxVidFmt = RGB_6Bit;
        break;
        case 0x09: //DSI-RGB6L
        break;
        case 0x0a: //RGB888
            ucRxVidFmt = RGB_8Bit;
        break;
        case 0x0b: //RAW8
        break;
        case 0x0c: //RAW10
        break;
        case 0x0d: //RAW12
        break;
        case 0x0e: //CSI-YUV422-10
            ucRxVidFmt = YUV422_10bit;
        break;
        default:
            ucRxVidFmt = RGB_8Bit;
        break;    
    }

    dev_dbg(dev,"MipiRx Input Format: %s",g_szStrRxFormat[VidFmt]);
    return ucRxVidFmt;
}

void Drv_MipiRx_InputSel(void)
{
    lt9211c_write(0xff,0xd0);
    #if (MIPIRX_INPUT_SEL == MIPI_CSI)
    lt9211c_write(0x04,0x10); //[4]1: CSI enable
    lt9211c_write(0x21,0xc6); //[7](dsi: hsync_level(for pcr adj) = hsync_level; csi:hsync_level(for pcr adj) = de_level)
    dev_dbg(dev,"Mipi CSI Input");
    #else 
    lt9211c_write(0x04,0x00); //[4]0: DSI enable
    lt9211c_write(0x21,0x46); //[7](dsi: hsync_level(for pcr adj) = hsync_level; csi:hsync_level(for pcr adj) = de_level)
    dev_dbg(dev,"Mipi DSI Input");
    #endif
}

void Drv_MipiRx_LaneSet(void)
{
    lt9211c_write(0xff,0x85);
    lt9211c_write(0x3f,0x08); //MLRX HS/LP control conmand enable
    lt9211c_write(0x40,0x04); //[2:0]pa_ch0_src_sel ch4 data
    lt9211c_write(0x41,0x03); //[2:0]pa_ch1_src_sel ch3 data
    lt9211c_write(0x42,0x02); //[2:0]pa_ch2_src_sel ch2 data
    lt9211c_write(0x43,0x01); //[2:0]pa_ch3_src_sel ch1 data

    lt9211c_write(0x45,0x04); //[2:0]pb_ch0_src_sel ch9 data
    lt9211c_write(0x46,0x03); //[2:0]pb_ch1_src_sel ch8 data
    lt9211c_write(0x47,0x02); //[2:0]pb_ch2_src_sel ch7 data
    lt9211c_write(0x48,0x01); //[2:0]pb_ch3_src_sel ch6 data
    
    #if MIPIRX_PORT_SWAP == ENABLED
    lt9211c_write(0x44,0x40); //[6]mlrx port A output select port B;[2:0]pa_ch4_src_sel ch0 data
    lt9211c_write(0x49,0x00); //[6]mlrx port B output select port B;[2:0]pb_ch4_src_sel ch5 data
    #else
    lt9211c_write(0x44,0x00); //[6]mlrx port A output select port A;[2:0]pa_ch4_src_sel ch0 data
    lt9211c_write(0x49,0x00); //[6]mlrx port B output select port A;[2:0]pb_ch4_src_sel ch5 data
    #endif

    #if MIPIRX_PORT_SEL == PORTB
    lt9211c_write(0x44,0x40); //[6]mlrx port A output select port B;[2:0]pa_ch4_src_sel ch0 data
    lt9211c_write(0x49,0x00); //[6]mlrx port B output select port B;[2:0]pb_ch4_src_sel ch5 data
    #endif

}

void Drv_MipiRxClk_Sel(void)
{
    /* CLK sel */
    lt9211c_write(0xff,0x85);
    lt9211c_write(0xe9,0x88); //sys clk sel from XTAL
    
    lt9211c_write(0xff,0x81);
    lt9211c_write(0x80,0x51); //[7:6]rx sram rd clk src sel ad dessc pcr clk
                                   //[5:4]rx sram wr clk src sel mlrx bytr clk
                                   //[1:0]video check clk sel from desscpll pix clk
    #if MIPIRX_PORT_SEL == PORTA
    lt9211c_write(0x81,0x10); //[5]0: mlrx byte clock select from ad_mlrxa_byte_clk
                                   //[4]1: rx output pixel clock select from ad_desscpll_pix_clk
    #elif MIPIRX_PORT_SEL == PORTB
    lt9211c_write(0x81,0x30); //[5]1: mlrx byte clock select from ad_mlrxb_byte_clk
                                   //[4]1: rx output pixel clock select from ad_desscpll_pix_clk
    #endif
    lt9211c_write(0xff,0x86);
    lt9211c_write(0x32,0x03); //video check frame cnt set: 3 frame
}

void Drv_MipiRx_PhyPowerOn(void)
{
    lt9211c_write(0xff,0xd0);
    lt9211c_write(0x00,(lt9211c_read(0x00) | MIPIRX_LANE_NUM));    // 0: 4 Lane / 1: 1 Lane / 2 : 2 Lane / 3: 3 Lane

    lt9211c_write(0xff,0x82);
    lt9211c_write(0x01,0x11); //MIPI RX portA & B disable

    #if MIPIRX_PORT_SEL == PORTA
    lt9211c_write(0x01,0x91); //MIPI RX portA enable
    lt9211c_write(0x02,0x00); //[5][1]:0 mipi mode, no swap
    lt9211c_write(0x03,0xcc); //port A & B eq current reference 
    lt9211c_write(0x09,0x21); //[3]0: select link clk from port-A, [1]0: mlrx_clk2pll disable
    lt9211c_write(0x13,0x0c); //MIPI port A clk lane rterm & high speed en
    
    #if MIPIRX_CLK_BURST == ENABLED
    lt9211c_write(0x13,0x00); //MIPI port A clk lane rterm & high speed en
    #endif
    dev_dbg(dev,"MIPI Input PortA");
    #elif MIPIRX_PORT_SEL == PORTB
    lt9211c_write(0x01,0x19); //MIPI RX portB enable
    lt9211c_write(0x02,0x00); //[5][1]:0 mipi mode, no swap
    lt9211c_write(0x03,0xcc); //port A & B eq current reference 
    lt9211c_write(0x09,0x29); //[3]1: select link clk from port-B, [1]0: mlrx_clk2pll disable
    lt9211c_write(0x14,0x03); //Port-B clk lane software enable
    
    #if MIPIRX_CLK_BURST == ENABLED
    lt9211c_write(0x14,0x00); //MIPI port A clk lane rterm & high speed en
    #endif
    dev_dbg(dev,"MIPI Input PortB");
    #else
    lt9211c_write(0x01,0x99); //MIPI RX portB enable
    lt9211c_write(0x02,0x00); //[5][1]:0 mipi mode, no swap
    lt9211c_write(0x03,0xcc); //port A & B eq current reference 
    lt9211c_write(0x09,0x29); //[3]1: select link clk from port-B, [1]0: mlrx_clk2pll disable
    lt9211c_write(0x13,0x0c); //MIPI port A clk lane rterm & high speed en
    lt9211c_write(0x14,0x03); //Port-B clk lane software enable
    
    #if MIPIRX_CLK_BURST == ENABLED
    lt9211c_write(0x13,0x00); //MIPI port A clk lane rterm use hardware mode
    lt9211c_write(0x14,0x00); //Port-B clk lane use hardware mode
    #endif
    #endif

    lt9211c_write(0xff,0xd0);
    lt9211c_write(0x01,0x00); //mipi rx data lane term enable time: 39ns;
    lt9211c_write(0x02,0x0e); //mipi rx hs settle time defult set: 0x05;
    lt9211c_write(0x05,0x00); //mipi rx lk lane term enable time: 39ns;
    lt9211c_write(0x0a,0x52);
    lt9211c_write(0x0b,0x30);
    
    lt9211c_write(0xff,0x81);
    lt9211c_write(0x09,0xde); //mipi rx dphy reset
    lt9211c_write(0x09,0xdf); //mipi rx dphy release
}

void Drv_LvdsTxSw_Rst(void)
{
    lt9211c_write(0xff,0x81);
    lt9211c_write(0x08,0x6f); //LVDS TX SW reset
    mdelay(2);
    lt9211c_write(0x08,0x7f);
    dev_dbg(dev,"LVDS Tx Video Out");
}

void Drv_LVDSTxPhy_PowerOff(void)
{
    lt9211c_write(0xff,0x82);
    lt9211c_write(0x36,0x00); //lvds enable
    lt9211c_write(0x37,0x00);
}

void Drv_LvdsTxPhy_Poweron(LvdsOutParameter *lvds_parameter)
{

    if(strstr(lvds_parameter->lvdstx_port_sel, "porta")){
        lt9211c_write(0xff,0x82);
        lt9211c_write(0x36,0x01); //lvds enable
        lt9211c_write(0x37,0x40);
        dev_dbg(dev,"LVDS Output PortA");
        #if LVDSTX_LANENUM == FIVE_LANE
        lt9211c_write(0x36,0x03);
        #endif 
    }else if(strstr(lvds_parameter->lvdstx_port_sel, "porta")){
        lt9211c_write(0xff,0x82);
        lt9211c_write(0x36,0x02); //lvds enable
        lt9211c_write(0x37,0x04);
        dev_dbg(dev,"LVDS Output PortB");
    }else if(strstr(lvds_parameter->lvdstx_port_sel, "dou_port")){
        lt9211c_write(0xff,0x82);
        lt9211c_write(0x36,0x03); //lvds enable
        lt9211c_write(0x37,0x44); //port rterm enable
        dev_dbg(dev,"LVDS Output PortA&B");
    }   
    lt9211c_write(0x38,0x14);
    lt9211c_write(0x39,0x31);
    lt9211c_write(0x3a,0xc8);
    lt9211c_write(0x3b,0x00);
    lt9211c_write(0x3c,0x0f);

    lt9211c_write(0x46,0x40);
    lt9211c_write(0x47,0x40);
    lt9211c_write(0x48,0x40);
    lt9211c_write(0x49,0x40);
    lt9211c_write(0x4a,0x40);
    lt9211c_write(0x4b,0x40);
    lt9211c_write(0x4c,0x40);
    lt9211c_write(0x4d,0x40);
    lt9211c_write(0x4e,0x40);
    lt9211c_write(0x4f,0x40);
    lt9211c_write(0x50,0x40);
    lt9211c_write(0x51,0x40);

    lt9211c_write(0xff,0x81);
    lt9211c_write(0x03,0xbf); //mltx reset
    lt9211c_write(0x03,0xff); //mltx release
}

void Drv_LvdsTxPll_RefPixClk_Set(void)
{
    lt9211c_write(0xff,0x82);
    lt9211c_write(0x30,0x00); //[7]0:txpll normal work; txpll ref clk sel pix clk
}

void Drv_LvdsTxPll_Config(LvdsOutParameter *lvds_parameter)
{
    u8 ucPreDiv = 0;
    u8 ucSericlkDiv = 0;
    u8 ucDivSet = 0;
    float ucPixClkDiv = 0;
    u32 ulLvdsTXPhyClk = 0;

    /* txphyclk = vco clk * ucSericlkDiv */
    if(strstr(lvds_parameter->lvdstx_port_sel, "dou_port")){
        ulLvdsTXPhyClk = (u32)(g_stRxVidTiming.ulPclk_Khz * 7 / 2); //2 port: byte clk = pix clk / 2;
        lt9211c_write(0xff,0x85);
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | BIT0_1)); //htotal -> 2n
        
        #if ((LVDSTX_COLORSPACE == YUV422) && (LVDSTX_COLORDEPTH == DEPTH_8BIT) && (LVDSTX_LANENUM == FIVE_LANE))
        ulLvdsTXPhyClk = (u32)(g_stRxVidTiming.ulPclk_Khz * 7 / 4); //2 port YUV422 8bit 5lane: byte clk = pix clk / 4;
        lt9211c_write(0xff,0x85);
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | BIT1_1)); //htotal -> 4n
        #endif
    }else{
        ulLvdsTXPhyClk = (u32)(g_stRxVidTiming.ulPclk_Khz * 7); //1 port: byte clk = pix clk;
        
        #if ((LVDSTX_COLORSPACE == YUV422) && (LVDSTX_COLORDEPTH == DEPTH_8BIT) && (LVDSTX_LANENUM == FIVE_LANE))
        ulLvdsTXPhyClk = (u32)(g_stRxVidTiming.ulPclk_Khz * 7 / 2); //1 port YUV422 8bit 5lane: byte clk = pix clk / 2;
        lt9211c_write(0xff,0x85);
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | BIT0_1)); //htotal -> 2n
        #endif
    }
    
    /*txpll prediv sel*/
    lt9211c_write(0xff,0x82);
    if (g_stRxVidTiming.ulPclk_Khz < 20000)
    {
        lt9211c_write(0x31,0x28); //[2:0]3'b000: pre div set div1
        ucPreDiv = 1;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 20000 && g_stRxVidTiming.ulPclk_Khz < 40000)
    {
        lt9211c_write(0x31,0x28); //[2:0]3'b000: pre div set div1
        ucPreDiv = 1;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 40000 && g_stRxVidTiming.ulPclk_Khz < 80000)
    {
        lt9211c_write(0x31,0x29); //[2:0]3'b001: pre div set div2
        ucPreDiv = 2;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 80000 && g_stRxVidTiming.ulPclk_Khz < 160000)
    {
        lt9211c_write(0x31,0x2a); //[2:0]3'b010: pre div set div4
        ucPreDiv = 4;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 160000 && g_stRxVidTiming.ulPclk_Khz < 320000)
    {
        lt9211c_write(0x31,0x2b); //[2:0]3'b011: pre div set div8
        ucPreDiv = 8;
//        ulLvdsTXPhyClk = ulDessc_Pix_Clk * 3.5;
    }
    else if (g_stRxVidTiming.ulPclk_Khz >= 320000)
    {
        lt9211c_write(0x31,0x2f); //[2:0]3'b111: pre div set div16
        ucPreDiv = 16;
//        ulLvdsTXPhyClk = ulDessc_Pix_Clk * 3.5;
    }

    /*txpll serick_divsel*/
    lt9211c_write(0xff,0x82);
    if (ulLvdsTXPhyClk >= 640000 )//640M~1.28G
    {
        lt9211c_write(0x32,0x42);
        ucSericlkDiv = 1; //sericlk div1 [6:4]:0x40
    }
    else if (ulLvdsTXPhyClk >= 320000 && ulLvdsTXPhyClk < 640000)
    {
        lt9211c_write(0x32,0x02);
        ucSericlkDiv = 2; //sericlk div2 [6:4]:0x00
    }
    else if (ulLvdsTXPhyClk >= 160000 && ulLvdsTXPhyClk < 320000)
    {
        lt9211c_write(0x32,0x12);
        ucSericlkDiv = 4; //sericlk div4 [6:4]:0x10
    }
    else if (ulLvdsTXPhyClk >= 80000 && ulLvdsTXPhyClk < 160000)
    {
        lt9211c_write(0x32,0x22);
        ucSericlkDiv = 8; //sericlk div8 [6:4]:0x20
    }
    else //40M~80M
    {
        lt9211c_write(0x32,0x32);
        ucSericlkDiv = 16; //sericlk div16 [6:4]:0x30
    }

    /* txpll_pix_mux_sel & txpll_pixdiv_sel*/
    lt9211c_write(0xff,0x82);
    if (g_stRxVidTiming.ulPclk_Khz > 150000)
    {
        lt9211c_write(0x33,0x04); //pixclk > 150000, pixclk mux sel (vco clk / 3.5)
        ucPixClkDiv = 3.5;
    }
    else
    {
        ucPixClkDiv = (u8)((ulLvdsTXPhyClk * ucSericlkDiv) / (g_stRxVidTiming.ulPclk_Khz * 7));

        if (ucPixClkDiv <= 1)
        {
            lt9211c_write(0x33,0x00); //pixclk div sel /7
        }
        else if ((ucPixClkDiv > 1) && (ucPixClkDiv <= 2))
        {
            lt9211c_write(0x33,0x01); //pixclk div sel /14
        }
        else if ((ucPixClkDiv > 2) && (ucPixClkDiv <= 4))
        {
            lt9211c_write(0x33,0x02); //pixclk div sel /28
        }
        else if ((ucPixClkDiv > 4) && (ucPixClkDiv <= 8))
        {
            lt9211c_write(0x33,0x03); //pixclk div sel /56
        }
        else
        {
            lt9211c_write(0x33,0x03); //pixclk div sel /56
        }
    }
    
    ucDivSet = (u8)((ulLvdsTXPhyClk * ucSericlkDiv) / (g_stRxVidTiming.ulPclk_Khz / ucPreDiv));
    
    lt9211c_write(0x34,0x01); //txpll div set software output enable
    lt9211c_write(0x35,ucDivSet);
    /*pr_debug("ulPclk_Khz: %ld, ucPreDiv: %d, ucSericlkDiv: %d, ucPixClkDiv: %.1f, ucDivSet: %d",
                    g_stRxVidTiming.ulPclk_Khz, ucPreDiv, ucSericlkDiv, ucPixClkDiv, ucDivSet);*/
    
    #if LVDS_SSC_SEL != NO_SSC

    lt9211c_write(0xff,0x82);
    lt9211c_write(0x34,0x00); //hardware mode
    
    lt9211c_write(0xff,0x85);
    lt9211c_write(0x6e,0x10); //sram select
    
    lt9211c_write(0xff,0x81);
    lt9211c_write(0x81,0x15); //clk select
    
    lt9211c_write(0xff,0x87);

    lt9211c_write(0x1e,0x00); //modulation disable
    
    lt9211c_write(0x17,0x02); //2 order

    
    lt9211c_write(0x18,0x04);
    lt9211c_write(0x19,0xd4); //ssc_prd

    #if LVDS_SSC_SEL == SSC_1920x1080_30k5
        lt9211c_write(0x1a,0x00);
        lt9211c_write(0x1b,0x12); //delta1
        lt9211c_write(0x1c,0x00);
        lt9211c_write(0x1d,0x24); //delta
        
        lt9211c_write(0x1f,0x1c); //M
        lt9211c_write(0x20,0x00);
        lt9211c_write(0x21,0x00);
        dev_dbg(dev,"LVDS Output SSC 1920x1080 30k5%");
    #elif LVDS_SSC_SEL == SSC_3840x2160_30k5
        lt9211c_write(0x1a,0x00);
        lt9211c_write(0x1b,0x12); //delta1
        lt9211c_write(0x1c,0x00);
        lt9211c_write(0x1d,0x24); //delta
        
        lt9211c_write(0x1f,0x1c); //M
        lt9211c_write(0x20,0x00);
        lt9211c_write(0x21,0x00);
        dev_dbg(dev,"LVDS Output SSC 3840x2160 30k5%");
    #endif

    lt9211c_write(0x1e,0x02); //modulation enable
    
    lt9211c_write(0xff,0x81);
    lt9211c_write(0x0c,0xfe); //txpll reset
    lt9211c_write(0x0c,0xff); //txpll release
    
    #endif
}

u8 Drv_LvdsTxPll_Cali(void)
{
    u8 ucloopx = 0;
    u8 ucRtn = FAIL;

    lt9211c_write(0xff,0x81);
    lt9211c_write(0x0c,0xfe); //txpll reset
    mdelay(1);
    lt9211c_write(0x0c,0xff); //txpll release

    do
    {
        lt9211c_write(0xff,0x87);
        lt9211c_write(0x0f,0x00);
        lt9211c_write(0x0f,0x01);
        mdelay(20);

        ucloopx++;
    }while((ucloopx < 3) && ((lt9211c_read(0x39) & 0x01) != 0x01));

    if(lt9211c_read(0x39) & 0x04)
    {
        ucRtn = SUCCESS;
        dev_dbg(dev,"Tx Pll Lock");
    }
    else
    {
        dev_err(dev,"Tx Pll Unlocked");
    }

    return ucRtn;
}

void Drv_LvdsTx_VidTiming_Set(void)
{
    u16 vss,eav,sav;
    mdelay(100);
    lt9211c_write(0xff,0x85); 
    
    vss = g_stVidChk.usVs + g_stVidChk.usVbp;
    eav = g_stVidChk.usHs + g_stVidChk.usHbp + g_stVidChk.usHact + 4;
    sav = g_stVidChk.usHs + g_stVidChk.usHbp;
    
    
    lt9211c_write(0x5f,0x00);    
    lt9211c_write(0x60,0x00);  
    lt9211c_write(0x62,(u8)(g_stVidChk.usVact>>8));         //vact[15:8]
    lt9211c_write(0x61,(u8)(g_stVidChk.usVact));            //vact[7:0]
    lt9211c_write(0x63,(u8)(vss));                           //vss[7:0]
    lt9211c_write(0x65,(u8)(eav>>8));                        //eav[15:8]
    lt9211c_write(0x64,(u8)(eav));                           //eav[7:0]
    lt9211c_write(0x67,(u8)(sav>>8));                        //sav[15:8]
    lt9211c_write(0x66,(u8)(sav));                           //sav[7:0] 
    
}


void Drv_LvdsTxPort_Set(LvdsOutParameter *lvds_parameter)
{
    lt9211c_write(0xff,0x85);
    if(strstr(lvds_parameter->lvdstx_port_sel, "porta") || strstr(lvds_parameter->lvdstx_port_sel, "portb"))
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0x80)); //[7]lvds function enable //[4]0:output 1port; [4]1:output 2port;
    //only portb output must use port copy from porta, so lvds digtial output port sel 2ports.
    else if(strstr(lvds_parameter->lvdstx_port_sel, "dou_port"))
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0x90)); //[7]lvds function enable //[4]0:output 1port; [4]1:output 2port;
}

void Drv_LvdsTxVidFmt_Set(void)
{
    lt9211c_write(0xff,0x85); 
#if (LVDSTX_MODE == SYNC_MODE)
    lt9211c_write(0X6e,(lt9211c_read(0x6e) & BIT3_0));
#elif (LVDSTX_MODE == DE_MODE)
    lt9211c_write(0X6e,(lt9211c_read(0x6e) | BIT3_1)); //[3]lvdstx de mode
#endif

#if (LVDSTX_DATAFORMAT == JEIDA)
    lt9211c_write(0x6f,(lt9211c_read(0x6f) | BIT6_1)); //[6]1:JEIDA MODE
    dev_dbg(dev,"Data Format: JEIDA");
#elif (LVDSTX_DATAFORMAT == VESA)
    lt9211c_write(0x6f,(lt9211c_read(0x6f) & BIT6_0)); //[6]0:VESA MODE;
    dev_dbg(dev,"Data Format: VESA");
#endif

#if (LVDSTX_COLORSPACE == RGB)
    dev_dbg(dev,"ColorSpace: RGB");
    #if (LVDSTX_COLORDEPTH == DEPTH_6BIT)
        dev_dbg(dev,"ColorDepth: 6Bit");
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0x40)); //RGB666 [6]RGB666 output must select jeida mode
        lt9211c_write(0x6f,(lt9211c_read(0x6f) & 0xf3));
    #elif (LVDSTX_COLORDEPTH == DEPTH_8BIT)
        dev_dbg(dev,"ColorDepth: 8Bit");
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0X04));
    #elif (LVDSTX_COLORDEPTH == DEPTH_10BIT)
        dev_dbg(dev,"ColorDepth: 10Bit");
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0X0c));
    #endif

#elif (LVDSTX_COLORSPACE == YUV422)
    dev_dbg(dev,"ColorSpace: YUV422");    
    lt9211c_write(0xff,0x85);
    #if (LVDSTX_COLORDEPTH == DEPTH_8BIT)
        dev_dbg(dev,"ColorDepth: 8Bit");
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0X04));
        #if (LVDSTX_LANENUM == FIVE_LANE)

                dev_dbg(dev,"LvdsLaneNum: 5Lane");
                lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0X40)); //YUV422-8bpc-5lane mode output must sel jeida mode
                lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0X28));  //YUV422-8bpc-5lane mode set
        #else
                dev_dbg(dev,"LvdsLaneNum: 4Lane");
                lt9211c_write(0x6f,(lt9211c_read(0x6f) & 0Xbf)); //YUV422-8bpc-4lane mode output must sel vesa mode
        #endif
    #endif
#endif

#if (LVDSTX_SYNC_INTER_MODE == ENABLED)
    Drv_LvdsTx_VidTiming_Set();    
    dev_dbg(dev,"Lvds Sync Code Mode: Internal"); //internal sync code mode
    lt9211c_write(0x68,(lt9211c_read(0x68) | 0X01));
    #if (LVDSTX_VIDEO_FORMAT == I_FORMAT)
        dev_dbg(dev,"Lvds Video Format: interlaced"); //internal sync code mode
        lt9211c_write(0x68,(lt9211c_read(0x68) | 0X02));
    #endif
    #if (LVDSTX_SYNC_CODE_SEND == REPECTIVE)
        dev_dbg(dev,"Lvds Sync Code Send: respectively."); //sync code send method sel respectively
        lt9211c_write(0x68,(lt9211c_read(0x68) | 0X04));
    #endif

#else
    lt9211c_write(0x68,0x00);
#endif 

}

void Drv_LvdsTxLaneNum_Set(void)
{
    lt9211c_write(0xff,0x85);
    lt9211c_write(0x4a,0x01); //[0]hl_swap_en; [7:6]tx_pt0_src_sel: 0-pta;1-ptb
    lt9211c_write(0x4b,0x00);
    lt9211c_write(0x4c,0x10);
    lt9211c_write(0x4d,0x20);
    lt9211c_write(0x4e,0x50);
    lt9211c_write(0x4f,0x30);

#if (LVDSTX_LANENUM  == FOUR_LANE)
        lt9211c_write(0x50,0x46); //[7:6]tx_pt1_src_sel: 0-pta;1-ptb
        lt9211c_write(0x51,0x10);
        lt9211c_write(0x52,0x20);
        lt9211c_write(0x53,0x50);
        lt9211c_write(0x54,0x30);
        lt9211c_write(0x55,0x00); //[7:4]pt1_tx4_src_sel
        lt9211c_write(0x56,0x20); //[3:0]pt1_tx5_src_sel
                                       //[6:5]rgd_mltx_src_sel: 0-mipitx;1-lvdstx
#elif (LVDSTX_LANENUM == FIVE_LANE)
        lt9211c_write(0x50,0x44); //[7:6]tx_pt1_src_sel: 0-pta;1-ptb
        lt9211c_write(0x51,0x00);
        lt9211c_write(0x52,0x10);
        lt9211c_write(0x53,0x20);
        lt9211c_write(0x54,0x50);
        lt9211c_write(0x55,0x30); //[7:4]pt1_tx4_src_sel
        lt9211c_write(0x56,0x24); //[3:0]pt1_tx5_src_sel
                                       //[6:5]rgd_mltx_src_sel: 0-mipitx;1-lvdstx

#if 0
        //swap when 9211 lvdstx_to_lvdsrx 2port 5lane link
        lt9211c_write(0x54,0x30);
        lt9211c_write(0x55,0x40);
#endif

#endif

}

void Drv_LvdsTxPort_Swap(LvdsOutParameter *lvds_parameter)
{

    if(strstr(lvds_parameter->lvdstx_port_swap, "enabled") || strstr(lvds_parameter->lvdstx_port_sel, "portb")){
        lt9211c_write(0xff,0x85);
        lt9211c_write(0x4a,0x41);
        lt9211c_write(0x50,(lt9211c_read(0x50) & BIT6_0));
    }else{
        lt9211c_write(0xff,0x85);
        lt9211c_write(0x4a,0x01);
        lt9211c_write(0x50,(lt9211c_read(0x50) | BIT6_1));

    }
}

void Drv_LvdsTxPort_Copy(LvdsOutParameter *lvds_parameter)
{

    #if (LVDSTX_PORT_COPY == ENABLED)
    lt9211c_write(0xff,0x82);
    lt9211c_write(0x36,(lt9211c_read(0x36) | 0x03)); //port swap enable when porta & portb enable

    lt9211c_write(0xff,0x85);

    if(strstr(lvds_parameter->lvdstx_port_sel, "porta")){
        lt9211c_write(0x4a,(lt9211c_read(0x4a) & 0xbf));
        lt9211c_write(0x50,(lt9211c_read(0x50) & 0xbf));
    }else if(strstr(lvds_parameter->lvdstx_port_sel, "portb")){
        lt9211c_write(0x6f,(lt9211c_read(0x6f) | 0x10)); //[7]lvds function enable //[4]0:output 1port; [4]1:output 2port;
        lt9211c_write(0x4a,(lt9211c_read(0x4a) | 0x40));
        lt9211c_write(0x50,(lt9211c_read(0x50) | 0x40));
        dev_dbg(dev,"Port B Copy");
    }
    
    #endif

}

void Drv_LvdsTxCsc_Set(void)
{
    #if LVDSTX_COLORSPACE == RGB
        dev_dbg(dev,"Csc Set:    RGB");
    #elif LVDSTX_COLORSPACE == YUV422
        lt9211c_write(0xff,0x86);
        if((lt9211c_read(0x87) & 0x10) == 0)
        {
            lt9211c_write(0x85,lt9211c_read(0x85) | 0x10);
        }
        else
        {
            lt9211c_write(0x87,lt9211c_read(0x87) & 0xef);
        }
        if((lt9211c_read(0x86) & 0x04) == 0)
        {
            lt9211c_write(0x86,lt9211c_read(0x86) | 0x40);
        }
        else
        {
            lt9211c_write(0x86,lt9211c_read(0x86) & 0xfb);
        }
        dev_dbg(dev,"Csc Set:    YUV422");
    #endif
}

void Mod_LvdsTxPll_RefPixClk_Get(void)
{

    /*mipi to lvds use desscpll pix clk as txpll ref clk*/
    g_stRxVidTiming.ulPclk_Khz = Drv_System_FmClkGet(AD_DESSCPLL_PIX_CLK);

}

void Mod_LvdsTxDig_Set(LvdsOutParameter *lvds_parameter)
{
    Drv_LvdsTxPort_Set(lvds_parameter);
    Drv_LvdsTxVidFmt_Set();
    Drv_LvdsTxLaneNum_Set();
    Drv_LvdsTxPort_Swap(lvds_parameter);
    Drv_LvdsTxPort_Copy(lvds_parameter);
    Drv_LvdsTxSw_Rst();
    Drv_LvdsTxCsc_Set();
}

void Mod_LvdsTx_StateJudge(void)
{
    //monitor upstream video stable.
    if(g_stChipTx.ucTxState > STATE_CHIPTX_UPSTREAM_VIDEO_READY)
    {
        if(g_stChipTx.b1UpstreamVideoReady == LONTIUM_FALSE)
        {
            Mod_SystemTx_SetState(STATE_CHIPTX_UPSTREAM_VIDEO_READY);
        }
    }
}

void Mod_LvdsTx_StateHandler(LvdsOutParameter *lvds_parameter)
{
    switch (g_stChipTx.ucTxState)
    {
        case STATE_CHIPTX_POWER_ON:
            Mod_SystemTx_SetState(STATE_CHIPTX_UPSTREAM_VIDEO_READY);
        break;

        case STATE_CHIPTX_UPSTREAM_VIDEO_READY:
            if(g_stChipTx.b1TxStateChanged == LONTIUM_TRUE)
            {
                Drv_LVDSTxPhy_PowerOff();
                g_stChipTx.b1TxStateChanged = LONTIUM_FALSE;
            }
        
            if(g_stChipTx.b1UpstreamVideoReady == LONTIUM_TRUE) 
            {
                Drv_SystemTxSram_Sel(LVDSTX);
                Drv_LvdsTxPhy_Poweron(lvds_parameter);
                Mod_SystemTx_SetState(STATE_CHIPTX_CONFIG_VIDEO);
            }
        break;

        case STATE_CHIPTX_CONFIG_VIDEO:
            Mod_LvdsTxPll_RefPixClk_Get();
            Drv_LvdsTxPll_RefPixClk_Set();
            Drv_LvdsTxPll_Config(lvds_parameter);
            if(Drv_LvdsTxPll_Cali() == SUCCESS)
            {
                Mod_SystemTx_SetState(STATE_CHIPTX_VIDEO_OUT);
            }
            else
            {
                dev_err(dev,"fail to Drv_LvdsTxPll_Cali ");
                Mod_SystemTx_SetState(STATE_CHIPTX_CONFIG_VIDEO);
            }
        break;

        case STATE_CHIPTX_VIDEO_OUT:
            Drv_VidChkAll_Get(&g_stVidChk);
            Mod_LvdsTxDig_Set(lvds_parameter);
            Mod_SystemTx_SetState(STATE_CHIPTX_PLAY_BACK);
        break;

        case STATE_CHIPTX_PLAY_BACK:
        break;
        
    }
}

void Mod_LvdsTx_Handler(LvdsOutParameter *lvds_parameter)
{
    Mod_LvdsTx_StateHandler(lvds_parameter);
}

void Mod_MipiRx_Init(void)
{
    memset(&g_stPcrPara,0 ,sizeof(StructPcrPara));
    memset(&g_stMipiRxVidTiming_Get,0 ,sizeof(SrtuctMipiRx_VidTiming_Get));
}

void Mod_MipiRxDig_Set(void)
{
    Drv_MipiRx_InputSel();
    Drv_MipiRx_LaneSet();
}

u8 Mod_MipiRx_VidChk_Stable(void)
{
    lt9211c_write(0xff, 0x86);
    if((lt9211c_read(0x40) & 0x01) == 0x01)
    {
        return LONTIUM_TRUE;
    }
    else
    {
        return LONTIUM_FALSE;
    }

}

void Mod_MipiRx_StateHandler(StructChipRxVidTiming *ptn_timings)
{
    switch (g_stChipRx.ucRxState)
    {
        case STATE_CHIPRX_POWER_ON :
            Mod_MipiRx_Init();
            Mod_SystemRx_SetState(STATE_CHIPRX_WAITE_SOURCE);
        break;
        
        case STATE_CHIPRX_WAITE_SOURCE:
            Drv_MipiRx_PhyPowerOn();
            Drv_MipiRxClk_Sel();
            Drv_System_VidChkClk_SrcSel(MLRX_BYTE_CLK);
            Drv_System_VidChk_SrcSel(MIPIDEBUG);
            Drv_SystemActRx_Sel(MIPIRX);
            Mod_MipiRxDig_Set();
            Mod_SystemRx_SetState(STATE_CHIPRX_VIDTIMING_CONFIG);
        break;
        
        case STATE_CHIPRX_VIDTIMING_CONFIG:
            if(Drv_MipiRx_VidTiming_Get() == LONTIUM_TRUE)
            {
                g_stChipRx.ucRxFormat = Drv_MipiRx_VidFmt_Get(g_stMipiRxVidTiming_Get.ucFmt);
                if (Drv_MipiRx_VidTiming_Sel(ptn_timings) == LONTIUM_TRUE)
                {
                    Mod_SystemRx_SetState(STATE_CHIPRX_PLL_CONFIG);
                }
                else
                {
                    dev_err(dev,"No Video Timing Matched");
                    Mod_SystemRx_SetState(STATE_CHIPRX_WAITE_SOURCE);
                }
            }
        break;
  
        case STATE_CHIPRX_PLL_CONFIG: 
            Drv_MipiRx_DesscPll_Set();
            if(Drv_MipiRx_PcrCali() == SUCCESS)
            {
                dev_dbg(dev,"mipirx LT9211C pcr stable");
                Drv_System_VidChkClk_SrcSel(DESSCPLL_PIX_CLK);
                Drv_System_VidChk_SrcSel(MIPIRX);
                Mod_SystemRx_SetState(STATE_CHIPRX_VIDEO_CHECK);
            }
            else
            {
                Mod_SystemRx_SetState(STATE_CHIPRX_VIDTIMING_CONFIG);
            }
        break;

        case STATE_CHIPRX_VIDEO_CHECK:

            dev_dbg(dev,"mipirx Video Check Stable");
            g_stChipRx.pHdmiRxNotify(MIPIRX_VIDEO_ON_EVENT);
            Mod_SystemRx_SetState(STATE_CHIPRX_PLAY_BACK);

        break;

        case STATE_CHIPRX_PLAY_BACK:
        break;
    }
}

void Mod_MipiRx_Handler(StructChipRxVidTiming *ptn_timings)
{
      Mod_MipiRx_StateHandler(ptn_timings);
}
