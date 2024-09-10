from typing import Literal, Iterable, NamedTuple
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

        self.generator_func = self.sine_wave if waveform == "sine" else self.square_wave

    def sine_wave(self, time_s: float) -> float:
        return self.amplitude * math.sin(2 * math.pi * self.frequency_hz * time_s + self.phase_rad)

    def square_wave(self, time_s: float) -> float:
        return self.amplitude * math.copysign(1, math.sin(2 * math.pi * self.frequency_hz * time_s + self.phase_rad))

    def __call__(self, time_s: float) -> float:
        return self.generator_func(time_s)


@dataclass
class ADCChannel:

    def __init__(
        self,
        resolution_bits: int,
        signal: AnalogSignal,
        reference_voltage: float = 0,
        noise: float = 0,
    ):
        assert resolution_bits > 0, "Resolution must be positive"
        assert reference_voltage > 0, "Reference voltage must be positive"
        if reference_voltage == 1:
            reference_voltage = signal.amplitude
        assert (
            signal.amplitude <= reference_voltage
        ), "Signal's amplitude must be less than or equal to the reference voltage"
        assert 0 <= noise <= 1, "Noise must be between 0 and 1"

        self.signal = signal
        self.resolution_bits = resolution_bits
        self.reference_voltage = reference_voltage
        self.noise = noise

        self.max_value = (1 << self.resolution_bits) - 1
        self.scale = self.max_value / self.reference_voltage

    def sample(self, time_s: float) -> int:
        value = self.signal(time_s)
        noise = self.noise * random.uniform(-1, 1) * self.reference_voltage
        value += noise
        result = round(value * self.scale + self.max_value)
        result = max(0, min(self.max_value, result))  # clip
        return result


@dataclass
class ADC:

    def __init__(
        self,
        resolution_bits: int,
        sampling_rate_hz: int,
        signals: Iterable[AnalogSignal],
        reference_voltages: Iterable[float] | None = None,
        noises: Iterable[float] | None = None,
    ):
        assert isinstance(resolution_bits, int), "Resolution must be an integer"
        assert isinstance(sampling_rate_hz, int), "Sampling rate must be an integer"
        assert all(isinstance(signal, AnalogSignal) for signal in signals), "Signals must be Analog_Signal instances"
        if reference_voltages is None:
            reference_voltages = [signal.amplitude for signal in signals]
        assert all(
            isinstance(reference_voltage, (int, float)) for reference_voltage in reference_voltages
        ), "Reference voltages must be numbers"
        if noises is None:
            noises = [0 for _ in signals]
        assert all(isinstance(noise, (int, float)) for noise in noises), "Noises must be numbers"

        assert resolution_bits > 0, "Resolution must be positive"
        assert sampling_rate_hz > 0, "Sampling rate must be positive"
        assert len(signals) == len(reference_voltages), "Signals and reference voltages must have the same length"
        assert len(signals) == len(noises), "Signals and noises must have the same length"

        self.resolution_bits = resolution_bits
        self.sampling_rate_hz = sampling_rate_hz

        self.sampling_period_s = 1 / self.sampling_rate_hz
        self.channels = list(
            ADCChannel(
                resolution_bits=resolution_bits,
                signal=signal,
                reference_voltage=reference_voltage,
                noise=noise,
            )
            for signal, reference_voltage, noise in zip(signals, reference_voltages, noises)
        )

    def start(self):
        start_time_s = time.time()
        while True:
            time.sleep(self.sampling_period_s)
            time_s = time.time()
            samples = tuple(ch.sample(time_s - start_time_s) for ch in self.channels)
            yield samples


@dataclass
class Sample:

    STRUCT_FORMAT = "@9Q"

    CSV_FORMAT = "seq_no={sequence_num},\tch0={channel_values[0]:4},ch1={channel_values[1]:4},ch2={channel_values[2]:4},\
        ch3={channel_values[3]:4},ch4={channel_values[4]:4},ch5={channel_values[5]:4},ts={timestamp_us},\tdelta={timedelta_us},"

    def __init__(
        self,
        sequence_num: int,
        channel_values: tuple[int],
        timestamp_us: int,
        timedelta_us: int,
    ):
        assert isinstance(sequence_num, int), "Sequence number must be an integer"
        assert isinstance(timestamp_us, int), "Timestamp must be an integer"
        assert isinstance(timedelta_us, int), "Time delta must be an integer"
        assert all(isinstance(value, int) for value in channel_values), "Channel values must be integers"
        assert all(isinstance(value, int) for value in channel_values), "Channel values must be integers"
        assert len(channel_values) == 6, "Channel values must be non-empty"

        assert sequence_num >= 0, "Sequence number must be non-negative"
        assert timestamp_us >= 0, "Timestamp must be non-negative"
        assert timedelta_us >= 0, "Time delta must be non-negative"
        # assert all(value >= 0 for value in channel_values), "Channel values must be non-negative"

        self.sequence_num = sequence_num
        self.channel_values = channel_values
        self.timestamp_us = timestamp_us
        self.timedelta_us = timedelta_us

    def __bytes__(self) -> bytes:
        return struct.pack(
            Sample.STRUCT_FORMAT,
            self.sequence_num,
            *self.channel_values,
            self.timestamp_us,
            self.timedelta_us,
        )

    def __str__(self) -> str:
        return Sample.CSV_FORMAT.format(
            sequence_num=self.sequence_num,
            channel_values=self.channel_values,
            timestamp_us=self.timestamp_us,
            timedelta_us=self.timedelta_us,
        )


class TimeSyncedSamples:

    def __init__(self, adc: ADC):
        self.adc = adc
        self.generator = adc.start()
        self.sequence_num = 0
        self.current_time_us = time.time_ns() // 1000

    def __iter__(self):
        return self

    def __next__(self):
        self.sequence_num += 1

        channel_values = next(self.generator)
        timestamp_us = time.time_ns() // 1000
        timedelta_us = timestamp_us - self.current_time_us

        self.current_time_us = timestamp_us

        return Sample(
            sequence_num=self.sequence_num,
            channel_values=channel_values,
            timestamp_us=timestamp_us,
            timedelta_us=timedelta_us,
        )
