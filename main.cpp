// main.cpp : Error-Tolerant against soft errors HyperLogLog
//
// This code used Hideaki Ohno's HyperLogLog as a starting point https://github.com/hideo55/cpp-HyperLogLog
//

#include "pch.h"
#include "hyperloglog.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <string>
#include <time.h>

using namespace hll;

/*
 * test_setup stores the command line arguments
 */

struct test_setup {
	uint8_t bit_width;
	int total_keys;
	int total_errors;
	int total_runs;
	int inject_faults;
	int protect_threshold;
	std::string input_keys;
	uint32_t records;
};


/*
 * sets the test setup with the command line arguments
 */

test_setup setup_HLL_test(int argc, char* argv[]) {
	test_setup setup;

	setup.bit_width         = 0;
	setup.total_keys        = -1;
	setup.total_errors      = -1;
	setup.total_runs        = -1;
	setup.input_keys        = "";
	setup.protect_threshold = -1;

	for (int i = 1; i < argc; i++) {
		std::stringstream str;
		str << argv[i];

		std::string argument = str.str();

		// argument 'w' is used for bit width, default value is w=10, valid values are [4, 30]

		if (argument[0] == 'w' || argument[0] == 'W') {
			int len = argument.size();
			setup.bit_width = std::stof(argument.substr(2, len - 1).c_str());
		}

		// argument 'k' is used for the random keys, default value is k=10000
		// valid values are {1000, 5000, 10000, 50000, 100000, 500000, 1000000, 10000000}

		if (argument[0] == 'k' || argument[0] == 'K') {
			int len = argument.size();
			setup.total_keys = std::stof(argument.substr(2, len - 1).c_str());
		}

		// argument 'e' is used for total errors, default value is e=1000

		if (argument[0] == 'e' || argument[0] == 'E') {
			int len = argument.size();
			setup.total_errors = std::stof(argument.substr(2, len - 1).c_str());
		}

		// argument 'r' is used for total runs, default value is r=1

		if (argument[0] == 'r' || argument[0] == 'R') {
			int len = argument.size();
			setup.total_runs = std::stof(argument.substr(2, len - 1).c_str());
		}

		// argument 't' is the protect threshold, default value is t=2

		if (argument[0] == 't' || argument[0] == 'T') {
			int len = argument.size();
			setup.protect_threshold = std::stof(argument.substr(2, len - 1).c_str());
		}

		// argument 'f' is used for the keys input file, when this argument is not present the test
		// is run using random keys

		if (argument[0] == 'f' || argument[0] == 'F') {
			int len = argument.size();
			setup.input_keys = argument.substr(2, len - 1).c_str();
		}
	}

	// set default values for the arguments

	if (setup.bit_width < 4 || setup.bit_width > 30) {
		setup.bit_width = 10;
	}

	setup.records = (1 << setup.bit_width);

	if (setup.total_keys == -1) {
		if (setup.input_keys.compare("") == 0)
			setup.total_keys = 10000; 
	}
	else {
		if (setup.total_keys > 50000000)
			setup.total_keys = 10000; 

		setup.input_keys = "";
	}

	if (setup.total_errors == -1)
		setup.total_errors = 1000;

	if (setup.total_runs == -1)
		setup.total_runs = 1000;

	if (setup.protect_threshold == -1)
		setup.protect_threshold = 2;

	setup.inject_faults = 1;

	return setup;
}


/*
 * shows the test setup on the console
 */

void display_HLL_test_parameters(test_setup setup) {
	std::cout << "HLL - HyperLogLog Cardinality Estimator \n\n";

	std::cout << "Bit width      w = " << (int)setup.bit_width << ",  " << setup.records << " registers \n";

	if (setup.input_keys.compare("") == 0)
		std::cout << "Total keys     k = " << setup.total_keys << "\n";
	else
		std::cout << "Input keys     f = '" << setup.input_keys << "' \n";
	
	std::cout << "Errors         e = " << setup.total_errors << "\n";
	std::cout << "Threshold      t = " << setup.protect_threshold << "\n";
	std::cout << "Runs           r = " << setup.total_runs << "\n";
}


