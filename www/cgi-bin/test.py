#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/html")
print("Status: 200 OK")
print()  # Fin des headers HTTP

# Lire le body depuis stdin (nécessaire pour POST)
length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(length)

print("<html><body>")
print("<h1>CGI Python 🎉</h1>")
print("<p>Contenu POST reçu :</p>")
print(f"<pre>{body}</pre>")
print("</body></html>")
