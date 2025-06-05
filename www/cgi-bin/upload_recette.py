#!/usr/bin/env python3

import cgi
import os
import html

# ✅ Récupérer le dossier d’upload depuis la variable d’environnement (Webserv doit l’injecter)
UPLOAD_DIR = os.environ.get("UPLOAD_DIR", "../uploads")  # fallback si non défini

def save_file(fileitem):
    if fileitem.filename:
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

    # Sécuriser le nom de fichier
    safe_titre = "".join(c for c in titre if c.isalnum() or c in (' ', '_')).rstrip()
    filename = f"recette_{safe_titre}.txt"
    filepath = os.path.join(UPLOAD_DIR, filename)

    # ✅ S'assurer que le dossier d'upload existe
    os.makedirs(UPLOAD_DIR, exist_ok=True)

    # Écrire la recette texte
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(f"Titre: {titre}\n")
        f.write(f"Auteur: {auteur}\n")
        f.write("Recette:\n")
        f.write(recette)

    # Gérer l'image si elle existe
    image_file = form["image"] if "image" in form else None
    image_filename = save_file(image_file) if image_file else None

    # Réponse HTML
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