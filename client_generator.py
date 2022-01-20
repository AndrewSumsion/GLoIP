from generator_common import parseHeaderFile, parseMetaFile, writeHeader

def writeFunction(file, name, function, meta):
    hashExpression = function["hashExpression"]
    argsString = function["argsString"]
    args = function["args"]
    retType = function["retType"]

    metaArgs = None
    if name in meta["functions"]:
        metaArgs = meta["functions"][name]
    
    if metaArgs != None and "custom" in metaArgs and metaArgs["custom"]:
        # don't generate code for a custom function, just provide its hash for the implementation
        file.write("#define HASH_" + name + " (" + hashExpression + ")\n")
        return

    declarationString = "GL_APICALL " + retType + " GL_APIENTRY " + name + "(" + argsString + ") {\n"
    file.write(declarationString)

    file.write("    uint32_t functionHash = " + hashExpression + ";\n\n")

    returnPrimitive = retType != "void"
    waitForReturn = returnPrimitive

    if not waitForReturn and metaArgs != None:
        # check if there are any blob return arguments
        for argName, argType in args:
            if argName in metaArgs and metaArgs[argName]["type"] == "return":
                waitForReturn = True
                break
    
    if metaArgs != None and "blocking" in metaArgs and metaArgs["blocking"]:
        waitForReturn = True

    file.write("    bool waitForReturn = " + ("true" if waitForReturn else "false") + ";\n")
    
    if returnPrimitive:
        file.write("    size_t returnSize = sizeof(" + retType + ");\n")
        file.write("    " + retType + " ret;\n\n")
    else:
        file.write("    size_t returnSize = 0;\n\n")
    
    argIndex = 1
    for argName, argType in args:
        argumentType = "PrimitiveArgument"
        if metaArgs != None and argName in metaArgs:
            if metaArgs[argName]["type"] == "blob":
                argumentType = "BlobArgument"
            if metaArgs[argName]["type"] == "return":
                argumentType = "BlobReturnArgument"
        
        argLine = "    " + argumentType + " arg" + str(argIndex) + " = " + argumentType

        if argumentType == "PrimitiveArgument":
            argLine += "(sizeof(" + argType + "), &" + argName + ");\n"
        if argumentType == "BlobArgument" or argumentType == "BlobReturnArgument":
            expression = metaArgs[argName]["expression"]
            if expression == "/string/":
                expression = "strlen(" + argName + ") + 1" # +1 to include null terminator
            argLine += "((" + expression + "), " + argName + ");\n"
        
        file.write(argLine)

        argIndex += 1
    
    file.write("\n")

    argumentsLine = "    Argument* args[] = {"
    for i in range(len(args)):
        argumentsLine += "&arg" + str(i + 1) + ", "
    argumentsLine += "};\n\n"

    file.write(argumentsLine)

    executeLine = "    gloip_execute(functionHash, waitForReturn, returnSize, " \
               + ("&ret" if returnPrimitive else "nullptr") + ", " + str(len(args)) + ", args);\n"
    
    file.write(executeLine)

    if returnPrimitive:
        file.write("\n    return ret;\n")

    file.write("}\n\n")


def generateClient(headerPath, metaPath, customPath, sourcePath):
    metaFile = open(metaPath, "r")
    meta = parseMetaFile(metaFile)
    metaFile.close()

    headerFile = open(headerPath, "r")
    functions = parseHeaderFile(headerFile)
    headerFile.close()

    sourceFile = open(sourcePath, "w")

    writeHeader(sourceFile, meta, "gloip_client.h")

    for function in functions:
        writeFunction(sourceFile, function, functions[function], meta)
    
    with open(customPath, "r") as custom:
        sourceFile.writelines(custom.readlines())
        sourceFile.write("\n")

    sourceFile.close()