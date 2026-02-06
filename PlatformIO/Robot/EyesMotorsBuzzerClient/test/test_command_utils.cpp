#include <unity.h>
#include "CommandUtils.h"

void test_noteToFreq_known() {
  TEST_ASSERT_EQUAL_INT(262, noteToFreq("C4"));
  TEST_ASSERT_EQUAL_INT(440, noteToFreq("A4"));
  TEST_ASSERT_EQUAL_INT(0, noteToFreq("REST"));
}

void test_parsePair() {
  int f=0, ms=0;
  TEST_ASSERT_TRUE(parsePair("C4,200", f, ms));
  TEST_ASSERT_EQUAL_INT(262, f);
  TEST_ASSERT_EQUAL_INT(200, ms);

  TEST_ASSERT_FALSE(parsePair("BADFORMAT", f, ms));
}

void test_parseMotorPayload() {
  auto r = parseMotorPayload("F,500");
  TEST_ASSERT_EQUAL_STRING("F", r.first.c_str());
  TEST_ASSERT_EQUAL_INT(500, r.second);

  auto r2 = parseMotorPayload("L");
  TEST_ASSERT_EQUAL_STRING("L", r2.first.c_str());
  TEST_ASSERT_EQUAL_INT(0, r2.second);
}

void test_parseGroupedJson() {
  std::string j = "[[\"eyes:angry\",\"audio:C4,200\"],[\"move:F,300\"]]";
  auto groups = parseGroupedJson(j);
  TEST_ASSERT_EQUAL_INT(2, (int)groups.size());
  TEST_ASSERT_EQUAL_INT(2, (int)groups[0].size());
  TEST_ASSERT_EQUAL_STRING("eyes:angry", groups[0][0].c_str());
  TEST_ASSERT_EQUAL_STRING("audio:C4,200", groups[0][1].c_str());
  TEST_ASSERT_EQUAL_INT(1, (int)groups[1].size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_noteToFreq_known);
  RUN_TEST(test_parsePair);
  RUN_TEST(test_parseMotorPayload);
  RUN_TEST(test_parseGroupedJson);
  return UNITY_END();
}
