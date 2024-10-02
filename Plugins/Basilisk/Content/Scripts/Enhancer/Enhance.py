#!/usr/bin/env python3

import os
import argparse
import torch
import torchaudio
from resemble_enhance.enhancer.inference import denoise, enhance

# Set the current working directory to the directory where the script is located
abspath = os.path.abspath(__file__)
dname = os.path.dirname(abspath)
os.chdir(dname)

if torch.cuda.is_available():
    device = "cuda"
else:
    device = "cpu"

def main(input_file_path, output_file_path):
    solver = "euler" # midpoint, rk4, euler
    num_function_evaluations = int(128) # 0 to 128 
    denoiser_strength = 0.1
    temperature = 0.6
    
    # load the audio
    dwav, sr = torchaudio.load(input_file_path)
    
    # Up the gain
    transform = torchaudio.transforms.Vol(gain=4, gain_type="db")
    dwav = transform(dwav)
    
    dwav = dwav.mean(dim=0)
    
    # Enhance should denoise automatically, no explicit denoise needed
    # out_wav, new_sr = denoise(dwav, sr, device)
    
    out_wav, new_sr = enhance(dwav, sr, device, nfe=num_function_evaluations, solver=solver, lambd=denoiser_strength, tau=temperature)

    out_wav = out_wav.cpu().numpy()
    
    # todo: write out_wav to output_file_path
    out_wav_tensor = torch.tensor(out_wav).float()

    if out_wav_tensor.dim() == 1:
        out_wav_tensor = out_wav_tensor.unsqueeze(0)

    torchaudio.save(output_file_path, out_wav_tensor, new_sr)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Separate vocals from an audio file.')
    parser.add_argument('input_file_path', type=str, help='Path to the input WAV file.')
    parser.add_argument('output_path', type=str, help='Path where the output files should be saved.')

    args = parser.parse_args()
    main(args.input_file_path, args.output_path)
