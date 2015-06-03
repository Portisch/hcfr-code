/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005-2011 Association Homecinema Francophone.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////
//
//  This file is subject to the terms of the GNU General Public License as
//  published by the Free Software Foundation.  A copy of this license is
//  included with this software distribution in the file COPYING.htm. If you
//  do not have a copy, you may obtain a copy by writing to the Free
//  Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details
/////////////////////////////////////////////////////////////////////////////
//  Author(s):
//	Fran�ois-Xavier CHABOUD
//	Georges GALLERAND
/////////////////////////////////////////////////////////////////////////////

// CIEChartView.cpp : implementation file
//

#include "stdafx.h"
#include "ColorHCFR.h"
#include "MainFrm.h"
#include "DataSetDoc.h"
#include "DocTempl.h"
#include "CIEChartView.h"
#include <math.h>
#include "ximage.h"
#include "savegraphdialog.h"
#include "graphcontrol.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void DrawCIEChart(CDC* pDC,int aWidth, int aHeight, BOOL doFullChart, BOOL doShowBlack, BOOL bCIEuv);
extern void DrawDeltaECurve(CDC* pDC, int cxMax, int cyMax, double DeltaE, BOOL bCIEuv );

#define FX_MINSIZETOSHOW_SCALEDETAILS 300
#define FX_MINSIZETOSHOW_TRIANGLEDETAILS 100
#define FX_MINSIZETOSHOW_REFDETAILS 300

/////////////////////////////////////////////////////////////////////////////
// CCIEGraphPoint

CCIEGraphPoint::CCIEGraphPoint(const ColorXYZ& color, double WhiteYRef, CString aName, BOOL bConvertCIEuv) :
    name(aName),
    bCIEuv(bConvertCIEuv),
    m_color(color)
{
    m_color = ColorXYZ(color / WhiteYRef);
    ColorxyY colorxyY(color);
    ColorLab colorLab(color, 1.0, GetColorReference());
    double aX = colorxyY[0]; 
    double aY = colorxyY[1];
    double aL = colorLab[0]; 
	if ( bCIEuv )
	{
		x = ( 4.0 * aX ) / ( (-2.0 * aX ) + ( 12.0 * aY ) + 3.0 );
		y = ( 9.0 * aY ) / ( (-2.0 * aX ) + ( 12.0 * aY ) + 3.0 );
	}
	else
	{
		x = aX; 
		y = aY; 
	}
}

int CCIEGraphPoint::GetGraphX(CRect rect) const
{
	return (int)(x*(double)rect.Width()/(bCIEuv?0.7:0.8));	// graph is from 0 to 0.8 in width
}

int CCIEGraphPoint::GetGraphY(CRect rect) const
{
	return (int)((double)rect.bottom-y*(double)rect.Height()/(bCIEuv?0.7:0.9));	// graph is from 0 to 0.9 in height
}

CPoint CCIEGraphPoint::GetGraphPoint(CRect rect) const
{
	return CPoint(GetGraphX(rect),GetGraphY(rect));
}


/////////////////////////////////////////////////////////////////////////////
// CCIEChartGrapher

CCIEChartGrapher::CCIEChartGrapher()
{
	m_refRedPrimaryBitmap.LoadBitmap(IDB_REFREDPRIMARY_BITMAP);
	m_refGreenPrimaryBitmap.LoadBitmap(IDB_REFGREENPRIMARY_BITMAP);
	m_refBluePrimaryBitmap.LoadBitmap(IDB_REFBLUEPRIMARY_BITMAP);
	m_refYellowSecondaryBitmap.LoadBitmap(IDB_REFYELLOWSECONDARY_BITMAP);
	m_refCyanSecondaryBitmap.LoadBitmap(IDB_REFCYANSECONDARY_BITMAP);
	m_refMagentaSecondaryBitmap.LoadBitmap(IDB_REFMAGENTASECONDARY_BITMAP);
	m_illuminantPointBitmap.LoadBitmap(IDB_ILLUMINANTPOINT_BITMAP);
	m_colorTempPointBitmap.LoadBitmap(IDB_COLORTEMPPOINT_BITMAP);
	m_redPrimaryBitmap.LoadBitmap(IDB_REDPRIMARY_BITMAP);
	m_greenPrimaryBitmap.LoadBitmap(IDB_GREENPRIMARY_BITMAP);
	m_bluePrimaryBitmap.LoadBitmap(IDB_BLUEPRIMARY_BITMAP);
	m_yellowSecondaryBitmap.LoadBitmap(IDB_YELLOWSECONDARY_BITMAP);
	m_cyanSecondaryBitmap.LoadBitmap(IDB_CYANSECONDARY_BITMAP);
	m_magentaSecondaryBitmap.LoadBitmap(IDB_MAGENTASECONDARY_BITMAP);
	m_redSatRefBitmap.LoadBitmap(IDB_REFREDSAT_BITMAP);
	m_greenSatRefBitmap.LoadBitmap(IDB_REFGREENSAT_BITMAP);
	m_blueSatRefBitmap.LoadBitmap(IDB_REFBLUESAT_BITMAP);
	m_yellowSatRefBitmap.LoadBitmap(IDB_REFYELLOWSAT_BITMAP);
	m_cyanSatRefBitmap.LoadBitmap(IDB_REFCYANSAT_BITMAP);
	m_magentaSatRefBitmap.LoadBitmap(IDB_REFMAGENTASAT_BITMAP);
	m_cc24SatRefBitmap.LoadBitmap(IDB_REFCC24SAT_BITMAP);
	m_grayPlotBitmap.LoadBitmap(IDB_GRAYPLOT_BITMAP);
	m_measurePlotBitmap.LoadBitmap(IDB_MEASUREPLOT_BITMAP);
	m_selectedPlotBitmap.LoadBitmap(IDB_SELECTEDPLOT_BITMAP);

	m_datarefRedBitmap.LoadBitmap(IDB_REFCROSS_RED);
	m_datarefGreenBitmap.LoadBitmap(IDB_REFCROSS_GREEN);
	m_datarefBlueBitmap.LoadBitmap(IDB_REFCROSS_BLUE);
	m_datarefYellowBitmap.LoadBitmap(IDB_REFCROSS_YELLOW);
	m_datarefCyanBitmap.LoadBitmap(IDB_REFCROSS_CYAN);
	m_datarefMagentaBitmap.LoadBitmap(IDB_REFCROSS_MAGENTA);

	m_doDisplayBackground=GetConfig()->GetProfileInt("CIE Chart","Display Background",TRUE);
	m_doDisplayDeltaERef=GetConfig()->GetProfileInt("CIE Chart","Display Delta E",FALSE);
	m_doShowReferences=GetConfig()->GetProfileInt("CIE Chart","Show References",TRUE);
	m_doShowDataRef=GetConfig()->GetProfileInt("CIE Chart","Show Reference Data",TRUE);
	m_doShowGrayScale=GetConfig()->GetProfileInt("CIE Chart","Display GrayScale",TRUE);
	m_doShowSaturationScale=GetConfig()->GetProfileInt("CIE Chart","Display Saturation Scale",TRUE);
	m_doShowSaturationScaleTarg=GetConfig()->GetProfileInt("CIE Chart","Display Saturation Scale Targets",TRUE);
	m_doShowCCScale=GetConfig()->GetProfileInt("CIE Chart","Display Color Checker Measures",TRUE);
	m_doShowCCScaleTarg=GetConfig()->GetProfileInt("CIE Chart","Display Color Checker Targets",TRUE);
	m_doShowMeasurements=GetConfig()->GetProfileInt("CIE Chart","Show Measurements",TRUE);
	m_bCIEuv=GetConfig()->GetProfileInt("CIE Chart","CIE uv mode",FALSE);

	m_ZoomFactor = 1000;
	m_DeltaX = 0;
	m_DeltaY = 0;
}

