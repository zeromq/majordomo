// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MDPWRAPPER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MDPWRAPPER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MDPWRAPPER_EXPORTS
#define MDPWRAPPER_API __declspec(dllexport)
#else
#define MDPWRAPPER_API __declspec(dllimport)
#endif

/*
class MDPWRAPPER_API Cmdpwrapper {
public:
	Cmdpwrapper(void);
	// TODO: add your methods here.
};

extern MDPWRAPPER_API int nmdpwrapper;
*/
#include <mdp.h>

//MDPWRAPPER_API int fnmdpwrapper(void);



MDPWRAPPER_API mdp_client_t * client_new (char *broker, int verbose);
MDPWRAPPER_API void client_destroy (mdp_client_t **self_p);
MDPWRAPPER_API void client_set_timeout (mdp_client_t *self, int timeout);
MDPWRAPPER_API int client_setsockopt (mdp_client_t *self, int option, const void *optval,    size_t optvallen);
MDPWRAPPER_API int client_getsockopt (mdp_client_t *self, int option, void *optval,    size_t *optvallen);
MDPWRAPPER_API void client_send_data (mdp_client_t *self, char *service, char *data, int size );
MDPWRAPPER_API void client_send_string (mdp_client_t *self, char *service, char *msg );
MDPWRAPPER_API void client_recv (mdp_client_t *self, char **service_p, char **response_p);
