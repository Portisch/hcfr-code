/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005-2008 Association Homecinema Francophone.  All rights reserved.
// Copyright (c) 2012 Hcfr project.  All rights reserved.
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
//  Georges GALLERAND
//  John Adcock
/////////////////////////////////////////////////////////////////////////////

// ArgyllSensor.h: interface for the ArgyllSensor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARGYLLSENSOR_H__1493C213_6A02_44C5_8EB7_55B469092E14__INCLUDED_)
#define AFX_ARGYLLSENSOR_H__1493C213_6A02_44C5_8EB7_55B469092E14__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "OneDeviceSensor.h"
#include "ArgyllPropPage.h"
#include "ArgyllMeterWrapper.h"

class CArgyllSensor : public COneDeviceSensor  
{
public:
	DECLARE_SERIAL(CArgyllSensor) ;

protected:
	CArgyllSensorPropPage m_ArgyllSensorPropertiesPage;
 
public:
	UINT	m_DisplayMode;
	UINT	m_ReadingMode;
    UINT    m_portNumber;
	BOOL	m_debugMode;
	BOOL	m_bAverageReads;
	double	m_MinRequestedY;
	UINT	m_MaxReadLoops;

private:
    ArgyllMeterWrapper* m_meter;

public:
	CArgyllSensor();
	virtual ~CArgyllSensor();

	// Overriden functions from CSensor
	virtual	void Copy(CSensor * p);	
	virtual void Serialize(CArchive& archive); 

	virtual BOOL Init( BOOL bForSimultaneousMeasures );
	virtual CColor MeasureColor(COLORREF aRGBValue);
	virtual BOOL Release();

	virtual void SetPropertiesSheetValues();
	virtual void GetPropertiesSheetValues();

	virtual LPCSTR GetStandardSubDir ()	{ return ( GetConfig()->m_bUseCalibrationFilesOnAllProbes) ? "Calibration Argyll" : ""; }
	virtual BOOL SensorNeedCalibration () { return FALSE; }
	virtual BOOL SensorAcceptCalibration () { return FALSE; }

	virtual BOOL HasSpectrumCapabilities ( int * pNbBands, int * pMinWaveLength, int * pMaxWaveLength, int * pBandWidth );

	virtual void GetUniqueIdentifier( CString & strId );
};

#endif // !defined(AFX_ARGYLLSENSOR_H__1493C213_6A02_44C5_8EB7_55B469092E14__INCLUDED_)