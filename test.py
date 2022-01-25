import os
import time

dry_run = False

skip_to_index = 82
i = 1
for order in ['L-G-B']:
    for partition_points in [(1, 1), (2, 5), (3, 3), (1,5), (3,5), (7,7), (2,7), (1,6)]:
        for little_freq in [500000, 1398000, 1800000]:
            for big_freq in [500000, 1398000, 1800000, 2208000]:
                partition_point_1, partition_point_2 = partition_points
                print("HALLO HALLO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DIT IS TEST NUMMER                                              ", i)

                i += 1;
                if i - 1 < skip_to_index:
                    continue

                command = f"""
                adb exec-out sh -x -c '
                    cd /data/local/workingdir &&
                    export LD_LIBRARY_PATH=/data/local/workingdir &&
                    echo {little_freq} > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq
                    echo {big_freq} > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq
                    ./graph_alexnet_all_pipe_sync --threads=4 --threads2=2 --n=60 --total_cores=6 --partition_point={partition_point_1} --partition_point2={partition_point_2} --order={order}
                    '
                """

                if dry_run:
                    print('would run:', command)
                else:
                    os.system(command)

                time.sleep(1)
