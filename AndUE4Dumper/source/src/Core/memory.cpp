#include "memory.h"

#include <fcntl.h>
#include <dirent.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>

#include <Logger.h>


KittyMemoryMgr kMgr{};