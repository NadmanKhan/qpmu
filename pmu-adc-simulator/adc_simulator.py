import time
import math
from typing import Iterable


FREQUENCY = 50  # Hz
I_MAX = 14  # A
V_MAX = 240  # V
SAMPLING_RATE = 1_200  # Hz (samples per cycle)


class Signal:
    '''
    A signal is a signal quantity with a fixed cycle frequency, amplitude, and initial 
    phase-angle, having a sinusoidal waveform. The value of the signal at a given time 
    is calculated using the formula:

    `amplitude * sin(2 * pi * frequency * t + initial_phase)`

    where 
    - `amplitude` is the amplitude (peak positive value) of the waveform -- in A or V
    - `frequency` is the frequency (cycles per second) of the waveform -- in Hz
    - `initial_phase` is the initial phase-angle -- in radians
    - `t` is the time at which the value of the signal is to be calculated -- in seconds
    - `sin` and `pi` are the function and constant respectively from the `math` module
    '''

    def __init__(self,
                 frequency: int,
                 amplitude: int,
                 initial_phase: float):
        '''
        Construct a signal with the given parameters.

        Parameters:
        - `frequency`: the frequency (cycles per second) of the waveform -- in Hz
        - `amplitude`: the amplitude (peak positive value) of the waveform -- in A or V
        - `initial_phase`: the initial phase-angle -- in radians
        '''
        self.frequency = int(abs(frequency))
        self.amplitude = int(abs(amplitude))
        self.initial_phase = float(initial_phase)

    def __call__(self, t: float) -> int:
        '''
        Returns the value of the signal at the given time.
        Has to be a non-negative value so as to mimic the
        real ADC.

        Parameters:
        - `t`: the time, in `float`, at which the value of the signal is to be 
          calculated
        '''
        return self.amplitude * math.sin(2 * math.pi * self.frequency * t
                                         + self.initial_phase)


class ADCSample:
    '''
    An ADCSample is a sample of the ADC's output. It contains 8 values in total:
    - values of the 6 channels of the ADC -- a `Iterable` of 6 non-negative `int`s
    - timestamp at which the sample was taken -- a positive `int`
    - time elapsed since the previous sample was taken -- a positive `int`

    The values of the 6 channels are in the following order:
    - `ch[0]`: current of phase A
    - `ch[1]`: current of phase B
    - `ch[2]`: current of phase C
    - `ch[3]`: voltage of phase A
    - `ch[4]`: voltage of phase B
    - `ch[5]`: voltage of phase C

    The `ts` is the number of nanoseconds elapsed since the start of the program.

    The `delta` is the number of nanoseconds elapsed since the previous sample was taken.

    The `__str__` method returns a string representation of the sample in the following
    f-string format:

    `f'ch0={ch[0]:4}, ch1={ch[1]:4}, ch2={ch[2]:4}, ch3={ch[3]:4}, \
    ch4={ch[4]:4}, ch5={ch[5]:4}, ts={timestamp:11}, delta={delta:6},\\n'`
    '''

    def __init__(self, ch: Iterable[int], ts: int, delta: int):
        '''
        Construct an ADCSample with the given parameters.

        Parameters:
        - `ch`: the values of the 6 channels of the ADC -- an `Iterable` of 6 
          non-negative `int`s
        - `ts`: the timestamp at which the sample was taken -- a positive `int`
        - `delta`: the time elapsed since the previous sample was taken -- a positive 
          `int`
        '''
        assert len(ch) == 6
        assert all([(type(value) == int and value >= 0) for value in ch])
        assert type(ts) == int and ts >= 0
        assert type(delta) == int and delta > 0
        self.ch = ch
        self.ts = ts
        self.delta = delta

    @classmethod
    def from_signals(cls, signals: Iterable[Signal], timestamp: int, delta: int):
        '''
        Construct an ADCSample from 6 signals.

        Parameters:
        - `signals`: the signals from which to construct the sample -- an `Iterable` of 6 
          `Signal` objects
        - `ts`: the timestamp at which the sample was taken -- a positive `int`
        - `delta`: the time elapsed since the previous sample was taken -- a positive 
          `int`
        '''
        assert len(signals) == 6
        assert all([type(signal) == Signal for signal in signals])
        assert type(timestamp) == int and timestamp >= 0
        assert type(delta) == int and delta > 0

        t = timestamp / (1_000_000_000)  # convert to seconds from nanoseconds

        # channel values must be non-negative
        channels = tuple(abs(int(x(t) + x.amplitude)) for x in signals)

        return cls(channels, timestamp, delta)

    @classmethod
    def from_string(cls, string: str):
        '''
        Construct an ADCSample from a string representation of the sample.

        Parameters:
        - `string`: the string representation of the sample -- a `str`
        '''
        assert type(string) == str
        assert string.startswith('ch0=') and string.endswith(',\n')

        tokens = string.split(',')
        assert len(tokens) == 9

        tokens.pop()  # remove the trailing newline

        ch = 6 * [int(0)]
        for i in range(6):
            key, value = tokens[i].split('=')
            assert key.strip() == f'ch{i}'
            vlaue = value.strip()
            assert vlaue.isdigit()
            ch[i] = int(value)

        kv_ts = tokens[-2].split('=')
        assert kv_ts[0].strip() == 'ts'
        kv_ts[1] = kv_ts[1].strip()
        assert kv_ts[1].isdigit()
        ts = int(kv_ts[1])

        kv_delta = tokens[-1].split('=')
        assert kv_delta[0].strip() == 'delta'
        kv_delta[1] = kv_delta[1].strip()
        assert kv_delta[1].isdigit()
        delta = int(kv_delta[1])

        return cls(ch, ts, delta)

    def __str__(self):
        '''
        Returns a string representation of the sample in the following
        f-string format:

        `f'ch0={ch[0]:4}, ch1={ch[1]:4}, ch2={ch[2]:4}, ch3={ch[3]:4}, \
        ch4={ch[4]:4}, ch5={ch[5]:4}, ts={timestamp:11}, delta={delta:6},\\n'`
        '''
        result = [f'ch{i}={int(ch):4},' for i,
                  ch in enumerate(self.ch)]
        result.append(f' ts={self.ts:11},')
        result.append(f' delta={self.delta:6},\n')
        return ''.join(result)


