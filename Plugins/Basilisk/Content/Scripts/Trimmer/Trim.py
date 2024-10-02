import argparse
import whisper
from pydub import AudioSegment

def trim_audio(input_file, output_file, leading_padding_ms=100, trailing_padding_ms=333):
    model = whisper.load_model("base")
    audio = whisper.load_audio(input_file)
    result = model.transcribe(audio, word_timestamps=True)

    start_trim_time = None
    # Initialize end_trim_time to capture the end of the last desired word
    end_trim_time = None
    key_words = ["inter", "stop", "record"]
    found_keywords = []

    for segment in result.get("segments", []):
        # Determine start time (first word's start time minus leading padding)
        if start_trim_time is None:
            start_trim_time = segment["start"]
            
        for word_info in segment["words"]:
            word_text = word_info["word"].lower()

            if key_words and key_words[0] in word_text:
                # If the keyword is found, use the end time of the previous word as end_trim_time
                # Adjust this logic to find the last word before the first keyword
                found_keywords.append(word_text)
                key_words.pop(0)

                if len(key_words) == 0:
                    # Use the end time of the last word before keywords for trimming
                    break
            else:
                # Update end_trim_time to the end of the current word if it's not a keyword
                end_trim_time = word_info["end"]

    if start_trim_time is not None:
        start_trim_time = max(start_trim_time - leading_padding_ms / 1000.0, 0)
    else:
        start_trim_time = 0.0
    if end_trim_time is not None:
        # Add trailing padding to the end_trim_time
        end_trim_time += trailing_padding_ms / 1000.0

    original_audio = AudioSegment.from_file(input_file)
    if end_trim_time is None:
        # If keywords are not found, use the end of the audio
        end_trim_time = len(original_audio) / 1000.0

    start_trim_ms = int(start_trim_time * 1000)
    end_trim_ms = int(end_trim_time * 1000)

    trimmed_audio = original_audio[start_trim_ms:end_trim_ms]
    trimmed_audio.export(output_file, format="wav")
    print(f"Audio trimmed successfully and saved to {output_file}.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Trim audio file based on Whisper timestamps and specific phrases, adjusting for silence.")
    parser.add_argument("input_file", type=str, help="Path to the input audio file")
    parser.add_argument("output_file", type=str, help="Path to the output trimmed audio file")
    args = parser.parse_args()

    trim_audio(args.input_file, args.output_file)
