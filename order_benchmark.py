import os
import time

dry_run = False

skip_to_index = 1
i = 1

for magic in [
    # ('G-L-B', (5, 6)),
    ('G-B-L', (7, 7)), # 11   90
    # ('G-B-L', (6, 7)), # 12.7
    # ('G-B-L', (5, 6)), # 20.7
    # ('L-G-B', (1, 1)),
    # ('L-B-G', (1, 7)),
    # ('B-G-L', (6, 6)),
    # ('B-L-G', (6, 7)),
    # ('G-L-B', (0, 1)),
    # ('G-B-L', (0, 6))
    ]:
    order, (partition_point_1, partition_point_2) = magic
    little_freq = 1800000
    big_freq = 2208000
    # little_freq = 50000
    # big_freq = 50000

    print("HALLO HALLO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DIT IS TEST NUMMER                                              ", i)

    i += 1;
    if i - 1 < skip_to_index:
        continue

    command = f"""
    adb exec-out sh -x -c '
        cd /data/local/Working_dir &&
        export LD_LIBRARY_PATH=/data/local/Working_dir &&
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
