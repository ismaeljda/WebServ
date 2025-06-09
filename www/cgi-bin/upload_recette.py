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
    print("Content-Type: text/html; charset=UTF-8")
    print()  # Ligne vide obligatoire
    
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
    
    print(f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <title>Recette enregistrée</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; background-color: #f5f5f5; }}
        .container {{ max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }}
        .recipe-image {{ max-width: 400px; height: auto; border-radius: 8px; margin: 20px 0; border: 2px solid #ddd; }}
        .btn {{ background-color: #4CAF50; color: white; padding: 12px 24px; text-decoration: none; border-radius: 5px; margin: 10px 5px; display: inline-block; }}
        .btn:hover {{ background-color: #45a049; }}
        .recipe-text {{ background: #f9f9f9; padding: 15px; border-radius: 5px; margin: 15px 0; white-space: pre-wrap; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🎉 Recette Enregistrée !</h1>
        <h2>Merci {html.escape(auteur)} !</h2>
        <p>Votre recette <strong>"{html.escape(titre)}"</strong> a bien été enregistrée.</p>
        
        {f'<div><h3>📸 Image de votre recette :</h3><img src="/uploads/{html.escape(image_filename)}" alt="Image de la recette" class="recipe-image"></div>' if image_filename else ''}
        
        <div>
            <h3>📝 Votre recette :</h3>
            <div class="recipe-text">{html.escape(recette)}</div>
        </div>
        
        <div style="text-align: center; margin-top: 30px;">
            <a href="/index.html" class="btn">🏠 Retour à l'accueil</a>
            <a href="/a-propos.html" class="btn">ℹ️ À Propos</a>
        </div>
        
        <hr style="margin: 30px 0;">
        <div style="font-size: 12px; color: #666;">
            <p>📁 Fichier sauvegardé : {html.escape(filepath)}</p>
            {f'<p>🖼️ Image : {html.escape(image_filename)}</p>' if image_filename else ''}
        </div>
    </div>
</body>
</html>""")

if __name__ == "__main__":  # ✅ CORRIGÉ
    main()