find_program(GLSLC_COMMAND glslc)
if(NOT GLSLC_COMMAND)
    message(FATAL_ERROR "glslc required - part of Vulkan SDK")
endif()

# Global variable to track generated SPIR-V files to avoid duplicate rules
set_property(GLOBAL PROPERTY SPIRV_GENERATED_FILES "")

#
# Generate SPIR-V files from the arguments. Returns a list of generated files.
#
function(spirv_shaders ret)
    set(options)
    set(oneValueArgs SPIRV_VERSION)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(_spirvshaders "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN})

    if(NOT _spirvshaders_SPIRV_VERSION)
        set(_spirvshaders_SPIRV_VERSION 1.0)
    endif()

    # Get the list of already generated SPIR-V files
    get_property(_generated_files GLOBAL PROPERTY SPIRV_GENERATED_FILES)

    foreach(GLSL ${_spirvshaders_SOURCES})
        string(MAKE_C_IDENTIFIER ${GLSL} IDENTIFIER)
        set(SPV_FILE "${AD_DEFINE_RES_ROOT_DIR}Shader/${GLSL}.spv")
        set(GLSL_FILE "${AD_DEFINE_RES_ROOT_DIR}Shader/${GLSL}")

        # Check if this SPV file has already been generated
        if(NOT SPV_FILE IN_LIST _generated_files)
            message("GLSL Command: ${GLSLC_COMMAND} ${GLSL_FILE} -o ${SPV_FILE}")

            add_custom_command(
                    OUTPUT ${SPV_FILE}
                    COMMAND ${GLSLC_COMMAND} ${GLSL_FILE} -o ${SPV_FILE}
                    DEPENDS ${GLSL_FILE}
                    VERBATIM
                    COMMENT "Compiling ${GLSL_FILE} to SPIR-V")
            
            # Add this SPV file to the list of generated files
            list(APPEND _generated_files ${SPV_FILE})
        endif()
        
        list(APPEND SPV_FILES ${SPV_FILE})
    endforeach()

    # Update the global list of generated SPIR-V files
    set_property(GLOBAL PROPERTY SPIRV_GENERATED_FILES "${_generated_files}")

    set(${ret} "${SPV_FILES}" PARENT_SCOPE)
endfunction(spirv_shaders)