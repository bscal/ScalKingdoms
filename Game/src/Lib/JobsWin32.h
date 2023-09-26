#pragma once

// Handles some additions Windows specific initialization. Raylib and Windows.h do not mix
void InitThread(void* handle, unsigned int threadID);