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

#if 0
/*
 * TODO LIST:
 *  [1] More more tests :)
 *  [2] Implement 'mode.strict'.
 *  [5] Replace every 'true/false' to const values.
 *
 * FIXME LIST:
 *  [4] The 'help()' works buggly: sometimes doeasn't show [value]s.
 *  [3] Update documentation: ./README.md ./doc/api-references.md ./demos/README.md ./src/README.md
 *
 * FUTURE LIST:
 *  [6] Extend ArgParse with: argc(), argv() native getter functions to _argc, _argv.
 *  [7] Extend './demos' with some typical example.
 */
#endif

#include "arg-parse.hpp"

#include <assert.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace argparse {

// Util functions.
namespace {

template <typename T>
const T returnValue(const std::string& value, const T& def)
{
    std::stringstream s;
    T retValue;
    s << value;
    s >> retValue;
    return !s.fail() ? retValue : def;
}

const bool returnValue(const std::string& value, const bool& def)
{
    return !(value.empty() || (value == "false") || (value == "off") || (value == "0"));
}

const std::string returnValue(const std::string& value, const std::string& def)
{
    return value;
}

template <typename T>
const T readOption(const std::string& oList, const std::string& key, const T& def)
{
    if (oList.empty())
        return def;

    std::stringstream optionList(oList);
    std::string option;
    while (std::getline(optionList, option, ',')) {
        const size_t posEq = option.find("=");
        if (posEq != std::string::npos) {
            if (option.substr(0, posEq) == key)
                return returnValue(option.substr(posEq + 1), def);
        }
    }

    return def;
}

const std::string interlacedOptions(const ArgParse::OptionList& oList)
{
    std::string interlacedStr;
    for(auto option : oList)
        interlacedStr += option + ',';
    return interlacedStr;
}

enum ParamType {
    ArgType = 0,
    ShortFlagType,
    ShortFlagsType,
    LongFlagWithEqType,
    LongFlagWithoutEqType,
};

const ParamType mapParamType(const std::string& arg)
{
    if (arg.size() <= 1 || arg[0] != '-' || (arg.size() == 2 && arg[1] == '-'))
        return ParamType::ArgType;

    assert(arg[0] == '-');
    if (arg.size() == 2) {
        assert(arg[1] != '-');
        return ParamType::ShortFlagType;
    }

    assert(arg.size() > 2);
    if (arg[1] != '-') {
        return ParamType::ShortFlagsType;
    }

    if (arg.find('=') != std::string::npos) {
        return ParamType::LongFlagWithEqType;
    }

    return ParamType::LongFlagWithoutEqType;
}

const bool findValue(const std::string& valueStr, const std::vector<std::string>& chooseList)
{
    if (!chooseList.size())
        return true;

    for (auto const& choose : chooseList) {
        if (choose == valueStr)
            return true;
    }

    return false;
}

const std::string getChoosesStr(const Value& value, const bool full)
{
    if (!value._chooseList.size())
        return "";

    std::stringstream s;
    std::string end(full ? "" : "|...");
    for (size_t i = 0; i < value._chooseList.size(); ++i)
        s << "|" << value._chooseList[i];

    return s.str().substr(1) + end;
}

void setArg(Arg& arg, const std::string& value)
{
    arg.isSet = true;
    arg.str = value;
}

} // namespace anonymous

// ArgPars

ArgParse::ArgParse(const std::string& oList)
{
#define AP_SET_OPTION(VALUE, KEY) (VALUE = readOption(oList, KEY, VALUE))
    AP_SET_OPTION(options.program.name, "program.name");
    AP_SET_OPTION(options.tab, "tab");
    AP_SET_OPTION(options.mode.strict, "mode.strict");
    if (AP_SET_OPTION(options.help.add, "help.add"))
        def(Flag("--help", "-h", "Show this help."));
    AP_SET_OPTION(options.help.compact, "help.compact");
    AP_SET_OPTION(options.help.show, "help.show");
#undef AP_SET_OPTION
}

ArgParse::ArgParse(const OptionList& oList)
    : ArgParse(interlacedOptions(oList))
{
}

