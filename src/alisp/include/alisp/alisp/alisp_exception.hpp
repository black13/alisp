#pragma once

#include "alisp/alisp/alisp_common.hpp"
#include "alisp/alisp/alisp_object.hpp"


namespace alisp
{


enum class SignalTag {
    UNKNOWN,

    PARSER,
    EVAL,
    INVALID_ARGUMENTS,
    ENV,
    USER
    
};


struct al_exception : public std::runtime_error
{
    al_exception(std::string what, SignalTag tag) : runtime_error(what), m_tag(tag) {}
    

    SignalTag tag() const { return m_tag; }
    const std::string& name() const { return m_signal_name; }
    
  protected:
    SignalTag m_tag;
    std::string m_signal_name;

};


struct signal_exception : public al_exception
{

    signal_exception(ALObjectPtr sym, ALObjectPtr list) :
        al_exception(format(sym, list), SignalTag::USER), m_sym(sym), m_list(list) {

        m_signal_name = m_sym->to_string();
    }

  private:
    ALObjectPtr m_sym;
    ALObjectPtr m_list;

    static std::string format(ALObjectPtr sym, ALObjectPtr list){
        std::ostringstream ss;
        ss << "Signal error <" << dump(sym) << "> :";
        ss << dump(list);
        ss << '\n';
        return ss.str();
    }
};


class parse_exception : public al_exception
{
  public:

    parse_exception(const std::string& t_why, const FileLocation& t_where, const std::string& t_input) :
        al_exception(format(t_why, t_where, t_input), SignalTag::PARSER)
    {
        m_signal_name = "parser-signal";
    }


  private:
    static std::string format(const std::string& t_why, const FileLocation& t_where, const std::string& t_input)
    {
        const static int LINE_CONTEXT = 1;
        std::ostringstream ss;

        ss << "\t" << "In file \'" << t_where.file << '\'' << ": " << "line: " << t_where.line << ", col: " << t_where.col << '\n';
        ss << "\t" << t_why << "\n";

        auto lines = utility::split(t_input, '\n');

        auto start_index = static_cast<int>(t_where.line) - LINE_CONTEXT < 0 ? 0 : static_cast<int>(t_where.line) - LINE_CONTEXT;
        auto end_index = (static_cast<int>(t_where.line) + LINE_CONTEXT) > static_cast<int>(std::size(lines)) ?
            static_cast<int>(std::size(lines)) : static_cast<int>(std::size(lines)) + LINE_CONTEXT;

        for (auto i = static_cast<size_t>(start_index); i < static_cast<size_t>(end_index); ++i) {
            ss << "\t" <<  i << " |" << "\t" << lines[i] << "\n";
        }

        return ss.str();
    }

};


class environment_error : public al_exception
{
  public:
    environment_error(const std::string& t_why) : al_exception(t_why, SignalTag::ENV) {
        m_signal_name = "environment-signal";
    }

};


class eval_error : public al_exception
{
  public:
    eval_error(const std::string& t_why) : al_exception(t_why, SignalTag::EVAL) {
        m_signal_name = "eval-signal";
    }

};



}
