# **Primary Frequency Regulation Monitor Audio Pipeline Validator**

## **Overview**

This project provides a utility to **convert audio data recorded via Primary Frequency Regulation Monitor’s Audio I/O driver** into playable WAV files. It’s designed for developers testing Primary Frequency Regulation Monitor’s data pipeline integrity using audio signals, such as voice, tones, or music. If the reconstructed `.wav` file sounds like the original, the I/O pipeline is functioning as expected.

This tool requires no additional hardware. Just **Primary Frequency Regulation Monitor**, a **microphone or audio source**, and this **Python script**.

**Note:** Primary Frequency Regulation Monitor’s audio features may require a paid license. Visit [serial-studio.com](http://localhost:4567/) for more details.

## **Audio Format**

Primary Frequency Regulation Monitor logs audio samples to CSV using the following structure:

```csv
RX Date/Time,Audio Input/Channel 1,Audio Input/Channel 2,...
2025/08/02 23:43:57::950,0.000000,0.002541,...
```

- **First column**: Timestamp (ignored by this tool)
- **Remaining columns**: Audio samples for each channel (float, typically normalized)
- **Channel count**: Mono, stereo, surround, etc., depending on how many channels are logged

## **Project Features**

- Converts any Primary Frequency Regulation Monitor-generated CSV into a valid `.wav` file
- Supports mono, stereo, and multichannel audio
- CLI options for:
  - Output sample rate (default: **44,100 Hz**)
  - Sample format (`float32`, `int16`, `int24`, `int32`, `uint8`)
- Automatically normalizes audio if needed
- Helps debug Primary Frequency Regulation Monitor pipelines by **listening to the reconstructed audio**

## **How to Use**

### 1. **Record Audio in Primary Frequency Regulation Monitor**
- Set the **input source** to an **Audio I/O** device
- Configure the number of channels (1 = mono, 2 = stereo, etc.)
- Enable data logging to a `.csv` file
- Speak, play music, or record any sound

### 2. **Convert CSV to WAV**

```bash
python3 csv2wav.py path/to/audio.csv
```

Optional: specify output path, custom sample rate, and format

```bash
python3 csv2wav.py path/to/audio.csv output.wav --rate 48000 --format int16
```

### 3. **Listen to the Result**
- Open the generated WAV file in any media player
- Compare it to your original source (mic recording, test tone, etc.)
- A clean, accurate playback indicates the **data path is working end to end**

## **Supported Sample Formats**

| Format    | Bit Depth | Range                      |
|-----------|-----------|----------------------------|
| float32   | 32-bit    | -1.0 to 1.0                |
| int16     | 16-bit    | -32768 to 32767            |
| int24     | 24-bit    | -8388608 to 8388607        |
| int32     | 32-bit    | -2147483648 to 2147483647  |
| uint8     | 8-bit     | 0 to 255 (biased unsigned) |

Default format is `float32`. For playback compatibility, `int16` is usually a safe bet.

## **Example Use Case**

As a developer or tester:

- You inject an audio signal into Primary Frequency Regulation Monitor using a virtual mic or file playback
- Primary Frequency Regulation Monitor logs the raw data into a CSV file
- You run `csv2wav.py` to convert it back into a WAV
- You **listen to verify** whether the WAV matches the original audio

### ✅ If it sounds correct → pipeline works  
### ❌ If it's clipped, noisy, or silent → investigate device config, sample rate, or formatting issues

## **Files Included**

- `csv2wav.py`: The core CSV to WAV conversion script
- `README.md`: Project documentation and usage guide

## **Dependencies**

- Python 3.x
- `numpy` (install via pip)

```bash
pip install numpy
```

## **Command Line Usage**

```bash
python3 csv2wav.py input.csv [output.wav] [--rate <hz>] [--in_format <type>]
```

- `input.csv`: CSV file exported from Primary Frequency Regulation Monitor
- `output.wav`: Optional name for output WAV file
- `--rate`: Optional sample rate in Hz (default: 44100)
- `--in_format`: Input format (`float32`, `int16`, `int24`, `int32`, `uint8`)

> [!TIP]  
> For advanced resampling, route audio through a [virtual loopback device](https://existential.audio/blackhole/) and set the target frequency with [Primary Frequency Regulation Monitor](http://localhost:4567). Then run `csv2wav.py` to export at the new frequency. This trick often delivers some of the cleanest downsampling results you'll hear, perfect for a slowed & reverb style mixes.  

## **Troubleshooting**

- **WAV plays silence**: Check if your CSV has actual audio values or just zeros
- **Distortion**: Input values may exceed [-1, 1]. Use proper normalization or rescaling
- **Wrong channel order**: Ensure your audio source is correctly mapped in Primary Frequency Regulation Monitor
