import time
import random

"""
1697058467547671292
      1936800000000
      64935980790
1697057024484139018

"""

if __name__ == '__main__':
    init_ns = time.time_ns()
    prev_ts = 0
    while True:
        time.sleep(1 / 20)
        ch0 = random.randint(0, 10)
        ch1 = random.randint(0, 10)
        ch2 = random.randint(0, 10)
        ch3 = random.randint(0, 20)
        ch4 = random.randint(0, 20)
        ch5 = random.randint(0, 20)
        curr_ns = time.time_ns()
        ts = curr_ns - init_ns
        delta = ts - prev_ts
        print(f'ch0={ch0:5}, ch1={ch1:5}, ch2={ch2:5}, ch3={ch3:5}, ch4={ch4:5}, ch5={ch5:5}, ts={ts:12}, delta={delta:6},')
        prev_ts = ts


