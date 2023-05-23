import logging

import parser_interface as PARSER_INTF
 
# Creating a logger object
logger = logging.getLogger(__name__.split('.')[0])

logger.setLevel(logging.INFO)

gCommentLine = "/"*75
gCommentStart = "///"
gIndent = " "*4

def get_display_fn_for_type(a_type):
    return "display{}".format(a_type)

def get_app_to_peer_fn_for_type(a_type):
    return "app_to_peer_{}".format(a_type)

def get_peer_to_app_fn_for_type(a_type):
    return "peer_to_app_{}".format(a_type)

def is_native_type(a_type):
    native_types = ["INT8", "UINT8", "INT16", "UINT16", "INT32", "UINT32", "INT64", "UINT64", "FLOAT"]

    if a_type in native_types:
        return True
    else:
        return False

def appint_type_to_peerint_type(a_type):
    type_map = {"INT8": "INT32", "UINT8": "UINT32", "INT16": "INT32", "UINT16": "UINT32"}

    if a_type in type_map:
        return type_map[a_type]
    else:
        return a_type

class InterfaceGen(PARSER_INTF.XMLToCode):
    def __init__(self, xmlFile, schemaFile, headerFile, sourceFile, namespace):
        super().__init__(xmlFile, schemaFile, headerFile, sourceFile)
        
        logger.info("Instantiating CPP InterfaceGen")

        meta_node = self.get_meta_node()

        self.meta_gen = CPPMetadataGen(meta_node, meta_node)
        self.imports_gen = CPPImportsGen(meta_node, self.get_imports_node())
        self.types_gen = CPPTypesGen(meta_node, self.get_types_node(), namespace)
        self.callbacks_gen = CPPCallbacksGen(meta_node, self.get_callbacks_node())
        self.functions_gen = CPPFunctionsGen(meta_node, self.get_functions_node())
        self.namespace = namespace

class CPPMetaField(PARSER_INTF.MetaField):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_start(self, targetFile):
        if self.suppress_key:
            field_string = "{}  {}\n\n".format(gCommentStart, self.value)
        else:
            field_string = "{}  {}: {}\n\n".format(gCommentStart, self.name, self.value)

        targetFile.write(field_string)

    def source_file_start(self, targetFile):
        self.declaration_file_start(targetFile)

