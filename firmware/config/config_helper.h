#ifndef CONFIG_HELPER_H
#define CONFIG_HELPER_H

// Single include point for the whole firmware. Preferences come from config.h,
// credentials from secrets.h. Both are gitignored; both have a .template
// beside them that IS committed.

// Release build (env:release): the downloadable .bin in the GitHub release.
// Built from the committed templates only, so nobody's own config.h or keys
// can end up in a public file; the keys are pasted on the settings page.
#ifdef ORBS_RELEASE_BUILD
    #include "config.h.template"
    #define WEATHER_API_KEY ""
#elif !__has_include("config.h")
    #error "Copy firmware/config/config.h.template to firmware/config/config.h and edit it to match your setup"
#else
    #include "config.h"
#endif

// The orb's network name: the mDNS name (<name>.local) and the DHCP hostname a
// router may list or publish in its own DNS. Shared by WifiWidget and OtaUpdater.
#ifndef OTA_HOSTNAME
    #define OTA_HOSTNAME "info-orbs"
#endif

#ifdef ORBS_RELEASE_BUILD
// keys defined above; no secrets.h, so no OTA password either
#elif !__has_include("secrets.h")
    #error "Copy firmware/config/secrets.h.template to firmware/config/secrets.h and paste in your own API keys"
#else
    #include "secrets.h"

    #if !defined(WEATHER_API_KEY)
        #error "secrets.h is missing WEATHER_API_KEY - free key at https://www.visualcrossing.com/sign-up"
    #endif
    // TIMEZONE_API_KEY is no longer used (2026-11-09: zones are a table in
    // firmware/src/core/settings/TimeZones.h). An old secrets.h that still
    // defines it builds fine; the value is ignored.

    // An empty string literal is 1 byte, so this catches a secrets.h that was
    // copied but never filled in - which otherwise only shows up as a blank
    // weather orb on the bench.
    //
    // C++ ONLY. platformio.ini force-includes this header into every translation
    // unit with -include, and that includes plain C files from libraries
    // (tjpgd.c, sd_diskio_crc.c). static_assert is a C++ keyword; unguarded it
    // is a syntax error there and the whole build dies in someone else's code.
    #if defined(__cplusplus) && defined(WEATHER_API_KEY)
static_assert(sizeof(WEATHER_API_KEY) > 1, "WEATHER_API_KEY is empty in firmware/config/secrets.h - paste your visualcrossing.com key");
    #endif
    // Optional, so only checked when present: an empty token would build and
    // then fail every fetch with HTTP 401
    #if defined(__cplusplus) && defined(TEMPEST_TOKEN)
static_assert(sizeof(TEMPEST_TOKEN) > 1, "TEMPEST_TOKEN is empty in firmware/config/secrets.h - delete the line, or paste a token from tempestwx.com > Settings > Data Authorizations");
    #endif
#endif

#endif
