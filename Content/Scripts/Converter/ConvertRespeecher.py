import argparse
import os
import re
import requests
import shutil
import string
import time
import uuid
import wave
import zipfile
from io import BytesIO

# Set the current working directory to the directory where the script is located
abspath = os.path.abspath(__file__)
dname = os.path.dirname(abspath)
os.chdir(dname)

voice_id_map = {
    "Aaron": "5ffd69e8-daf0-4fc0-89c5-2830b15c52a2",
    "Gregory": "9d146b18-6373-492c-bc41-d6a188836623",
    "Fletcher": "1e691f05-5cc1-42f2-9a51-5f4ace9e35db",
    "Vincent": "7b3d6e68-8057-4d7c-a9c1-7face6cc740d",
    "Kai": "13389aed-7b59-4223-ad7e-ebebe7f59a5f",
    "Orest": "1cb7cacf-dc8c-47b0-9ff6-dbb4a13d9d5f",
    "Alastair": "de93e4a8-d4b4-4acf-ab9b-c89e0f193252",
    "Askold": "2f6cfb1a-5b15-4cea-bde9-60d946c4eb9c",
    "Barnes": "c4352504-7b43-4c63-9d61-b5bc5c6109c2",
    "Bilbo": "eed680af-a822-46f2-8258-213e4681f224",
    "Bohdan": "0c6d1536-965a-42ac-8a11-e4bb95a60e25",
    "Cedric": "5f7d1de8-5634-4c91-8a8b-705d77c3eae5",
    "David": "6b7b6a20-9c5f-403e-91b3-65a9073ca5c5",
    "Declan": "87cd44c4-0638-4713-9363-0442f0e7cf65",
    "Dmytro": "d2633291-438c-4639-aa94-b3e02c528d28",
    "Hugo": "9bf59ee4-be61-49eb-8afd-771795a4931e",
    "Ivar": "33d70ef0-64ec-4239-bc93-841a34f9e4f7",
    "Jax": "818da616-75cd-4249-8ffb-ce7380ee0b19",
    "Jovan": "807979cc-a7f8-49a8-9382-70a858a26530",
    "Lavrin": "12641c60-956b-437e-af3f-5ed53ece66d9",
    "Liszt": "e14d527d-9b86-4e73-b273-a29d1b52220d",
    "Mannix": "652d4fc5-b68d-4f1d-903b-e4c3dec2ef19",
    "Nestor": "3c3e1fd4-876b-4b91-9cc3-d3ba8af19685",
    "Olaf": "52629a6f-fa97-4e72-9966-9617836f79a7",
    "Panas": "bec02644-2732-4c21-9773-398ee2793fe8",
    "Paul": "5cc03ca5-05c6-464b-98c9-4f6edfb2211b",
    "Peregrin": "81769516-c228-40c4-94dd-b65c75d3e990",
    "Pylyp": "c3389f63-6d93-499a-824c-182aa6667a45",
    "Ritter": "dbed8fee-808f-4e58-b1b4-9a13a375e252",
    "Roland": "f074a656-5d20-44e6-9292-aeb36b5e6b02",
    "Roman": "bb53a3bf-870f-4323-ba0b-32dbcac0f933",
    "Serge": "7afad338-4e81-42f2-80eb-ee278237d5ba",
    "Spencer": "a8acc3cd-7244-4022-8a8e-e6864b315469",
    "Tarleton": "cc5a9ae5-a751-4028-bd96-67734c8ccc26",
    "Trevor": "26a66489-8fbe-4316-9cef-0292fb0cbc7b",
    "Tristan": "16348ea1-2563-46a6-96d2-9db5629ba339",
    "Uhum": "4a6fb4f7-c25a-4751-af4f-eff2c2d4307f",
    "Uriah": "464fe2b5-4a42-485f-be84-c06bbc7cf978",
    "Vadym": "4e1dfdb8-a52c-4aae-bad2-fb471dfda2dc",
    "Vigil": "83f670fb-cb45-49fa-bafd-96071172bf61",
    "Viktor": "a4eed28b-ea55-431c-88d2-da5e083a5c9c",
    "Winston": "b25db1be-ddc7-4e3a-8c55-155d2b1a5a3e",
    "Xavier": "85ea7530-acf4-4751-9d18-2face25555f2",
    "Yulian": "031413e0-e818-45cd-8af8-a4c98a2166ed",
    "Zahar": "4d4b0ed5-d34b-456f-a5c0-13074179e4fb",
    "Zane": "be5cf579-c504-4d5b-82f3-57c16665d94a",

    "Constance": "1f0d99b6-a08f-4f2e-80d8-8c00bc837629",
    "Lorelei": "90ee548b-1544-4d33-8836-ff829a427ef4",
    "Alice": "78e29195-7ab3-46ab-9d72-1ecfee9ab076",
    "Samantha": "3364af14-5ead-4b1a-af57-5f24fded88d2",
    "Marta": "8664d197-635d-48bf-9a37-e528c1c18e5a",
    "Kamila": "036aedf4-1e93-4638-be4b-beaa03eedf55",
    "Neve": "c868e326-e668-40b0-974c-59221e11d819",
    "Ada": "bdc57b97-5771-4066-898a-7ab75ec9b3e3",
    "Agatha": "a2c4468a-b6e6-48c8-9685-9f7daf3ff0ef",
    "Beatriz": "be2d282b-89dd-480c-ad7e-74304a3e7e8a",
    "Beverly": "e5e2c24c-241a-4840-a5e2-ed87ca5e04fa",
    "Claudia": "28f139e5-25ca-4516-bd03-0e32095e6e87",
    "Daryna": "7aee8e81-29c1-4038-9d47-63fc178894cd",
    "Dominika": "3c8ada0d-3f69-4743-b7ec-9d8a25620c52",
    "Elaine": "595580fc-023a-4a43-80cf-1fd7d8ec5d2c",
    "Eness": "15bd2bcd-7fe2-4be7-89c5-57a10b888103",
    "Erika": "5612de31-3b1d-4982-8989-d29c3c970f0f",
    "Flora": "1e6cb51b-1043-4c2f-8201-ce5968188279",
    "Gervor": "db1af5b4-e07a-4f13-bdcf-33a0b39fee66",
    "Halyna": "20341f94-3ba8-4052-9a2b-9823f4b0d872",
    "Ilaria": "d602e50b-42f1-4a91-a9cb-000884311564",
    "Imogen": "1ea43c0f-fe98-4137-afb3-cddb0aedc519",
    "Irene": "da15eddf-f8f9-4634-8049-d553e4dbc58e",
    "Iryna": "362269d8-d4d8-4246-85b0-447ccdac250f",
    "Iskra": "fb33975e-c6fc-47e0-a689-8eac529622c2",
    "Kira": "fff81b7e-16e5-4a1d-b3ed-db4122e0b896",
    "Laura": "f812c129-0497-4126-a558-037a372052cf",
    "Maeve": "c377f5a8-c022-41f7-b3d3-de1dd7f2b041",
    "Mary": "2695c013-f7b0-4018-b6a9-f0ec64437b89",
    "Noelle": "92d966ca-e0b1-4814-a0da-d0ca984cf571",
    "Norah": "5fca8f1d-6ddf-4735-9b80-5ac06e3088a6",
    "Odarka": "d7ab3cee-d8a2-48b0-8498-103ca913ab02",
    "Oddleif": "e78b8017-1680-4fb6-91f4-eac824284b00",
    "Oksana": "1a1beed0-d80e-4b1c-b93d-7a8a50232f01",
    "Olena": "82ba1860-6b36-482d-9731-6574fce16d2b",
    "Olivia": "1ed934ed-d8ee-4f3b-aa19-c0afb3697336",
    "Orysya": "aedc4b2c-6063-4431-ba59-41f9a8eb8d35",
    "Qwynn": "a0026e92-8ccd-4da7-bc60-946d928bfd0d",
    "Ramona": "c5574ae2-f68e-4a9a-a59a-b7f8138936f9",
    "Roksolana": "d44a7f38-1318-4fdd-b6b0-623e4cfd5e8f",
    "Salome": "a66883d1-e2a0-4181-8052-6a97342afa7b",
    "Sara": "90027efe-c987-40ff-a8ff-d62e85b08d6d",
    "Soledad": "a95559c0-3891-4ef1-b5c1-7cdb4e19c71a",
    "Theresa": "814ce0d7-4c21-4d5d-a9a7-a6e99a7b5bd0",
    "Uma": "60de729f-999f-41d8-bd21-26176633d735",
    "Veronika": "a70e6ae5-205b-4f2f-a9a4-5ae7c92fc6d0",
    "Vesna": "e79367ce-95e3-4b4e-8e26-898b68c71f10",
    "Victoria": "d1d0dead-6d7a-495f-a5ca-cf300d297026",
    "Vira": "da5e811c-e6ff-4179-aa0b-851ab40cb3dd",
    "Warda": "4c1b3a33-a2b7-46e4-aa65-e33e46b36015",
    "Whitney": "2d766558-1bcd-44d6-a262-6951b4c2518e",
    "Yana": "c850d395-6cc1-45f8-af9e-6d48246d7d28",
    "Yasna": "b15eb2b7-98f9-4cc9-8cfa-dba5197ef578",
    "Yolanda": "6773d2d3-5c11-413d-a88d-54efdc7f7f7f",
    "Zadie": "bd801b1f-e443-462c-a745-e17920175ea8",
    "Zlata": "9e04d28b-202f-4608-9152-b8109b4bac5e",
}

