from elevenlabs.client import ElevenLabs

def main():
    api_key = os.getenv("ELEVEN_API_KEY")
    client = ElevenLabs(api_key=api_key)
    
    for next in client.voices.get_all().voices:
        print("Name : %s, Id: %s" % (next.name, next.voice_id)) 

if __name__ == "__main__":
    main()
