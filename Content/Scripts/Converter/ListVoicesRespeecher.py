import os
import requests

def login(url, email, password):
    login_endpoint = f"{url}/api/login"
    data = {
        "email": email,
        "password": password
    }
    response = requests.post(login_endpoint, json=data)
    if response.status_code == 200:
        return response.cookies['session_id'], response.json()['csrf_token']
    else:
        raise Exception("Login failed")

def get_voices(url, csrf_token, session_id):
    voices_endpoint = f"{url}/api/v2/voices"
    headers = {"X-Csrf-Token": csrf_token}
    cookies = {"session_id": session_id}
    params = {
        "limit": 100,
        "species": "human",
        "gender": "male",
    }
    response = requests.get(voices_endpoint, headers=headers, cookies=cookies, params=params)
    if response.status_code == 200:
        return response.json()['list']
    else:
        raise Exception("Failed to get voices")

def main():
    url = "https://gateway.respeecher.com"
    email = os.getenv('RESPEECHER_EMAIL')
    password = os.getenv('RESPEECHER_PASSWORD')

    session_id, csrf_token = login(url, email, password)
    voices = get_voices(url, csrf_token, session_id)
    for voice in voices:
        #print(f"Voice ID: {voice['id']}, Name: {voice['name']}, Gender: {voice.get('gender', 'N/A')}, Age Group: {voice.get('age_group', 'N/A')}")
        #print(f"Voice ID: {voice['id']}, Name: {voice['name']}, Gender: {voice.get('gender', 'N/A')}, Age Group: {voice.get('age_group', 'N/A')}")
        print(voice['name'] + ": " + voice['id'])

if __name__ == "__main__":
    main()