def login(url, email, password):
    login_endpoint = f"{url}/api/login"
    data = {
        "email": email,
        "password": password,
        # "session_duration": 60,
    }
    response = requests.post(login_endpoint, json=data)
    if response.status_code == 200:
        return response.cookies['session_id'], response.json()['csrf_token']
    else:
        raise Exception("Login failed: " + response.text)

def find_or_create_project(url, project_name, csrf_token, session_id, api_key):
    project_list_endpoint = f"{url}/api/projects"
    headers = {"X-Csrf-Token": csrf_token}#, "api-key": api_key}
    cookies = {"session_id": session_id}
    response = requests.get(project_list_endpoint, headers=headers, cookies=cookies)
    
    if response.status_code == 200:
        for project in response.json()['list']:
            if project['name'] == project_name:
                return project['id']
        
        # If project not found, create a new one
        data = {"name": project_name, "owner": response.json()['user']['id']}
        project_response = requests.post(project_list_endpoint, json=data, headers=headers, cookies=cookies)
        if project_response.status_code == 200:
            return project_response.json()['id']
        else:
            raise Exception("Failed to find or create project")
    else:
        raise Exception("Failed to get project status: " + project_name + ": " + response.text)

def create_folder(url, project_id, folder_name, csrf_token, session_id, api_key):
    create_folder_endpoint = f"{url}/api/folders"
    headers = {"X-Csrf-Token": csrf_token}#, "api-key": api_key}
    cookies = {"session_id": session_id}
    data = {
        "name": folder_name,
        "project_id": project_id
    }
    response = requests.post(create_folder_endpoint, json=data, headers=headers, cookies=cookies)
    
    if response.status_code == 200:
        return response.json()['id']  # Assuming the folder creation response includes the folder ID
    else:
        raise Exception("Failed to create folder: " + folder_name + ": " + response.text)

