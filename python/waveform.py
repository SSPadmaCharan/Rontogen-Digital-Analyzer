import matplotlib.pyplot as plt


class WaveformViewer:

    def __init__(self):

        plt.ion()

        self.fig, self.axes = plt.subplots(
            4,
            1,
            sharex=True,
            figsize=(12, 7)
        )

        self.channels = [
            (self.axes[0], "CH3"),
            (self.axes[1], "CH2"),
            (self.axes[2], "CH1"),
            (self.axes[3], "CH0")
        ]

        self.lines = []

        for axis, name in self.channels:

            line, = axis.step(
                [],
                [],
                where="post"
            )

            self.lines.append(line)

            axis.set_ylim(-0.2, 1.2)

            axis.set_yticks([0, 1])

            axis.set_ylabel(name)

            axis.grid(True)

        self.axes[-1].set_xlabel("Time (ms)")

        self.fig.suptitle(
            "Rontogen Digital Analyzer - V0.7.2"
        )

        self.fig.tight_layout()

        self.fig.show()


    def update(self, samples, sample_rate):

        ch0 = [(sample >> 0) & 1 for sample in samples]
        ch1 = [(sample >> 1) & 1 for sample in samples]
        ch2 = [(sample >> 2) & 1 for sample in samples]
        ch3 = [(sample >> 3) & 1 for sample in samples]

        time_axis = [
            (i / sample_rate) * 1000
            for i in range(len(samples))
        ]

        channel_data = [
            ch3,
            ch2,
            ch1,
            ch0
        ]

        for line, channel in zip(
            self.lines,
            channel_data
        ):
            line.set_data(
                time_axis,
                channel
            )

        duration_ms = (
            len(samples) / sample_rate
        ) * 1000

        for axis in self.axes:
            axis.set_xlim(
                0,
                duration_ms
            )

        self.fig.canvas.draw_idle()

        self.fig.canvas.flush_events()

        plt.pause(0.001)


    def close(self):

        plt.close(self.fig)