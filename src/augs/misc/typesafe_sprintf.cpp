#if BUILD_UNIT_TESTS
#include <catch.hpp>

#include "augs/math/vec2.h"
#include "augs/misc/typesafe_sprintf.h"

TEST_CASE("Type-safe sprintf", "Several tests") {
	// corner cases
	REQUIRE("%x%x%%%%f%c%ddasdfs" == typesafe_sprintf("%x%x%%%%f%c%ddasdfs"));
	REQUIRE(L"%x%x%%%%f%c%ddasdfs" == typesafe_sprintf(L"%x%x%%%%f%c%ddasdfs"));
	REQUIRE("2,3,%x" == typesafe_sprintf("%x,%x,%x", 2, 3));
	REQUIRE(L"2,3,%x" == typesafe_sprintf(L"%x,%x,%x", 2, 3));
	REQUIRE("2,3,5" == typesafe_sprintf("%x,%x,%x", 2, 3, 5, 7, 8, 6, 5, 435, 534, 324534, "nice"));
	REQUIRE(L"2,3,5" == typesafe_sprintf(L"%x,%x,%x", 2, 3, 5, 7, 8, 6, 5, 435, 534, 324534, L"nice"));

	REQUIRE("1,2,3:4" == typesafe_sprintf("%x,%x,%x:%x", 1, 2, 3, 4));
	REQUIRE("abc,2,3:def" == typesafe_sprintf("%x,%x,%x:%x", "abc", 2, 3, "def"));
	REQUIRE("abc,2.55,3.14:def" == typesafe_sprintf("%x,%x,%x:%x", "abc", 2.55, 3.14f, "def"));

	vec2 test(123, 412);

	REQUIRE("Vector is equal to: (123;412)" == typesafe_sprintf("Vector is equal to: %x", test));
	REQUIRE("Vector is equal to: (123.00;412.00)" == typesafe_sprintf("Vector is equal to: (%f2;%f2)", test.x, test.y));

	int errid = 1282;
	std::string location = "augs::create";

	REQUIRE("OpenGL error 1282 in augs::create" == typesafe_sprintf("OpenGL error %x in %x", errid, location));
}
#endif