void CCIEChartGrapher::MakeBgBitmap(CRect rect, BOOL bWhiteBkgnd)	// Create background bitmap
{
    int		i;
	CDC		ScreenDC;
	
	ScreenDC.CreateDC ( "DISPLAY", NULL, NULL, NULL );

    CDC bgDC;
    bgDC.CreateCompatibleDC(&ScreenDC);


    if(m_drawBitmap.m_hObject)
        m_drawBitmap.DeleteObject();
    m_drawBitmap.CreateCompatibleBitmap(&ScreenDC,rect.Width(),rect.Height());

    if(m_bgBitmap.m_hObject)
        m_bgBitmap.DeleteObject();
    m_bgBitmap.CreateCompatibleBitmap(&ScreenDC,rect.Width(),rect.Height());

    if(m_gamutBitmap.m_hObject)
        m_gamutBitmap.DeleteObject();
	if(m_doDisplayBackground)
	    m_gamutBitmap.CreateCompatibleBitmap(&ScreenDC,rect.Width(),rect.Height());

	BITMAP bm;
	GetColorApp() -> m_chartBitmap.GetBitmap(&bm);

    CBitmap *pOldBitmap=bgDC.SelectObject(&m_bgBitmap);
	int oldMode=bgDC.GetStretchBltMode();

	CDC memDC;
	memDC.CreateCompatibleDC( &ScreenDC );

	ScreenDC.DeleteDC ();
	
	if(m_doDisplayBackground)
	{
		// Manual draw of CIE chart: very slow and ugly...
//		DrawCIEChart(&bgDC,rect.Width(),rect.Height(),FALSE,TRUE, FALSE);	

		// Use correctly stretched bitmap instead 
		CBitmap* pOld;
		
		if ( bWhiteBkgnd )
		{
			if ( m_bCIEuv )
				pOld = memDC.SelectObject( & GetColorApp() -> m_chartBitmap_uv_white );
			else
				pOld = memDC.SelectObject( & GetColorApp() -> m_chartBitmap_white );
		}
		else
		{
			if ( m_bCIEuv )
				pOld = memDC.SelectObject( & GetColorApp() -> m_chartBitmap_uv );
			else
				pOld = memDC.SelectObject( & GetColorApp() -> m_chartBitmap );
		}
		
		bgDC.SetStretchBltMode(HALFTONE);
		SetBrushOrgEx(bgDC, 0,0, NULL);
		bgDC.StretchBlt(0,0,rect.right,rect.bottom,&memDC,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
	    memDC.SelectObject(pOld);
	}
	else
		bgDC.FillSolidRect(rect,bWhiteBkgnd?RGB(255,255,255):RGB(0,0,0));

	if ( m_doDisplayDeltaERef )
	{
		DrawDeltaECurve(&bgDC, rect.Width(),rect.Height(), 3.0, m_bCIEuv );
		DrawDeltaECurve(&bgDC, rect.Width(),rect.Height(), 10.0, m_bCIEuv );
	}

	// Draw axis
    CPen axisPen(PS_SOLID,1,RGB(64,64,64));
    CPen *pOldPen = bgDC.SelectObject(&axisPen); 

	bgDC.SetTextAlign(TA_BOTTOM);
	bgDC.SetTextColor(bWhiteBkgnd?RGB(0,0,0):RGB(255,255,255));
	bgDC.SetBkMode(TRANSPARENT);

	// Initializes a CFont object with the specified characteristics. 
	CFont font;
	font.CreatePointFont(min(120,(100*rect.Width()/bm.bmWidth)),"Default",NULL);

	CFont* pOldFont = bgDC.SelectObject(&font);

	for(i=0;i<(m_bCIEuv?7:8);i++)	// Draw X axis
	{
		int x=(int)(rect.Width()*i/(m_bCIEuv?7.0:8.0));
		CString str;

		str.Format("%.1f",i/10.0);
		bgDC.MoveTo(x,0);
		bgDC.LineTo(x,rect.bottom);
		if(i && min(rect.Width(),rect.Height()) > FX_MINSIZETOSHOW_REFDETAILS )
			bgDC.TextOut(x+2,rect.bottom,str); // Draw axis label
	}

	for(i=0;i<(m_bCIEuv?7:9);i++) 	// Draw Y axis
	{
		int y=(int)(rect.Height()*i/(m_bCIEuv?7.0:9.0));
		CString str;

		str.Format("%.1f",(m_bCIEuv?0.7:0.9)-i/10.0);
		bgDC.MoveTo(0,y);
		bgDC.LineTo(rect.right,y);
		if(i && min(rect.Width(),rect.Height()) > FX_MINSIZETOSHOW_REFDETAILS)
			bgDC.TextOut(2,y,str);
	}

	bgDC.SelectObject(pOldPen);
	bgDC.SelectObject(pOldFont);

	CGraphControl::DrawFiligree ( &bgDC, rect, bWhiteBkgnd?RGB(192,192,192):RGB(64,64,64) );
	
	// Create stretched bitmap for gamut hilighting

	if(m_doDisplayBackground)
	{
		bgDC.SelectObject(&m_gamutBitmap);
		GetColorApp() -> m_lightenChartBitmap.GetBitmap(&bm);
		CBitmap* pOld = memDC.SelectObject(m_bCIEuv ? & GetColorApp() -> m_lightenChartBitmap_uv : & GetColorApp() -> m_lightenChartBitmap);
		bgDC.SetStretchBltMode(HALFTONE);
		SetBrushOrgEx(bgDC, 0,0, NULL);

		// Manual draw of CIE chart: very slow and ugly...
//		DrawCIEChart(&bgDC,rect.Width(),rect.Height(),TRUE,FALSE, FALSE);	

		// Use correctly stretched bitmap instead 
		bgDC.StretchBlt(0,0,rect.right,rect.bottom,&memDC,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);

		memDC.SelectObject(pOld);

		if ( m_doDisplayDeltaERef )
		{
			DrawDeltaECurve(&bgDC, rect.Width(),rect.Height(), 3.0, m_bCIEuv );
			DrawDeltaECurve(&bgDC, rect.Width(),rect.Height(), 10.0, m_bCIEuv );
		}
	}

	bgDC.SetStretchBltMode(oldMode); 
	bgDC.SelectObject(pOldBitmap);
}

void CCIEChartGrapher::DrawAlphaBitmap(CDC *pDC, const CCIEGraphPoint& aGraphPoint, CBitmap *pBitmap, CRect rect, CPPToolTip * pTooltip, CWnd * pWnd, CCIEGraphPoint * pRefPoint)
{
	ASSERT(pBitmap);
	ASSERT(pDC);

	BITMAP bm;
	pBitmap->GetBitmap(&bm);

	// Add tootip if name is defined
	if(!aGraphPoint.name.IsEmpty())
	{
		CString str, str2, str3;
		int x=aGraphPoint.GetGraphX(rect)+m_DeltaX;
		int y=aGraphPoint.GetGraphY(rect)+m_DeltaY;
		// Note: remove 5 pixels for transparency area of bitmap
		CRect rect_tip(x-bm.bmWidth/2+5,y-bm.bmHeight/2+5,x+bm.bmWidth/2-5,y+bm.bmHeight/2-5);
		if ( m_bCIEuv )
			str.Format("u': %.3f, v': %.3f",aGraphPoint.x,aGraphPoint.y);
		else
			str.Format("x: %.3f, y: %.3f",aGraphPoint.x,aGraphPoint.y);
		
		if ( pRefPoint )
		{
            double dE  = aGraphPoint.GetNormalizedColor().GetDeltaE(1.0, pRefPoint->GetNormalizedColor(), 1.0, GetColorReference(), GetConfig()->m_dE_form, false, GetConfig()->gw_Weight );
			str2.Format ( ", Delta E: %.1f\n",dE );
			str3.LoadString (IDS_DISTANCEINCIEXY);
			str2 += str3;

			double dXY = aGraphPoint.GetNormalizedColor().GetDeltaxy(pRefPoint->GetNormalizedColor(), GetColorReference());
			str3.Format ( ": %.3f xy", dXY );
			str2 += str3;
		}
		
		if ( pTooltip )
			pTooltip -> AddTool(pWnd, "<b>"+CString(aGraphPoint.name) +"</b> \n" +str+str2,&rect_tip);
	}

	CDC memDC;
	memDC.CreateCompatibleDC( pDC );

	CBitmap* pOld = memDC.SelectObject(pBitmap);
	BLENDFUNCTION bf;
	bf.BlendOp=AC_SRC_OVER;
	bf.BlendFlags=0;
	bf.AlphaFormat=0x01;  // AC_SRC_ALPHA=0x01
	bf.SourceConstantAlpha=255;
	AlphaBlend(*pDC,aGraphPoint.GetGraphX(rect)-bm.bmWidth/2,aGraphPoint.GetGraphY(rect)-bm.bmHeight/2,bm.bmWidth,bm.bmHeight,memDC,0,0,bm.bmWidth,bm.bmHeight,bf);

	memDC.SelectObject(pOld);
}

void CCIEChartGrapher::DrawChart(CDataSetDoc * pDoc, CDC* pDC, CRect rect, CPPToolTip * pTooltip, CWnd * pWnd) 
{
	CColorHCFRApp *	pApp = GetColorApp();
	CString			Msg, Msg2, Msg3;
	CDataSetDoc *	pDataRef = GetDataRef();
                    
				char*  PatName[96]={
                    "White",
                    "6J",
                    "5F",
                    "6I",
                    "6K",
                    "5G",
                    "6H",
                    "5H",
                    "7K",
                    "6G",
                    "5I",
                    "6F",
                    "8K",
                    "5J",
                    "Black",
                    "2B",
                    "2C",
                    "2D",
                    "2E",
                    "2F",
                    "2G",
                    "2H",
                    "2I",
                    "2J",
                    "2K",
                    "2L",
                    "2M",
                    "3B",
                    "3C",
                    "3D",
                    "3E",
                    "3F",
                    "3G",
                    "3H",
                    "3I",
                    "3J",
                    "3K",
                    "3L",
                    "3M",
                    "4B",
                    "4C",
                    "4D",
                    "4E",
                    "4F",
                    "4G",
                    "4H",
                    "4I",
                    "4J",
                    "4K",
                    "4L",
                    "4M",
                    "5B",
                    "5C",
                    "5D",
                    "5K",
                    "5L",
                    "5M",
                    "6B",
                    "6C",
                    "6D",
                    "6L",
                    "6M",
                    "7B",
                    "7C",
                    "7D",
                    "7E",
                    "7F",
                    "7G",
                    "7H",
                    "7I",
                    "7J",
                    "7L",
                    "7M",
                    "8B",
                    "8C",
                    "8D",
                    "8E",
                    "8F",
                    "8G",
                    "8H",
                    "8I",
                    "8J",
                    "8L",
                    "8M",
                    "9B",
                    "9C",
                    "9D",
                    "9E",
                    "9F",
                    "9G",
                    "9H",
                    "9I",
                    "9J",
                    "9K",
                    "9L",
                    "9M" };

                    
					char*  PatNameCMS[19]={
						"White",
						"Black",
						"2E",
						"2F",
						"2K",
						"5D",
						"7E",
						"7F",
						"7G",
						"7H",
						"7I",
						"7J",
						"8D",
						"8E",
						"8F",
						"8G",
						"8H",
						"8I",
						"8J" };
                    
						char*  PatNameCPS[19]={
						"White",
						"D7",
						"D8",
						"E7",
						"E8",
						"F7",
						"F8",
						"G7",
						"G8",
						"H7",
						"H8",
						"I7",
						"I8",
						"J7",
						"J8",
						"CP-Light",
						"CP-Dark",
						"Dark Skin",
						"Light Skin" };

						char*  PatNameAXIS[71]={
						"Black",
						"White 10",
						"White 20",
						"White 30",
						"White 40",
						"White 50",
						"White 60",
						"White 70",
						"White 80",
						"White 90",
						"White 100",
						"Red 10",
						"Red 20",
						"Red 30",
						"Red 40",
						"Red 50",
						"Red 60",
						"Red 70",
						"Red 80",
						"Red 90",
						"Red 100",
						"Green 10",
						"Green 20",
						"Green 30",
						"Green 40",
						"Green 50",
						"Green 60",
						"Green 70",
						"Green 80",
						"Green 90",
						"Green 100",
						"Blue 10",
						"Blue 20",
						"Blue 30",
						"Blue 40",
						"Blue 50",
						"Blue 60",
						"Blue 70",
						"Blue 80",
						"Blue 90",
						"Blue 100", 
						"Cyan 10",
						"Cyan 20",
						"Cyan 30",
						"Cyan 40",
						"Cyan 50",
						"Cyan 60",
						"Cyan 70",
						"Cyan 80",
						"Cyan 90",
						"Cyan 100", 
						"Magenta 10",
						"Magenta 20",
						"Magenta 30",
						"Magenta 40",
						"Magenta 50",
						"Magenta 60",
						"Magenta 70",
						"Magenta 80",
						"Magenta 90",
						"Magenta 100", 
						"Yellow 10",
						"Yellow 20",
						"Yellow 30",
						"Yellow 40",
						"Yellow 50",
						"Yellow 60",
						"Yellow 70",
						"Yellow 80",
						"Yellow 90",
						"Yellow 100"
						};

	if ( pDataRef == pDoc || ! m_doShowDataRef )
		pDataRef = NULL;

	// Wait for background thread terminating background bitmaps creation
	if ( WAIT_TIMEOUT == WaitForSingleObject ( pApp -> m_hCIEEvent, 0 ) )
	{
		CWaitCursor wait;
		if ( pApp -> m_hCIEThread )
		{
			// Increase background thread priority
			::SetThreadPriority ( pApp -> m_hCIEThread, THREAD_PRIORITY_NORMAL );
		}
		
		WaitForSingleObject ( pApp -> m_hCIEEvent, INFINITE );
	}

	if ( pTooltip )
		pTooltip -> RemoveAllTools();

	// Default is SDTV / NTSC
	Msg.LoadString ( IDS_NTSCREDREF );
	Msg2.LoadString ( IDS_NTSCGREENREF );
	Msg3.LoadString ( IDS_NTSCBLUEREF );

	if(GetConfig()->m_colorStandard == PALSECAM)
	{
		Msg.LoadString ( IDS_PALREDREF );
		Msg2.LoadString ( IDS_PALGREENREF );
		Msg3.LoadString ( IDS_PALBLUEREF );
	}else if(GetConfig()->m_colorStandard == CUSTOM)
	{
		Msg.LoadString ( IDS_CUSTREDREF );
		Msg2.LoadString ( IDS_CUSTGREENREF );
		Msg3.LoadString ( IDS_CUSTBLUEREF );
	} else if(GetConfig()->m_colorStandard == UHDTV)
	{
		Msg.LoadString ( IDS_UHDTVREDREF );
		Msg2.LoadString ( IDS_UHDTVGREENREF );
		Msg3.LoadString ( IDS_UHDTVBLUEREF );
	} else if(GetConfig()->m_colorStandard == HDTV || GetConfig()->m_colorStandard == sRGB)
	{
		Msg.LoadString ( IDS_REC709REDREF );
		Msg2.LoadString ( IDS_REC709GREENREF );
		Msg3.LoadString ( IDS_REC709BLUEREF );
	}else if(GetConfig()->m_colorStandard == HDTVa || GetConfig()->m_colorStandard == HDTVb )
	{
		Msg.LoadString ( IDS_REC709aREDREF );
		Msg2.LoadString ( IDS_REC709aGREENREF );
		Msg3.LoadString ( IDS_REC709aBLUEREF );
	} else if(GetConfig()->m_colorStandard == CC6)
	{
		Msg.LoadString ( IDS_RECCC6REDREF );
		Msg2.LoadString ( IDS_RECCC6GREENREF );
		Msg3.LoadString ( IDS_RECCC6BLUEREF );
	}

	ColorXYZ cR=GetColorReference().GetRed();
	ColorXYZ cG=GetColorReference().GetGreen();
	ColorXYZ cB=GetColorReference().GetBlue();
	ColorXYZ cY=GetColorReference().GetYellow();
	ColorXYZ cC=GetColorReference().GetCyan();
	ColorXYZ cM=GetColorReference().GetMagenta();

	CColor aColor[6];
	
	aColor[0].SetXYZValue(cR);
	aColor[1].SetXYZValue(cG);
	aColor[2].SetXYZValue(cB);
	aColor[3].SetXYZValue(cY);
	aColor[4].SetXYZValue(cC);
	aColor[5].SetXYZValue(cM);
	ColorRGB rgb[6];
	for(int i=0;i<6;i++)
		rgb[i]=aColor[i].GetRGBValue ( GetColorReference() );
	double r[6],g[6],b[6];
    double gamma=(GetConfig()->m_useMeasuredGamma)?(GetConfig()->m_GammaAvg):(GetConfig()->m_GammaRef);
//    CColor White = pDoc->GetMeasure()-> GetGray ( pDoc->GetMeasure()->GetGrayScaleSize() - 1 );
//	CColor Black = pDoc->GetMeasure()->GetGray ( 0 );
	CColor White = pDoc->GetMeasure()->GetOnOffWhite();
	CColor Black = pDoc->GetMeasure()->GetOnOffBlack();

    for(int i=0;i<6;i++)
    {
        double r1,g1,b1;
		r[i]=rgb[i][0];
		g[i]=rgb[i][1];
		b[i]=rgb[i][2];
        if (GetConfig()->m_GammaOffsetType == 4 && White.isValid() && Black.isValid())
           gamma = log(GetBT1886(pow(aColor[i].GetY(),1.0/2.22),White,Black,GetConfig()->m_GammaRel, GetConfig()->m_Split))/log(pow(aColor[i].GetY(),1.0/2.22));
        r1=(r[i]<=0||r[i]>=1)?min(max(r[i],0),1):pow(pow(r[i],1.0/2.22),gamma);
        g1=(g[i]<=0||g[i]>=1)?min(max(g[i],0),1):pow(pow(g[i],1.0/2.22),gamma);
        b1=(b[i]<=0||b[i]>=1)?min(max(b[i],0),1):pow(pow(b[i],1.0/2.22),gamma);
		aColor[i].SetRGBValue (ColorRGB(r1,g1,b1),GetColorReference());	
    }

	CCIEGraphPoint refRedPrimaryPoint(aColor[0].GetXYZValue(), 1.0, Msg, m_bCIEuv);
	CCIEGraphPoint refGreenPrimaryPoint(aColor[1].GetXYZValue(), 1.0, Msg2, m_bCIEuv);
	CCIEGraphPoint refBluePrimaryPoint(aColor[2].GetXYZValue(), 1.0, Msg3, m_bCIEuv);

	CCIEGraphPoint whiteRef(GetColorReference().GetWhite(), 1.0, "", m_bCIEuv);

	Msg.LoadString ( (GetColorReference().m_standard!=CC6)?IDS_YELLOWSECONDARYREF:IDS_CC6YELLOWSECONDARYREF );
	CCIEGraphPoint refYellowSecondaryPoint(aColor[3].GetXYZValue(), 1.0, Msg, m_bCIEuv);

	Msg.LoadString (  (GetColorReference().m_standard!=CC6)?IDS_CYANSECONDARYREF:IDS_CC6CYANSECONDARYREF );
	CCIEGraphPoint refCyanSecondaryPoint(aColor[4].GetXYZValue(), 1.0, Msg, m_bCIEuv);

	Msg.LoadString ( (GetColorReference().m_standard!=CC6)?IDS_MAGENTASECONDARYREF:IDS_CC6MAGENTASECONDARYREF );
	CCIEGraphPoint refMagentaSecondaryPoint(aColor[5].GetXYZValue(), 1.0, Msg, m_bCIEuv);

	CCIEGraphPoint illuminantA(ColorXYZ(ColorxyY(0.4476,0.4074)),1.0, "Illuminant A", m_bCIEuv);
	CCIEGraphPoint illuminantB(ColorXYZ(ColorxyY(0.3484,0.3516)), 1.0, "Illuminant B", m_bCIEuv);
	CCIEGraphPoint illuminantC(ColorXYZ(ColorxyY(0.3101,0.3162)), 1.0, "Illuminant C", m_bCIEuv);
	CCIEGraphPoint illuminantD65(ColorXYZ(ColorxyY(0.3127,0.3291)), 1.0, "Illuminant D65", m_bCIEuv);

	Msg.LoadString ( IDS_TEMPERATURE );
	CCIEGraphPoint colorTempPoint2700(ColorXYZ(ColorxyY(0.4614,0.4158)), 1.0, Msg+" 2700", m_bCIEuv);   
	CCIEGraphPoint colorTempPoint3000(ColorXYZ(ColorxyY(0.4388,0.4095)), 1.0, Msg+" 3000", m_bCIEuv);	  
//	CCIEGraphPoint colorTempPoint3500(0.4075,0.3962,100.0,Msg+" 3500", m_bCIEuv);   
	CCIEGraphPoint colorTempPoint4000(ColorXYZ(ColorxyY(0.3827,0.3820)), 1.0, Msg+" 4000", m_bCIEuv);  
	CCIEGraphPoint colorTempPoint5500(ColorXYZ(ColorxyY(0.3346,0.3451)), 1.0, Msg+" 5500", m_bCIEuv);   
	CCIEGraphPoint colorTempPoint9300(ColorXYZ(ColorxyY(0.2866,0.2950)), 1.0, Msg+" 9300", m_bCIEuv);   

	ColorXYZ redPrimaryColor=pDoc->GetMeasure()->GetRedPrimary().GetXYZValue();
	ColorXYZ greenPrimaryColor=pDoc->GetMeasure()->GetGreenPrimary().GetXYZValue();
	ColorXYZ bluePrimaryColor=pDoc->GetMeasure()->GetBluePrimary().GetXYZValue();

	BOOL hasPrimaries= redPrimaryColor.isValid() && greenPrimaryColor.isValid() &&
					   bluePrimaryColor.isValid();

	// Take sum of primary colors Y by default, in case of no white measure found
	double YWhite = redPrimaryColor[2]+greenPrimaryColor[2]+bluePrimaryColor[2];
	
	if ( pDoc -> GetMeasure () -> GetOnOffWhite ().isValid() )
		YWhite = pDoc -> GetMeasure () -> GetOnOffWhite () [ 1 ]; //onoff white is always grayscale white

	Msg.LoadString ( IDS_REDPRIMARY );
	CCIEGraphPoint redPrimaryPoint(redPrimaryColor, YWhite, Msg, m_bCIEuv);
	Msg.LoadString ( IDS_GREENPRIMARY );
	CCIEGraphPoint greenPrimaryPoint(greenPrimaryColor, YWhite, Msg, m_bCIEuv);
	Msg.LoadString ( IDS_BLUEPRIMARY );
	CCIEGraphPoint bluePrimaryPoint(bluePrimaryColor, YWhite, Msg, m_bCIEuv);

	ColorXYZ yellowSecondaryColor=pDoc->GetMeasure()->GetYellowSecondary().GetXYZValue();
	ColorXYZ cyanSecondaryColor=pDoc->GetMeasure()->GetCyanSecondary().GetXYZValue();
	ColorXYZ magentaSecondaryColor=pDoc->GetMeasure()->GetMagentaSecondary().GetXYZValue();

	BOOL hasSecondaries= yellowSecondaryColor.isValid() && cyanSecondaryColor.isValid() &&
					   magentaSecondaryColor.isValid();

	Msg.LoadString ( IDS_YELLOWSECONDARY );
	CCIEGraphPoint yellowSecondaryPoint(yellowSecondaryColor, YWhite, Msg, m_bCIEuv);
	Msg.LoadString ( IDS_CYANSECONDARY );
	CCIEGraphPoint cyanSecondaryPoint(cyanSecondaryColor, YWhite, Msg, m_bCIEuv);
	Msg.LoadString ( IDS_MAGENTASECONDARY );
	CCIEGraphPoint magentaSecondaryPoint(magentaSecondaryColor, YWhite, Msg, m_bCIEuv);

	ColorXYZ datarefRed;
	ColorXYZ datarefGreen;
	ColorXYZ datarefBlue;
	ColorXYZ datarefYellow;
	ColorXYZ datarefCyan;
	ColorXYZ datarefMagenta;
	
	BOOL hasdatarefPrimaries = FALSE;
	BOOL hasdatarefSecondaries = FALSE;

	double YWhiteRef = 1.0;

	if ( pDataRef )
	{
		datarefRed=pDataRef->GetMeasure()->GetRedPrimary().GetXYZValue();
		datarefGreen=pDataRef->GetMeasure()->GetGreenPrimary().GetXYZValue();
		datarefBlue=pDataRef->GetMeasure()->GetBluePrimary().GetXYZValue();
		datarefYellow=pDataRef->GetMeasure()->GetYellowSecondary().GetXYZValue();
		datarefCyan=pDataRef->GetMeasure()->GetCyanSecondary().GetXYZValue();
		datarefMagenta=pDataRef->GetMeasure()->GetMagentaSecondary().GetXYZValue();

		hasdatarefPrimaries = datarefRed.isValid() && datarefGreen.isValid() && datarefBlue.isValid();
		hasdatarefSecondaries = datarefYellow.isValid() && datarefCyan.isValid() && datarefMagenta.isValid();

		YWhiteRef = datarefRed[1]+datarefGreen[1]+datarefBlue[1];
		
		if ( pDataRef -> GetMeasure () -> GetOnOffWhite ().isValid() )
			YWhiteRef = pDataRef -> GetMeasure () -> GetOnOffWhite () [ 1 ]; //onoff white is always grayscale white
	}
	
	Msg.LoadString ( IDS_DATAREF_RED );
	CCIEGraphPoint datarefRedPoint(datarefRed, YWhiteRef, Msg, m_bCIEuv);
	Msg.LoadString ( IDS_DATAREF_GREEN );
	CCIEGraphPoint datarefGreenPoint(datarefGreen, YWhiteRef, Msg, m_bCIEuv);
	Msg.LoadString ( IDS_DATAREF_BLUE );
	CCIEGraphPoint datarefBluePoint(datarefBlue, YWhiteRef, Msg, m_bCIEuv);

	Msg.LoadString ( IDS_DATAREF_YELLOW );
	CCIEGraphPoint datarefYellowPoint(datarefYellow, YWhiteRef, Msg, m_bCIEuv);
	Msg.LoadString ( IDS_DATAREF_CYAN );
	CCIEGraphPoint datarefCyanPoint(datarefCyan, YWhiteRef, Msg, m_bCIEuv);
	Msg.LoadString ( IDS_DATAREF_MAGENTA );
	CCIEGraphPoint datarefMagentaPoint(datarefMagenta, YWhiteRef, Msg, m_bCIEuv);


	// Draw background bitmap
	CDC dcBg;
	dcBg.CreateCompatibleDC(pDC);
	CBitmap *pOldBitmap=dcBg.SelectObject(&m_bgBitmap);
	pDC->BitBlt(0,0,rect.Width(),rect.Height(),&dcBg,0,0,SRCCOPY);
	dcBg.SelectObject(pOldBitmap);

	// Fill triangle with ligthen chart
	if(m_doDisplayBackground && hasPrimaries)
	{
		CBrush gamutBrush;
		CPoint ptVertex[6];
		gamutBrush.CreatePatternBrush(&m_gamutBitmap);
		CBrush *pOldBrush=pDC->SelectObject(&gamutBrush);
		if (hasSecondaries)
		{
			ptVertex[0] = redPrimaryPoint.GetGraphPoint(rect);
			ptVertex[1] = yellowSecondaryPoint.GetGraphPoint(rect);
			ptVertex[2] = greenPrimaryPoint.GetGraphPoint(rect);
			ptVertex[3] = cyanSecondaryPoint.GetGraphPoint(rect);
			ptVertex[4] = bluePrimaryPoint.GetGraphPoint(rect);
			ptVertex[5] = magentaSecondaryPoint.GetGraphPoint(rect);
		}
		else
		{
			ptVertex[0] = redPrimaryPoint.GetGraphPoint(rect);
			ptVertex[1] = redPrimaryPoint.GetGraphPoint(rect);
			ptVertex[2] = greenPrimaryPoint.GetGraphPoint(rect);
			ptVertex[3] = greenPrimaryPoint.GetGraphPoint(rect);
			ptVertex[4] = bluePrimaryPoint.GetGraphPoint(rect);
			ptVertex[5] = bluePrimaryPoint.GetGraphPoint(rect);
		}

		CRgn triangleRgn;
		triangleRgn.CreatePolygonRgn(ptVertex,6,WINDING);
		pDC->PaintRgn(&triangleRgn);
		pDC->SelectObject(pOldBrush);
	}

	int penWidth=(min(rect.Width(),rect.Height()) > FX_MINSIZETOSHOW_TRIANGLEDETAILS) ? 3: 2;

 	// Draw reference gamut triangle
    CPen refPrimariesPen(PS_SOLID,penWidth,RGB(128,128,128));
    CPen *pOldPen = pDC->SelectObject(&refPrimariesPen); 

	pDC->SetBkMode(TRANSPARENT);
	if(m_doDisplayBackground)
		pDC->SetROP2(R2_MASKPEN);
	else
		pDC->SetROP2(R2_COPYPEN);

	pDC->MoveTo(refRedPrimaryPoint.GetGraphPoint(rect));
	pDC->LineTo(refYellowSecondaryPoint.GetGraphPoint(rect));
	pDC->LineTo(refGreenPrimaryPoint.GetGraphPoint(rect));
	pDC->LineTo(refCyanSecondaryPoint.GetGraphPoint(rect));
	pDC->LineTo(refBluePrimaryPoint.GetGraphPoint(rect));
	pDC->LineTo(refMagentaSecondaryPoint.GetGraphPoint(rect));
	pDC->LineTo(refRedPrimaryPoint.GetGraphPoint(rect));

	pDC->SelectObject(pOldPen);

	pDC->SetBkMode(TRANSPARENT);
	pDC->SetROP2(R2_COPYPEN);

	if ( hasdatarefPrimaries )
	{
		CPen datarefPrimariesPen(PS_SOLID,penWidth-1,RGB(192,192,192));
		pOldPen = pDC->SelectObject(&datarefPrimariesPen); 

		pDC->MoveTo(datarefRedPoint.GetGraphPoint(rect));
		pDC->LineTo(datarefGreenPoint.GetGraphPoint(rect));
		pDC->LineTo(datarefBluePoint.GetGraphPoint(rect));
		pDC->LineTo(datarefRedPoint.GetGraphPoint(rect));
		
		pDC->SelectObject(pOldPen);
	}

    CPen primariesPen(PS_SOLID,penWidth,RGB(255,255,255));
    pOldPen = pDC->SelectObject(&primariesPen); 

	// Draw gamut triangle
	if(hasPrimaries)
	{
		if (hasSecondaries)
		{
			pDC->MoveTo(redPrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(yellowSecondaryPoint.GetGraphPoint(rect));
			pDC->LineTo(greenPrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(cyanSecondaryPoint.GetGraphPoint(rect));
			pDC->LineTo(bluePrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(magentaSecondaryPoint.GetGraphPoint(rect));
			pDC->LineTo(redPrimaryPoint.GetGraphPoint(rect));
		}
		else
		{
			pDC->MoveTo(redPrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(greenPrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(bluePrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(redPrimaryPoint.GetGraphPoint(rect));
		}
	}
	pDC->SelectObject(pOldPen);

	// Draw white reference white dashed cross 
	if(min(rect.Width(),rect.Height()) > FX_MINSIZETOSHOW_SCALEDETAILS )
	{
		CPen whiteCrossPen(PS_DOT,1,RGB(192,192,192));
		pOldPen = pDC->SelectObject(&whiteCrossPen); 

		pDC->MoveTo(5,whiteRef.GetGraphY(rect));
		pDC->LineTo(rect.right-5,whiteRef.GetGraphY(rect));
		pDC->MoveTo(whiteRef.GetGraphX(rect),5);
		pDC->LineTo(whiteRef.GetGraphX(rect),rect.bottom-5);

		pDC->SelectObject(pOldPen);
	}

	// Draw bitmaps on triangle vertex
	if(min(rect.Width(),rect.Height()) > FX_MINSIZETOSHOW_TRIANGLEDETAILS)  // Enough room to draw details
	{
		DrawAlphaBitmap(pDC,refRedPrimaryPoint,&m_refRedPrimaryBitmap,rect,pTooltip,pWnd);
		DrawAlphaBitmap(pDC,refGreenPrimaryPoint,&m_refGreenPrimaryBitmap,rect,pTooltip,pWnd);
		DrawAlphaBitmap(pDC,refBluePrimaryPoint,&m_refBluePrimaryBitmap,rect,pTooltip,pWnd);
		
		DrawAlphaBitmap(pDC,refYellowSecondaryPoint,&m_refYellowSecondaryBitmap,rect,pTooltip,pWnd);
		DrawAlphaBitmap(pDC,refCyanSecondaryPoint,&m_refCyanSecondaryBitmap,rect,pTooltip,pWnd);
		DrawAlphaBitmap(pDC,refMagentaSecondaryPoint,&m_refMagentaSecondaryBitmap,rect,pTooltip,pWnd);

		if(m_doDisplayBackground)
			pDC->SetTextColor(RGB(0,0,0));
		else
			pDC->SetTextColor(RGB(255,255,255));
		pDC->SetBkMode(TRANSPARENT);

		if(m_doShowReferences)
		{
			BITMAP bm;
			GetColorApp() -> m_chartBitmap.GetBitmap(&bm);
			// Initializes a CFont object with the specified characteristics. 
			CFont font;
			font.CreatePointFont(min(120,(100*rect.Width()/bm.bmWidth)),"Arial",NULL);

			CFont* pOldFont = pDC->SelectObject(&font);

			// Draw ref illuminant points
			pDC->SetTextAlign(TA_TOP|TA_LEFT);
			DrawAlphaBitmap(pDC,illuminantA,&m_illuminantPointBitmap,rect,pTooltip,pWnd);
			pDC->TextOut(illuminantA.GetGraphX(rect)-2,illuminantA.GetGraphY(rect)+4,"A");
			DrawAlphaBitmap(pDC,illuminantB,&m_illuminantPointBitmap,rect,pTooltip,pWnd);
			pDC->TextOut(illuminantB.GetGraphX(rect)+4,illuminantB.GetGraphY(rect)+4,"B");
			DrawAlphaBitmap(pDC,illuminantC,&m_illuminantPointBitmap,rect,pTooltip,pWnd);
			pDC->TextOut(illuminantC.GetGraphX(rect)+4,illuminantC.GetGraphY(rect)+4,"C");
			DrawAlphaBitmap(pDC,illuminantD65,&m_illuminantPointBitmap,rect,pTooltip,pWnd);
			pDC->SetTextAlign(TA_BOTTOM|TA_RIGHT);
			pDC->TextOut(illuminantC.GetGraphX(rect)-4,illuminantC.GetGraphY(rect)-4,"D65");

			// Draw color temp points
			pDC->SetTextAlign(TA_BOTTOM|TA_RIGHT);
			DrawAlphaBitmap(pDC,colorTempPoint9300,&m_colorTempPointBitmap,rect,pTooltip,pWnd);
			pDC->TextOut(colorTempPoint9300.GetGraphX(rect)-2,colorTempPoint9300.GetGraphY(rect)-2,"9300");
			pDC->SetTextAlign(TA_BOTTOM|TA_RIGHT);
			DrawAlphaBitmap(pDC,colorTempPoint4000,&m_colorTempPointBitmap,rect,pTooltip,pWnd);
			pDC->TextOut(colorTempPoint4000.GetGraphX(rect)-2,colorTempPoint4000.GetGraphY(rect)-2,"4000");
			DrawAlphaBitmap(pDC,colorTempPoint5500,&m_colorTempPointBitmap,rect,pTooltip,pWnd);
			pDC->TextOut(colorTempPoint5500.GetGraphX(rect)-2,colorTempPoint5500.GetGraphY(rect)-2,"5500");
			DrawAlphaBitmap(pDC,colorTempPoint3000,&m_colorTempPointBitmap,rect,pTooltip,pWnd);
			pDC->SetTextAlign(TA_BOTTOM|TA_RIGHT);
			pDC->TextOut(colorTempPoint3000.GetGraphX(rect)+2,colorTempPoint3000.GetGraphY(rect)-2,"3000");
			DrawAlphaBitmap(pDC,colorTempPoint2700,&m_colorTempPointBitmap,rect,pTooltip,pWnd);
			pDC->SetTextAlign(TA_BOTTOM|TA_LEFT);
			pDC->TextOut(colorTempPoint2700.GetGraphX(rect)-2,colorTempPoint2700.GetGraphY(rect)-2,"2700");

			pDC->SelectObject(pOldFont);
		}

		if(hasPrimaries && hasSecondaries)
		{
			// Draw lines between primaries and secondaries
			CPen secondariesPen(PS_DOT,1,RGB(64,64,64));
			pOldPen = pDC->SelectObject(&secondariesPen); 

			pDC->MoveTo(redPrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(cyanSecondaryPoint.GetGraphPoint(rect));

			pDC->MoveTo(greenPrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(magentaSecondaryPoint.GetGraphPoint(rect));

			pDC->MoveTo(bluePrimaryPoint.GetGraphPoint(rect));
			pDC->LineTo(yellowSecondaryPoint.GetGraphPoint(rect));

			pDC->SelectObject(pOldPen);
		}
		
		if ( hasPrimaries )
		{
			DrawAlphaBitmap(pDC,redPrimaryPoint,&m_redPrimaryBitmap,rect,pTooltip,pWnd,&refRedPrimaryPoint);
			DrawAlphaBitmap(pDC,greenPrimaryPoint,&m_greenPrimaryBitmap,rect,pTooltip,pWnd,&refGreenPrimaryPoint);
			DrawAlphaBitmap(pDC,bluePrimaryPoint,&m_bluePrimaryBitmap,rect,pTooltip,pWnd,&refBluePrimaryPoint);
		}
		
		if ( hasSecondaries )
		{
			DrawAlphaBitmap(pDC,yellowSecondaryPoint,&m_yellowSecondaryBitmap,rect,pTooltip,pWnd,&refYellowSecondaryPoint);
			DrawAlphaBitmap(pDC,cyanSecondaryPoint,&m_cyanSecondaryBitmap,rect,pTooltip,pWnd,&refCyanSecondaryPoint);
			DrawAlphaBitmap(pDC,magentaSecondaryPoint,&m_magentaSecondaryBitmap,rect,pTooltip,pWnd,&refMagentaSecondaryPoint);
		}

		if ( hasdatarefPrimaries )
		{
			DrawAlphaBitmap(pDC,datarefRedPoint,&m_datarefRedBitmap,rect,pTooltip,pWnd,&refRedPrimaryPoint);
			DrawAlphaBitmap(pDC,datarefGreenPoint,&m_datarefGreenBitmap,rect,pTooltip,pWnd,&refGreenPrimaryPoint);
			DrawAlphaBitmap(pDC,datarefBluePoint,&m_datarefBlueBitmap,rect,pTooltip,pWnd,&refBluePrimaryPoint);
		}

		if ( hasdatarefSecondaries )
		{
			DrawAlphaBitmap(pDC,datarefYellowPoint,&m_datarefYellowBitmap,rect,pTooltip,pWnd,&refYellowSecondaryPoint);
			DrawAlphaBitmap(pDC,datarefCyanPoint,&m_datarefCyanBitmap,rect,pTooltip,pWnd,&refCyanSecondaryPoint);
			DrawAlphaBitmap(pDC,datarefMagentaPoint,&m_datarefMagentaBitmap,rect,pTooltip,pWnd,&refMagentaSecondaryPoint);
		}
	}

	if(m_doShowGrayScale)
	{
		BOOL bIRE = pDoc->GetMeasure()->m_bIREScaleMode;
		int nSize = pDoc->GetMeasure()->GetGrayScaleSize();
		
		double YWhiteGray = YWhite;
		if ( nSize > 0 )
			YWhiteGray = pDoc -> GetMeasure () -> GetGray ( nSize - 1 ) [ 1 ];
		
		
		CColor GrayClr;
		
		double Gamma, Offset;
		pDoc->ComputeGammaAndOffset(&Gamma, &Offset, 3, 1, nSize, false);

		for(int i=0;i<nSize;i++)
		{
			CString str;
			Msg.LoadString ( IDS_GRAYIRE );
			str.Format(Msg,i*100/(pDoc->GetMeasure()->GetGrayScaleSize()-1));
            ColorXYZ aColor(pDoc->GetMeasure()->GetGray(i).GetXYZValue());
            ColorXYZ refColor(GetColorReference().GetWhite());
            double valy;

            // Determine Reference Y luminance for Delta E calculus
            if ( GetConfig ()->m_dE_gray > 0 || GetConfig ()->m_dE_form == 5 )
            {
				double x = ArrayIndexToGrayLevel ( i, nSize, GetConfig () -> m_bUseRoundDown );
    			CColor White = pDoc -> GetMeasure () -> GetGray ( nSize - 1 );
	    		CColor Black = pDoc -> GetMeasure () -> GetGray ( 0 );
                if (GetConfig()->m_GammaOffsetType == 4 && White.isValid() && Black.isValid() )
			    {
				   double valx = GrayLevelToGrayProp(x, GetConfig () -> m_bUseRoundDown);
                   valy = GetBT1886(valx, White, Black, GetConfig()->m_GammaRel, GetConfig()->m_Split);
			    }
			    else
			    {
				   double valx=(GrayLevelToGrayProp(x, GetConfig () -> m_bUseRoundDown)+Offset)/(1.0+Offset);
				   valy=pow(valx, GetConfig()->m_useMeasuredGamma?(GetConfig()->m_GammaAvg):(GetConfig()->m_GammaRef));
			    }

                ColorxyY tmpColor(GetColorReference().GetWhite());
                tmpColor[2] = valy;
                if ( GetConfig ()->m_dE_gray == 2 || GetConfig ()->m_dE_form == 5 )
                    tmpColor[2] = aColor [ 1 ] / YWhite;
                refColor = ColorXYZ(tmpColor);
            }
            else
            {
                // Use actual gray luminance as correct reference (absolute) 
                    YWhiteGray = aColor [ 1 ];
            }

            CCIEGraphPoint grayRef(refColor, 1.0,"", m_bCIEuv);

            CCIEGraphPoint grayPoint(pDoc->GetMeasure()->GetGray(i).GetXYZValue(), YWhiteGray, str, m_bCIEuv);

            DrawAlphaBitmap(pDC,grayPoint,&m_grayPlotBitmap,rect,pTooltip,pWnd,&grayRef);
		}
	}

	if(m_doShowSaturationScaleTarg) 
	{
		CString str;
		for(int i=0;i<pDoc->GetMeasure()->GetSaturationSize();i++)
		{

			if (i != 0)
			{
			Msg.LoadString ( IDS_REDSATPERCENTREF );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint RedPointRef(pDoc->GetMeasure()->GetRefSat(0, (double)i / (double)(pDoc->GetMeasure()->GetSaturationSize()-1), (GetConfig()->m_colorStandard==HDTVa||GetConfig()->m_colorStandard==HDTVb)).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,RedPointRef,&m_redSatRefBitmap,rect,pTooltip,pWnd);
			Msg.LoadString ( IDS_GREENSATPERCENTREF );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint GreenPointRef(pDoc->GetMeasure()->GetRefSat(1, (double)i / (double)(pDoc->GetMeasure()->GetSaturationSize()-1), (GetConfig()->m_colorStandard==HDTVa||GetConfig()->m_colorStandard==HDTVb)).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,GreenPointRef,&m_greenSatRefBitmap,rect,pTooltip,pWnd);
						Msg.LoadString ( IDS_BLUESATPERCENTREF );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint BluePointRef(pDoc->GetMeasure()->GetRefSat(2, (double)i / (double)(pDoc->GetMeasure()->GetSaturationSize()-1), (GetConfig()->m_colorStandard==HDTVa||GetConfig()->m_colorStandard==HDTVb)).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,BluePointRef,&m_blueSatRefBitmap,rect,pTooltip,pWnd);
			Msg.LoadString ( IDS_YELLOWSATPERCENTREF );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint YellowPointRef(pDoc->GetMeasure()->GetRefSat(3, (double)i / (double)(pDoc->GetMeasure()->GetSaturationSize()-1), (GetConfig()->m_colorStandard==HDTVa||GetConfig()->m_colorStandard==HDTVb)).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,YellowPointRef,&m_yellowSatRefBitmap,rect,pTooltip,pWnd);
			Msg.LoadString ( IDS_CYANSATPERCENTREF );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint CyanPointRef(pDoc->GetMeasure()->GetRefSat(4, (double)i / (double)(pDoc->GetMeasure()->GetSaturationSize()-1), (GetConfig()->m_colorStandard==HDTVa||GetConfig()->m_colorStandard==HDTVb)).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,CyanPointRef,&m_cyanSatRefBitmap,rect,pTooltip,pWnd);
			Msg.LoadString ( IDS_MAGENTASATPERCENTREF );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint MagentaPointRef(pDoc->GetMeasure()->GetRefSat(5, (double)i / (double)(pDoc->GetMeasure()->GetSaturationSize()-1), (GetConfig()->m_colorStandard==HDTVa||GetConfig()->m_colorStandard==HDTVb)).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,MagentaPointRef,&m_magentaSatRefBitmap,rect,pTooltip,pWnd);
			}

		}
	}
	if(m_doShowCCScaleTarg) 
	{
		CString str;
		BOOL isExtPat =( GetConfig()->m_CCMode == USER || GetConfig()->m_CCMode == CM10SAT || GetConfig()->m_CCMode == CM10SAT75 || GetConfig()->m_CCMode == CM5SAT || GetConfig()->m_CCMode == CM5SAT75 || GetConfig()->m_CCMode == CM4SAT || GetConfig()->m_CCMode == CM4SAT75 || GetConfig()->m_CCMode == CM4LUM || GetConfig()->m_CCMode == CM5LUM || GetConfig()->m_CCMode == CM10LUM || GetConfig()->m_CCMode == RANDOM250 || GetConfig()->m_CCMode == RANDOM500 || GetConfig()->m_CCMode == CM6NB || GetConfig()->m_CCMode == CMDNR);
        if (GetConfig()->m_CCMode != CCSG && !isExtPat && GetConfig()->m_CCMode != CMS && GetConfig()->m_CCMode != CPS && GetConfig()->m_CCMode != AXIS)
        {
			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_1a:(GetConfig()->m_CCMode == SKIN?IDS_CC_1b:IDS_CC_1) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc1Point(pDoc->GetMeasure()->GetRefCC24Sat(0).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc1Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);
		
			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_2a:(GetConfig()->m_CCMode == SKIN?IDS_CC_2b:IDS_CC_2) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc2Point(pDoc->GetMeasure()->GetRefCC24Sat(1).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc2Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_3a:(GetConfig()->m_CCMode == SKIN?IDS_CC_3b:IDS_CC_3) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc3Point(pDoc->GetMeasure()->GetRefCC24Sat(2).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc3Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_4a:(GetConfig()->m_CCMode == SKIN?IDS_CC_4b:IDS_CC_4) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc4Point(pDoc->GetMeasure()->GetRefCC24Sat(3).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc4Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_5a:(GetConfig()->m_CCMode == SKIN?IDS_CC_5b:IDS_CC_5) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc5Point(pDoc->GetMeasure()->GetRefCC24Sat(4).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc5Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_6a:(GetConfig()->m_CCMode == SKIN?IDS_CC_6b:IDS_CC_6) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc6Point(pDoc->GetMeasure()->GetRefCC24Sat(5).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc6Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_7a:(GetConfig()->m_CCMode == SKIN?IDS_CC_7b:IDS_CC_7) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc7Point(pDoc->GetMeasure()->GetRefCC24Sat(6).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc7Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_8a:(GetConfig()->m_CCMode == SKIN?IDS_CC_8b:IDS_CC_8) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc8Point(pDoc->GetMeasure()->GetRefCC24Sat(7).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc8Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_9a:(GetConfig()->m_CCMode == SKIN?IDS_CC_9b:IDS_CC_9) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc9Point(pDoc->GetMeasure()->GetRefCC24Sat(8).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc9Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_10a:(GetConfig()->m_CCMode == SKIN?IDS_CC_10b:IDS_CC_10) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc10Point(pDoc->GetMeasure()->GetRefCC24Sat(9).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc10Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_11a:(GetConfig()->m_CCMode == SKIN?IDS_CC_11b:IDS_CC_11) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc11Point(pDoc->GetMeasure()->GetRefCC24Sat(10).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc11Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_12a:(GetConfig()->m_CCMode == SKIN?IDS_CC_12b:IDS_CC_12) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc12Point(pDoc->GetMeasure()->GetRefCC24Sat(11).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc12Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_13a:(GetConfig()->m_CCMode == SKIN?IDS_CC_13b:IDS_CC_13) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc13Point(pDoc->GetMeasure()->GetRefCC24Sat(12).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc13Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_14a:(GetConfig()->m_CCMode == SKIN?IDS_CC_14b:IDS_CC_14) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc14Point(pDoc->GetMeasure()->GetRefCC24Sat(13).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc14Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_15a:(GetConfig()->m_CCMode == SKIN?IDS_CC_15b:IDS_CC_15) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc15Point(pDoc->GetMeasure()->GetRefCC24Sat(14).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc15Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_16a:(GetConfig()->m_CCMode == SKIN?IDS_CC_16b:IDS_CC_16) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc16Point(pDoc->GetMeasure()->GetRefCC24Sat(15).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc16Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_17a:(GetConfig()->m_CCMode == SKIN?IDS_CC_17b:IDS_CC_17) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc17Point(pDoc->GetMeasure()->GetRefCC24Sat(16).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc17Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_18a:(GetConfig()->m_CCMode == SKIN?IDS_CC_18b:IDS_CC_18) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc18Point(pDoc->GetMeasure()->GetRefCC24Sat(17).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc18Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_19a:(GetConfig()->m_CCMode == SKIN?IDS_CC_19b:IDS_CC_19) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc19Point(pDoc->GetMeasure()->GetRefCC24Sat(18).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc19Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_20a:(GetConfig()->m_CCMode == SKIN?IDS_CC_20b:IDS_CC_20) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc20Point(pDoc->GetMeasure()->GetRefCC24Sat(19).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc20Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_21a:(GetConfig()->m_CCMode == SKIN?IDS_CC_21b:IDS_CC_21) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc21Point(pDoc->GetMeasure()->GetRefCC24Sat(20).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc21Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_22a:(GetConfig()->m_CCMode == SKIN?IDS_CC_22b:IDS_CC_22) );
			str.Format(Msg, 10);			
			CCIEGraphPoint cc22Point(pDoc->GetMeasure()->GetRefCC24Sat(21).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc22Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);
			
			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_23a:(GetConfig()->m_CCMode == SKIN?IDS_CC_23b:IDS_CC_23) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc23Point(pDoc->GetMeasure()->GetRefCC24Sat(22).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc23Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_24a:(GetConfig()->m_CCMode == SKIN?IDS_CC_24b:IDS_CC_24) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc24Point(pDoc->GetMeasure()->GetRefCC24Sat(23).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc24Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);
            }
            else
            {
				CCPatterns ccPat = GetConfig()->m_CCMode;
                if (ccPat == CCSG || ccPat == CMS || ccPat == CPS || ccPat == AXIS )
                {
                    for (int i=0; i < (ccPat==CCSG?96:(ccPat==AXIS?71:19)); i++)
                    {
            			Msg.SetString ( ccPat==CCSG?PatName[i]:(ccPat==CMS?PatNameCMS[i]:(ccPat==CPS?PatNameCPS[i]:PatNameAXIS[i])) );
	            		str.Format(Msg, 10);
		            	CCIEGraphPoint cc24Point(pDoc->GetMeasure()->GetRefCC24Sat(i).GetXYZValue(),
			        					  YWhite,
				        				  str, m_bCIEuv);
			            DrawAlphaBitmap(pDC,cc24Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);
                    }
                }
                else
                {
                    for (int i=0; i < GetConfig()->GetCColorsSize(); i++)
                    {
                        char aBuf[50];
						std::string name = GetConfig()->GetCColorsN(i);
						sprintf(aBuf,"Color %s Reference", name.c_str());
                        Msg.SetString(aBuf);
	        		    str.Format(Msg, 10);
		        	    CCIEGraphPoint cc24Point(pDoc->GetMeasure()->GetRefCC24Sat(i).GetXYZValue(),
			        					  YWhite,
				        				  str, m_bCIEuv);
			            DrawAlphaBitmap(pDC,cc24Point,&m_cc24SatRefBitmap,rect,pTooltip,pWnd);
                    }
                }

            }
	}

	if(m_doShowSaturationScale) 
	{
			CString str,cc24str;
		for(int i=0;i<pDoc->GetMeasure()->GetSaturationSize();i++)
		{

			Msg.LoadString ( IDS_REDSATPERCENT );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint RedPoint(pDoc->GetMeasure()->GetRedSat(i).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,RedPoint,&m_redPrimaryBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( IDS_GREENSATPERCENT );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint GreenPoint(pDoc->GetMeasure()->GetGreenSat(i).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,GreenPoint,&m_greenPrimaryBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( IDS_BLUESATPERCENT );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint BluePoint(pDoc->GetMeasure()->GetBlueSat(i).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,BluePoint,&m_bluePrimaryBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( IDS_YELLOWSATPERCENT );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint YellowPoint(pDoc->GetMeasure()->GetYellowSat(i).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,YellowPoint,&m_yellowSecondaryBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( IDS_CYANSATPERCENT );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint CyanPoint(pDoc->GetMeasure()->GetCyanSat(i).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,CyanPoint,&m_cyanSecondaryBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( IDS_MAGENTASATPERCENT );
			str.Format(Msg, (i*100/(pDoc->GetMeasure()->GetSaturationSize()-1)));
			CCIEGraphPoint MagentaPoint(pDoc->GetMeasure()->GetMagentaSat(i).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,MagentaPoint,&m_magentaSecondaryBitmap,rect,pTooltip,pWnd);
		}
	}
	if(m_doShowCCScale)
	{
		CString str;
		BOOL isExtPat =( GetConfig()->m_CCMode == USER || GetConfig()->m_CCMode == CM10SAT || GetConfig()->m_CCMode == CM10SAT75 || GetConfig()->m_CCMode == CM5SAT || GetConfig()->m_CCMode == CM5SAT75 || GetConfig()->m_CCMode == CM4SAT || GetConfig()->m_CCMode == CM4SAT75 || GetConfig()->m_CCMode == CM4LUM || GetConfig()->m_CCMode == CM5LUM || GetConfig()->m_CCMode == CM10LUM || GetConfig()->m_CCMode == RANDOM250 || GetConfig()->m_CCMode == RANDOM500 || GetConfig()->m_CCMode == CM6NB || GetConfig()->m_CCMode == CMDNR);
        if (GetConfig()->m_CCMode != CCSG && !isExtPat && GetConfig()->m_CCMode != CMS && GetConfig()->m_CCMode != CPS && GetConfig()->m_CCMode != AXIS)
        {
			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_1a:(GetConfig()->m_CCMode == SKIN?IDS_CC_1b:IDS_CC_1) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc1Point(pDoc->GetMeasure()->GetCC24Sat(0).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc1Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_2a:(GetConfig()->m_CCMode == SKIN?IDS_CC_2b:IDS_CC_2) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc2Point(pDoc->GetMeasure()->GetCC24Sat(1).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc2Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_3a:(GetConfig()->m_CCMode == SKIN?IDS_CC_3b:IDS_CC_3) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc3Point(pDoc->GetMeasure()->GetCC24Sat(2).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc3Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_4a:(GetConfig()->m_CCMode == SKIN?IDS_CC_4b:IDS_CC_4) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc4Point(pDoc->GetMeasure()->GetCC24Sat(3).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc4Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_5a:(GetConfig()->m_CCMode == SKIN?IDS_CC_5b:IDS_CC_5) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc5Point(pDoc->GetMeasure()->GetCC24Sat(4).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc5Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_6a:(GetConfig()->m_CCMode == SKIN?IDS_CC_6b:IDS_CC_6) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc6Point(pDoc->GetMeasure()->GetCC24Sat(5).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc6Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_7a:(GetConfig()->m_CCMode == SKIN?IDS_CC_7b:IDS_CC_7) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc7Point(pDoc->GetMeasure()->GetCC24Sat(6).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc7Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_8a:(GetConfig()->m_CCMode == SKIN?IDS_CC_8b:IDS_CC_8) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc8Point(pDoc->GetMeasure()->GetCC24Sat(7).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc8Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_9a:(GetConfig()->m_CCMode == SKIN?IDS_CC_9b:IDS_CC_9) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc9Point(pDoc->GetMeasure()->GetCC24Sat(8).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc9Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_10a:(GetConfig()->m_CCMode == SKIN?IDS_CC_10b:IDS_CC_10) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc10Point(pDoc->GetMeasure()->GetCC24Sat(9).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc10Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_11a:(GetConfig()->m_CCMode == SKIN?IDS_CC_11b:IDS_CC_11) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc11Point(pDoc->GetMeasure()->GetCC24Sat(10).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc11Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_12a:(GetConfig()->m_CCMode == SKIN?IDS_CC_12b:IDS_CC_12) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc12Point(pDoc->GetMeasure()->GetCC24Sat(11).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc12Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_13a:(GetConfig()->m_CCMode == SKIN?IDS_CC_13b:IDS_CC_13) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc13Point(pDoc->GetMeasure()->GetCC24Sat(12).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc13Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_14a:(GetConfig()->m_CCMode == SKIN?IDS_CC_14b:IDS_CC_14) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc14Point(pDoc->GetMeasure()->GetCC24Sat(13).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc14Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_15a:(GetConfig()->m_CCMode == SKIN?IDS_CC_15b:IDS_CC_15) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc15Point(pDoc->GetMeasure()->GetCC24Sat(14).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc15Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_16a:(GetConfig()->m_CCMode == SKIN?IDS_CC_16b:IDS_CC_16) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc16Point(pDoc->GetMeasure()->GetCC24Sat(15).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc16Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_17a:(GetConfig()->m_CCMode == SKIN?IDS_CC_17b:IDS_CC_17) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc17Point(pDoc->GetMeasure()->GetCC24Sat(16).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc17Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_18a:(GetConfig()->m_CCMode == SKIN?IDS_CC_18b:IDS_CC_18) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc18Point(pDoc->GetMeasure()->GetCC24Sat(17).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc18Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_19a:(GetConfig()->m_CCMode == SKIN?IDS_CC_19b:IDS_CC_19) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc19Point(pDoc->GetMeasure()->GetCC24Sat(18).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc19Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_20a:(GetConfig()->m_CCMode == SKIN?IDS_CC_20b:IDS_CC_20) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc20Point(pDoc->GetMeasure()->GetCC24Sat(19).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc20Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_21a:(GetConfig()->m_CCMode == SKIN?IDS_CC_21b:IDS_CC_21) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc21Point(pDoc->GetMeasure()->GetCC24Sat(20).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc21Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_22a:(GetConfig()->m_CCMode == SKIN?IDS_CC_22b:IDS_CC_22) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc22Point(pDoc->GetMeasure()->GetCC24Sat(21).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc22Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_23a:(GetConfig()->m_CCMode == SKIN?IDS_CC_23b:IDS_CC_23) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc23Point(pDoc->GetMeasure()->GetCC24Sat(22).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc23Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);

			Msg.LoadString ( GetConfig()->m_CCMode == CMC?IDS_CC_24a:(GetConfig()->m_CCMode == SKIN?IDS_CC_24b:IDS_CC_24) );
			str.Format(Msg, 10);
			CCIEGraphPoint cc24Point(pDoc->GetMeasure()->GetCC24Sat(23).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,cc24Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);
         }
         else
         {
			 CCPatterns ccPat = GetConfig()->m_CCMode;
             if (ccPat == CCSG || ccPat == CMS || ccPat == CPS || ccPat == AXIS)
             {
	            for (int i = 0; i < (ccPat == CCSG?96:(ccPat==AXIS?71:19)); i++)
                {
          			Msg.SetString ( ccPat==CCSG?PatName[i]:(ccPat==CMS?PatNameCMS[i]:(ccPat==CPS?PatNameCPS[i]:PatNameAXIS[i])) );
	    	    	str.Format(Msg, 10);
		    	    CCIEGraphPoint cc24Point(pDoc->GetMeasure()->GetCC24Sat(i).GetXYZValue(),
			    					  YWhite,
				    				  str, m_bCIEuv);
			        DrawAlphaBitmap(pDC,cc24Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);
                }
             }
             else
             {
                    for (int i=0; i < GetConfig()->GetCColorsSize(); i++)
                    {
						char aBuf[50];
						std::string name = GetConfig()->GetCColorsN(i);
                        sprintf(aBuf,"Color %s", name.c_str());
                        Msg.SetString(aBuf);
	        		    str.Format(Msg, 10);
    		    	    CCIEGraphPoint cc24Point(pDoc->GetMeasure()->GetCC24Sat(i).GetXYZValue(),
			    					  YWhite,
				    				  str, m_bCIEuv);
	    		        DrawAlphaBitmap(pDC,cc24Point,&m_grayPlotBitmap,rect,pTooltip,pWnd);
                    }
             }
         }
	}

	if(m_doShowMeasurements)
		for(int i=max(0,pDoc->GetMeasure()->GetMeasurementsSize()-20);i<pDoc->GetMeasure()->GetMeasurementsSize();i++)
		{
			CString str;
			Msg.LoadString ( IDS_MEASURENUM );
			str.Format(Msg,i);
			CCIEGraphPoint measurePoint(pDoc->GetMeasure()->GetMeasurement(i).GetXYZValue(),
								  YWhite,
								  str, m_bCIEuv);
			DrawAlphaBitmap(pDC,measurePoint,&m_measurePlotBitmap,rect,pTooltip,pWnd);
		}

	if ( pDoc->m_SelectedColor.isValid())
	{
		Msg.LoadString ( IDS_SELECTION );
		CCIEGraphPoint measurePoint(pDoc->m_SelectedColor.GetXYZValue(),
								 YWhite,
								 Msg, m_bCIEuv);
		DrawAlphaBitmap(pDC,measurePoint,&m_selectedPlotBitmap,rect,pTooltip,pWnd);
	}
}

void CCIEChartGrapher::SaveGraphFile ( CDataSetDoc * pDoc, CSize ImageSize, LPCSTR lpszPathName, int ImageFormat, int ImageQuality, bool PDF )
{
	int				format;

	switch ( ImageFormat )
	{
		case 0: format = CXIMAGE_FORMAT_JPG; break;
		case 1: format = CXIMAGE_FORMAT_BMP; break;
		case 2: format = CXIMAGE_FORMAT_PNG; break;
		default: format = CXIMAGE_FORMAT_JPG; break;
	}

    CRect rect(0,0,ImageSize.cx,ImageSize.cy);

	CDC ScreenDC;
	ScreenDC.CreateDC ( "DISPLAY", NULL, NULL, NULL );

	CDC dc2;
    dc2.CreateCompatibleDC(&ScreenDC);

	CBitmap bitmap; 
    bitmap.CreateCompatibleBitmap(&ScreenDC,rect.Width(),rect.Height());

	ScreenDC.DeleteDC ();

    CBitmap *pOldBitmap=dc2.SelectObject(&bitmap);

	MakeBgBitmap(rect,GetConfig()->m_bWhiteBkgndOnFile && !PDF);
	DrawChart ( pDoc, & dc2, rect, NULL, NULL );
	dc2.SelectObject(pOldBitmap);

	CxImage *pImage = new CxImage();
	pImage->CreateFromHBITMAP(bitmap);

	if (pImage->IsValid())
	{
		pImage->SetJpegQuality(ImageQuality);
		pImage->Save(lpszPathName,format);
	}

	delete pImage;
}

/////////////////////////////////////////////////////////////////////////////
// CCIEChartView

IMPLEMENT_DYNCREATE(CCIEChartView, CSavingView)

CCIEChartView::CCIEChartView()
	: CSavingView()
{
	m_bDelayedUpdate = FALSE;
}

CCIEChartView::~CCIEChartView()
{
}

BEGIN_MESSAGE_MAP(CCIEChartView, CSavingView)
	//{{AFX_MSG_MAP(CCIEChartView)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWBACKGROUND, OnUpdateCieShowbackground)
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWDELTAE, OnUpdateCieShowDeltaE)
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWREFERENCES, OnUpdateCieShowreferences)
	ON_UPDATE_COMMAND_UI(IDM_LUM_GRAPH_DATAREF, OnUpdateCieGraphShowDataRef)
	ON_COMMAND(IDM_CIE_SHOWREFERENCES, OnCieShowreferences)
	ON_COMMAND(IDM_LUM_GRAPH_DATAREF, OnCieGraphShowDataRef)
	ON_COMMAND(IDM_CIE_SHOWBACKGROUND, OnCieShowbackground)
	ON_COMMAND(IDM_CIE_SHOWDELTAE, OnCieShowDeltaE)
	ON_COMMAND(IDM_CIE_SHOWGRAYSCALE, OnCieShowGrayScale)
	ON_COMMAND(IDM_CIE_SHOWSATURATIONSCALE, OnCieShowSaturationScale)
	ON_COMMAND(IDM_CIE_SHOWSATURATIONSCALETARG, OnCieShowSaturationScaleTarg)
	ON_COMMAND(IDM_CIE_SHOWCCSCALE, OnCieShowCCScale)
	ON_COMMAND(IDM_CIE_SHOWCCSCALETARG, OnCieShowCCScaleTarg)
	ON_COMMAND(IDM_CIE_SHOWMEASUREMENTS, OnCieShowMeasurements)
	ON_COMMAND(IDM_GRAPH_Y_ZOOM_IN, OnGraphZoomIn)
	ON_COMMAND(IDM_GRAPH_Y_ZOOM_OUT, OnGraphZoomOut)
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWMEASUREMENTS, OnUpdateCieShowMeasurements)
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWGRAYSCALE, OnUpdateCieShowGrayScale)
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWSATURATIONSCALE, OnUpdateCieShowSaturationScale)
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWSATURATIONSCALETARG, OnUpdateCieShowSaturationScaleTarg)
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWCCSCALE, OnUpdateCieShowCCScale)
	ON_UPDATE_COMMAND_UI(IDM_CIE_SHOWCCSCALETARG, OnUpdateCieShowCCScaleTarg)
	ON_COMMAND(IDM_CIE_SAVECHART, OnCieSavechart)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(IDM_HELP, OnHelp)
	ON_COMMAND(IDM_CIE_UV, OnCieUv)
	ON_UPDATE_COMMAND_UI(IDM_CIE_UV, OnUpdateCieUv)
	ON_WM_MOUSEWHEEL()
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCIEChartView diagnostics

#ifdef _DEBUG
void CCIEChartView::AssertValid() const
{
	CSavingView::AssertValid();
}

void CCIEChartView::Dump(CDumpContext& dc) const
{
	CSavingView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCIEChartView message handlers

void CCIEChartView::OnInitialUpdate() 
{
	m_tooltip.Create(this);	
	m_tooltip.SetBehaviour(PPTOOLTIP_MULTIPLE_SHOW);
	m_tooltip.SetNotify(TRUE);
}

void CCIEChartView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CRect	Rect;
	
	// Do nothing when not concerned
	switch ( lHint )
	{
		case UPD_NEARBLACK:
		case UPD_NEARWHITE:
		case UPD_CONTRAST:
		case UPD_GENERATORCONFIG:
			 return;
	}

	if ( IsWindowVisible () )
	{
		m_bDelayedUpdate = FALSE;
		GetReferenceRect ( & Rect );
		m_Grapher.MakeBgBitmap(Rect,GetConfig()->m_bWhiteBkgndOnScreen);
		Invalidate(TRUE);
	}
	else
	{
		// CIE chart is inside CMultiFrame window and is currently hidden. Do not recompute bitmap
		m_bDelayedUpdate = TRUE;
	}
}

DWORD CCIEChartView::GetUserInfo ()
{
	return	( ( m_Grapher.m_doDisplayBackground		& 0x0001 )	<< 0 )
		  + ( ( m_Grapher.m_doDisplayDeltaERef		& 0x0001 )	<< 1 )
		  + ( ( m_Grapher.m_doShowReferences		& 0x0001 )	<< 2 )
		  + ( ( m_Grapher.m_doShowDataRef			& 0x0001 )	<< 3 )
		  + ( ( m_Grapher.m_doShowGrayScale			& 0x0001 )	<< 4 )
		  + ( ( m_Grapher.m_doShowSaturationScale	& 0x0001 )	<< 5 )
		  + ( ( m_Grapher.m_doShowSaturationScaleTarg	& 0x0001 )	<< 6 )
		  + ( ( m_Grapher.m_doShowCCScale	& 0x0001 )	<< 7 )
		  + ( ( m_Grapher.m_doShowCCScaleTarg	& 0x0001 )	<< 8 )
		  + ( ( m_Grapher.m_doShowMeasurements		& 0x0001 )	<< 9 )
		  + ( ( m_Grapher.m_bCIEuv					& 0x0001 )	<< 10 );
}

void CCIEChartView::SetUserInfo ( DWORD dwUserInfo )
{
	m_Grapher.m_doDisplayBackground		= ( dwUserInfo >> 0 ) & 0x0001;
	m_Grapher.m_doDisplayDeltaERef		= ( dwUserInfo >> 1 ) & 0x0001;
	m_Grapher.m_doShowReferences		= ( dwUserInfo >> 2 ) & 0x0001;
	m_Grapher.m_doShowDataRef			= ( dwUserInfo >> 3 ) & 0x0001;
	m_Grapher.m_doShowGrayScale			= ( dwUserInfo >> 4 ) & 0x0001;
	m_Grapher.m_doShowSaturationScale	= ( dwUserInfo >> 5 ) & 0x0001;
	m_Grapher.m_doShowSaturationScaleTarg	= ( dwUserInfo >> 6 ) & 0x0001;
	m_Grapher.m_doShowCCScale	= ( dwUserInfo >> 7 ) & 0x0001;
	m_Grapher.m_doShowCCScaleTarg	= ( dwUserInfo >> 8 ) & 0x0001;
	m_Grapher.m_doShowMeasurements		= ( dwUserInfo >> 9 ) & 0x0001;
	m_Grapher.m_bCIEuv					= ( dwUserInfo >> 10 ) & 0x0001;
	
	m_bDelayedUpdate = TRUE;
}

void CCIEChartView::OnDraw(CDC* pDC) 
{
	if ( m_bDelayedUpdate )
	{
		// Perform late update
		OnUpdate ( NULL, 0, NULL );
	}

	CRect rect, refrect;
	GetClientRect(&rect);
	GetReferenceRect(&refrect);

	CDC dcDraw;
	dcDraw.CreateCompatibleDC(pDC);
	CBitmap *pOldBitmap=dcDraw.SelectObject(&m_Grapher.m_drawBitmap);
	m_Grapher.DrawChart ( GetDocument (), & dcDraw, refrect, & m_tooltip, this );
	pDC->BitBlt(0,0,rect.Width(),rect.Height(),&dcDraw,-m_Grapher.m_DeltaX,-m_Grapher.m_DeltaY,SRCCOPY);
	dcDraw.SelectObject(pOldBitmap);

//	DrawChart(pDC);
}

void CCIEChartView::SaveChart() 
{
	CSaveGraphDialog dialog;

	if(dialog.DoModal()!=IDOK)
		return;

    CRect rect;
	CSize size;

	switch(dialog.m_sizeType)
	{
		case 0:
		    GetClientRect(&rect);
			size = CSize(rect.Width(),rect.Height());
			break;
		case 1:
			size = CSize(300,200);
			break;
		case 2:
			size = CSize(600,400);
			break;
		case 3:
			size = CSize(dialog.m_saveWidth,dialog.m_saveHeight);
			break;

	}
 
	char *defExt;
	char *filter;

	switch(dialog.m_fileType)
	{
		case 0:
			defExt="jpg";
			filter="Jpeg File (*.jpg)|*.jpg||";
			break;
		case 1:
			defExt="bmp";
			filter="Bitmap File (*.bmp)|*.bmp||";
			break;
		case 2:
			defExt="jpg";
			filter="Portable Network Graphic File (*.png)|*.png||";
			break;
	}

	CFileDialog fileSaveDialog( FALSE, defExt, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter );
	if(fileSaveDialog.DoModal()==IDOK)
	{
		m_Grapher.SaveGraphFile ( GetDocument (), size, fileSaveDialog.GetPathName(), dialog.m_fileType, dialog.m_jpegQuality );

		// Recompute BgBitmap to match client size
		CRect clientRect;
		GetReferenceRect(&clientRect);
		m_Grapher.MakeBgBitmap(clientRect,GetConfig()->m_bWhiteBkgndOnScreen);
	}
}

BOOL CCIEChartView::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

void CCIEChartView::OnSize(UINT nType, int cx, int cy) 
{
	if(cx && cy)
	{
		CRect ClientRect = CRect(CPoint(0,0),CSize(cx,cy));
		CRect RefRect;

		if ( m_Grapher.m_ZoomFactor > 1000 )
		{
			// Zoom is active: adjust deltaX and deltaY
			do
			{
				RefRect = CRect(CPoint(0,0),CSize(cx*m_Grapher.m_ZoomFactor/1000,cy*m_Grapher.m_ZoomFactor/1000));
				if ( m_Grapher.m_ZoomFactor >= 1200 && ( RefRect.Width () > 2000 || RefRect.Height () > 2000 ) )
					m_Grapher.m_ZoomFactor -= 200;
				else
					break;
			} while ( TRUE );

			if ( RefRect.right + m_Grapher.m_DeltaX < ClientRect.right )
				m_Grapher.m_DeltaX = ClientRect.right - RefRect.right;

			if ( RefRect.bottom + m_Grapher.m_DeltaY < ClientRect.bottom )
				m_Grapher.m_DeltaY = ClientRect.bottom - RefRect.bottom;
		}
		else
			RefRect = ClientRect;

		m_Grapher.MakeBgBitmap(RefRect,GetConfig()->m_bWhiteBkgndOnScreen);
	}
	Invalidate(FALSE);
}

void CCIEChartView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// load and display popup menu
	CNewMenu menu;
	menu.LoadMenu(IDR_CIE_MENU);
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup);
	
	pPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
		point.x, point.y, GetParent());
}

void CCIEChartView::OnUpdateCieShowbackground(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doDisplayBackground);
}

void CCIEChartView::OnUpdateCieShowDeltaE(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doDisplayDeltaERef);
}

void CCIEChartView::OnUpdateCieShowreferences(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doShowReferences);
}

void CCIEChartView::OnUpdateCieGraphShowDataRef(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doShowDataRef);
}

void CCIEChartView::OnUpdateCieShowGrayScale(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doShowGrayScale);
}

void CCIEChartView::OnUpdateCieShowSaturationScale(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doShowSaturationScale);
}

void CCIEChartView::OnUpdateCieShowSaturationScaleTarg(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doShowSaturationScaleTarg);
}

void CCIEChartView::OnUpdateCieShowCCScale(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doShowCCScale);
}

void CCIEChartView::OnUpdateCieShowCCScaleTarg(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doShowCCScaleTarg);
}

void CCIEChartView::OnUpdateCieShowMeasurements(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_doShowMeasurements);
}

void CCIEChartView::OnCieShowbackground() 
{
	m_Grapher.m_doDisplayBackground = !m_Grapher.m_doDisplayBackground;
	GetConfig()->WriteProfileInt("CIE Chart","Display Background",m_Grapher.m_doDisplayBackground);
	CRect rect;
	GetReferenceRect(&rect);
	m_Grapher.MakeBgBitmap(rect,GetConfig()->m_bWhiteBkgndOnScreen);
	Invalidate(TRUE);
}

void CCIEChartView::OnCieShowDeltaE() 
{
	m_Grapher.m_doDisplayDeltaERef = !m_Grapher.m_doDisplayDeltaERef;
	GetConfig()->WriteProfileInt("CIE Chart","Display Delta E",m_Grapher.m_doDisplayDeltaERef);
	CRect rect;
	GetReferenceRect(&rect);
	m_Grapher.MakeBgBitmap(rect,GetConfig()->m_bWhiteBkgndOnScreen);
	Invalidate(TRUE);
}

void CCIEChartView::OnCieShowreferences() 
{
	m_Grapher.m_doShowReferences = !m_Grapher.m_doShowReferences;
	GetConfig()->WriteProfileInt("CIE Chart","Show References",m_Grapher.m_doShowReferences);
	Invalidate(TRUE);
}

void CCIEChartView::OnCieGraphShowDataRef()
{
	m_Grapher.m_doShowDataRef = !m_Grapher.m_doShowDataRef;
	GetConfig()->WriteProfileInt("CIE Chart","Show Reference Data",m_Grapher.m_doShowDataRef);
	Invalidate(TRUE);
}


void CCIEChartView::OnCieShowGrayScale() 
{
	m_Grapher.m_doShowGrayScale = !m_Grapher.m_doShowGrayScale;
	GetConfig()->WriteProfileInt("CIE Chart","Display GrayScale",m_Grapher.m_doShowGrayScale);
	Invalidate(TRUE);
}

void CCIEChartView::OnCieShowSaturationScale() 
{
	m_Grapher.m_doShowSaturationScale = !m_Grapher.m_doShowSaturationScale;
	GetConfig()->WriteProfileInt("CIE Chart","Display Saturation Scale",m_Grapher.m_doShowSaturationScale);
	Invalidate(TRUE);
}

void CCIEChartView::OnCieShowSaturationScaleTarg() 
{
	m_Grapher.m_doShowSaturationScaleTarg = !m_Grapher.m_doShowSaturationScaleTarg;
	GetConfig()->WriteProfileInt("CIE Chart","Display Saturation Scale Targets",m_Grapher.m_doShowSaturationScaleTarg);
	Invalidate(TRUE);
}

void CCIEChartView::OnCieShowCCScale() 
{
	m_Grapher.m_doShowCCScale = !m_Grapher.m_doShowCCScale;
	GetConfig()->WriteProfileInt("CIE Chart","Display Color Checker measures",m_Grapher.m_doShowCCScale);
	Invalidate(TRUE);
}

void CCIEChartView::OnCieShowCCScaleTarg() 
{
	m_Grapher.m_doShowCCScaleTarg = !m_Grapher.m_doShowCCScaleTarg;
	GetConfig()->WriteProfileInt("CIE Chart","Display Color Checker Targets",m_Grapher.m_doShowCCScaleTarg);
	Invalidate(TRUE);
}

void CCIEChartView::OnCieShowMeasurements() 
{
	m_Grapher.m_doShowMeasurements = !m_Grapher.m_doShowMeasurements;
	GetConfig()->WriteProfileInt("CIE Chart","Show Measurements",m_Grapher.m_doShowMeasurements);
	Invalidate(TRUE);
}

void CCIEChartView::OnGraphZoomIn() 
{
	CRect	ClientRect, RefRect;
	GetClientRect ( & ClientRect );

	m_Grapher.m_ZoomFactor += 200;
	if ( m_Grapher.m_ZoomFactor > 4000 )
		m_Grapher.m_ZoomFactor = 4000;

	GetReferenceRect ( & RefRect );
	if ( m_Grapher.m_ZoomFactor >= 1200 && ( RefRect.Width () > 2000 || RefRect.Height () > 2000 ) )
	{
		m_Grapher.m_ZoomFactor -= 200;
		GetReferenceRect ( & RefRect );
	}

	if ( m_Grapher.m_DeltaX > 0 )
		m_Grapher.m_DeltaX = 0;
	else if ( m_Grapher.m_DeltaX < ClientRect.right - RefRect.right )
		m_Grapher.m_DeltaX = ClientRect.right - RefRect.right;

	if ( m_Grapher.m_DeltaY > 0 )
		m_Grapher.m_DeltaY = 0;
	else if ( m_Grapher.m_DeltaY < ClientRect.bottom - RefRect.bottom )
		m_Grapher.m_DeltaY = ClientRect.bottom - RefRect.bottom;

	m_Grapher.MakeBgBitmap(RefRect,GetConfig()->m_bWhiteBkgndOnScreen);
	
	Invalidate ( TRUE );
}

void CCIEChartView::OnGraphZoomOut() 
{
	CRect	ClientRect, RefRect;
	GetClientRect ( & ClientRect );

	m_Grapher.m_ZoomFactor -= 200;
	if ( m_Grapher.m_ZoomFactor < 1000 )
		m_Grapher.m_ZoomFactor = 1000;

	GetReferenceRect ( & RefRect );

	if ( m_Grapher.m_DeltaX > 0 )
		m_Grapher.m_DeltaX = 0;
	else if ( m_Grapher.m_DeltaX < ClientRect.right - RefRect.right )
		m_Grapher.m_DeltaX = ClientRect.right - RefRect.right;

	if ( m_Grapher.m_DeltaY > 0 )
		m_Grapher.m_DeltaY = 0;
	else if ( m_Grapher.m_DeltaY < ClientRect.bottom - RefRect.bottom )
		m_Grapher.m_DeltaY = ClientRect.bottom - RefRect.bottom;

	m_Grapher.MakeBgBitmap(RefRect,GetConfig()->m_bWhiteBkgndOnScreen);
	
	Invalidate ( TRUE );
}

void CCIEChartView::OnCieUv() 
{
	m_Grapher.m_bCIEuv = !m_Grapher.m_bCIEuv;
	GetConfig()->WriteProfileInt("CIE Chart","CIE uv mode",m_Grapher.m_bCIEuv);

	CRect rect;
	GetReferenceRect(&rect);
	m_Grapher.MakeBgBitmap(rect,GetConfig()->m_bWhiteBkgndOnScreen);
	Invalidate(TRUE);
}

void CCIEChartView::OnUpdateCieUv(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_Grapher.m_bCIEuv);
}

BOOL CCIEChartView::PreTranslateMessage(MSG* pMsg) 
{
	m_tooltip.RelayEvent(pMsg);
	
	return CSavingView::PreTranslateMessage(pMsg);
}

void CCIEChartView::OnCieSavechart() 
{
	SaveChart();
}

void CCIEChartView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	SetCapture ();
	m_CurMousePoint = point;
	UpdateTestColor ( point );
}

void CCIEChartView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if ( GetCapture () )
	{
		if ( m_Grapher.m_ZoomFactor > 1000 )
		{
			// Update 
			int		OldDeltaX = m_Grapher.m_DeltaX;
			int		OldDeltaY = m_Grapher.m_DeltaY;
			CRect	ClientRect, RefRect;

			GetClientRect ( & ClientRect );
			GetReferenceRect ( & RefRect );

			m_Grapher.m_DeltaX += point.x - m_CurMousePoint.x;
			if ( m_Grapher.m_DeltaX > 0 )
				m_Grapher.m_DeltaX = 0;
			else if ( m_Grapher.m_DeltaX < ClientRect.right - RefRect.right )
				m_Grapher.m_DeltaX = ClientRect.right - RefRect.right;

			m_Grapher.m_DeltaY += point.y - m_CurMousePoint.y;
			if ( m_Grapher.m_DeltaY > 0 )
				m_Grapher.m_DeltaY = 0;
			else if ( m_Grapher.m_DeltaY < ClientRect.bottom - RefRect.bottom )
				m_Grapher.m_DeltaY = ClientRect.bottom - RefRect.bottom;
			
			m_CurMousePoint = point;

			ScrollWindow ( m_Grapher.m_DeltaX - OldDeltaX, m_Grapher.m_DeltaY - OldDeltaY );
		}

		UpdateTestColor ( point );
		ReleaseCapture ();
	}
}

void CCIEChartView::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if ( GetCapture () )
	{
		if ( m_Grapher.m_ZoomFactor > 1000 )
		{
			// Update 
			int		OldDeltaX = m_Grapher.m_DeltaX;
			int		OldDeltaY = m_Grapher.m_DeltaY;
			CRect	ClientRect, RefRect;

			GetClientRect ( & ClientRect );
			GetReferenceRect ( & RefRect );

			m_Grapher.m_DeltaX += point.x - m_CurMousePoint.x;
			if ( m_Grapher.m_DeltaX > 0 )
				m_Grapher.m_DeltaX = 0;
			else if ( m_Grapher.m_DeltaX < ClientRect.right - RefRect.right )
				m_Grapher.m_DeltaX = ClientRect.right - RefRect.right;

			m_Grapher.m_DeltaY += point.y - m_CurMousePoint.y;
			if ( m_Grapher.m_DeltaY > 0 )
				m_Grapher.m_DeltaY = 0;
			else if ( m_Grapher.m_DeltaY < ClientRect.bottom - RefRect.bottom )
				m_Grapher.m_DeltaY = ClientRect.bottom - RefRect.bottom;
			
			m_CurMousePoint = point;

			ScrollWindow ( m_Grapher.m_DeltaX - OldDeltaX, m_Grapher.m_DeltaY - OldDeltaY );
		}

		UpdateTestColor ( point );
	}
}


BOOL CCIEChartView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	// TODO: Add your message handler code here and/or call default
	CRect	ClientRect, RefRect;
	GetClientRect ( & ClientRect );

	if ( zDelta < 0 )
	{
		m_Grapher.m_ZoomFactor += 200;
		if ( m_Grapher.m_ZoomFactor > 4000 )
			m_Grapher.m_ZoomFactor = 4000;

		GetReferenceRect ( & RefRect );
		if ( m_Grapher.m_ZoomFactor >= 1200 && ( RefRect.Width () > 2000 || RefRect.Height () > 2000 ) )
			m_Grapher.m_ZoomFactor -= 200;
	}
	else if ( zDelta > 0 )
	{
		m_Grapher.m_ZoomFactor -= 200;
		if ( m_Grapher.m_ZoomFactor < 1000 )
			m_Grapher.m_ZoomFactor = 1000;
	}

	GetReferenceRect ( & RefRect );

	if ( m_Grapher.m_DeltaX > 0 )
		m_Grapher.m_DeltaX = 0;
	else if ( m_Grapher.m_DeltaX < ClientRect.right - RefRect.right )
		m_Grapher.m_DeltaX = ClientRect.right - RefRect.right;

	if ( m_Grapher.m_DeltaY > 0 )
		m_Grapher.m_DeltaY = 0;
	else if ( m_Grapher.m_DeltaY < ClientRect.bottom - RefRect.bottom )
		m_Grapher.m_DeltaY = ClientRect.bottom - RefRect.bottom;

	m_Grapher.MakeBgBitmap(RefRect,GetConfig()->m_bWhiteBkgndOnScreen);
	
	Invalidate ( FALSE );

	return TRUE;
}

void CCIEChartView::UpdateTestColor ( CPoint point )
{
	int		nR = 0, nG = 0, nB = 0;
	CRect	rect;
	double	x, y;
	double	u, v;
	double	cmax;
	double	base, coef;
    double gamma = (GetConfig()->m_useMeasuredGamma)?(GetConfig()->m_GammaAvg):(GetConfig()->m_GammaRef);
    ColorRGB	RGBColor;

	GetReferenceRect ( & rect );

	x = (double)(point.x-m_Grapher.m_DeltaX) / (double)rect.Width() * (m_Grapher.m_bCIEuv ? 0.7 : 0.8);
	y = (double)(rect.bottom-(point.y-m_Grapher.m_DeltaY)) / (double)rect.Height() * (m_Grapher.m_bCIEuv ? 0.7 : 0.9);

	if ( m_Grapher.m_bCIEuv )
	{
		u = x;
		v = y;
		x = ( 9.0 * u ) / ( ( 6.0 * u ) - ( 16.0 * v ) + 12.0 );
		y = ( 4.0 * v ) / ( ( 6.0 * u ) - ( 16.0 * v ) + 12.0 );
	}

	if ( x > 0.0 && x < 1.0 && y > 0.0 && y < 1.0 )
	{
		CColor	ClickedColor ( x, y );
//        CColor White = GetDocument() -> GetMeasure () -> GetGray ( GetDocument()->GetMeasure()->GetGrayScaleSize() - 1 );
//	    CColor Black = GetDocument() -> GetMeasure () -> GetGray ( 0 );
		CColor White = GetDocument()->GetMeasure()->GetOnOffWhite();
		CColor Black = GetDocument()->GetMeasure()->GetOnOffBlack();

		RGBColor = ClickedColor.GetRGBValue ((GetColorReference()));
        double r=RGBColor[0],g=RGBColor[1],b=RGBColor[2];
        if (GetConfig()->m_GammaOffsetType == 4 && White.isValid() && Black.isValid()) 
        {
    		r = (r<=0||r>=1)?min(max(r,0),1):pow(r,log(GetBT1886(r, White, Black,GetConfig()->m_GammaRel, GetConfig()->m_Split))/log(r));
	    	g = (g<=0||g>=1)?min(max(g,0),1):pow(g,log(GetBT1886(g, White, Black,GetConfig()->m_GammaRel, GetConfig()->m_Split))/log(g));
		    b = (b<=0||b>=1)?min(max(b,0),1):pow(b,log(GetBT1886(b, White, Black,GetConfig()->m_GammaRel, GetConfig()->m_Split))/log(b));
        }
        else
        {
    		r = (r<=0||r>=1)?min(max(r,0),1):pow(r,gamma);
	    	g = (g<=0||g>=1)?min(max(g,0),1):pow(g,gamma);
		    b = (b<=0||b>=1)?min(max(b,0),1):pow(b,gamma);
        }

		cmax = max(r,g);
		if ( b>cmax)
			cmax=b;

		if(GetDocument()->GetGenerator()->m_b16_235)
		{
			base = 16.0;
			coef = 235.0 - 16.0;
		}
		else
		{
			base = 0.0;
			coef = 255.0;
		}

		nR = (int) (r/cmax*coef+base);
		nG = (int) (g/cmax*coef+base);
		nB = (int) (b/cmax*coef+base);
	}

	( (CMainFrame *) ( AfxGetApp () -> m_pMainWnd ) ) ->m_wndTestColorWnd.m_colorPicker.SetColor ( RGB(nR,nG,nB) );
	( (CMainFrame *) ( AfxGetApp () -> m_pMainWnd ) ) ->m_wndTestColorWnd.RedrawWindow ();
}

void CCIEChartView::GetReferenceRect ( LPRECT lpRect )
{
	GetClientRect ( lpRect );
	lpRect -> right = lpRect -> right * m_Grapher.m_ZoomFactor / 1000;
	lpRect -> bottom = lpRect -> bottom * m_Grapher.m_ZoomFactor / 1000;
}

void CCIEChartView::OnHelp() 
{
	GetConfig () -> DisplayHelp ( HID_CIECHART, NULL );
}



void CCIEChartView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	int		OldDeltaX = m_Grapher.m_DeltaX;
	int		OldDeltaY = m_Grapher.m_DeltaY;
	CRect	ClientRect, RefRect;

	GetClientRect ( & ClientRect );
	GetReferenceRect ( & RefRect );

	switch ( nChar )
	{
		case VK_UP:
			 m_Grapher.m_DeltaY += 10;
			 break;
		case VK_DOWN:
			 m_Grapher.m_DeltaY -= 10;
			 break;
		case VK_LEFT:
			 m_Grapher.m_DeltaX += 10;
			 break;
		case VK_RIGHT:
			 m_Grapher.m_DeltaX -= 10;
			 break;
	}

	if ( m_Grapher.m_DeltaX > 0 )
		m_Grapher.m_DeltaX = 0;
	else if ( m_Grapher.m_DeltaX < ClientRect.right - RefRect.right )
		m_Grapher.m_DeltaX = ClientRect.right - RefRect.right;

	if ( m_Grapher.m_DeltaY > 0 )
		m_Grapher.m_DeltaY = 0;
	else if ( m_Grapher.m_DeltaY < ClientRect.bottom - RefRect.bottom )
		m_Grapher.m_DeltaY = ClientRect.bottom - RefRect.bottom;
	
	ScrollWindow ( m_Grapher.m_DeltaX - OldDeltaX, m_Grapher.m_DeltaY - OldDeltaY );

	CSavingView::OnKeyDown(nChar, nRepCnt, nFlags);
}
