#pragma once
// A cross-platform/compiler system for suppressing warnings in code.
// Largely based on: https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/

// disable clang-format because it will add spaces to the warning names
// clang-format off
// NOLINTBEGIN
#ifdef __clang__
#define BEE_DO_PRAGMA(X) _Pragma(#X)
#define BEE_DISABLE_WARNING_PUSH BEE_DO_PRAGMA(clang diagnostic push)
#define BEE_DISABLE_WARNING_POP BEE_DO_PRAGMA(clang diagnostic pop)
#define BEE_DISABLE_WARNING(warningName) BEE_DO_PRAGMA(clang diagnostic ignored #warningName)
#define BEE_DISABLE_WARNING_DEPRECATED_DECLARATIONS BEE_DISABLE_WARNING(-Wdeprecated-declarations)
#define BEE_DISABLE_WARNING_HIDES_LOCAL_DECLARATION
#define BEE_DISABLE_WARNING_MISSING_FIELD_INITIALIZERS BEE_DISABLE_WARNING(-Wmissing-field-initializers)
#define BEE_DISABLE_WARNING_NAMELESS_STRUCT
#define BEE_DISABLE_WARNING_SIGNED_UNSIGNED_MISMATCH BEE_DISABLE_WARNING(-Wsign-conversion)
#define BEE_DISABLE_WARNING_SIZE_T_CONVERSION
#define BEE_DISABLE_WARNING_UNREFERENCED_LOCAL_VARIABLE BEE_DISABLE_WARNING(-Wunused-variable)
#define BEE_DISABLE_WARNING_UNUSED_PARAMETER BEE_DISABLE_WARNING(-Wunused-parameter)
#else
#define BEE_DISABLE_WARNING_PUSH __pragma(warning(push))
#define BEE_DISABLE_WARNING_POP __pragma(warning(pop))
#define BEE_DISABLE_WARNING(warningNumber) __pragma(warning(disable : warningNumber))
#define BEE_DISABLE_WARNING_DEPRECATED_DECLARATIONS BEE_DISABLE_WARNING(4267)
#define BEE_DISABLE_WARNING_HIDES_LOCAL_DECLARATION BEE_DISABLE_WARNING(4456)
#define BEE_DISABLE_WARNING_MISSING_FIELD_INITIALIZERS
#define BEE_DISABLE_WARNING_NAMELESS_STRUCT BEE_DISABLE_WARNING(4201)
#define BEE_DISABLE_WARNING_SIGNED_UNSIGNED_MISMATCH BEE_DISABLE_WARNING(4018)
#define BEE_DISABLE_WARNING_SIZE_T_CONVERSION BEE_DISABLE_WARNING(4267)
#define BEE_DISABLE_WARNING_UNREFERENCED_LOCAL_VARIABLE BEE_DISABLE_WARNING(4101)
#define BEE_DISABLE_WARNING_UNUSED_PARAMETER
#endif
// NOLINTEND
// clang-format on



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/