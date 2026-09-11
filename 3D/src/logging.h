#pragma once

#include <cstdarg>

// Custom raylib trace log callback: prefixes every log line with a
// timestamp and drops raylib's spammy per-frame "TEXT..." info logs.
void CustomTraceLog(int msgType, const char *text, va_list args);
