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

int LittleFrequencyTable[] = {500000, 667000, 1000000, 1200000, 1398000, 1512000, 1608000, 1704000, 1800000};
int BigFrequencyTable[] = {500000, 667000, 1000000, 1200000, 1398000, 1512000, 1608000, 1704000, 1800000, 1908000, 2016000, 2100000, 2208000};

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

bool latency_condition = false;
bool fps_condition = false;

bool target_changed = false; // Set to true when target has changed, set to false when target is reached

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


void set_big_freq(int new_big_freq_index)  {
	int new_freq = BigFrequencyTable[new_big_freq_index];
	string command = "echo " + to_string(new_freq) + " > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq";
	system(command.c_str());
	printf("Setting frequency of big cores from %d to %d (%d)\n", cur_big_freq_index, new_big_freq_index, new_freq);
	cur_big_freq_index = new_big_freq_index;
}

void set_little_freq(int new_little_freq_index)  {
	int new_freq = LittleFrequencyTable[new_little_freq_index];
	string command = "echo " + to_string(new_freq) + " > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
	system(command.c_str());
	printf("Setting frequency of little cores from %d to %d (%d)\n", cur_little_freq_index, new_little_freq_index, new_freq);
	cur_little_freq_index = new_little_freq_index;
}

void update_target() {
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
}

void reach_target_performance();

/*
 * In this stage, the governor will keep runnning the test over and over again until the target has changed or
 * performance is no longer satisfactory for some other reason
 */
void steady_state() {
	update_target();
	run_test();

	if (target_changed ||
			!fps_condition ||
			!latency_condition) {
		reach_target_performance();
		return;
	}

	steady_state();
}

/*
 * In this stage, the governor will lower power consumption as much as possible while keeping the performance target satisfied
 */
void lower_power_consumption() {
	while (cur_big_freq_index > 0) {
		set_big_freq(cur_big_freq_index - 1) ;
		run_test();

		if (fps_condition && latency_condition) {
			printf("Conditions still met, dropping more\n");
			continue;
		} else {
			printf("Conditions now failing, restore frequency\n");
			set_big_freq(cur_big_freq_index + 1);
			break;
		}
	}

	printf("Now try backing off little CPU freq\n");

	while(cur_little_freq_index > 0) {
		set_little_freq(cur_little_freq_index - 1);
		run_test();

		if (fps_condition && latency_condition) {
			printf("Conditions still met, dropping more\n");
			continue;
		} else {
			printf("Conditions now failing, restore frequency\n");
			set_little_freq(cur_little_freq_index + 1);
			break;
		}
	}

	printf("Solution was found.\n big_freq: %d \t little_freq: %d \t partition_point_1: %d \t partition_point_2: %d \t order: %s\n",
			BigFrequencyTable[cur_big_freq_index], LittleFrequencyTable[cur_little_freq_index], partition_point_1, partition_point_2, order.c_str());

	target_changed = false;
	steady_state();
}

/*
 * In this stage, the governor will do what it can to reach target throughput and latency.
 * It won't care as much about low power consumption. After reaching its target, it calls
 * lower_power_consumption().
 */
void reach_target_performance() {
	if (target_fps > 11) {

	} else {
		printf("Low FPS target, we can get by without using the GPU");

	}

	run_test();

	if (fps_condition && latency_condition) {
		printf("Latency and throughput targets met.\n");
		lower_power_consumption();
		return;
	}

	bool wasAbleToChangeFrequency = false;
	if (partition_point_1 > 0) {
		if (cur_little_freq_index < max_little_freq_index) {
			int deltaToMax = max_big_freq_index - cur_little_freq_index;
			set_little_freq(cur_little_freq_index + std::max(deltaToMax / 2, 1));
			wasAbleToChangeFrequency = true;
		}
	} else {
		set_little_freq(0);
	}

	if (partition_point_2 < 7) {
		if (cur_big_freq_index < max_big_freq_index) {
			int deltaToMax = max_little_freq_index - cur_big_freq_index;
			set_big_freq(cur_big_freq_index + std::max(deltaToMax / 2, 1));
			wasAbleToChangeFrequency = true;
		}
	} else {
		set_big_freq(0);
	}

	if (!wasAbleToChangeFrequency) {
		if (partition_point_1 == 1 &&
				partition_point_2 == 1) {
			printf("FPS target still not reached with max clocks, using GPU is required to reach target performance\n");
			partition_point_1 = 1;
			partition_point_2 = 6;
			set_big_freq(max_big_freq_index/2);
			set_little_freq(max_little_freq_index/2);
		} else if (partition_point_2 == 6) {
			printf("Performance still not good enough, use a little more of the big CPU\n");
			printf("Partition point 2: 6->5\n");
			partition_point_2 = 5;
		} else {
			printf("Performance still not good enough, but there's nothing we can do.\n");
		}
	}

	// if (stage_one_inference_time < stage_three_inference_time) {
	// if (partition_point_2 > 5) {
	// 	if (partition_point_2 < 5) {
	// 		/* Push Layers from Third Stage (Big CPU) to GPU to Meet Target Performance */
	// 		partition_point_2 = partition_point_2 + 1;
	// 		printf("Reducing the Size of Pipeline Partition 3\n");
	// 	} else {
	// 		printf("No Solution Found\n");
	// 		break;
	// 	}
	// } else {
	// 	if (partition_point_1 > 1) {
	// 		/* Push Layers from First Stage (Little CPU) to GPU to Meet Target Performance */
	// 		partition_point_1 = partition_point_1 - 1;
	// 		printf("Reducing the Size of Pipeline Partition 1\n");
	// 	} else{
	// 		printf("No Solution Found\n");
	// 		break;
	// 	}
	// }
	// }

	// Try again
	reach_target_performance();
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
	n_frames = 10;
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

	// Initialize little and big CPU with lowest frequency
	set_big_freq(0);
	set_little_freq(0);
	// string command;
	// command = "echo " + to_string(LittleFrequencyTable[0]) + " > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
	// system(command.c_str());
	// command = "echo " + to_string(BigFrequencyTable[0]) + " > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq";
	// system(command.c_str());

	update_target();
	reach_target_performance();

  	return 0;
}
