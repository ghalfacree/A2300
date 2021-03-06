/** Name: ConfigDduc.cpp
*
* Copyright(c) 2013 Loctronix Corporation
* http://www.loctronix.com
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifdef WIN32
	#define NOMINMAX
#endif

#include <A2300/ConfigDduc.h>
#include <A2300/ConfigDevice.h>
#include <algorithm>

namespace A2300 {

ConfigDduc::ConfigDduc( byte idComponent, const std::string& sname, 
					   ConfigDevice* pDevice, bool bReset, double uiSamplingRateHz )
:   m_idComponent( idComponent), m_sName ( sname), m_pDevice(pDevice),
	m_bEnable(false), m_byteMode( Default), 
	m_uiSampRate( uiSamplingRateHz ), m_uiSamplingDivisor(16), 
	m_iPhaseRate(0)
{
	if( bReset) Reset( uiSamplingRateHz);
}

ConfigDduc::~ConfigDduc() 
{
	// TODO Auto-generated destructor stub
}

void ConfigDduc::Enable( bool bEnable)
{
	m_bEnable = bEnable;
	Mode( m_byteMode);
}

void ConfigDduc::Reset( double uiSamplingRateHz)
{
	m_uiSampRate		= uiSamplingRateHz;
	m_bEnable			= false;

	TransportDci& td = m_pDevice->Dci0Transport();
	td.SetProperty<byte>(m_idComponent, DSP_DDUC_CTRL, DSP_DDUC_CTRL_RESET);
	td.SetProperty<byte>(m_idComponent, DSP_DDUC_CTRL, DSP_DDUC_CTRL_DISABLED);
}

/** 
* Clears fifos and without changing configuration.
*/
void ConfigDduc::Clear()
{
	//Add Bypass Strobe flag if Bypass Cic specified.
	byte flags = (byte)( m_byteMode | ((m_byteMode & ((byte) BypassCic)) ? DSP_DDUC_CTRL_BYPASSCICSTROBE : (byte) 0));
	flags |= (byte)((m_bEnable) ? DSP_DDUC_CTRL_ENABLED : DSP_DDUC_CTRL_DISABLED);

	TransportDci& td = m_pDevice->Dci0Transport();
	td.SetProperty<byte>(m_idComponent, DSP_DDUC_CTRL,flags | DSP_DDUC_CTRL_RESET );
	td.SetProperty<byte>(m_idComponent, DSP_DDUC_CTRL,flags );

//	byte buff[DCI_MAX_MSGSIZE];
//	int 			   len   = Dci_TypedProperties_Init(buff, DCI_MAX_MSGSIZE, m_idComponent,  2, NULL);
//	Dci_Property* props = Dci_TypedProperties_GetProperties( (Dci_TypedProperties*) buff);
//	props[0].idtype 		= PT_BYTE;
//	props[0].idprop 		= DSP_DDUC_CTRL;
//	props[0].value.vByte 	= flags | DSP_DDUC_CTRL_RESET;
//
//	props[1].idtype 		= PT_BYTE;
//	props[1].idprop 		= DSP_DDUC_CTRL;
//	props[1].value.vByte 	= flags;
//
//	TransportDci& td = m_pDevice->Dci0Transport();
//	td.SendMsg(buff,len, false);
}


/**
* Sets thes DDUC operating mode.
*/
void ConfigDduc::Mode( byte modeFlags)
{
	m_byteMode = modeFlags;
	//Add Bypass Strobe flag if Bypass Cic specified.
	byte flags = (byte)(m_byteMode | ((m_byteMode & ((byte) BypassCic)) ? DSP_DDUC_CTRL_BYPASSCICSTROBE : (byte) 0));
	flags |= (byte) (m_bEnable ? DSP_DDUC_CTRL_ENABLED : DSP_DDUC_CTRL_DISABLED);

	TransportDci& td = m_pDevice->Dci0Transport();
	td.SetProperty<byte>(m_idComponent, DSP_DDUC_CTRL,flags );
}

/** 
* Sets the hose sampling rate which is a multiple of the Default sampling rate.
*/
double ConfigDduc::HostSamplingRate( double  dRateHz, bool bAutoSetMode )
{
	double dphase = m_uiSampRate / dRateHz;
	m_uiSamplingDivisor = (uint16)( dphase + 0.5);
	m_uiSamplingDivisor = std::max<uint16>( 1, std::min<uint16>( m_uiSamplingDivisor, 8192));
	
	//determine which half-band filters are activated
    uint16 divisor = m_uiSamplingDivisor;

	if (bAutoSetMode)
	{
	    byte flags;
	    byte mask = BypassCordic| BypassCic | BypassHalfband;

	    if( m_uiSamplingDivisor == 1)
		{
			flags = BypassCic | DSP_DDUC_CTRL_BYPASSCICSTROBE | BypassHalfband ;
		}
		else if( m_uiSamplingDivisor % 2 == 0)
		{
			flags = Normal;
			divisor /= 2;
		}
		else
			flags = BypassHalfband;

		m_byteMode = (byte)((m_byteMode & ~mask) | flags);
	}
	
	//Set the decimation rate and turn on halband filters on ASR-2300.
	TransportDci& td = m_pDevice->Dci0Transport();
	td.SetProperty<uint16>(m_idComponent, DSP_DDUC_SAMPRATE,  divisor);

    return HostSamplingRate();
}

double ConfigDduc::FrequencyOffset( double dOffsetHz)
{
	double dphase = dOffsetHz / m_uiSampRate * A2300_CORDIC_RES;
	m_iPhaseRate = (int32) (dphase + 0.5);
	TransportDci& td = m_pDevice->Dci0Transport();
	td.SetProperty<int32>(m_idComponent, DSP_DDUC_PHASERATE, m_iPhaseRate);
	return FrequencyOffset();
}

std::string ConfigDduc::name()
{
	return m_sName;
}
byte ConfigDduc::componentId()
{
	return m_idComponent;
}
void ConfigDduc::Reset()
{
	Reset( m_uiSampRate);
}
void ConfigDduc::Synch()
{
	//TODO
}

} /* namespace A2300 */
