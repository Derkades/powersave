/*Instructions to Run
On Your Computer:
	arm-linux-androideabi-clang++ -static-libstdc++ Governor.cpp -o Governor
	adb push Governor /data/local/Working_dir
On the Board:
	chmod +x Governor.sh
	./Governor graph_alexnet_all_pipe_sync #NumberOFPartitions #TargetFPS #TargetLatency
*/


#include <stdio.h>      /* printf */
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */
#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>
using namespace std;

#define FPS_VARIATION_CONSTANT 1.01
#define CPU_LITTLE 0
#define CPU_BIG 2

int little_freq_table[] = {500000, 667000, 1000000, 1200000, 1398000, 1512000, 1608000, 1704000, 1800000};
int big_freq_table[] = {500000, 667000, 1000000, 1200000, 1398000, 1512000, 1608000, 1704000, 1800000, 1908000, 2016000, 2100000, 2208000};

int cur_little_freq_index = 0;
int cur_big_freq_index = 0;

int max_little_freq_index = 8;
int max_big_freq_index = 12;

int partition_point_1 = 1;
int partition_point_2 = 1;

std::string graph;
std::string order;
int n_frames;

int partitions = 0;
int target_fps = 0;
int target_latency = 0;
int achieved_fps = -1;
int achieved_latency = -1;

bool latency_condition = false;
bool fps_condition = false;
bool safe_latency_condition = false;
bool safe_fps_condition = false;

float stage_one_inference_time = 0;
float stage_two_inference_time = 0;
float stage_three_inference_time = 0;

/* Get feedback by parsing the results */
void ParseResults() {
	float fps;
	float latency;
	ifstream myfile("output.txt");
	cout << endl;
	/* Read Output.txt File and Extract Data */
	for (std::string line; getline(myfile, line); ) {
		string temp;
		/* Extract Frame Rate */
		if (line.find("Frame rate is:") == 0) {
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> fps) {
					printf("Throughput is: %f fps\n", fps);
					break;
				}
				temp = "";
			}
		}
		/* Extract Frame latency */
		if (line.find("Frame latency is:") == 0) {
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> latency){
					printf("latency is: %f ms\n", latency);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage One Inference Time */
		if (line.find("stage1_inference_time:") == 0) {
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> stage_one_inference_time){
					//printf("stage_one_inference_time is: %f ms\n", stage_one_inference_time);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage Two Inference Time */
		if (line.find("stage2_inference_time:") == 0) {
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> stage_two_inference_time){
					//printf("stage_two_inference_time is: %f ms\n", stage_two_inference_time);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage Three Inference Time */
		if (line.find("stage3_inference_time:") == 0) {
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> stage_three_inference_time) {
					//printf("stage_three_inference_time is: %f ms\n", stage_three_inference_time);
					break;
				}
				temp = "";
			}
		}
	}
	/* Check Throughput and latency Constraints */
	latency_condition = latency <= target_latency;
	fps_condition = fps >= target_fps;
	safe_latency_condition = latency * FPS_VARIATION_CONSTANT <= target_latency;
	safe_fps_condition = fps >= target_fps * FPS_VARIATION_CONSTANT;
	achieved_latency = latency;
	achieved_fps = fps;
}

void run_test() {
	char formatted_command[150];
	sprintf(formatted_command,
			"./%s --threads=4 --threads2=2 --target=NEON --n=%d --partition_point=%d --partition_point2=%d --order=%s > output.txt",
			graph.c_str(), n_frames, partition_point_1, partition_point_2, order.c_str());
	printf("Running command: %s\n", formatted_command);
	system(formatted_command);

	ParseResults();

	printf("Inference stage times: %.2f %.2f %.2f\n", stage_one_inference_time, stage_two_inference_time, stage_three_inference_time);
}


