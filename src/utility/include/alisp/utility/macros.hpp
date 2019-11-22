#pragma once


#if (defined(__GNUC__) && __GNUC__ >= 3)
#define ALISP_UNLIKELY(x) (__builtin_expect(x, 0))
#define ALISP_LIKELY(x) (__builtin_expect(!!(x), 1))
#else
#define ALISP_UNLIKELY(x) (x)
#define ALISP_LIKELY(x) (x)
#endif


#define ALISP_DISALLOW_COPY_ASSIGN(TypeName)    \
    TypeName(const TypeName&) = delete;         \
    void operator=(const TypeName&) = delete

#define ALISP_DISALLOW_COPY_ASSIGN_MOVE(TypeName)   \
    TypeName(const TypeName&) = delete;             \
    TypeName(TypeName&&) = delete;                  \
    void operator=(const TypeName&) = delete;       \
    void operator=(TypeName&&) = delete




#if DEBUG_LOGGING
#define ALISP_HERE(message)																							\
	do {																																	\
		std::cout << "-> here() called in " << __FILE__ << " line " << __LINE__	\
							<< ". " << #message << std::endl;													\
	} while (0);
#else
#define ALISP_HERE(message) (void)0
#endif