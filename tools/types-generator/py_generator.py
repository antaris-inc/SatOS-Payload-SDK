import logging

import parser_interface as PARSER_INTF

# Creating a logger object
logger = logging.getLogger(__name__.split('.')[0])

# Setting the threshold of logger to DEBUG
logger.setLevel(logging.INFO)

gCommentLine = "#"*75
gCommentStart = "#"
gIndent = " "*4

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

class InterfaceGen(PARSER_INTF.XMLToCode):
    def __init__(self, xmlFile, schemaFile, headerFile, sourceFile, moduleNamespace):
        super().__init__(xmlFile, schemaFile, headerFile, sourceFile)

        logger.info("Instantiating Python InterfaceGen")

        meta_node = self.get_meta_node()

        self.meta_gen = PyMetadataGen(meta_node, meta_node)
        self.imports_gen = PyImportsGen(meta_node, self.get_imports_node())
        self.types_gen = PyTypesGen(meta_node, self.get_types_node(), moduleNamespace)
        self.callbacks_gen = PyCallbacksGen(meta_node, self.get_callbacks_node())
        self.functions_gen = PyFunctionsGen(meta_node, self.get_functions_node())

class PyMetaField(PARSER_INTF.MetaField):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_start(self, targetFile):
        if self.suppress_key:
            field_string = "{}  {}\n\n".format(gCommentStart, self.value)
        else:
            field_string = "{}  {}: {}\n\n".format(gCommentStart, self.name, self.value)

        targetFile.write(field_string)

class PyMetadataGen(PARSER_INTF.MetadataGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for x in self.xml_meta_fields:
            self.meta_fields.append(PyMetaField(xmlMetaData, x))

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


class PyImport(PARSER_INTF.Import):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_start(self, targetFile):
        if self.should_print("Python", "Interface"):
            targetFile.write("from satos_payload_sdk.gen import {}\n".format(self.name))

class PyImportsGen(PARSER_INTF.ImportsGen):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        logger.debug("Collected imports {}".format(self.xml_imports))

        for x in self.xml_imports:
            self.imports.append(PyImport(xmlMetaData, x))

        for i in self.imports:
            logger.debug(i)

    def declaration_file_start(self, targetFile):
        for i in self.imports:
            i.declaration_file_start(targetFile)

class PyEnumValue(PARSER_INTF.EnumValue):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_body(self, targetFile):
        global gIndent
        global gCommentStart

        targetFile.write("{}{} = {} {} {}\n".format(gIndent, self.name, self.value, gCommentStart, self.description))

    def reverse_dictionary_component(self, targetFile):
        targetFile.write("{} : \"{}\"".format(self.value, self.name))

class PyEnum(PARSER_INTF.Enum):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

        for x in self.xml_enum_values:
            self.enum_values.append(PyEnumValue(xmlMetaData, x))

    def declaration_file_body(self, targetFile):
        global gIndent

        targetFile.write("# ENUM: {} - {}\n".format(self.name, self.description))
        targetFile.write("class {}:\n".format(self.name))

        for e in self.enum_values:
            e.declaration_file_body(targetFile)

        # reverse dictionary
        targetFile.write("\n\n{}reverse_dict = {}".format(gIndent, "{"))

        index = 0

        for e in self.enum_values:
            e.reverse_dictionary_component(targetFile)

            if (index < (len(self.enum_values) - 1)):
                targetFile.write(", ")
            else:
                targetFile.write("{}\n\n".format("}"))

            index += 1

        targetFile.write('\n\n')

class PyField(PARSER_INTF.Field):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)
        self.array = ""

        if self.array_xml != None and self.array_xml != "0":
            self.array = "[{}]".format(self.array_xml)

    def declaration_file_body(self, targetFile):
        global gIndent
        local_indent = gIndent * 2

        targetFile.write("{}self.{} = {}\n".format(local_indent, self.name, self.name))

    def print_display_component(self, targetFile):
        global gIndent
        local_indent = gIndent * 2

        targetFile.write("{}ret_str += str(self.{}) + \"\\n\"\n".format(local_indent, self.name))

