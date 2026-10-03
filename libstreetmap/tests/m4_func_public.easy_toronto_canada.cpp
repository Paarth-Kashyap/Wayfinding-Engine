#include <random>
#include <iostream>
#include <UnitTest++/UnitTest++.h>

#include "StreetsDatabaseAPI.h"
#include "m1.h"
#include "m3.h"
#include "m4.h"

#include "unit_test_util.h"
#include "courier_verify.h"

using ece297test::relative_error;
using ece297test::courier_path_is_legal;


SUITE(easy_toronto_canada_public) {
    TEST(easy_toronto_canada) {
        std::vector<DeliveryInf> deliveries;
        std::vector<IntersectionIdx> depots;
        float turn_penalty;
        std::vector<CourierSubPath> result_path;
        bool is_legal;

        deliveries = {DeliveryInf(171961, 41792), DeliveryInf(145152, 151088), DeliveryInf(96058, 57692), DeliveryInf(173265, 66308), DeliveryInf(52222, 48968), DeliveryInf(131184, 61922), DeliveryInf(110624, 130404), DeliveryInf(44815, 103959), DeliveryInf(105931, 111575), DeliveryInf(54071, 111889), DeliveryInf(34725, 105882), DeliveryInf(153323, 71800), DeliveryInf(37717, 36213), DeliveryInf(174923, 92754), DeliveryInf(83337, 123016), DeliveryInf(191528, 179887), DeliveryInf(104028, 161047), DeliveryInf(135783, 47419), DeliveryInf(132416, 171471), DeliveryInf(175707, 157213), DeliveryInf(150803, 105756), DeliveryInf(122449, 155253), DeliveryInf(186424, 13106), DeliveryInf(156304, 54603), DeliveryInf(136971, 140068)};
        depots = {13, 49589};
        turn_penalty = 30.000000000;
        {
        	ECE297_TIME_CONSTRAINT(50000);
        	
        result_path = travelingCourier(turn_penalty, deliveries, depots);
        }

        is_legal = courier_path_is_legal(deliveries, depots, result_path);
        CHECK(is_legal);

        if(is_legal) {
        	double path_cost = ece297test::compute_courier_path_travel_time(result_path, turn_penalty);
        	std::cout << "QoR easy_toronto_canada: " << path_cost << std::endl;
        } else {
        	std::cout << "QoR easy_toronto_canada: INVALID" << std::endl;
        }

    } //easy_toronto_canada

} //easy_toronto_canada_public

