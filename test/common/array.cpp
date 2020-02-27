// Copyright © 2020 Giorgio Audrito. All Rights Reserved.

#include <array>

#include "gtest/gtest.h"

#include "lib/common/array.hpp"

std::array<int,3> gett() {
    return {3,4,4};
}

std::array<int,3> getu() {
    return {1,2,0};
}

std::array<int,3> getv() {
    return {0,1,1};
}

std::array<int,3> getw() {
    return {1,3,1};
}


TEST(ArrayTest, Addition) {
    std::array<int,3> t = gett();
    std::array<int,3> u = getu();
    std::array<int,3> v = getv();
    std::array<int,3> w = getw();
    EXPECT_EQ(w,    u  +   v  );
    EXPECT_EQ(w,    u  +getv());
    EXPECT_EQ(w, getu()+   v  );
    EXPECT_EQ(w, getu()+getv());
    EXPECT_EQ(t,    v  +   3  );
    EXPECT_EQ(t, getv()+   3  );
    EXPECT_EQ(t,    3  +   v  );
    EXPECT_EQ(t,    3  +getv());
    u += v;
    EXPECT_EQ(w, u);
    v += 3;
    EXPECT_EQ(t, v);
}

TEST(ArrayTest, Subtraction) {
    std::array<int,3> t = gett();
    std::array<int,3> u = getu();
    std::array<int,3> v = getv();
    std::array<int,3> w = getw();
    EXPECT_EQ(v,    w  -   u  );
    EXPECT_EQ(v,    w  -getu());
    EXPECT_EQ(v, getw()-   u  );
    EXPECT_EQ(v, getw()-getu());
    EXPECT_EQ(t,    v  +   3  );
    EXPECT_EQ(t, getv()+   3  );
    EXPECT_EQ(t,    3  +   v  );
    EXPECT_EQ(t,    3  +getv());
    w -= u;
    EXPECT_EQ(v, w);
    t -= 3;
    EXPECT_EQ(v, t);
}

TEST(ArrayTest, Multiplication) {
    std::array<int,2> t = {1,2};
    std::array<int,2> u = {2,4};
    std::array<int,2> v = {3,1};
    std::array<int,2> w = {3,4};
    EXPECT_EQ(u,  t * 2);
    EXPECT_EQ(u,  2 * t);
    EXPECT_EQ(10, u * v);
    EXPECT_EQ(5,  norm(w));
    t *= 2;
    EXPECT_EQ(u, t);
}
