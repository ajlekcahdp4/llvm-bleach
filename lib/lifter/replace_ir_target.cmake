file(READ ${INPUT_FILE_PATH} FILE_CONTENTS)

string(REGEX REPLACE
  "target triple = \"[^\"]*\"" "target triple = \"unknown-unknown-unknown\""
  FILE_CONTENTS "${FILE_CONTENTS}"
)
string(REGEX REPLACE
  "target datalayout = \"[^\"]*\"" "target datalayout = \"\""
  FILE_CONTENTS "${FILE_CONTENTS}"
)

string(REGEX REPLACE
  "attributes #([0-9]+) = \\{[^}]*\\}"
  "attributes #\\1 = { \"tune-cpu\"=\"generic\" }"
  FILE_CONTENTS "${FILE_CONTENTS}")

file(WRITE "${OUTPUT_FILE_PATH}" "${FILE_CONTENTS}")
