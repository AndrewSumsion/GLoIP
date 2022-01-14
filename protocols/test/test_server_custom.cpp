#include <cstring>

void gloipRedirect_getProperty(bool* returnedSomething, uint8_t* returnSize, void* returnLocation, uint8_t numArgs, Argument** args) {
    GLOIP_CHECK_ARGS(TYPE_CUSTOM);

    *returnedSomething = false;
    *returnSize = 0;

    CustomArgument* arg = (CustomArgument*) args[0];
    int propID;
    memcpy(&propID, arg->data, 4);

    const char* str = getProperty(propID);
    int strLength = strlen(str) + 1;

    arg->reallocate(strLength);
    memcpy(arg->data, str, strLength);
}