import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
import numpy as np
import random
import ctypes


class ADC_Simulator:
    def __init__(self, root):
        self.root = root
        self.root.title("ADC Simulation")

        # Configure grid weights for resizing
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)

        # Create a canvas and scrollbar for scrolling
        canvas = tk.Canvas(root)
        scrollbar = ttk.Scrollbar(root, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)

        scrollable_frame.bind(
            "<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )

        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.grid(row=0, column=0, sticky="nsew")
        scrollbar.grid(row=0, column=1, sticky="ns")
        canvas.configure(yscrollcommand=scrollbar.set)

        # Define UI components in scrollable frame
        self.create_widgets(scrollable_frame)

        # Adjust row and column weights for resizing
        for i in range(10):
            scrollable_frame.columnconfigure(i, weight=1)
        for i in range(8):
            scrollable_frame.rowconfigure(i, weight=1)

    def create_widgets(self, parent):
        # Voltage Amplitude Control
        voltage_frame = ttk.LabelFrame(parent, text="Voltage Source Parameters")
        voltage_frame.grid(row=0, column=0, padx=5, pady=5, sticky="ew")

        ttk.Label(voltage_frame, text="Voltage Amplitude (V)").grid(
            row=0, column=0, padx=5, pady=5
        )
        self.voltage_amplitude_var = tk.DoubleVar(value=230.0)
        voltage_amplitude_spin = ttk.Spinbox(
            voltage_frame,
            from_=0.0,
            to=400.0,
            increment=0.1,
            textvariable=self.voltage_amplitude_var,
        )
        voltage_amplitude_spin.grid(row=0, column=1, padx=5, pady=5)
        voltage_amplitude_scale = ttk.Scale(
            voltage_frame,
            from_=0.0,
            to=400.0,
            orient="horizontal",
            variable=self.voltage_amplitude_var,
        )
        voltage_amplitude_scale.grid(row=0, column=2, padx=5, pady=5)

        # Current Amplitude Control
        current_frame = ttk.LabelFrame(parent, text="Current Source Parameters")
        current_frame.grid(row=1, column=0, padx=5, pady=5, sticky="ew")

        ttk.Label(current_frame, text="Current Amplitude (A)").grid(
            row=0, column=0, padx=5, pady=5
        )
        self.current_amplitude_var = tk.DoubleVar(value=5.0)
        current_amplitude_spin = ttk.Spinbox(
            current_frame,
            from_=0.0,
            to=100.0,
            increment=0.1,
            textvariable=self.current_amplitude_var,
        )
        current_amplitude_spin.grid(row=0, column=1, padx=5, pady=5)
        current_amplitude_scale = ttk.Scale(
            current_frame,
            from_=0.0,
            to=100.0,
            orient="horizontal",
            variable=self.current_amplitude_var,
        )
        current_amplitude_scale.grid(row=0, column=2, padx=5, pady=5)

        # Voltage Channel Scaling and Noise
        voltage_scale_frame = ttk.LabelFrame(
            parent, text="Voltage Channel Scaling and Noise"
        )
        voltage_scale_frame.grid(row=2, column=0, padx=5, pady=5, sticky="ew")

        self.voltage_scales = []
        self.voltage_noises = []
        for i, phase in enumerate(["VA", "VB", "VC"]):
            ttk.Label(voltage_scale_frame, text=f"{phase} Scale").grid(
                row=i, column=0, padx=5, pady=5
            )
            scale_var = tk.DoubleVar(value=1.0)
            self.voltage_scales.append(scale_var)
            scale_spin = ttk.Spinbox(
                voltage_scale_frame,
                from_=0.0,
                to=10.0,
                increment=0.1,
                textvariable=scale_var,
            )
            scale_spin.grid(row=i, column=1, padx=5, pady=5)
            scale_scale = ttk.Scale(
                voltage_scale_frame,
                from_=0.0,
                to=10.0,
                orient="horizontal",
                variable=scale_var,
            )
            scale_scale.grid(row=i, column=2, padx=5, pady=5)

            ttk.Label(voltage_scale_frame, text=f"{phase} Noise Level").grid(
                row=i, column=3, padx=5, pady=5
            )
            noise_var = tk.DoubleVar(value=0.0)
            self.voltage_noises.append(noise_var)
            noise_spin = ttk.Spinbox(
                voltage_scale_frame,
                from_=0.0,
                to=5.0,
                increment=0.1,
                textvariable=noise_var,
            )
            noise_spin.grid(row=i, column=4, padx=5, pady=5)
            noise_scale = ttk.Scale(
                voltage_scale_frame,
                from_=0.0,
                to=5.0,
                orient="horizontal",
                variable=noise_var,
            )
            noise_scale.grid(row=i, column=5, padx=5, pady=5)

        # Current Channel Scaling and Noise
        current_scale_frame = ttk.LabelFrame(
            parent, text="Current Channel Scaling and Noise"
        )
        current_scale_frame.grid(row=3, column=0, padx=5, pady=5, sticky="ew")

        self.current_scales = []
        self.current_noises = []
        for i, phase in enumerate(["IA", "IB", "IC"]):
            ttk.Label(current_scale_frame, text=f"{phase} Scale").grid(
                row=i, column=0, padx=5, pady=5
            )
            scale_var = tk.DoubleVar(value=1.0)
            self.current_scales.append(scale_var)
            scale_spin = ttk.Spinbox(
                current_scale_frame,
                from_=0.0,
                to=10.0,
                increment=0.1,
                textvariable=scale_var,
            )
            scale_spin.grid(row=i, column=1, padx=5, pady=5)
            scale_scale = ttk.Scale(
                current_scale_frame,
                from_=0.0,
                to=10.0,
                orient="horizontal",
                variable=scale_var,
            )
            scale_scale.grid(row=i, column=2, padx=5, pady=5)

            ttk.Label(current_scale_frame, text=f"{phase} Noise Level").grid(
                row=i, column=3, padx=5, pady=5
            )
            noise_var = tk.DoubleVar(value=0.0)
            self.current_noises.append(noise_var)
            noise_spin = ttk.Spinbox(
                current_scale_frame,
                from_=0.0,
                to=5.0,
                increment=0.1,
                textvariable=noise_var,
            )
            noise_spin.grid(row=i, column=4, padx=5, pady=5)
            noise_scale = ttk.Scale(
                current_scale_frame,
                from_=0.0,
                to=5.0,
                orient="horizontal",
                variable=noise_var,
            )
            noise_scale.grid(row=i, column=5, padx=5, pady=5)

        # ADC Parameters
        adc_frame = ttk.LabelFrame(parent, text="ADC Parameters")
        adc_frame.grid(row=4, column=0, padx=5, pady=5, sticky="ew")

        ttk.Label(adc_frame, text="Bit Length").grid(row=0, column=0, padx=5, pady=5)
        self.bit_length_var = tk.IntVar(value=12)
        bit_length_spin = ttk.Spinbox(
            adc_frame,
            from_=8,
            to=32,
            increment=1,
            textvariable=self.bit_length_var,
            validate="focusout",
            validatecommand=self.validate_bit_length,
        )
        bit_length_spin.grid(row=0, column=1, padx=5, pady=5)

        ttk.Label(adc_frame, text="Sampling Rate (Hz)").grid(
            row=1, column=0, padx=5, pady=5
        )
        self.sampling_rate_var = tk.DoubleVar(value=1000.0)
        sampling_rate_spin = ttk.Spinbox(
            adc_frame,
            from_=1.0,
            to=50000.0,
            increment=1.0,
            textvariable=self.sampling_rate_var,
            validate="focusout",
            validatecommand=self.validate_sampling_rate,
        )
        sampling_rate_spin.grid(row=1, column=1, padx=5, pady=5)

        ttk.Label(adc_frame, text="Frequency (Hz)").grid(row=2, column=0, padx=5, pady=5)
        self.frequency_var = tk.DoubleVar(value=50.0)
        frequency_spin = ttk.Spinbox(
            adc_frame,
            from_=1.0,
            to=1000.0,
            increment=0.1,
            textvariable=self.frequency_var,
            validate="focusout",
            validatecommand=self.validate_frequency,
        )
        frequency_spin.grid(row=2, column=1, padx=5, pady=5)

        ttk.Label(adc_frame, text="Noise Level").grid(row=3, column=0, padx=5, pady=5)
        self.noise_level_var = tk.DoubleVar(value=0.0)
        noise_level_spin = ttk.Spinbox(
            adc_frame, from_=0.0, to=5.0, increment=0.1, textvariable=self.noise_level_var
        )
        noise_level_spin.grid(row=3, column=1, padx=5, pady=5)

        # Phase Difference
        phase_frame = ttk.LabelFrame(parent, text="Phase Difference")
        phase_frame.grid(row=5, column=0, padx=5, pady=5, sticky="ew")

        ttk.Label(phase_frame, text="Phase Difference (degrees)").grid(
            row=0, column=0, padx=5, pady=5
        )
        self.phase_difference_var = tk.DoubleVar(value=0.0)
        phase_difference_spin = ttk.Spinbox(
            phase_frame,
            from_=-180.0,
            to=180.0,
            increment=0.1,
            textvariable=self.phase_difference_var,
        )
        phase_difference_spin.grid(row=0, column=1, padx=5, pady=5)

        # I/O Configuration
        io_frame = ttk.LabelFrame(parent, text="I/O Configuration")
        io_frame.grid(row=6, column=0, padx=5, pady=5, sticky="ew")

        self.io_mode_var = tk.StringVar(value="Console")
        io_modes = ["Console", "TCP", "UDP"]
        io_mode_menu = ttk.OptionMenu(
            io_frame, self.io_mode_var, *io_modes, command=self.update_io_mode
        )
        io_mode_menu.grid(row=0, column=0, padx=5, pady=5)

        self.host_label = ttk.Label(io_frame, text="Host Address")
        self.host_entry = ttk.Entry(io_frame)
        self.port_label = ttk.Label(io_frame, text="Port Number")
        self.port_entry = ttk.Entry(io_frame)

        # Initialize host and port entries as hidden
        self.host_label.grid(row=1, column=0, padx=5, pady=5)
        self.host_entry.grid(row=1, column=1, padx=5, pady=5)
        self.port_label.grid(row=1, column=2, padx=5, pady=5)
        self.port_entry.grid(row=1, column=3, padx=5, pady=5)
        self.update_io_mode(self.io_mode_var.get())

        # Sample Display
        display_frame = ttk.LabelFrame(parent, text="Sample Display")
        display_frame.grid(row=7, column=0, padx=5, pady=5, sticky="ew")

        self.sample_vars = [tk.StringVar(value="0.0") for _ in range(6)]
        self.sample_labels = []
        for i, phase in enumerate(["VA", "VB", "VC", "IA", "IB", "IC"]):
            ttk.Label(display_frame, text=phase).grid(row=i, column=0, padx=5, pady=5)
            sample_label = ttk.Label(
                display_frame, textvariable=self.sample_vars[i], relief="sunken", width=10
            )
            sample_label.grid(row=i, column=1, padx=5, pady=5)
            self.sample_labels.append(sample_label)

        # Control Buttons
        control_frame = ttk.Frame(parent)
        control_frame.grid(row=8, column=0, padx=5, pady=5, sticky="ew")

        self.start_button = ttk.Button(
            control_frame, text="Start", command=self.start_simulation
        )
        self.start_button.grid(row=0, column=0, padx=5, pady=5)
        self.stop_button = ttk.Button(
            control_frame, text="Stop", command=self.stop_simulation, state=tk.DISABLED
        )
        self.stop_button.grid(row=0, column=1, padx=5, pady=5)

    def update_io_mode(self, mode):
        if mode in ["TCP", "UDP"]:
            self.host_label.grid(row=1, column=0, padx=5, pady=5)
            self.host_entry.grid(row=1, column=1, padx=5, pady=5)
            self.port_label.grid(row=1, column=2, padx=5, pady=5)
            self.port_entry.grid(row=1, column=3, padx=5, pady=5)
        else:
            self.host_label.grid_remove()
            self.host_entry.grid_remove()
            self.port_label.grid_remove()
            self.port_entry.grid_remove()

    def start_simulation(self):
        self.simulation_running = True
        self.start_button.config(state=tk.DISABLED)
        self.stop_button.config(state=tk.NORMAL)
        self.time_counter = 0
        self.run_simulation()

    def stop_simulation(self):
        self.simulation_running = False
        self.start_button.config(state=tk.NORMAL)
        self.stop_button.config(state=tk.DISABLED)

    def run_simulation(self):
        if self.simulation_running:
            # Time increment for simulation
            time_step = 1.0 / self.sampling_rate_var.get()
            self.time_counter += time_step

            # Calculate sample values for each channel
            for i in range(3):
                voltage_amplitude = self.voltage_amplitude_var.get()
                current_amplitude = self.current_amplitude_var.get()
                frequency = self.frequency_var.get()
                phase_difference = np.radians(self.phase_difference_var.get())
                noise_level_v = self.voltage_noises[i].get()
                noise_level_i = self.current_noises[i].get()
                scale_v = self.voltage_scales[i].get()
                scale_i = self.current_scales[i].get()

                # Phase shift for each channel (120 degrees apart)
                phase_shift = np.radians(120 * i)
                voltage_phase = phase_shift
                current_phase = phase_shift + phase_difference

                # Generate sinusoidal waveforms with noise
                voltage_sample = (
                    voltage_amplitude
                    * scale_v
                    * np.sin(2 * np.pi * frequency * self.time_counter + voltage_phase)
                )
                current_sample = (
                    current_amplitude
                    * scale_i
                    * np.sin(2 * np.pi * frequency * self.time_counter + current_phase)
                )

                # Add noise to the signal
                voltage_sample += random.uniform(-noise_level_v, noise_level_v)
                current_sample += random.uniform(-noise_level_i, noise_level_i)

                # Quantize the values based on bit length
                max_value = (2 ** self.bit_length_var.get()) - 1
                voltage_sample = max(
                    0,
                    min(
                        max_value,
                        int(
                            (voltage_sample + voltage_amplitude)
                            / (2 * voltage_amplitude)
                            * max_value
                        ),
                    ),
                )
                current_sample = max(
                    0,
                    min(
                        max_value,
                        int(
                            (current_sample + current_amplitude)
                            / (2 * current_amplitude)
                            * max_value
                        ),
                    ),
                )

                # Update the sample variables for display
                self.sample_vars[i].set(f"{voltage_sample:.2f}")
                self.sample_vars[i + 3].set(f"{current_sample:.2f}")

            # Simulate next sample update after a delay
            self.root.after(int(1000 / self.sampling_rate_var.get()), self.run_simulation)

    # Input validation methods
    def validate_bit_length(self):
        return True

    def validate_sampling_rate(self):
        return True

    def validate_frequency(self):
        return True

    def validate_noise_level(self):
        return True


if __name__ == "__main__":
    root = tk.Tk()
    app = ADC_Simulator(root)
    root.mainloop()
