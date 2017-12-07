// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBATECARD_DLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBATECARD_DLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LIBATECARD_DLL_EXPORTS
#define LIBATECARD_DLL_API __declspec(dllexport)
#else
#define LIBATECARD_DLL_API __declspec(dllimport)
#endif

// This class is exported from the libatecard_dll.dll
class LIBATECARD_DLL_API Clibatecard_dll {
public:
	Clibatecard_dll(void);
	// TODO: add your methods here.
};

extern LIBATECARD_DLL_API int nlibatecard_dll;

LIBATECARD_DLL_API int fnlibatecard_dll(void);
