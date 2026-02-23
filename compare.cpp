#include "compare.h"

#include <vector>
#include <stdexcept>
#include <cctype>
#include <iostream>
#include <unordered_map>
#include <random>
#include <cstdint>
#include <string>

// ---------- Токены ----------

enum class TokenType {
    Identifier, // R1, R2, ...
    Number,     // 123
    Plus,       // +
    Mul,        // * (явное или вставленное неявное умножение)
    LParen,     // (
    RParen      // )
};

struct Token {
    TokenType type;
    std::string text;
};

// ---------- Лексер ----------

static std::vector<Token> TokenizeRaw(const std::string& expr) {
    std::vector<Token> tokens;
    std::size_t i = 0;

    while (i < expr.size()) {
        char c = expr[i];

        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }

        if (c == '(') { tokens.push_back({TokenType::LParen, "("}); ++i; }
        else if (c == ')') { tokens.push_back({TokenType::RParen, ")"}); ++i; }
        else if (c == '+') { tokens.push_back({TokenType::Plus, "+"}); ++i; }
        else if (c == '*') { tokens.push_back({TokenType::Mul, "*"}); ++i; }
        else if (c == 'R') {
            std::size_t j = i + 1;
            while (j < expr.size() && std::isdigit(static_cast<unsigned char>(expr[j]))) ++j;
            if (j == i + 1) throw std::runtime_error("Invalid identifier (R without digits)");
            tokens.push_back({TokenType::Identifier, expr.substr(i, j - i)});
            i = j;
        }
        else if (std::isdigit(static_cast<unsigned char>(c))) {
            std::size_t j = i + 1;
            while (j < expr.size() && std::isdigit(static_cast<unsigned char>(expr[j]))) ++j;
            tokens.push_back({TokenType::Number, expr.substr(i, j - i)});
            i = j;
        }
        else {
            throw std::runtime_error(std::string("Unexpected character in expression: ") + c);
        }
    }

    return tokens;
}

// ---------- Вставка неявного умножения ----------

static bool CanBeLeftOfMul(TokenType t) {
    return t == TokenType::Identifier || t == TokenType::Number || t == TokenType::RParen;
}
static bool CanBeRightOfMul(TokenType t) {
    return t == TokenType::Identifier || t == TokenType::Number || t == TokenType::LParen;
}

static std::vector<Token> InsertImplicitMultiplication(const std::vector<Token>& raw) {
    std::vector<Token> result;
    result.reserve(raw.size() * 2);

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

    output.reserve(tokens.size());
    opStack.reserve(tokens.size());

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
                if (opStack.empty()) throw std::runtime_error("Mismatched parentheses");
                opStack.pop_back(); // remove '('
                break;
        }
    }

    while (!opStack.empty()) {
        if (opStack.back().type == TokenType::LParen || opStack.back().type == TokenType::RParen)
            throw std::runtime_error("Mismatched parentheses at the end");
        output.push_back(opStack.back());
        opStack.pop_back();
    }

    return output;
}

// ---------- Rk -> k ----------

static long long identifierToIndex(const std::string& name) {
    // name starts with 'R' and has digits
    long long v = 0;
    for (std::size_t i = 1; i < name.size(); ++i) {
        char c = name[i];
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw std::runtime_error("Invalid identifier: " + name);
        v = v * 10 + (c - '0');
    }
    return v;
}

// ---------- Надёжное вычисление: случайные подстановки (uint64_t) ----------

static uint64_t getRandomForR(long long idx,
                             std::unordered_map<long long, uint64_t>& mp,
                             std::mt19937_64& rng)
{
    auto it = mp.find(idx);
    if (it != mp.end()) return it->second;

    uint64_t x = rng();
    // не даём нулю слишком часто портить произведения
    if (x == 0) x = 0x9e3779b97f4a7c15ULL;
    mp.emplace(idx, x);
    return x;
}

static uint64_t EvalRPN64(const std::vector<Token>& rpn,
                         std::unordered_map<long long, uint64_t>& mp,
                         std::mt19937_64& rng)
{
    std::vector<uint64_t> st;
    st.reserve(rpn.size());

    for (const auto& tok : rpn) {
        if (tok.type == TokenType::Identifier) {
            long long idx = identifierToIndex(tok.text);
            st.push_back(getRandomForR(idx, mp, rng));
        }
        else if (tok.type == TokenType::Number) {
            // числа считаем константами
            uint64_t v = static_cast<uint64_t>(std::stoull(tok.text));
            st.push_back(v);
        }
        else if (tok.type == TokenType::Plus || tok.type == TokenType::Mul) {
            if (st.size() < 2) throw std::runtime_error("Not enough operands");
            uint64_t b = st.back(); st.pop_back();
            uint64_t a = st.back(); st.pop_back();
            uint64_t res = (tok.type == TokenType::Plus) ? (a + b) : (a * b);
            st.push_back(res);
        }
        else {
            throw std::runtime_error("Unexpected token in RPN");
        }
    }

    if (st.size() != 1) throw std::runtime_error("Invalid RPN (stack size != 1)");
    return st.back();
}

static std::vector<Token> CompileToRPN(const std::string& expr) {
    auto raw    = TokenizeRaw(expr);
    auto tokens = InsertImplicitMultiplication(raw);
    return ToRPN(tokens);
}

void CompareExpressions(const std::string& expr1,
                        const std::string& expr2)
{
    try {
        // Компилируем в RPN один раз (быстрее и надёжнее)
        const auto rpn1 = CompileToRPN(expr1);
        const auto rpn2 = CompileToRPN(expr2);

        // Кол-во прогонов: 8 обычно хватает с огромным запасом
        constexpr int TRIES = 8;

        bool equal = true;

        for (int t = 0; t < TRIES; ++t) {
            // разные seed'ы => независимые проверки
            std::mt19937_64 rng(0xC0FFEEULL + 0x9e3779b97f4a7c15ULL * (uint64_t)(t + 1));
            std::unordered_map<long long, uint64_t> mp;
            mp.reserve(256);

            uint64_t v1 = EvalRPN64(rpn1, mp, rng);
            uint64_t v2 = EvalRPN64(rpn2, mp, rng);

            if (v1 != v2) {
                equal = false;
                break;
            }
        }

        if (equal) {
            std::cout << "Result: expressions are VERY LIKELY EQUAL (randomized test).\n\n";
        } else {
            std::cout << "Result: expressions are DIFFERENT.\n\n";
        }
    }
    catch (const std::exception& ex) {
        std::cout << "Error while evaluating expressions: " << ex.what() << "\n";
    }
}
