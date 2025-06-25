/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2000-2002 Fritz Elfert <felfert@to.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */
#ifndef _PSITIME_H_
#define _PSITIME_H_

#include "config.h"

#include <iostream>
#include <cstdint>

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include <plpintl.h>

/**
 * Holds a Psion time value.
 * Psion time values are 64 bit
 * integers describing the time
 * since 01.01.0001 in microseconds.
 */
typedef struct psi_timeval_t {
    /**
     * Prints a psi_timeval in human readable format.
     */
    friend std::ostream &operator<<(std::ostream &o, const psi_timeval_t &ptv) {
	std::ostream::fmtflags old = o.flags();
	uint64_t micro = ptv.tv_high;
	micro = (micro << 32) | ptv.tv_low;
	micro /= 1000000;
	int s = micro % 60;
	micro /= 60;
	int m = micro % 60;
	micro /= 60;
	int h = micro % 24;
	micro /= 24;
	int d = micro % 365;
	micro /= 365;
	int y = micro;
	o << std::dec;
	if (y > 0)
	    o << y << ((y > 1) ? _(" years ") : _(" year "));
	if (d > 0)
	    o << d << ((d > 1) ? _(" days ") : _(" day "));
	if (h > 0)
	    o << h << ((h != 1) ? _(" hours ") : _(" hour "));
	if (m > 0)
	    o << m << ((m != 1) ? _(" minutes ") : _(" minute "));
	o << s << ((s != 1) ? _(" seconds") : _(" second"));
	o.flags(old);
	return o;
    }
    /**
    * The lower 32 bits
    */
    uint32_t tv_low;
    /**
    * The upper 32 bits
    */
    uint32_t tv_high;
} psi_timeval;

/**
 * holds a Psion time zone description.
 */
typedef struct psi_timezone_t {
    friend std::ostream &operator<<(std::ostream &s, const psi_timezone_t &ptz) {
	std::ostream::fmtflags old = s.flags();
	int h = ptz.utc_offset / 3600;
	int m = ptz.utc_offset % 3600;
	s << "offs: " << std::dec << h << "h";
	if (m != 0)
	    s << ", " << m << "m";
	s.flags(old);
	return s;
    }
    signed long   utc_offset;
    unsigned long dst_zones;
    unsigned long home_zone;
} psi_timezone;

/**
 * Psion time related utility class.
 *
 * PsiTime provides easy access to the time format, used
 * when communicating with a Psion. Internally, the time
 * is always normalized to GMT. The time value can be set
 * and retrieved in both Unix and Psion formats. This
 * allows easy conversion between both formats.
 * NOTE: For proper conversion, the current timezone of
 * the Psion has to be set. For EPOC devices, the
 * timezone can be evaluated using
 * @ref rpcs::getMachineInfo . For SIBO devices,
 * unfortunately there is no known method of retrieving
 * this information. Therefore, if the timezone is
 * <em>not</em> set, a fallback using the environment
 * variable <em>PSI_TZ</em> is provided. Users should
 * set this variable to the offset of their time zone
 * in seconds. If <em>PSI_TZ</em> is net set, a second
 * fallback uses the local machine's setup, which assumes
 * that both Psion and local machine have the same
 * time zone and daylight settings.
 *
 * @author Fritz Elfert <felfert@to.com>
 */
class PsiTime {
public:
    /**
    * Contructs a new instance.
    *
    * @param _utv A Unix time value for initialization.
    * @param _utz A Unix timezone for initialization.
    */
    PsiTime(struct timeval *_utv, struct timezone *_utz = nullptr);

    /**
    * Contructs a new instance.
    *
    * @param time A Unix time value for initialization.
    */
    PsiTime(time_t time);

    /**
    * Contructs a new instance.
    *
    * @param _ptv A Psion time value for initialization.
    * @param _ptz A Psion timezone for initialization.
    */
    PsiTime(psi_timeval *_ptv, psi_timezone *_ptz = nullptr);

    /**
    * Contructs a new instance.
    *
    * @param _ptvHi The high 32 bits of a Psion time value for initialization.
    * @param _ptvLo The low 32 bits of a Psion time value for initialization.
    */
    PsiTime(const uint32_t _ptvHi, const uint32_t _ptvLo);

    /**
    * Constructs a new instance, initializing to now.
    */
    PsiTime(void);

    /**
    * A copy-constructor
    */
    PsiTime(const PsiTime &t);

    /**
    * Destroys the instance.
    */
    ~PsiTime();