/*
 * validates the input file
 */

bool validateInputFile(std::string keys) {
	std::ifstream inputFile;

	inputFile.open(keys);

	if (inputFile.is_open())
		inputFile.close();
	else 
		return false;

	return true;
}


/*
 * calculates the length of an array of char
 */

int length(const char* str, int max) {
	int len = 0;

	for (int i = 0; i < max; i++) 
		if (str[i] == 0)
			break;
		else
			len++;

	return len;
}


/*
 * HLL test using random keys or an input file
 */

void HLL_test(int run, test_setup setup) {
	// HyperLogLog initialization

	HyperLogLog hll(setup.bit_width);
	
	// if the file with the input keys is null, the test is run with random keys

	if (setup.input_keys.compare("") == 0) {

		// test with random keys

		srand(time(NULL));

		int rand_rule;
		uint64_t rand_key;
		const uint64_t base = 17179869184;
		const uint64_t offset = 8589934592;

		std::stringstream ss;
		std::string str_key;
		char key[60];

		for (int i = 1; i <= setup.total_keys; i++) {
			rand_rule = rand() % 1000000 + 1;

			rand_key = (uint64_t) i + (uint64_t)(rand_rule * base) + (uint64_t) offset;

			ss.str("");
			ss << rand_key;
			strcpy_s(key, ss.str().c_str());

			hll.add(key, length(key, 60));
		}

	}
	else {

		// test with input file

		setup.total_keys = 0;

		if (validateInputFile(setup.input_keys)) {
			std::ifstream inputFile;
			char key[60];

			inputFile.open(setup.input_keys);

			if (inputFile.is_open()) {
				while (!inputFile.eof()) {
					inputFile.getline(key, 60);

					hll.add(key, length(key, 60));

					setup.total_keys++;
				}

				inputFile.close();
			}
		}
		else
			std::cout << "Input file '" << setup.input_keys << "' not found!\n";

	}

	// estimate arrays have 8 rows for bits [0-]7 and 2 columns for unprotected and protected estimates,
	// index 0 is used for unprotected estimates and index 1 for protected estimates using RM

	double min_estimate[8][2];
	double max_estimate[8][2];
	double avg_estimate[8][2];
	double estimate[8][2];

	double global_min_estimate[2];
	double global_max_estimate[2];
	double global_avg_estimate[2];
	double global_estimate[2];

	// HLL performance test

	clock_t start, end;

	double elapsed_time;
	double min_time[2] = { POW_2_32, POW_2_32 };
	double max_time[2] = { 0.0, 0.0 };
	double avg_time[2] = { 0.0, 0.0 };

	// initial values for min estimates are POW_2_32 and zero for max and avg estimates

	for (int b = 0; b < 8; b++) {
		min_estimate[b][0] = POW_2_32;
		min_estimate[b][1] = POW_2_32;
		max_estimate[b][0] = 0.0;
		max_estimate[b][1] = 0.0;
		avg_estimate[b][0] = 0.0;
		avg_estimate[b][1] = 0.0;
	}

	// initial values for global estimates

	for (int i = 0; i < 2; i++) {
		global_min_estimate[i] = POW_2_32;
		global_max_estimate[i] = 0.0;
		global_avg_estimate[i] = 0.0;
		global_estimate[i] = 0.0;
	}

	// error free estimate

	double error_free_estimate = hll.estimate(false);
	double dev;

	// up to total_errors are injected in each bit (0 to 7) of each HLL counter (0 to setup.records)

	for (int i = 1; i <= setup.total_errors; i++) {
				
		for (int b = 0; b < 8; b++) {
	
			for (int c = 0; c < setup.records; c++) {

				// flip the bit at position b of the counter c

				if (setup.inject_faults == 1)
					hll.flipBit(c, b);

				// unprotected estimate is retrieved with hll.estimate(false)

				start = clock();

				estimate[b][0] = hll.estimate(false);

				end = clock();

				// unprotected estimate performance

				elapsed_time = ((double)(end - start)) / CLOCKS_PER_SEC;

				if (min_time[0] > elapsed_time)
					min_time[0] = elapsed_time;

				if (max_time[0] < elapsed_time)
					max_time[0] = elapsed_time;

				avg_time[0] = avg_time[0] + elapsed_time;

				// unprotected estimates

				dev = 100 * (estimate[b][0] - error_free_estimate) / error_free_estimate;

				if (min_estimate[b][0] > dev)
					min_estimate[b][0] = dev;

				if (max_estimate[b][0] < dev)
					max_estimate[b][0] = dev;

				avg_estimate[b][0] = avg_estimate[b][0] + dev;

				// counter protection using threshold

				start = clock();

				hll.protect((uint8_t)setup.protect_threshold);

				// protected estimate is retrieved with hll.estimate(true)

				estimate[b][1] = hll.estimate(true);

				end = clock();

				// protected estimate performance

				elapsed_time = ((double)(end - start)) / CLOCKS_PER_SEC;

				if (min_time[1] > elapsed_time)
					min_time[1] = elapsed_time;

				if (max_time[1] < elapsed_time)
					max_time[1] = elapsed_time;

				avg_time[1] = avg_time[1] + elapsed_time;

				// protected estimates

				dev = 100 * (estimate[b][1] - error_free_estimate) / error_free_estimate;

				if (min_estimate[b][1] > dev)
					min_estimate[b][1] = dev;

				if (max_estimate[b][1] < dev)
					max_estimate[b][1] = dev;

				avg_estimate[b][1] = avg_estimate[b][1] + dev;

				// global error free estimate

				global_estimate[0] = global_estimate[0] + error_free_estimate;
				global_estimate[1] = global_estimate[1] + error_free_estimate;

				// restore the bit flip

				if (setup.inject_faults == 1)
					hll.flipBit(c, b);

			}

		}

	}

	// avg time performance

	avg_time[0] = avg_time[0] / (setup.total_errors * 8 * setup.records);
	avg_time[1] = avg_time[1] / (setup.total_errors * 8 * setup.records);

	// performance output

	std::stringstream ss;
	std::string data_file;
	std::fstream test_output;

	ss.str("");
	ss << "perf_w" << (int)setup.bit_width << "_k" << setup.total_keys << "_e" << setup.total_errors << "_t" << setup.protect_threshold << "_r" << setup.total_runs << ".txt";
	data_file = ss.str();

	test_output.open(data_file, std::fstream::app);

	test_output << run << "\t" << min_time[0] << "\t" << avg_time[0] << "\t" << max_time[0] << "\t"
		<< min_time[1] << "\t" << avg_time[1] << "\t" << max_time[1] << "\n";

	test_output.close();

	// average estimates for unproyected and protected HLL and global estimates update

	for (int b = 0; b < 8; b++) {
		avg_estimate[b][0] = avg_estimate[b][0] / setup.records;
		avg_estimate[b][1] = avg_estimate[b][1] / setup.records;
	}

	for (int b = 0; b < 8; b++) {
		// global min estimate is the absolute min

		if (global_min_estimate[0] > min_estimate[b][0])
			global_min_estimate[0] = min_estimate[b][0];

		if (global_min_estimate[1] > min_estimate[b][1])
			global_min_estimate[1] = min_estimate[b][1];

		// global max estimate is the absolute max

		if (global_max_estimate[0] < max_estimate[b][0])
			global_max_estimate[0] = max_estimate[b][0];

		if (global_max_estimate[1] < max_estimate[b][1])
			global_max_estimate[1] = max_estimate[b][1];

		// global avg estimate is the mean of avg

		global_avg_estimate[0] = global_avg_estimate[0] + avg_estimate[b][0];
		global_avg_estimate[1] = global_avg_estimate[1] + avg_estimate[b][1];
	}

	global_avg_estimate[0] = global_avg_estimate[0] / 8;
	global_avg_estimate[1] = global_avg_estimate[1] / 8;

	global_estimate[0] = global_estimate[0] / (double)(setup.total_errors * setup.records * 8);
	global_estimate[1] = global_estimate[1] / (double)(setup.total_errors * setup.records * 8);

	// the test produces 16 output files for the estimates corresponding to bit positions 0 to 7 for
	// unprotected and protected HLL estimates
		
	// estimates for bits 0 to 7 for unprotected and protected scenarios

	for (int b = 0; b < 8; b++) {

		// unprotected HLL

		ss.str("");
		ss << "hll0_b" << b << "_w" << (int)setup.bit_width << "_k" << setup.total_keys << "_e" << setup.total_errors << "_t" << setup.protect_threshold << "_r" << setup.total_runs << ".txt";
		data_file = ss.str();

		test_output.open(data_file, std::fstream::app);

		test_output << run << "\t" << (int)round(estimate[b][0]) << "\t"
				    << min_estimate[b][0] << "\t" << avg_estimate[b][0] << "\t" << max_estimate[b][0] << "\n";

		test_output.close();

		// protected HLL

		ss.str("");
		ss << "hll1_b" << b << "_w" << (int)setup.bit_width << "_k" << setup.total_keys << "_e" << setup.total_errors << "_t" << setup.protect_threshold << "_r" << setup.total_runs << ".txt";
		data_file = ss.str();

		test_output.open(data_file, std::fstream::app);

		test_output << run << "\t" << (int)round(estimate[b][1]) << "\t"
			<< min_estimate[b][1] << "\t" << avg_estimate[b][1] << "\t" << max_estimate[b][1] << "\n";

		test_output.close();
	}

	// global estimates

	std::fstream test_global_output;

	// unprotected HLL

	ss.str("");
	ss << "hll0_w" << (int)setup.bit_width << "_k" << setup.total_keys << "_e" << setup.total_errors << "_t" << setup.protect_threshold << "_r" << setup.total_runs << ".txt";
	data_file = ss.str();

	test_global_output.open(data_file, std::fstream::app);

	test_global_output << run << "\t" << (int)round(global_estimate[0]) << "\t"
					   << global_min_estimate[0] << "\t" << global_avg_estimate[0] << "\t" << global_max_estimate[0] << "\n";

	test_global_output.close();

	// protected HLL

	ss.str("");
	ss << "hll1_w" << (int)setup.bit_width << "_k" << setup.total_keys << "_e" << setup.total_errors << "_t" << setup.protect_threshold << "_r" << setup.total_runs << ".txt";
	data_file = ss.str();

	test_global_output.open(data_file, std::fstream::app);

	test_global_output << run << "\t" << (int)round(global_estimate[1]) << "\t"
					   << global_min_estimate[1] << "\t" << global_avg_estimate[1] << "\t" << global_max_estimate[1] << "\n";

	test_global_output.close();

}


int main(int argc, char **argv) {

	// command line arguments for HyperLogLog Cardinality Estimator
	//
	// w: bit width, default value is w=10 and valid values are [4,30]
	// k: random keys, default value is k=10000, valid values are {500, 1000, 5000, 10000, 50000, 100000, 500000, 1000000}
	// e: errors, default value is e=1000
	// r: runs, default value is 1000, it is used to run the test multiple times
	// t: protect threshold, default value is t=2
	// f: input keys, when not present the test is run with a set of random keys
	//
	// example: hll.exe w=20 k=100000 e=10000
	//          hll.exe w=10 k=100000 e=10000 r=10000 t=0
	//          hll.exe k=1000000
	//          hll.exe w=20 f=1000_ips.txt e=10000

	test_setup setup = setup_HLL_test(argc, argv);

	// show the HLL test parameters

	display_HLL_test_parameters(setup);

	// HLL test
	
	for (int i = 1; i <= setup.total_runs; i++)
		HLL_test(i, setup);

	return 0;
}