const Flag& ArgParse::addFlag(const Flag& flag, const CallBackFunc cbf)
{
    const std::string name = flag._longFlag + flag._shortFlag;
    assert(!name.empty());

    Flag* flagPtr = &(_flags[name] = flag);
    assert(flagPtr == &(_flags[name]));

    if (!flag._longFlag.empty()) {
        _longFlags[flag._longFlag] = flagPtr;
    }
    if (!flag._shortFlag.empty()) {
        _shortFlags[flag._shortFlag] = flagPtr;
    }

    flagPtr->_callBackFunc = cbf;
    return *flagPtr;
}

const Flag& ArgParse::def(const Flag& flag, const CallBackFunc cbf)
{
    if (!flag.isDefined || !flag.isValid())
        return Flag::WrongFlag;
    return addFlag(flag, cbf);
}

const Arg& ArgParse::def(const Arg& arg)
{
    if (!arg.isDefined)
        return Arg::WrongArg;
    _args.push_back(arg);
    return _args.back();
}

const bool ArgParse::parse(const int argc_, char* const argv_[])
{
    if ((argc_ < 1) || (!argv_) || (!argv_[0])) {
        addError(Errors::ArgVIsEmpty, "Wrong argument count: 0!", argv_ ? reinterpret_cast<void*>(argv_[0]) : nullptr);
        return false;
    }
    for (int argIndex = 0; argIndex < argc_; ++argIndex) {
        if (argv_[argIndex] == nullptr) {
            std::string message = std::string("The 'argc' > count('argv'): ") + std::to_string(argc_) + " != " + std::to_string(argIndex);
            addError(Errors::ArgCBiggerThanElementsOfArgV, message, reinterpret_cast<void*>(argv_[0]));
            return false;
        }
    }
    const size_t argc = (size_t)argc_;

    // Set program name.
    if (options.program.name.empty())
        options.program.name = std::string(argv_[0]);

    // Calculate number of required arguments.
    int requiredArgs = 0;
    for (auto const& arg : _args)
        if (arg.isRequired)
            requiredArgs++;

    std::vector<CallBackFunc> callBackFuncs;
    // Parse params.
    for (size_t adv = 1, argCount = 0; adv < argc; ++adv) {
        struct {
            std::string paramStr;
            const ParamType paramType;
            const bool hasNextParam;
            std::string valueStr;
        } param = {
            std::string(argv_[adv]),
            mapParamType(param.paramStr),
            adv + 1 < argc,
            std::string(param.hasNextParam ? argv_[adv + 1] : ""),
        };

#define AP_IS_LONG (true)
#define AP_CHECH_VALUE (true)
#define AP_CHECK_FLAG_EXIST(FLAGS, IS_LONG) do { \
        if (FLAGS.find(param.paramStr) == FLAGS.end()) { \
            if (IS_LONG) \
                addFlag(Flag(param.paramStr, "")); \
            else \
                addFlag(Flag("", param.paramStr)); \
            FLAGS[param.paramStr]->isDefined = false; \
            counts.flags.undefined++; \
        } else \
            counts.flags.defined++; \
    } while (false)

#define AP_SETUP_FLAG(FLAGS, CHECH_VALUE) do { \
        Flag* flag = FLAGS[param.paramStr]; \
        assert(flag); \
        flag->isSet = true; \
        if (flag->_callBackFunc) { \
            callBackFuncs.push_back(flag->_callBackFunc); \
        } \
    \
        if ((CHECH_VALUE) && flag->hasValue) { \
            if (!param.hasNextParam && flag->value.isRequired) { \
                addError(Errors::RequiredFlagValueMissing, "Missing required value.", flag); \
            } else if ((flag->value.isRequired) \
                       && (mapParamType(param.valueStr) != ParamType::ArgType) \
                       && (check(param.valueStr))) { \
                addError(Errors::RequiredFlagValueMissing, "Missing required value, next is a defined flag.", flag); \
            } else if (!findValue(param.valueStr, flag->value._chooseList)) { \
                    addError(Errors::RequiredFlagValueMissing, "Did not find choose in list.", flag); \
            } else { \
                flag->value.str = param.valueStr; \
                if (param.paramType != LongFlagWithEqType) \
                    ++adv; \
            } \
        } \
    } while (false)

        switch (param.paramType) {
        case ParamType::ShortFlagsType: {
            const std::string shortFlags = param.paramStr;
            for (size_t i = 1; i < shortFlags.size(); ++i) {
                param.paramStr = std::string("-") + shortFlags[i];
                const bool checkValue = i == (param.paramStr.size() - 1);
                AP_CHECK_FLAG_EXIST(_shortFlags, !AP_IS_LONG);
                AP_SETUP_FLAG(_shortFlags, checkValue);
            }
            break;
        }
        case ParamType::ShortFlagType:
            AP_CHECK_FLAG_EXIST(_shortFlags, !AP_IS_LONG);
            AP_SETUP_FLAG(_shortFlags, AP_CHECH_VALUE);
            break;
        case ParamType::LongFlagWithEqType: {
            const bool isDefinedFullParam = _longFlags.find(param.paramStr) != _longFlags.end();
            if (!isDefinedFullParam) {
                const size_t posEq = param.paramStr.find("=");
                param.valueStr = param.paramStr.substr(posEq + 1);
                param.paramStr = param.paramStr.substr(0, posEq);

                if (_longFlags.find(param.paramStr) == _longFlags.end()) {
                    // Add undefined flag with value.
                    def(Flag(param.paramStr, "", "", Value("_")));
                    _longFlags[param.paramStr]->value.str = param.valueStr;
                    counts.flags.undefined++;
                } else {
                    AP_CHECK_FLAG_EXIST(_longFlags, AP_IS_LONG);
                }
            }
            AP_SETUP_FLAG(_longFlags, AP_CHECH_VALUE);
            break;
        }
        case ParamType::LongFlagWithoutEqType:
            AP_CHECK_FLAG_EXIST(_longFlags, AP_IS_LONG);
            AP_SETUP_FLAG(_longFlags, AP_CHECH_VALUE);
            break;
        case ParamType::ArgType:
            if (_args.size() > argCount) {
                if (_args[argCount].isRequired)
                    requiredArgs--;
                setArg(_args[argCount], param.paramStr);
                counts.args.defined++;
            } else {
                _args.push_back(Arg("", "", !Arg::Required, Value(param.paramStr)));
                counts.args.undefined++;
            }
            argCount++;
            break;
        default:
            assert(false);
            break;
        };
