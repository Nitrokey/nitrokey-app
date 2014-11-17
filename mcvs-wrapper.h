#ifndef MCVS_H
#define MCVS_H


#define STRNCPY ( (dest) (dest_size) (source) (source_size) )   \
#ifdef _MSC_VER                                                 \
strncpy_s( (dest), (dest_size), (source), (source_size) );      \
#else                                                           \
strcpy ((dest), (source), (source_size) );                      \
#endif


#define STRCAT( (dest) , (dest_size) , (source) )   \
#ifdef _MSC_VER                                     \
strcat_s ( (dest) , (dest_size), (source) );        \
#else                                               \
do{
if ( strlen(DebugNewText) > sizeof(DebugText_Stick20) - strlen(DebugText_Stick20) -1 )
                return;
strcat (DebugText_Stick20, DebugNewText);                                          \
}while(0);


#endif

#define SNPRINTF( (dest) , (dest_size) , (format), ...)                     \
#ifdef _MSC_VER                                                             \
_snprintf_s ( (dest), (dest_size), (format) , __VA_ARGS__);     \
#else                                                                       \
snprintf ( (dest), (dest_size), (format) ,  __VA_ARGS__);      \
#endif

#define FOPEN( fp, file, options)                   \
do {                                                \
    #ifdef _MSC_VER                                 \
    errno_t Err_t;                                  \
    Err_t = fopen_s ( (fp) , (file), (options));    \
    (fp) = 0;                                       \
    #else                                           \
    (fp) = fopen ((file), (options));               \
    if (NULL == (fp))                               \
        (fp) = 0;                                   \
    #endif                                          \
}while(0);


#endif /* MCVS_H */
