import os
import requests
import json
import sys
import unreal

def remove_useless_information(response_json):
    # Load the JSON string into a Python dictionary
    data = response_json

    # Create a new dictionary to store the start time and word for each segment
    new_data = {'segments': []}

    # Iterate over each segment in the segments list
    for segment in data['segments']:
        # Create a new segment dictionary
        new_segment = {'words': []}

        # Iterate over each word in the words list of the current segment
        for word in segment['words']:
            # Retrieve the start time and word associated with the current word
            start_time = word['start']
            word_text = word['word']

            # Add the start time and word to the new segment dictionary
            new_segment['words'].append({'start': start_time, 'word': word_text})

        # Add the new segment to the new data dictionary
        new_data['segments'].append(new_segment)

    # Convert the new_data dictionary to a JSON object and return it
    return json.dumps(new_data)

def send_audio_file_to_subtitle_api(audio_file_path):
    url = 'http://localhost:5000/subtitle'
    
    unreal.log("Audio File: " + audio_file_path)
    
    with open(audio_file_path, 'rb') as f:
        files = {
            'file': f
        }
        response = requests.post(url, files=files)

    # Check if the request was successful
    if response.status_code == 200:
        # Parse the response as JSON and return it
        return response.json()
    else:
        unreal.log("Request failed with status code: " + str(response.status_code))
        return None

if __name__ == "__main__":
    audio_file_path = sys.argv[1]
    output_file_path = sys.argv[2]

    response_json = send_audio_file_to_subtitle_api(audio_file_path)

    response_json = remove_useless_information(response_json)
        
    os.makedirs(os.path.dirname(output_file_path), exist_ok=True)
    
    # Save the response JSON to a file
    with open(output_file_path, 'w') as f:
        f.write(response_json)
