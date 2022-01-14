GL_APICALL const char* GL_APIENTRY getProperty(int propID) {
    uint32_t functionHash = HASH_getProperty;

    bool waitForReturn = true;
    size_t returnSize = 0;

    CustomArgument arg1 = CustomArgument(4);
    memcpy(arg1.data, &propID, 4);

    Argument* args[] = {&arg1};

    gloip_execute(functionHash, waitForReturn, returnSize, nullptr, 1, args);

    char* heapStringBuffer = new char[arg1.size];
    memcpy(heapStringBuffer, arg1.data, arg1.size);

    return (const char*) heapStringBuffer;
}