#undef AP_SETUP_FLAG
#undef AP_CHECK_FLAG_EXIST
#undef AP_CHECH_VALUE
#undef AP_IS_LONG
    }

    if (requiredArgs) {
        for (int i = requiredArgs; i; --i)
            addError(Errors::RequiredArgumentMissing, "Required argument missing!", &_args[_args.size() - i]);
        return false;
    }

    // Call functions.
    if (!_errors.size()) {
        for (auto const& func : callBackFuncs)
            func();
    }

    return !_errors.size();
}

const std::string ArgParse::help()
{
    const std::string tab = options.tab;
    std::stringstream help;

    // Print program name.
    help << "usage: " << options.program.name;

    // Add '[options]' for usage part.
    if (counts.flags.defined)
        help << " [options]";

    // Print arguments after programname.
    for (auto const& it : _args) {
        const Arg& arg = it;
        if (!arg._name.empty()) {
            if (arg.isRequired)
                help << " <" << arg._name << "> ";
            else
                help << " [<" << arg._name << ">] ";
        }
    }

    // Print arguments.
    help << std::endl << std::endl << "Arguments:" << std::endl;

    for (auto const& arg : _args) {
        if (!arg._name.empty()
            && ((options.help.show == Options::Help::ShowOnesWithDescription && !arg._description.empty())
                || (options.help.show == Options::Help::ShowAllDefined && arg.isDefined)
                || options.help.show == Options::Help::ShowAll)) {
            if (arg.isRequired)
                help << tab << "<" << arg._name << ">";
            else
                help << tab << "[<" << arg._name << ">]";
            if (!arg._description.empty())
                help << tab << arg._description;
            help << std::endl;
        }
    }

    // Print option flags.
    help << std::endl << "Option flags:" << std::endl;

    for (auto const& it : _flags) {
#define AP_HAS_NAME(CH, VALNAME) do { \
        if (flag.value.isRequired) \
            help << CH << "<" << VALNAME << ">"; \
        else \
            help << CH << "[<" << VALNAME << ">]"; \
    } while(false)

#define AP_PRINT_FLAG(FLAG, L) do { \
        help << FLAG; \
        if (flag.value._chooseList.size() > 0) \
            AP_HAS_NAME(" ", getChoosesStr(flag.value, hasLongFlag ? L : (L ? 0 : 1))); \
        else if (!flag.value._name.empty()) \
            AP_HAS_NAME(" ", flag.value._name); \
    } while(false)

        const Flag& flag = it.second;
        const bool hasShortFlag = !flag._shortFlag.empty();
        const bool hasLongFlag = !flag._longFlag.empty();
        if ((options.help.show == Options::Help::ShowOnesWithDescription && flag._description.empty())
            || (options.help.show == Options::Help::ShowAllDefined && !flag.isDefined))
            continue;

        if (hasShortFlag || hasLongFlag)
            help << tab;
        if (hasShortFlag)
            AP_PRINT_FLAG(flag._shortFlag, 1);
        if (hasLongFlag) {
            if (hasShortFlag)
                help << ", ";
            AP_PRINT_FLAG(flag._longFlag, 0);
        }
        if (hasShortFlag || hasLongFlag) {
            if (!options.help.compact)
                help << std::endl << tab;
            help << tab << flag._description << std::endl;
        }

