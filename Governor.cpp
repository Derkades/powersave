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
using namespace std;

int LittleFrequencyTable[]={500000, 667000, 1000000, 1200000, 1398000, 1512000, 1608000, 1704000, 1800000};
int BigFrequencyTable[]={500000, 667000, 1000000, 1200000, 1398000, 1512000, 1608000, 1704000, 1800000, 1908000, 2016000, 2100000, 2208000};

int cur_little_freq_index = 0;
int cur_big_freq_index = 0;

int max_little_freq_index=8;
int max_big_freq_index=12;

bool LatencyCondition=0;
bool FPSCondition=0;


float StageOneInferenceTime=0;
float StageTwoInferenceTime=0;
float StageThreeInferenceTime=0;

int partitions=0;
int Target_FPS=0;
int Target_Latency=0;



/* Get feedback by parsing the results */
void ParseResults(){
	float FPS;
	float Latency;
	ifstream myfile("output.txt");
	cout<<endl;
	/* Read Output.txt File and Extract Data */
	for( std::string line; getline( myfile, line ); )
	{
		string temp;
		/* Extract Frame Rate */
		if ( line.find("Frame rate is:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> FPS){
					printf("Throughput is: %f FPS\n", FPS);
					break;
				}
				temp = "";
			}
		}
		/* Extract Frame Latency */
		if ( line.find("Frame latency is:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> Latency){
					printf("Latency is: %f ms\n", Latency);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage One Inference Time */
		if ( line.find("stage1_inference_time:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> StageOneInferenceTime){
					//printf("StageOneInferenceTime is: %f ms\n", StageOneInferenceTime);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage Two Inference Time */
		if ( line.find("stage2_inference_time:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> StageTwoInferenceTime){
					//printf("StageTwoInferenceTime is: %f ms\n", StageTwoInferenceTime);
					break;
				}
				temp = "";
			}
		}
		/* Extract Stage Three Inference Time */
		if ( line.find("stage3_inference_time:")==0 ){
			//cout<<"line is: "<<line<<std::endl;
			std::istringstream ss(line);
			while (!ss.eof()) {
				/* extracting word by word from stream */
				ss >> temp;
				/* Checking the given word is float or not */
				if (stringstream(temp) >> StageThreeInferenceTime){
					//printf("StageThreeInferenceTime is: %f ms\n", StageThreeInferenceTime);
					break;
				}
				temp = "";
			}
		}
	}
	/* Check Throughput and Latency Constraints */
	LatencyCondition = Latency <= Target_Latency;
	FPSCondition = FPS >= Target_FPS;
}

void run_test(string graph, string order, int n_frames, int partition_point_1, int partition_point_2) {
	char formatted_command[150];
	sprintf(formatted_command,
			"./%s --threads=4 --threads2=2 --target=NEON --n=%d --partition_point=%d --partition_point2=%d --order=%s > output.txt",
			graph.c_str(), n_frames, partition_point_1, partition_point_2, order.c_str());
	printf("Running command: %s\n", formatted_command);
	system(formatted_command);

	ParseResults();
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


int main(int argc, char *argv[]) {
	if (argc < 5) {
		std::cout << "Wrong number of input arguments.\n";
		return -1;
	}

	string graph=argv[1];
	partitions=atoi(argv[2]);
	Target_FPS=atoi(argv[3]);
	Target_Latency=atoi(argv[4]);

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
	string command;
	command = "echo " + to_string(LittleFrequencyTable[0]) + " > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
	system(command.c_str());
	command = "echo " + to_string(BigFrequencyTable[0]) + " > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq";
	system(command.c_str());

	int n_frames = 10;
	/* Start with running half network on Little CPU and half network on Big CPU with GPU empty in the middle */
	int partition_point_1 = 1;
	int partition_point_2 = 5;
	string order = "L-G-B";

	while(true) {
		run_test(graph, order, n_frames, partition_point_1, partition_point_2);

		if (FPSCondition && LatencyCondition) {
			printf("Latency and throughput targets met.\n");

			while(partition_point_2 > partition_point_1) {
				printf("Moving some GPU workload to big CPU\n");
				partition_point_2--;
				run_test(graph, order, n_frames, partition_point_1, partition_point_2);
				if (!FPSCondition || !LatencyCondition) {
					printf("Conditions no longer met, restoring partition size\n");
					partition_point_2++;
					break;
				} else {
					printf("Conditions still met, lower GPU usage further\n");
				}
			}

			while(cur_big_freq_index > 0) {
				set_big_freq(cur_big_freq_index - 1) ;
				run_test(graph, order, n_frames, partition_point_1, partition_point_2);

				if (FPSCondition && LatencyCondition) {
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
				run_test(graph, order, n_frames, partition_point_1, partition_point_2);

				if (FPSCondition && LatencyCondition) {
					printf("Conditions still met, dropping more\n");
					continue;
				} else {
					printf("Conditions now failing, restore frequency\n");
					set_little_freq(cur_little_freq_index + 1);
					break;
				}
			}

			printf("Solution Was Found.\n TargetBigFrequency:%d \t TargetLittleFrequency:%d \t partition_point_1:%d \t partition_point_2:%d \t Order:%s\n",
			BigFrequencyTable[cur_big_freq_index ] ,LittleFrequencyTable[cur_little_freq_index], partition_point_1, partition_point_2, order.c_str());
			break;
		}

		printf("Target performance not satisfied\n\n");

		bool wasAbleToChangeFrequency = false;
		if (cur_little_freq_index < max_little_freq_index) {
			int deltaToMax = max_big_freq_index - cur_little_freq_index;
			set_little_freq(cur_little_freq_index + std::max(deltaToMax / 2, 1));
			wasAbleToChangeFrequency = true;
		}

		if (cur_big_freq_index < max_big_freq_index) {
			int deltaToMax = max_little_freq_index - cur_big_freq_index ;
			set_big_freq(cur_big_freq_index + std::max(deltaToMax / 2, 1));
			wasAbleToChangeFrequency = true;
		}

		if (!wasAbleToChangeFrequency) {
			if (StageOneInferenceTime < StageThreeInferenceTime) {
				if (partition_point_2 < partitions) {
					/* Push Layers from Third Stage (Big CPU) to GPU to Meet Target Performance */
					partition_point_2 = partition_point_2 + 1;
					printf("Reducing the Size of Pipeline Partition 3\n");
				} else {
					printf("No Solution Found\n");
					break;
				}
			} else {
				if (partition_point_1 > 1) {
					/* Push Layers from First Stage (Little CPU) to GPU to Meet Target Performance */
					partition_point_1 = partition_point_1 - 1;
					printf("Reducing the Size of Pipeline Partition 1\n");
				} else{
					printf("No Solution Found\n");
					break;
				}
			}
		}
	}

  	return 0;
}
