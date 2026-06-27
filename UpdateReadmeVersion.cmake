file(READ "${MOD_JSON_FILE}" MOD_JSON)

string(JSON MOD_VERSION GET "${MOD_JSON}" "version")

file(READ "${README_FILE}" README_CONTENT)

string(REGEX REPLACE "\"version\": \">=[^\"]*\"" "\"version\": \">=${MOD_VERSION}\"" README_CONTENT "${README_CONTENT}")

file(WRITE "${README_FILE}" "${README_CONTENT}")
