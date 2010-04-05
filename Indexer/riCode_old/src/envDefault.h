/**
 * Environment defines
 * @author: anisio@dcc.ufmg.br
 */

#ifndef _ENV_DEFAULT_H__
#define _ENV_DEFAULT_H__

#include "zlib.h"

namespace RICPNS {

#define COMPRESSION_LEVEL Z_DEFAULT_COMPRESSION
#define CHUNK 262144 // 256K

#define MAX_FILE_SIZE 1610612736

#define DEBUG 1

#define USE_INPUT_BUFFER 1
#define INPUT_BUFFER_SIZE 20971520 // 20MB

#define MAX_STRING_SIZE 1024

#define MAX_PAGE_SIZE 3145728 // 3MB

} // namespace RICPNS

#endif