def sanitize_filename(filename):
    # Trim leading nonsense
    sanitized = re.sub(r'^[^a-z]+', '', filename.lower().replace("_", " "))
    # Only keep characters that are lowercase letters, numbers
    sanitized = re.sub(r'[^a-z0-9]', '', sanitized)
    # Limiting name length, make it unique enough to avoid a conflict
    return sanitized[:32] + " " + str(uuid.uuid1())[:7]

def submit_file(url, folder_id, file_path, csrf_token, session_id, api_key):
    recordings_endpoint = f"{url}/api/recordings"
    headers = {"X-Csrf-Token": csrf_token}#, "api-key": api_key}
    cookies = {"session_id": session_id}
    files = {'data': open(file_path, 'rb')}
    data = {'parent_folder_id': folder_id, 'microphone': 'file'}
    response = requests.post(recordings_endpoint, files=files, data=data, headers=headers, cookies=cookies)
    if response.status_code == 200:
        return response.json()['id']
    else:
        raise Exception("Failed to submit file")

def convert_file(url, original_id, voice_id, pitch_correction, csrf_token, session_id, api_key):
    conversion_order_endpoint = f"{url}/api/v2/orders"
    headers = {"X-Csrf-Token": csrf_token}#, "api-key": api_key}
    cookies = {"session_id": session_id}
    data = {
        "original_id": original_id,
        "conversions": [
            {
                "voice_id": voice_id,
                "semitones_correction": pitch_correction
            }
        ]
    }
    response = requests.post(conversion_order_endpoint, json=data, headers=headers, cookies=cookies)
    if response.status_code == 200:
        result = response.json()[0]
        resultId = result['id']
        userId = result['user_id']
        trackingId = result['tracking_id']
        conversion_id = result['conversion_id']
        
        return resultId, userId, trackingId, conversion_id
    else:
        raise Exception("Conversion failed")

