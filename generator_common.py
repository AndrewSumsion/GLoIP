def readHeader(file, meta):
    line = file.readline()
    while line and not line.strip().startswith("endheader"):
        meta["header"] += line
        line = file.readline()


def parseMetaFile(file):
    meta = {
        "header": "",
        "functions": {}
    }

    """
    structure of meta dictionary:
    {
        "header": "#include ...",
        "functions": {
            "exampleFunction": {
                "arg1": {"type": "blob", "expression": "10 * sizeof(GLint)"}
            }
        }
    }
    """

    currentFunction = ""
    metaLine = "."
    while metaLine:
        metaLine = file.readline()

        args = metaLine.strip().split(" ")

        if args[0] == "header":
            readHeader(file, meta)
            continue

        if args[0] == "func":
            currentFunction = args[1]
            meta["functions"][args[1]] = {}
            continue

        if args[0] == "custom":
            meta["functions"][currentFunction]["custom"] = True
            currentFunction = ""
            continue

        if args[0] == "bsize":
            startIndex = metaLine.index("bsize")
            startIndex += len("bsize " + args[1]) + 1
            meta["functions"][currentFunction][args[1]] = {
                "type": "blob",
                "expression": metaLine[startIndex:].strip()
            }
            continue

        if args[0] == "rsize":
            startIndex = metaLine.index("rsize")
            startIndex += len("rsize " + args[1]) + 1
            meta["functions"][currentFunction][args[1]] = {
                "type": "return",
                "expression": metaLine[startIndex:].strip()
            }
            continue

        if args[0] == "blocking":
            meta["functions"][currentFunction]["blocking"] = True
            continue
        
        if args[0] == "check_param":
            if not "param_checks" in meta["functions"][currentFunction]:
                meta["functions"][currentFunction]["param_checks"] = []
            
            startIndex = metaLine.index("check_param") + len("check_param ") + len(args[1]) + 1

            paramCheck = {
                "error": args[1],
                "condition": metaLine[startIndex:].strip()
            }
            meta["functions"][currentFunction]["param_checks"].append(paramCheck)
        
        if args[0] == "unsupported":
            meta["functions"][currentFunction]["unsupported"] = True
            continue
    
    return meta

def parseHeaderFile(file):
    functions = {}

    hashCounter = 0
    line = "."
    while line:
        line = file.readline()
        if not line.startswith("GL_APICALL"):
            continue

        retTypeLeft = len("GL_APICALL ")
        retTypeRight = line.index("GL_APIENTRY")

        retType = line[retTypeLeft:retTypeRight].strip()

        nameLeft = line.index("GL_APIENTRY ") + len("GL_APIENTRY ")
        nameRight = line.index("(")

        name = line[nameLeft:nameRight].strip()

        functionHash = hashCounter
        hashCounter += 1

        argsString = line[line.index("(") + 1 : line.index(")")].strip()

        args = []

        if argsString != "void" and argsString != "":
            for s in argsString.split(","):
                kv = s.strip().split(" ")
                if kv[-1].startswith("*"):
                    kv[-1] = kv[-1][1:]
                    kv[-2] = kv[-2] + "*"
                args.append((kv[-1], kv[-2]))

        functions[name] = {
            "hashExpression": str(functionHash),
            "argsString": argsString,
            "args": args,
            "retType": retType
        }
    
    return functions

def writeHeader(file, meta, headerName):
    file.write(meta["header"])

    file.write("#include \"" + headerName + "\"\n")
    file.write("\n")
    file.write("#include <cstdint>\n")
    file.write("using std::uint8_t;\n")
    file.write("using std::uint32_t;\n")
    file.write("\n")
    file.write("#include <cstring>\n")
    file.write("using std::memcpy;\n")
    file.write("\n")
    file.write("#include <cstdio>\n")
    file.write("\n")