# Optional relay

A small process on a computer in the same LAN. The Dot posts chat JSON to it. The process adds the xAI key, or forwards the body to Ollama, and streams the answer back.

The key is the environment variable `XAI_API_KEY`. It is not written to the card and not printed.

```bash
export XAI_API_KEY=...          # omit this for a local Ollama
export GROK_UPSTREAM=https://api.x.ai/v1
python3 relay.py
```

On the card:

```text
base_url=http://192.168.1.20:8787/v1
path=/chat/completions
model=grok-4.3
api_key=
```

For Ollama on that computer, set `GROK_UPSTREAM=http://127.0.0.1:11434/v1` and set `model` to a model Ollama has. Ollama's `/v1/chat/completions` speaks the same JSON this app builds.

The script is not part of the firmware build. It was not run against a live key in the scaffold. [FEASIBILITY.md](../FEASIBILITY.md) says when to use it.