def retry_conversion(url, recording_id, csrf_token, session_id, api_key):
    retry_endpoint = f"{url}/api/v2/orders/retry/{recording_id}"
    headers = {"X-Csrf-Token": csrf_token}#, "api-key": api_key}
    cookies = {"session_id": session_id}
    
    response = requests.post(retry_endpoint, headers=headers, cookies=cookies)
    if response.status_code == 200:
        print("Retry successful, conversion re-initiated.")
        return True
    else:
        print(f"Retry failed for {recording_id}, status code: {response.status_code}, response: {response.text}")
        return False

def check_conversion_status(url, recording_id, csrf_token, session_id, api_key):
    status_endpoint = f"{url}/api/recordings/{recording_id}"
    headers = {"X-Csrf-Token": csrf_token}#, "api-key": api_key}
    cookies = {"session_id": session_id}
    
    back_off = 1
    attempts = 12

    while True:
        if attempts <= 0:
            return False, ""
        
        attempts -= 1
        response = requests.get(status_endpoint, headers=headers, cookies=cookies)
        
        if response.status_code == 200:
            status = response.json().get('state')
            if status == 'done':
                print("Conversion completed.")
                print(response.json())
                return True, response.json().get('id')
            elif status == 'processing' or status == 'postprocessing':
                print("Conversion in progress, waiting...")
            elif (status == 'failed' or status == 'error'):
                print(f"Conversion failed for {recording_id}, attempting retry...")
                if not retry_conversion(url, recording_id, csrf_token, session_id, api_key):
                    return False, ""
            else:
                print(f"Conversion failed or in an unknown state for {recording_id}: {status}")
        else:
            print(f"Failed to check conversion status, status code: {response.status_code}")
        
        # Wait the backoff time before next attempt
        time.sleep(back_off)
        back_off = min(back_off * 2, 5)  # Avoid excessively long wait times
    
    print("Timed out waiting for conversion...")
    return False, ""
    
