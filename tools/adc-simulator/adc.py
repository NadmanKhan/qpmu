from typing import List, Literal, Sequence, Tuple
from dataclasses import dataclass, InitVar
from statistics import median
import random
import math
import time
import struct


@dataclass
class AnalogSignal:
    frequency_hz: float
    amplitude: float
    phase_rad: float

    waveform: InitVar[Literal["sine", "square"]] = "sine"

    def __post_init__(self, waveform: Literal["sine", "square"]):
        assert isinstance(self.frequency_hz, (int, float)), "Frequency must be a number"
        assert isinstance(self.amplitude, (int, float)), "Amplitude must be a number"
        assert isinstance(self.phase_rad, (int, float)), "Phase must be a number"

        assert self.frequency_hz > 0, "Frequency must be positive"
        assert self.amplitude >= 0, "Amplitude must be non-negative"
        assert waveform in ["sine", "square"], "Waveform must be 'sine' or 'square'"

        def sine_wave_value(time_s: float) -> float:
            return self.amplitude * math.sin(
                2 * math.pi * self.frequency_hz * time_s + self.phase_rad
            )

        def square_wave_value(time_s: float) -> float:
            return self.amplitude * math.copysign(
                1, math.sin(2 * math.pi * self.frequency_hz * time_s + self.phase_rad)
            )

        self.value_fn = square_wave_value if waveform == "square" else sine_wave_value

    def __call__(self, time_s: float) -> float:
        return self.value_fn(time_s)


ADC_SAMPLE_STRUCT_FORMAT = "@q6H2q"  # 1 sequence number (signed 64-bit), 6 ADC values (unsigned 16-bit), 1 system timestamp (microseconds; signed 64-bit), 1 time delta since previous sample (microseconds; signed 64-bit)
ADC_SAMPLE_STRUCT_BUFSIZE = struct.calcsize(ADC_SAMPLE_STRUCT_FORMAT)
ADC_SAMPLE_CSV_FORMAT = "seq={sequence_number},\t ch0={channel_values[0]:4}, ch1={channel_values[1]:4}, ch2={channel_values[2]:4}, ch3={channel_values[3]:4}, ch4={channel_values[4]:4}, ch5={channel_values[5]:4},\t ts={timestamp_usec},\t delta={time_delta_usec}"


