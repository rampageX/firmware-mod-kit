#ifndef __BINTREE4BMAIN__H
#define __BINTREE4BMAIN__H

#undef BT_CLSID
#define BT_CLSID CLSID_CMatchFinderBT4b

#undef BT_NAMESPACE
#define BT_NAMESPACE NBT4b

#define HASH_ARRAY_2
#define HASH_ARRAY_3
#define HASH_BIG

#include "BinTreeMFMain.h"

#undef HASH_ARRAY_2
#undef HASH_ARRAY_3
#undef HASH_BIG

#endif

