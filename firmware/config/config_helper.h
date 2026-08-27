#ifndef CONFIG_HELPER_H
#define CONFIG_HELPER_H

// Single include point for the whole firmware. Preferences come from config.h,
// credentials from secrets.h. Both are gitignored; both have a .template
// beside them that IS committed.

#if !__has_include("config.h")
    #error "Copy firmware/config/config.h.template to firmware/config/config.h and edit it to match your setup"
#else
    #include "config.h"
#endif

#if !__has_include("secrets.h")
    #error "Copy firmware/config/secrets.h.template to firmware/config/secrets.h and paste in your own API keys"
#else
    #include "secrets.h"

    #if !defined(WEATHER_API_KEY)
        #error "secrets.h is missing WEATHER_API_KEY - free key at https://www.visualcrossing.com/sign-up"
    #endif
    #if !defined(TIMEZONE_API_KEY)
        #error "secrets.h is missing TIMEZONE_API_KEY - free key at https://timezonedb.com/register"
    #endif

    // An empty string literal is 1 byte, so this catches a secrets.h that was
    // copied but never filled in - which otherwise only shows up as a blank
    // weather orb on the bench.
    //
    // C++ ONLY. platformio.ini force-includes this header into every translation
    // unit with -include, and that includes plain C files from libraries
    // (tjpgd.c, sd_diskio_crc.c). static_assert is a C++ keyword; unguarded it
    // is a syntax error there and the whole build dies in someone else's code.
    #if defined(__cplusplus) && defined(WEATHER_API_KEY) && defined(TIMEZONE_API_KEY)
static_assert(sizeof(WEATHER_API_KEY) > 1, "WEATHER_API_KEY is empty in firmware/config/secrets.h - paste your visualcrossing.com key");
static_assert(sizeof(TIMEZONE_API_KEY) > 1, "TIMEZONE_API_KEY is empty in firmware/config/secrets.h - paste your timezonedb.com key");
    #endif
#endif

#endif
