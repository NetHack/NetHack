// Some shims for compatibility with different versions of LLVM and Clang

#ifndef COMPAT_H
#define COMPAT_H

#include <clang/Basic/Version.h>

namespace clang_compat {

	// The pointer type returned by CreateASTConsumer...
#if ((CLANG_VERSION_MAJOR<<8) + CLANG_VERSION_MINOR) >= 0x0306
	// ...requires a unique_ptr in 3.6 and later...
	typedef std::unique_ptr<clang::ASTConsumer> ASTConsumerPtr;
#else
	// ...and a bare pointer otherwise
	typedef clang::ASTConsumer *ASTConsumerPtr;
#endif

}

#endif // COMPAT_H
