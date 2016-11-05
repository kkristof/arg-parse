#ifndef ARG_PARSE_H
#define ARG_PARSE_H

/* Copyright (C) 2016, Szilard Ledan <szledan@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * TODO: README
 *  ArgParse:
 *      Arg : Value
 *      Flag { Value }
 *      ErrorCodes;
 */

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <initializer_list>

namespace argparse {

typedef void (*CallBackFunc)(void);

struct Value;
struct Arg;
struct Flag;

// ArgPars

class ArgParse {
public:
    enum ErrorCodes {
        NoError = 0,
        ErrorRequiredArgumentMissing,
        ErrorRequiredFlagValueMissing,
        ErrorARGVEmpty,
    };

    struct ArgError {
        enum ArgErrorType {
            GeneralType,
            ArgType,
            FlagType,
        } const type;

        const ErrorCodes errorCode;
        union {
            const void* _ptr;
            const Arg* arg;
            const Flag* flag;
        };
        const std::string errorMessage;
    };

    typedef std::initializer_list<std::string> OptionList;

    ArgParse(const OptionList& = {});

    const Arg& add(const Arg&);
    const Flag& add(const Flag&, CallBackFunc = nullptr);

    const bool parse(const int argc, char* const argv[]);

    const std::string help();
    const std::string error();
    const std::vector<ArgError>& errors() { return _errors; }

    const bool checkFlag(const std::string& flagStr);
    template<typename T>
    const bool checkFlagAndReadValue(const std::string& flagStr, T* value);

    Arg const& operator[](const std::size_t& idx);
    Flag const& operator[](const std::string& idx);
    Flag const& operator[](const char* idx);

    struct Counts {
        size_t undefinedArgs;
        size_t definedArgs;
        size_t undefinedFlags;
        size_t definedFlags;
    } counts = { 0u, 0u, 0u, 0u };

    struct Options {
        struct Option {
            struct Value {
                enum {
                    Unused = -1,
                    // Boolean.
                    NotSet = 0,
                    Set = 1,
                    // Not boolean.
                    NotBool,
                    FourSpace,
                };
                const bool isSet() const { return state != NotSet; }

                std::string value;
                int state;
            };

            const std::string name;
            const Value init;
            Value current;
        }

        programName = { "program-name", { "", Option::Value::NotSet } },
        tab = { "tab", { "    ", Option::Value::FourSpace } },
        mode = { "mode", { "compact", Option::Value::Set } },
        helpFlag = { "help-flag", { "on", Option::Value::Set } };

        static void set(Option&, const std::string& value, const int& state = Option::Value::Unused);
    } options;

private:
    void addError(const ErrorCodes&, const std::string& errorMsg);
    void addError(const ErrorCodes&, const std::string& errorMsg, const Arg*);
    void addError(const ErrorCodes&, const std::string& errorMsg, const Flag*);

    std::vector<Arg> _args;
    std::map<std::string, Flag> _flags;
    std::map<std::string, Flag*> _longFlags;
    std::map<std::string, Flag*> _shortFlags;
    std::vector<ArgError> _errors;
};

inline std::ostream& operator<<(std::ostream& os, const ArgParse::ArgError& err);

// Value

typedef std::initializer_list<std::string> ChooseList;

struct Value {
    Value(const Value& v);

    Value(const std::string& defaultValue = "",
          const std::string& name = "",
          const std::string& description = "");

    Value(const std::string& defaultValue,
          const ChooseList& chooseList,
          const std::string& name = "",
          const std::string& description = "");

    const bool empty() const { return str.empty(); }

    std::string str;

// private:
    const std::string _getChoosesStr(bool full = true) const;

    std::string _name;
    std::string _description;
    std::vector<std::string> _chooseList;
    bool _isValueNeeded;
};

// Arg

struct Arg : Value {
    static const bool IsNeeded = true;

    Arg(const Arg& a);

    Arg(const std::string& name = "",
        const std::string& description = "",
        const bool isNeeded = false,
        const Value& defaultValue = Value());
    Arg(const Value& value);

    void setArg(const std::string& value);

    bool isSet;
// private:
    bool _isArgNeeded;
    CallBackFunc _callBackFunc;
};

// Flag

struct Flag {
    Flag(const Flag& f);

    Flag(const std::string& longFlag = "",
         const std::string& shortFlag = "",
         const std::string& description = "");

    Flag(const std::string& longFlag,
         const std::string& shortFlag,
         const std::string& description,
         const Value value);

    bool isSet;
    bool hasValue;
    Value value;
// private:
    std::string _longFlag;
    std::string _shortFlag;
    std::string _description;
    CallBackFunc _callBackFunc;
};

} // namespace argparse

#endif // ARG_PARSE_H
