/* NetHack 5.0  nhcfrg.r -- code-fragment resource for the classic PowerPC
 * (CFM) build.  The PEF container is the entire data fork
 * (kDataForkCFragLocator + kCFragGoesToEOF); the app's SIZE/heap partition
 * comes from nhsize.r, not from here. */
#include "CodeFragments.r"

#ifndef CFRAG_NAME
#define CFRAG_NAME "NetHack"
#endif

resource 'cfrg' (0) {
	{
		kPowerPCCFragArch, kIsCompleteCFrag, kNoVersionNum, kNoVersionNum,
		kDefaultStackSize, kNoAppSubFolder,
		kApplicationCFrag, kDataForkCFragLocator, kZeroOffset, kCFragGoesToEOF,
		CFRAG_NAME
	}
};