class PyStruct(PARSER_INTF.Struct):
    def __init__(self, xmlMetaData, xmlElement, moduleNamespace):
        super().__init__(xmlMetaData, xmlElement)

        for x in self.xml_fields:
            field = PyField(xmlMetaData, x)
            if field.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_App):
                self.fields.append(field)

        self.namespace = moduleNamespace

    def write_peer_to_app_function(self, targetFile):
        local_indent = gIndent
        obj_constructor_params = ""
        targetFile.write("def {}(peer_struct):\n".format(get_peer_to_app_fn_for_type(self.name)))
        for f in self.fields:
            targetFile.write("{}{} = peer_struct.{}\n".format(local_indent, f.name, f.name))
            if obj_constructor_params == "":
                obj_constructor_params = f.name
            else:
                obj_constructor_params += ", " + f.name

        targetFile.write("{}return {}({})\n\n".format(local_indent, self.name, obj_constructor_params))

    def write_app_to_peer_function(self, targetFile):
        local_indent = gIndent
        obj_constructor_params = ""
        targetFile.write("def {}(app_struct):\n".format(get_app_to_peer_fn_for_type(self.name)))
        for f in self.fields:
            if obj_constructor_params == "":
                obj_constructor_params = "{} = app_struct.{}".format(f.name, f.name)
            else:
                obj_constructor_params += ", {} = app_struct.{}".format(f.name, f.name)

        targetFile.write("{}return {}_pb2.{}({})\n\n".format(local_indent, self.namespace, self.name, obj_constructor_params))

    def declaration_file_body(self, targetFile):
        global gIndent
        local_indent = gIndent
        local_second_indent = gIndent * 2

        if (0 == len(self.fields)):
            return

        targetFile.write("## @class: {}\n## @brief: {}\n".format(self.name, self.description))
        for f in self.fields:
            targetFile.write("## @param: {:<48}:    {:<48}\n".format(f.name, f.description))

        targetFile.write("class {}:\n".format(self.name))

        # make a constructor
        targetFile.write("{}def __init__(self".format(local_indent))
        for f in self.fields:
            targetFile.write(", {}".format(f.name))

        targetFile.write("):\n")

        for f in self.fields:
            f.declaration_file_body(targetFile)

        targetFile.write("\n")

        # make a stringify function
        targetFile.write("{}def __str__(self):\n".format(local_indent))
        targetFile.write("{}ret_str = \"\"\n".format(local_second_indent))

        for f in self.fields:
            targetFile.write("{}ret_str += \"{}:\\n\"\n".format(local_second_indent, f.name))
            f.print_display_component(targetFile)

        targetFile.write("\n{}return ret_str".format(local_second_indent))

        targetFile.write("\n\n")

        # make a display function (for backward compatibility)
        targetFile.write("{}def display(self):\n".format(local_indent))
        targetFile.write("{}print(str(self))\n\n".format(local_second_indent))

        self.write_peer_to_app_function(targetFile)
        self.write_app_to_peer_function(targetFile)

class PyTypesGen(PARSER_INTF.TypesGen):
    def __init__(self, xmlMetaData, xmlElement, moduleNamespace):
        super().__init__(xmlMetaData, xmlElement)

        for e in self.xml_enums:
            an_enum = PyEnum(xmlMetaData, e)
            if an_enum.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_App):
                self.enums.append(an_enum)

        for s in self.xml_structs:
            a_struct = PyStruct(xmlMetaData, s, moduleNamespace)
            if a_struct.check_applicability(PARSER_INTF.Generator.attrib_consumed_by_App):
                self.structs.append(a_struct)

    def declaration_file_body(self, targetFile):
        targetFile.write("\n# >>>> Data Types <<<<<\n\n")
        for e in self.enums:
            e.declaration_file_body(targetFile)

        for s in self.structs:
            s.declaration_file_body(targetFile)

class PyCallbacksGen(PARSER_INTF.Generator):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_start(self, targetFile):
        pass

    def declaration_file_body(self, targetFile):
        pass

    def declaration_file_end(self, targetFile):
        pass

    def source_file_start(self, targetFile):
        pass

    def source_file_body(self, targetFile):
        pass

    def source_file_end(self, targetFile):
        pass

class PyFunctionsGen(PARSER_INTF.Generator):
    def __init__(self, xmlMetaData, xmlElement):
        super().__init__(xmlMetaData, xmlElement)

    def declaration_file_start(self, targetFile):
        pass

    def declaration_file_body(self, targetFile):
        pass

    def declaration_file_end(self, targetFile):
        pass

    def source_file_start(self, targetFile):
        pass

    def source_file_body(self, targetFile):
        pass

    def source_file_end(self, targetFile):
        pass
