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

int partition_point_1;
int partition_point_2;

std::string graph;
std::string order;
int n_frames;

int partitions = 0;
int target_fps = 0;
int target_latency = 0;

bool latency_condition = false;
bool fps_condition = false;

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
	target_fps = (int) strtol(line.c_str(), NULL, 10);
	if (errno != 0) {
		perror("target_fps format");
		exit(1);
	}
	getline(target_file, line);
	if (errno != 0) {
		perror("target_latency read");
		exit(1);
	}
	target_latency = (int) strtol(line.c_str(), NULL, 10);
	if (errno != 0) {
		perror("target_latency format");
		exit(1);
	}
	printf("Target fps is %d, target latency is %d\n", target_fps, target_latency);
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

	if (target_fps > 11) {
		printf("High fps target, using GPU and pre-raising big/little clock\n");
		partition_point_1 = 1;
		partition_point_2 = 6;
		set_big_freq(max_big_freq_index/2);
		set_little_freq(max_little_freq_index/2);
	} else {
		partition_point_1 = 1;
		partition_point_2 = 1;
	}

	while(true) {
		update_target();
		run_test();

		if (fps_condition && latency_condition) {
			printf("latency and throughput targets met.\n");

			// while(partition_point_2 > partition_point_1) {
			// 	printf("Moving some GPU workload to big CPU\n");
			// 	partition_point_2--;
			// 	run_test(graph, order, n_frames, partition_point_1, partition_point_2);
			// 	if (!fps_condition || !latency_condition) {
			// 		printf("Conditions no longer met, restoring partition size\n");
			// 		partition_point_2++;
			// 		break;
			// 	} else {
			// 		printf("Conditions still met, lower GPU usage further\n");
			// 	}
			// }

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

			printf("Solution Was Found.\n TargetBigFrequency:%d \t TargetLittleFrequency:%d \t partition_point_1:%d \t partition_point_2:%d \t Order:%s\n",
			BigFrequencyTable[cur_big_freq_index], LittleFrequencyTable[cur_little_freq_index], partition_point_1, partition_point_2, order.c_str());
			break;
		}

		printf("Target performance not satisfied\n\n");

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
			if (partition_point_2 == 6) {
				printf("Partition point 2: 6->5\n");
				partition_point_2 = 5;
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
		}
	}

  	return 0;
}