@dataclass
class TimestampedADC:
    resolution_bits: int
    sampling_rate_hz: int
    reference_voltage_v: float
    channels: Sequence[Tuple[AnalogSignal, float]]

    def __post_init__(self):
        assert isinstance(self.resolution_bits, int), "Resolution must be an integer"
        assert isinstance(
            self.sampling_rate_hz, int
        ), "Sampling rate must be an integer"
        assert isinstance(
            self.reference_voltage_v, (float, int)
        ), "Reference voltage must be a number"
        assert len(self.channels) == 6, "There must be 6 channels"
        assert all(
            isinstance(signal, AnalogSignal) and isinstance(noise, float)
            for signal, noise in self.channels
        ), "Channels must be pairs of AnalogSignal and float"

        assert self.resolution_bits > 0, "Resolution must be positive"
        assert self.sampling_rate_hz > 0, "Sampling rate must be positive"
        assert self.reference_voltage_v > 0, "Reference voltage must be positive"
        assert all(
            0 <= noise <= 1 for _, noise in self.channels
        ), "Noise must be between 0 and 1"

        self.sampling_period_s = 1 / self.sampling_rate_hz
        self.max_value = 2**self.resolution_bits - 1
        self.scale = (self.max_value / 2) / self.reference_voltage_v

    def sample(self, signal: AnalogSignal, noise: float, time_s: float) -> int:
        result = math.floor(
            signal(time_s) * self.scale + self.max_value // 2
        )  # scale and offset to ADC range
        result += noise * random.uniform(-1, 1) * self.reference_voltage_v
        result = max(0, min(int(result), self.max_value))  # clamp to ADC range
        return result

    def stream(self):
        prev_time_usec = 0
        sequence_number = 0

        while True:
            time.sleep(self.sampling_period_s * 0.9)
            curr_time_usec = time.time_ns() // 1000

            yield TimestampedADC.Sample(
                sequence_number=sequence_number,
                channel_values=tuple(
                    self.sample(s, n, curr_time_usec / 1e6) for s, n in self.channels
                ),
                timestamp_usec=curr_time_usec,
                time_delta_usec=curr_time_usec - prev_time_usec,
            )

            sequence_number += 1
            prev_time_usec = curr_time_usec

    @classmethod
    def stream_from_presampled_data(cls, presampled_data: List[dict[str, int]]):
        samples = [
            TimestampedADC.Sample(
                sequence_number=0,
                channel_values=(
                    sample_data["ch0"],
                    sample_data["ch1"],
                    sample_data["ch2"],
                    sample_data["ch3"],
                    sample_data["ch4"],
                    sample_data["ch5"],
                ),
                timestamp_usec=0,
                time_delta_usec=sample_data["time_delta"],
                do_validate=True,
            )
            for sample_data in presampled_data
        ]

        # Adjust the first sample's timedelta to be the median of all samples' timedeltas
        samples[0].time_delta_usec = int(
            median(sample.time_delta_usec for sample in samples)
        )

        prev_time_usec = 0
        sequence_number = 0

        while True:
            for sample in samples:
                time.sleep(sample.time_delta_usec / 1e6)
                curr_time_usec = time.time_ns() // 1000

                sample.sequence_number = sequence_number
                sample.timestamp_usec = curr_time_usec
                sample.time_delta_usec = curr_time_usec - prev_time_usec
                yield sample

                sequence_number += 1
                prev_time_usec = curr_time_usec

    @dataclass
    class Sample:
        sequence_number: int
        channel_values: Sequence[int]
        timestamp_usec: int
        time_delta_usec: int

        do_validate: InitVar[bool] = False

        def __post_init__(self, do_validate: bool):
            if do_validate:
                assert isinstance(
                    self.sequence_number, int
                ), "Sequence number must be an integer"
                assert all(
                    isinstance(value, int) for value in self.channel_values
                ), "Channel values must be integers"
                assert (
                    len(self.channel_values) == 6
                ), "Channel values must have 6 elements"
                assert isinstance(
                    self.timestamp_usec, int
                ), "Timestamp must be an integer"
                assert isinstance(self.time_delta_usec, int), "Delta must be an integer"

        def __bytes__(self) -> bytes:
            b = struct.pack(
                ADC_SAMPLE_STRUCT_FORMAT,
                self.sequence_number,
                *self.channel_values,
                self.timestamp_usec,
                self.time_delta_usec,
            )
            return b

        def __str__(self) -> str:
            return ADC_SAMPLE_CSV_FORMAT.format(
                sequence_number=self.sequence_number,
                channel_values=self.channel_values,
                timestamp_usec=self.timestamp_usec,
                time_delta_usec=self.time_delta_usec,
            )

        @classmethod
        def from_bytes(cls, data: bytes) -> "TimestampedADC.Sample":
            values = struct.unpack(ADC_SAMPLE_STRUCT_FORMAT, data)
            sequence_number = values[0]
            channel_values = values[1:7]
            timestamp_usec = values[7]
            time_delta_usec = values[8]
            return cls(sequence_number, channel_values, timestamp_usec, time_delta_usec)

        @classmethod
        def from_str(cls, data: str) -> "TimestampedADC.Sample":
            def value_from_kvstring(kvs: str) -> int:
                return int(kvs.split("=")[1])

            kvstrings = data.split(",")
            sequence_number = value_from_kvstring(kvstrings[0])
            channel_values = tuple(value_from_kvstring(kvs) for kvs in kvstrings[1:7])
            timestamp_usec = value_from_kvstring(kvstrings[7])
            time_delta_usec = value_from_kvstring(kvstrings[8])
            return cls(sequence_number, channel_values, timestamp_usec, time_delta_usec)