void set_core_freq(int cpu_type, int new_freq_index)  {
	int new_freq = cpu_type == CPU_BIG ? big_freq_table[new_freq_index] : little_freq_table[new_freq_index];
	string command = "echo " + to_string(new_freq) + " > /sys/devices/system/cpu/cpufreq/policy" + to_string(cpu_type) + "/scaling_max_freq";
	system(command.c_str());
	const char *cpu_type_str = cpu_type == CPU_BIG ? "big" : "little";
	int cur_freq_index = cpu_type == CPU_BIG ? cur_big_freq_index : cur_little_freq_index;
	printf("Setting frequency of %s cores from %d to %d (%d)\n", cpu_type_str, cur_freq_index, new_freq_index, new_freq);
	// cur_big_freq_index = new_big_freq_index;
	if (cpu_type == CPU_BIG) {
		cur_big_freq_index = new_freq_index;
	} else {
		cur_little_freq_index = new_freq_index;
	}
}

/*
 * Read target FPS and latency from text file so it can be updated at runtime
 */
bool update_target() {
	ifstream target_file("target.txt");
	std::string line;
	getline(target_file, line);
	if (errno != 0) {
		perror("target_fps read");
		exit(1);
	}
	int new_target_fps = (int) strtol(line.c_str(), NULL, 10);
	if (errno != 0) {
		perror("target_fps format");
		exit(1);
	}
	getline(target_file, line);
	if (errno != 0) {
		perror("target_latency read");
		exit(1);
	}
	int new_target_latency = (int) strtol(line.c_str(), NULL, 10);
	if (errno != 0) {
		perror("target_latency format");
		exit(1);
	}

	bool target_changed = false;

	if (new_target_fps != target_fps) {
		printf("Target FPS changed from %d to %d\n", target_fps, new_target_fps);
		target_fps = new_target_fps;
		target_changed = true;
	}

	if (new_target_latency != target_latency) {
		printf("Target latency changed from %d to %d\n", target_latency, new_target_latency);
		target_latency = new_target_latency;
		target_changed = true;
	}
	return target_changed;
}

void find_order();

/*
 * In this stage, the governor will keep runnning the test over and over again until the target has changed or
 * performance is no longer satisfactory for some other reason
 */
void steady_state() {
	printf("Keep running test in a loop\n");
	n_frames = 60;

	if (update_target()) {
		printf("Target changed\n");
		find_order();
		return;
	}

	run_test();

	if (!fps_condition ||
			!latency_condition) {
		printf("FPS/latency conditions failed\n");
		find_order();
		return;
	}

	steady_state();
}

void lower_core_freq_binary_search(int cpu_type) {
	int lo = 0;
	int hi = cpu_type == CPU_BIG ? max_big_freq_index : max_little_freq_index;

	printf("Now try backing off %s CPU freq\n", cpu_type == CPU_BIG ? "big" : "little");

	while (hi != lo) {
		int mid = (lo + hi) / 2;
		printf("binary search lo=%d hi=%d mid=%d\n", lo, hi, mid);
		set_core_freq(cpu_type, mid);

		run_test();

		if (safe_fps_condition && safe_latency_condition) {
			printf("Conditions still met\n");
			hi = mid;
		} else {
			printf("Conditions now failing\n");
			lo = mid + 1;
		}
	}
	set_core_freq(cpu_type, hi);
}

/*
 * In this stage, the governor will lower power consumption by reducing core
 * frequencies as much as possible while keeping the performance target satisfied.
 */
void lower_frequencies() {
	lower_core_freq_binary_search(CPU_BIG);
	lower_core_freq_binary_search(CPU_LITTLE);

	printf("Solution was found.\n big_freq: %d \t little_freq: %d \t partition_point_1: %d \t partition_point_2: %d \t order: %s\n",
			big_freq_table[cur_big_freq_index], little_freq_table[cur_little_freq_index], partition_point_1, partition_point_2, order.c_str());

	steady_state();
}

/*
 * In this stage, the governor will try to lower power consumption by shifting
 * partition points, staying in a given order. For example, it will try to shift
 * one partitionable section from the big CPU to the GPU.
 */
