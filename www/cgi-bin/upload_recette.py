#!/usr/bin/env python3

import cgi
import os
import html

UPLOAD_DIR = "../uploads"  # adapte le chemin en fonction de ton serveur

def save_file(fileitem):
    if fileitem.filename:
        # Nettoyer le nom de fichier
        filename = os.path.basename(fileitem.filename)
        filepath = os.path.join(UPLOAD_DIR, filename)
        with open(filepath, 'wb') as f:
            f.write(fileitem.file.read())
        return filename
    return None

def main():
    print("Content-Type: text/html\n")
    form = cgi.FieldStorage()

    titre = form.getfirst("titre", "").strip()
    auteur = form.getfirst("auteur", "").strip()
    recette = form.getfirst("recette", "").strip()

    # Sauvegarde du texte
    safe_titre = "".join(c for c in titre if c.isalnum() or c in (' ', '_')).rstrip()
    filename = f"recette_{safe_titre}.txt"
    filepath = os.path.join(UPLOAD_DIR, filename)

    if not os.path.exists(UPLOAD_DIR):
        os.makedirs(UPLOAD_DIR)

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"Titre: {titre}\n")
        f.write(f"Auteur: {auteur}\n")
        f.write("Recette:\n")
        f.write(recette)

    # Sauvegarde de l'image si présente
    image_file = form["image"] if "image" in form else None
    image_filename = save_file(image_file) if image_file else None

    # Affichage HTML
    print(f"""
    <html>
    <head><title>Recette enregistrée</title></head>
    <body>
      <h2>Merci {html.escape(auteur)} !</h2>
      <p>Votre recette <strong>{html.escape(titre)}</strong> a bien été enregistrée.</p>
      {"<p>Image uploadée : " + html.escape(image_filename) + "</p>" if image_filename else ""}
      <p><a href="/a-propos.html">Retour à la page À Propos</a></p>
    </body>
    </html>
    """)

if __name__ == "__main__":
    main()