def download_waveform_zip(url, original_id, csrf_token, session_id, api_key, output_path):
    # Define the endpoint for exporting the project to a ZIP archive
    export_endpoint = f"{url}/api/recordings/export"
    headers = {"X-Csrf-Token": csrf_token}#, "api-key": api_key}
    cookies = {"session_id": session_id}
    params = {"original_id": original_id}
    
    print("downloading...")
    
    # Make the request to download the ZIP archive
    response = requests.get(export_endpoint, headers=headers, cookies=cookies, params=params, stream=True)

    if response.status_code == 200:
        # create a temp folder for the output
        unzip_path = os.path.join(dname, "temp")
        os.makedirs(os.path.dirname(unzip_path), exist_ok=True)
    
        # Open the ZIP file from the response bytes
        with zipfile.ZipFile(BytesIO(response.content)) as thezip:
            wav_files_info = []
            for f in thezip.namelist():
                if f.endswith('.wav'):
                    # Use BytesIO to read the file in memory
                    with thezip.open(f) as file:
                        with wave.open(BytesIO(file.read()), 'rb') as wav_file:
                            channels = wav_file.getnchannels()
                            file_size = thezip.getinfo(f).file_size
                            wav_files_info.append((f, channels, file_size))
            
            # Filter to find the file(s) with the smallest number of channels
            min_channels = min(wav_files_info, key=lambda item: item[1])[1]
            smallest_channel_wavs = [f for f in wav_files_info if f[1] == min_channels]
            
            # Select the smallest file by size among those with the smallest number of channels
            smallest_wav_file, _, _ = min(smallest_channel_wavs, key=lambda item: item[2])
            
            # Extract the selected WAV file to the specified output directory
            thezip.extract(smallest_wav_file, path=unzip_path)
            print(f"Extracted {smallest_wav_file} to {unzip_path}")
            
            # Copy to the target directory
            shutil.copy2(os.path.join(unzip_path, smallest_wav_file), output_path)
            # It's safer to remove the directory after ensuring the copy operation is successful
            shutil.rmtree(unzip_path)
            print(f"Successfully processed {smallest_wav_file}.")
    else:
        print(f"Failed to download ZIP archive, status code: {response.status_code}")
            
def download_waveform(url, waveform_url, csrf_token, session_id, api_key, output_path):
    export_endpoint = f"{url}/api/recordings/export"
    headers = {"X-Csrf-Token": csrf_token}
    cookies = {"session_id": session_id}
    data = {
        "original_id": waveform_url,
    }
    # headers = {"api-key": api_key}
    # cookies = { }
    
    print(waveform_url)
    print("downloading... " + export_endpoint)
    print(csrf_token)
    print(os.getenv('RESPEECHER_API_KEY'))
    print(session_id)

    # Make the request to download the wav file
    response = requests.get(export_endpoint, headers=headers, cookies=cookies, params=data)
    
    if response.status_code == 200:
        with open(output_path, mode="wb") as file:
            file.write(response.content)
        print("Respeecher audio conversion success")
    else:
        raise Exception(f"Failed to download waveform: {response.text}, status code: {response.status_code}")

def main(file_path, output_path, target_voice, pitch_correction):
    url = "https://gateway.respeecher.com"
    email = os.getenv('RESPEECHER_EMAIL')
    password = os.getenv('RESPEECHER_PASSWORD')
    api_key = os.getenv('RESPEECHER_API_KEY')
    project_name = "Resonance"
    
    if not email:
        raise Exception("Missing RESPEECHER_EMAIL environment variable")
    if not password:
        raise Exception("Missing RESPEECHER_PASSWORD environment variable")
    
    if target_voice in voice_id_map:
        target_voice_id = voice_id_map[target_voice]
    else:
        raise Exception("missing voice id for " + target_voice)

    session_id, csrf_token = login(url, email, password)
    project_id = find_or_create_project(url, project_name, csrf_token, session_id, api_key)
    base_file_name = os.path.splitext(os.path.basename(file_path))[0]
    folder_name = sanitize_filename(base_file_name)
    folder_id = create_folder(url, project_id, folder_name, csrf_token, session_id, api_key)
    original_id = submit_file(url, folder_id, file_path, csrf_token, session_id, api_key)
    resultId, userId, trackingId, conversion_id = convert_file(url, original_id, target_voice_id, pitch_correction, csrf_token, session_id, api_key)
    if conversion_id:
        i = 5
        isDone, waveform_url = check_conversion_status(url, conversion_id, csrf_token, session_id, api_key)
        if isDone:
            # download_waveform(url, waveform_url, csrf_token, session_id, api_key, output_path)
            download_waveform_zip(url, waveform_url, csrf_token, session_id, api_key, output_path)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert a WAV file to a target voice using Respeecher API")
    parser.add_argument("file_path", type=str, help="Path to the WAV file")
    parser.add_argument("output_path", type=str, help="Path to the output WAV file")
    parser.add_argument("target_voice", type=str, help="Name of the target voice")
    parser.add_argument("pitch_correction", type=float, help="Pitch correction value")

    args = parser.parse_args()

    main(args.file_path, args.output_path, args.target_voice, args.pitch_correction)
