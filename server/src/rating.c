#include "../include/rating.h"

#include <math.h>
#include <string.h>

double rating_expected_score(int rating_a, int rating_b) {
    return 1.0 / (1.0 + pow(10.0, (rating_b - rating_a) / 400.0));
}

rating_change_t rating_calculate(int red_rating, int black_rating, const char* result, int k_factor) {
    rating_change_t change = {0, 0};

    double red_expected = rating_expected_score(red_rating, black_rating);
    double black_expected = 1.0 - red_expected;

    double red_score, black_score;

    if (strcmp(result, "red_win") == 0) {
        red_score = 1.0;
        black_score = 0.0;
    } else if (strcmp(result, "black_win") == 0) {
        red_score = 0.0;
        black_score = 1.0;
    } else {
        red_score = 0.5;
        black_score = 0.5;
    }

    change.red_change = (int)round(k_factor * (red_score - red_expected));
    change.black_change = (int)round(k_factor * (black_score - black_expected));

    return change;
}
