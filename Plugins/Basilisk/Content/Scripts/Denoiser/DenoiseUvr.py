#!/usr/bin/env python3

import os
import argparse
import shutil
from audio_separator import Separator
from pydub import AudioSegment

# Set the current working directory to the directory where the script is located
abspath = os.path.abspath(__file__)
dname = os.path.dirname(abspath)
os.chdir(dname)

def main(input_file_path, output_file_path):

    # 1. Copy the input_file_path to a temp directory, modifying the wav to increase gain.
    temp_input_path = os.path.join(dname, "temp", os.path.basename(input_file_path))
    os.makedirs(os.path.dirname(temp_input_path), exist_ok=True)
    audio = AudioSegment.from_wav(input_file_path)
    louder_audio = audio + 8  # Increase gain (in decibels)
    louder_audio.export(temp_input_path, format="wav")

    # 2. Use the new temp file as input to the separator, and output the results to another temp location
    temp_output_path = os.path.join(dname, "temp_output")
    os.makedirs(temp_output_path, exist_ok=True)

    model_file_dir = os.path.join(dname, "Models/")
    
    separator = Separator(temp_input_path,
                          model_name="Kim_Vocal_2",
                          model_file_dir=model_file_dir,
                          output_dir=temp_output_path,
                          use_cuda=False,
                          denoise_enabled=True)
    
    primary_stem_path, _ = separator.separate()
    primary_stem_full_path = os.path.join(temp_output_path, primary_stem_path)
    
    if not os.path.exists(primary_stem_full_path):
        print(f"Error: {primary_stem_full_path} does not exist.")
        return

    # 3. Copy the primary_stem_full_path file to output_file_path
    shutil.copy2(primary_stem_full_path, output_file_path)

    # Cleanup temp directories
    shutil.rmtree(os.path.join(dname, "temp"))
    shutil.rmtree(temp_output_path)

    print(output_file_path)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Separate vocals from an audio file.')
    parser.add_argument('input_file_path', type=str, help='Path to the input WAV file.')
    parser.add_argument('output_path', type=str, help='Path where the output files should be saved.')

    args = parser.parse_args()
    main(args.input_file_path, args.output_path)
