#!/usr/bin/env python3
import sys
import datetime
from pathlib import Path

LOG_DIR = Path(__file__).parent / "logs"
LOG_FILE = LOG_DIR / "session.log"

def log_action(message: str):
    LOG_DIR.mkdir(exist_ok=True, parents=True)
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    entry = f"[{timestamp}] {message}\n"
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(entry)
    print(f"Logged: {entry.strip()}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        msg = " ".join(sys.argv[1:])
        log_action(msg)
    else:
        print("Usage: python3 log_action.py 'Your action description'")
