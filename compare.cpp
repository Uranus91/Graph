#include "compare.h"

#include <vector>
#include <stdexcept>
#include <cctype>
#include <iostream>

// ---------- Токены ----------

enum class TokenType {
    Identifier, // R1, R2, ...
    Number,     // 123 (на всякий случай поддерживаем)
    Plus,       // +
    Mul,        // * (явное или вставленное неявное умножение)
    LParen,     // (
    RParen      // )
};

struct Token {
    TokenType type;
    std::string text;
};

// ---------- Лексер: разбираем строку в токены (без неявного умножения) ----------

static std::vector<Token> TokenizeRaw(const std::string& expr) {
    std::vector<Token> tokens;
    std::size_t i = 0;

    while (i < expr.size()) {
        char c = expr[i];

        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }

        if (c == '(') {
            tokens.push_back({TokenType::LParen, "("});
            ++i;
        } else if (c == ')') {
            tokens.push_back({TokenType::RParen, ")"});
            ++i;
        } else if (c == '+') {
            tokens.push_back({TokenType::Plus, "+"});
            ++i;
        } else if (c == '*') {
            tokens.push_back({TokenType::Mul, "*"});
            ++i;
        } else if (c == 'R') {
            // Identifier: R + цифры
            std::size_t j = i + 1;
            while (j < expr.size() && std::isdigit(static_cast<unsigned char>(expr[j]))) {
                ++j;
            }
            tokens.push_back({TokenType::Identifier, expr.substr(i, j - i)});
            i = j;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            // Числовая константа
            std::size_t j = i + 1;
            while (j < expr.size() && std::isdigit(static_cast<unsigned char>(expr[j]))) {
                ++j;
            }
            tokens.push_back({TokenType::Number, expr.substr(i, j - i)});
            i = j;
        } else {
            throw std::runtime_error(std::string("Unexpected character in expression: ") + c);
        }
    }

    return tokens;
}

// ---------- Вставка неявного умножения ----------
// R1R2  -> R1 * R2
// R1(R2+R3) -> R1 * (R2+R3)
// (R1+R2)R3 -> (R1+R2) * R3

static bool CanBeLeftOfMul(TokenType t) {
    return t == TokenType::Identifier ||
           t == TokenType::Number     ||
           t == TokenType::RParen;
}

static bool CanBeRightOfMul(TokenType t) {
    return t == TokenType::Identifier ||
           t == TokenType::Number     ||
           t == TokenType::LParen;
}

static std::vector<Token> InsertImplicitMultiplication(const std::vector<Token>& raw) {
    std::vector<Token> result;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        result.push_back(raw[i]);
        if (i + 1 < raw.size()) {
            TokenType t1 = raw[i].type;
            TokenType t2 = raw[i + 1].type;
            if (CanBeLeftOfMul(t1) && CanBeRightOfMul(t2)) {
                result.push_back({TokenType::Mul, "*"});
            }
        }
    }
    return result;
}

// ---------- Shunting-yard: инфикс → RPN ----------

static int precedence(TokenType t) {
    switch (t) {
        case TokenType::Plus: return 1;
        case TokenType::Mul:  return 2;
        default:              return 0;
    }
}

static bool isOperator(TokenType t) {
    return (t == TokenType::Plus || t == TokenType::Mul);
}

static std::vector<Token> ToRPN(const std::vector<Token>& tokens) {
    std::vector<Token> output;
    std::vector<Token> opStack;

    for (const auto& tok : tokens) {
        switch (tok.type) {
            case TokenType::Identifier:
            case TokenType::Number:
                output.push_back(tok);
                break;

            case TokenType::Plus:
            case TokenType::Mul: {
                while (!opStack.empty() &&
                       isOperator(opStack.back().type) &&
                       precedence(opStack.back().type) >= precedence(tok.type)) {
                    output.push_back(opStack.back());
                    opStack.pop_back();
                }
                opStack.push_back(tok);
                break;
            }

            case TokenType::LParen:
                opStack.push_back(tok);
                break;

            case TokenType::RParen:
                while (!opStack.empty() && opStack.back().type != TokenType::LParen) {
                    output.push_back(opStack.back());
                    opStack.pop_back();
                }
                if (opStack.empty()) {
                    throw std::runtime_error("Mismatched parentheses");
                }
                opStack.pop_back(); // удаляем '('
                break;
        }
    }

    while (!opStack.empty()) {
        if (opStack.back().type == TokenType::LParen ||
            opStack.back().type == TokenType::RParen) {
            throw std::runtime_error("Mismatched parentheses at the end");
        }
        output.push_back(opStack.back());
        opStack.pop_back();
    }

    return output;
}

// ---------- Преобразование идентификатора Rk → значение k ----------

static long long identifierToValue(const std::string& name) {
    if (name.empty() || name[0] != 'R' || name.size() == 1) {
        throw std::runtime_error("Invalid identifier: " + name);
    }
    // берем всё после 'R'
    long long v = 0;
    for (std::size_t i = 1; i < name.size(); ++i) {
        char c = name[i];
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            throw std::runtime_error("Invalid identifier: " + name);
        }
        v = v * 10 + (c - '0');
    }
    return v; // R1→1, R2→2, ...
}

// ---------- Вычисление RPN ----------

static long long EvalRPN(const std::vector<Token>& rpn) {
    std::vector<long long> st;
    st.reserve(rpn.size());

    for (const auto& tok : rpn) {
        if (tok.type == TokenType::Identifier) {
            long long v = identifierToValue(tok.text);
            st.push_back(v);
        } else if (tok.type == TokenType::Number) {
            long long v = std::stoll(tok.text);
            st.push_back(v);
        } else if (tok.type == TokenType::Plus || tok.type == TokenType::Mul) {
            if (st.size() < 2) {
                throw std::runtime_error("Not enough operands for operator");
            }
            long long b = st.back(); st.pop_back();
            long long a = st.back(); st.pop_back();
            long long res = (tok.type == TokenType::Plus) ? (a + b) : (a * b);
            st.push_back(res);
        } else {
            throw std::runtime_error("Unexpected token in RPN");
        }
    }

    if (st.size() != 1) {
        throw std::runtime_error("Invalid RPN expression (stack size != 1)");
    }

    return st.back();
}

// ---------- Внешняя функция: всё вместе ----------

static long long EvaluateExpressionWithRnAsNumbers(const std::string& expr) {
    auto raw    = TokenizeRaw(expr);
    auto tokens = InsertImplicitMultiplication(raw);
    auto rpn    = ToRPN(tokens);
    return EvalRPN(rpn);
}

void CompareExpressions(const std::string& expr1,
                        const std::string& expr2)
{
    try {
        long long val1 = EvaluateExpressionWithRnAsNumbers(expr1);
        long long val2 = EvaluateExpressionWithRnAsNumbers(expr2);

        std::cout << "First expression value:  " << val1 << "\n";
        std::cout << "Second expression value: " << val2 << "\n";

        if (val1 == val2) {
            std::cout << "Result: expressions are EQUAL.\n\n";
        } else {
            std::cout << "Result: expressions are DIFFERENT.\n\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "Error while evaluating expressions: " << ex.what() << "\n";
    }
}
