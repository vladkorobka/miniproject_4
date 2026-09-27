# Викликається на КОЖНУ збірку (custom target у main/CMakeLists.txt) і пише
# build_time.h з поточним локальним часом ПК. Використання:
#   cmake -DOUT=<шлях до build_time.h> -P gen_build_time.cmake

# Одна мітка на все, щоб поля не "розʼїхались" на межі секунди.
string(TIMESTAMP STAMP "%Y%m%d%H%M%S")

string(SUBSTRING "${STAMP}" 0 4 Y)
string(SUBSTRING "${STAMP}" 4 2 MO)
string(SUBSTRING "${STAMP}" 6 2 D)
string(SUBSTRING "${STAMP}" 8 2 H)
string(SUBSTRING "${STAMP}" 10 2 MI)
string(SUBSTRING "${STAMP}" 12 2 S)

# "09" у C - некоректний вісімковий літерал, тому провідний нуль прибираємо.
foreach(var MO D H MI S)
    string(REGEX REPLACE "^0([0-9])$" "\\1" ${var} "${${var}}")
endforeach()

file(WRITE "${OUT}"
"#pragma once
/* Згенеровано main/gen_build_time.cmake на кожну збірку. Не редагувати. */
#define BUILD_YEAR   ${Y}
#define BUILD_MONTH  ${MO}
#define BUILD_DAY    ${D}
#define BUILD_HOUR   ${H}
#define BUILD_MIN    ${MI}
#define BUILD_SEC    ${S}
#define BUILD_STAMP  \"${STAMP}\"
")