class CPPMetadataGen(PARSER_INTF.MetadataGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for x in self.xml_meta_fields:
            self.meta_fields.append(CPPMetaField(xmlMetaData, x))

        logger.debug("Collected meta-data fields {}".format(self.meta_fields))

        for m in self.meta_fields:
            logger.debug(m)

    def declaration_file_start(self, targetFile):
        targetFile.write("{}\n".format(gCommentLine))
        targetFile.write("{}\n".format(gCommentStart))
        for field in self.meta_fields:
            field.declaration_file_start(targetFile)
        targetFile.write("{}\n".format(gCommentStart))
        targetFile.write("{}\n\n\n".format(gCommentLine))

    def source_file_start(self, targetFile):
        self.declaration_file_start(targetFile)

class CPPImport(PARSER_INTF.Import):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_start(self, targetFile):
        if self.should_print("Cpp", "Interface"):
            targetFile.write("#include \"{}\"\n".format(self.name))

    def source_file_start(self, targetFile):
        if self.should_print("Cpp", "Lib"):
            targetFile.write("#include \"{}\"\n".format(self.name))

class CPPImportsGen(PARSER_INTF.ImportsGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        logger.debug("Collected imports {}".format(self.xml_imports))

        for x in self.xml_imports:
            self.imports.append(CPPImport(xmlMetaData, x))

        for i in self.imports:
            logger.debug(i)

    def declaration_file_start(self, targetFile):
        file_name = PARSER_INTF.get_file_basename_with_extension(targetFile.name)
        file_name_for_guard = file_name.upper()
        file_name_for_guard = (file_name_for_guard.replace("-", "_")).replace(".", "_")

        targetFile.write("\n#ifndef __AUTOGEN_{}__\n#define __AUTOGEN_{}__\n\n".format(file_name_for_guard, file_name_for_guard))

        for i in self.imports:
            i.declaration_file_start(targetFile)
       
        targetFile.write("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")

    def declaration_file_end(self, targetFile):
        targetFile.write("\n#ifdef __cplusplus\n} // extern C\n#endif\n\n")
        targetFile.write("\n\n#endif\n")

    def source_file_start(self, targetFile):
        for i in self.imports:
            i.source_file_start(targetFile)
       
        targetFile.write("\nextern \"C\" {\n\n")

    def source_file_end(self, targetFile):
        targetFile.write("\n} // extern \"C\"\n\n")

class CPPEnumValue(PARSER_INTF.EnumValue):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_body(self, targetFile):
        enum_string = "{},".format(self.value)
        targetFile.write("    {:<32} = {:<32} ///< {}\n".format(self.name, enum_string, self.description))

class CPPEnum(PARSER_INTF.Enum):
    def __init__(self, xmlMetaData, xmlElement, namespace):
        super().__init__(xmlMetaData, xmlElement)

        self.namespace = namespace

        for x in self.xml_enum_values:
            self.enum_values.append(CPPEnumValue(xmlMetaData, x))

    def declaration_file_start(self, targetFile):
        targetFile.write("/// @enum {}\n/// @brief {}\n".format(self.name, self.description))
        targetFile.write("typedef enum {} {}\n".format(self.name, "{"))

        for e in self.enum_values:
            e.declaration_file_body(targetFile)

        targetFile.write('{} {};\n\n'.format("}", self.name))

        targetFile.write("void {}(void *obj);\n".format(get_display_fn_for_type(self.name)))
        targetFile.write("void {}(void *ptr_src_app, void *ptr_dst_peer);\n".format(get_app_to_peer_fn_for_type(self.name)))
        targetFile.write("void {}(void *ptr_src_peer, void *ptr_dst_app);\n\n".format(get_peer_to_app_fn_for_type(self.name)))

    def source_file_body(self, targetFile):
        global gIndent
        
        logger.debug("Building display function for {}".format(self.name))

        # build out the display function
        targetFile.write("void\n{}(void *obj)\n{}\n".format(get_display_fn_for_type(self.name), "{"))
        targetFile.write("{}printf(\"{} => {}\\n\", \"{}\", {});\n".format(gIndent, "%s", "%d", self.name, "*(INT32 *)obj"))
        targetFile.write("{}\n\n".format("}"))

        if self.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
            # build out app to peer
            targetFile.write("void\n{}(void *ptr_src_app, void *ptr_dst_peer)\n{}\n".format(get_app_to_peer_fn_for_type(self.name), "{"))

            # create local variables of appropriate types
            targetFile.write("{}{} *src = ({} *)ptr_src_app;\n".format(gIndent, self.name, self.name))
            targetFile.write("{}{} *dst = ({} *)ptr_dst_peer;\n\n".format(gIndent, self.name, self.name))

            targetFile.write("{}*dst = *src;\n".format(gIndent, ))

            targetFile.write("\n{}\n\n".format("}"))

class CPPField(PARSER_INTF.Field):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.array = ""

        if self.array_xml != None and self.array_xml != "0":
            self.array = "[{}]".format(self.array_xml)

    def declaration_file_body(self, targetFile):
        field_string = "{}{};".format(self.name, self.array)
        targetFile.write("    {:<48}{:<48} ///< @var {}\n".format(self.type, field_string, self.description))

    def print_display_component(self, targetFile):
        global gIndent

        targetFile.write("{}printf(\"{} ==>\\n\");\n".format(gIndent, self.name))
        if self.array == "":
            targetFile.write("{}{}((void *)&p->{});\n".format(gIndent, get_display_fn_for_type(self.type), self.name))
        else:
            targetFile.write("{}for (int i = 0; i < {}; i++) {}\n".format(gIndent, self.array_xml, "{"))
            targetFile.write("{}{}((void *)&p->{}[i]);\n".format(gIndent * 2, get_display_fn_for_type(self.type), self.name))
            targetFile.write("{}{}\n\n".format(gIndent, "}"))

    def print_app_to_peer_component(self, targetFile, namespace):
        global gIndent
        tmpVarName = self.get_peer_local_tmp_varname()
        type_cast = ""

        if not is_native_type(self.type):
            type_cast = "({}{})".format(namespace, self.type)

        if self.array == "":
            targetFile.write("{}{}(&src->{}, &{}); // {}\n\n".format(gIndent, get_app_to_peer_fn_for_type(self.type), self.name, tmpVarName, self.name))
            targetFile.write("{}dst->set_{}({}{});\n\n".format(gIndent, self.name, type_cast, tmpVarName))
        else:
            if self.type != "INT8":
                targetFile.write("{}for (int i = 0; i < {}; i++) {} // {}\n".format(gIndent, self.array_xml, "{", self.name))
                targetFile.write("{}{}(&src->{}, &{});\n".format(gIndent * 2, get_app_to_peer_fn_for_type(self.type), self.name, tmpVarName))
                targetFile.write("{}{}\n".format(gIndent, "}"))
            else:
                targetFile.write("{}dst->set_{}(&src->{}[0]);\n\n".format(gIndent, self.name, self.name, ))

    def print_peer_to_app_component(self, targetFile, namespace):
        global gIndent
        tmpVarName = self.get_peer_local_tmp_varname()
        type_cast = ""

        if not is_native_type(self.type):
            type_cast = "({})".format(self.type)

        if self.array == "":
            targetFile.write("{}dst->{} = {}src->{}();\n".format(gIndent, self.name, type_cast, self.name))
        else:
            if self.type != "INT8":
                targetFile.write("{}for (int i = 0; i < {}; i++) {} // {}\n".format(gIndent, self.array_xml, "{", self.name))
                targetFile.write("{}{}(&src->{}, &{});\n".format(gIndent * 2, get_peer_to_app_fn_for_type(self.type), self.name, tmpVarName))
                targetFile.write("{}{}\n".format(gIndent, "}"))
            else:
                targetFile.write("{}strcpy(&dst->{}[0], src->{}().c_str());\n".format(gIndent, self.name, self.name))

    def get_peer_local_tmp_varname(self):
        return "__tmp_{}".format(self.name)

    def print_peer_tmp_local_var(self, targetFile, namespace):
        global gIndent

        if self.array != "" and self.type == "INT8":
            # targetFile.write("{}string {}(&src->{}[0]);\n".format(gIndent, self.get_peer_local_tmp_varname(), self.name))
            pass # copy strings directly without using a tmp local
        else:
            targetFile.write("{}{} {};\n".format(gIndent, appint_type_to_peerint_type(self.type), self.get_peer_local_tmp_varname()))

class CPPStruct(PARSER_INTF.Struct):
    def __init__(self, xmlMetaData, xmlElement, namespace):
        super().__init__(xmlMetaData, xmlElement)

        self.namespace = namespace

        for x in self.xml_fields:
            field = CPPField(xmlMetaData, x)
            if field.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_App):
                self.fields.append(field)

    def declaration_file_start(self, targetFile):
        targetFile.write("struct {};\ntypedef struct {} {};\n\n".format(self.name, self.name, self.name))

    def declaration_file_body(self, targetFile):
        targetFile.write("/// @struct {}\n/// @brief {}\n".format(self.name, self.description))
        targetFile.write("struct {} {}\n".format(self.name, "{"))
        for f in self.fields:
            f.declaration_file_body(targetFile)
        targetFile.write("};\n\n")

        targetFile.write("void {}(const void *obj);\n".format(get_display_fn_for_type(self.name)))

        if self.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
            targetFile.write("void {}(const void *ptr_src_app, void *ptr_dst_peer);\n".format(get_app_to_peer_fn_for_type(self.name)))
            targetFile.write("void {}(const void *ptr_src_peer, void *ptr_dst_app);\n\n".format(get_peer_to_app_fn_for_type(self.name)))

    def source_file_body(self, targetFile):
        global gIndent
        
        # build out the display function
        targetFile.write("void\n{}(const void *obj)\n{}\n".format(get_display_fn_for_type(self.name), "{"))
        targetFile.write("{}{} *p = ({} *)obj;\n\n".format(gIndent, self.name, self.name))
        targetFile.write("{}printf(\"{} %p =>\\n\", obj);\n\n".format(gIndent, self.name))

        for f in self.fields:
            f.print_display_component(targetFile)

        targetFile.write("\n{}\n\n".format("}"))

        if self.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_Peer):
            # build out app to peer
            targetFile.write("void\n{}(const void *ptr_src_app, void *ptr_dst_peer)\n{}\n".format(get_app_to_peer_fn_for_type(self.name), "{"))

            # create local variables of appropriate types
            targetFile.write("{}{} *src = ({} *)ptr_src_app;\n".format(gIndent, self.name, self.name))
            targetFile.write("{}{}{} *dst = ({}{} *)ptr_dst_peer;\n\n".format(gIndent, self.namespace, self.name, self.namespace, self.name))

            for f in self.fields:
                f.print_peer_tmp_local_var(targetFile, self.namespace)

            targetFile.write("\n")

            for f in self.fields:
                f.print_app_to_peer_component(targetFile, self.namespace)

            targetFile.write("\n{}\n\n".format("}"))

            # build out peer to app
            targetFile.write("void\n{}(const void *ptr_src_peer, void *ptr_dst_app)\n{}\n".format(get_peer_to_app_fn_for_type(self.name), "{"))

            # create local variables of appropriate types
            targetFile.write("{}{} *dst = ({} *)ptr_dst_app;\n".format(gIndent, self.name, self.name))
            targetFile.write("{}{}{} *src = ({}{} *)ptr_src_peer;\n\n".format(gIndent, self.namespace, self.name, self.namespace, self.name))

            for f in self.fields:
                f.print_peer_to_app_component(targetFile, self.namespace)

            targetFile.write("\n{}\n\n".format("}"))


