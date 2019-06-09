#pragma once

#include <string>
#include <sstream>
#include <variant>


namespace alisp
{

enum class TokenType
{
    ID,
    STRING,
    NUMBER,
    REAL_NUMBER,
    
    LEFT_BRACE,
    RIGHT_BRACE,

    LEFT_BRACKET,
    RIGHT_BRACKET,

    AMPER,
    QUOTATION_MARKS,
    QUOTE,
    BACKQUOTE,
    AT,
    COLON
};

#define ENUM_CASE(type) case TokenType::type :   \
    return std::string{#type};                   \
    break

std::string get_token_str(TokenType type)
{
    switch (type) {
        ENUM_CASE(ID);
        ENUM_CASE(STRING);
        ENUM_CASE(NUMBER);
        ENUM_CASE(REAL_NUMBER);
        
        ENUM_CASE(AT);
        ENUM_CASE(COLON);
        ENUM_CASE(BACKQUOTE);
        ENUM_CASE(QUOTE);
        ENUM_CASE(QUOTATION_MARKS);
        
        ENUM_CASE(LEFT_BRACE);
        ENUM_CASE(RIGHT_BRACE);
        ENUM_CASE(RIGHT_BRACKET);
        ENUM_CASE(LEFT_BRACKET);
        ENUM_CASE(AMPER);
      default : return std::string{"UNKNOWN"};
    }
}
#undef ENUM_CASE



class Token;
inline std::ostream& operator<<(std::ostream& os, const Token& x);

class Token
{
  public:


    Token(TokenType type_) : type(type_), content(){};

    template<typename T>
    Token(TokenType type_, T content_) : type(type_){
        content = std::move(content_);
    }

    std::string str() const
    {
        std::ostringstream os;
        os << *this;
        return os.str();
    };


    template<typename T>
    T getContentAs() const
    {
        return std::get<T>(this->content);
    }
    
    TokenType getType() const
    {
        return type;
    };

    friend inline std::ostream& operator<<(std::ostream& os, const Token& x);
  private:
    TokenType type;
    std::variant<int, float, std::string> content;
};

inline std::ostream& operator<<(std::ostream& os, const Token& x)
{
    os << "<Token (";
    os << get_token_str(x.getType());
    os << ") ";

    if (const auto val1 = std::get_if<0>(&x.content)) 
        os << *val1;

    else if (const auto val2 = std::get_if<1>(&x.content)) 
        os << *val2;

    else if (const auto val3 = std::get_if<2>(&x.content)) 
        os << *val3;
    
    os << ">";
    return os;
}



}  // namespace alisp
