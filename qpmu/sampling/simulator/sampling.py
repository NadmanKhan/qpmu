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
        signal_to_noise_ratio: float = 1,
    ):
        assert resolution_bits > 0, "Resolution must be positive"
        assert reference_voltage > 0, "Reference voltage must be positive"
        if reference_voltage == 1:
            reference_voltage = signal.amplitude
        assert (
            signal.amplitude <= reference_voltage
        ), "Signal's amplitude must be less than or equal to the reference voltage"
        assert signal_to_noise_ratio >= 1, "Signal-to-noise ratio must be greater than or equal to 1"

        self.signal = signal
        self.resolution_bits = resolution_bits
        self.reference_voltage = reference_voltage
        self.signal_to_noise_ratio = signal_to_noise_ratio

        self.max_value = (1 << self.resolution_bits) - 1
        self.offset = self.max_value / 2
        self.scale = self.max_value / self.reference_voltage
        self.coeficient_of_variance = 1 / self.signal_to_noise_ratio

    def sample(self, time_s: float) -> int:
        value = self.signal(time_s)
        noise = random.uniform(-1, 1) * self.reference_voltage * self.coeficient_of_variance
        value += noise
        result = round(value * self.scale + self.offset)
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
        signal_to_noise_ratios: Iterable[float] | None = None,
    ):
        assert isinstance(resolution_bits, int), "Resolution must be an integer"
        assert isinstance(sampling_rate_hz, int), "Sampling rate must be an integer"
        assert all(isinstance(signal, AnalogSignal) for signal in signals), "Signals must be Analog_Signal instances"
        if reference_voltages is None:
            reference_voltages = [signal.amplitude for signal in signals]
        assert all(
            isinstance(reference_voltage, (int, float)) for reference_voltage in reference_voltages
        ), "Reference voltages must be numbers"
        if signal_to_noise_ratios is None:
            signal_to_noise_ratios = [1 for _ in signals]
        assert all(
            isinstance(signal_to_noise_ratio, (int, float)) for signal_to_noise_ratio in signal_to_noise_ratios
        ), "Signal-to-noise ratios must be numbers"

        assert resolution_bits > 0, "Resolution must be positive"
        assert sampling_rate_hz > 0, "Sampling rate must be positive"
        assert len(signals) == len(reference_voltages), "Signals and reference voltages must have the same length"
        assert len(signals) == len(
            signal_to_noise_ratios
        ), "Signals and signal-to-noise ratios must have the same length"

        self.resolution_bits = resolution_bits
        self.sampling_rate_hz = sampling_rate_hz

        self.sampling_period_s = 1 / self.sampling_rate_hz
        self.channels = list(
            ADCChannel(
                resolution_bits=resolution_bits,
                signal=signal,
                reference_voltage=reference_voltage,
                signal_to_noise_ratio=signal_to_noise_ratio,
            )
            for signal, reference_voltage, signal_to_noise_ratio in zip(
                signals, reference_voltages, signal_to_noise_ratios
            )
        )

    def start(self):
        start_time_s = time.time()
        while True:
            time.sleep(self.sampling_period_s)
            samples = tuple(ch.sample(time_s - start_time_s) for ch in self.channels)
            time_s = time.time()
            yield samples


@dataclass
class TimeSyncedSample:
    sequence_num: int = 0
    channel_values: tuple[int] = (0, 0, 0, 0, 0, 0)
    timestamp_us: int = 0
    timedelta_us: int = 0

    def pack(self) -> bytes:
        assert isinstance(self.sequence_num, int), "Sequence number must be an integer"
        assert isinstance(self.timestamp_us, int), "Timestamp must be an integer"
        assert isinstance(self.timedelta_us, int), "Time delta must be an integer"
        assert all(isinstance(value, int) for value in self.channel_values), "Channel values must be integers"
        assert len(self.channel_values) == 6, "There must be 6 channel values"

        assert self.sequence_num >= 0, "Sequence number must be non-negative"
        assert self.timestamp_us >= 0, "Timestamp must be non-negative"
        assert self.timedelta_us >= 0, "Time delta must be non-negative"
        assert all(value >= 0 for value in self.channel_values), "Channel values must be non-negative"

        return struct.pack(
            "9Q",
            self.sequence_num,
            *self.channel_values,
            self.timestamp_us,
            self.timedelta_us,
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

        return TimeSyncedSample(
            sequence_num=self.sequence_num,
            channel_values=channel_values,
            timestamp_us=timestamp_us,
            timedelta_us=timedelta_us,
        )
