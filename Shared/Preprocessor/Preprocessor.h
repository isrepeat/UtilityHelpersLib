#pragma once
#define PP_STRINGIFY_BASE(x) #x
#define PP_STRINGIFY(x) PP_STRINGIFY_BASE(x)


#if !defined(DISABLE_PREPROCESSOR_MESSAGES)
#ifdef __PROJECT_NAME__
#	define PREPROCESSOR_MSG(msg) "  PREPROCESSOR<" ## __PROJECT_NAME__ ">: \"" msg "\""
#else
#	define PREPROCESSOR_MSG(msg) "  PREPROCESSOR: \"" msg "\""
#endif
#else
#define PREPROCESSOR_MSG(msg) ""
#endif


#define DISABLE_PREPROCESSOR_FILE_INCLUDED
#if !defined(DISABLE_PREPROCESSOR_FILE_INCLUDED)
#define PREPROCESSOR_FILE_INCLUDED(file) PREPROCESSOR_MSG("Included '" file "'")
#else
#define PREPROCESSOR_FILE_INCLUDED(file) ""
#endif


#pragma message(PREPROCESSOR_FILE_INCLUDED("Preprocessor.h"))