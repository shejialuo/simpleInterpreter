#include "evaluator.hpp"
#include "lexer.hpp"
#include "object.hpp"
#include "parser.hpp"
#include "spdlog/spdlog.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

Evaluator evaluator{};

std::unique_ptr<Object> testEval(const std::string &input);
bool testIntegerObject(Object *object, int64_t expected);
bool testBooleanObject(Object *object, bool expected);
bool testNullObject(Object *object);

std::unique_ptr<Object> testEval(const std::string &input) {
  Lexer lexer{input};
  Parser parser{&lexer};

  auto program = parser.parseProgram();

  return std::move(evaluator.eval(program.get()));
}

bool testIntegerObject(Object *object, int64_t expected) {
  Integer *integer = dynamic_cast<Integer *>(object);

  if (integer == nullptr) {
    spdlog::error("object is not Integer");
    return false;
  }

  if (integer->value != expected) {
    spdlog::error("object has wrong value. got={}, want={}", integer->value, expected);
    return false;
  }

  return true;
}

bool testBooleanObject(Object *object, bool expected) {
  Boolean *boolean = dynamic_cast<Boolean *>(object);

  if (boolean == nullptr) {
    spdlog::error("object is not Boolean");
    return false;
  }

  if (boolean->value != expected) {
    spdlog::error("object has wrong value. got={}, want={}", boolean->value, expected);
    return false;
  }

  return true;
}

bool testNullObject(Object *object) {
  if (object != nullptr) {
    spdlog::error("object is not nullptr");
    return false;
  }
  return true;
}

TEST(Evaluator, TestEvalIntegerExpression) {
  struct TestData {
    std::string input;
    int64_t expected;

    TestData(const std::string &s, int64_t v) : input{s}, expected{v} {}
  };

  std::vector<TestData> tests{
      {"5", 5},
      {"10", 10},
      {"-5", -5},
      {"-10", -10},
      {"5 + 5 + 5 + 5 - 10", 10},
      {"2 * 2 * 2 * 2 * 2", 32},
      {"-50 + 100 + -50", 0},
      {"5 * 2 + 10", 20},
      {"5 + 2 * 10", 25},
      {"20 + 2 * -10", 0},
      {"50 / 2 * 2 + 10", 60},
      {"2 * (5 + 10)", 30},
      {"3 * 3 * 3 + 10", 37},
      {"3 * (3 * 3) + 10", 37},
      {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
  };

  for (auto &&test : tests) {
    auto evaluated = testEval(test.input);
    if (!testIntegerObject(evaluated.get(), test.expected)) {
      FAIL();
    }
  }
}

TEST(Evaluator, TestEvalBooleanExpression) {
  struct TestData {
    std::string input;
    bool expected;

    TestData(const std::string &s, bool v) : input{s}, expected{v} {}
  };

  std::vector<TestData> tests{
      {"true", true},
      {"false", false},
      {"1 < 2", true},
      {"1 > 2", false},
      {"1 < 1", false},
      {"1 > 1", false},
      {"1 == 1", true},
      {"1 != 1", false},
      {"1 == 2", false},
      {"1 != 2", true},
      {"true == true", true},
      {"false == false", true},
      {"true == false", false},
      {"true != false", true},
      {"false != true", true},
      {"(1 < 2) == true", true},
      {"(1 < 2) == false", false},
      {"(1 > 2) == true", false},
      {"(1 > 2) == false", true},
  };

  for (auto &&test : tests) {
    auto evaluated = testEval(test.input);
    if (!testBooleanObject(evaluated.get(), test.expected)) {
      FAIL();
    }
  }
}

TEST(Evaluator, TestBangOperator) {
  struct TestData {
    std::string input;
    bool expected;

    TestData(const std::string &s, bool v) : input{s}, expected{v} {}
  };

  std::vector<TestData> tests{
      {"!true", false},
      {"!false", true},
      {"!5", false},
      {"!!true", true},
      {"!!false", false},
      {"!!5", true},
  };

  for (auto &&test : tests) {
    auto evaluated = testEval(test.input);
    if (!testBooleanObject(evaluated.get(), test.expected)) {
      FAIL();
    }
  }
}

TEST(Evaluator, TestIfElseExpressions1) {
  struct TestData {
    std::string input;
    int64_t expected;

    TestData(const std::string &s, int64_t v) : input{s}, expected{v} {}
  };

  std::vector<TestData> tests{
      {"if (true) { 10 }", 10},
      {"if (1) { 10 }", 10},
      {"if (1 < 2) { 10 }", 10},
      {"if (1 > 2) { 10 } else { 20 }", 20},
      {"if (1 < 2) { 10 } else { 20 }", 10},
  };

  for (auto &&test : tests) {
    auto evaluated = testEval(test.input);
    if (!testIntegerObject(evaluated.get(), test.expected)) {
      FAIL();
    }
  }
}

TEST(Evaluator, TestIfElseExpressions2) {
  struct TestData {
    std::string input;

    TestData(const std::string &s) : input{s} {}
  };

  std::vector<TestData> tests{
      {"if (false) { 10 }"},
      {"if (0) {10}"},
      {"if (1 > 2) {10}"},
      {"if (!1) {10}"},
  };

  for (auto &&test : tests) {
    auto evaluated = testEval(test.input);
    if (!testNullObject(evaluated.get())) {
      FAIL();
    }
  }
}

TEST(Evaluator, TestReturnStatements) {
  struct TestData {
    std::string input;
    int64_t expected;

    TestData(const std::string &s, int64_t v) : input{s}, expected{v} {}
  };

  std::vector<TestData> tests{
      {"return 10;", 10},
      {"return 10; 9;", 10},
      {"return 2 * 5; 9;", 10},
      {"9; return 2 * 5; 9;", 10},
      {"if (10 > 1) { if (10 > 1) {return 10;} return 1;}", 10},
  };

  for (auto &&test : tests) {
    auto evaluated = testEval(test.input);
    if (!testIntegerObject(evaluated.get(), test.expected)) {
      FAIL();
    }
  }
}

TEST(Evaluator, TestErrorHandling) {
  struct TestData {
    std::string input;
    std::string expectedMessage;

    TestData(const std::string &s, const std::string &m) : input{s}, expectedMessage{m} {}
  };

  std::vector<TestData> tests{
      {
          "5 + true;",
          "type mismatch: INTEGER + BOOLEAN",
      },
      {
          "5 + true; 5;",
          "type mismatch: INTEGER + BOOLEAN",
      },
      {
          "-true;",
          "unknown operator: -BOOLEAN",
      },
      {
          "true + false;",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
      {
          "5; true + false; 5",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
      {
          "if (10 > 1) { true + false; }",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
      {
          "if (10 > 1) { if (10 > 1) {return true + false;} return 1;}",
          "unknown operator: BOOLEAN + BOOLEAN",
      },
  };

  for (auto &&test : tests) {
    auto evaluated = testEval(test.input);

    Error *error = dynamic_cast<Error *>(evaluated.get());

    if (error == nullptr) {
      spdlog::error("no error object returned");
      FAIL();
    }

    if (error->message != test.expectedMessage) {
      spdlog::error("wrong error message. expected='{}', got='{}'", test.expectedMessage, error->message);
      FAIL();
    }
  }
}