void tune_partition_points() {
	int success_partition_point_1 = partition_point_1;
	int success_partition_point_2 = partition_point_2;

	bool more_tests_available = true;
	do {
		if (order.compare("L-G-B")) {
			// Try to get by with less big CPU
			// Increasing GPU usage won't matter for power consumption
			if (partition_point_2 == 5) {
				partition_point_2 = 6;
			} else {
				more_tests_available = false;
			}
		} else if (order.compare("G-B-L")) {
			more_tests_available = false;
		} else {
			more_tests_available = false;
		}

		run_test();

		if (!safe_latency_condition || !safe_fps_condition) {
			// Restore previous working partition points and move on to lowering frequencies
			partition_point_1 = success_partition_point_1;
			partition_point_2 = success_partition_point_2;
			lower_frequencies();
			return;
		}

		success_partition_point_1 = partition_point_1;
		success_partition_point_2 = partition_point_2;
		// Try more tests if available
	} while (more_tests_available);

	if (order.compare("L-G-B") && partition_point_1 == partition_point_2) {
		while(partition_point_1 < 7) {
			partition_point_1++;
			partition_point_2++;
			run_test();
			if (!safe_fps_condition || !safe_latency_condition) {
				printf("Conditions now failing, restore partition points\n");
				partition_point_1--;
				partition_point_2--;
				break;
			}
			printf("Conditions still met, shifting more\n");
		}
	}

	// No more tests available, performance still satisfactory
	lower_frequencies();
}

void find_order() {
	n_frames = 15;
	set_core_freq(CPU_LITTLE, max_little_freq_index);
	set_core_freq(CPU_BIG, max_big_freq_index);

	printf("Try LGB with GPU first, this is a balanced profile\n");
	order = "L-G-B";
	partition_point_1 = 1;
	partition_point_2 = 5;

	run_test();

	if (safe_latency_condition && safe_fps_condition) {
		printf("Try to get by with LBG without GPU, the slowest profile but with lowest power consumption\n");
		order = "L-B-G";
		partition_point_1 = 1;
		partition_point_2 = 7;
		run_test();
		if (safe_latency_condition && safe_fps_condition) {
			tune_partition_points();
			return;
		} else {
			printf("Revert\n");
			order = "L-G-B";
			partition_point_1 = 1;
			partition_point_2 = 5;
			tune_partition_points();
			return;
		}
	} else if (safe_fps_condition && !safe_latency_condition) {
		printf("FPS target met, latency target unmet\n");
		printf("Try the profile with extremely low latency\n");
		order = "G-B-L";
		partition_point_1 = 7;
		partition_point_2 = 7;
	} else if (!safe_fps_condition && safe_latency_condition) {
		printf("Higher throughput, don't care about latency that much\n");
		order = "G-B-L";
		partition_point_1 = 5;
		partition_point_2 = 6;
	} else {
		printf("Impossible target\n");
		exit(1);
	}

	run_test();

	if (safe_fps_condition && safe_latency_condition) {
		tune_partition_points();
		return;
	}

	printf("Impossible target\n");
	exit(1);
}

int main(int argc, char *argv[]) {
	// if (argc < 5) {
	if (argc != 2) {
		std::cout << "Wrong number of input arguments.\n";
		return -1;
	}

	graph = argv[1];
	partitions = 3;
	// partitions = atoi(argv[2]);
	// target_fps = atoi(argv[3]);
	// target_latency = atoi(argv[4]);
	order = "L-G-B";
	// n_frames = 10;
	update_target();

	// Check if processor is available
	if (!system(NULL)) {
		exit(EXIT_FAILURE);
	}

	// Set OpenCL library path
	setenv("LD_LIBRARY_PATH", "/data/local/workingdir", 1);

	// Setup Performance Governor (CPU)
	system("echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
	system("echo performance > /sys/devices/system/cpu/cpufreq/policy2/scaling_governor");

	update_target();
	find_order();

  	return 0;
}
