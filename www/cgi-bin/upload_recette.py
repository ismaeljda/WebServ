#!/usr/bin/env python3

import cgi
import os
import html

UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "./uploads")

def save_file(fileitem):
    if fileitem.filename:
        filename = os.path.basename(fileitem.filename)
        filepath = os.path.join(UPLOAD_DIR, filename)
        with open(filepath, 'wb') as f:
            f.write(fileitem.file.read())
        return filename
    return None

def main():
    print("Content-Type: text/html; charset=UTF-8\r\n")

    form = cgi.FieldStorage()

    titre = form.getfirst("titre", "sans titre").strip()
    auteur = form.getfirst("auteur", "inconnu").strip()
    recette = form.getfirst("recette", "").strip()

    safe_titre = "".join(c for c in titre if c.isalnum() or c in (' ', '_')).rstrip()
    filename = f"recette_{safe_titre}.txt"
    filepath = os.path.join(UPLOAD_DIR, filename)

    os.makedirs(UPLOAD_DIR, exist_ok=True)

    try:
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(f"Titre: {titre}\n")
            f.write(f"Auteur: {auteur}\n")
            f.write("Recette:\n")
            f.write(recette)
    except Exception as e:
        print("<p>Erreur lors de la sauvegarde de la recette :</p>")
        print(f"<pre>{html.escape(str(e))}</pre>")
        return

    image_filename = None
    if "image" in form:
        image_file = form["image"]
        if hasattr(image_file, 'filename') and image_file.filename:
            image_filename = save_file(image_file)

    print(f"""
    <!DOCTYPE html>
    <html lang="fr">
    <head>
      <meta charset="UTF-8">
      <title>Recette enregistrée</title>
    </head>
    <body>
      <h2>Merci {html.escape(auteur)} !</h2>
      <p>Votre recette <strong>{html.escape(titre)}</strong> a bien été enregistrée.</p>
      {"<p>Image uploadée : " + html.escape(image_filename) + "</p>" if image_filename else ""}
      <p><a href="/index.html">Retour à la page d'accueil</a></p>
      <p><a href="/a-propos.html">Retour à la page À Propos</a></p>
      <p><small>→ Fichier censé être enregistré ici : {html.escape(os.path.abspath(filepath))}</small></p>
      <p><small>→ Fichier enregistré ici : {html.escape(filepath)}</small></p>
    </body>
    </html>
    """)

if __name__ == "__main__":
    main()
