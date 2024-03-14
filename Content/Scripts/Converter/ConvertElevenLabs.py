import argparse
import json
import os
import requests
from elevenlabs.client import ElevenLabs

def main(file_path, output_path, target_voice, pitch_correction):
    api_key = os.getenv("ELEVEN_API_KEY")
    client = ElevenLabs(api_key=api_key)
    voice_id = target_voice # default to passed param
    
    # Convert friendly name to voice id
    for next in client.voices.get_all().voices:
        if next.name == target_voice:
            voice_id = next.voice_id 

    url = f"https://api.elevenlabs.io/v1/speech-to-speech/{voice_id}"

    model_id = "eleven_english_sts_v2"
    voice_settings = {
        "similarity_boost": 0.30,
        "stability": 0.85,
        "style": 0.85,
        "use_speaker_boost": True
    }

    # Prepare the files for the multipart request
    files = {
        'audio': (file_path, open(file_path, 'rb'), 'audio/wav'),
        'model_id': (None, model_id),
        'voice_settings': (None, json.dumps(voice_settings)),
    }

    # Prepare the headers
    headers = {
        'xi-api-key': api_key,
    }

    # Make the request
    response = requests.post(url, files=files, headers=headers)

    # Check the response
    if response.status_code == 200:
        # Assuming the response is an audio file, you can save it directly
        with open(output_path, 'wb') as out_file:
            out_file.write(response.content)
        print("Audio file has been successfully generated and saved.")
    else:
        print(f"Failed to generate audio. Status code: {response.status_code}, Response: {response.text}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert a WAV file to a target voice using Respeecher API")
    parser.add_argument("file_path", type=str, help="Path to the WAV file")
    parser.add_argument("output_path", type=str, help="Path to the output WAV file")
    parser.add_argument("target_voice", type=str, help="Name of the target voice")
    parser.add_argument("pitch_correction", type=float, help="Pitch correction value")

    args = parser.parse_args()

    main(args.file_path, args.output_path, args.target_voice, args.pitch_correction)
