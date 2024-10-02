import sys
import requests
import unreal

if __name__ == "__main__":
    url = 'http://localhost:5000/everything'
    
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    model_type = sys.argv[3]
    pitch_correction = float(sys.argv[4])
    model_name = sys.argv[5]
    accent = ""

    if (len(sys.argv) >= 7):
        accent = sys.argv[6]
        unreal.log("Accent: " + sys.argv[6])
    else:
        unreal.log("Accent: No voice accent provided, going with default \"\" voice accent")
    
    unreal.log("Input Path: " + input_path)
    unreal.log("Output Path: " + output_path)
    unreal.log("Model Type: " + model_type)
    unreal.log("Pitch Correction: " + str(pitch_correction))
    unreal.log("Model Name: " + model_name)
    unreal.log("Accent: " + accent)

    if (input_path == output_path):
        unreal.log("Input and Output paths are the exactly the same, please provide different paths")
        sys.exit(1)

    # Open the file in binary mode
    with open(input_path, 'rb') as f:
        files = {'file': (input_path, f, 'audio/wav')}
        data = {'model_type': model_type, 'target_voice': model_name, 'accent': accent, 'pitch_correction': pitch_correction}
        # POST the file and the data to the server
        response = requests.post(url, files=files, data=data)

        # Check the response status code
        if response.status_code == 200:
            # Assuming the server sends back the converted audio file
            with open(output_path, 'wb') as out_file:
                out_file.write(response.content)
            print("Voice converted audio saved as ", output_path)
        else:
            print(f"Failed to convert audio: {response.status_code} - {response.text}")