class CPPTypesGen(PARSER_INTF.TypesGen):
    def __init__(self, xmlMetaData, xmlElement, namespace):
        super().__init__(xmlMetaData, xmlElement)

        self.namespace = namespace

        for e in self.xml_enums:
            an_enum = CPPEnum(xmlMetaData, e, namespace)
            if an_enum.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_App):
                self.enums.append(an_enum)

        for s in self.xml_structs:
            a_struct = CPPStruct(xmlMetaData, s, namespace)
            if a_struct.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_App):
                self.structs.append(a_struct)

    def declaration_file_start(self, targetFile):
        targetFile.write("\n// >>>> Forward declarations and enums <<<<<\n\n")
        for e in self.enums:
            e.declaration_file_start(targetFile)

        for s in self.structs:
            s.declaration_file_start(targetFile)

    def declaration_file_body(self, targetFile):
        targetFile.write("\n// >>>> Data Types <<<<<\n\n")
        for e in self.enums:
            e.declaration_file_body(targetFile)

        for s in self.structs:
            s.declaration_file_body(targetFile)

    def source_file_body(self, targetFile):
        for s in self.structs:
            s.source_file_body(targetFile)

        for e in self.enums:
            e.source_file_body(targetFile)

class CPPFuncParam(PARSER_INTF.FuncParam):
    def __init__(self, xmlMetaData, xmlElement, is_first, is_last):
        super().__init__(xmlMetaData, xmlElement)
        self.is_first = is_first
        self.is_last = is_last

    def declaration_file_body(self, targetFile, print_var_name):
        if print_var_name:
            if self.is_last:
                var_string = "{}".format(self.variable_name)
            else:
                var_string = "{},".format(self.variable_name)
            targetFile.write("    {:<32}{:<32} ///< @param {}\n".format(self.type, var_string, self.description))
        else:
            targetFile.write("    {:<32} ///< @param {}\n".format(self.type, self.description))

