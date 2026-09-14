#pragma once

// Windows API подключается через этот заголовок, чтобы его макросы и
// необязательные COM/OLE-зависимости не проникали в код приложения.
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NODRAWTEXT)
#define NODRAWTEXT
#endif
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <Windows.h>