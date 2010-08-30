
#include <check.h>


START_TEST(praster_test)
{

	
} 
END_TEST

START_TEST(read_vector_test)
{
	
}
END_TEST

Suite* projectedraster_suite(void)
{
	Suite *s = suite_create("ProjectedRaster");
	
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, praster_test);
	suite_add_tcase(s, tc_core);

	return s;
}


int main(void)
{
	int number_failed = 0;
	Suite *s = projectedraster_suite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	
	return 0;

}