class CPPFunctionPtr(PARSER_INTF.FunctionPtr):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        is_first = None
        is_last = None
        list_max_idx = len(self.xml_parameters) - 1 

        temp_xml_param_list = []

        for index, param in enumerate(self.xml_parameters):
            temp_param = CPPFuncParam(xmlMetaData, param, False, False)
            if temp_param.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_App):
                logger.debug("For function {}, param {} of type {} is applicable to peer because consumed_by = {}".format(self.name, temp_param.variable_name, temp_param.type, temp_param.consumed_by))
                temp_xml_param_list.append(temp_param)

        list_max_idx = len(temp_xml_param_list) - 1

        for index, temp_xml_entry in enumerate(temp_xml_param_list):
            if index == 0:
                is_first = True
            else:
                is_first = False

            if index == list_max_idx:
                is_last = True
            else:
                is_last = False

            temp_xml_entry.is_first = is_first
            temp_xml_entry.is_last = is_last
            self.parameters.append(temp_xml_entry)

    def declaration_file_body(self, targetFile):
        targetFile.write("/// @brief Callback function type {}\n/// @typedef {}\n\n".format(self.name, self.description))
        targetFile.write("typedef {}\n(*{})\n(\n".format(self.return_spec.get_return_type(PARSER_INTF.FunctionReturnType.RETURN_TYPE_app), self.name))
        for p in self.parameters:
            p.declaration_file_body(targetFile, False)

        targetFile.write(");\n")

        targetFile.write("static inline void\n")
        targetFile.write("display{}(void *obj) {} printf(\"%p\\n\", obj); {}\n".format(self.name, "{", "}"))


