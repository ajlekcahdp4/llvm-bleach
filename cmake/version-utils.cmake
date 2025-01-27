function(get_source_info path version)
  set(pregenerated_version_file ${path}/version.json)
  cmake_path(NORMAL_PATH pregenerated_version_file)
  if(EXISTS ${pregenerated_version_file})
    message(
      STATUS
        "Parsing pregenerated version file from ${pregenerated_version_file}")
    file(READ ${pregenerated_version_file} version_json)
    string(JSON pregenerated_version GET ${version_json} version)
    string(REGEX MATCH "([0-9]+)\.([0-9]+)\.([0-9]+)" version_trimmed
                 "${pregenerated_version}")
    if(version_trimmed STREQUAL "")
      set(version_trimmed "0.0.0")
    endif()
    set(${version}
        ${version_trimmed}
        PARENT_SCOPE)
    return()
  endif()
  set(${version}
      "0.0.0"
      PARENT_SCOPE)
endfunction()
