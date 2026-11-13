#pragma once
// The device log on the orb: events kept in two small files on the same
// filesystem as the start-up picture, shown and downloaded at /log (linked
// from the settings page). DeviceLog.h has the rules and the privacy note:
//
//   NEVER put a network name, a password, an API key, a station ID, a street
//   address or a web address with a key in it into an event.

#include "DeviceLog.h"

class WebServer;

namespace DeviceLog {

// Once, right after the filesystem is mounted. Writes the start line: the
// firmware and why the orb started (power, a restart, a crash, a watchdog).
void begin(bool filesystemMounted);

// An event, printf style, from any task. Never touches the flash: the main
// loop writes it on its next pass. Keep it under 80 characters.
void note(const char *format, ...) __attribute__((format(printf, 1, 2)));

// Every pass of the main loop: writes what is waiting
void tick();

// The orb's local time, right after it was read. Dates the lines.
void clock(int64_t localEpoch);

// Before every ESP.restart(): says why and gets everything onto the flash
void beforeRestart(const char *why);

// One page's refresh result, after every refresh; logs only the changes
// (first failure, data stale, good again). `page` is a plain word such as
// "weather": never a host, a topic or a station.
void page(const char *page, devlog::PageWatch &watch, bool ok, bool stale);

// The whole log as plain text; download = offer it as a file to save
void send(WebServer &server, bool download);

} // namespace DeviceLog