#undef AP_PRINT_FLAG
#undef AP_HAS_NAME
    }

    return help.str();
}

inline std::ostream& operator<<(std::ostream& os, const ArgParse::Errors& err)
{
    std::string typeSpecMsg("");
    if (err.suspect.type == ArgParse::Errors::Suspect::FlagType)
        typeSpecMsg = std::string(", flag: ") + err.suspect.flag->_longFlag + " " + err.suspect.flag->_shortFlag;
    else if (err.suspect.type == ArgParse::Errors::Suspect::ArgType)
        typeSpecMsg = std::string(", arg: ") + err.suspect.arg->_name;

    os << "error: '" << err.message << "', code: " << err.code << typeSpecMsg << ".";
    return os;
}

const std::string ArgParse::error()
{
    std::stringstream error;
    for (auto const& err : _errors)
        error << err << std::endl;
    return error.str();
}

const std::vector<ArgParse::Errors>&ArgParse::errors() const
{
    return _errors;
}

const bool ArgParse::check(const std::string& flagStr)
{
    return (_longFlags.find(flagStr) != _longFlags.end()) || (_shortFlags.find(flagStr) != _shortFlags.end());
}

template<typename T>
const bool ArgParse::checkAndRead(const std::string& flagStr, T* value)
{
    if (!check(flagStr) || !value)
        return false;

    // FIXME: long and short flags also
    const Flag& flag = *_longFlags[flagStr];
    if (!flag.hasValue)
        return false;

    std::stringstream s;
    s << flag.value.str;
    s >> (*value);
    return !s.fail();
}

const Flag& ArgParse::operator[](const std::string& idx)
{
    std::string flagStr(idx);
    switch (mapParamType(flagStr)) {
    case ParamType::ArgType:
        return Flag::WrongFlag;
    case ParamType::ShortFlagsType:
        flagStr = flagStr.substr(0, 2);
        // Fall through.
    case ParamType::ShortFlagType:
        if (_shortFlags.find(flagStr) == _shortFlags.end())
            return def(Flag("", flagStr));
        return *(_shortFlags[flagStr]);
    case ParamType::LongFlagWithEqType:
        if (_longFlags.find(flagStr) == _longFlags.end())
            flagStr = flagStr.substr(0, flagStr.find("="));
        // Fall through.
    case ParamType::LongFlagWithoutEqType:
        if (_longFlags.find(flagStr) == _longFlags.end())
            return def(Flag(flagStr));
        return *(_longFlags[flagStr]);
    default:
        assert(false);
        break;
    };

    return Flag::WrongFlag;
}