class CPPCallbacksGen(PARSER_INTF.CallbacksGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for fptr in self.xml_fptrs:
            self.fptrs.append(CPPFunctionPtr(xmlMetaData, fptr))

        logger.debug("Collected fptrs {}".format(self.fptrs))
        for f in self.fptrs:
            logger.debug(str(f))

    def declaration_file_body(self, targetFile):
        targetFile.write("\n// >>>> Callback function-ptr types <<<<<\n\n")

        for fptr in self.fptrs:
            fptr.declaration_file_body(targetFile)

class CPPFunction(PARSER_INTF.Function):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        is_first = None
        is_last = None
        list_max_idx = len(self.xml_parameters) - 1
 
        temp_xml_param_list = []

        for index, param in enumerate(self.xml_parameters):
            temp_param = CPPFuncParam(xmlMetaData, param, False, False)
            if temp_param.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_App):
                logger.debug("For function {}, param {} of type {} is applicable to peer because consumed_by = {}".format(self.name, temp_param.variable_name, temp_param.type, temp_param.consumed_by))
                temp_xml_param_list.append(temp_param)

        list_max_idx = len(temp_xml_param_list) - 1

        for index, temp_xml_entry in enumerate(temp_xml_param_list):
            if index == 0:
                is_first = True
            else:
                is_first = False

            if index == list_max_idx:
                is_last = True
            else:
                is_last = False

            temp_xml_entry.is_first = is_first
            temp_xml_entry.is_last = is_last
            self.parameters.append(temp_xml_entry)

    def declaration_file_body(self, targetFile):
        targetFile.write("/// @brief Function {}\n/// @fn {}\n".format(self.name, self.description))
        targetFile.write("{}\n{}\n(\n".format(self.return_spec.get_return_type(PARSER_INTF.FunctionReturnType.RETURN_TYPE_app), self.name))
        for p in self.parameters:
            p.declaration_file_body(targetFile, True)

        targetFile.write(");\n\n")

    def source_file_body(self, targetFile):
        targetFile.write("{}\n{}\n{}\n".format(self.return_type, self.name, "("))
        for p in self.parameters:
            p.declaration_file_body(targetFile, True)

        targetFile.write(")\n{\n")
        targetFile.write("    return {};\n{}\n\n".format(self.success_return, "}"))

class CPPFunctionsGen(PARSER_INTF.FunctionsGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for fptr in self.xml_funcs:
            self.functions.append(CPPFunction(xmlMetaData, fptr))

        logger.debug("Collected functions {}".format(self.functions))
        for f in self.functions:
            logger.debug(str(f))

    def declaration_file_body(self, targetFile):
        targetFile.write("\n// >>>> Function Prototypes <<<<<\n\n")

        for f in self.functions:
            f.declaration_file_body(targetFile)

    def source_file_body(self, targetFile):
        # do not generate these for now - maybe these are better off being written by hand
        return
        # targetFile.write("\n// >>>> Interface functions <<<<<\n\n")

        # for f in self.functions:
            # f.source_file_body(targetFile)
