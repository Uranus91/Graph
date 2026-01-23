#ifndef EXPR_COMPARE_H
#define EXPR_COMPARE_H

#include <string>

// Сравнивает два выражения вида:
//   ((R4+R5)(R2+R3)R1 + (R3R4R5+...))
// Подставляет R1=1, R2=2, R3=3, ...
// Ничего не возвращает, выводит результат в std::cout.
void CompareExpressions(const std::string& expr1,
                        const std::string& expr2);

#endif // EXPR_COMPARE_H
