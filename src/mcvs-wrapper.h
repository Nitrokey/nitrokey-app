#ifndef MCVS_H
#define MCVS_H


#ifdef _MSC_VER

#define STRCPY(dest, size, source)                          strcpy_s( (dest), (size), (source) );
#define STRNCPY(dest,dest_size,source,source_size)          strncpy_s( (dest), (dest_size), (source), (source_size) );
#define STRCAT(dest,dest_size,source)                       strcat_s ( (dest) , (dest_size), (source) );
#define SNPRINTF( dest, dest_size, format, ...)             sprintf_s ( (dest), (dest_size), (format) , ##__VA_ARGS__);


#define FOPEN(FP, file, options)                    \
    do {                                                \
        errno_t Err_t;                                  \
        Err_t = fopen_s ( &(FP) , (file), (options));   \
        if (Err_t != 0)                                 \
            (FP) = 0;                                   \
    }while(0);

#else

#define STRCPY(dest, size, source)          strncpy( (dest), (source), (size));
#define STRNCPY(dest, dest_size, source, source_size)          strncpy( (dest), (source), (dest_size));

/*
   #define STRNCPY(dest,dest_size,source,source_size) \ do{ \ if ( (source_size) < (dest_size)) { \ strncpy ((dest), (source), (source_size) ); \
   (dest)[ (source_size) ] = '\0'; \ } else { \ (void)0; \ } \ }while(0); */

#define STRCAT(dest,dest_size,source)                                           \
    do{                                                                             \
        if ( strlen((source)) <= (dest_size) - strlen((dest)) -1 ){                 \
            printf("%zu %zu %zu\n", strlen((source)), sizeof((dest)), strlen((dest))); \
            strcat ((dest), (source));                                              \
        } else {                                                                    \
            (void)0;                                                                \
        }                                                                           \
    }while(0);


#define SNPRINTF( dest , dest_size , format, ...)     snprintf ( (dest), (dest_size), (format) ,  ##__VA_ARGS__);


#define FOPEN(FP, file, options)                    \
    do{                                                 \
        (FP) = fopen ((file), (options));               \
        if (NULL == (FP))                               \
            (FP) = 0;                                   \
    }while(0);

#endif /* _MSC_VER */

#endif /* MCVS_H */
