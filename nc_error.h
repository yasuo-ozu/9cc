#ifdef DEBUG
#define E_DEBUG(msg,...)	fprintf(stderr,msg "\n",__VA_ARGS__+0)
#else
#define E_DEBUG(msg,...)	
#endif

#define E_FATAL(msg,...)	fprintf(stderr,"fatal: " msg "\n",__VA_ARGS__+0)
