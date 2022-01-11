#ifndef GLOIP_TEST_H
#define GLOIP_TEST_H 1

#define GL_APICALL extern "C"
#define GL_APIENTRY

GL_APICALL void GL_APIENTRY printString(const char* string);
GL_APICALL void GL_APIENTRY getString(char* string);
GL_APICALL void GL_APIENTRY printFloat(float f);
GL_APICALL void GL_APIENTRY printHex(int count, const char* buffer);
GL_APICALL int GL_APIENTRY getHash(int count, const char* buffer);
GL_APICALL int GL_APIENTRY getBalance(int uid);
GL_APICALL int GL_APIENTRY customFunction(const char*const* strings);

#endif // GLOIP_TEST_H