class ADC:

    def __init__(self,
                 sampling_rate: int = SAMPLING_RATE,
                 i_max: int = I_MAX,
                 v_max: int = V_MAX,
                 frequency: int = FREQUENCY):
        assert type(sampling_rate) == int and sampling_rate > 0
        assert type(i_max) == int and i_max > 0
        assert type(v_max) == int and v_max > 0
        assert type(frequency) == int and frequency > 0

        self.sampling_rate = sampling_rate
        self.i_max = i_max
        self.v_max = v_max
        self.frequency = frequency

        self.signals = [
            Signal(
                amplitude=i_max,
                frequency=frequency,
                initial_phase=0 * (2 * math.pi / 3)
            ),
            Signal(
                amplitude=i_max,
                frequency=frequency,
                initial_phase=1 * (2 * math.pi / 3)
            ),
            Signal(
                amplitude=i_max,
                frequency=frequency,
                initial_phase=2 * (2 * math.pi / 3)
            ),
            Signal(
                amplitude=v_max,
                frequency=frequency,
                initial_phase=0 * (2 * math.pi / 3) + math.pi / 4
            ),
            Signal(
                amplitude=v_max,
                frequency=frequency,
                initial_phase=1 * (2 * math.pi / 3) + math.pi / 4
            ),
            Signal(
                amplitude=v_max,
                frequency=frequency,
                initial_phase=2 * (2 * math.pi / 3) + math.pi / 4
            )
        ]

        assert len(self.signals) == 6

    def samples(self):
        '''
        A generator that yields `ADCSample` objects at the given sampling rate.
        '''
        init_time_ns = time.time_ns()  # initial time in nanoseconds
        prev_ts = int(0)  # previous timestamp, required to calculate `delta`

        while True:
            time.sleep(1 / self.sampling_rate)

            ts = time.time_ns() - init_time_ns  # ts = timestamp
            delta = ts - prev_ts  # delta = time elapsed since previous sample

            yield ADCSample.from_signals(self.signals, ts, delta)

            prev_ts = ts


if __name__ == '__main__':
    adc = ADC()
    for sample in adc.samples():
        print(sample)
