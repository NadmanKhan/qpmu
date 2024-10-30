from typing import Literal, Iterable, Union, Tuple
from dataclasses import dataclass
import math
import random
import time
import struct


@dataclass
class AnalogSignal:

    def __init__(
        self,
        frequency_hz: float,
        amplitude: float,
        phase_rad: float,
        waveform: Literal["sine", "square"] = "sine",
    ):
        assert isinstance(frequency_hz, (int, float)), "Frequency must be a number"
        assert isinstance(amplitude, (int, float)), "Amplitude must be a number"
        assert isinstance(phase_rad, (int, float)), "Phase must be a number"

        assert frequency_hz > 0, "Frequency must be positive"
        assert amplitude >= 0, "Amplitude must be non-negative"
        assert waveform in ["sine", "square"], "Waveform must be 'sine' or 'square'"

        self.frequency_hz = frequency_hz
        self.amplitude = amplitude
        self.phase_rad = phase_rad
        self.waveform = waveform

        self.generator_func = self.square_wave if waveform == "square" else self.sine_wave

    def sine_wave(self, time_s: float) -> float:
        return self.amplitude * math.sin(2 * math.pi * self.frequency_hz * time_s + self.phase_rad)

    def square_wave(self, time_s: float) -> float:
        return self.amplitude * math.copysign(1, math.sin(2 * math.pi * self.frequency_hz * time_s + self.phase_rad))

    def __call__(self, time_s: float) -> float:
        return self.generator_func(time_s)


@dataclass
class TimestampedADC:

    def __init__(
        self,
        resolution_bits: int,
        sampling_rate_hz: int,
        reference_voltage_v: float,
        channels: Iterable[Tuple[AnalogSignal, float]],
    ):
        assert isinstance(resolution_bits, int), "Resolution must be an integer"
        assert isinstance(sampling_rate_hz, int), "Sampling rate must be an integer"
        assert isinstance(reference_voltage_v, (float, int)), "Reference voltage must be a number"
        assert len(channels) == 6, "There must be 6 channels"
        assert all(
            isinstance(signal, AnalogSignal) and isinstance(noise, float) for signal, noise in channels
        ), "Channels must be pairs of AnalogSignal and float"

        assert resolution_bits > 0, "Resolution must be positive"
        assert sampling_rate_hz > 0, "Sampling rate must be positive"
        assert reference_voltage_v > 0, "Reference voltage must be positive"
        assert all(0 <= noise <= 1 for _, noise in channels), "Noise must be between 0 and 1"

        self.resolution_bits = resolution_bits
        self.sampling_rate_hz = sampling_rate_hz
        self.reference_voltage_v = reference_voltage_v
        self.channels = channels

        self.sampling_period_s = 1 / self.sampling_rate_hz
        self.max_value = 2**self.resolution_bits - 1
        self.scale = self.max_value / self.reference_voltage_v

    def generate_sample(self, signal: AnalogSignal, noise: float, time_s: float) -> int:
        noise_amount = noise * random.uniform(-1, 1) * self.reference_voltage_v
        value = signal(time_s) + noise_amount
        result = round(value * self.scale + self.max_value)  # scale and offset to ADC range
        result = max(0, min(result, self.max_value))  # clamp to ADC range
        return result

    def stream(self):
        sequence_num = 1
        tracker_time_ns = time.time_ns()
        while True:
            time.sleep(self.sampling_period_s * 0.9)
            time_ns = time.time_ns()
            yield TimestampedADC.Sample(
                sequence_num=sequence_num,
                channel_values=tuple(self.generate_sample(s, n, time_ns / 1e9) for s, n in self.channels),
                timestamp_us=time_ns // 1000,
                timedelta_us=(time_ns - tracker_time_ns) // 1000,
            )
            sequence_num += 1
            tracker_time_ns = time_ns

    @dataclass
    class Sample:

        STRUCT_FORMAT = "@9q"

        CSV_FORMAT = "seq_no={sequence_num},ts={timestamp_us},\tdelta={timedelta_us},\tch0={channel_values[0]:4},ch1={channel_values[1]:4},ch2={channel_values[2]:4},\
            ch3={channel_values[3]:4},ch4={channel_values[4]:4},ch5={channel_values[5]:4},"

        def __init__(
            self,
            sequence_num: int,
            channel_values: Tuple[int],
            timestamp_us: int,
            timedelta_us: int,
        ):
            assert isinstance(sequence_num, int), "Sequence number must be an integer"
            assert isinstance(timestamp_us, int), "Timestamp must be an integer"
            assert isinstance(timedelta_us, int), "Time delta must be an integer"
            assert all(isinstance(value, int) for value in channel_values), "Channel values must be integers"
            assert len(channel_values) == 6, "Channel values must have 6 elements"

            self.sequence_num = sequence_num
            self.channel_values = channel_values
            self.timestamp_us = timestamp_us
            self.timedelta_us = timedelta_us

        def __bytes__(self) -> bytes:
            return struct.pack(
                TimestampedADC.Sample.STRUCT_FORMAT,
                self.sequence_num,
                self.timestamp_us,
                self.timedelta_us,
                *self.channel_values,
            )

        def __str__(self) -> str:
            return TimestampedADC.Sample.CSV_FORMAT.format(
                sequence_num=self.sequence_num,
                timestamp_us=self.timestamp_us,
                timedelta_us=self.timedelta_us,
                channel_values=self.channel_values,
            )

        @classmethod
        def from_bytes(cls, data: bytes) -> "TimestampedADC.Sample":
            values = struct.unpack(cls.STRUCT_FORMAT, data)
            sequence_num = values[0]
            timestamp_us = values[1]
            timedelta_us = values[2]
            channel_values = values[3:9]
            return cls(sequence_num, timestamp_us, timedelta_us, channel_values)

        @classmethod
        def from_str(cls, data: str) -> "TimestampedADC.Sample":
            def value_from_kvstring(kvs: str) -> int:
                return int(kvs.split("=")[1])

            kvstrings = data.split(",")
            sequence_num = value_from_kvstring(kvstrings[0])
            timestamp_us = value_from_kvstring(kvstrings[1])
            timedelta_us = value_from_kvstring(kvstrings[2])
            channel_values = tuple(value_from_kvstring(kvs) for kvs in kvstrings[3:9])
            return cls(sequence_num, timestamp_us, timedelta_us, channel_values)
