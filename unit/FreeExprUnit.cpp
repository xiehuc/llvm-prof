#include <gtest/gtest.h>
#include <memory>
#include <functional>

#include "FreeExpression.h"

TEST(FreeExpr, BadConstruct)
{
   EXPECT_EQ(FreeExpression::Construct("bad_construct"), nullptr);
   EXPECT_EQ(FreeExpression::Construct(""), nullptr);
}

TEST(FreeExpr, BadParam)
{
   std::unique_ptr<FreeExpression> linear(FreeExpression::Construct("linear"));
   EXPECT_EQ(linear->init_param(""), 0);
   EXPECT_EQ(linear->init_param("n"), 0);
   EXPECT_EQ(linear->init_param("k="), 0);
   EXPECT_EQ(linear->init_param("=0.5"), 0);
   EXPECT_EQ(linear->init_param("k=b"), 0);
   FreeExpression& expr = *linear;
   EXPECT_EQ(expr(0), 0);
   EXPECT_EQ(expr(1), 0);
}

TEST(FreeExpr, AbnormalParam)
{
   std::unique_ptr<FreeExpression> linear(FreeExpression::Construct("linear"));
   EXPECT_EQ(linear->init_param("b=6.59372E6"), 1);
   FreeExpression& expr = *linear;
   EXPECT_EQ(expr(0), 6.59372E6);
}