    /**
    * Modifies the value of this instance.
    *
    * @param _ptv The new Psion time representation.
    */
    void setPsiTime(psi_timeval *_ptv);

    /**
    * Modifies the value of this instance.
    *
    * @param stime The new SIBO time representation.
    */
    void setSiboTime(uint32_t stime);

    /**
    * Modifies the value of this instance.
    *
    * @param _ptvHi The high 32 bits of a Psion time.
    * @param _ptvLo The low 32 bits of a Psion time.
    */
    void setPsiTime(const uint32_t _ptvHi, const uint32_t _ptvLo);

    /**
    * Sets the Psion time zone of this instance.
    *
    * @param _ptz The new Psion time zone.
    */
    void setPsiZone(psi_timezone *_ptz);

    /**
    * Sets the value of this instance.
    *
    * @param _utv The new Unix time representation.
    */
    void setUnixTime(struct timeval *_utv);

    /**
    * Sets the value of this instance.
    *
    * @param _utv The new Unix time representation.
    */
    void setUnixTime(time_t time);

    /**
    * Sets the value of this instance to the
    * current time of the Unix machine.
    */
    void setUnixNow(void);

    /**
    * Retrieves the instance's current value
    * in Unix time format.
    *
    * @returns The instance's current time as Unix struct timeval.
    */
    struct timeval &getTimeval(void);

    /**
    * Retrieves the instance's current value
    * in Unix time format.
    *
    * @returns The instance's current time as Unix time_t.
    */
    time_t getTime(void);

    /**
    * Retrieves the instance's current value
    * in SIBO time format.
    *
    * @returns The instance's current time as SIBO time.
    */
    uint32_t getSiboTime();

    /**
    * Retrieves the instance's current value
    * in Psion time format.
    *
    * @returns The instance's current time a Psion struct psi_timeval_t.
    */
    psi_timeval &getPsiTimeval(void);

    /**
    * Retrieves the instance's current value
    * in Psion time format, high 32 bits.
    *
    * @returns The instance's current time as lower 32 bits of
    * a Psion struct psi_timeval_t.
    */
    uint32_t getPsiTimeLo(void);

    /**
    * Retrieves the instance's current value
    * in Psion time format, low 32 bits.
    *
    * @returns The instance's current time as upper 32 bits of
    * a Psion struct psi_timeval_t.
    */
    uint32_t getPsiTimeHi(void);

    /**
    * Prints the instance's value in human readable format.
    * This function uses the current locale setting for
    * formatting the time.
    *
    * @param s The stream to be written.
    * @param t The instance whose value should be displayed.
    *
    * @returns The stream.
    */
    friend std::ostream &operator<<(std::ostream &s, const PsiTime &t);

    /**
    * Assignment operator
    */
    PsiTime &operator=(const PsiTime &t);

    /**
     * Comparison operators
     */
    bool operator==(const PsiTime &t);
    bool operator<(const PsiTime &t);
    bool operator>(const PsiTime &t);

    enum zone {
	PSI_TZ_NONE = 0,
	PSI_TZ_EUROPEAN = 1,
	PSI_TZ_NORTHERN = 2,
	PSI_TZ_SOUTHERN = 4,
	PSI_TZ_HOME = 0x40000000
    };

private:
    void psi2unix(void);
    void unix2psi(void);
    void tryPsiZone();

    psi_timeval ptv;
    psi_timezone ptz;
    struct timeval utv;
    struct timezone utz;
    bool ptzValid;
};

/**
 * A singleton wrapper for a @ref psi_timezone . This class is used
 * by @ref PsiTime to initialize its psi_timezone variable.
 * PsiZone itself is initialized from within @ref rpcs::getMachineInfo .
 * In an application, you typically call this at the very beginning, just
 * after connection setup. From then on, a single PsiZone instance is
 * held in memory and used by the various constructors of PsiTime.
 *
 * @author Fritz Elfert <felfert@to.com>
 */
class PsiZone {
    friend class rpcs32;

public:
    /**
    * Retrieve the singleton object.
    * If it does not exist, it is created.
    */
    static PsiZone &getInstance();

    /**
    * Retrieve the Psion time zone.
    *
    * @param ptz The time zone is returned here.
    *
    * @returns false, if the time zone is not
    *          known (yet).
    */
    bool getZone(psi_timezone &ptz);

private:
    /**
    * This objects instance (singleton)
    */
    static PsiZone *_instance;

    /**
    * Private constructor.
    */
    PsiZone();

    void setZone(psi_timezone &ptz);

    bool _ptzValid;
    psi_timezone _ptz;
};
#endif
