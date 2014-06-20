#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "catcierge_fsm.h"
#include "minunit.h"
#include "catcierge_test_config.h"
#include "catcierge_test_helpers.h"
#include "catcierge_args.h"
#include "catcierge_types.h"
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "catcierge_test_common.h"

static char *run_success_tests()
{
	int i;
	int j;
	catcierge_grb_t grb;
	catcierge_args_t *args = &grb.args;

	catcierge_grabber_init(&grb);

	args->saveimg = 0;
	args->matcher_type = MATCHER_HAAR;
	args->haar.cascade = CATCIERGE_CASCADE;

	if (catcierge_haar_matcher_init(&grb.haar, &args->haar))
	{
		return "Failed to init haar matcher";
	}

	catcierge_haar_matcher_print_settings(&args->haar);

	catcierge_set_state(&grb, catcierge_state_waiting);

	for (j = 6; j <= 9; j++)
	{
		catcierge_test_STATUS("Test series %d", j);

		// This is the initial image that obstructs the frame
		// and triggers the matching.
		load_test_image_and_run(&grb, j, 1);
		mu_assert("Expected MATCHING state", (grb.state == catcierge_state_matching));
	
		// Match against 4 pictures, and decide the lockout status.
		for (i = 1; i <= 4; i++)
		{
			load_test_image_and_run(&grb, j, i);
		}
	
		mu_assert("Expected KEEP OPEN state", (grb.state == catcierge_state_keepopen));

		load_test_image_and_run(&grb, 1, 5);
		mu_assert("Expected WAITING state", (grb.state == catcierge_state_waiting));
	}

	catcierge_haar_matcher_destroy(&grb.haar);
	catcierge_grabber_destroy(&grb);

	return NULL;
}

static char *run_failure_tests(catcierge_haar_prey_method_t prey_method)
{
	int i;
	int j;
	catcierge_grb_t grb;
	catcierge_args_t *args = &grb.args;

	catcierge_grabber_init(&grb);

	args->saveimg = 0;
	args->matcher_type = MATCHER_HAAR;
	args->ok_matches_needed = 3;

	catcierge_haar_matcher_args_init(&args->haar);
	args->haar.prey_method = prey_method;
	args->haar.prey_steps = 2;
	args->haar.cascade = CATCIERGE_CASCADE;

	if (catcierge_haar_matcher_init(&grb.haar, &args->haar))
	{
		return "Failed to init haar matcher";
	}

	catcierge_haar_matcher_print_settings(&args->haar);

	catcierge_set_state(&grb, catcierge_state_waiting);

	for (j = 10; j <= 14; j++)
	{
		catcierge_test_STATUS("Test series %d", j);

		// This is the initial image that obstructs the frame
		// and triggers the matching.
		load_test_image_and_run(&grb, j, 1);
		mu_assert("Expected MATCHING state", (grb.state == catcierge_state_matching));
	
		// Match against 4 pictures, and decide the lockout status.
		for (i = 1; i <= 4; i++)
		{
			load_test_image_and_run(&grb, j, i);
		}

		mu_assert("Expected LOCKOUT state", (grb.state == catcierge_state_lockout));

		load_test_image_and_run(&grb, 1, 5);
		mu_assert("Expected WAITING state", (grb.state == catcierge_state_waiting));
	}

	catcierge_haar_matcher_destroy(&grb.haar);
	catcierge_grabber_destroy(&grb);

	return NULL;
}

int TEST_catcierge_fsm_haar_matcher(int argc, char **argv)
{
	char *e = NULL;
	int ret = 0;
	catcierge_test_HEADLINE("TEST_catcierge_fsm_haar_matcher");

	catcierge_haar_matcher_usage();

	CATCIERGE_RUN_TEST((e = run_success_tests()),
		"Run success tests. Without obstruct",
		"Success match without obstruct", &ret);

	CATCIERGE_RUN_TEST((e = run_failure_tests(PREY_METHOD_NORMAL)),
		"Run failure tests. Normal prey matching",
		"Failure tests with Normal prey matching", &ret);

	CATCIERGE_RUN_TEST((e = run_failure_tests(PREY_METHOD_ADAPTIVE)),
		"Run failure tests. Adaptive prey matching",
		"Failure tests with Adaptive prey matching", &ret);

	return ret;
}