Flag const& ArgParse::operator[](const char* idx)
{
    return ArgParse::operator[](std::string(idx));
}

const Arg& ArgParse::operator[](const std::size_t& idx)
{
    return idx < _args.size() ? _args[idx] : Arg::WrongArg;
}

const Arg& ArgParse::operator[](const int idx)
{
    return operator[](std::size_t(idx));
}

void ArgParse::addError(const ArgParse::Errors::Codes& errorCode, const std::string& errorMsg, const ArgParse::Errors::Suspect& suspect)
{
    const Errors ae = { errorCode, errorMsg, suspect };
    _errors.push_back(ae);
}

void ArgParse::addError(const ArgParse::Errors::Codes& errorCode, const std::string& errorMsg, const void* ptr)
{
    addError(errorCode, errorMsg, { Errors::Suspect::GeneralType, ptr });
}

void ArgParse::addError(const ArgParse::Errors::Codes& errorCode, const std::string& errorMsg, const Flag* flag)
{
    addError(errorCode, errorMsg, { Errors::Suspect::FlagType, reinterpret_cast<const void*>(flag) });
}

void ArgParse::addError(const ArgParse::Errors::Codes& errorCode, const std::string& errorMsg, const Arg* arg)
{
    addError(errorCode, errorMsg, { Errors::Suspect::ArgType, reinterpret_cast<const void*>(arg) });
}

// Value
const bool Value::Required = true;

Value::Value(const Value& v)
    : isRequired(v.isRequired)
    , isSet(v.isSet)
    , str(v.str)
    , _chooseList(v._chooseList)
    , _name(v._name)
    , _description(v._description)
{
}

Value::Value(const std::string& defaultValue, const bool& require, const std::string& name, const std::string& description)
    : isRequired(require)
    , isSet(false)
    , str(defaultValue)
    , _name(name)
    , _description(description)
{
}

Value::Value(const ChooseList& chooseList, const bool& require, const std::string& name, const std::string& description)
    : Value(chooseList.size() ? *(chooseList.begin()) : "", require, name, description)
{
    for(auto choose : chooseList)
        _chooseList.push_back(choose);
}

// Flag

const Flag Flag::WrongFlag = Flag();

Flag::Flag(const Flag& f)
    : isSet(f.isSet)
    , isDefined(f.isDefined)
    , hasValue(f.hasValue)
    , value(f.value)
    , _longFlag(f._longFlag)
    , _shortFlag(f._shortFlag)
    , _description(f._description)
    , _callBackFunc(f._callBackFunc)
{
}

Flag::Flag(const std::string& lFlag, const std::string& sFlag, const std::string& dscrptn)
    : isSet(false)
    , isDefined(!lFlag.empty() || !sFlag.empty())
    , hasValue(false)
    , value(Value())
    , _longFlag(lFlag)
    , _shortFlag(sFlag)
    , _description(dscrptn)
    , _callBackFunc(nullptr)
{
}

Flag::Flag(const std::string& lFlag, const std::string& sFlag, const std::string& dscrptn, const Value definedValue)
    : Flag(lFlag, sFlag, dscrptn)
{
    hasValue = true;
    value = definedValue;
}

const bool Flag::isValid() const
{
    if (_longFlag.empty() && _shortFlag.empty())
        return false;

    const bool longFlagValid = (_longFlag.size() > 2) && (_longFlag[0] == '-') && (_longFlag[1] == '-');
    const bool shortFlagValid = (_shortFlag.size() == 2) && (_shortFlag[0] == '-');

    if (longFlagValid || shortFlagValid)
        return true;

    return false;
}

// Arg

const Arg Arg::WrongArg = Arg();

Arg::Arg(const Arg& a)
    : Value((Value)a)
    , isDefined(a.isDefined)
{
}

Arg::Arg(const std::string& name, const std::string& description, const bool require, const Value& defaultValue)
    : Value(defaultValue)
    , isDefined(!name.empty())
{
    isRequired = require;
    _name = name;
    _description = description;
}

Arg::Arg(const Value& value)
    : Arg("", "", false, value)
{
}

} // namespace argparse
