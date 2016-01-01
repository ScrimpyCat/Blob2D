//This file is automatically generated, modifications will be lost!
#include "ExpressionSetup.h"
#include "ExpressionEvaluator.h"

#include "ControlFlowExpressions.h"
#include "EqualityExpressions.h"
#include "IOExpressions.h"
#include "MacroExpressions.h"
#include "MathExpressions.h"
#include "ProjectExpressions.h"
#include "StateExpressions.h"
#include "WindowExpressions.h"

void CCExpressionSetup(void)
{
    CCExpressionEvaluatorRegister("begin", CCControlFlowExpressionBegin);
    CCExpressionEvaluatorRegister("if", CCControlFlowExpressionBranch);
    CCExpressionEvaluatorRegister("=", CCEqualityExpressionEqual);
    CCExpressionEvaluatorRegister("!=", CCEqualityExpressionNotEqual);
    CCExpressionEvaluatorRegister("<=", CCEqualityExpressionLessThanEqual);
    CCExpressionEvaluatorRegister(">=", CCEqualityExpressionGreaterThanEqual);
    CCExpressionEvaluatorRegister("<", CCEqualityExpressionLessThan);
    CCExpressionEvaluatorRegister(">", CCEqualityExpressionGreaterThan);
    CCExpressionEvaluatorRegister("print", CCIOExpressionPrint);
    CCExpressionEvaluatorRegister("quote", CCMacroExpressionQuote);
    CCExpressionEvaluatorRegister("unquote", CCMacroExpressionUnquote);
    CCExpressionEvaluatorRegister("+", CCMathExpressionAddition);
    CCExpressionEvaluatorRegister("-", CCMathExpressionSubtract);
    CCExpressionEvaluatorRegister("*", CCMathExpressionMultiply);
    CCExpressionEvaluatorRegister("/", CCMathExpressionDivide);
    CCExpressionEvaluatorRegister("min", CCMathExpressionMinimum);
    CCExpressionEvaluatorRegister("max", CCMathExpressionMaximum);
    CCExpressionEvaluatorRegister("random", CCMathExpressionRandom);
    CCExpressionEvaluatorRegister("game", CCProjectExpressionGame);
    CCExpressionEvaluatorRegister("state!", CCStateExpressionCreateState);
    CCExpressionEvaluatorRegister("enum!", CCStateExpressionCreateEnum);
    CCExpressionEvaluatorRegister("fun!", CCStateExpressionCreateFunction);
    CCExpressionEvaluatorRegister("window-percent-width", CCWindowExpressionPercentageWidth);
    CCExpressionEvaluatorRegister("window-percent-height", CCWindowExpressionPercentageHeight);
    CCExpressionEvaluatorRegister("window-width", CCWindowExpressionWidth);
    CCExpressionEvaluatorRegister("window-height", CCWindowExpressionHeight);
}
