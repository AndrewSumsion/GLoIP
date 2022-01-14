#ifndef GLOIP_TEST_H
#define GLOIP_TEST_H 1

#include <cstddef>

#define GL_APICALL extern "C"
#define GL_APIENTRY 

GL_APICALL void GL_APIENTRY printString(const char* str);
GL_APICALL void GL_APIENTRY getString(size_t maxSize, char* str);
GL_APICALL void GL_APIENTRY printIntegers(int numIntegers, const int* integers);
GL_APICALL float GL_APIENTRY randomFloat();
GL_APICALL double GL_APIENTRY randomDouble();
GL_APICALL int GL_APIENTRY randomInt();
GL_APICALL void GL_APIENTRY printDouble(double d);
GL_APICALL double GL_APIENTRY squareRoot(double d);
GL_APICALL int GL_APIENTRY getHash(int count, const char* buffer);
GL_APICALL int GL_APIENTRY getBalance(int uid);
GL_APICALL const char* GL_APIENTRY getProperty(int propID);

#endif // GLOIP_TEST_H